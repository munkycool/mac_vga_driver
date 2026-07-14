#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

// Stub for getchar_timeout_us (needed by common.c)
int getchar_timeout_us(uint32_t timeout_us) {
    (void)timeout_us;
    return -1;
}

// Stub for time_us_64
uint64_t time_us_64(void) {
    return 0;
}

static uint8_t palette_r[8] = { 0, 255,  52, 255,   0, 255,  90, 255 };
static uint8_t palette_g[8] = { 0,  59, 199, 204, 122,  45, 200, 255 };
static uint8_t palette_b[8] = { 0,  48,  89,   0, 255,  85, 250, 255 };

void save_bmp(const char *filename, uint8_t *buffer) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("Failed to open output file");
        return;
    }

    uint32_t width = 320;
    uint32_t height = 240;
    uint32_t row_size = (width * 3 + 3) & ~3;
    uint32_t pixel_data_size = row_size * height;
    uint32_t file_size = 54 + pixel_data_size;

    uint8_t header[54] = {
        'B', 'M',
        file_size & 0xFF, (file_size >> 8) & 0xFF, (file_size >> 16) & 0xFF, (file_size >> 24) & 0xFF,
        0, 0, 0, 0,
        54, 0, 0, 0, // offset to pixel data
        40, 0, 0, 0, // info header size
        width & 0xFF, (width >> 8) & 0xFF, (width >> 16) & 0xFF, (width >> 24) & 0xFF,
        height & 0xFF, (height >> 8) & 0xFF, (height >> 16) & 0xFF, (height >> 24) & 0xFF,
        1, 0,       // planes
        24, 0,      // bits per pixel
        0, 0, 0, 0, // compression
        0, 0, 0, 0, // image size
        0, 0, 0, 0, // x pixels per meter
        0, 0, 0, 0, // y pixels per meter
        0, 0, 0, 0, // colors used
        0, 0, 0, 0  // important colors
    };

    fwrite(header, 1, 54, f);

    uint8_t *row_buf = calloc(row_size, 1);
    // BMP is bottom-up
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < (int)width; x++) {
            uint8_t pixel = buffer[y * width + x];
            uint8_t color_idx = pixel & 0x07;
            row_buf[x * 3 + 0] = palette_b[color_idx]; // B
            row_buf[x * 3 + 1] = palette_g[color_idx]; // G
            row_buf[x * 3 + 2] = palette_r[color_idx]; // R
        }
        fwrite(row_buf, 1, row_size, f);
    }
    free(row_buf);
    fclose(f);
}

int main() {
    uint8_t *buffer = malloc(320 * 240);
    if (!buffer) return 1;
    memset(buffer, 0, 320 * 240);

    init_perkins();
    // Render frame 0
    play_perkins(buffer, 0);

    save_bmp("/home/familyp/.gemini/antigravity/brain/0d74834b-e3e4-4ce8-8ced-151970452c41/perkins.bmp", buffer);
    printf("Saved perkins.bmp successfully!\n");

    free(buffer);
    return 0;
}
