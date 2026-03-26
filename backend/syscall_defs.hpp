#ifndef SYSCALL_DEFS_HPP
#define SYSCALL_DEFS_HPP

#include <unistd.h>
#include <sys/syscall.h>

// ====================================================================
// IMPORTANTE: REEMPLAZA ESTOS NÚMEROS CON LOS DE TU TABLA DE SYSCALLS
// ====================================================================
#define __NR_move_mouse_absolute 557 // <-- REEMPLAZAR
#define __NR_send_mouse_button   558 // <-- REEMPLAZAR
#define __NR_get_system_usage    559 // <-- REEMPLAZAR
#define __NR_send_key_event      549 // <-- REEMPLAZAR (el de tu teclado)
// ====================================================================

// Estructura para la syscall de CPU/RAM
struct sys_resource_usage {
    unsigned long ram_total_kb;
    unsigned long ram_free_kb;
    unsigned int ram_usage_percent;
    unsigned int cpu_usage_percent;
};

// Códigos de los botones del mouse (de linux/input-event-codes.h)
#define BTN_LEFT   0x110
#define BTN_RIGHT  0x111
#define BTN_MIDDLE 0x112

// Declaramos las funciones C para que C++ pueda llamarlas
extern "C" {
    long move_mouse_absolute(int x, int y);
    long send_mouse_button(int button_code, int state);
    long get_system_usage(struct sys_resource_usage* info);
    long send_key_event(int keycode);
}

#endif // SYSCALL_DEFS_HPP  