#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

// Evitar conflicto con PAM_SERVICE definido en /usr/include/security/_pam_types.h
#define PAM_SERVICE_NAME "login"

// Grupos del proyecto
#define GROUP_CONTROL "remotecontrol"
#define GROUP_VIEW "remoteview"

// -----------------------------------------------------------
// Declaraciones internas
// -----------------------------------------------------------

static int pam_conv_callback(int num_msg, const struct pam_message **msg,
                             struct pam_response **resp, void *appdata_ptr);

int check_group_membership_c(const char *username, const char *group_name);

// -----------------------------------------------------------
// Callback PAM
// -----------------------------------------------------------

static int pam_conv_callback(int num_msg, const struct pam_message **msg,
                             struct pam_response **resp, void *appdata_ptr) {
    if (num_msg <= 0) return PAM_CONV_ERR;

    struct pam_response *responses = calloc(num_msg, sizeof(struct pam_response));
    if (!responses) return PAM_CONV_ERR;

    const char *password = (const char *)appdata_ptr;
    //impprimir servicio pam
    fprintf(stderr, "DEBUG CALLBACK: Servicio PAM: %s\n", PAM_SERVICE_NAME);
    fprintf(stderr, "DEBUG CALLBACK: Intentando autenticar usuario con password de longitud: %zu\n", strlen(password)); // Imprime longitud

    
    for (int i = 0; i < num_msg; i++) {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF) {
            fprintf(stderr, "DEBUG CALLBACK: Recibido PROMPT_ECHO_OFF. Inyectando password.\n"); 
            responses[i].resp = strdup(password);

        } else if (msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
            responses[i].resp = NULL;
        } else {
            free(responses);
            return PAM_CONV_ERR;
        }
    }

    *resp = responses;
    return PAM_SUCCESS;
}

// -----------------------------------------------------------
// Verificación de grupos
// -----------------------------------------------------------

int check_group_membership_c(const char *username, const char *group_name) {
    struct passwd *pw = getpwnam(username);

    if (pw == NULL) {
        fprintf(stderr, "DEBUG GRUPOS: No se encontró el usuario %s.\n", username);
        return 0;
    }


    int ngroups = 0;
    gid_t *groups = NULL;

    getgrouplist(username, pw->pw_gid, NULL, &ngroups);

    if (ngroups <= 0) {
        fprintf(stderr, "DEBUG GRUPOS: El usuario %s no pertenece a ningún grupo (o error al obtener la lista).\n", username);
        return 0;
    }

    groups = malloc(ngroups * sizeof(gid_t));
    if (!groups) return 0;

    getgrouplist(username, pw->pw_gid, groups, &ngroups);

    int found = 0;
    fprintf(stderr, "DEBUG GRUPOS: Grupos encontrados para %s (%d grupos):\n", username, ngroups);
    for (int i = 0; i < ngroups; i++) {
        struct group *gr = getgrgid(groups[i]);
        if (gr && strcmp(gr->gr_name, group_name) == 0) {
            found = 1;
            break;
        }
    }

    free(groups);
    if (found == 0) {
        fprintf(stderr, "DEBUG GRUPOS: El usuario %s NO pertenece al grupo de destino (%s).\n", username, group_name);
    }
    return found;
}

// -----------------------------------------------------------
// Función principal expuesta al C++
// -----------------------------------------------------------

int authenticate_and_check_groups_c(const char *username, const char *password) {
    int pam_result;
    pam_handle_t *pam_handler = NULL;

    struct pam_conv conversation = {pam_conv_callback, (void *)password};
    pam_result = pam_start(PAM_SERVICE_NAME, username, &conversation, &pam_handler);

    if (pam_result != PAM_SUCCESS) {
        return 0;
    }

    pam_result = pam_authenticate(pam_handler, 0);
    if (pam_result == PAM_SUCCESS) {
        pam_result = pam_acct_mgmt(pam_handler, 0);
    }

    if (pam_result != PAM_SUCCESS) {
        const char *error_msg = pam_strerror(pam_handler, pam_result);
        fprintf(stderr, ">>> FALLO CRITICO PAM: Usuario %s, Codigo PAM: %d, Mensaje: %s\n", username, pam_result, error_msg);
        pam_end(pam_handler, pam_result);
        return 0;
    }

    int is_control = check_group_membership_c(username, GROUP_CONTROL);
    int is_view = check_group_membership_c(username, GROUP_VIEW);

    pam_end(pam_handler, pam_result);

    fprintf(stderr, "DEBUG AUTH: User=%s, Control=%d, View=%d\n", username, is_control, is_view);

    if (is_control) return 2;
    else if (is_view) return 1;
    else return 0;
}
