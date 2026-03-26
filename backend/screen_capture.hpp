#ifndef SCREEN_CAPTURE_HPP
#define SCREEN_CAPTURE_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <linux/types.h>

// Las estructuras de tu syscall, adaptadas para C++
struct ScreenFormat {
    __u32 pitch;
    __u32 bpp;
};

struct ScreenCaptureInfo {
    __u64 buffer_user;
    __u64 buffer_len;
    __u32 width;
    __u32 height;
    ScreenFormat format;
    __u64 required_bytes;
    __u32 flags;
    __u32 region_x, region_y, region_w, region_h;
    __u64 reserved;
};

class ScreenCapturer {
public:
    ScreenCapturer();
    ~ScreenCapturer();

    // Obtiene los metadatos de la pantalla (ancho, alto, etc.)
    bool getScreenInfo();

    // Captura un frame y lo guarda en nuestro buffer interno
    bool captureFrame();

    // Permite acceder a los datos crudos del frame capturado
    const std::vector<uint8_t>& getFrameBuffer() const;
    
    // Getters para la información de la pantalla
    uint32_t getWidth() const { return info.width; }
    uint32_t getHeight() const { return info.height; }
    uint32_t getBpp() const { return info.format.bpp; }

private:
    ScreenCaptureInfo info;
    std::vector<uint8_t> frameBuffer;
    bool info_loaded = false;
};

#endif // SCREEN_CAPTURE_HPP