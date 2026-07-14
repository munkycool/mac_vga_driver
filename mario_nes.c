#include "common.h"

// --- Constants ---
#define LEVEL_WIDTH     1000
#define CAMERA_MAX      (LEVEL_WIDTH - 320)
#define NUM_BLOCKS      20
#define NUM_AIR_COINS   5
#define NUM_GOOMBAS     4

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

    // Initialize Blocks (20 blocks matching NES 1-1 layout start)
    // 1. Single Q-block
    blocks[0] = (Block){160, 140, true, false};
    // 2. Row of 5 blocks + 1 Q-block above
    blocks[1] = (Block){360, 140, false, false}; // Brick
    blocks[2] = (Block){376, 140, true, false};  // Q-block
    blocks[3] = (Block){392, 140, false, false}; // Brick
    blocks[4] = (Block){408, 140, true, false};  // Q-block
    blocks[5] = (Block){424, 140, false, false}; // Brick
    blocks[6] = (Block){392, 80, true, false};   // Q-block above
    // 3. Row of 3 blocks
    blocks[7] = (Block){510, 140, false, false}; // Brick
    blocks[8] = (Block){526, 140, true, false};  // Q-block
    blocks[9] = (Block){542, 140, false, false}; // Brick
    // 4. Solid staircase pyramid before flagpole (10 blocks)
    blocks[10] = (Block){780, 203, false, false}; // Col 1
    blocks[11] = (Block){796, 203, false, false}; // Col 2
    blocks[12] = (Block){796, 187, false, false};
    blocks[13] = (Block){812, 203, false, false}; // Col 3
    blocks[14] = (Block){812, 187, false, false};
    blocks[15] = (Block){812, 171, false, false};
    blocks[16] = (Block){828, 203, false, false}; // Col 4
    blocks[17] = (Block){828, 187, false, false};
    blocks[18] = (Block){828, 171, false, false};
    blocks[19] = (Block){828, 155, false, false};

    // Initialize Air Coins
    air_coins[0] = (AirCoin){200, 160, true};
    air_coins[1] = (AirCoin){220, 150, true};
    air_coins[2] = (AirCoin){240, 160, true};
    air_coins[3] = (AirCoin){640, 150, true};
    air_coins[4] = (AirCoin){660, 150, true};

    // Initialize Goombas (4 Goombas at NES positions)
    goombas[0] = (Goomba){220, 210, -1, true, 0};
    goombas[1] = (Goomba){400, 210, -1, true, 0};
    goombas[2] = (Goomba){640, 210, -1, true, 0};
    goombas[3] = (Goomba){700, 210, -1, true, 0};

    // Initialize Koopa
    koopa = (Koopa){500, 210, -1, KOOPA_WALK};
}

// --- Sprites & Drawing Helpers ---

static const char *mario_walk_0[16] = {
    "....RRRRR...",
    "....RRRRRRR.",
    "....MMMYYKY.",
    "....MYMKKYY.",
    "....MYMMYYY.",
    "....MMYYYY..",
    ".....YYYY...",
    "....RRBRR...",
    "...RRRBRRR..",
    "..RRRRBBRRR.",
    "..YYRBYBRYY.",
    "..YYBBBBBYY.",
    "..YYBBBBBYY.",
    "...BBB.BBB..",
    "..MMM...MMM.",
    ".MMMM...MMMM"
};

static const char *mario_walk_1[16] = {
    "....RRRRR...",
    "....RRRRRRR.",
    "....MMMYYKY.",
    "....MYMKKYY.",
    "....MYMMYYY.",
    "....MMYYYY..",
    ".....YYYY...",
    "....RBBBR...",
    "...RRBBBRR..",
    "..RRRBBBBRR.",
    "..YYBBBBBYY.",
    "..YYBBBBBYY.",
    "...BBBBBBB..",
    "...BBB.BBB..",
    "..MMM...MMM.",
    ".MMMM....MMM"
};

static const char *mario_walk_2[16] = {
    "....RRRRR...",
    "....RRRRRRR.",
    "....MMMYYKY.",
    "....MYMKKYY.",
    "....MYMMYYY.",
    "....MMYYYY..",
    ".....YYYY...",
    "....RRBRR...",
    "...RRRBRRR..",
    "..RRRRBBRRR.",
    "..YYRBYBRYY.",
    "..YYBBBBBYY.",
    "..YYBBBBBYY.",
    "...BBB.BBB..",
    "...MMM..MMM.",
    "...MMM...MMM"
};

static const char *mario_jump[16] = {
    ".....RRRRR..",
    "....RRRRRRRR",
    "....MMMYYMY.",
    "....MYMYYYY.",
    "....MYMMYYMY",
    "....MMYYYY..",
    ".....YYYY...",
    "....RBBBR...",
    "...RRRBBBR..",
    "..RRRRBBBRR.",
    "..YYRBBBYYY.",
    "...YBBBBBY..",
    "....BBBBB...",
    "....BBB.BB..",
    "....MM...M..",
    "....M......."
};

static const char *mario_slide[16] = {
    "....RRRRR...",
    "....RRRRRRR.",
    "....MMMYYKY.",
    "....MYMKKYY.",
    "....MYMMYYY.",
    "....MMYYYY..",
    ".....YYYY...",
    "....RRBRR...",
    "....RRBRR...",
    "....RRBBR...",
    "....RBBB....",
    "....BBBB....",
    "....BBBB....",
    "....BBB.....",
    "....MMM.....",
    "....MMM....."
};

static const char *goomba_walk_0[16] = {
    "......MMMM......",
    "....MMMMMMMM....",
    "...MMMMMMMMMM...",
    "..MMKKMMMMKKMM..",
    "..MKKKKMMKKKKM..",
    "..MKKWKKKKWKKM..",
    "..MKWWKKKKWWKM..",
    "..MMWWKKKKWWMM..",
    "...MMKKKKKKMM...",
    "....MMMMMMMM....",
    ".....MMMMMM.....",
    "....MMMMMMMM....",
    "...MMMMMMMMMM...",
    "..KKKMMMMMMKKK..",
    ".KKKKMMMMMMKKKK.",
    ".KKK........KKK."
};

static const char *goomba_walk_1[16] = {
    "......MMMM......",
    "....MMMMMMMM....",
    "...MMMMMMMMMM...",
    "..MMKKMMMMKKMM..",
    "..MKKKKMMKKKKM..",
    "..MKKWKKKKWKKM..",
    "..MKWWKKKKWWKM..",
    "..MMWWKKKKWWMM..",
    "...MMKKKKKKMM...",
    "....MMMMMMMM....",
    ".....MMMMMM.....",
    "....MMMMMMMM....",
    "...MMMMMMMMMM...",
    "..KKKMMMMMMKKK..",
    "..KKKMMMMMMKKK..",
    "....KKK..KKK...."
};

static const char *goomba_squished[16] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "....MMMMMMMM....",
    "..MMMMMMMMMMMM..",
    ".MMKKMMMMMMKKMM.",
    "MKKKKKKKKKKKKKKM",
    "MKKWKKKKKKKKWKKM",
    "MKWWKKKKKKKKWWKM",
    "KKKKKKKKKKKKKKKK"
};

static const char *koopa_walk_0[24] = {
    "......YYYY......",
    "....YYYYYYYY....",
    "...YYWWYYWWYY...",
    "..YYWKKYYWKKYY..",
    "..YYKKKKKKKKYY..",
    "...YYYYYYYYYY...",
    "....YYYYYYYY....",
    "....YYYYYYYY....",
    "...GGGGGGGGGG...",
    "..GGGGGGGGGGGG..",
    ".GGGGWWWWWWGGGG.",
    ".GGGWWWWWWWWGGG.",
    ".GGWWWWWWWWWWGG.",
    "..GWWWWWWWWWWG..",
    "...WWWWWWWWWW...",
    "....GGGGGGGG....",
    "....GGGGGGGG....",
    "....GGGGGGGG....",
    "....YY....YY....",
    "....YY....YY....",
    "....YY....YY....",
    "....YY....YY....",
    "...YYY....YYY...",
    "..YYYY....YYYY.."
};

static const char *koopa_walk_1[24] = {
    "......YYYY......",
    "....YYYYYYYY....",
    "...YYWWYYWWYY...",
    "..YYWKKYYWKKYY..",
    "..YYKKKKKKKKYY..",
    "...YYYYYYYYYY...",
    "....YYYYYYYY....",
    "....YYYYYYYY....",
    "...GGGGGGGGGG...",
    "..GGGGGGGGGGGG..",
    ".GGGGWWWWWWGGGG.",
    ".GGGWWWWWWWWGGG.",
    ".GGWWWWWWWWWWGG.",
    "..GWWWWWWWWWWG..",
    "...WWWWWWWWWW...",
    "....GGGGGGGG....",
    "....GGGGGGGG....",
    "....GGGGGGGG....",
    ".....YY..YY.....",
    ".....YY..YY.....",
    ".....YY..YY.....",
    "....YY....YY....",
    "...YYY....YYY...",
    "..YYY......YYY.."
};

static const char *koopa_shell[16] = {
    "......GGGG......",
    "....GGGGGGGG....",
    "...GGGGGGGGGG...",
    "..GGGGGGGGGGGG..",
    ".GGGGWWWWWWGGGG.",
    ".GGGWWWWWWWWGGG.",
    ".GGWWWWWWWWWWGG.",
    "..GWWWWWWWWWWG..",
    "...WWWWWWWWWW...",
    "....GGGGGGGG....",
    "....GGGGGGGG....",
    ".....GGGGGG.....",
    "......GGGG......",
    "................",
    "................",
    "................"
};

static void draw_nes_sprite(uint8_t *buffer, int x_center, int y_bottom, const char *sprite[], int width, int height, bool flipped) {
    int start_x = x_center - width / 2;
    int start_y = y_bottom - height;
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            int col_idx = flipped ? (width - 1 - c) : c;
            char color_char = sprite[r][col_idx];
            if (color_char == '.') continue;
            uint8_t color = 0;
            switch (color_char) {
                case 'K': color = 0; break;
                case 'R': color = 1; break;
                case 'G': color = 2; break;
                case 'Y': color = 3; break;
                case 'B': color = 4; break;
                case 'M': color = 5; break;
                case 'C': color = 6; break;
                case 'W': color = 7; break;
                default: continue;
            }
            draw_pixel(buffer, start_x + c, start_y + r, color);
        }
    }
}

static void draw_brick_block(uint8_t *buffer, int x, int y) {
    draw_rect(buffer, x, y, x + 15, y + 15, 5); // Brown
    draw_rect(buffer, x, y, x + 15, y, 0);
    draw_rect(buffer, x, y + 4, x + 15, y + 4, 0);
    draw_rect(buffer, x, y + 8, x + 15, y + 8, 0);
    draw_rect(buffer, x, y + 12, x + 15, y + 12, 0);
    
    draw_rect(buffer, x, y, x, y + 4, 0);
    draw_rect(buffer, x + 8, y, x + 8, y + 4, 0);
    draw_rect(buffer, x + 4, y + 4, x + 4, y + 8, 0);
    draw_rect(buffer, x + 12, y + 4, x + 12, y + 8, 0);
    draw_rect(buffer, x, y + 8, x, y + 12, 0);
    draw_rect(buffer, x + 8, y + 8, x + 8, y + 12, 0);
    draw_rect(buffer, x + 4, y + 12, x + 4, y + 15, 0);
    draw_rect(buffer, x + 12, y + 12, x + 12, y + 15, 0);

    for (int by = 0; by < 16; by += 4) {
        int shift = (by == 4 || by == 12) ? 4 : 0;
        draw_pixel(buffer, x + 1 + shift, y + by + 1, 3);
        draw_pixel(buffer, x + 2 + shift, y + by + 1, 3);
        draw_pixel(buffer, x + 9 - shift, y + by + 1, 3);
        draw_pixel(buffer, x + 10 - shift, y + by + 1, 3);
    }
}

static void draw_q_block(uint8_t *buffer, int x, int y, int frame) {
    draw_rect(buffer, x, y, x + 15, y + 15, 3); // Yellow
    draw_rect(buffer, x, y, x + 15, y, 7);      // White highlights
    draw_rect(buffer, x, y, x, y + 15, 7);
    draw_rect(buffer, x + 15, y, x + 15, y + 15, 0);
    draw_rect(buffer, x, y + 15, x + 15, y + 15, 0);
    
    draw_pixel(buffer, x + 1, y + 1, 0);
    draw_pixel(buffer, x + 14, y + 1, 0);
    draw_pixel(buffer, x + 1, y + 14, 0);
    draw_pixel(buffer, x + 14, y + 14, 0);
    
    // Draw '?'
    draw_rect(buffer, x + 5, y + 3, x + 10, y + 3, 0);
    draw_rect(buffer, x + 10, y + 4, x + 10, y + 6, 0);
    draw_rect(buffer, x + 7, y + 7, x + 9, y + 8, 0);
    draw_rect(buffer, x + 7, y + 9, x + 8, y + 10, 0);
    draw_rect(buffer, x + 7, y + 12, x + 8, y + 13, 0);
}

static void draw_hit_block(uint8_t *buffer, int x, int y) {
    draw_rect(buffer, x, y, x + 15, y + 15, 5); // Brown
    draw_rect(buffer, x, y, x + 15, y, 0);
    draw_rect(buffer, x, y + 15, x + 15, y + 15, 0);
    draw_rect(buffer, x, y, x, y + 15, 0);
    draw_rect(buffer, x + 15, y, x + 15, y + 15, 0);
    draw_rect(buffer, x + 2, y + 2, x + 13, y + 13, 0);
    draw_rect(buffer, x + 3, y + 3, x + 12, y + 12, 5);
    draw_pixel(buffer, x + 1, y + 1, 0);
    draw_pixel(buffer, x + 14, y + 1, 0);
    draw_pixel(buffer, x + 1, y + 14, 0);
    draw_pixel(buffer, x + 14, y + 14, 0);
}

static void draw_ground_block(uint8_t *buffer, int x, int y) {
    draw_rect(buffer, x, y, x + 15, y + 15, 5); // Brown base
    draw_rect(buffer, x, y, x + 15, y, 0);      // Black borders
    draw_rect(buffer, x, y, x, y + 15, 0);
    draw_rect(buffer, x + 15, y, x + 15, y + 15, 0);
    draw_rect(buffer, x, y + 15, x + 15, y + 15, 0);
    
    draw_rect(buffer, x + 1, y + 1, x + 14, y + 1, 3); // Yellow highlight
    draw_rect(buffer, x + 1, y + 1, x + 1, y + 14, 3);
    
    draw_rect(buffer, x + 2, y + 14, x + 14, y + 14, 0);
    draw_rect(buffer, x + 14, y + 2, x + 14, y + 14, 0);
    
    draw_pixel(buffer, x + 4, y + 4, 0);
    draw_pixel(buffer, x + 5, y + 4, 0);
    draw_pixel(buffer, x + 5, y + 5, 0);
    draw_pixel(buffer, x + 10, y + 9, 0);
    draw_pixel(buffer, x + 11, y + 9, 0);
    draw_pixel(buffer, x + 11, y + 10, 0);
}

static void draw_nes_cloud(uint8_t *buffer, int x, int y) {
    draw_rect(buffer, x + 8, y, x + 24, y + 15, 7);
    draw_rect(buffer, x, y + 6, x + 31, y + 15, 7);
    draw_rect(buffer, x + 4, y + 3, x + 28, y + 15, 7);
    draw_rect(buffer, x + 4, y + 15, x + 28, y + 16, 6);
    draw_rect(buffer, x + 8, y + 16, x + 24, y + 17, 6);
}

static void draw_castle_brick(uint8_t *buffer, int x, int y, int w, int h) {
    draw_rect(buffer, x, y, x + w, y + h, 1); // Dark red
    for (int cy = y; cy <= y + h; cy += 4) {
        draw_rect(buffer, x, cy, x + w, cy, 0);
        int shift = (((cy - y) / 4) % 2) * 4;
        for (int cx = x + shift; cx <= x + w; cx += 8) {
            draw_rect(buffer, cx, cy, cx, cy + 4, 0);
        }
    }
}

void play_mario_nes(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);
    
    // Sky blue background
    draw_rect(buffer, 0, 0, 319, 218, 6);
    
    // Draw clouds (parallax scrolling)
    for (int cx = 100; cx < 1000; cx += 300) {
        int px = cx - camera_x / 2;
        if (px >= -40 && px < 320) {
            draw_nes_cloud(buffer, px, 40);
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
        draw_ground_block(buffer, px, 219);
    }

    // Draw Green Pipes (using the Flappy Bird style)
    int pipes_x[3] = {320, 460, 580};
    int pipes_h[3] = {32, 48, 64};
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
        draw_castle_brick(buffer, castle_x, 160, 50, 58);       // Main wall
        draw_castle_brick(buffer, castle_x + 10, 140, 30, 20);  // Upper tower
        // Battlements
        for (int bx = 0; bx < 50; bx += 10) {
            draw_rect(buffer, castle_x + bx, 156, castle_x + bx + 5, 159, 1);
            draw_rect(buffer, castle_x + bx, 156, castle_x + bx + 5, 156, 0);
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

        // Block side obstruction check (so Mario jumps up staircase/blocks)
        for (int i = 0; i < NUM_BLOCKS; i++) {
            int bx = blocks[i].x;
            int by = blocks[i].y;
            if (mario_x < bx && mario_x + 8 >= bx - 3 && mario_y >= by - 4 && mario_y <= by + 16) {
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
            if (blocks[i].is_hit) {
                draw_hit_block(buffer, bx, blocks[i].y);
            } else if (blocks[i].is_q_block) {
                draw_q_block(buffer, bx, blocks[i].y, frame_counter);
            } else {
                draw_brick_block(buffer, bx, blocks[i].y);
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
                const char **goomba_sprite = ((frame_counter / 6) % 2 == 0) ? goomba_walk_0 : goomba_walk_1;
                draw_nes_sprite(buffer, gx, g->y + 7, goomba_sprite, 16, 16, false);
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
                draw_nes_sprite(buffer, gx, g->y + 7, goomba_squished, 16, 16, false);
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
                const char **koopa_sprite = ((frame_counter / 6) % 2 == 0) ? koopa_walk_0 : koopa_walk_1;
                draw_nes_sprite(buffer, kx, koopa.y + 5, koopa_sprite, 16, 24, koopa.vx > 0);
            } else { // Shell/Slide Shell
                draw_nes_sprite(buffer, kx, koopa.y + 5, koopa_shell, 16, 16, false);
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
        const char **mario_sprite;
        bool is_flipped = (mario_vx < 0);
        if (level_state == MARIO_FLAG_SLIDE) {
            mario_sprite = mario_slide;
        } else if (mario_is_jumping) {
            mario_sprite = mario_jump;
        } else if (mario_vx == 0) {
            mario_sprite = mario_walk_0;
        } else {
            int anim = mario_anim_frame % 4;
            if (anim == 0) mario_sprite = mario_walk_0;
            else if (anim == 1) mario_sprite = mario_walk_1;
            else if (anim == 2) mario_sprite = mario_walk_0;
            else mario_sprite = mario_walk_2;
        }
        draw_nes_sprite(buffer, mx, mario_y + 1, mario_sprite, 12, 16, is_flipped);
    }

    // Draw HUD Score
    draw_score(buffer, mario_score, 10, 10, 2, 7);
}
