import React, { useState } from 'react';
import Login from '../components/Login/Login'; // Importamos el formulario
import authService from '../services/authService'; // Importamos el servicio

// Esta página recibe una función 'onLoginSuccess' para avisar a App.jsx
const LoginPage = ({ onLoginSuccess }) => {
    const [errorMessage, setErrorMessage] = useState('');

    const handleLogin = async (username, password) => {
        const result = await authService.login(username, password);
        if (result.success) {
            onLoginSuccess(); // Avisamos que el login fue exitoso
        } else {
            setErrorMessage(result.message); // Mostramos el error
        }
    };

    return <Login onLogin={handleLogin} errorMessage={errorMessage} />;
};

export default LoginPage;