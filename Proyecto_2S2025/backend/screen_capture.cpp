#include "screen_capture.hpp"
#include <sys/syscall.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

#define __NR_capture_screen 551 // Tu número de syscall

// Banderas
#define SCAP_FLAG_METADATA_ONLY (1 << 0)

ScreenCapturer::ScreenCapturer() {
    // Inicializamos la estructura a ceros
    info = {0};
}

ScreenCapturer::~ScreenCapturer() {
    // El destructor se encargará de liberar la memoria del vector automáticamente
}

bool ScreenCapturer::getScreenInfo() {
    info.flags = SCAP_FLAG_METADATA_ONLY;
    long ret = syscall(__NR_capture_screen, &info);
    if (ret < 0) {
        perror("syscall(getScreenInfo)");
        return false;
    }
    if (info.required_bytes == 0 || info.width == 0 || info.height == 0) {
        std::cerr << "Error: El kernel devolvió metadatos inválidos." << std::endl;
        return false;
    }
    
    // Redimensionamos nuestro buffer para que coincida con el tamaño necesario
    frameBuffer.resize(info.required_bytes);
    info_loaded = true;
    std::cout << "Info de pantalla obtenida: " << info.width << "x" << info.height << ", " << info.required_bytes << " bytes" << std::endl;
    return true;
}

bool ScreenCapturer::captureFrame() {
    if (!info_loaded) {
        std::cerr << "Error: Debes llamar a getScreenInfo() antes de capturar un frame." << std::endl;
        return false;
    }

    info.flags = 0; // Captura de pantalla completa
    info.buffer_user = reinterpret_cast<uint64_t>(frameBuffer.data());
    info.buffer_len = frameBuffer.size();

    long ret = syscall(__NR_capture_screen, &info);
    if (ret < 0) {
        perror("syscall(captureFrame)");
        return false;
    }
    return true;
}

const std::vector<uint8_t>& ScreenCapturer::getFrameBuffer() const {
    return frameBuffer;
}