#include "common.h"

// Fixed-point scale (8 bits fraction)
#define FP_SHIFT        8
#define TO_FP(x)        ((x) << FP_SHIFT)
#define FROM_FP(x)      ((x) >> FP_SHIFT)

// Game constants
#define BIRD_X          80
#define GRAVITY         25     // ~0.1 pixels/frame^2
#define JUMP_IMPULSE    -600   // ~-2.3 pixels/frame
#define TERM_VELOCITY   1500   // ~5.8 pixels/frame
#define GAP_HEIGHT      65

typedef struct {
    int x;
    int gap_y;
    bool passed;
} FB_Pipe;

static uint8_t flappy_bird_pixel_color(char pixel) {
    switch (pixel) {
        case 'k': return 0;
        case 'y': return 3;
        case 'o': return 1;
        case 'w': return 7;
        case 'e': return 7;
        case 'p': return 0;
        case 'h': return 7;
        default:  return 255;
    }
}

static void draw_flappy_bird_sprite(uint8_t *buffer, int x, int y, const char *const *sprite) {
    for (int row = 0; sprite[row] != NULL; row++) {
        const char *line = sprite[row];
        for (int col = 0; line[col] != '\0'; col++) {
            uint8_t color = flappy_bird_pixel_color(line[col]);
            if (color != 255) {
                draw_pixel(buffer, x + col, y + row, color);
            }
        }
    }
}

static const char *const FLAPPY_BIRD_WING_UP[] = {
    "................",
    "................",
    ".....kkkk.......",
    "...kkyyyyk......",
    "..kyyhhyyyyk....",
    ".kyyywwwyyyoo...",
    ".kyyyywyyyyoo....",
    "..kyyyyyyykk....",
    "...kkyyyyk......",
    ".....kkk........",
    "................",
    NULL
};

static const char *const FLAPPY_BIRD_WING_DOWN[] = {
    "................",
    "................",
    ".....kkkk.......",
    "...kkyyyyk......",
    "..kyyhhyyyyk....",
    ".kyyyyywyyyyoo..",
    ".kyyywwwyyyyoo..",
    "..kyyyyyyykk....",
    "...kkyyyyk......",
    ".....kkk........",
    "................",
    NULL
};

static int bird_y;     // 24.8 fixed point
static int bird_vy;    // 24.8 fixed point
static FB_Pipe pipes[2];
static int flappy_score;

void init_flappy_bird() {
    bird_y = TO_FP(120);
    bird_vy = 0;
    
    pipes[0].x = 240;
    pipes[0].gap_y = 90 + (get_rand() % 60); // 90 to 149
    pipes[0].passed = false;
    
    pipes[1].x = 420;
    pipes[1].gap_y = 90 + (get_rand() % 60);
    pipes[1].passed = false;
    
    flappy_score = 0;
}

void play_flappy_bird(uint8_t *buffer, int frame_counter) {
    // 1. Draw Background (Sky: Cyan 6)
    clear_screen(buffer);
    draw_rect(buffer, 0, 0, 319, 219, 6);

    // Draw Parallax Clouds
    for (int i = 0; i < 3; i++) {
        int cx = ((i * 150) - (frame_counter / 4)) % 400 - 50;
        if (cx > -30 && cx < 320) {
            draw_circle(buffer, cx, 50, 8, 7);
            art_circle_shaded(buffer, cx, 50, 8, 6, 7);
            draw_circle(buffer, cx + 6, 53, 6, 7);
            art_circle_shaded(buffer, cx + 6, 53, 6, 6, 7);
            draw_circle(buffer, cx - 6, 53, 6, 7);
            art_circle_shaded(buffer, cx - 6, 53, 6, 6, 7);
        }
    }

    // Parallax Distant Hills (Blue 4)
    for (int mx = -50; mx < 370; mx += 120) {
        int px = mx - (frame_counter / 8) % 120;
        draw_rect(buffer, px, 195, px + 80, 219, 4);
        draw_rect(buffer, px + 10, 190, px + 70, 195, 4);
    }

    // Parallax Closer Hills (Green 2)
    for (int hx = -30; hx < 350; hx += 80) {
        int px = hx - (frame_counter / 4) % 80;
        draw_circle(buffer, px + 40, 215, 20, 2);
    }

    // Ground: Green 2 with scrolling pattern
    draw_rect(buffer, 0, 220, 319, 239, 2);
    int ground_scroll = (frame_counter * 2) % 12;
    for (int x = -12; x < 332; x += 12) {
        int px = x - ground_scroll;
        draw_rect(buffer, px, 220, px + 1, 224, 7); // white highlight grass blades
        draw_rect(buffer, px + 5, 220, px + 6, 222, 0); // black shadow blades
    }
    draw_rect(buffer, 0, 220, 319, 220, 7); // white grass outline

    // 2. Locate Target Pipe for AI
    int target_idx = 0;
    if (pipes[0].x + 28 < BIRD_X - 6) {
        target_idx = 1;
    } else if (pipes[1].x + 28 < BIRD_X - 6) {
        target_idx = 0;
    } else {
        target_idx = (pipes[0].x < pipes[1].x) ? 0 : 1;
    }
    
    FB_Pipe *target = &pipes[target_idx];
    int by_pixels = FROM_FP(bird_y);

    // AI logic: Jump if below target gap center + 10 pixels
    bool flap = false;
    if (by_pixels > target->gap_y + 10) {
        flap = true;
    }

    if (flap) {
        bird_vy = JUMP_IMPULSE;
    }

    // Apply Physics
    bird_vy += GRAVITY;
    if (bird_vy > TERM_VELOCITY) bird_vy = TERM_VELOCITY;
    bird_y += bird_vy;
    by_pixels = FROM_FP(bird_y);

    // Ground and Ceiling collision
    if (by_pixels < 10) {
        bird_y = TO_FP(10);
        bird_vy = 0;
        by_pixels = 10;
    }
    if (by_pixels > 212) {
        init_flappy_bird();
        return;
    }

    // 3. Update & Draw Pipes
    for (int i = 0; i < 2; i++) {
        pipes[i].x -= 2;

        if (pipes[i].x < -30) {
            pipes[i].x = 330;
            pipes[i].gap_y = 90 + (get_rand() % 60);
            pipes[i].passed = false;
        }

        // Scoring
        if (!pipes[i].passed && pipes[i].x + 14 < BIRD_X) {
            pipes[i].passed = true;
            flappy_score++;
        }

        int px = pipes[i].x;
        int gy = pipes[i].gap_y;

        // Draw Pipes (Green 2 with white highlights and black shadow outlines)
        // Top Pipe
        draw_rect(buffer, px, 0, px + 24, gy - GAP_HEIGHT / 2 - 1, 2); // base body
        draw_rect(buffer, px + 2, 0, px + 3, gy - GAP_HEIGHT / 2 - 1, 7); // base highlight
        draw_rect(buffer, px + 22, 0, px + 23, gy - GAP_HEIGHT / 2 - 1, 0); // base shadow
        
        draw_rect(buffer, px - 2, gy - GAP_HEIGHT / 2 - 12, px + 26, gy - GAP_HEIGHT / 2 - 1, 2); // lip body
        draw_rect(buffer, px, gy - GAP_HEIGHT / 2 - 11, px + 1, gy - GAP_HEIGHT / 2 - 2, 7); // lip highlight
        draw_rect(buffer, px + 23, gy - GAP_HEIGHT / 2 - 11, px + 24, gy - GAP_HEIGHT / 2 - 2, 0); // lip shadow
        draw_rect(buffer, px - 2, gy - GAP_HEIGHT / 2 - 1, px + 26, gy - GAP_HEIGHT / 2 - 1, 0); // lip bottom outline
        
        // Bottom Pipe
        draw_rect(buffer, px, gy + GAP_HEIGHT / 2, px + 24, 219, 2); // base body
        draw_rect(buffer, px + 2, gy + GAP_HEIGHT / 2, px + 3, 219, 7); // base highlight
        draw_rect(buffer, px + 22, gy + GAP_HEIGHT / 2, px + 23, 219, 0); // base shadow
        
        draw_rect(buffer, px - 2, gy + GAP_HEIGHT / 2, px + 26, gy + GAP_HEIGHT / 2 + 11, 2); // lip body
        draw_rect(buffer, px, gy + GAP_HEIGHT / 2 + 1, px + 1, gy + GAP_HEIGHT / 2 + 10, 7); // lip highlight
        draw_rect(buffer, px + 23, gy + GAP_HEIGHT / 2 + 1, px + 24, gy + GAP_HEIGHT / 2 + 10, 0); // lip shadow
        draw_rect(buffer, px - 2, gy + GAP_HEIGHT / 2, px + 26, gy + GAP_HEIGHT / 2, 0); // lip top outline
        draw_rect(buffer, px - 2, gy + GAP_HEIGHT / 2 + 11, px + 26, gy + GAP_HEIGHT / 2 + 11, 0); // lip bottom outline

        // Collision Check: Bird is circle at (BIRD_X, by_pixels) with radius 6
        if (BIRD_X + 6 >= px - 2 && BIRD_X - 6 <= px + 26) {
            if (by_pixels - 6 <= gy - GAP_HEIGHT / 2) {
                // Collided top
                init_flappy_bird();
                return;
            }
            if (by_pixels + 6 >= gy + GAP_HEIGHT / 2) {
                // Collided bottom
                init_flappy_bird();
                return;
            }
        }
    }

    // 4. Draw Flappy Bird with a cleaner pixel-art sprite
    bool wing_up = (bird_vy < 0) ? ((frame_counter / 3) & 1) == 0 : ((frame_counter / 6) & 1) == 0;
    const char *const *bird_sprite = wing_up ? FLAPPY_BIRD_WING_UP : FLAPPY_BIRD_WING_DOWN;
    draw_flappy_bird_sprite(buffer, BIRD_X - 8, by_pixels - 6, bird_sprite);

    // Draw HUD Score
    draw_score(buffer, flappy_score, 10, 10, 2, 7);
}
