import React from 'react';
import styles from './Header.module.css';

// --- INICIO DE LA MODIFICACIÓN ---
const Header = ({ userGroup, onLogout, nativeDims, pointerCoords, stats }) => {
// --- FIN DE LA MODIFICACIÓN ---
    
    return (
        <header className={styles.header}>
            <div className={styles.title}>
                Proyecto Sopes 2
            </div>
            
            <div className={styles.infoDisplay}>
                <span>Resolución: <strong>{nativeDims.width || '---'}x{nativeDims.height || '---'}</strong></span>
                <span>Puntero: <strong>({pointerCoords.x}, {pointerCoords.y})</strong></span>
                
                {/* --- INICIO DE LA MODIFICACIÓN --- */}
                {stats ? (
                    <>
                        <span>CPU: <strong>{stats.cpu_usage_percent}%</strong></span>
                        <span>RAM: <strong>{stats.ram_usage_percent}%</strong></span>
                    </>
                ) : (
                    <span>Cargando stats...</span>
                )}
                {/* --- FIN DE LA MODIFICACIÓN --- */}
            </div>

            <div className={styles.controls}>
                {/* El botón de métricas ya no es necesario */}
                <div className={styles.userInfo}>
                    Rol: <strong>{userGroup}</strong>
                </div>
                <button onClick={onLogout} className={styles.logoutButton}>
                    Cerrar Sesión
                </button>
            </div>
        </header>
    );
};

export default Header;