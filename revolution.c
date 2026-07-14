#include "common.h"
#include <math.h>

static const char *COMMUNIST_QUOTES[] = {
    "WORKERS OF THE\nWORLD, UNITE!\nYOU HAVE NOTHING\nTO LOSE BUT\nYOUR CHAINS!\n- KARL MARX",
    "THE PHILOSOPHERS\nHAVE ONLY\nINTERPRETED THE\nWORLD; THE POINT\nIS TO CHANGE IT.\n- KARL MARX",
    "LIBERATION IS\nAN ACT OF\nHISTORICAL\nNECESSITY.\n- FRANTZ FANON"
};

static const char *PANTHER_QUOTES[] = {
    "ALL POWER TO\nTHE PEOPLE!\n- BLACK PANTHER\n  PARTY",
    "YOU CAN JAIL A\nREVOLUTIONARY,\nBUT YOU CAN'T\nJAIL THE\nREVOLUTION.\n- FRED HAMPTON",
    "I AM CHANGING\nTHE THINGS I\nCANNOT ACCEPT.\n- ANGELA DAVIS",
    "WE GO ON TO\nREDISTRIBUTE\nTHE WEALTH.\n- HUEY P. NEWTON"
};

static const char *QUEER_QUOTES[] = {
    "NO PRIDE FOR\nSOME WITHOUT\nLIBERATION\nFOR ALL!\n- MARSHA P.\n  JOHNSON",
    "I'M NOT ONLY IN\nTHE FIGHT FOR\nQUEER RIGHTS,\nBUT ALL RIGHTS.\n- SYLVIA RIVERA",
    "THE REVOLUTION\nHAS ALWAYS BEEN\nQUEER & TRANS!\n- STONEWALL '69",
    "GENDER IS A\nCONSTRUCT TO\nBE DESTROYED.\n- QUEER MUTINY"
};

static int current_subpage = 0;
static int quote_idx = 0;
static char typed_quote[256];
static int typed_chars = 0;
static int type_timer = 0;

void init_revolution() {
    current_subpage = get_rand() % 3;
    type_timer = 0;
    typed_chars = 0;

    if (current_subpage == 0) {
        quote_idx = get_rand() % (sizeof(COMMUNIST_QUOTES) / sizeof(COMMUNIST_QUOTES[0]));
        snprintf(typed_quote, sizeof(typed_quote), "%s", COMMUNIST_QUOTES[quote_idx]);
    } else if (current_subpage == 1) {
        quote_idx = get_rand() % (sizeof(PANTHER_QUOTES) / sizeof(PANTHER_QUOTES[0]));
        snprintf(typed_quote, sizeof(typed_quote), "%s", PANTHER_QUOTES[quote_idx]);
    } else {
        quote_idx = get_rand() % (sizeof(QUEER_QUOTES) / sizeof(QUEER_QUOTES[0]));
        snprintf(typed_quote, sizeof(typed_quote), "%s", QUEER_QUOTES[quote_idx]);
    }
}

// Draw a beautiful hammer and sickle (yellow/gold) on a waving flag
static void draw_hammer_sickle(uint8_t *buf, int cx, int cy, int size) {
    // Sickle arc
    for (int r = size - 3; r <= size + 1; r++) {
        for (float theta = -0.5f; theta < 2.5f; theta += 0.05f) {
            int sx = cx + (int)(cosf(theta) * (float)r);
            int sy = cy - (int)(sinf(theta) * (float)r * 0.8f);
            draw_pixel(buf, sx, sy, 3); // Gold
        }
    }
    // Sickle point
    draw_rect(buf, cx - 2, cy - size - 1, cx + 2, cy - size + 1, 3);
    
    // Hammer head
    draw_rect(buf, cx - size/2 - 2, cy - 2, cx - size/2 + 6, cy + 4, 3);
    draw_rect(buf, cx - size/2 + 2, cy - 6, cx - size/2 + 8, cy + 8, 3);
    
    // Hammer & Sickle Handles
    for (int i = 0; i < size; i++) {
        draw_pixel(buf, cx - size/2 - i/2, cy + i/2, 3); // hammer handle
        draw_pixel(buf, cx + i/2, cy + i/2, 3);          // sickle handle
    }
}

// Draw a leaping black panther silhouette
static void draw_black_panther(uint8_t *buf, int cx, int cy) {
    // Body (sleek curve)
    for (int dx = -35; dx < 35; dx++) {
        float height = 8.0f * cosf((float)dx * 0.04f);
        draw_rect(buf, cx + dx, cy - (int)height - 2, cx + dx, cy - (int)height + 4, 0); // Black silhouette
    }
    // Head & Ears
    draw_circle(buf, cx - 35, cy - 10, 8, 0);
    draw_rect(buf, cx - 40, cy - 20, cx - 37, cy - 16, 0); // Left ear
    draw_rect(buf, cx - 34, cy - 20, cx - 31, cy - 16, 0); // Right ear
    // Glowing green eye
    draw_pixel(buf, cx - 37, cy - 11, 2); // Green eye
    
    // Front Legs
    for (int i = 0; i < 15; i++) {
        draw_rect(buf, cx - 30 + i, cy - 5 + i, cx - 28 + i, cy - 2 + i, 0);
        draw_rect(buf, cx - 27 + i, cy - 3 + i, cx - 25 + i, cy + i, 0);
    }
    // Back Legs
    for (int i = 0; i < 18; i++) {
        draw_rect(buf, cx + 20 + i, cy - 2 + i, cx + 22 + i, cy + 1 + i, 0);
        draw_rect(buf, cx + 24 + i, cy - 4 + i, cx + 26 + i, cy - 1 + i, 0);
    }
    // Tail
    for (int dx = 30; dx < 55; dx++) {
        float ty = 5.0f * sinf((float)(dx - 30) * 0.15f);
        draw_rect(buf, cx + dx, cy + 2 + (int)ty, cx + dx + 1, cy + 4 + (int)ty, 0);
    }
}

// Draw a pink triangle and a raised fist (representing queer liberation)
static void draw_queer_fist(uint8_t *buf, int cx, int cy) {
    // 1. Pink Triangle (Stonewall / ACT UP symbol)
    for (int y = -25; y <= 25; y++) {
        int w = 25 - y;
        draw_rect(buf, cx - w/2, cy + y, cx + w/2, cy + y, 5); // Pink/Magenta
    }

    // 2. Raised Fist (Black silhouette in front of triangle)
    // Wrist/Forearm
    draw_rect(buf, cx - 6, cy + 10, cx + 6, cy + 30, 0);
    // Palm / Base of fingers
    draw_rect(buf, cx - 9, cy - 10, cx + 9, cy + 10, 0);
    // Folded fingers (4 blocks)
    draw_rect(buf, cx - 9, cy - 13, cx - 6, cy - 8, 0); // Index
    draw_rect(buf, cx - 5, cy - 15, cx - 2, cy - 8, 0); // Middle
    draw_rect(buf, cx - 1, cy - 15, cx + 2, cy - 8, 0); // Ring
    draw_rect(buf, cx + 3, cy - 13, cx + 6, cy - 8, 0); // Pinky
    // Folded Thumb
    draw_rect(buf, cx - 13, cy - 5, cx - 7, cy, 0);
    
    // Highlight inner fist lines with white for separation
    draw_rect(buf, cx - 6, cy - 7, cx - 6, cy + 5, 7);
    draw_rect(buf, cx - 2, cy - 7, cx - 2, cy + 5, 7);
    draw_rect(buf, cx + 2, cy - 7, cx + 2, cy + 5, 7);
    draw_rect(buf, cx - 10, cy - 2, cx - 7, cy - 2, 7);
}

void play_revolution(uint8_t *buf, int frame_counter) {
    // Clear screen
    clear_screen(buf);

    // Draw background and graphics based on sub-page
    if (current_subpage == 0) {
        // --- 1. REVOLUTIONARY SOCIALISM & COMMUNISM ---
        // Waving red flag background
        int flag_start_x = 20;
        int flag_end_x = 150;
        int flag_h = 100;
        int base_y = 60;

        // Flagpole
        draw_rect(buf, 15, 30, 19, 210, 7); // white pole
        draw_circle(buf, 17, 26, 4, 3);     // gold top

        for (int x = flag_start_x; x <= flag_end_x; x++) {
            float angle = (float)x * 0.05f - (float)frame_counter * 0.12f;
            int wave = (int)(sinf(angle) * 6.0f);
            
            float factor = (float)(x - flag_start_x) / (float)(flag_end_x - flag_start_x);
            wave = (int)((float)wave * factor);

            for (int y = 0; y < flag_h; y++) {
                int py = base_y + y + wave;
                if (py < 0 || py >= 240) continue;
                
                // Red background
                uint8_t color = 1; // Red
                draw_pixel(buf, x, py, color);
            }
        }

        // Hammer and Sickle waving in center of flag
        float mid_angle = (float)85 * 0.05f - (float)frame_counter * 0.12f;
        int mid_wave = (int)(sinf(mid_angle) * 6.0f * 0.5f);
        draw_hammer_sickle(buf, 85, 110 + mid_wave, 18);

        // Subtitle header
        draw_string(buf, "SOCIALISM", 175, 30, 2, 1);
        draw_string(buf, "RETRO ATTRACT", 175, 48, 1, 3);

    } else if (current_subpage == 1) {
        // --- 2. BLACK PANTHER PARTY ---
        // Pan-African Flag Background Stripes (Red, Black, Green)
        for (int y = 0; y < 240; y++) {
            uint8_t color = 0;
            if (y < 80) color = 1;      // Red
            else if (y < 160) color = 0; // Black
            else color = 2;             // Green
            
            for (int x = 0; x < 320; x++) {
                // Diagonal scanlines / mesh dither
                if ((x + y) % 4 == 0 && color != 0) {
                    draw_pixel(buf, x, y, 7); // Neon white grid accents
                } else {
                    draw_pixel(buf, x, y, color);
                }
            }
        }

        // Leaping panther silhouette in the middle left
        draw_black_panther(buf, 100, 110);

        // Header
        draw_string(buf, "BLACK PANTHERS", 175, 30, 1, 7);
        draw_string(buf, "ALL POWER TO", 175, 42, 1, 3);
        draw_string(buf, "THE PEOPLE!", 175, 52, 1, 1);

    } else {
        // --- 3. QUEER LIBERATION ---
        // Waving rainbow pride stripes in background
        for (int y = 0; y < 240; y += 4) {
            for (int x = 0; x < 320; x += 4) {
                // Wave distortion
                float wave = sinf((float)x * 0.02f - (float)frame_counter * 0.1f) * 8.0f;
                int ry = y + (int)wave;
                if (ry < 0 || ry >= 240) continue;

                int stripe = ry / 40;
                uint8_t color = 0;
                if (stripe == 0) color = 1;      // Red
                else if (stripe == 1) color = 3; // Orange/Yellow
                else if (stripe == 2) color = 2; // Green
                else if (stripe == 3) color = 4; // Blue
                else color = 5;                  // Purple/Magenta

                art_dither_pixel(buf, x, ry, color, 2);
            }
        }

        // Queer Fist & Pink Triangle Symbol
        draw_queer_fist(buf, 80, 110);

        // Header
        draw_string(buf, "QUEER LIBERATION", 160, 30, 1, 5);
        draw_string(buf, "STONEWALL RIOTS", 160, 42, 1, 7);
        draw_string(buf, "NO COMPROMISE", 160, 52, 1, 6);
    }

    // --- 4. RENDER TYPEWRITER QUOTE AT BOTTOM ---
    draw_rect(buf, 10, 175, 310, 176, 7); // Divider line
    
    int curr_x = 15;
    int curr_y = 182;
    int scale = 1;
    uint8_t text_color = (current_subpage == 0) ? 3 : ((current_subpage == 1) ? 7 : 6);

    for (int i = 0; i < typed_chars; i++) {
        char c = typed_quote[i];
        if (c == '\n') {
            curr_x = 15;
            curr_y += 8;
        } else {
            draw_char(buf, c, curr_x, curr_y, scale, text_color);
            curr_x += 4 * scale;
        }
    }

    // Typewriter timing physics
    if (typed_chars < (int)strlen(typed_quote)) {
        type_timer++;
        if (type_timer >= 2) {
            type_timer = 0;
            typed_chars++;
        }
    }

    // Blinking cursor
    if (frame_counter % 20 < 10) {
        draw_rect(buf, curr_x, curr_y, curr_x + 3, curr_y + 5, text_color);
    }
}
