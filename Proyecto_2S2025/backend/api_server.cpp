#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

// Librerías de Pistache
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/http.h>
#include <pistache/mime.h>

// Librería para JSON
#include <nlohmann/json.hpp>

// Nuestros módulos personalizados
#include "screen_capture.hpp"
#include "jpeg_converter.hpp"

#include "syscall_defs.hpp"

// Módulo de autenticación en C
extern "C" {
    #include "auth_logic.h"
}

// Usamos los namespaces para código más limpio
using namespace Pistache;
using namespace Pistache::Http;
using namespace Pistache::Rest;
using json = nlohmann::json;

// --- FUNCIÓN DE AYUDA PARA CORS ---
// Añade las cabeceras necesarias para que el frontend pueda comunicarse con el backend
void addCorsHeaders(Http::ResponseWriter& response) {
    response.headers().add<Header::AccessControlAllowOrigin>("*");
    response.headers().add<Header::AccessControlAllowMethods>("POST, GET, OPTIONS");
    response.headers().add<Header::AccessControlAllowHeaders>("Content-Type");
}

long direct_syscall_move(long syscall_number, int x, int y) {
    long result;
    __asm__ volatile (
        "syscall"
        : "=a" (result) // Salida: el resultado se guardará en la variable 'result' (registro rax)
        : "a" (syscall_number), // Entrada 1: el número de la syscall (registro rax)
          "D" (x),            // Entrada 2: el primer argumento (registro rdi)
          "S" (y)             // Entrada 3: el segundo argumento (registro rsi)
        : "rcx", "r11", "memory" // Registros que la instrucción syscall puede modificar
    );
    return result;
}


// --- MANEJADOR DE AUTENTICACIÓN ---
// Esta clase se encarga de la lógica de login
class AuthHandler {
public:
    // Responde a la petición "pre-vuelo" (preflight) del navegador
    static void handleOptions(const Rest::Request&, Http::ResponseWriter response) {
        addCorsHeaders(response);
        response.send(Http::Code::Ok);
    }
    
    // Procesa la petición de login con usuario y contraseña
    static void handleAuth(const Rest::Request& request, Http::ResponseWriter response) {
        addCorsHeaders(response);

        if (request.method() != Http::Method::Post) {
            response.send(Http::Code::Method_Not_Allowed, "Solo POST es permitido.");
            return;
        }

        Http::Code code = Http::Code::Ok;
        std::string status_message;
        std::string user_group;

        try {
            auto json_data = json::parse(request.body());
            std::string username = json_data.at("username").get<std::string>();
            std::string password = json_data.at("password").get<std::string>();
            
            int auth_result = authenticate_and_check_groups_c(username.c_str(), password.c_str());

            if (auth_result == 2) {
                user_group = "remote_control";
                status_message = "Autenticación exitosa.";
            } else if (auth_result == 1) {
                user_group = "remote_view";
                status_message = "Autenticación exitosa.";
            } else {
                code = Http::Code::Unauthorized;
                status_message = "Credenciales inválidas o grupo incorrecto.";
                user_group = "denied";
            }
        } catch (const json::exception& e) {
            code = Http::Code::Bad_Request;
            status_message = std::string("JSON inválido: ") + e.what();
            user_group = "error";
        } catch (const std::exception& e) {
            code = Http::Code::Internal_Server_Error;
            status_message = std::string("Error interno: ") + e.what();
            user_group = "error";
        }

        json response_json = {
            {"status", (code == Http::Code::Ok ? "success" : "error")},
            {"message", status_message},
            {"group", user_group}
        };

        auto mime = Http::Mime::MediaType::fromString("application/json");
        response.send(code, response_json.dump(), mime);
    }
};

// --- MANEJADOR DE CAPTURA DE PANTALLA ---
// Esta clase se encarga de servir los fotogramas del escritorio
class ScreenHandler {
public:


    static void handleGetScreenInfo(const Rest::Request&, Http::ResponseWriter response) {
        addCorsHeaders(response);
        ScreenCapturer capturer;
        if (capturer.getScreenInfo()) {
            json info_json = {
                {"width", capturer.getWidth()},
                {"height", capturer.getHeight()}
            };
            response.send(Http::Code::Ok, info_json.dump());
        } else {
            response.send(Http::Code::Internal_Server_Error, "No se pudo obtener la información de la pantalla.");
        }
    }




    static void handleGetScreen(const Rest::Request&, Http::ResponseWriter response) {
        addCorsHeaders(response);

        // Creamos una instancia del capturador en cada petición.
        // Esto es seguro y no bloquea el servidor.
        ScreenCapturer capturer;
        
        // 1. Obtener información de la pantalla y capturar el fotograma actual
        if (!capturer.getScreenInfo() || !capturer.captureFrame()) {
            response.send(Http::Code::Internal_Server_Error, "Fallo al capturar el fotograma.");
            return;
        }

        // 2. Comprimir el fotograma crudo a formato JPEG
        std::vector<unsigned char> jpeg_data = JpegConverter::compress(
            capturer.getFrameBuffer(),
            capturer.getWidth(),
            capturer.getHeight(),
            80 // Calidad del JPEG (0-100)
        );

        // 3. Enviar la imagen JPEG como respuesta
        auto mime = Http::Mime::MediaType::fromString("image/jpeg");
        response.send(Http::Code::Ok, std::string(jpeg_data.begin(), jpeg_data.end()), mime);
    }
};



class InputHandler {
public:
    // Endpoint para mover el mouse
    static void handleMouseMove(const Rest::Request& request, Http::ResponseWriter response) {
        addCorsHeaders(response);
        try {
            auto body = json::parse(request.body());
            int x = body.at("x").get<int>();
            int y = body.at("y").get<int>();

            // --- INICIO DEL CAMBIO ---
            // En lugar de llamar a syscall(), llamamos a nuestra nueva función directa.
            long result = direct_syscall_move(__NR_move_mouse_absolute, x, y);
            // --- FIN DEL CAMBIO ---

            
            
            if (result == 0) {
                response.send(Http::Code::Ok, "Llamada directa a syscall enviada.");
            } else {
                response.send(Http::Code::Internal_Server_Error, "Llamada directa a syscall falló.");
            }

        } catch (const std::exception& e) {
            response.send(Http::Code::Bad_Request, e.what());
        }
    }
    // Endpoint para los clics del mouse
    static void handleMouseClick(const Rest::Request& request, Http::ResponseWriter response) {
        addCorsHeaders(response);
        try {
            auto body = json::parse(request.body());
            std::string button_str = body.at("button").get<std::string>();
            int state = body.at("state").get<int>();
            int button_code = 0;

            if (button_str == "left") button_code = BTN_LEFT;
            else if (button_str == "right") button_code = BTN_RIGHT;
            else if (button_str == "middle") button_code = BTN_MIDDLE;
            else throw std::runtime_error("Botón de mouse no válido");

            syscall(__NR_send_mouse_button, button_code, state);
            response.send(Http::Code::Ok);
        } catch (const std::exception& e) {
            response.send(Http::Code::Bad_Request, e.what());
        }
    }

    // Endpoint para eventos de teclado
    static void handleKeyEvent(const Rest::Request& request, Http::ResponseWriter response) {
        addCorsHeaders(response);
        try {
            auto body = json::parse(request.body());
            int keycode = body.at("keycode").get<int>();
            syscall(__NR_send_key_event, keycode);
            response.send(Http::Code::Ok);
        } catch (const std::exception& e) {
            response.send(Http::Code::Bad_Request, e.what());
        }
    }
};

// --- NUEVO: MANEJADOR DE RECURSOS (CPU Y RAM) ---
class ResourceHandler {
public:
    static void handleGetResources(const Rest::Request&, Http::ResponseWriter response) {
        addCorsHeaders(response);
        struct sys_resource_usage usage;

        if (syscall(__NR_get_system_usage, &usage) == 0) {
            json res_json = {
                {"ram_total_kb", usage.ram_total_kb},
                {"ram_free_kb", usage.ram_free_kb},
                {"ram_usage_percent", usage.ram_usage_percent},
                {"cpu_usage_percent", usage.cpu_usage_percent}
            };
            response.send(Http::Code::Ok, res_json.dump());
        } else {
            response.send(Http::Code::Internal_Server_Error, "No se pudo llamar a la syscall de recursos.");
        }
    }
};




// --- FUNCIÓN PRINCIPAL ---
// --- FUNCIÓN PRINCIPAL ---
int main() {
    Address addr(Ipv4::any(), Port(9080));
    auto opts = Http::Endpoint::options().threads(4);
    Http::Endpoint server(addr);
    server.init(opts);
    Router router;
    
    // --- Configuración de Rutas (Endpoints) ---

    // Rutas de autenticación
    Routes::Post(router, "/api/auth", Routes::bind(&AuthHandler::handleAuth));
    Routes::Options(router, "/api/auth", Routes::bind(&AuthHandler::handleOptions));

    // Rutas de captura de pantalla
    Routes::Get(router, "/api/screen/info", Routes::bind(&ScreenHandler::handleGetScreenInfo));
    Routes::Get(router, "/api/screen/frame", Routes::bind(&ScreenHandler::handleGetScreen));
    Routes::Options(router, "/api/screen/.*", Routes::bind(&AuthHandler::handleOptions));

    // --- INICIO DE LA CORRECCIÓN ---
    // Rutas de Input (versión explícita para evitar problemas)
    Routes::Post(router, "/api/mouse/move", Routes::bind(&InputHandler::handleMouseMove));
    Routes::Options(router, "/api/mouse/move", Routes::bind(&AuthHandler::handleOptions)); // <-- AÑADIDA

    Routes::Post(router, "/api/mouse/click", Routes::bind(&InputHandler::handleMouseClick));
    Routes::Options(router, "/api/mouse/click", Routes::bind(&AuthHandler::handleOptions)); // <-- AÑADIDA

    Routes::Post(router, "/api/keyboard/key", Routes::bind(&InputHandler::handleKeyEvent));
    Routes::Options(router, "/api/keyboard/key", Routes::bind(&AuthHandler::handleOptions)); // <-- AÑADIDA
    // --- FIN DE LA CORRECCIÓN ---

    // Ruta de recursos (si ya la tienes)
    Routes::Get(router, "/api/resources", Routes::bind(&ResourceHandler::handleGetResources));
    Routes::Options(router, "/api/resources", Routes::bind(&AuthHandler::handleOptions));

    server.setHandler(router.handler());
    std::cout << "Servidor iniciado en http://localhost:9080" << std::endl;
    server.serve();
}