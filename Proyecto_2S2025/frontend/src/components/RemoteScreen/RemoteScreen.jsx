import React, { useState, useEffect, useRef } from 'react';
import styles from './RemoteScreen.module.css';
import inputService from '../../services/inputService';
import { jsToLinuxKeycode } from '../../utils/keyMapper';

const RemoteScreen = ({ nativeDims, onPointerMove, userGroup }) => {
    const [imageUrl, setImageUrl] = useState('');
    const [error, setError] = useState('');
    const containerRef = useRef(null);
    const imageRef = useRef(null);
    const intervalRef = useRef(null);
    const isControlEnabled = userGroup === 'remote_control';

    // Para no saturar la red, limitamos los eventos de movimiento
    const lastMoveTime = useRef(0);
    const MOUSE_MOVE_THROTTLE = 50; // Envía una actualización de movimiento cada 50ms como máximo

    // Efecto para el streaming de la imagen
    useEffect(() => {
        const fetchFrame = async () => {
            try {
                const response = await fetch('http://192.168.163.128:9080/api/screen/frame');
                if (!response.ok) throw new Error(`Error del servidor: ${response.status}`);
                const imageBlob = await response.blob();
                const newImageUrl = URL.createObjectURL(imageBlob);
                if (imageUrl) URL.revokeObjectURL(imageUrl);
                setImageUrl(newImageUrl);
                if (error) setError('');
            } catch (err) {
                console.error("Fallo al obtener el fotograma:", err);
                setError('Error de conexión con el servidor remoto.');
                if (intervalRef.current) clearInterval(intervalRef.current);
            }
        };
        intervalRef.current = setInterval(fetchFrame, 1000);
        return () => {
            if (intervalRef.current) clearInterval(intervalRef.current);
            if (imageUrl) URL.revokeObjectURL(imageUrl);
        };
    }, [imageUrl]);

    // Función que calcula las coordenadas remotas a partir de un evento del mouse
    const calculateRemoteCoords = (event) => {
        if (!imageRef.current || !nativeDims || nativeDims.width === 0) return null;
        const rect = imageRef.current.getBoundingClientRect();
        const mouseX = event.clientX - rect.left;
        const mouseY = event.clientY - rect.top;
        const remoteX = Math.round((mouseX / rect.width) * nativeDims.width);
        const remoteY = Math.round((mouseY / rect.height) * nativeDims.height);
        if (remoteX >= 0 && remoteX <= nativeDims.width && remoteY >= 0 && remoteY <= nativeDims.height) {
            return { x: remoteX, y: remoteY };
        }
        return null;
    };

    // Manejador para el movimiento del mouse
    const handleMouseMove = (event) => {
        const coords = calculateRemoteCoords(event);
        if (coords) {
            onPointerMove(coords); // Actualiza la UI para mostrar las coordenadas
            if (!isControlEnabled) return;

            const now = Date.now();
            if (now - lastMoveTime.current > MOUSE_MOVE_THROTTLE) {
                inputService.sendMouseMove(coords.x, coords.y);
                lastMoveTime.current = now;
            }
        } else {
            onPointerMove({ x: '---', y: '---' });
        }
    };

    // Manejador para los botones del mouse (presionar y soltar)
    const handleMouseButton = (event, state) => {

        if (containerRef.current) {
            containerRef.current.focus();
        }

        if (!isControlEnabled) return;
        
        event.preventDefault();

        const buttonMap = { 0: 'left', 1: 'middle', 2: 'right' };
        const button = buttonMap[event.button];
        
        if (button) {
            inputService.sendMouseClick(button, state);
        }
    };

    const handleKeyDown = (event) => {
        // --- ¡AÑADE ESTA LÍNEA! ---
        console.log('--- ¡onKeyDown SE DISPARÓ! ---', event.code);
        // -------------------------

        if (!isControlEnabled) return; 
        event.preventDefault();

        const keycode = jsToLinuxKeycode(event);
        
        if (keycode) {
            console.log(`Keycode encontrado: ${keycode}. Enviando...`); // Añadamos este también
            inputService.sendKeyEvent(keycode);
        } else {
            console.warn(`Tecla no mapeada: ${event.code}`);
        }
    };

    return (
        <div 
            ref={containerRef} // <-- AÑADE ESTA LÍNEA
            className={styles.screenContainer} 
            onMouseMove={handleMouseMove}
            onMouseLeave={() => onPointerMove({ x: '---', y: '---' })}
            onMouseDown={(e) => handleMouseButton(e, 1)} // 1 = presionar
            onMouseUp={(e) => handleMouseButton(e, 0)}   // 0 = soltar
            onContextMenu={(e) => e.preventDefault()}  // Previene el menú contextual del navegador
            // --- INICIO DE LA MODIFICACIÓN ---
            onKeyDown={handleKeyDown}
            // Hacemos que el div sea "focusable" para que pueda recibir eventos de teclado.
            // Es crucial hacer clic en la pantalla una vez para que esto se active.
            tabIndex="0" 
            // --- FIN DE LA MODIFICACIÓN ---
        >
            {error ? (
                <p className={styles.error}>{error}</p>
            ) : imageUrl ? (
                <img 
                    ref={imageRef} 
                    src={imageUrl} 
                    alt="Escritorio Remoto" 
                    className={styles.screenImage}
                    onDragStart={(e) => e.preventDefault()} 
                />
            ) : (
                <p className={styles.placeholder}>Iniciando transmisión...</p>
            )}
        </div>
    );
};

export default RemoteScreen;