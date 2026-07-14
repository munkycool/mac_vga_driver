#include "common.h"
#include <math.h>

static float text_x = 100.0f;
static float text_y = 120.0f;
static float text_dx = 1.5f;
static float text_dy = 1.2f;

void init_happy() {
    text_x = 50.0f + (get_rand() % 120);
    text_y = 100.0f + (get_rand() % 40);
    text_dx = (get_rand() % 2 == 0) ? 1.4f : -1.4f;
    text_dy = (get_rand() % 2 == 0) ? 1.0f : -1.0f;
}

static void draw_happy_cloud(uint8_t *buf, int cx, int cy) {
    if (cx < -60 || cx > 380) return;
    
    // Draw cloud puffs (White - 7)
    draw_circle(buf, cx, cy, 14, 7);
    draw_circle(buf, cx - 14, cy + 3, 10, 7);
    draw_circle(buf, cx + 14, cy + 3, 10, 7);
    draw_circle(buf, cx - 7, cy - 7, 11, 7);
    draw_circle(buf, cx + 7, cy - 7, 11, 7);
    
    // Fill interior
    draw_rect(buf, cx - 18, cy - 4, cx + 18, cy + 9, 7);

    // Smiling face on the cloud!
    // Eyes (Black - 0)
    draw_pixel(buf, cx - 5, cy, 0);
    draw_pixel(buf, cx + 5, cy, 0);
    // Rosy cheeks (Magenta/Pink - 5)
    draw_pixel(buf, cx - 9, cy + 2, 5);
    draw_pixel(buf, cx + 9, cy + 2, 5);
    // Smile (Black - 0)
    draw_pixel(buf, cx - 2, cy + 3, 0);
    draw_pixel(buf, cx, cy + 4, 0);
    draw_pixel(buf, cx + 2, cy + 3, 0);
}

static void draw_flower(uint8_t *buf, int x, int y, int frame, uint8_t petal_color) {
    float sway = sinf((float)frame * 0.05f + (float)x * 0.1f) * 5.0f;
    int stem_top_x = x + (int)sway;
    int stem_top_y = y - 18;

    // Draw green stem (Green - 2)
    int steps = 10;
    for (int s = 0; s <= steps; s++) {
        int sx = x + ((stem_top_x - x) * s) / steps;
        int sy = y + ((stem_top_y - y) * s) / steps;
        draw_pixel(buf, sx, sy, 2);
        draw_pixel(buf, sx + 1, sy, 2); // Thicker stem
    }

    // Draw leaves
    draw_pixel(buf, x + (int)(sway * 0.5f) - 3, y - 8, 2);
    draw_pixel(buf, x + (int)(sway * 0.5f) + 3, y - 6, 2);

    // Draw petals (petal_color)
    draw_circle(buf, stem_top_x - 3, stem_top_y, 3, petal_color);
    draw_circle(buf, stem_top_x + 3, stem_top_y, 3, petal_color);
    draw_circle(buf, stem_top_x, stem_top_y - 3, 3, petal_color);
    draw_circle(buf, stem_top_x, stem_top_y + 3, 3, petal_color);

    // Center (Yellow - 3)
    draw_circle(buf, stem_top_x, stem_top_y, 1, 3);
}

static void draw_butterfly(uint8_t *buf, int bx, int by, int frame, uint8_t color) {
    // Body (Black - 0)
    draw_rect(buf, bx, by - 3, bx, by + 3, 0);
    // Wings flap animation
    int wing_w = (frame % 10 < 5) ? 4 : 1;
    draw_rect(buf, bx - wing_w, by - 4, bx - 1, by - 1, color);
    draw_rect(buf, bx - wing_w, by + 1, bx - 1, by + 3, color);
    draw_rect(buf, bx + 1, by - 4, bx + wing_w, by - 1, color);
    draw_rect(buf, bx + 1, by + 1, bx + wing_w, by + 3, color);
}

void play_happy(uint8_t *buf, int frame) {
    // 1. Dithered sky (Blue 4 to Cyan 6)
    for (int y = 0; y < 160; y++) {
        int factor = (y * 16) / 160; // 0 to 15
        uint8_t *row = &buf[y * 320];
        for (int x = 0; x < 320; x++) {
            int bayer = art_bayer4_at(x, y);
            uint8_t col = (bayer < factor) ? 6 : 4;
            uint8_t pixel = col | 0x08;
            row[x] = (pixel << 4) | pixel;
        }
    }

    // 2. Glowing Smiling Sun
    int sun_cx = 260;
    int sun_cy = 45;
    float base_angle = (float)frame * 0.02f;
    for (int i = 0; i < 8; i++) {
        float angle = base_angle + i * 0.785398f; // PI / 4
        float cos_a = cosf(angle);
        float sin_a = sinf(angle);
        int x1 = sun_cx + (int)(cos_a * 21.0f);
        int y1 = sun_cy + (int)(sin_a * 21.0f);
        int x2 = sun_cx + (int)(cos_a * (28.0f + sinf((float)frame * 0.08f + i) * 3.0f));
        int y2 = sun_cy + (int)(sin_a * (28.0f + sinf((float)frame * 0.08f + i) * 3.0f));
        
        // Draw ray (Yellow - 3)
        int steps = 10;
        for (int s = 0; s <= steps; s++) {
            int rx = x1 + ((x2 - x1) * s) / steps;
            int ry = y1 + ((y2 - y1) * s) / steps;
            draw_pixel(buf, rx, ry, 3);
        }
    }
    // Sun base circle
    draw_circle(buf, sun_cx, sun_cy, 18, 3);
    
    // Sun Face
    // Eyes (Black - 0)
    draw_pixel(buf, sun_cx - 5, sun_cy - 4, 0);
    draw_pixel(buf, sun_cx + 5, sun_cy - 4, 0);
    draw_pixel(buf, sun_cx - 5, sun_cy - 3, 0);
    draw_pixel(buf, sun_cx + 5, sun_cy - 3, 0);
    // Rosy Cheeks (Red - 1)
    draw_pixel(buf, sun_cx - 9, sun_cy, 1);
    draw_pixel(buf, sun_cx + 9, sun_cy, 1);
    // Big Smile
    draw_pixel(buf, sun_cx - 4, sun_cy + 2, 0);
    draw_pixel(buf, sun_cx - 3, sun_cy + 4, 0);
    draw_pixel(buf, sun_cx - 2, sun_cy + 5, 0);
    draw_pixel(buf, sun_cx,     sun_cy + 6, 0);
    draw_pixel(buf, sun_cx + 2, sun_cy + 5, 0);
    draw_pixel(buf, sun_cx + 3, sun_cy + 4, 0);
    draw_pixel(buf, sun_cx + 4, sun_cy + 2, 0);

    // 3. Clouds
    int cx1 = (frame / 2) % 460 - 80;
    int cx2 = (frame / 3 + 220) % 460 - 80;
    draw_happy_cloud(buf, cx1, 40);
    draw_happy_cloud(buf, cx2, 70);

    // 4. Hill 1 (Background, Dark Green/Blue dither)
    for (int x = 0; x < 320; x++) {
        int hy = 142 + (int)(12.0f * sinf((float)x * 0.015f + 1.2f));
        for (int y = hy; y < 240; y++) {
            int bayer = art_bayer4_at(x, y);
            uint8_t col = (bayer < 6) ? 0 : 2; // Green/Black dither
            draw_pixel(buf, x, y, col);
        }
    }

    // 5. Hill 2 (Foreground, Bright Green/Yellow dither)
    for (int x = 0; x < 320; x++) {
        int hy = 172 + (int)(16.0f * sinf((float)x * 0.011f));
        for (int y = hy; y < 240; y++) {
            int bayer = art_bayer4_at(x, y);
            uint8_t col = (bayer < 5) ? 3 : 2; // Green/Yellow dither
            draw_pixel(buf, x, y, col);
        }
    }

    // 6. Swaying Flowers
    static const int fx[5] = {45, 105, 165, 225, 285};
    static const uint8_t fcol[5] = {1, 5, 1, 5, 1}; // Red (1) and Magenta (5)
    for (int i = 0; i < 5; i++) {
        int hy = 172 + (int)(16.0f * sinf((float)fx[i] * 0.011f));
        draw_flower(buf, fx[i], hy, frame, fcol[i]);
    }

    // 7. Fluttering Butterflies
    float b1x = 100.0f + 60.0f * sinf((float)frame * 0.015f) + 20.0f * cosf((float)frame * 0.05f);
    float b1y = 90.0f + 30.0f * cosf((float)frame * 0.011f) + 10.0f * sinf((float)frame * 0.06f);
    float b2x = 220.0f + 50.0f * sinf((float)frame * 0.018f) + 15.0f * cosf((float)frame * 0.04f);
    float b2y = 80.0f + 25.0f * cosf((float)frame * 0.013f) + 8.0f * sinf((float)frame * 0.07f);
    draw_butterfly(buf, (int)b1x, (int)b1y, frame, 5); // Pink/Magenta
    draw_butterfly(buf, (int)b2x, (int)b2y, frame, 1); // Red

    // 8. Bouncing "BE HAPPY!" Text
    text_x += text_dx;
    text_y += text_dy;

    // Bounds checking
    if (text_x <= 10.0f || text_x >= 230.0f) {
        text_dx = -text_dx;
    }
    if (text_y <= 110.0f || text_y >= 215.0f) { // Keep text on the green hills mostly or floating nicely
        text_dy = -text_dy;
    }

    // Draw bouncing text in happy changing color
    uint8_t text_color = 1 + (frame / 30) % 7; // Cycle colors 1 to 7 (exclude black)
    if (text_color == 4) text_color = 3; // Keep it high contrast against dark blue parts
    
    // Draw drop shadow for readability
    draw_string(buf, "BE HAPPY! :)", (int)text_x + 2, (int)text_y + 2, 2, 0);
    draw_string(buf, "BE HAPPY! :)", (int)text_x, (int)text_y, 2, text_color);
}
