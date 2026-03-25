// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/capability.h>
#include <linux/input-event-codes.h>

static struct input_dev *usac_mouse;
static DEFINE_MUTEX(usac_mouse_lock);

/* =======================================================
 * CREACIÓN DEL DISPOSITIVO VIRTUAL DE MOUSE
 * ======================================================= */

static int usac_create_mouse(void)
{
    int err;
    const int MAX_WIDTH = 1155;
    const int MAX_HEIGHT = 608;

    pr_info("usac: creando dispositivo virtual de mouse\n");
    usac_mouse = input_allocate_device();
    if (!usac_mouse)
        return -ENOMEM;

    usac_mouse->name = "USAC Virtual Mouse";
    usac_mouse->id.bustype = BUS_USB;

    /* Tipos de eventos soportados */
    __set_bit(EV_ABS, usac_mouse->evbit);
    __set_bit(EV_KEY, usac_mouse->evbit);
    __set_bit(EV_SYN, usac_mouse->evbit);

    /* Movimiento absoluto */
    __set_bit(ABS_X, usac_mouse->absbit);
    __set_bit(ABS_Y, usac_mouse->absbit);
    input_set_abs_params(usac_mouse, ABS_X, 0, MAX_WIDTH, 0, 0);
    input_set_abs_params(usac_mouse, ABS_Y, 0, MAX_HEIGHT, 0, 0);

    /* Botones */
    __set_bit(BTN_LEFT, usac_mouse->keybit);
    __set_bit(BTN_RIGHT, usac_mouse->keybit);
    __set_bit(BTN_MIDDLE, usac_mouse->keybit);

    err = input_register_device(usac_mouse);
    if (err) {
        input_free_device(usac_mouse);
        usac_mouse = NULL;
        return err;
    }

    return 0;
}

/* =======================================================
 * INICIALIZACIÓN Y DESTRUCCIÓN DEL MÓDULO
 * ======================================================= */

static int __init usac_mouse_init(void)
{
    pr_info("usac: inicializando módulo de mouse\n");
    return usac_create_mouse();
}

static void __exit usac_mouse_exit(void)
{
    pr_info("usac: destruyendo dispositivo virtual de mouse\n");
    if (usac_mouse)
        input_unregister_device(usac_mouse);
}

module_init(usac_mouse_init);
module_exit(usac_mouse_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Syscalls de Mouse para USACLinux");

/* =======================================================
 * SYSCALLS DE MOUSE
 * ======================================================= */

/* Mover el mouse a coordenadas absolutas */
SYSCALL_DEFINE2(move_mouse_absolute, int, x, int, y)
{
    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    mutex_lock(&usac_mouse_lock);
    if (!usac_mouse) {
        mutex_unlock(&usac_mouse_lock);
        return -ENODEV;
    }

    // --- INICIO DEL CAMBIO ---
    // En lugar de usar las funciones 'helper', enviaremos los eventos manualmente.
    // Esto a veces puede resolver problemas de contexto sutiles en daemons.
    
    // 1. Reporta el nuevo valor para el eje X
    input_event(usac_mouse, EV_ABS, ABS_X, x);
    
    // 2. Reporta el nuevo valor para el eje Y
    input_event(usac_mouse, EV_ABS, ABS_Y, y);
    
    // 3. Envía un evento de "sincronización" para decirle al sistema
    //    que hemos terminado de enviar este paquete de eventos (X e Y).
    //    Esto es lo que hace 'input_sync' por debajo.
    input_event(usac_mouse, EV_SYN, SYN_REPORT, 0);
    // --- FIN DEL CAMBIO ---

    mutex_unlock(&usac_mouse_lock);
    return 0;
}

/* Simular clic de mouse (presionar/soltar) */
SYSCALL_DEFINE2(send_mouse_button, int, button_code, int, state)
{
    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    if (button_code != BTN_LEFT && button_code != BTN_RIGHT && button_code != BTN_MIDDLE)
        return -EINVAL;

    if (state < 0 || state > 1)
        return -EINVAL;

    mutex_lock(&usac_mouse_lock);
    if (!usac_mouse) {
        mutex_unlock(&usac_mouse_lock);
        return -ENODEV;
    }

    input_report_key(usac_mouse, button_code, state);
    input_sync(usac_mouse);

    mutex_unlock(&usac_mouse_lock);
    return 0;
}
