#include "common.h"

// Squid Sprites (Row 0)
static const uint16_t SQUID_SPRITE[2][8] = {
    {0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x24, 0x5A, 0xA5}, // Frame A
    {0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x18, 0x66, 0x99}  // Frame B
};

// Crab Sprites (Row 1)
static const uint16_t CRAB_SPRITE[2][8] = {
    {0x084, 0x048, 0x0FC, 0x1BB, 0x3FF, 0x2FD, 0x285, 0x06C}, // Frame A
    {0x084, 0x449, 0x4FD, 0x7BB, 0x3FE, 0x1FC, 0x084, 0x306}  // Frame B
};

// Octopus Sprites (Row 2)
static const uint16_t OCTOPUS_SPRITE[2][8] = {
    {0x0F0, 0x3FC, 0x7FE, 0xE67, 0xFFF, 0x198, 0x30C, 0xC03}, // Frame A
    {0x0F0, 0x3FC, 0x7FE, 0xE67, 0xFFF, 0x3FC, 0x606, 0x30C}  // Frame B
};

// Player Ship Sprite
static const uint16_t PLAYER_SPRITE[8] = {
    0x0040, 0x0040, 0x00E0, 0x0FFE, 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF
};

// Mystery UFO Sprite
static const uint16_t UFO_SPRITE[8] = {
    0x07E0, 0x1FF8, 0x3FFC, 0x6DB6, 0xFFFF, 0x1C70, 0x0820, 0x0000
};

// --- Game Variables ---
static int inv_x = 80, inv_y = 40;
static int inv_dir = 1;
static bool inv_alive[3][5];
static int inv_player_x = 160;
static int inv_laser_x = 0, inv_laser_y = 0;
static bool inv_laser_active = false;

// Multiple Enemy Lasers
#define MAX_ENEMY_LASERS 3
static int inv_enemy_laser_x[MAX_ENEMY_LASERS];
static int inv_enemy_laser_y[MAX_ENEMY_LASERS];
static bool inv_enemy_laser_active[MAX_ENEMY_LASERS];

// Bunkers
#define BUNKER_COLS 6
#define BUNKER_ROWS 4
#define BLOCK_W 5
#define BLOCK_H 4

static const bool BUNKER_SHAPE[BUNKER_ROWS][BUNKER_COLS] = {
    {0, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1},
    {1, 1, 0, 0, 1, 1},
    {1, 0, 0, 0, 0, 1}
};

static uint8_t bunker_blocks[3][BUNKER_ROWS][BUNKER_COLS];
static const int bunker_positions[3] = {70, 150, 230};

static int invaders_score = 0;
static int inv_lives = 3;

// Starfield
typedef struct {
    int x, y;
    uint8_t color;
    int speed;
} Star;

static Star stars[25];

// Particle System
#define MAX_PARTICLES 40
typedef struct {
    bool active;
    int x, y; // scaled by 16
    int dx, dy; // scaled by 16
    int life;
    uint8_t color;
} Particle;

static Particle particles[MAX_PARTICLES];

// Screen Shake
static int shake_timer = 0;
static int shake_x = 0;
static int shake_y = 0;

// Game State Flow
typedef enum {
    PLAY_STATE_RUNNING,
    PLAY_STATE_PLAYER_EXPLODING,
    PLAY_STATE_GAME_OVER
} PlayState;

static PlayState game_state;
static int state_timer = 0;

// UFO
static int ufo_x = 0;
static int ufo_y = 25;
static int ufo_dir = 1;
static bool ufo_active = false;
static int ufo_timer = 0;

// --- Helper Functions ---
static void update_shake() {
    if (shake_timer > 0) {
        shake_timer--;
        shake_x = (get_rand() % 5) - 2;
        shake_y = (get_rand() % 5) - 2;
    } else {
        shake_x = 0;
        shake_y = 0;
    }
}

static void draw_pixel_s(uint8_t *buffer, int x, int y, uint8_t color) {
    draw_pixel(buffer, x + shake_x, y + shake_y, color);
}

static void draw_rect_s(uint8_t *buffer, int x1, int y1, int x2, int y2, uint8_t color) {
    draw_rect(buffer, x1 + shake_x, y1 + shake_y, x2 + shake_x, y2 + shake_y, color);
}

static void draw_sprite_centered_s(uint8_t *buffer, int cx, int cy, const uint16_t *sprite, int width, int height, uint8_t color) {
    int start_x = cx - width / 2 + shake_x;
    int start_y = cy - height / 2 + shake_y;
    for (int r = 0; r < height; r++) {
        uint16_t row_data = sprite[r];
        for (int c = 0; c < width; c++) {
            if ((row_data >> (width - 1 - c)) & 1) {
                draw_pixel(buffer, start_x + c, start_y + r, color);
            }
        }
    }
}

static void draw_score_s(uint8_t *buffer, int score, int x, int y, int scale, uint8_t color) {
    draw_score(buffer, score, x + shake_x, y + shake_y, scale, color);
}

static void draw_string_s(uint8_t *buffer, const char *str, int x, int y, int scale, uint8_t color) {
    draw_string(buffer, str, x + shake_x, y + shake_y, scale, color);
}

static void init_stars() {
    for (int i = 0; i < 25; i++) {
        stars[i].x = get_rand() % 320;
        stars[i].y = get_rand() % 240;
        stars[i].speed = (get_rand() % 2) + 1;
        stars[i].color = (get_rand() % 3 == 0) ? 7 : ((get_rand() % 2 == 0) ? 6 : 4);
    }
}

static void update_and_draw_stars(uint8_t *buffer) {
    for (int i = 0; i < 25; i++) {
        stars[i].y += stars[i].speed;
        if (stars[i].y >= 240) {
            stars[i].y = 0;
            stars[i].x = get_rand() % 320;
        }
        draw_pixel_s(buffer, stars[i].x, stars[i].y, stars[i].color);
    }
}

static void init_bunkers() {
    for (int b = 0; b < 3; b++) {
        for (int r = 0; r < BUNKER_ROWS; r++) {
            for (int c = 0; c < BUNKER_COLS; c++) {
                bunker_blocks[b][r][c] = BUNKER_SHAPE[r][c] ? 3 : 0;
            }
        }
    }
}

static void draw_bunkers(uint8_t *buffer) {
    for (int b = 0; b < 3; b++) {
        int bx = bunker_positions[b] - 15;
        int by = 180;
        for (int r = 0; r < BUNKER_ROWS; r++) {
            for (int c = 0; c < BUNKER_COLS; c++) {
                int hp = bunker_blocks[b][r][c];
                if (hp > 0) {
                    uint8_t color = (hp == 3) ? 2 : ((hp == 2) ? 3 : 1);
                    draw_rect_s(buffer, bx + c * BLOCK_W, by + r * BLOCK_H, 
                                bx + (c + 1) * BLOCK_W - 1, by + (r + 1) * BLOCK_H - 1, color);
                }
            }
        }
    }
}

static void init_particles() {
    for (int p = 0; p < MAX_PARTICLES; p++) {
        particles[p].active = false;
    }
}

static void spawn_particles(int x, int y, uint8_t color, int count) {
    for (int i = 0; i < count; i++) {
        int found = -1;
        for (int p = 0; p < MAX_PARTICLES; p++) {
            if (!particles[p].active) {
                found = p;
                break;
            }
        }
        if (found == -1) break;
        particles[found].active = true;
        particles[found].x = x * 16;
        particles[found].y = y * 16;
        particles[found].dx = (get_rand() % 65) - 32;
        particles[found].dy = (get_rand() % 65) - 32;
        particles[found].life = 10 + (get_rand() % 15);
        particles[found].color = color;
    }
}

static void update_and_draw_particles(uint8_t *buffer) {
    for (int p = 0; p < MAX_PARTICLES; p++) {
        if (particles[p].active) {
            particles[p].x += particles[p].dx;
            particles[p].y += particles[p].dy;
            particles[p].life--;
            if (particles[p].life <= 0) {
                particles[p].active = false;
            } else {
                int px = particles[p].x / 16;
                int py = particles[p].y / 16;
                uint8_t color = (particles[p].life < 5) ? 1 : particles[p].color;
                draw_pixel_s(buffer, px, py, color);
            }
        }
    }
}

static void init_enemy_lasers() {
    for (int i = 0; i < MAX_ENEMY_LASERS; i++) {
        inv_enemy_laser_active[i] = false;
    }
}

// --- Main API Functions ---
void init_invaders() {
    inv_x = 80; inv_y = 40;
    inv_dir = 1;
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 5; c++) inv_alive[r][c] = true;
    }
    inv_player_x = 160;
    inv_laser_active = false;
    init_enemy_lasers();
    init_bunkers();
    init_stars();
    init_particles();
    invaders_score = 0;
    inv_lives = 3;
    game_state = PLAY_STATE_RUNNING;
    state_timer = 0;
    shake_timer = 0;
    shake_x = 0;
    shake_y = 0;
    ufo_active = false;
    ufo_timer = 200 + (get_rand() % 200);
}

void play_invaders(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);
    update_shake();
    update_and_draw_stars(buffer);
    
    // Draw HUD
    draw_score_s(buffer, invaders_score, 10, 10, 2, 7);
    char lives_str[16];
    sprintf(lives_str, "LIVES %d", inv_lives);
    draw_string_s(buffer, lives_str, 240, 10, 2, 2);
    
    if (game_state == PLAY_STATE_RUNNING) {
        // Check win/loss
        bool all_dead = true;
        int alive_count = 0;
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 5; c++) {
                if (inv_alive[r][c]) {
                    all_dead = false;
                    alive_count++;
                }
            }
        }
        
        if (all_dead) {
            invaders_score += 500;
            inv_x = 80; inv_y = 40;
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 5; c++) inv_alive[r][c] = true;
            }
            init_bunkers();
            ufo_active = false;
            ufo_timer = 200 + (get_rand() % 200);
            return;
        }
        
        if (inv_y > 170) {
            game_state = PLAY_STATE_GAME_OVER;
            state_timer = 120;
            return;
        }
        
        // Invaders Movement Speed Tuning
        int move_interval = 3;
        if (alive_count > 10) move_interval = 3;
        else if (alive_count > 5) move_interval = 2;
        else move_interval = 1;
        
        int step_size = 1;
        if (alive_count == 1) step_size = 2;
        
        if (frame_counter % move_interval == 0) {
            inv_x += inv_dir * step_size;
            if (inv_x > 200) { inv_dir = -1; inv_y += 8; }
            else if (inv_x < 20) { inv_dir = 1; inv_y += 8; }
        }
        
        // Draw Invaders
        int wave = (frame_counter / 16) % 2;
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 5; c++) {
                if (inv_alive[r][c]) {
                    int ax = inv_x + c * 20;
                    int ay = inv_y + r * 14;
                    uint8_t color = (r == 0) ? 1 : ((r == 1) ? 5 : 6);
                    const uint16_t *sprite = NULL;
                    int width = 8;
                    if (r == 0) {
                        sprite = SQUID_SPRITE[wave];
                        width = 8;
                    } else if (r == 1) {
                        sprite = CRAB_SPRITE[wave];
                        width = 11;
                    } else {
                        sprite = OCTOPUS_SPRITE[wave];
                        width = 12;
                    }
                    draw_sprite_centered_s(buffer, ax, ay, sprite, width, 8, color);
                }
            }
        }
        
        // UFO Spawning & Movement
        if (!ufo_active) {
            ufo_timer--;
            if (ufo_timer <= 0) {
                ufo_active = true;
                ufo_dir = (get_rand() % 2 == 0) ? 1 : -1;
                ufo_x = (ufo_dir == 1) ? -10 : 330;
            }
        } else {
            ufo_x += ufo_dir * 2;
            if ((ufo_dir == 1 && ufo_x > 330) || (ufo_dir == -1 && ufo_x < -10)) {
                ufo_active = false;
                ufo_timer = 300 + (get_rand() % 300);
            } else {
                draw_sprite_centered_s(buffer, ufo_x, ufo_y, UFO_SPRITE, 16, 8, 1);
            }
        }
        
        // Draw Bunkers
        draw_bunkers(buffer);
        
        // AI Pathfinding & Targeting
        int target_x = inv_player_x;
        if (ufo_active) {
            target_x = ufo_x;
        } else {
            int target_col = -1;
            int max_y = -1;
            for (int c = 0; c < 5; c++) {
                for (int r = 2; r >= 0; r--) {
                    if (inv_alive[r][c]) {
                        int ay = inv_y + r * 14;
                        if (ay > max_y) {
                            max_y = ay;
                            target_col = c;
                        }
                        break;
                    }
                }
            }
            if (target_col != -1) {
                target_x = inv_x + target_col * 20;
            }
        }
        
        // AI Dodge Calculation
        int dodge_offset = 0;
        for (int l = 0; l < MAX_ENEMY_LASERS; l++) {
            if (inv_enemy_laser_active[l] && inv_enemy_laser_y[l] > 100 && inv_enemy_laser_y[l] < 210) {
                bool blocked = false;
                for (int b = 0; b < 3; b++) {
                    int bx = bunker_positions[b] - 15;
                    if (inv_enemy_laser_x[l] >= bx && inv_enemy_laser_x[l] < bx + (BUNKER_COLS * BLOCK_W)) {
                        int col = (inv_enemy_laser_x[l] - bx) / BLOCK_W;
                        if (col >= 0 && col < BUNKER_COLS) {
                            for (int row = 0; row < BUNKER_ROWS; row++) {
                                if (bunker_blocks[b][row][col] > 0) {
                                    blocked = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (blocked) break;
                }
                
                if (!blocked && abs(inv_enemy_laser_x[l] - inv_player_x) < 16) {
                    if (inv_enemy_laser_x[l] < inv_player_x) {
                        dodge_offset = 15;
                    } else {
                        dodge_offset = -15;
                    }
                    break;
                }
            }
        }
        
        int final_target_x = target_x;
        if (dodge_offset != 0) {
            final_target_x = inv_player_x + dodge_offset;
        }
        
        if (final_target_x < 20) final_target_x = 20;
        if (final_target_x > 300) final_target_x = 300;
        
        if (inv_player_x < final_target_x) inv_player_x += 2;
        else if (inv_player_x > final_target_x) inv_player_x -= 2;
        
        // Draw Player
        draw_sprite_centered_s(buffer, inv_player_x, 210, PLAYER_SPRITE, 13, 8, 2);
        
        // Shoot logic (Avoid hitting own bunkers)
        bool bunker_above = false;
        for (int b = 0; b < 3; b++) {
            int bx = bunker_positions[b] - 15;
            if (inv_player_x >= bx && inv_player_x < bx + (BUNKER_COLS * BLOCK_W)) {
                int col = (inv_player_x - bx) / BLOCK_W;
                if (col >= 0 && col < BUNKER_COLS) {
                    for (int row = 0; row < BUNKER_ROWS; row++) {
                        if (bunker_blocks[b][row][col] > 0) {
                            bunker_above = true;
                            break;
                        }
                    }
                }
            }
            if (bunker_above) break;
        }
        
        if (!inv_laser_active && !bunker_above) {
            inv_laser_active = true;
            inv_laser_x = inv_player_x;
            inv_laser_y = 204;
        }
        
        // Player Laser update & collision
        if (inv_laser_active) {
            inv_laser_y -= 4;
            draw_rect_s(buffer, inv_laser_x - 1, inv_laser_y - 2, inv_laser_x + 1, inv_laser_y + 2, 3);
            
            // Hit Bunkers
            bool hit_bunker = false;
            int by_top = 180;
            for (int b = 0; b < 3; b++) {
                int bx = bunker_positions[b] - 15;
                if (inv_laser_x >= bx && inv_laser_x < bx + (BUNKER_COLS * BLOCK_W) && 
                    inv_laser_y >= by_top && inv_laser_y < by_top + (BUNKER_ROWS * BLOCK_H)) {
                    
                    int col = (inv_laser_x - bx) / BLOCK_W;
                    if (col >= 0 && col < BUNKER_COLS) {
                        for (int row = BUNKER_ROWS - 1; row >= 0; row--) {
                            if (bunker_blocks[b][row][col] > 0) {
                                bunker_blocks[b][row][col]--;
                                inv_laser_active = false;
                                spawn_particles(inv_laser_x, inv_laser_y, 3, 5);
                                hit_bunker = true;
                                break;
                            }
                        }
                    }
                }
                if (hit_bunker) break;
            }
            
            // Hit UFO
            if (inv_laser_active && ufo_active) {
                if (inv_laser_x >= ufo_x - 8 && inv_laser_x <= ufo_x + 8 && 
                    inv_laser_y >= ufo_y - 4 && inv_laser_y <= ufo_y + 4) {
                    ufo_active = false;
                    inv_laser_active = false;
                    invaders_score += 150 + (get_rand() % 4) * 50; // 150, 200, 250, 300
                    spawn_particles(ufo_x, ufo_y, 1, 15);
                    shake_timer = 15;
                    ufo_timer = 400 + (get_rand() % 400);
                }
            }
            
            // Hit Invaders
            if (inv_laser_active) {
                for (int r = 2; r >= 0; r--) {
                    for (int c = 0; c < 5; c++) {
                        if (inv_alive[r][c]) {
                            int ax = inv_x + c * 20;
                            int ay = inv_y + r * 14;
                            if (inv_laser_x >= ax - 6 && inv_laser_x <= ax + 6 && 
                                inv_laser_y >= ay - 6 && inv_laser_y <= ay + 6) {
                                inv_alive[r][c] = false;
                                inv_laser_active = false;
                                invaders_score += (3 - r) * 50;
                                spawn_particles(ax, ay, (r == 0) ? 1 : ((r == 1) ? 5 : 6), 10);
                                break;
                            }
                        }
                    }
                    if (!inv_laser_active) break;
                }
            }
            
            if (inv_laser_y < 10) inv_laser_active = false;
        }
        
        // Spawn Enemy Lasers
        if (frame_counter % 40 == 0) {
            int slot = -1;
            for (int l = 0; l < MAX_ENEMY_LASERS; l++) {
                if (!inv_enemy_laser_active[l]) {
                    slot = l;
                    break;
                }
            }
            
            if (slot != -1) {
                int cols[5];
                int active_cols_count = 0;
                for (int c = 0; c < 5; c++) {
                    if (inv_alive[0][c] || inv_alive[1][c] || inv_alive[2][c]) {
                        cols[active_cols_count++] = c;
                    }
                }
                
                if (active_cols_count > 0) {
                    int col = cols[get_rand() % active_cols_count];
                    int row = -1;
                    for (int r = 2; r >= 0; r--) {
                        if (inv_alive[r][col]) {
                            row = r;
                            break;
                        }
                    }
                    
                    if (row != -1) {
                        inv_enemy_laser_active[slot] = true;
                        inv_enemy_laser_x[slot] = inv_x + col * 20;
                        inv_enemy_laser_y[slot] = inv_y + row * 14 + 6;
                    }
                }
            }
        }
        
        // Update Enemy Lasers
        for (int l = 0; l < MAX_ENEMY_LASERS; l++) {
            if (inv_enemy_laser_active[l]) {
                inv_enemy_laser_y[l] += 3;
                draw_rect_s(buffer, inv_enemy_laser_x[l] - 1, inv_enemy_laser_y[l] - 2, 
                            inv_enemy_laser_x[l] + 1, inv_enemy_laser_y[l] + 2, 1);
                
                // Laser-on-Laser Collision
                if (inv_laser_active) {
                    if (abs(inv_laser_x - inv_enemy_laser_x[l]) < 4 && abs(inv_laser_y - inv_enemy_laser_y[l]) < 4) {
                        inv_laser_active = false;
                        inv_enemy_laser_active[l] = false;
                        spawn_particles(inv_laser_x, inv_laser_y, 3, 5);
                        continue;
                    }
                }
                
                // Hits Bunkers
                bool hit_bunker = false;
                int by_top = 180;
                for (int b = 0; b < 3; b++) {
                    int bx = bunker_positions[b] - 15;
                    if (inv_enemy_laser_x[l] >= bx && inv_enemy_laser_x[l] < bx + (BUNKER_COLS * BLOCK_W) && 
                        inv_enemy_laser_y[l] >= by_top && inv_enemy_laser_y[l] < by_top + (BUNKER_ROWS * BLOCK_H)) {
                        
                        int col = (inv_enemy_laser_x[l] - bx) / BLOCK_W;
                        if (col >= 0 && col < BUNKER_COLS) {
                            for (int row = 0; row < BUNKER_ROWS; row++) {
                                if (bunker_blocks[b][row][col] > 0) {
                                    bunker_blocks[b][row][col]--;
                                    inv_enemy_laser_active[l] = false;
                                    spawn_particles(inv_enemy_laser_x[l], inv_enemy_laser_y[l], 1, 5);
                                    hit_bunker = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (hit_bunker) break;
                }
                
                if (hit_bunker) continue;
                
                // Hits Player
                if (inv_enemy_laser_x[l] >= inv_player_x - 6 && inv_enemy_laser_x[l] <= inv_player_x + 6 && 
                    inv_enemy_laser_y[l] >= 206 && inv_enemy_laser_y[l] <= 215) {
                    
                    inv_enemy_laser_active[l] = false;
                    spawn_particles(inv_player_x, 210, 2, 20);
                    shake_timer = 25;
                    inv_lives--;
                    
                    if (inv_lives > 0) {
                        game_state = PLAY_STATE_PLAYER_EXPLODING;
                        state_timer = 45;
                    } else {
                        game_state = PLAY_STATE_GAME_OVER;
                        state_timer = 120;
                    }
                    continue;
                }
                
                if (inv_enemy_laser_y[l] > 230) {
                    inv_enemy_laser_active[l] = false;
                }
            }
        }
    } else if (game_state == PLAY_STATE_PLAYER_EXPLODING) {
        state_timer--;
        
        // Draw static world
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 5; c++) {
                if (inv_alive[r][c]) {
                    int ax = inv_x + c * 20;
                    int ay = inv_y + r * 14;
                    uint8_t color = (r == 0) ? 1 : ((r == 1) ? 5 : 6);
                    const uint16_t *sprite = (r == 0) ? SQUID_SPRITE[0] : ((r == 1) ? CRAB_SPRITE[0] : OCTOPUS_SPRITE[0]);
                    int width = (r == 0) ? 8 : ((r == 1) ? 11 : 12);
                    draw_sprite_centered_s(buffer, ax, ay, sprite, width, 8, color);
                }
            }
        }
        
        draw_bunkers(buffer);
        
        for (int l = 0; l < MAX_ENEMY_LASERS; l++) {
            if (inv_enemy_laser_active[l]) {
                draw_rect_s(buffer, inv_enemy_laser_x[l] - 1, inv_enemy_laser_y[l] - 2, 
                            inv_enemy_laser_x[l] + 1, inv_enemy_laser_y[l] + 2, 1);
            }
        }
        
        if (state_timer <= 0) {
            inv_player_x = 160;
            inv_laser_active = false;
            init_enemy_lasers();
            game_state = PLAY_STATE_RUNNING;
        }
    } else if (game_state == PLAY_STATE_GAME_OVER) {
        state_timer--;
        draw_string_s(buffer, "GAME OVER", 110, 110, 2, 1);
        
        if (state_timer <= 0) {
            init_invaders();
        }
    }
    
    update_and_draw_particles(buffer);
}
