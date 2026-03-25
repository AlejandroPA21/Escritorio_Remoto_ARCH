#include "jpeg_converter.hpp"

#include <cstdio> // 1. Añade este encabezado para definir el tipo 'FILE'

#include <jpeglib.h>
#include <stdexcept>
#include <iostream>
#include <vector>

// La syscall entrega los píxeles en formato BGRA (o RGBA). JPEG necesita RGB.
// Esta función convierte una fila de píxeles al formato correcto.
void bgra_to_rgb(const uint8_t* src_row, uint8_t* dest_row, int width) {
    for (int i = 0; i < width; ++i) {
        dest_row[i * 3 + 0] = src_row[i * 4 + 2]; // R
        dest_row[i * 3 + 1] = src_row[i * 4 + 1]; // G
        dest_row[i * 3 + 2] = src_row[i * 4 + 0]; // B
    }
}

std::vector<unsigned char> JpegConverter::compress(
    const std::vector<uint8_t>& pixel_buffer, int width, int height, int quality) {
    
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char *mem_buffer = nullptr;
    unsigned long mem_size = 0;
    jpeg_mem_dest(&cinfo, &mem_buffer, &mem_size);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3; // RGB
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    std::vector<uint8_t> row_rgb(width * 3);
    const uint8_t* all_pixels = pixel_buffer.data();

    while (cinfo.next_scanline < cinfo.image_height) {
        const uint8_t* src_row = all_pixels + cinfo.next_scanline * (width * 4); // Asumimos 4 bytes/píxel (BGRA)
        bgra_to_rgb(src_row, row_rgb.data(), width);
        JSAMPROW row_pointer = row_rgb.data();
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    
    std::vector<unsigned char> jpeg_data(mem_buffer, mem_buffer + mem_size);

    jpeg_destroy_compress(&cinfo);
    free(mem_buffer);

    return jpeg_data;
}