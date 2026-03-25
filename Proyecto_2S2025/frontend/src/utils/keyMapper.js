// Este mapa es solo una muestra. Para un proyecto completo, necesitarías un mapa mucho más grande.
// Puedes encontrar listas completas buscando "linux input event codes".
const keycodeMap = {
    // Letras
    'KeyA': 30, 'KeyB': 48, 'KeyC': 46, 'KeyD': 32, 'KeyE': 18, 'KeyF': 33,
    'KeyG': 34, 'KeyH': 35, 'KeyI': 23, 'KeyJ': 36, 'KeyK': 37, 'KeyL': 38,
    'KeyM': 50, 'KeyN': 49, 'KeyO': 24, 'KeyP': 25, 'KeyQ': 16, 'KeyR': 19,
    'KeyS': 31, 'KeyT': 20, 'KeyU': 22, 'KeyV': 47, 'KeyW': 17, 'KeyX': 45,
    'KeyY': 21, 'KeyZ': 44,
    // Números
    'Digit1': 2, 'Digit2': 3, 'Digit3': 4, 'Digit4': 5, 'Digit5': 6,
    'Digit6': 7, 'Digit7': 8, 'Digit8': 9, 'Digit9': 10, 'Digit0': 11,
    // Teclas especiales
    'Enter': 28,
    'Escape': 1,
    'Backspace': 14,
    'Tab': 15,
    'Space': 57,
    'Minus': 12,
    'Equal': 13,
    'BracketLeft': 26,
    'BracketRight': 27,
    'Backslash': 43,
    'Semicolon': 39,
    'Quote': 40,
    'Backquote': 41,
    'Comma': 51,
    'Period': 52,
    'Slash': 53,
    // Teclas de control (¡Importante! Tu syscall actual no soporta modificadores)
    // 'ControlLeft': 29, 'ShiftLeft': 42, 'AltLeft': 56, 
    // Flechas
    'ArrowUp': 103,
    'ArrowLeft': 105,
    'ArrowRight': 106,
    'ArrowDown': 108,
};

/**
 * Traduce un evento de teclado de JS al keycode de Linux.
 * @param {KeyboardEvent} event El evento del navegador.
 * @returns {number | null} El keycode de Linux o null si no se encuentra.
 */
export function jsToLinuxKeycode(event) {
    // Usamos 'event.code' porque es independiente del layout del teclado (ej. 'KeyQ' siempre es 'KeyQ')
    return keycodeMap[event.code] || null;
}