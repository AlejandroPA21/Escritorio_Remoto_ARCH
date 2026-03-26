// drivers/input/usac/syscalls_usac.c
// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/capability.h>
#include <linux/console.h>
#include <linux/screen_info.h>

static struct input_dev *usac_mouse;
static DEFINE_MUTEX(usac_lock);



static int usac_create_kbd(void)
{
    int err;

    pr_info("usac: intentando crear dispositivo de teclado virtual\n");
    usac_kbd = input_allocate_device();
    if (!usac_kbd) {
        pr_err("usac: falló la asignación de memoria para el teclado virtual\n");
        return -ENOMEM;
    }

    usac_kbd->name = "usac-virtual-keyboard";
    usac_kbd->phys = "usac/input1";
    usac_kbd->id.bustype = BUS_VIRTUAL;
    usac_kbd->id.vendor  = 0x0002;
    usac_kbd->id.product = 0x0002;
    usac_kbd->id.version = 0x0001;

    __set_bit(EV_KEY, usac_kbd->evbit);
    __set_bit(EV_SYN, usac_kbd->evbit);
    /* Permitimos todo el rango KEY_* (0..255 típicamente); libinput filtrará lo no soportado */
    bitmap_fill(usac_kbd->keybit, KEY_CNT);

    err = input_register_device(usac_kbd);
    if (err) {
        pr_err("usac: falló el registro del teclado virtual (error %d)\n", err);
        input_free_device(usac_kbd);
        usac_kbd = NULL;
        return err;
    }
    pr_info("usac: dispositivo de teclado virtual creado exitosamente\n");
    return 0;
}

static void usac_destroy_devices(void)
{
    if (usac_kbd) {
        pr_info("usac: destruyendo dispositivo de teclado virtual\n");
        input_unregister_device(usac_kbd);
        usac_kbd = NULL;
    }
}

static int __init usac_init(void)
{
    int err;

    pr_info("usac: inicializando módulo de syscalls\n");

    err = usac_create_kbd();
    if (err)
        goto fail;

    pr_info("usac: mouse y teclado virtuales registrados. ¡Listo para syscalls!\n");
    return 0;
fail:
    pr_err("usac: la inicialización falló, limpiando dispositivos\n");
    usac_destroy_devices();
    return err;
}

static void __exit usac_exit(void)
{
    usac_destroy_devices();
    pr_info("usac: módulo de syscalls terminado\n");
}

module_init(usac_init);
module_exit(usac_exit);
MODULE_DESCRIPTION("USAC syscalls (mouse/keyboard/screen) with virtual input devices");
MODULE_LICENSE("GPL");



/* 2) Simular pulsación de tecla (down + up) */
SYSCALL_DEFINE1(send_key_event, int, keycode)
{
    if (!capable(CAP_SYS_ADMIN)) {
        pr_warn("usac: intento de 'send_key_event' sin privilegios (UID: %d)\n", from_kuid_munged(current_user_ns(), current_uid()));
        return -EPERM;
    }

    if (keycode < 0 || keycode >= KEY_CNT) {
        pr_warn("usac: 'send_key_event' recibió un keycode inválido (%d)\n", keycode);
        return -EINVAL;
    }

    mutex_lock(&usac_lock);
    if (!usac_kbd) {
        pr_err("usac: 'send_key_event' falló, dispositivo de teclado no disponible\n");
        mutex_unlock(&usac_lock);
        return -ENODEV;
    }

    pr_info("usac: enviando evento de tecla para keycode %d\n", keycode);
    input_report_key(usac_kbd, keycode, 1); /* keydown */
    input_sync(usac_kbd);
    input_report_key(usac_kbd, keycode, 0); /* keyup */
    input_sync(usac_kbd);

    mutex_unlock(&usac_lock);
    return 0;
}

