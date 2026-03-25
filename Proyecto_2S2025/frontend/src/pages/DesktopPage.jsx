import React, { useState, useEffect } from 'react';
import Header from '../components/Header/Header';
import RemoteScreen from '../components/RemoteScreen/RemoteScreen';
import styles from './DesktopPage.module.css';
import statsService from '../services/statsService'; // <-- IMPORTA EL NUEVO SERVICIO

const DesktopPage = ({ onLogout }) => {
    const userGroup = sessionStorage.getItem('userGroup');
    const [nativeDims, setNativeDims] = useState({ width: 0, height: 0 });
    const [pointerCoords, setPointerCoords] = useState({ x: 0, y: 0 });
    
    // --- INICIO DE LA MODIFICACIÓN ---
    const [stats, setStats] = useState(null); // Estado para guardar las estadísticas

    // Al cargar la página, pedimos la resolución nativa (tu código original)
    useEffect(() => {
        const fetchScreenInfo = async () => {
            try {
                const response = await fetch('http://192.168.163.128:9080/api/screen/info');
                const data = await response.json();
                setNativeDims({ width: data.width, height: data.height });
            } catch (error) {
                console.error("Error al obtener info de la pantalla:", error);
            }
        };
        fetchScreenInfo();
    }, []);

    // Nuevo efecto para pedir las estadísticas en un bucle
    useEffect(() => {
        const fetchStats = async () => {
            const statsData = await statsService.fetchStats();
            setStats(statsData);
        };
        
        fetchStats(); // Llama una vez al inicio
        const intervalId = setInterval(fetchStats, 2000); // Y luego cada 2 segundos

        return () => clearInterval(intervalId); // Limpia el intervalo al salir
    }, []);
    // --- FIN DE LA MODIFICACIÓN ---

    return (
        <div className={styles.desktopPage}>
            <Header 
                userGroup={userGroup} 
                onLogout={onLogout}
                nativeDims={nativeDims}
                pointerCoords={pointerCoords}
                stats={stats} // <-- PASA LAS ESTADÍSTICAS AL HEADER
            />
            <main className={styles.mainContent}>
                <RemoteScreen 
                    nativeDims={nativeDims}
                    onPointerMove={setPointerCoords}
                    userGroup={userGroup}
                />
            </main>
        </div>
    );
};

export default DesktopPage;