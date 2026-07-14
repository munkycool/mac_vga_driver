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

static void draw_string_centered_multiline(uint8_t *buffer, const char *str, int cx, int y, int scale, uint8_t color) {
    char line[64];
    int line_idx = 0;
    int curr_y = y;
    int idx = 0;
    while (1) {
        char c = str[idx];
        if (c == '\n' || c == '\0') {
            line[line_idx] = '\0';
            if (line_idx > 0) {
                int len = line_idx;
                int width = len * 4 * scale - scale;
                int x = cx - width / 2;
                // Draw shadow
                draw_string(buffer, line, x + scale, curr_y + scale, scale, 0);
                draw_string(buffer, line, x, curr_y, scale, color);
            }
            curr_y += 8 * scale; // line spacing
            line_idx = 0;
            if (c == '\0') break;
        } else {
            if (line_idx < 63) {
                line[line_idx++] = c;
            }
        }
        idx++;
    }
}

void init_happy_bd() {
    // Purely frame-based animation
}

void play_happy_bd(uint8_t *buf, int frame) {
    // 1. Sky: Dark night sky dither (Blue 4 to Black 0)
    for (int y = 0; y < 150; y++) {
        int d = y / 10; // 0 to 14
        if (d > 15) d = 15;
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (d >= t) ? 4 : 0; // Blue (4) to Black (0)
            draw_pixel(buf, x, y, col);
        }
    }

    // 2. Twinkling stars in sky
    for (int i = 0; i < 15; i++) {
        int sx = (i * 27 + 13) % 320;
        int sy = (i * 11 + 7) % 110;
        int phase = (frame + i * 15) % 60;
        if (phase < 30) {
            uint8_t star_color = (phase < 10 || phase > 20) ? 6 : 7; // Twinkle Cyan (6) or White (7)
            draw_pixel(buf, sx, sy, star_color);
            if (phase == 15) {
                draw_pixel(buf, sx-1, sy, 4);
                draw_pixel(buf, sx+1, sy, 4);
                draw_pixel(buf, sx, sy-1, 4);
                draw_pixel(buf, sx, sy+1, 4);
            }
        }
    }

    // 3. Moon breathing (calm, pulsing moon in center)
    // Breathing calculation: 8 second cycle (approx 530 frames at 66.7Hz)
    // 530 frames = 2 * PI / 0.012f
    float breath = sinf((float)frame * 0.012f);
    int r = 18 + (int)(6.0f * breath); // Radius ranges from 12 to 24
    int cx = 160;
    int cy = 60;

    // Outer glow circles (Cyan - 6)
    draw_circle(buf, cx, cy, r + 4, 6);
    draw_circle(buf, cx, cy, r + 2, 6);
    // Moon body (White - 7)
    draw_circle(buf, cx, cy, r, 7);

    // Smiling face on the moon
    if (r >= 14) {
        draw_pixel(buf, cx - 4, cy - 2, 6);
        draw_pixel(buf, cx + 4, cy - 2, 6);
        draw_pixel(buf, cx - 2, cy + 2, 6);
        draw_pixel(buf, cx - 1, cy + 3, 6);
        draw_pixel(buf, cx,     cy + 3, 6);
        draw_pixel(buf, cx + 1, cy + 3, 6);
        draw_pixel(buf, cx + 2, cy + 2, 6);
    }

    // 4. Ocean waves at bottom (y = 145 to 240)
    for (int x = 0; x < 320; x++) {
        // Wave horizon
        int wave_y = 145 + (int)(4.0f * sinf((float)x * 0.03f + (float)frame * 0.02f) +
                                2.0f * cosf((float)x * 0.07f - (float)frame * 0.04f));
        for (int y = wave_y; y < 240; y++) {
            // Gradient dither for deep sea (Cyan 6 to Blue 4)
            int dist = y - wave_y;
            int factor = (dist * 16) / 95; // 0 to 15
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < factor) ? 6 : 4;

            // Moon reflection path: shimmering water column
            int ref_width = 8 + (y - 145) / 4;
            if (x >= 160 - ref_width && x <= 160 + ref_width) {
                int ref_phase = (frame + y * 2) % 30;
                if ((x + ref_phase) % 6 < 3) {
                    col = 7; // White shimmer
                } else {
                    col = 6; // Cyan highlight
                }
            }
            draw_pixel(buf, x, y, col);
        }
    }

    // 5. Cycling support quotes for BD / Bipolar stability
    const char *BD_QUOTES[] = {
        "BREATHE IN...\nBREATHE OUT...",
        "YOU ARE THE OCEAN,\nNOT THE WAVES.",
        "THIS PHASE WILL PASS.",
        "BALANCE WILL RETURN.",
        "IT'LL PASS."
    };
    int quote_idx = (frame / 300) % 5;
    
    // Dynamically replace text if breathing quote to guide the user
    char breathe_buf[64];
    const char *display_text = BD_QUOTES[quote_idx];
    if (quote_idx == 0) {
        if (breath > 0.0f) {
            sprintf(breathe_buf, "BREATHE IN...\n(MOON EXPANDS)");
        } else {
            sprintf(breathe_buf, "BREATHE OUT...\n(MOON CONTRACTS)");
        }
        display_text = breathe_buf;
    }

    // Draw centering text on the ocean backdrop
    draw_string_centered_multiline(buf, display_text, 160, 185, 2, 3); // Yellow - 3 for readability
}

void init_happy_mdd() {
    // Purely frame-based animation
}

void play_happy_mdd(uint8_t *buf, int frame) {
    // 1. Sky: Gorgeous dithered sunrise gradient (Black 0 -> Blue 4 -> Magenta 5 -> Yellow 3)
    for (int y = 0; y < 140; y++) {
        uint8_t col_a, col_b;
        int factor;
        if (y < 40) {
            col_a = 0; col_b = 4; // Black to Blue
            factor = (y * 16) / 40;
        } else if (y < 85) {
            col_a = 4; col_b = 5; // Blue to Magenta
            factor = ((y - 40) * 16) / 45;
        } else {
            col_a = 5; col_b = 3; // Magenta to Yellow/Orange
            factor = ((y - 85) * 16) / 55;
        }
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < factor) ? col_b : col_a;
            draw_pixel(buf, x, y, col);
        }
    }

    // 2. Slow rising sun (behind mountains)
    int sun_y = 110 - (int)(12.0f * sinf((float)frame * 0.005f));
    int sun_cx = 160;
    // Draw sun base and rays
    draw_circle(buf, sun_cx, sun_y, 20, 3); // Yellow
    draw_circle(buf, sun_cx, sun_y, 14, 7); // White core

    // Draw some simple rays
    for (int angle_deg = 0; angle_deg < 360; angle_deg += 45) {
        float rad = (float)angle_deg * 3.14159f / 180.0f;
        int rx1 = sun_cx + (int)(cosf(rad) * 23.0f);
        int ry1 = sun_y + (int)(sinf(rad) * 23.0f);
        int rx2 = sun_cx + (int)(cosf(rad) * 30.0f);
        int ry2 = sun_y + (int)(sinf(rad) * 30.0f);
        draw_rect(buf, (rx1+rx2)/2, (ry1+ry2)/2, (rx1+rx2)/2, (ry1+ry2)/2, 3);
    }

    // 3. Midground Mountains (Dark Blue 4 dithered with Black 0)
    for (int x = 0; x < 320; x++) {
        int my = 120 + (int)(16.0f * sinf((float)x * 0.015f + 0.8f) + 5.0f * cosf((float)x * 0.035f));
        for (int y = my; y < 240; y++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < 5) ? 0 : 4;
            draw_pixel(buf, x, y, col);
        }
    }

    // 4. Foreground Green Hills (Green 2 dithered with Yellow 3)
    for (int x = 0; x < 320; x++) {
        int hy = 165 + (int)(10.0f * cosf((float)x * 0.012f - 0.5f));
        for (int y = hy; y < 240; y++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < 6) ? 2 : 3;
            draw_pixel(buf, x, y, col);
        }
    }

    // 5. Floating warm fireflies/sparks rising slowly
    for (int i = 0; i < 8; i++) {
        int sx = (i * 53 + frame / 3) % 320;
        int sy = 230 - ((frame + i * 40) % 190);
        int phase = (frame + i * 9) % 40;
        if (phase < 20) {
            draw_pixel(buf, sx, sy, 3); // Yellow glow
            draw_pixel(buf, sx + 1, sy, 7); // White center spark
        }
    }

    // 6. Supportive messages for MDD / Depression hope
    const char *MDD_QUOTES[] = {
        "YOU'LL FEEL BETTER.",
        "ONE STEP AT A TIME.",
        "THE SUN WILL RISE AGAIN.",
        "IT IS OK TO REST.",
        "YOU ARE NOT ALONE.",
        "IT'LL PASS."
    };
    int quote_idx = (frame / 300) % 6;
    draw_string_centered_multiline(buf, MDD_QUOTES[quote_idx], 160, 35, 2, 7); // White text with black shadow
}

void init_happy_ocd() {
    // Purely frame-based animation
}

void play_happy_ocd(uint8_t *buf, int frame) {
    // 1. Background sky/rain scene: Navy Blue (4) and Black (0)
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < 3) ? 4 : 0; // Soft navy blue dither
            draw_pixel(buf, x, y, col);
        }
    }

    // 2. Drifting clouds in the background (thoughts moving away)
    int cx1 = (frame / 2) % 420 - 100;
    if (cx1 > -60 && cx1 < 320) {
        draw_circle(buf, cx1, 45, 12, 4);
        draw_circle(buf, cx1 - 10, 48, 8, 4);
        draw_circle(buf, cx1 + 10, 48, 8, 4);
        draw_rect(buf, cx1 - 15, 45, cx1 + 15, 52, 4);
    }
    int cx2 = (frame / 4 + 180) % 420 - 100;
    if (cx2 > -60 && cx2 < 320) {
        draw_circle(buf, cx2, 25, 9, 4);
        draw_circle(buf, cx2 - 8, 27, 6, 4);
        draw_circle(buf, cx2 + 8, 27, 6, 4);
        draw_rect(buf, cx2 - 12, 25, cx2 + 12, 30, 4);
    }

    // 3. Raindrops and ripples
    for (int i = 0; i < 20; i++) {
        int rx = (i * 37 + 13) % 320;
        int ry = (frame * 6 + i * 47) % 240;
        draw_rect(buf, rx, ry, rx, ry + 5, 6); // Cyan rain streak

        if (ry > 200) {
            int ripple_y = 210 + (i % 25);
            int r_size = (frame % 8) / 2;
            if (r_size > 0) {
                draw_circle(buf, rx, ripple_y, r_size, 4); // blue ripple
            }
        }
    }

    // 4. Cozy hanging lantern on the side (Warm grounding presence)
    int lx = 35; // Lantern X
    draw_rect(buf, lx - 15, 100, lx - 13, 230, 0); // Black pole
    draw_rect(buf, lx - 15, 100, lx + 5, 102, 0);  // Hanger arm
    draw_rect(buf, lx + 3, 102, lx + 4, 110, 0);   // Hanging string
    
    draw_rect(buf, lx - 3, 110, lx + 10, 132, 0);  // Frame
    draw_rect(buf, lx - 1, 112, lx + 8, 130, 3);   // Glowing yellow core
    if (frame % 14 < 7) {
        draw_rect(buf, lx + 2, 118, lx + 5, 124, 1); // Red wick glow
    }

    // 5. Bordered centering box for OCD grounding quotes
    int bx1 = 80;
    int by1 = 85;
    int bx2 = 280;
    int by2 = 155;

    draw_rect(buf, bx1 + 3, by1 + 3, bx2 + 3, by2 + 3, 0);
    draw_rect(buf, bx1, by1, bx2, by2, 6);
    draw_rect(buf, bx1 + 2, by1 + 2, bx2 - 2, by2 - 2, 0);

    // OCD / Intrusive Thought Quotes
    const char *OCD_QUOTES[] = {
        "IT'S JUST A THOUGHT.",
        "LET IT DRIFT BY.",
        "YOU ARE SAFE\nRIGHT NOW.",
        "SIT WITH\nUNCERTAINTY.",
        "YOU ARE STRONGER\nTHAN THE THOUGHTS.",
        "IT'LL PASS."
    };
    int quote_idx = (frame / 300) % 6;
    draw_string_centered_multiline(buf, OCD_QUOTES[quote_idx], 180, 105, 2, 7); // White text inside box
}

void init_happy_mania() {
    // Purely frame-based slow waving animation
}

void play_happy_mania(uint8_t *buf, int frame) {
    // 1. Sky: Quiet night sky with a slowly waving Aurora Borealis (Cyan 6 / Green 2)
    for (int y = 0; y < 160; y++) {
        for (int x = 0; x < 320; x++) {
            // Waving math
            float wave1 = sinf((float)x * 0.012f + (float)frame * 0.006f) * 16.0f;
            float wave2 = cosf((float)x * 0.024f - (float)frame * 0.003f) * 8.0f;
            int target_y = 65 + (int)(wave1 + wave2);
            int dist = abs(y - target_y);
            
            uint8_t col = 0; // Black space
            if (dist < 28) {
                int t = art_bayer4_at(x, y);
                int factor = (dist * 16) / 28;
                if (t > factor) {
                    col = (dist < 12) ? 6 : 2; // Cyan (6) core, Green (2) edges
                }
            }
            if (col == 0) {
                // Background navy gradient
                int d = y / 10;
                int t = art_bayer4_at(x, y);
                col = (d >= t) ? 4 : 0;
            }
            draw_pixel(buf, x, y, col);
        }
    }

    // 2. Twinkling, slowly moving background particles (calm rain/drift of stars)
    for (int i = 0; i < 10; i++) {
        int sx = (i * 37 + frame / 5) % 320;
        int sy = (i * 19 + frame / 4) % 150;
        if ((frame + i * 13) % 40 < 20) {
            draw_pixel(buf, sx, sy, 7); // White twinkling star
        }
    }

    // 3. Ground meadow (Dark Green 2)
    for (int y = 160; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < 5) ? 2 : 0; // quiet green/black dither
            draw_pixel(buf, x, y, col);
        }
    }

    // 4. Centered Grounding Message (soothing quotes to slow down manic rushes)
    const char *MANIA_QUOTES[] = {
        "SLOW DOWN.",
        "IT'S OKAY TO PAUSE.",
        "LET THE RUSH PASS.",
        "FEEL THE GROUND.",
        "BREATHE DEEP.",
        "YOU ARE SAFE."
    };
    int quote_idx = (frame / 300) % 6;
    draw_string_centered_multiline(buf, MANIA_QUOTES[quote_idx], 160, 185, 2, 3); // Yellow - 3 for warmth
}

void init_happy_psychosis() {
    // Purely frame-based grounded cabin animation
}

void play_happy_psychosis(uint8_t *buf, int frame) {
    // 1. Sky: Quiet midnight blue (4)
    for (int y = 0; y < 160; y++) {
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < 2) ? 4 : 0;
            draw_pixel(buf, x, y, col);
        }
    }
    
    // 2. Ground (Green 2 and Black 0 dither)
    for (int y = 160; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < 6) ? 2 : 0;
            draw_pixel(buf, x, y, col);
        }
    }

    // 3. Deeply Rooted Oak Tree (Left side)
    draw_rect(buf, 40, 100, 48, 165, 0); // Black trunk
    draw_rect(buf, 35, 160, 39, 165, 0); // roots
    draw_rect(buf, 49, 160, 53, 165, 0);
    draw_circle(buf, 44, 95, 20, 2); // foliage
    draw_circle(buf, 30, 85, 15, 2);
    draw_circle(buf, 58, 85, 15, 2);
    draw_circle(buf, 44, 70, 18, 2);

    // 4. Cozy Grounded Cabin (Right side)
    int cx = 220;
    int cy = 120;
    // Walls (Beige / Yellow 3 dithered with White 7)
    for (int y = cy; y < cy + 45; y++) {
        for (int x = cx; x < cx + 60; x++) {
            int t = art_bayer4_at(x, y);
            uint8_t col = (t < 8) ? 3 : 7;
            draw_pixel(buf, x, y, col);
        }
    }
    // Roof (Red - 1)
    for (int y = cy - 20; y < cy; y++) {
        int w = (y - (cy - 20)) * 3 / 2; // expanding triangle
        draw_rect(buf, cx + 30 - w, y, cx + 30 + w, y, 1);
    }
    // Door (Black - 0)
    draw_rect(buf, cx + 15, cy + 20, cx + 27, cy + 45, 0);
    // Warm lit Window (Yellow - 3, glowing)
    draw_rect(buf, cx + 38, cy + 12, cx + 50, cy + 24, 0); // frame
    draw_rect(buf, cx + 40, cy + 14, cx + 48, cy + 22, 3); // glass
    
    // Chimney & Smoke puffs
    draw_rect(buf, cx + 48, cy - 22, cx + 54, cy - 8, 0); // black chimney
    for (int i = 0; i < 3; i++) {
        int smoke_phase = (frame + i * 50) % 150;
        int sx = cx + 51 + smoke_phase / 3;
        int sy = cy - 25 - smoke_phase / 4;
        if (smoke_phase < 120) {
            int r_sz = 2 + smoke_phase / 30;
            draw_circle(buf, sx, sy, r_sz, 6); // Smoke puff in Cyan - 6
        }
    }

    // 5. Centered Grounding Box for Reality Anchors
    int bx1 = 80;
    int by1 = 175;
    int bx2 = 280;
    int by2 = 230;

    draw_rect(buf, bx1 + 2, by1 + 2, bx2 + 2, by2 + 2, 0); // shadow
    draw_rect(buf, bx1, by1, bx2, by2, 2); // Green border
    draw_rect(buf, bx1 + 1, by1 + 1, bx2 - 1, by2 - 1, 0); // Black interior

    const char *PSY_QUOTES[] = {
        "YOU ARE HERE, RIGHT NOW.",
        "THIS MOMENT IS REAL.",
        "FEEL THE GROUND.",
        "THE FOG WILL CLEAR.",
        "YOU ARE SAFE.",
        "IT WILL PASS."
    };
    int quote_idx = (frame / 300) % 6;
    draw_string_centered_multiline(buf, PSY_QUOTES[quote_idx], 180, 185, 1, 7); // White text inside box
}

