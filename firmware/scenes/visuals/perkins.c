#include "common.h"
#include <math.h>

void init_perkins() {
    // No initialization state needed
}

void play_perkins(uint8_t *buf, int frame) {
    // 1. Draw sky (Dark navy blue night sky gradient)
    for (int y = 0; y < 220; y++) {
        for (int x = 0; x < 320; x++) {
            int d = y / 20; 
            if (d > 7) d = 7;
            int t = art_bayer4_at(x, y);
            uint8_t col = (d >= t) ? 4 : 0; // Blue (4) to Black (0)
            draw_pixel(buf, x, y, col);
        }
    }
    
    // Twinkling stars in sky
    for (int i = 0; i < 15; i++) {
        int sx = (i * 23 + 17) % 320;
        int sy = (i * 13 + 11) % 100;
        int phase = (frame + i * 8) % 40;
        if (phase < 20) {
            draw_pixel(buf, sx, sy, 7); // white star
            if (phase % 4 == 0) {
                draw_pixel(buf, sx - 1, sy, 6);
                draw_pixel(buf, sx + 1, sy, 6);
                draw_pixel(buf, sx, sy - 1, 6);
                draw_pixel(buf, sx, sy + 1, 6);
            }
        }
    }

    // 2. Draw ground (parking lot)
    for (int y = 220; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < 4) ? 7 : 0; // Dark grey dither
            draw_pixel(buf, x, y, col);
        }
    }
    // Yellow parking lines
    for (int x = 20; x < 320; x += 60) {
        draw_rect(buf, x, 225, x + 4, 240, 3); // yellow lines
    }

    // 3. Draw Perkins Building
    // Walls: beige/cream (Yellow (3) and White (7) dither)
    for (int y = 140; y < 220; y++) {
        for (int x = 30; x < 250; x++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < 8) ? 3 : 7; 
            draw_pixel(buf, x, y, col);
        }
    }

    // Foundation line
    draw_rect(buf, 30, 218, 250, 220, 0);

    // Iconic green sloped roof/awning
    for (int y = 110; y < 140; y++) {
        int x_left = 30 + (140 - y);
        int x_right = 250 - (140 - y);
        draw_rect(buf, x_left, y, x_right, y, 2); // Green (2)
        if (y == 139) {
            draw_rect(buf, x_left, y, x_right, y, 0);
        }
    }
    // Red accent stripe on the awning
    for (int x = 40; x < 240; x++) {
        draw_pixel(buf, x, 130, 1); // Red accent line
    }

    // 4. Large Storefront Windows
    // Left window
    draw_rect(buf, 50, 155, 110, 195, 0); // frame
    draw_rect(buf, 52, 157, 108, 193, 3); // warm interior glow (Yellow)
    for (int y = 180; y < 193; y++) {
        for (int x = 52; x < 108; x++) {
            if (art_bayer4_at(x, y) < 6) {
                draw_pixel(buf, x, y, 1); // Orange/Red glow dither
            }
        }
    }
    draw_rect(buf, 80, 157, 81, 193, 0);
    draw_rect(buf, 52, 175, 108, 176, 0);

    // Right window
    draw_rect(buf, 170, 155, 230, 195, 0); // frame
    draw_rect(buf, 172, 157, 228, 193, 3); // warm interior glow
    for (int y = 180; y < 193; y++) {
        for (int x = 172; x < 228; x++) {
            if (art_bayer4_at(x, y) < 6) {
                draw_pixel(buf, x, y, 1);
            }
        }
    }
    draw_rect(buf, 200, 157, 201, 193, 0);
    draw_rect(buf, 172, 175, 228, 176, 0);

    // 5. Double Glass Doors (Center)
    draw_rect(buf, 125, 160, 155, 220, 0); // frame
    draw_rect(buf, 127, 162, 139, 220, 7); // white glass glow
    draw_rect(buf, 141, 162, 153, 220, 7);
    draw_rect(buf, 138, 185, 139, 195, 3); // brass handles
    draw_rect(buf, 141, 185, 142, 195, 3);

    // 6. Bakery Case details inside windows
    draw_rect(buf, 55, 185, 75, 186, 0);
    draw_rect(buf, 55, 190, 75, 191, 0);
    draw_pixel(buf, 60, 184, 1); // red cherry pie
    draw_pixel(buf, 68, 184, 5); // berry pie
    draw_pixel(buf, 64, 189, 3); // lemon pie
    draw_pixel(buf, 72, 189, 7); // cake

    // Dining customers silhouette
    draw_rect(buf, 180, 180, 188, 193, 0);
    draw_rect(buf, 212, 182, 220, 193, 0);

    // 7. Iconic Perkins Flag Sign on a Pole
    draw_rect(buf, 275, 50, 278, 220, 7); // silver pole
    draw_rect(buf, 274, 50, 279, 52, 0);

    // Large Green Sign Board
    draw_rect(buf, 245, 60, 308, 105, 0); 
    draw_rect(buf, 247, 62, 306, 103, 2); // Green sign
    draw_rect(buf, 249, 64, 304, 65, 3);  // Gold border
    draw_rect(buf, 249, 100, 304, 101, 3);
    draw_rect(buf, 249, 64, 250, 101, 3);
    draw_rect(buf, 303, 64, 304, 101, 3);

    // Text: "Perkins" in gold letters
    draw_string(buf, "Perkins", 253, 73, 2, 3);

    // Smaller lower board: "BAKERY"
    draw_rect(buf, 252, 110, 301, 125, 0); 
    draw_rect(buf, 254, 112, 299, 123, 7); // white board
    draw_string(buf, "BAKERY", 260, 115, 1, 2); // green "BAKERY" text
}
