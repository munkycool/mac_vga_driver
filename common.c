#include "common.h"

// --- LCG RNG State ---
static uint32_t rng_state = 0;

// --- Alphanumeric Font Engine (3x5 pixel grid for compact arcade displays) ---
const uint16_t ALPHANUM_FONT[41] = {
    0b111101101101111, // 0 (0)
    0b010010010010010, // 1 (1)
    0b111001111100111, // 2 (2)
    0b111001111001111, // 3 (3)
    0b101101111001001, // 4 (4)
    0b111100111001111, // 5 (5)
    0b111100111101111, // 6 (6)
    0b111001001010010, // 7 (7)
    0b111101111101111, // 8 (8)
    0b111101111001111, // 9 (9)
    0b111101111101101, // A (10)
    0b110101110101110, // B (11)
    0b111100100100111, // C (12)
    0b110101101101110, // D (13)
    0b111100111100111, // E (14)
    0b111100111100100, // F (15)
    0b111100101101111, // G (16)
    0b101101111101101, // H (17)
    0b010010010010010, // I (18)
    0b001001001101110, // J (19)
    0b101101110101101, // K (20)
    0b100100100100111, // L (21)
    0b101111101101101, // M (22)
    0b110110101101101, // N (23)
    0b111101101101111, // O (24)
    0b111101111100100, // P (25)
    0b111101101111011, // Q (26)
    0b111101111101101, // R (27)
    0b111100111001111, // S (28)
    0b111010010010010, // T (29)
    0b101101101101111, // U (30)
    0b101101101101010, // V (31)
    0b101101101111101, // W (32)
    0b101101010101101, // X (33)
    0b101101010010010, // Y (34)
    0b111001010100111, // Z (35)
    0b000000000000000, // Space (36)
    0b000010000010000, // : (37)
    0b010010010000010, // ! (38)
    0b000000111000000, // - (39)
    0b111001010000010  // ? (40)
};

void seed_rng() {
    rng_state = (uint32_t)time_us_64();
}

uint32_t get_rand() {
    rng_state = rng_state * 1103515245 + 12345;
    return (rng_state / 65536) % 32768;
}

int isqrt(int value) {
    if (value < 0) return 0;
    int res = 0;
    int one = 1 << 30;
    while (one > value) one >>= 2;
    while (one != 0) {
        if (value >= res + one) {
            value -= res + one;
            res += one << 1;
        }
        res >>= 1;
        one >>= 2;
    }
    return res;
}

// --- Shuffle Bag State Picker ---
static enum ProgramState state_bag[NUM_STATES];
static int state_bag_index = NUM_STATES; // Trigger initial shuffle

static void shuffle_state_bag() {
    for (int i = 0; i < NUM_STATES; i++) {
        state_bag[i] = (enum ProgramState)i;
    }
    // Fisher-Yates shuffle
    for (int i = NUM_STATES - 1; i > 0; i--) {
        int j = get_rand() % (i + 1);
        enum ProgramState temp = state_bag[i];
        state_bag[i] = state_bag[j];
        state_bag[j] = temp;
    }
    state_bag_index = 0;
}

enum ProgramState pick_next_state(enum ProgramState current) {
    if (state_bag_index >= NUM_STATES) {
        shuffle_state_bag();
        // Avoid consecutive duplicate if the first element of the new shuffle is the same as current
        if (state_bag[0] == current && NUM_STATES > 1) {
            int swap_idx = 1 + (get_rand() % (NUM_STATES - 1));
            enum ProgramState temp = state_bag[0];
            state_bag[0] = state_bag[swap_idx];
            state_bag[swap_idx] = temp;
        }
    }
    return state_bag[state_bag_index++];
}

void clear_screen(uint8_t *buffer) {
    memset(buffer, 0x88, 320 * 240); // Clear to safe sync-high black
}

void draw_pixel(uint8_t *buffer, int x, int y, uint8_t color) {
    if (x < 0 || x >= 320 || y < 0 || y >= 240) return;
    uint8_t pixel = (color & 0x07) | 0x08; // Keep GP10 (Sync) HIGH
    buffer[y * 320 + x] = (pixel << 4) | pixel;
}

void draw_rect(uint8_t *buffer, int x1, int y1, int x2, int y2, uint8_t color) {
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            draw_pixel(buffer, x, y, color);
        }
    }
}

// --- 4K ULTRA HD PRO PLUS ULTRA rendering engine ---

static const uint8_t ART_BAYER2[2][2] = {{0, 2}, {3, 1}};

void art_dither_pixel(uint8_t *buf, int x, int y, uint8_t color, int density4) {
    if (density4 <= 0) return;
    if (density4 >= 4) { draw_pixel(buf, x, y, color); return; }
    int bx = ((unsigned)x) & 1;
    int by = ((unsigned)y) & 1;
    if (ART_BAYER2[by][bx] < density4) draw_pixel(buf, x, y, color);
}

void art_dither_rect_v(uint8_t *buf, int x1, int y1, int x2, int y2,
                       uint8_t color_a, uint8_t color_b, int height) {
    for (int y = y1; y <= y2; y++) {
        int d = ((y - y1) * 15) / (height > 1 ? height - 1 : 1);
        for (int x = x1; x <= x2; x++) {
            if (d >= art_bayer4_at(x, y)) draw_pixel(buf, x, y, color_b);
            else                          draw_pixel(buf, x, y, color_a);
        }
    }
}

void art_dither_rect_h(uint8_t *buf, int x1, int y1, int x2, int y2,
                       uint8_t color_a, uint8_t color_b, int width) {
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            int d = ((x - x1) * 15) / (width > 1 ? width - 1 : 1);
            if (d >= art_bayer4_at(x, y)) draw_pixel(buf, x, y, color_b);
            else                          draw_pixel(buf, x, y, color_a);
        }
    }
}

void art_block(uint8_t *buf, int x, int y, uint8_t color, int scale) {
    if (scale <= 1) {
        draw_pixel(buf, x, y, color);
        return;
    }
    draw_rect(buf, x, y, x + scale - 1, y + scale - 1, color);
}

void art_rect(uint8_t *buf, int x1, int y1, int x2, int y2, uint8_t color, int scale) {
    if (scale <= 1) {
        draw_rect(buf, x1, y1, x2, y2, color);
        return;
    }
    for (int dy = y1; dy <= y2; dy++) {
        for (int dx = x1; dx <= x2; dx++) {
            art_block(buf, dx * scale, dy * scale, color, scale);
        }
    }
}

void art_sprite(uint8_t *buf, const uint8_t *pixels, int w, int h,
                int ox, int oy, int scale, uint8_t transparent) {
    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            uint8_t col = pixels[r * w + c];
            if (col != transparent) {
                art_block(buf, ox + c * scale, oy + r * scale, col, scale);
            }
        }
    }
}

void art_shaded_rect(uint8_t *buf, int x1, int y1, int x2, int y2,
                     uint8_t base, uint8_t hi, uint8_t shadow) {
    draw_rect(buf, x1, y1, x2, y2, base);
    for (int x = x1; x <= x2; x++) art_dither_pixel(buf, x, y1, hi, 3);
    for (int x = x1; x <= x2; x++) art_dither_pixel(buf, x, y2, shadow, 2);
    for (int y = y1 + 1; y < y2; y++) art_dither_pixel(buf, x1, y, hi, 2);
    for (int y = y1 + 1; y < y2; y++) art_dither_pixel(buf, x2, y, shadow, 2);
}

void art_circle_shaded(uint8_t *buf, int cx, int cy, int r,
                       uint8_t color_dark, uint8_t color_light) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) {
                uint8_t col = (dx + dy < -r / 3) ? color_light :
                              (dx + dy >  r / 3) ? color_dark : color_light;
                draw_pixel(buf, cx + dx, cy + dy, col);
            }
        }
    }
}

void art_specular(uint8_t *buf, int x, int y, uint8_t color) {
    draw_pixel(buf, x, y, color);
    art_dither_pixel(buf, x + 1, y, color, 2);
    art_dither_pixel(buf, x, y + 1, color, 2);
}

void draw_circle(uint8_t *buffer, int cx, int cy, int r, uint8_t color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) {
                draw_pixel(buffer, cx + x, cy + y, color);
            }
        }
    }
}

int get_char_idx(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (c == ' ') return 36;
    if (c == ':') return 37;
    if (c == '!') return 38;
    if (c == '-') return 39;
    if (c == '?') return 40;
    return 36;
}

void draw_char(uint8_t *buffer, char c, int x, int y, int scale, uint8_t color) {
    int idx = get_char_idx(c);
    uint16_t glyph = ALPHANUM_FONT[idx];
    for (int r = 0; r < 5; r++) {
        for (int col = 0; col < 3; col++) {
            int bit_idx = 14 - (r * 3 + col);
            if ((glyph >> bit_idx) & 1) {
                draw_rect(buffer, x + col * scale, y + r * scale, x + (col + 1) * scale - 1, y + (r + 1) * scale - 1, color);
            }
        }
    }
}

void draw_string(uint8_t *buffer, const char *str, int x, int y, int scale, uint8_t color) {
    int idx = 0;
    while (str[idx] != '\0') {
        draw_char(buffer, str[idx], x + idx * (4 * scale), y, scale, color);
        idx++;
    }
}

void draw_score(uint8_t *buffer, int score, int x, int y, int scale, uint8_t color) {
    char buf[16];
    sprintf(buf, "%04d", score);
    draw_string(buffer, buf, x, y, scale, color);
}

void draw_pro_mario(uint8_t *buffer, int x, int y) {
    // Clean, crisp NES-style Mario sprite (no noisy dithering or specular spots).
    // Centred at (x, y), y = foot baseline.
    // Colors: 0=black, 1=red, 3=skin/yellow, 4=blue, 5=brown, 7=white.

    // === HAT ===
    draw_rect(buffer, x - 4, y - 19, x + 4, y - 16, 1); // Hat crown (Red)
    draw_rect(buffer, x - 5, y - 15, x + 5, y - 14, 1); // Hat brim (Red)
    draw_rect(buffer, x - 5, y - 13, x + 5, y - 13, 0); // Hat bottom outline (Black)

    // === HAIR (back of head) ===
    draw_rect(buffer, x - 5, y - 12, x - 4, y - 9, 5);  // Brown hair

    // === FACE ===
    draw_rect(buffer, x - 3, y - 12, x + 4, y - 9, 3);  // Skin base
    // Eye
    draw_pixel(buffer, x + 1, y - 11, 7); // White
    draw_pixel(buffer, x + 2, y - 11, 0); // Black pupil
    // Nose (skin color extending forward)
    draw_rect(buffer, x + 3, y - 10, x + 5, y - 9, 3);

    // === MUSTACHE ===
    draw_rect(buffer, x + 1, y - 8, x + 4, y - 8, 5);   // Brown mustache
    draw_pixel(buffer, x + 2, y - 7, 5);

    // === TORSO & ARMS ===
    // Red sleeves/shirt
    draw_rect(buffer, x - 5, y - 7, x + 4, y - 4, 1);
    // Overalls (Blue straps)
    draw_rect(buffer, x - 2, y - 7, x - 1, y - 4, 4);
    draw_rect(buffer, x + 1, y - 7, x + 2, y - 4, 4);
    // Yellow buttons
    draw_pixel(buffer, x - 2, y - 4, 3);
    draw_pixel(buffer, x + 1, y - 4, 3);

    // Hands (White gloves)
    draw_rect(buffer, x - 7, y - 6, x - 6, y - 5, 7);
    draw_rect(buffer, x + 5, y - 6, x + 6, y - 5, 7);

    // === PANTS / LOWER OVERALLS ===
    draw_rect(buffer, x - 4, y - 3, x + 3, y - 1, 4); // Solid blue pants
    draw_rect(buffer, x - 1, y - 2, x, y - 1, 0);     // Leg gap outline

    // === BOOTS ===
    draw_rect(buffer, x - 5, y, x - 1, y, 5);         // Left boot (Brown)
    draw_rect(buffer, x, y, x + 4, y, 5);             // Right boot (Brown)
    draw_rect(buffer, x - 5, y + 1, x - 1, y + 1, 0); // Boot outlines
    draw_rect(buffer, x, y + 1, x + 4, y + 1, 0);
}

__attribute__((weak)) void init_undertale() {}
__attribute__((weak)) void play_undertale(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);
    draw_string(buffer, "UNDERTALE DEMO", 80, 100, 2, 7);
    draw_string(buffer, "COMING SOON", 100, 120, 2, 7);
}

__attribute__((weak)) void init_deltarune() {}
__attribute__((weak)) void play_deltarune(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);
    draw_string(buffer, "DELTARUNE DEMO", 80, 100, 2, 7);
    draw_string(buffer, "COMING SOON", 100, 120, 2, 7);
}

// --- Playable Game Input System ---
volatile InputState global_input = {
    .up = false,
    .down = false,
    .left = false,
    .right = false,
    .action1 = false,
    .action2 = false,
    .skip = false,
    .interactive = false,
    .timeout_ticks = 0
};

static int decay_up = 0;
static int decay_down = 0;
static int decay_left = 0;
static int decay_right = 0;
static int decay_action1 = 0;
static int decay_action2 = 0;
static int decay_skip = 0;

static int escape_state = 0;

void update_input() {
    int c;
    while ((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        global_input.interactive = true;
        global_input.timeout_ticks = 0;

        if (escape_state == 0) {
            if (c == 27) { // Escape code
                escape_state = 1;
            } else {
                if (c == 'w' || c == 'W') decay_up = 8;
                else if (c == 's' || c == 'S') decay_down = 8;
                else if (c == 'a' || c == 'A') decay_left = 8;
                else if (c == 'd' || c == 'D') decay_right = 8;
                else if (c == 'z' || c == 'Z' || c == ' ' || c == '\r' || c == '\n') decay_action1 = 8;
                else if (c == 'x' || c == 'X' || c == 'c' || c == 'C') decay_action2 = 8;
                else if (c == 'q' || c == 'Q') decay_skip = 8;
            }
        } else if (escape_state == 1) {
            if (c == '[') {
                escape_state = 2;
            } else {
                escape_state = 0;
            }
        } else if (escape_state == 2) {
            if (c == 'A') decay_up = 8;      // Up arrow
            else if (c == 'B') decay_down = 8;    // Down arrow
            else if (c == 'C') decay_right = 8;   // Right arrow
            else if (c == 'D') decay_left = 8;    // Left arrow
            escape_state = 0;
        }
    }

    if (decay_up > 0) { global_input.up = true; decay_up--; } else { global_input.up = false; }
    if (decay_down > 0) { global_input.down = true; decay_down--; } else { global_input.down = false; }
    if (decay_left > 0) { global_input.left = true; decay_left--; } else { global_input.left = false; }
    if (decay_right > 0) { global_input.right = true; decay_right--; } else { global_input.right = false; }
    if (decay_action1 > 0) { global_input.action1 = true; decay_action1--; } else { global_input.action1 = false; }
    if (decay_action2 > 0) { global_input.action2 = true; decay_action2--; } else { global_input.action2 = false; }
    if (decay_skip > 0) { global_input.skip = true; decay_skip--; } else { global_input.skip = false; }

    if (global_input.interactive) {
        global_input.timeout_ticks++;
        if (global_input.timeout_ticks > 2000) { // ~30 seconds at 66.7Hz
            global_input.interactive = false;
        }
    }
}


