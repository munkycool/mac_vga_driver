#include "common.h"

// 16x16 Sanic Sprite
// 0: Black, 1: Red, 2: Green, 3: Yellow, 4: Blue, 7: White, 8: Transparent
static const uint8_t SANIC_SPRITE[16][16] = {
    {8,8,8,4,4,4,4,8,8,8,8,8,8,8,8,8},
    {8,8,4,4,4,4,4,4,8,8,8,8,8,8,8,8},
    {8,4,4,4,4,4,4,4,4,4,8,8,8,8,8,8},
    {4,4,4,4,4,4,4,4,4,4,4,8,8,8,8,8},
    {4,4,7,7,7,4,4,7,7,7,4,4,8,8,8,8},
    {4,7,7,0,7,7,7,7,0,7,7,4,8,8,8,8},
    {4,7,2,0,2,7,7,2,0,2,7,4,8,8,8,8}, // Green derpy eyes
    {4,7,7,7,7,7,7,7,7,7,7,4,8,8,8,8},
    {4,4,3,3,3,3,3,3,3,3,4,4,8,8,8,8}, // Yellow mouth
    {8,4,4,3,3,3,3,3,3,4,4,8,8,8,8,8},
    {8,8,4,4,4,4,4,4,4,4,8,8,8,8,8},
    {8,8,7,7,4,4,4,4,7,7,8,8,8,8,8,8}, // White gloves
    {8,8,4,4,4,4,4,4,4,4,8,8,8,8,8},
    {8,8,8,4,4,8,8,4,4,8,8,8,8,8,8,8},
    {8,8,1,1,1,8,8,1,1,1,8,8,8,8,8,8}, // Red shoes
    {8,8,1,1,1,8,8,1,1,1,8,8,8,8,8,8}
};

static int sanic_x = 0;
static int sanic_y = 120;
static int sanic_score = 0;

// Rings
static int ring_x[3];
static int ring_y[3];
static bool ring_active[3];

// Speed trail
#define MAX_TRAILS 10
static int trail_x[MAX_TRAILS];
static int trail_y[MAX_TRAILS];
static int trail_count = 0;

void init_sanic() {
    sanic_x = -20;
    sanic_y = 120;
    sanic_score = 0;
    trail_count = 0;

    for (int i = 0; i < 3; i++) {
        ring_x[i] = 80 + i * 100;
        ring_y[i] = 80 + (get_rand() % 80);
        ring_active[i] = true;
    }
}

void play_sanic(uint8_t *buffer, int frame_counter) {
    // Clear screen
    clear_screen(buffer);

    // Speed trails
    if (frame_counter % 2 == 0) {
        if (trail_count < MAX_TRAILS) {
            trail_x[trail_count] = sanic_x;
            trail_y[trail_count] = sanic_y;
            trail_count++;
        } else {
            for (int i = 1; i < MAX_TRAILS; i++) {
                trail_x[i - 1] = trail_x[i];
                trail_y[i - 1] = trail_y[i];
            }
            trail_x[MAX_TRAILS - 1] = sanic_x;
            trail_y[MAX_TRAILS - 1] = sanic_y;
        }
    }

    // Draw speed trails (fading blue dots)
    for (int i = 0; i < trail_count; i++) {
        draw_rect(buffer, trail_x[i], trail_y[i] + 4, trail_x[i] + 4, trail_y[i] + 8, 4); // Blue dots
    }

    // Distorted Green Hill zone floor
    draw_rect(buffer, 0, 200, 319, 239, 2); // Green grass
    for (int x = 0; x < 320; x += 16) {
        int pattern_y = 208 + ((x / 16) % 2) * 8;
        draw_rect(buffer, x, pattern_y, x + 8, pattern_y + 8, 1); // Brown dirt squares
    }

    // Auto / Interactive Movement physics
    if (global_input.interactive) {
        // Playable controls
        if (global_input.up) {
            sanic_y -= 4;
        }
        if (global_input.down) {
            sanic_y += 4;
        }
        if (global_input.left) {
            sanic_x -= 3;
        }
        if (global_input.right) {
            sanic_x += 5;
        } else {
            sanic_x += 3; // Constant scroll
        }
        
        // Keep in screen bounds
        if (sanic_y < 20) sanic_y = 20;
        if (sanic_y > 168) sanic_y = 168;
    } else {
        // Attract Mode: automated sine wave movement
        sanic_x += 4;
        sanic_y = 100 + (int)(40 * (frame_counter % 80 - 40)*(frame_counter % 80 - 40)/1600);
        // loop sanic around screen
        if (sanic_y > 168) sanic_y = 168;
    }

    if (sanic_x > 320) {
        sanic_x = -20;
    }
    if (sanic_x < -20) {
        sanic_x = 320;
    }

    // Draw Rings and handle collection
    for (int i = 0; i < 3; i++) {
        if (ring_active[i]) {
            draw_circle(buffer, ring_x[i], ring_y[i], 4, 3); // Yellow ring
            draw_circle(buffer, ring_x[i], ring_y[i], 2, 0); // Black cutout

            // Collision check
            if (abs(sanic_x + 16 - ring_x[i]) < 20 && abs(sanic_y + 16 - ring_y[i]) < 20) {
                ring_active[i] = false;
                sanic_score += 100;
            }
        } else {
            // Respawn ring
            if (frame_counter % 120 == 0) {
                ring_x[i] = 40 + (get_rand() % 240);
                ring_y[i] = 60 + (get_rand() % 100);
                ring_active[i] = true;
            }
        }
    }

    // Draw Sanic sprite — 4K ULTRA HD PRO PLUS ULTRA (2x block upscale)
    for (int r = 0; r < 16; r++) {
        for (int c = 0; c < 16; c++) {
            uint8_t color = SANIC_SPRITE[r][c];
            if (color != 8) {
                int shake_x = 0;
                int shake_y = 0;
                if (frame_counter % 4 < 2) {
                    shake_x = (get_rand() % 3) - 1;
                    shake_y = (get_rand() % 3) - 1;
                }
                art_block(buffer, sanic_x + c * ART_HD_SCALE + shake_x,
                          sanic_y + r * ART_HD_SCALE + shake_y, color, ART_HD_SCALE);
            }
        }
    }

    // Text: GOTTA GO FAST
    draw_string(buffer, "GOTTA GO FAST", 80, 20, 2, 1); // Red text
    draw_string(buffer, "GOTTA GO FAST", 82, 22, 2, 3); // Yellow shadow

    // Draw score
    draw_score(buffer, sanic_score, 10, 10, 1, 7);

    // Interactive prompt
    if (!global_input.interactive) {
        if (frame_counter % 40 < 20) {
            draw_string(buffer, "PRESS ANY KEY TO PLAY", 60, 180, 1, 7);
        }
    } else {
        draw_string(buffer, "PLAYING - WASD TO MOVE", 50, 180, 1, 6);
    }
}
