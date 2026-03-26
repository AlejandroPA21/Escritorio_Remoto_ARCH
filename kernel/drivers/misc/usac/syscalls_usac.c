// SPDX-License-Identifier: GPL-2.0
// USAC syscalls – Práctica 2: Hilos y sincronización
#include <linux/module.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/capability.h>
#include <linux/cred.h>

// --- Acceso a Espacio de Usuario y Memoria ---
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/iosys-map.h>

// --- Sistema de Archivos y Rutas ---
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/stat.h>

// --- Hilos y Sincronización ---
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/delay.h>

// --- Estructuras de Datos y Utilidades ---
#include <linux/list.h>
#include <linux/kfifo.h>
#include <linux/hashtable.h>
#include <linux/string.h>
#include <linux/errno.h>

// --- Subsistema de Framebuffer (fbdev) ---
#include <linux/fb.h>

// --- Subsistema de Direct Rendering Manager (DRM) ---
#include <drm/drm_device.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_connector.h>
#include <drm/drm_plane.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_modeset_lock.h>


/* ============================================================
 * 1. CAPTURA DE PANTALLA
 * ============================================================ */

extern struct fb_info *registered_fb[];
extern int num_registered_fb;

/* ============================================================
   screen_capture_info
   - flags: modo de captura
   - región: usada si el flag REGION está activo
   - buffer_user: ptr de usuario (puede ser NULL si solo metadata)
   ============================================================ */
#define SCRCAP_FLAG_META_ONLY   (1 << 0)
#define SCRCAP_FLAG_REGION      (1 << 1)
#define SCRCAP_FLAG_ROW_COPY    (1 << 2)

struct scrcap_format {
    __u32 pitch;
    __u32 bpp;
};

struct scrcap_info {
    __u64 buffer_user;
    __u64 buffer_len;
    __u32 width;
    __u32 height;
    struct scrcap_format format;
    __u64 required_bytes;
    __u32 flags;
    __u32 region_x;
    __u32 region_y;
    __u32 region_w;
    __u32 region_h;
    __u64 reserved;
};

/* ------------------------------------------------------------
   Helper: obtener drm_device desde fb0
   ------------------------------------------------------------ */
static struct drm_device *scrcap_get_drm_from_fb0(void)
{
    if (num_registered_fb <= 0 || !registered_fb[0])
        return NULL;

    struct fb_info *info = registered_fb[0];
    struct drm_fb_helper *helper = (struct drm_fb_helper *)info->par;
    return helper ? helper->dev : NULL;
}

/* ------------------------------------------------------------
   Helper: encontrar framebuffer activo
   ------------------------------------------------------------ */
static int scrcap_find_fb(struct drm_device *drm,
                          struct drm_framebuffer **out_fb,
                          u32 *out_w, u32 *out_h)
{
    struct drm_modeset_acquire_ctx ctx;
    int ret = 0;

    drm_modeset_acquire_init(&ctx, 0);

retry:
    ret = drm_modeset_lock_all_ctx(drm, &ctx);
    if (ret == -EDEADLK) {
        drm_modeset_backoff(&ctx);
        goto retry;
    }
    if (ret)
        goto end;

    struct drm_connector *conn;
    struct drm_connector_list_iter iter;
    struct drm_crtc *active_crtc = NULL;

    drm_connector_list_iter_begin(drm, &iter);
    drm_for_each_connector_iter(conn, &iter) {
        if (conn->status == connector_status_connected &&
            conn->state && conn->state->crtc) {
            active_crtc = conn->state->crtc;
            break;
        }
    }
    drm_connector_list_iter_end(&iter);

    if (!active_crtc) {
        ret = -ENODEV;
        goto unlock;
    }

    struct drm_plane *pl;
    struct drm_framebuffer *fb = NULL;

    drm_for_each_plane(pl, drm) {
        if (pl->type == DRM_PLANE_TYPE_PRIMARY &&
            pl->state && pl->state->fb &&
            pl->state->crtc == active_crtc) {
            fb = pl->state->fb;
            drm_framebuffer_get(fb);
            break;
        }
    }

    if (!fb) {
        ret = -ENODEV;
        goto unlock;
    }

    *out_w = fb->width ? fb->width : active_crtc->state->mode.hdisplay;
    *out_h = fb->height ? fb->height : active_crtc->state->mode.vdisplay;
    *out_fb = fb;

unlock:
    drm_modeset_drop_locks(&ctx);
end:
    drm_modeset_acquire_fini(&ctx);
    return ret;
}

/* ------------------------------------------------------------
   Copia fila por fila una región
   ------------------------------------------------------------ */
static int scrcap_copy_rows(const void *mapped_base,
                            u32 fb_pitch,
                            u32 bpp,
                            u32 x,
                            u32 y,
                            u32 w,
                            u32 h,
                            void __user *user_ptr)
{
    size_t row_bytes = (size_t)w * (bpp / 8);  /* CORRECCIÓN: pixeles × bytes */
    int line;

    for (line = 0; line < (int)h; ++line) {
        const u8 *src = (const u8 *)mapped_base +
                        (size_t)(y + line) * fb_pitch +
                        (size_t)x * (bpp / 8);

        void __user *dst = (void __user *)((uintptr_t)user_ptr + (size_t)line * row_bytes);

        if (copy_to_user(dst, src, row_bytes))
            return -EFAULT;
    }
    return 0;
}

/* ============================================================
   Syscall principal
   ============================================================ */
SYSCALL_DEFINE1(capture_screen, struct scrcap_info __user *, uinfo)
{
    struct scrcap_info info;
    struct drm_device *drm;
    struct drm_framebuffer *fb = NULL;
    u32 sw = 0, sh = 0;
    int ret = 0;

    if (!capable(CAP_SYS_ADMIN) && !capable(CAP_SYS_RAWIO))
        return -EPERM;

    if (copy_from_user(&info, uinfo, sizeof(info)))
        return -EFAULT;

    drm = scrcap_get_drm_from_fb0();
    if (!drm)
        return -ENODEV;

    ret = scrcap_find_fb(drm, &fb, &sw, &sh);
    if (ret)
        return ret;

    info.width = sw;
    info.height = sh;
    info.format.bpp = fb->format ? fb->format->cpp[0] * 8 : 32;
    info.format.pitch = info.width * (info.format.bpp / 8);
    info.required_bytes = (u64)info.format.pitch * (u64)info.height;

    if ((info.flags & SCRCAP_FLAG_META_ONLY) != 0) {
        if (copy_to_user(uinfo, &info, sizeof(info)))
            ret = -EFAULT;
        goto release_fb;
    }

    u32 reg_x = 0, reg_y = 0, reg_w = sw, reg_h = sh;
    if ((info.flags & SCRCAP_FLAG_REGION) != 0) {
        reg_x = info.region_x;
        reg_y = info.region_y;
        reg_w = info.region_w;
        reg_h = info.region_h;

        if (reg_w == 0 || reg_h == 0 || reg_x + reg_w > sw || reg_y + reg_h > sh) {
            ret = -EINVAL;
            goto release_fb;
        }
        info.required_bytes = (u64)reg_w * (u64)reg_h * (info.format.bpp / 8);
    }

    if (!info.buffer_user || info.buffer_len < info.required_bytes) {
        if (copy_to_user(uinfo, &info, sizeof(info)))
            ret = -EFAULT;
        ret = -ENOSPC;
        goto release_fb;
    }

    struct iosys_map map[DRM_FORMAT_MAX_PLANES];
    ret = drm_gem_fb_vmap(fb, map, NULL);
    if (ret) {
        ret = -EIO;
        goto release_fb;
    }

    void *base = map[0].vaddr + fb->offsets[0];
    if (!base) {
        ret = -EIO;
        goto unmap_fb;
    }

    if ((info.flags & SCRCAP_FLAG_ROW_COPY) != 0 || info.required_bytes > (64ULL << 20)) {
        ret = scrcap_copy_rows(base, info.format.pitch, info.format.bpp,
                               reg_x, reg_y, reg_w, reg_h,
                               (void __user *)(uintptr_t)info.buffer_user);
    } else {
        void *tmp = vmalloc(info.required_bytes);
        if (!tmp) {
            ret = -ENOMEM;
            goto unmap_fb;
        }
        u32 line;
        size_t row_bytes = (size_t)reg_w * (info.format.bpp / 8);
        for (line = 0; line < reg_h; ++line) {
            void *dst = (u8 *)tmp + (size_t)line * row_bytes;
            void *src = (u8 *)base + (size_t)(reg_y + line) * info.format.pitch + reg_x * (info.format.bpp / 8);
            memcpy(dst, src, row_bytes);
        }
        if (copy_to_user((void __user *)(uintptr_t)info.buffer_user, tmp, info.required_bytes))
            ret = -EFAULT;
        vfree(tmp);
    }

    if (copy_to_user(uinfo, &info, sizeof(info)))
        ret = -EFAULT;

unmap_fb:
    drm_gem_fb_vunmap(fb, map);
release_fb:
    drm_framebuffer_put(fb);
    return ret;
}