#include "common.h"

// --- Constants ---
#define LEVEL_WIDTH     1000
#define CAMERA_MAX      (LEVEL_WIDTH - 320)
#define NUM_BLOCKS      6
#define NUM_AIR_COINS   5
#define NUM_GOOMBAS     3

// --- Structs ---
typedef struct {
    int x, y;
    bool is_q_block;
    bool is_hit;
} Block;

typedef struct {
    int x, y;
    bool active;
} AirCoin;

typedef struct {
    int x, y;
    int vx;
    bool alive;
    int squish_timer;
} Goomba;

typedef enum { KOOPA_WALK, KOOPA_SHELL, KOOPA_SLIDE } KoopaState;
typedef struct {
    int x, y;
    int vx;
    KoopaState state;
} Koopa;

// --- State Variables ---
static int mario_x = 20;
static int mario_y = 210;
static int mario_vx = 2;
static int mario_vy = 0;
static bool mario_is_jumping = false;
static int mario_anim_frame = 0;
static int mario_score = 0;
static int camera_x = 0;

static Block blocks[NUM_BLOCKS];
static AirCoin air_coins[NUM_AIR_COINS];
static Goomba goombas[NUM_GOOMBAS];
static Koopa koopa;

// Rising coin animation
static int coin_anim_x = 0;
static int coin_anim_y = 0;
static int coin_anim_timer = 0;

// Flagpole state
static enum { MARIO_PLAYING, MARIO_FLAG_SLIDE, MARIO_CASTLE_RUN, MARIO_LEVEL_END } level_state;
static int level_end_timer = 0;

void init_mario_nes() {
    mario_x = 20;
    mario_y = 210;
    mario_vx = 2;
    mario_vy = 0;
    mario_is_jumping = false;
    mario_anim_frame = 0;
    mario_score = 0;
    camera_x = 0;
    coin_anim_timer = 0;
    level_state = MARIO_PLAYING;
    level_end_timer = 0;

    // Initialize Blocks
    blocks[0] = (Block){180, 140, true, false};
    blocks[1] = (Block){200, 140, false, false};
    blocks[2] = (Block){220, 140, true, false};
    blocks[3] = (Block){480, 130, false, false};
    blocks[4] = (Block){500, 130, true, false};
    blocks[5] = (Block){520, 130, false, false};

    // Initialize Air Coins
    air_coins[0] = (AirCoin){300, 160, true};
    air_coins[1] = (AirCoin){320, 150, true};
    air_coins[2] = (AirCoin){340, 160, true};
    air_coins[3] = (AirCoin){640, 150, true};
    air_coins[4] = (AirCoin){660, 150, true};

    // Initialize Goombas
    goombas[0] = (Goomba){320, 210, -1, true, 0};
    goombas[1] = (Goomba){620, 210, -1, true, 0};
    goombas[2] = (Goomba){760, 210, -1, true, 0};

    // Initialize Koopa
    koopa = (Koopa){540, 210, -1, KOOPA_WALK};
}

void play_mario_nes(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);
    
    // Sky blue background
    draw_rect(buffer, 0, 0, 319, 218, 6);
    
    // Draw clouds (parallax scrolling)
    for (int cx = 100; cx < 1000; cx += 300) {
        int px = cx - camera_x / 2;
        if (px >= -30 && px < 320) {
            draw_circle(buffer, px, 50, 12, 7);
            draw_circle(buffer, px + 10, 50, 10, 7);
            draw_circle(buffer, px - 10, 50, 10, 7);
        }
    }

    // Camera follow Mario
    if (level_state == MARIO_PLAYING) {
        camera_x = mario_x - 120;
        if (camera_x < 0) camera_x = 0;
        if (camera_x > CAMERA_MAX) camera_x = CAMERA_MAX;
    }

    // Draw Ground
    int ground_scroll = camera_x % 16;
    for (int x = -16; x < 336; x += 16) {
        int px = x - ground_scroll;
        draw_rect(buffer, px, 219, px + 14, 239, 1);
        draw_rect(buffer, px + 1, 220, px + 13, 238, 3);
    }

    // Draw Green Pipes (using the Flappy Bird style)
    int pipes_x[3] = {240, 410, 700};
    int pipes_h[3] = {32, 48, 32};
    for (int i = 0; i < 3; i++) {
        int px = pipes_x[i] - camera_x;
        int py = 219 - pipes_h[i];
        if (px >= -20 && px < 320) {
            // Draw Pipe Base
            draw_rect(buffer, px - 10, py + 8, px + 10, 218, 2);       // base body (Green 2)
            draw_rect(buffer, px - 8,  py + 8, px - 7,  218, 7);       // base highlight (White 7)
            draw_rect(buffer, px + 8,  py + 8, px + 9,  218, 0);       // base shadow (Black 0)

            // Draw Pipe Lip
            draw_rect(buffer, px - 12, py, px + 12, py + 7, 2);        // lip body (Green 2)
            draw_rect(buffer, px - 10, py + 1, px - 9, py + 6, 7);     // lip highlight (White 7)
            draw_rect(buffer, px + 9, py + 1, px + 10, py + 6, 0);     // lip shadow (Black 0)
            draw_rect(buffer, px - 12, py + 7, px + 12, py + 7, 0);    // lip bottom outline (Black 0)
            draw_rect(buffer, px - 12, py, px + 12, py, 0);            // lip top outline (Black 0)
        }
    }

    // Draw Flagpole
    int pole_x = 880 - camera_x;
    if (pole_x >= -20 && pole_x < 320) {
        draw_rect(buffer, pole_x - 1, 70, pole_x + 1, 218, 2); // Green pole
        draw_circle(buffer, pole_x, 67, 3, 3);                 // Yellow ball top
        // Draw Flag
        int flag_y = (level_state == MARIO_FLAG_SLIDE) ? mario_y - 12 : 76;
        if (flag_y > 200) flag_y = 200;
        draw_rect(buffer, pole_x - 16, flag_y, pole_x - 2, flag_y + 10, 7); // White flag
        draw_rect(buffer, pole_x - 12, flag_y + 3, pole_x - 6, flag_y + 7, 1); // Red emblem spot
    }

    // Draw Castle
    int castle_x = 920 - camera_x;
    if (castle_x >= -60 && castle_x < 320) {
        draw_rect(buffer, castle_x, 160, castle_x + 50, 218, 1);       // Main wall
        draw_rect(buffer, castle_x + 10, 140, castle_x + 40, 160, 1);  // Upper tower
        // Battlements
        for (int bx = 0; bx < 50; bx += 10) {
            draw_rect(buffer, castle_x + bx, 156, castle_x + bx + 5, 159, 1);
        }
        draw_rect(buffer, castle_x + 20, 188, castle_x + 30, 218, 0); // Doorway
    }

    // --- PHYSICS & INTERACTION ---
    if (level_state == MARIO_PLAYING) {
        mario_x += mario_vx;
        
        // Jump check
        bool need_jump = false;
        
        // Pipe obstruction check
        for (int i = 0; i < 3; i++) {
            if (mario_x < pipes_x[i] && mario_x + 8 >= pipes_x[i] - 10 && mario_y >= 218 - pipes_h[i]) {
                // Obstructed! Back off and jump
                mario_x = pipes_x[i] - 10 - 8;
                need_jump = true;
            }
        }

        // Jump over upcoming enemies
        for (int i = 0; i < NUM_GOOMBAS; i++) {
            if (goombas[i].alive && abs(goombas[i].x - mario_x) < 45 && mario_y >= 210) {
                need_jump = true;
            }
        }
        if (koopa.state == KOOPA_WALK && abs(koopa.x - mario_x) < 45 && mario_y >= 210) {
            need_jump = true;
        }

        if (need_jump && !mario_is_jumping) {
            mario_vy = -9;
            mario_is_jumping = true;
        }

        // Apply Gravity
        mario_y += mario_vy;
        mario_vy += 1; // gravity acceleration

        // Ground landing
        if (mario_y >= 210) {
            mario_y = 210;
            mario_vy = 0;
            mario_is_jumping = false;
        }

        // Stand/Hit Blocks check
        bool on_block = false;
        for (int i = 0; i < NUM_BLOCKS; i++) {
            int bx1 = blocks[i].x;
            int bx2 = blocks[i].x + 16;
            int by = blocks[i].y;
            
            // Horizontal overlap
            if (mario_x >= bx1 - 3 && mario_x <= bx2 + 3) {
                // Head hit from below
                if (mario_vy < 0 && mario_y - 9 <= by + 16 && mario_y - 9 >= by + 8) {
                    mario_y = by + 16 + 9;
                    mario_vy = 0;
                    if (!blocks[i].is_hit) {
                        blocks[i].is_hit = true;
                        mario_score += 200;
                        // Trigger coin pop animation
                        coin_anim_x = blocks[i].x + 4;
                        coin_anim_y = blocks[i].y - 8;
                        coin_anim_timer = 15;
                    }
                }
                // Landing on block top
                else if (mario_vy >= 0 && mario_y + 3 >= by && mario_y - 9 <= by) {
                    mario_y = by - 4;
                    mario_vy = 0;
                    mario_is_jumping = false;
                    on_block = true;
                }
            }
        }

        // Stand on pipes check
        for (int i = 0; i < 3; i++) {
            int py = 218 - pipes_h[i];
            if (mario_x >= pipes_x[i] - 14 && mario_x <= pipes_x[i] + 14) {
                if (mario_vy >= 0 && mario_y + 3 >= py && mario_y - 9 <= py) {
                    mario_y = py - 4;
                    mario_vy = 0;
                    mario_is_jumping = false;
                }
            }
        }

        // Collect air coins
        for (int i = 0; i < NUM_AIR_COINS; i++) {
            if (air_coins[i].active && abs(mario_x - air_coins[i].x) < 10 && abs(mario_y - air_coins[i].y) < 16) {
                air_coins[i].active = false;
                mario_score += 100;
            }
        }

        // Reach flagpole check
        if (mario_x >= 880) {
            mario_x = 880;
            mario_vx = 0;
            mario_vy = 2; // Slide down velocity
            level_state = MARIO_FLAG_SLIDE;
            mario_score += 500;
        }

        // Animation frame
        if (!mario_is_jumping) {
            mario_anim_frame = (frame_counter / 4) % 4;
        } else {
            mario_anim_frame = 2; // Jump frame
        }

    } else if (level_state == MARIO_FLAG_SLIDE) {
        mario_y += mario_vy;
        if (mario_y >= 210) {
            mario_y = 210;
            mario_vx = 2; // Start running to castle
            level_state = MARIO_CASTLE_RUN;
        }
    } else if (level_state == MARIO_CASTLE_RUN) {
        mario_x += mario_vx;
        mario_anim_frame = (frame_counter / 4) % 4;
        if (mario_x >= 945) { // Enters door
            level_state = MARIO_LEVEL_END;
            level_end_timer = 40; // hold before reset
        }
    } else if (level_state == MARIO_LEVEL_END) {
        level_end_timer--;
        if (level_end_timer <= 0) {
            init_mario_nes();
        }
    }

    // --- DRAW BLOCKS & ITEMS ---
    for (int i = 0; i < NUM_BLOCKS; i++) {
        int bx = blocks[i].x - camera_x;
        if (bx >= -16 && bx < 320) {
            uint8_t color = blocks[i].is_hit ? 1 : (blocks[i].is_q_block ? 3 : 5);
            draw_rect(buffer, bx, blocks[i].y, bx + 15, blocks[i].y + 15, color);
            // Draw [?] inside question blocks
            if (blocks[i].is_q_block && !blocks[i].is_hit) {
                draw_rect(buffer, bx + 6, blocks[i].y + 4, bx + 10, blocks[i].y + 5, 7);
                draw_rect(buffer, bx + 8, blocks[i].y + 6, bx + 9, blocks[i].y + 11, 7);
            }
        }
    }

    // Draw Air Coins
    for (int i = 0; i < NUM_AIR_COINS; i++) {
        if (air_coins[i].active) {
            int cx = air_coins[i].x - camera_x;
            if (cx >= -10 && cx < 320) {
                draw_circle(buffer, cx, air_coins[i].y, 3, 3); // Yellow coin
                draw_rect(buffer, cx - 1, air_coins[i].y - 2, cx + 1, air_coins[i].y + 2, 7);
            }
        }
    }

    // Draw Rising Coin Animation
    if (coin_anim_timer > 0) {
        coin_anim_timer--;
        int cx = coin_anim_x - camera_x;
        int cy = coin_anim_y - (15 - coin_anim_timer);
        if (cx >= -10 && cx < 320) {
            draw_circle(buffer, cx + 3, cy, 3, 3);
            draw_rect(buffer, cx + 2, cy - 2, cx + 4, cy + 2, 7);
        }
    }

    // --- ENEMIES UPDATES & DRAW ---
    // Goombas
    for (int i = 0; i < NUM_GOOMBAS; i++) {
        Goomba *g = &goombas[i];
        if (g->alive) {
            g->x += g->vx;
            // Bound checks / bounce back on pipes
            for (int p = 0; p < 3; p++) {
                if (abs(g->x - pipes_x[p]) < 18) {
                    g->vx = -g->vx;
                    g->x += g->vx * 2;
                }
            }

            int gx = g->x - camera_x;
            if (gx >= -16 && gx < 320) {
                // Draw Goomba
                draw_circle(buffer, gx, g->y, 6, 1); // Brown body
                draw_rect(buffer, gx - 4, g->y + 2, gx + 4, g->y + 5, 5); // Feet
                draw_pixel(buffer, gx - 2, g->y - 2, 7); // Eyes
                draw_pixel(buffer, gx + 2, g->y - 2, 7);
            }

            // Collision with Mario
            if (level_state == MARIO_PLAYING && abs(mario_x - g->x) < 12 && abs(mario_y - g->y) < 12) {
                // If Mario is falling onto Goomba
                if (mario_vy > 0 && mario_y < g->y - 2) {
                    g->alive = false;
                    g->squish_timer = 20; // squished frame
                    mario_vy = -5; // Bounce Mario
                    mario_score += 100;
                } else {
                    // Hurt Mario
                    mario_score = (mario_score > 50) ? mario_score - 50 : 0;
                    g->alive = false; // remove goomba to avoid infinite damage
                }
            }
        } else if (g->squish_timer > 0) {
            g->squish_timer--;
            int gx = g->x - camera_x;
            if (gx >= -16 && gx < 320) {
                draw_rect(buffer, gx - 8, g->y + 3, gx + 8, g->y + 5, 1); // squished flat
            }
        }
    }

    // Koopa
    if (koopa.x > -20 && koopa.x < 1020) {
        // Physics update
        if (koopa.state == KOOPA_WALK) {
            koopa.x += koopa.vx;
            for (int p = 0; p < 3; p++) {
                if (abs(koopa.x - pipes_x[p]) < 18) {
                    koopa.vx = -koopa.vx;
                    koopa.x += koopa.vx * 2;
                }
            }
        } else if (koopa.state == KOOPA_SLIDE) {
            koopa.x += koopa.vx;
            // Bounce shell off screen bounds or pipes
            if (koopa.x < 10) { koopa.x = 10; koopa.vx = -koopa.vx; }
            if (koopa.x > 990) { koopa.x = 990; koopa.vx = -koopa.vx; }
            for (int p = 0; p < 3; p++) {
                if (abs(koopa.x - pipes_x[p]) < 18) {
                    koopa.vx = -koopa.vx;
                    koopa.x += koopa.vx * 2;
                }
            }

            // Shell slides and kills Goombas!
            for (int i = 0; i < NUM_GOOMBAS; i++) {
                if (goombas[i].alive && abs(koopa.x - goombas[i].x) < 14) {
                    goombas[i].alive = false;
                    goombas[i].squish_timer = 20;
                    mario_score += 200;
                }
            }
        }

        // Draw Koopa
        int kx = koopa.x - camera_x;
        if (kx >= -16 && kx < 320) {
            if (koopa.state == KOOPA_WALK) {
                draw_circle(buffer, kx, koopa.y - 4, 5, 2); // Green shell body
                draw_circle(buffer, kx + (koopa.vx > 0 ? 4 : -4), koopa.y - 9, 3, 3); // Yellow head
                draw_rect(buffer, kx - 2, koopa.y + 1, kx - 1, koopa.y + 5, 3); // Legs
                draw_rect(buffer, kx + 1, koopa.y + 1, kx + 2, koopa.y + 5, 3);
            } else { // Shell/Slide Shell
                draw_circle(buffer, kx, koopa.y + 1, 6, 2); // Green shell
                draw_rect(buffer, kx - 4, koopa.y - 2, kx + 4, koopa.y - 1, 7); // White shell trim
            }
        }

        // Collision with Mario
        if (level_state == MARIO_PLAYING && abs(mario_x - koopa.x) < 14 && abs(mario_y - koopa.y) < 14) {
            if (mario_vy > 0 && mario_y < koopa.y - 2) {
                // Jump on Koopa
                if (koopa.state == KOOPA_WALK) {
                    koopa.state = KOOPA_SHELL;
                    koopa.vx = 0;
                    mario_vy = -6; // bounce
                    mario_score += 100;
                } else if (koopa.state == KOOPA_SHELL) {
                    koopa.state = KOOPA_SLIDE;
                    koopa.vx = (mario_x < koopa.x) ? 5 : -5;
                    mario_vy = -6; // bounce
                } else if (koopa.state == KOOPA_SLIDE) {
                    koopa.state = KOOPA_SHELL;
                    koopa.vx = 0;
                    mario_vy = -6; // bounce
                }
            } else {
                // Side touch
                if (koopa.state == KOOPA_SHELL) {
                    // Kick shell
                    koopa.state = KOOPA_SLIDE;
                    koopa.vx = (mario_x < koopa.x) ? 5 : -5;
                    mario_x -= 10; // push Mario back slightly
                } else if (koopa.state == KOOPA_SLIDE) {
                    // Slide hurts Mario
                    mario_score = (mario_score > 50) ? mario_score - 50 : 0;
                    koopa.x = mario_x - 50; // push koopa back
                } else {
                    // Walk hurts Mario
                    mario_score = (mario_score > 50) ? mario_score - 50 : 0;
                    koopa.vx = -koopa.vx; // turn back
                    koopa.x += koopa.vx * 10;
                }
            }
        }
    }

    // --- DRAW MARIO ---
    if (level_state != MARIO_LEVEL_END) {
        int mx = mario_x - camera_x;
        draw_pro_mario(buffer, mx, mario_y);
    }

    // Draw HUD Score
    draw_score(buffer, mario_score, 10, 10, 2, 7);
}
