#ifndef JPEG_CONVERTER_HPP
#define JPEG_CONVERTER_HPP

#include <vector>
#include <cstdint>

class JpegConverter {
public:
    // Comprime un buffer de píxeles BGRA/RGBA a formato JPEG en memoria
    static std::vector<unsigned char> compress(
        const std::vector<uint8_t>& pixel_buffer,
        int width,
        int height,
        int quality = 75 // Calidad del JPEG (0-100)
    );
};

#endif // JPEG_CONVERTER_HPP