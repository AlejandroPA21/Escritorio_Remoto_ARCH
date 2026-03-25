import React, { useState } from 'react';
import LoginPage from './pages/LoginPage.jsx';
import DesktopPage from './pages/DesktopPage.jsx';
import authService from './services/authService.js';
import './App.css';

function App() {
    // El estado se inicializa verificando si ya existe una sesión
    const [isAuthenticated, setIsAuthenticated] = useState(authService.isAuthenticated());

    // Esta función se llama cuando el login es exitoso
    const handleLoginSuccess = () => {
        setIsAuthenticated(true);
    };

    // --- ESTA ES LA NUEVA FUNCIÓN ---
    // Se llamará cuando el usuario haga clic en "Cerrar Sesión"
    const handleLogout = () => {
        authService.logout();      // 1. Llama al servicio para borrar la sesión
        setIsAuthenticated(false); // 2. Actualiza el estado para mostrar el login
    };

    return (
        <div className="App">
            {isAuthenticated ? (
                // Pasamos la función handleLogout a la página del escritorio
                <DesktopPage onLogout={handleLogout} />
            ) : (
                // Pasamos la función handleLoginSuccess a la página de login
                <LoginPage onLoginSuccess={handleLoginSuccess} />
            )}
        </div>
    );
}

export default App;