const API_URL = 'http://192.168.163.128:9080/api'; // La IP de tu VM

const inputService = {
    /**
     * Envía las coordenadas del movimiento del mouse al backend.
     * @param {number} x La coordenada X en la pantalla remota.
     * @param {number} y La coordenada Y en la pantalla remota.
     */
    sendMouseMove: async (x, y) => {
        try {
            await fetch(`${API_URL}/mouse/move`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ x, y }),
            });
        } catch (error) {
            console.error('Error al enviar movimiento de mouse:', error);
        }
    },

    /**
     * Envía un evento de clic de mouse al backend.
     * @param {'left' | 'right' | 'middle'} button El botón que se presionó.
     * @param {0 | 1} state 1 para presionar (down), 0 para soltar (up).
     */
    sendMouseClick: async (button, state) => {
        try {
            await fetch(`${API_URL}/mouse/click`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ button, state }),
            });
        } catch (error) {
            console.error('Error al enviar clic de mouse:', error);
        }
    },

    /**
     * Envía un evento de tecla (pulsación) al backend.
     * @param {number} keycode El keycode de Linux.
     */
    sendKeyEvent: async (keycode) => {
        try {
            await fetch(`${API_URL}/keyboard/key`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                // Recuerda que tu backend solo espera el keycode
                body: JSON.stringify({ keycode }), 
            });
            console.log(`Enviando evento de tecla: ${keycode}`);
        } catch (error) {
            console.error('Error al enviar evento de tecla:', error);
        }
    },
};

export default inputService;