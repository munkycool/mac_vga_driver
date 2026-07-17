#include "common.h"
#include "celeste_classic.h"
#include "celeste_ai_weights.h"
#include <stdarg.h>

// Include PICO-8 Assets
#include "tilemap.h"
#include "p8_gfx.h"
#include "p8_font.h"

// --- PICO-8 Framebuffer ---
static uint8_t pico8_fb[128][128];
static uint8_t draw_palette[16];
static int camera_x = 0;
static int camera_y = 0;
static int celeste_frame_counter = 0;

// Expose cached player state for the AI bot
static float p_x = -1, p_y = -1;
static float p_vx = 0, p_vy = 0;
static int p_djump = 0;
static _Bool p_on_ground = false;
static _Bool p_wall_left = false;
static _Bool p_wall_right = false;
static int p_room_x = 0, p_room_y = 0;

static bool ai_left = false;
static bool ai_right = false;
static bool ai_up = false;
static bool ai_down = false;
static bool ai_jump = false;
static bool ai_dash = false;

// --- AI Sequence Playback State ---
static int  ai_room = 0;
static int  ai_step = 0;
static int  ai_tick = 0;
static bool ai_player_alive = false; // for death / respawn detection



// Snow background particles for the surrounding border area
#define MAX_BG_SNOW 20
typedef struct {
    int x, y;
    int speed;
} BGSnow;
static BGSnow bg_snow[MAX_BG_SNOW];

// --- Palette mappings ---
// PICO-8 16 colors mapped to our 8 color VGA palette:
// 0: Black, 1: Red, 2: Green, 3: Yellow, 4: Blue, 5: Magenta, 6: Cyan, 7: White
static const uint8_t pico8_to_vga_palette[16] = {
    0, // 0: Black
    4, // 1: Dark Blue -> Blue
    4, // 2: Dark Purple -> Blue/Magenta
    2, // 3: Dark Green -> Green
    1, // 4: Brown -> Red
    0, // 5: Dark Grey -> Black
    7, // 6: Light Grey -> White
    7, // 7: White -> White
    1, // 8: Red -> Red
    3, // 9: Orange -> Yellow/Red
    3, // 10: Yellow -> Yellow
    2, // 11: Green -> Green
    6, // 12: Blue -> Cyan
    4, // 13: Indigo -> Blue
    5, // 14: Pink -> Magenta
    7  // 15: Peach -> White
};

// Helper to draw pixel inside local 128x128 PICO-8 frame
static inline void p8_pix(int x, int y, int col) {
    if (x >= 0 && x < 128 && y >= 0 && y < 128) {
        pico8_fb[y][x] = draw_palette[col % 16];
    }
}

// Helper to fill rectangle inside local PICO-8 frame
static inline void p8_fillrect(int x0, int y0, int x1, int y1, int col) {
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            p8_pix(x, y, col);
        }
    }
}

// Helper to draw sprite inside local PICO-8 frame
static inline void p8_spr(int cx, int cy, int sx, int sy, bool flipx, bool flipy) {
    for (int y = 0; y < 8; y++) {
        int target_y = sy + y;
        if (target_y >= 0 && target_y < 128) {
            for (int x = 0; x < 8; x++) {
                int src_y = flipy ? (7 - y) : y;
                int src_x = flipx ? (7 - x) : x;
                uint8_t col = (p8_gfx[cy][cx][src_y] >> (4 * src_x)) & 0xf;
                if (col != 0) { // Color 0 is transparent in spr() by default in PICO-8
                    p8_pix(sx + x, target_y, col);
                }
            }
        }
    }
}

// --- PICO-8 Callback Interface for ccleste ---
static int p8_call(CELESTE_P8_CALLBACK_TYPE calltype, ...) {
    va_list args;
    va_start(args, calltype);
    int ret = 0;

    switch (calltype) {
        case CELESTE_P8_MUSIC:
            // Music stub
            break;

        case CELESTE_P8_SFX:
            // Sound effect stub
            break;

        case CELESTE_P8_PAL: {
            int c0 = va_arg(args, int);
            int c1 = va_arg(args, int);
            draw_palette[c0 % 16] = c1 % 16;
            break;
        }

        case CELESTE_P8_PAL_RESET: {
            for (int i = 0; i < 16; i++) draw_palette[i] = i;
            break;
        }

        case CELESTE_P8_CIRCFILL: {
            int cx = va_arg(args, int) - camera_x;
            int cy = va_arg(args, int) - camera_y;
            int r = va_arg(args, int);
            int col = va_arg(args, int);
            // Simple PICO-8 style circle fill helper (for points / small circles)
            if (r <= 1) {
                p8_fillrect(cx - 1, cy, cx + 1, cy, col);
                p8_fillrect(cx, cy - 1, cx, cy + 1, col);
            } else if (r == 2) {
                p8_fillrect(cx - 2, cy - 1, cx + 2, cy + 1, col);
                p8_fillrect(cx - 1, cy - 2, cx + 1, cy + 2, col);
            } else {
                p8_fillrect(cx - r, cy - 1, cx + r, cy + 1, col);
                p8_fillrect(cx - 1, cy - r, cx + 1, cy + r, col);
            }
            break;
        }

        case CELESTE_P8_RECTFILL: {
            int x0 = va_arg(args, int) - camera_x;
            int y0 = va_arg(args, int) - camera_y;
            int x1 = va_arg(args, int) - camera_x;
            int y1 = va_arg(args, int) - camera_y;
            int col = va_arg(args, int);
            p8_fillrect(x0, y0, x1, y1, col);
            break;
        }

        case CELESTE_P8_LINE: {
            int x0 = va_arg(args, int) - camera_x;
            int y0 = va_arg(args, int) - camera_y;
            int x1 = va_arg(args, int) - camera_x;
            int y1 = va_arg(args, int) - camera_y;
            int col = va_arg(args, int);
            // PICO-8 line drawing (simplified for vertical/horizontal or short diagonal lines)
            if (x0 == x1) {
                int start_y = y0 < y1 ? y0 : y1;
                int end_y = y0 < y1 ? y1 : y0;
                for (int y = start_y; y <= end_y; y++) p8_pix(x0, y, col);
            } else {
                int start_x = x0 < x1 ? x0 : x1;
                int end_x = x0 < x1 ? x1 : x0;
                for (int x = start_x; x <= end_x; x++) p8_pix(x, y0, col);
            }
            break;
        }

        case CELESTE_P8_CAMERA: {
            camera_x = va_arg(args, int);
            camera_y = va_arg(args, int);
            break;
        }

        case CELESTE_P8_PRINT: {
            const char *str = va_arg(args, const char*);
            int x = va_arg(args, int) - camera_x;
            int y = va_arg(args, int) - camera_y;
            int col = va_arg(args, int);
            for (const char *ch = str; *ch != '\0'; ch++) {
                uint32_t glyph = p8_font[(uint8_t)*ch];
                for (int dy = 0; dy < 8; dy += 2) {
                    if (glyph & (1u << 0)) p8_pix(x + 0, y + dy + 0, col);
                    if (glyph & (1u << 1)) p8_pix(x + 1, y + dy + 0, col);
                    if (glyph & (1u << 2)) p8_pix(x + 2, y + dy + 0, col);
                    if (glyph & (1u << 3)) p8_pix(x + 3, y + dy + 0, col);
                    if (glyph & (1u << 4)) p8_pix(x + 0, y + dy + 1, col);
                    if (glyph & (1u << 5)) p8_pix(x + 1, y + dy + 1, col);
                    if (glyph & (1u << 6)) p8_pix(x + 2, y + dy + 1, col);
                    if (glyph & (1u << 7)) p8_pix(x + 3, y + dy + 1, col);
                    glyph >>= 8;
                }
                x += 4;
            }
            break;
        }

        case CELESTE_P8_SPR: {
            int sprite = va_arg(args, int);
            int x = va_arg(args, int) - camera_x;
            int y = va_arg(args, int) - camera_y;
            int cols = va_arg(args, int);
            int rows = va_arg(args, int);
            int flipx = va_arg(args, int);
            int flipy = va_arg(args, int);
            (void)cols; (void)rows; // Celeste Classic only uses 1x1 sprite calls
            p8_spr(sprite % 16, sprite / 16, x, y, flipx, flipy);
            break;
        }

        case CELESTE_P8_MAP: {
            int celx = va_arg(args, int);
            int cely = va_arg(args, int);
            int sx = va_arg(args, int) - camera_x;
            int sy = va_arg(args, int) - camera_y;
            int celw = va_arg(args, int);
            int celh = va_arg(args, int);
            int layer = va_arg(args, int);
            for (int cy = 0; cy < celh; cy++) {
                for (int cx = 0; cx < celw; cx++) {
                    int tile = tilemap_data[(cely + cy) * 128 + (celx + cx)];
                    if ((tile_flags[tile] & layer) == layer) {
                        p8_spr(tile % 16, tile / 16, sx + cx * 8, sy + cy * 8, false, false);
                    }
                }
            }
            break;
        }

        case CELESTE_P8_MGET: {
            int tx = va_arg(args, int);
            int ty = va_arg(args, int);
            if (tx >= 0 && tx < 128 && ty >= 0 && ty < 64) {
                ret = tilemap_data[tx + ty * 128];
            } else {
                ret = 0;
            }
            break;
        }

        case CELESTE_P8_FGET: {
            int tile = va_arg(args, int);
            int flag = va_arg(args, int);
            ret = !!(tile_flags[tile] & (1 << flag));
            break;
        }

        case CELESTE_P8_BTN: {
            int b = va_arg(args, int);
            if (global_input.interactive) {
                if (b == 0) ret = global_input.left;
                if (b == 1) ret = global_input.right;
                if (b == 2) ret = global_input.up;
                if (b == 3) ret = global_input.down;
                if (b == 4) ret = global_input.action1; // Jump (Z/Space)
                if (b == 5) ret = global_input.action2; // Dash (X/C)
            } else {
                if (b == 0) ret = ai_left;
                if (b == 1) ret = ai_right;
                if (b == 2) ret = ai_up;
                if (b == 3) ret = ai_down;
                if (b == 4) ret = ai_jump;
                if (b == 5) ret = ai_dash;
            }
            break;
        }

        default:
            break;
    }

    va_end(args);
    return ret;
}

// --- Wrapper functions called by main state machine ---

void init_celeste() {
    camera_x = 0;
    camera_y = 0;
    for (int i = 0; i < 16; i++) draw_palette[i] = i;
    memset(pico8_fb, 0, sizeof pico8_fb);

    // Set callback interface in ccleste
    Celeste_P8_set_call_func(p8_call);
    Celeste_P8_set_rndseed(12345);
    Celeste_P8_init();
    // Skip title screen so the AI starts from the same state as training
    // (training used Celeste_P8__DEBUG() before loading each room)
    Celeste_P8__DEBUG();

    // Reset AI playback state
    ai_room = 0; ai_step = 0; ai_tick = 0; ai_player_alive = false;

    // Initialize snow particles
    for (int i = 0; i < MAX_BG_SNOW; i++) {
        bg_snow[i].x = get_rand() % 320;
        bg_snow[i].y = get_rand() % 240;
        bg_snow[i].speed = 1 + (get_rand() % 2);
    }
}

void play_celeste(uint8_t *buffer, int frame_counter) {
    celeste_frame_counter = frame_counter;

    // Cache player state
    get_celeste_player_state(&p_x, &p_y, &p_vx, &p_vy, &p_djump, &p_on_ground, &p_wall_left, &p_wall_right, &p_room_x, &p_room_y);

    if (!global_input.interactive) {
        ai_left = false; ai_right = false; ai_up = false;
        ai_down = false; ai_jump = false; ai_dash = false;

        bool alive_now = (p_x >= 0);

        if (!alive_now) {
            // Spawn animation or dead — press nothing.
            // If we just died, reset so we replay from step 0 on respawn.
            if (ai_player_alive) {
                ai_step = 0;
                ai_tick  = 0;
            }
        } else {
            // Player is alive — run the pre-trained sequence.
            // Detect room advance and switch sequences.
            if (p_room_x != ai_room) {
                ai_room = (p_room_x < CELESTE_AI_NUM_ROOMS) ? p_room_x : 0;
                ai_step = 0;
                ai_tick  = 0;
            }

            int room    = (ai_room < CELESTE_AI_NUM_ROOMS) ? ai_room : 0;
            int seq_len = celeste_ai_seq_lens[room];
            int action  = (ai_step < seq_len)
                          ? celeste_ai_sequences[room][ai_step] : 0;

            switch (action) {
                case 1:  ai_left  = true; break;
                case 2:  ai_right = true; break;
                case 3:  ai_jump  = true; break;
                case 4:  ai_dash  = true; break;
                case 5:  ai_left  = true; ai_jump = true; break;
                case 6:  ai_right = true; ai_jump = true; break;
                case 7:  ai_left  = true; ai_dash = true; break;
                case 8:  ai_right = true; ai_dash = true; break;
                case 9:  ai_up    = true; ai_dash = true; break;
                case 10: ai_left  = true; ai_up   = true; ai_dash = true; break;
                case 11: ai_right = true; ai_up   = true; ai_dash = true; break;
                default: break;
            }

            if (++ai_tick >= CELESTE_AI_REPEAT) {
                ai_tick = 0;
                if (++ai_step >= seq_len) ai_step = 0;
            }
        }

        ai_player_alive = alive_now;
    }

    // 1. Update Game logic
    Celeste_P8_update();

    // 2. Render PICO-8 game to local framebuffer
    memset(pico8_fb, 0, sizeof pico8_fb);
    Celeste_P8_draw();

    // 3. Draw Background on full 320x240 screen
    // Sunset gradient (Cyan 6 to Magenta 5 to Dark Blue 4)
    for (int y = 0; y < 240; y++) {
        uint8_t color_sky = 4;
        if (y < 80) color_sky = 6;
        else if (y < 155) color_sky = 5;
        draw_rect(buffer, 0, y, 319, y, color_sky);
    }

    // Parallax Mountain peaks
    int bg_scroll = frame_counter / 6;
    for (int mx = -60; mx < 380; mx += 100) {
        int px = mx - (bg_scroll % 100);
        for (int h = 0; h < 50; h++) {
            int y_level = 170 + h;
            draw_rect(buffer, px - h, y_level, px + h, y_level, 4);
        }
    }

    // Drifting snow particles in the surrounding border
    for (int i = 0; i < MAX_BG_SNOW; i++) {
        bg_snow[i].y += bg_snow[i].speed;
        if (bg_snow[i].y > 240) {
            bg_snow[i].y = 0;
            bg_snow[i].x = get_rand() % 320;
        }
        draw_pixel(buffer, bg_snow[i].x, bg_snow[i].y, 7);
    }

    // 4. Centered 1.5x nearest-neighbor upscale of 128x128 PICO-8 screen onto the 320x240 screen
    // PICO-8 area size: 192x192. Centered at X: 64 to 255, Y: 24 to 215
    for (int Y = 0; Y < 192; Y++) {
        int src_y = (Y * 2) / 3;
        for (int X = 0; X < 192; X++) {
            int src_x = (X * 2) / 3;
            uint8_t p8_color = pico8_fb[src_y][src_x];
            uint8_t vga_color = pico8_to_vga_palette[p8_color];
            draw_pixel(buffer, 64 + X, 24 + Y, vga_color);
        }
    }

    // 5. Draw a clean border box outline around the play area (VGA White 7 / Dark Blue 4)
    draw_rect(buffer, 62, 22, 63, 217, 7); // Left border
    draw_rect(buffer, 256, 22, 257, 217, 7); // Right border
    draw_rect(buffer, 62, 22, 257, 23, 7); // Top border
    draw_rect(buffer, 62, 216, 257, 217, 7); // Bottom border

    // Attract mode text
    if (!global_input.interactive) {
        if (frame_counter % 40 < 20) {
            draw_string(buffer, "PRESS ANY KEY TO PLAY", 80, 222, 1, 7);
        }
    } else {
        draw_string(buffer, "PLAYING CELESTE CLASSIC 1-1", 55, 222, 1, 6);
    }
}
