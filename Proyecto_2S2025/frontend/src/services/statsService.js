const API_URL = 'http://192.168.163.128:9080/api'; // La IP de tu VM

const statsService = {
    /**
     * Busca las estadísticas de CPU y RAM del servidor.
     * @returns {Promise<object>} Un objeto con los datos de uso del sistema.
     */
    fetchStats: async () => {
        try {
            const response = await fetch(`${API_URL}/resources`);
            if (!response.ok) {
                throw new Error('Error al obtener las estadísticas del servidor');
            }
            const data = await response.json();
            return data;
        } catch (error) {
            console.error('Error en el servicio de estadísticas:', error);
            // Devolvemos un objeto por defecto en caso de error
            return {
                cpu_usage_percent: '??',
                ram_usage_percent: '??'
            };
        }
    },
};

export default statsService;