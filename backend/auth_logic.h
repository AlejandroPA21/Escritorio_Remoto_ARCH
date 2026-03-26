// auth_logic.h
#ifndef AUTH_LOGIC_H
#define AUTH_LOGIC_H

/**
 * @brief Función principal que realiza la autenticación PAM y verifica la pertenencia a grupos.
 * @param username Nombre de usuario.
 * @param password Contraseña.
 * @return 0 (Error/Denegado), 1 (remote_view), 2 (remote_control).
 */
int authenticate_and_check_groups_c(const char *username, const char *password);

#endif // AUTH_LOGIC_H