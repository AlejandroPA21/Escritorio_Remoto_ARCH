import React, { useState } from 'react';
import styles from './Login.module.css'; // Importamos los nuevos estilos

const Login = ({ onLogin, errorMessage }) => {
    const [username, setUsername] = useState('');
    const [password, setPassword] = useState('');

    const handleSubmit = (event) => {
        event.preventDefault();
        onLogin(username, password);
    };

    return (
        <div className={styles.loginContainer}>
            <div className={styles.header}>
                <h2>Acceso Remoto</h2>
                <h1>USACLinux</h1>
            </div>

            <form onSubmit={handleSubmit} className={styles.form}>
                <input
                    className={styles.input}
                    type="text"
                    value={username}
                    onChange={(e) => setUsername(e.target.value)}
                    placeholder="Usuario"
                    required
                />
                <input
                    className={styles.input}
                    type="password"
                    value={password}
                    onChange={(e) => setPassword(e.target.value)}
                    placeholder="Contraseña"
                    required
                />
                <button type="submit" className={styles.button}>Ingresar</button>
            </form>
            
            {errorMessage && <p className={styles.errorMessage}>{errorMessage}</p>}
        </div>
    );
};

export default Login;