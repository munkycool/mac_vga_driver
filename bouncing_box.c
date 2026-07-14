#include "common.h"

static int dvd_x = 10, dvd_y = 10;
static int dvd_dx = 2, dvd_dy = 2;
static uint8_t dvd_color = 1;

void draw_hd_dvd(uint8_t *buffer, int x, int y, uint8_t color) {
    draw_rect(buffer, x, y, x + 3, y + 14, color);
    draw_rect(buffer, x + 4, y, x + 8, y + 1, color);
    draw_rect(buffer, x + 4, y + 13, x + 8, y + 14, color);
    draw_rect(buffer, x + 9, y + 2, x + 11, y + 12, color);
    draw_rect(buffer, x + 4, y + 4, x + 6, y + 10, 0); 
    
    for (int i = 0; i < 8; i++) {
        draw_rect(buffer, x + 13 + i, y + i * 2, x + 14 + i, y + i * 2 + 1, color);
        draw_rect(buffer, x + 27 - i, y + i * 2, x + 28 - i, y + i * 2 + 1, color);
    }
    
    int x2 = x + 32;
    draw_rect(buffer, x2, y, x2 + 3, y + 14, color);
    draw_rect(buffer, x2 + 4, y, x2 + 8, y + 1, color);
    draw_rect(buffer, x2 + 4, y + 13, x2 + 8, y + 14, color);
    draw_rect(buffer, x2 + 9, y + 2, x2 + 11, y + 12, color);
    draw_rect(buffer, x2 + 4, y + 4, x2 + 6, y + 10, 0); 
    
    for (int ex = -24; ex <= 24; ex++) {
        int val = 9216 - 16 * ex * ex;
        int ey = 0;
        if (val >= 0) {
            ey = isqrt(val) / 24;
        }
        draw_pixel(buffer, x + 22 + ex, y + 19 + ey, color);
        draw_pixel(buffer, x + 22 + ex, y + 19 - ey, color);
    }
}

void init_bouncing_box() {
    dvd_x = 10;
    dvd_y = 10;
    dvd_dx = 2;
    dvd_dy = 2;
    dvd_color = 1;
}

void play_dvd(uint8_t *buffer) {
    clear_screen(buffer);
    for(int x = 0; x < 320; x++) {
        draw_pixel(buffer, x, 0, 7); 
        draw_pixel(buffer, x, 239, 7); 
    }
    for(int y = 0; y < 240; y++) {
        draw_pixel(buffer, 0, y, 7); 
        draw_pixel(buffer, 319, y, 7); 
    }
    draw_hd_dvd(buffer, dvd_x, dvd_y, dvd_color);
}

void tick_bouncing_box() {
    dvd_x += dvd_dx; 
    dvd_y += dvd_dy;
    if (dvd_x <= 1 || dvd_x >= 320 - 57) { dvd_dx = -dvd_dx; dvd_color = (dvd_color % 7) + 1; }
    if (dvd_y <= 1 || dvd_y >= 240 - 25) { dvd_dy = -dvd_dy; dvd_color = (dvd_color % 7) + 1; }
}
