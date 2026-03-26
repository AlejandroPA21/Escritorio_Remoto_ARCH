cmake_minimum_required(VERSION 3.10)
project(AuthServer C CXX)

# Estándares recomendados
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Incluir directorios para encabezados (nlohmann/json y otros)
include_directories(
    /usr/include/nlohmann
)

# Archivos fuente
add_executable(auth_server
    api_server.cpp
    auth_logic.c
    screen_capture.cpp
    jpeg_converter.cpp
)

# Enlace de librerías
target_link_libraries(auth_server
    pistache     # ya instalada globalmente
    pthread
    pam
    jpeg 
)
