// Este objeto contendrá todas nuestras funciones para hablar con la API de autenticación.
const authService = {
    // La función login recibe username y password
    login: async (username, password) => {
        try {
        
            const response = await fetch('http://192.168.163.128:9080/api/auth', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ username, password }),
            });

            const data = await response.json();

            // Si la respuesta del servidor es exitosa (código 200-299)
            if (response.ok) {
                // Guardamos el grupo/rol del usuario en el sessionStorage del navegador
                sessionStorage.setItem('userGroup', data.group);
                return { success: true, group: data.group };
            } else {
                // Si hay un error (ej. 401 Unauthorized), devolvemos el mensaje
                return { success: false, message: data.message };
            }
        } catch (error) {
            // Manejamos errores de red
            console.error('Error de conexión:', error);
            return { success: false, message: 'No se pudo conectar al servidor.' };
        }
    },

    // Función para cerrar sesión
    logout: () => {
        sessionStorage.removeItem('userGroup');
    },

    // Función para verificar si hay una sesión activa
    isAuthenticated: () => {
        return sessionStorage.getItem('userGroup') !== null;
    }
};

export default authService;