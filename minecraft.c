#include "common.h"
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

// --- Fixed-Point Math Engine ---
#define FP_SHIFT 16
typedef int32_t fixed_t;

#define TO_FP(x)   ((fixed_t)((x) * 65536.0f))
#define INT_VAL(x) ((x) >> FP_SHIFT)
#define FP_FRAC(x) ((x) & 0xFFFF)
#define FP_MUL(x, y) ((fixed_t)(((int64_t)(x) * (y)) >> FP_SHIFT))
#define FP_DIV(x, y) ((fixed_t)((((int64_t)(x)) << FP_SHIFT) / (y)))

// Precomputed Tables
static fixed_t sin_table[512];
static fixed_t cos_table[512];
static int col_angle_offset[320];
static fixed_t col_cos_corr[320];

// --- Minecraft Definitions ---
#define WORLD_X 64
#define WORLD_Y 64
#define WORLD_Z 12

#define BLOCK_AIR 0
#define BLOCK_WATER 1
#define BLOCK_GRASS 2
#define BLOCK_DIRT 3
#define BLOCK_STONE 4
#define BLOCK_WOOD 5
#define BLOCK_PLANK 6
#define BLOCK_LEAVES 7
#define BLOCK_SAND 8
#define BLOCK_GLASS 9
#define BLOCK_COAL_ORE 10
#define BLOCK_IRON_ORE 11
#define BLOCK_GOLD_ORE 12
#define BLOCK_DIAMOND_ORE 13
#define BLOCK_FLOWER_RED 14
#define BLOCK_FLOWER_YEL 15
#define BLOCK_BRICK 16
#define BLOCK_OBSIDIAN 17

static uint8_t world[WORLD_X][WORLD_Y][WORLD_Z];

// --- Player State ---
static fixed_t player_x;
static fixed_t player_y;
static fixed_t player_z;
static fixed_t player_vx;
static fixed_t player_vy;
static fixed_t player_vz;
static int cam_angle = 0;   // 0 to 511
static int cam_pitch = 0;   // -80 to 80 (y-shearing offset)

static int player_health = 20; // 10 hearts (2 half-hearts each)
static int player_dead_timer = 0;
static bool on_ground = false;

// Inventory & Tools
static int inv_items[18] = {0};
static int current_tool = 0; // 0: Hand, 1: Wood Pick, 2: Stone Pick, 3: Iron Pick, 4: Diamond Pick, 5: Diamond Sword
static int selected_slot = 0; // 0 to 7

// HUD & Combat Feedback
static int swing_timer = 0;
static int screen_shake = 0;
static int chat_timer = 0;
static char chat_msg1[64] = "";
static char chat_msg2[64] = "";

static void add_chat(const char *msg) {
    snprintf(chat_msg2, sizeof(chat_msg2), "%s", chat_msg1);
    snprintf(chat_msg1, sizeof(chat_msg1), "%s", msg);
    chat_timer = 120;
}

// AI Steve Variables
static fixed_t target_x;
static fixed_t target_y;
static int ai_state = 0; // 0: Wander, 1: Gather Wood, 2: Mine Stone, 3: Mine Ores, 4: Fight Mob
static int ai_timer = 0;
static int ai_target_mob = -1;

// Day/Night Cycle
static int time_of_day = 300; // 0 to 2000
static uint8_t sky_color = 6;  // Cyan (6)
static int ambient_light = 0;  // 0: bright day, 1: sunset, 2: dark night, 3: sunrise
static float cloud_offset = 0.0f;

// --- Block Textures (8x8 pixels, 8-color palette) ---
// Colors: 0:Black, 1:Red, 2:Green, 3:Yellow, 4:Blue, 5:Magenta, 6:Cyan, 7:White
static const uint8_t TEXTURES[18][8][8] = {
    // Air (0) - Unused
    {{0}},
    // Water (1)
    {
        {4,4,4,6,4,4,4,4},
        {4,4,6,6,4,4,4,6},
        {4,6,4,4,4,4,6,6},
        {6,4,4,4,4,6,4,4},
        {4,4,4,4,6,4,4,4},
        {4,4,4,6,6,4,4,4},
        {4,4,6,4,4,4,4,6},
        {4,6,4,4,4,4,6,4}
    },
    // Grass Top (2)
    {
        {2,2,2,3,2,2,2,2},
        {2,2,3,3,2,2,2,3},
        {2,3,2,2,2,2,3,3},
        {3,2,2,2,2,3,2,2},
        {2,2,2,2,3,2,2,2},
        {2,2,2,3,3,2,2,2},
        {2,2,3,2,2,2,2,3},
        {2,3,2,2,2,2,3,2}
    },
    // Dirt (3)
    {
        {3,1,3,3,1,3,0,3},
        {1,3,1,0,3,1,3,1},
        {3,1,3,3,1,3,1,3},
        {3,3,1,3,0,3,3,1},
        {1,3,0,3,3,1,3,3},
        {3,1,3,1,3,3,1,3},
        {3,3,1,3,1,3,3,0},
        {0,3,3,1,3,0,3,3}
    },
    // Stone (4)
    {
        {7,0,7,7,0,7,7,7},
        {7,7,0,7,7,7,0,7},
        {0,7,7,7,7,0,7,7},
        {7,7,7,0,7,7,7,0},
        {7,0,7,7,7,0,7,7},
        {7,7,7,0,7,7,7,7},
        {7,7,0,7,7,7,0,7},
        {0,7,7,7,0,7,7,7}
    },
    // Wood Trunk (5)
    {
        {1,3,1,1,3,1,0,1},
        {1,1,3,1,1,1,3,1},
        {3,1,1,1,3,1,1,1},
        {1,1,1,3,1,1,1,3},
        {1,3,1,1,1,3,1,1},
        {1,1,3,1,1,1,3,1},
        {3,1,1,3,1,1,1,1},
        {1,1,1,1,3,1,1,3}
    },
    // Wood Planks (6)
    {
        {3,3,3,3,3,3,3,3},
        {3,0,3,3,3,0,3,3},
        {3,3,3,3,3,3,3,3},
        {0,0,0,0,0,0,0,0},
        {3,3,3,3,3,3,3,3},
        {3,3,3,0,3,3,3,0},
        {3,3,3,3,3,3,3,3},
        {0,0,0,0,0,0,0,0}
    },
    // Leaves (7)
    {
        {2,0,2,2,0,2,2,2},
        {2,2,0,2,2,2,0,2},
        {0,2,2,2,2,0,2,2},
        {2,2,2,0,2,2,2,0},
        {2,0,2,2,2,0,2,2},
        {2,2,2,0,2,2,2,2},
        {2,2,0,2,2,2,0,2},
        {0,2,2,2,0,2,2,2}
    },
    // Sand (8)
    {
        {3,3,3,7,3,3,3,3},
        {3,3,7,7,3,3,3,7},
        {3,7,3,3,3,3,7,7},
        {7,3,3,3,3,7,3,3},
        {3,3,3,3,7,3,3,3},
        {3,3,3,7,7,3,3,3},
        {3,3,7,3,3,3,3,7},
        {3,7,3,3,3,3,7,3}
    },
    // Glass (9)
    {
        {6,6,6,6,6,6,6,6},
        {6,0,0,0,0,0,7,6},
        {6,0,0,0,0,7,0,6},
        {6,0,0,0,7,0,0,6},
        {6,0,0,7,0,0,0,6},
        {6,0,7,0,0,0,0,6},
        {6,7,0,0,0,0,0,6},
        {6,6,6,6,6,6,6,6}
    },
    // Coal Ore (10)
    {
        {7,0,7,7,0,7,7,7},
        {7,7,0,7,7,7,0,7},
        {0,7,0,0,7,0,7,7},
        {7,7,0,0,7,7,7,0},
        {7,0,7,7,7,0,7,7},
        {7,0,0,0,7,7,7,7},
        {7,7,0,7,7,7,0,7},
        {0,7,7,7,0,7,7,7}
    },
    // Iron Ore (11)
    {
        {7,0,7,7,0,7,7,7},
        {7,7,0,7,7,7,0,7},
        {0,7,3,3,7,0,7,7},
        {7,7,3,1,7,7,7,0},
        {7,0,7,7,7,0,7,7},
        {7,7,3,3,7,7,7,7},
        {7,7,0,7,7,7,0,7},
        {0,7,7,7,0,7,7,7}
    },
    // Gold Ore (12)
    {
        {7,0,7,7,0,7,7,7},
        {7,7,0,7,7,7,0,7},
        {0,7,3,3,7,0,7,7},
        {7,7,3,3,7,7,7,0},
        {7,0,7,7,7,0,7,7},
        {7,7,3,3,7,7,7,7},
        {7,7,0,7,7,7,0,7},
        {0,7,7,7,0,7,7,7}
    },
    // Diamond Ore (13)
    {
        {7,0,7,7,0,7,7,7},
        {7,7,0,7,7,7,0,7},
        {0,7,6,6,7,0,7,7},
        {7,7,6,6,7,7,7,0},
        {7,0,7,7,7,0,7,7},
        {7,7,6,6,7,7,7,7},
        {7,7,0,7,7,7,0,7},
        {0,7,7,7,0,7,7,7}
    },
    // Flower Red (14)
    {
        {0,0,0,0,0,0,0,0},
        {0,0,0,1,0,0,0,0},
        {0,0,1,3,1,0,0,0},
        {0,0,0,1,0,0,0,0},
        {0,0,0,2,0,0,0,0},
        {0,0,2,2,0,0,0,0},
        {0,0,0,2,0,0,0,0},
        {2,2,2,2,2,2,2,2}
    },
    // Flower Yellow (15)
    {
        {0,0,0,0,0,0,0,0},
        {0,0,0,3,0,0,0,0},
        {0,0,3,7,3,0,0,0},
        {0,0,0,3,0,0,0,0},
        {0,0,0,2,0,0,0,0},
        {0,0,0,2,2,0,0,0},
        {0,0,0,2,0,0,0,0},
        {2,2,2,2,2,2,2,2}
    },
    // Brick (16)
    {
        {1,1,1,1,1,1,1,1},
        {1,0,1,1,1,0,1,1},
        {1,1,1,1,1,1,1,1},
        {0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,1},
        {1,1,1,0,1,1,1,0},
        {1,1,1,1,1,1,1,1},
        {0,0,0,0,0,0,0,0}
    },
    // Obsidian (17)
    {
        {0,5,0,0,5,0,0,0},
        {5,0,5,0,0,5,0,5},
        {0,0,0,5,0,0,0,0},
        {0,5,0,0,5,0,5,0},
        {5,0,0,5,0,0,0,5},
        {0,0,5,0,0,5,0,0},
        {0,5,0,0,5,0,5,0},
        {5,0,0,0,0,5,0,5}
    }
};

// --- Particle System ---
typedef struct {
    fixed_t x, y, z;
    fixed_t vx, vy, vz;
    uint8_t color;
    int life;
} Particle;

#define MAX_PARTICLES 32
static Particle particles[MAX_PARTICLES];

static void spawn_particle(fixed_t x, fixed_t y, fixed_t z, uint8_t color) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life <= 0) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].z = z;
            particles[i].vx = TO_FP(((get_rand() % 100) - 50) * 0.001f);
            particles[i].vy = TO_FP(((get_rand() % 100) - 50) * 0.001f);
            particles[i].vz = TO_FP((get_rand() % 100) * 0.0015f);
            particles[i].color = color;
            particles[i].life = 15 + (get_rand() % 15);
            break;
        }
    }
}

// --- Mob Definitions (10 Types) ---
#define MOB_CREEPER  0
#define MOB_ZOMBIE   1
#define MOB_SKELETON 2
#define MOB_ENDERMAN 3
#define MOB_SPIDER   4
#define MOB_SLIME    5
#define MOB_PIG      6
#define MOB_SHEEP    7
#define MOB_COW      8
#define MOB_CHICKEN  9

typedef struct {
    fixed_t x, y, z;
    fixed_t vx, vy;
    int type;
    int health;
    int max_health;
    bool active;
    int state_timer;
    int hit_flash;
    int special_timer; // creeper hiss, skeleton reload
} Mob;

#define MAX_MOBS 16
static Mob mobs[MAX_MOBS];

// Mob sprites (represented compactly as width, height, and color indices)
static const uint8_t MOB_COLORS[10][4] = {
    {0, 2, 3, 0}, // 0: Creeper (Green, Yellow-green)
    {0, 2, 4, 1}, // 1: Zombie (Green, Blue, Red)
    {0, 7, 0, 0}, // 2: Skeleton (White, black)
    {0, 5, 0, 0}, // 3: Enderman (Magenta/Purple, black)
    {0, 1, 0, 0}, // 4: Spider (Red eyes, black)
    {0, 2, 7, 0}, // 5: Slime (Green, white)
    {0, 5, 7, 1}, // 6: Pig (Pink/Magenta, white, red)
    {0, 7, 3, 0}, // 7: Sheep (White wool, yellow head)
    {0, 7, 3, 0}, // 8: Cow (White spots, yellow horns/face)
    {0, 7, 3, 1}  // 9: Chicken (White, yellow beak, red wattle)
};

// Returns raw pixel indices (0:transparent, 1:primary, 2:secondary, 3:tertiary)
// Sprites are mapped as columns x rows
static uint8_t get_mob_pixel(int type, int r, int c, int w, int h) {
    (void)h; (void)w;
    if (type == MOB_CREEPER) { // 8x16
        if (r < 4) { // Head
            if (r == 2 && (c == 2 || c == 5)) return 0; // Black eyes
            if (r == 3 && (c >= 3 && c <= 4)) return 0; // Mouth
            return 1;
        }
        if (r >= 13) return (c % 3 == 0) ? 0 : 1; // Feet
        return ((r + c) % 2 == 0) ? 1 : 2; // Body texture
    }
    if (type == MOB_ZOMBIE) { // 8x16
        if (r < 4) { // Head
            if (r == 2 && (c == 2 || c == 5)) return 3; // Red eyes
            return 1; // Green head
        }
        if (r < 12) return 2; // Blue clothes
        return 1; // Green pants/feet
    }
    if (type == MOB_SKELETON) { // 8x16
        if (r < 4) {
            if (r == 2 && (c == 2 || c == 5)) return 0; // eye holes
            return 1; // White skull
        }
        if (c == 1 || c == 6) return 0; // thin arms/space
        if (r >= 12 && (c != 2 && c != 5)) return 0; // thin legs
        return 1;
    }
    if (type == MOB_ENDERMAN) { // 8x24
        if (r < 3) {
            if (r == 1 && (c == 2 || c == 5)) return 1; // Magenta glowing eyes
            return 0; // Black head
        }
        if (c == 1 || c == 6) return 0; // super thin arms
        return 0; // Black body
    }
    if (type == MOB_SPIDER) { // 16x8
        if (r < 3 && (c == 5 || c == 6 || c == 9 || c == 10)) return 1; // Red eyes
        // Spider legs
        if (r >= 4 && (c % 3 == 0)) return 0;
        return 2; // Black body
    }
    if (type == MOB_SLIME) { // 8x8
        if (r == 3 && (c == 2 || c == 5)) return 2; // white eyes
        return 1; // Green slime cube
    }
    if (type == MOB_PIG) { // 12x8
        if (c >= 8 && r < 4) { // Head
            if (r == 2 && c == 10) return 2; // white/black eye
            if (r == 3 && c == 11) return 3; // snout
            return 1; // Pink
        }
        if (r >= 6 && (c == 2 || c == 6)) return 0; // leg gaps
        return 1; // Pink body
    }
    if (type == MOB_SHEEP) { // 12x8
        if (c >= 8 && r < 4) {
            if (r == 2 && c == 9) return 2; // yellow face
            return 1; // White head
        }
        if (r >= 6 && (c == 2 || c == 6)) return 2; // yellow legs
        return 1; // White wool
    }
    if (type == MOB_COW) { // 12x8
        if (c >= 8 && r < 4) {
            if (r == 0 && c == 8) return 1; // horn
            return 2; // Brown face
        }
        if (r >= 6 && (c == 2 || c == 6)) return 2; // Brown legs
        return ((r + c) % 3 == 0) ? 1 : 2; // spotted body
    }
    if (type == MOB_CHICKEN) { // 8x8
        if (r < 3 && c >= 5) {
            if (r == 1 && c == 6) return 2; // beak
            if (r == 2 && c == 5) return 3; // wattle
            return 1;
        }
        if (r >= 6) return 2; // yellow feet
        return 1; // white body
    }
    return 0;
}

// --- Collision System & Terrain Functions ---

static fixed_t get_terrain_height(fixed_t x, fixed_t y) {
    int ix = INT_VAL(x);
    int iy = INT_VAL(y);
    if (ix < 0) { ix = 0; }
    if (ix >= WORLD_X) { ix = WORLD_X - 1; }
    if (iy < 0) { iy = 0; }
    if (iy >= WORLD_Y) { iy = WORLD_Y - 1; }

    for (int z = WORLD_Z - 1; z >= 0; z--) {
        uint8_t bt = world[ix][iy][z];
        if (bt != BLOCK_AIR && bt != BLOCK_WATER) {
            return (z + 1) << 16;
        }
    }
    return 0;
}

static bool check_player_collision(fixed_t x, fixed_t y, fixed_t z) {
    fixed_t r_xz = TO_FP(0.28f);
    fixed_t h_z = TO_FP(1.6f);
    int x1 = INT_VAL(x - r_xz);
    int x2 = INT_VAL(x + r_xz);
    int y1 = INT_VAL(y - r_xz);
    int y2 = INT_VAL(y + r_xz);
    int z1 = INT_VAL(z);
    int z2 = INT_VAL(z + h_z);

    for (int bx = x1; bx <= x2; bx++) {
        for (int by = y1; by <= y2; by++) {
            for (int bz = z1; bz <= z2; bz++) {
                if (bx < 0 || bx >= WORLD_X || by < 0 || by >= WORLD_Y || bz < 0 || bz >= WORLD_Z) {
                    if (bz < 0) return true;
                    continue;
                }
                uint8_t bt = world[bx][by][bz];
                if (bt != BLOCK_AIR && bt != BLOCK_WATER) {
                    return true;
                }
            }
        }
    }
    return false;
}

static bool check_mob_collision(fixed_t x, fixed_t y, fixed_t z, int type) {
    fixed_t r_xz = TO_FP(0.28f);
    fixed_t h_z = TO_FP(1.6f);
    if (type == MOB_SPIDER) { r_xz = TO_FP(0.45f); h_z = TO_FP(0.8f); }
    if (type == MOB_CHICKEN || type == MOB_SLIME) { r_xz = TO_FP(0.2f); h_z = TO_FP(0.6f); }
    if (type == MOB_ENDERMAN) { h_z = TO_FP(2.4f); }
    
    int x1 = INT_VAL(x - r_xz);
    int x2 = INT_VAL(x + r_xz);
    int y1 = INT_VAL(y - r_xz);
    int y2 = INT_VAL(y + r_xz);
    int z1 = INT_VAL(z);
    int z2 = INT_VAL(z + h_z);

    for (int bx = x1; bx <= x2; bx++) {
        for (int by = y1; by <= y2; by++) {
            for (int bz = z1; bz <= z2; bz++) {
                if (bx < 0 || bx >= WORLD_X || by < 0 || by >= WORLD_Y || bz < 0 || bz >= WORLD_Z) {
                    if (bz < 0) return true;
                    continue;
                }
                uint8_t bt = world[bx][by][bz];
                if (bt != BLOCK_AIR && bt != BLOCK_WATER) {
                    return true;
                }
            }
        }
    }
    return false;
}

// --- Procedural Generation ---
void init_minecraft() {
    // Math Tables
    for (int i = 0; i < 512; i++) {
        float rad = (float)i * (2.0f * M_PI / 512.0f);
        sin_table[i] = TO_FP(sinf(rad));
        cos_table[i] = TO_FP(cosf(rad));
    }
    for (int col = 0; col < 320; col++) {
        col_angle_offset[col] = (col - 160) * 85 / 320; // 60 deg FOV
        float rad = (float)col_angle_offset[col] * (2.0f * M_PI / 512.0f);
        col_cos_corr[col] = TO_FP(cosf(rad));
    }

    // Timers & Stats
    time_of_day = 300;
    sky_color = 6;
    ambient_light = 0;
    player_health = 20;
    player_dead_timer = 0;
    swing_timer = 0;
    screen_shake = 0;
    selected_slot = 5; // Start selected on Pickaxe/Sword slot
    
    for (int i = 0; i < 18; i++) inv_items[i] = 0;
    inv_items[BLOCK_DIRT] = 3;
    inv_items[BLOCK_WOOD] = 0;
    current_tool = 1; // start with Wooden Pickaxe

    // 1. Terrain Height Generation
    const int sea_level = 3;
    for (int x = 0; x < WORLD_X; x++) {
        for (int y = 0; y < WORLD_Y; y++) {
            float fx = (float)x * 0.12f;
            float fy = (float)y * 0.12f;
            float wave = sinf(fx) * 1.5f + cosf(fy) * 1.5f + sinf(fx * 0.5f + fy * 0.5f) * 0.8f;
            int h = 5 + (int)roundf(wave);
            if (h < 2) h = 2;
            if (h >= WORLD_Z - 2) h = WORLD_Z - 3;

            for (int z = 0; z < WORLD_Z; z++) {
                if (z < h - 2) {
                    world[x][y][z] = BLOCK_STONE;
                } else if (z < h - 1) {
                    world[x][y][z] = BLOCK_DIRT;
                } else if (z < h) {
                    // Sand beach, else grass
                    if (h <= sea_level + 1) {
                        world[x][y][z] = BLOCK_SAND;
                    } else {
                        world[x][y][z] = BLOCK_GRASS;
                    }
                } else if (z < sea_level) {
                    world[x][y][z] = BLOCK_WATER;
                } else {
                    world[x][y][z] = BLOCK_AIR;
                }
            }
        }
    }

    // 2. 3D Cave Digging
    for (int x = 1; x < WORLD_X - 1; x++) {
        for (int y = 1; y < WORLD_Y - 1; y++) {
            for (int z = 1; z < WORLD_Z - 1; z++) {
                if (world[x][y][z] == BLOCK_STONE || world[x][y][z] == BLOCK_DIRT) {
                    float val = sinf(x * 0.35f) * cosf(y * 0.35f) * sinf(z * 0.7f);
                    if (val > 0.45f) {
                        world[x][y][z] = BLOCK_AIR;
                    }
                }
            }
        }
    }

    // 3. Ore Placements (Coal, Iron, Gold, Diamond)
    for (int i = 0; i < 60; i++) {
        int ox = 2 + (get_rand() % (WORLD_X - 4));
        int oy = 2 + (get_rand() % (WORLD_Y - 4));
        int oz = 1 + (get_rand() % (WORLD_Z - 4));
        if (world[ox][oy][oz] == BLOCK_STONE) {
            uint8_t ore = BLOCK_COAL_ORE;
            if (oz < 3) ore = BLOCK_DIAMOND_ORE;
            else if (oz < 5) ore = BLOCK_GOLD_ORE;
            else if (oz < 7) ore = BLOCK_IRON_ORE;
            world[ox][oy][oz] = ore;
        }
    }

    // 4. Forests (Trees) & Flowers
    for (int i = 0; i < 40; i++) {
        int tx = 3 + (get_rand() % (WORLD_X - 6));
        int ty = 3 + (get_rand() % (WORLD_Y - 6));
        fixed_t th_fp = get_terrain_height(tx << 16, ty << 16);
        int th = INT_VAL(th_fp);
        if (th > 2 && th < WORLD_Z - 4) {
            if (world[tx][ty][th-1] == BLOCK_GRASS) {
                // Spawn tree trunk
                for (int tz = th; tz < th + 4; tz++) {
                    world[tx][ty][tz] = BLOCK_WOOD;
                }
                // Leaves dome
                for (int lx = tx - 2; lx <= tx + 2; lx++) {
                    for (int ly = ty - 2; ly <= ty + 2; ly++) {
                        for (int lz = th + 2; lz <= th + 4; lz++) {
                            if (lx == tx && ly == ty && lz < th + 4) continue;
                            world[lx][ly][lz] = BLOCK_LEAVES;
                        }
                    }
                }
            }
        }
    }
    
    // Spawn Flowers
    for (int i = 0; i < 30; i++) {
        int fx = 2 + (get_rand() % (WORLD_X - 4));
        int fy = 2 + (get_rand() % (WORLD_Y - 4));
        int fz = INT_VAL(get_terrain_height(fx << 16, fy << 16));
        if (fz > 0 && world[fx][fy][fz - 1] == BLOCK_GRASS && world[fx][fy][fz] == BLOCK_AIR) {
            world[fx][fy][fz] = (get_rand() % 2 == 0) ? BLOCK_FLOWER_RED : BLOCK_FLOWER_YEL;
        }
    }

    // 5. Spawn Mobs
    for (int i = 0; i < MAX_MOBS; i++) {
        mobs[i].active = true;
        mobs[i].type = i % 10; // 0 to 9
        mobs[i].max_health = (mobs[i].type == MOB_ENDERMAN) ? 40 : ((mobs[i].type >= MOB_PIG) ? 10 : 20);
        mobs[i].health = mobs[i].max_health;
        mobs[i].x = TO_FP(5.0f + (get_rand() % (WORLD_X - 10)));
        mobs[i].y = TO_FP(5.0f + (get_rand() % (WORLD_Y - 10)));
        mobs[i].z = get_terrain_height(mobs[i].x, mobs[i].y);
        mobs[i].vx = 0;
        mobs[i].vy = 0;
        mobs[i].state_timer = 0;
        mobs[i].hit_flash = 0;
        mobs[i].special_timer = 0;
    }

    // Reset particles
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;

    // Player Initial State
    player_x = TO_FP(32.0f);
    player_y = TO_FP(32.0f);
    player_z = get_terrain_height(player_x, player_y) + TO_FP(1.1f);
    player_vx = 0;
    player_vy = 0;
    player_vz = 0;
    cam_angle = 128; // Face North-ish
    cam_pitch = -10;  // Slightly look down
    
    // AI Target Init
    target_x = player_x;
    target_y = player_y;
    ai_state = 0;
    ai_timer = 0;
    
    add_chat("[ Steve Spawned ]");
    add_chat("World procedural generation complete.");
}

// --- Interactive & AI Controller ---
static void handle_input_and_ai(int frame_counter) {
    if (player_dead_timer > 0) return;

    fixed_t move_speed = TO_FP(0.09f);
    fixed_t rot_speed = 5; // yaw steps

    // 1. Check if user is actively playing
    if (global_input.interactive) {
        // Toggle Pitch/Strafe mode if Action2 (C/X) is held
        if (global_input.action2) {
            // Pitch Look
            if (global_input.up) cam_pitch = cam_pitch + 3;
            if (global_input.down) cam_pitch = cam_pitch - 3;
            if (cam_pitch > 70) cam_pitch = 70;
            if (cam_pitch < -70) cam_pitch = -70;

            // Strafe movement
            if (global_input.left) {
                player_vx += FP_MUL(cos_table[(cam_angle - 128) & 511], move_speed);
                player_vy += FP_MUL(sin_table[(cam_angle - 128) & 511], move_speed);
            }
            if (global_input.right) {
                player_vx += FP_MUL(cos_table[(cam_angle + 128) & 511], move_speed);
                player_vy += FP_MUL(sin_table[(cam_angle + 128) & 511], move_speed);
            }
        } else {
            // Turn Left/Right
            if (global_input.left) cam_angle = (cam_angle - rot_speed) & 511;
            if (global_input.right) cam_angle = (cam_angle + rot_speed) & 511;

            // Move Forward/Backward
            if (global_input.up) {
                player_vx += FP_MUL(cos_table[cam_angle], move_speed);
                player_vy += FP_MUL(sin_table[cam_angle], move_speed);
            }
            if (global_input.down) {
                player_vx -= FP_MUL(cos_table[cam_angle], move_speed);
                player_vy -= FP_MUL(sin_table[cam_angle], move_speed);
            }
        }

        // Jump
        if (global_input.action1 && on_ground) {
            player_vz = TO_FP(0.20f);
            on_ground = false;
        }

        // Switch hotbar items
        if ((frame_counter % 20) == 0 && get_rand() % 8 == 0) {
            selected_slot = (selected_slot + 1) % 8;
        }

    } else {
        // 2. AI Steve Autonomous Autoplay
        ai_timer++;
        
        // State Machine Decision
        if (ai_timer > 150) {
            ai_timer = 0;
            // Scan for solid blocks nearby to mine, or mobs to target
            int px = INT_VAL(player_x);
            int py = INT_VAL(player_y);
            
            // Crafting checks:
            if (inv_items[BLOCK_WOOD] >= 2) {
                inv_items[BLOCK_WOOD] -= 2;
                inv_items[BLOCK_PLANK] += 8;
                add_chat("[AI Steve: Crafted 8 Planks]");
            }
            if (inv_items[BLOCK_PLANK] >= 3 && current_tool < 1) {
                inv_items[BLOCK_PLANK] -= 3;
                current_tool = 1;
                selected_slot = 5;
                add_chat("[AI Steve: Crafted Wood Pick]");
            }
            if (inv_items[BLOCK_STONE] >= 3 && inv_items[BLOCK_PLANK] >= 2 && current_tool < 2) {
                inv_items[BLOCK_STONE] -= 3;
                inv_items[BLOCK_PLANK] -= 2;
                current_tool = 2;
                selected_slot = 5;
                add_chat("[AI Steve: Crafted Stone Pick]");
            }
            if (inv_items[BLOCK_STONE] >= 8 && current_tool < 5) {
                inv_items[BLOCK_STONE] -= 8;
                current_tool = 5; // Diamond Sword (or Stone/Iron Sword equivalent)
                selected_slot = 5;
                add_chat("[AI Steve: Crafted Sword]");
            }

            // Target scanning
            bool found_target = false;
            
            // Check for nearby hostile mobs to fight
            for (int i = 0; i < MAX_MOBS; i++) {
                if (mobs[i].active && mobs[i].type < MOB_PIG) {
                    fixed_t dx = mobs[i].x - player_x;
                    fixed_t dy = mobs[i].y - player_y;
                    if (FP_MUL(dx, dx) + FP_MUL(dy, dy) < TO_FP(64.0f)) { // within 8 blocks
                        ai_state = 4; // Fight Mob state
                        ai_target_mob = i;
                        found_target = true;
                        break;
                    }
                }
            }

            if (!found_target) {
                // Scan surroundings for wood (BLOCK_WOOD) or stone/ores
                for (int dx = -5; dx <= 5 && !found_target; dx++) {
                    for (int dy = -5; dy <= 5 && !found_target; dy++) {
                        int tx = px + dx;
                        int ty = py + dy;
                        if (tx >= 0 && tx < WORLD_X && ty >= 0 && ty < WORLD_Y) {
                            for (int tz = 0; tz < WORLD_Z; tz++) {
                                uint8_t bt = world[tx][ty][tz];
                                if (bt == BLOCK_WOOD) {
                                    ai_state = 1; // Gather Wood
                                    target_x = TO_FP(tx + 0.5f);
                                    target_y = TO_FP(ty + 0.5f);
                                    found_target = true;
                                    break;
                                } else if (bt == BLOCK_STONE || (bt >= BLOCK_COAL_ORE && bt <= BLOCK_DIAMOND_ORE)) {
                                    ai_state = (bt == BLOCK_STONE) ? 2 : 3;
                                    target_x = TO_FP(tx + 0.5f);
                                    target_y = TO_FP(ty + 0.5f);
                                    found_target = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if (!found_target) {
                // Default: Wander
                ai_state = 0;
                target_x = player_x + TO_FP(((get_rand() % 10) - 5));
                target_y = player_y + TO_FP(((get_rand() % 10) - 5));
                if (target_x < TO_FP(2)) target_x = TO_FP(2);
                if (target_x > TO_FP(WORLD_X - 3)) target_x = TO_FP(WORLD_X - 3);
                if (target_y < TO_FP(2)) target_y = TO_FP(2);
                if (target_y > TO_FP(WORLD_Y - 3)) target_y = TO_FP(WORLD_Y - 3);
            }
        }

        // Executive AI controls
        fixed_t dest_x = target_x;
        fixed_t dest_y = target_y;
        
        if (ai_state == 4) { // Fight targeted mob
            if (ai_target_mob >= 0 && mobs[ai_target_mob].active) {
                dest_x = mobs[ai_target_mob].x;
                dest_y = mobs[ai_target_mob].y;
            } else {
                ai_timer = 999; // trigger new task
            }
        }

        // Steer toward destination
        fixed_t dx = dest_x - player_x;
        fixed_t dy = dest_y - player_y;
        fixed_t dist_sq = FP_MUL(dx, dx) + FP_MUL(dy, dy);

        if (dist_sq > TO_FP(0.04f)) {
            // Face target direction
            float rad = atan2f((float)dy / 65536.0f, (float)dx / 65536.0f);
            int dest_angle = (int)(rad * 512.0f / (2.0f * M_PI));
            if (dest_angle < 0) dest_angle += 512;
            
            // Interpolate Yaw
            int angle_diff = dest_angle - cam_angle;
            while (angle_diff < -256) angle_diff += 512;
            while (angle_diff > 256) angle_diff -= 512;
            cam_angle = (cam_angle + angle_diff / 5) & 511;

            // Smooth pitch lookup bobbing
            cam_pitch = (cam_pitch * 9 + -10) / 10;

            // Move
            player_vx += FP_MUL(cos_table[cam_angle], move_speed);
            player_vy += FP_MUL(sin_table[cam_angle], move_speed);
        }

        // Auto-Jump when blocked
        if (on_ground && (frame_counter % 5 == 0)) {
            fixed_t fwd_x = player_x + FP_MUL(cos_table[cam_angle], TO_FP(0.45f));
            fixed_t fwd_y = player_y + FP_MUL(sin_table[cam_angle], TO_FP(0.45f));
            if (check_player_collision(fwd_x, fwd_y, player_z)) {
                player_vz = TO_FP(0.20f); // Jump!
                on_ground = false;
            }
        }

        // AI Mine / Interact logic
        if (swing_timer == 0 && (get_rand() % 25 == 0)) {
            fixed_t ray_x = player_x + FP_MUL(cos_table[cam_angle], TO_FP(1.2f));
            fixed_t ray_y = player_y + FP_MUL(sin_table[cam_angle], TO_FP(1.2f));
            int block_x = INT_VAL(ray_x);
            int block_y = INT_VAL(ray_y);
            int block_z = INT_VAL(player_z + TO_FP(0.1f));

            if (block_x >= 0 && block_x < WORLD_X && block_y >= 0 && block_y < WORLD_Y && block_z >= 0 && block_z < WORLD_Z) {
                uint8_t bt = world[block_x][block_y][block_z];
                if (bt != BLOCK_AIR && bt != BLOCK_WATER) {
                    // Swing weapon & break block
                    swing_timer = 12;
                    world[block_x][block_y][block_z] = BLOCK_AIR;
                    inv_items[bt]++;
                    
                    // Particles
                    for (int p=0; p<6; p++) spawn_particle(ray_x, ray_y, player_z + TO_FP(0.3f), TEXTURES[bt][3][3]);
                    
                    char log_msg[32];
                    sprintf(log_msg, "Steve mined block %d", bt);
                    add_chat(log_msg);
                }
            }

            // Hit mob combat range check
            if (ai_state == 4 && ai_target_mob >= 0 && mobs[ai_target_mob].active) {
                fixed_t combat_dx = mobs[ai_target_mob].x - player_x;
                fixed_t combat_dy = mobs[ai_target_mob].y - player_y;
                if (FP_MUL(combat_dx, combat_dx) + FP_MUL(combat_dy, combat_dy) < TO_FP(2.5f)) { // close enough
                    swing_timer = 12;
                    int damage = (current_tool == 5) ? 7 : 3;
                    mobs[ai_target_mob].health -= damage;
                    mobs[ai_target_mob].hit_flash = 8;
                    mobs[ai_target_mob].vx += combat_dx * 2; // knockback
                    mobs[ai_target_mob].vy += combat_dy * 2;
                    
                    // Blood/hit particles
                    for (int p=0; p<6; p++) spawn_particle(mobs[ai_target_mob].x, mobs[ai_target_mob].y, mobs[ai_target_mob].z + TO_FP(0.6f), 1);
                    
                    add_chat("Steve attacked monster!");

                    if (mobs[ai_target_mob].health <= 0) {
                        mobs[ai_target_mob].active = false;
                        add_chat("Steve defeated monster!");
                        ai_timer = 999; // trigger new task
                    }
                }
            }
        }
    }
}

// --- Physics Update ---
static void update_physics() {
    if (player_dead_timer > 0) {
        player_dead_timer--;
        if (player_dead_timer <= 0) {
            // Respawn Steve
            init_minecraft();
        }
        return;
    }

    // Apply drag/friction
    player_vx = FP_MUL(player_vx, TO_FP(0.65f));
    player_vy = FP_MUL(player_vy, TO_FP(0.65f));

    // Water checks
    int px = INT_VAL(player_x);
    int py = INT_VAL(player_y);
    int pz = INT_VAL(player_z);
    bool in_water = false;
    if (px >= 0 && px < WORLD_X && py >= 0 && py < WORLD_Y && pz >= 0 && pz < WORLD_Z) {
        if (world[px][py][pz] == BLOCK_WATER) in_water = true;
    }

    // Apply gravity
    if (in_water) {
        player_vz -= TO_FP(0.005f); // low gravity
        if (player_vz < -TO_FP(0.04f)) player_vz = -TO_FP(0.04f); // terminal velocity in water
    } else {
        player_vz -= TO_FP(0.016f); // regular gravity
        if (player_vz < -TO_FP(0.35f)) player_vz = -TO_FP(0.35f); // terminal velocity
    }

    // X update & collision
    fixed_t next_x = player_x + player_vx;
    if (check_player_collision(next_x, player_y, player_z)) {
        player_vx = 0;
    } else {
        player_x = next_x;
    }

    // Y update & collision
    fixed_t next_y = player_y + player_vy;
    if (check_player_collision(player_x, next_y, player_z)) {
        player_vy = 0;
    } else {
        player_y = next_y;
    }

    // Z update & collision
    fixed_t next_z = player_z + player_vz;
    if (check_player_collision(player_x, player_y, next_z)) {
        if (player_vz < 0) {
            on_ground = true;
            // Fall damage check
            if (player_vz < -TO_FP(0.24f) && !in_water) {
                int damage = INT_VAL((-player_vz - TO_FP(0.24f)) * 40);
                if (damage > 0) {
                    player_health -= damage;
                    screen_shake = 12;
                    add_chat("Steve fell from a high place!");
                }
            }
        }
        player_vz = 0;
    } else {
        player_z = next_z;
        if (player_vz != 0) on_ground = false;
    }

    // Out of bounds check (falling into the void!)
    if (player_z < -TO_FP(4.0f)) {
        player_health = 0;
    }

    if (player_health <= 0) {
        player_health = 0;
        player_dead_timer = 120; // 2 seconds death screen
        add_chat("Steve died!");
    }
}

// --- Mobs & Particles Update ---
static void update_mobs_and_particles() {
    // 1. Update Particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0) {
            particles[i].life--;
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].z += particles[i].vz;
            particles[i].vz -= TO_FP(0.008f); // particle gravity
        }
    }

    // 2. Update Mobs
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active) continue;

        if (mobs[i].hit_flash > 0) mobs[i].hit_flash--;

        mobs[i].state_timer++;

        // Monster Aggro / Pursuit AI
        fixed_t dx = player_x - mobs[i].x;
        fixed_t dy = player_y - mobs[i].y;
        fixed_t dist_sq = FP_MUL(dx, dx) + FP_MUL(dy, dy);

        fixed_t m_speed = TO_FP(0.025f);
        if (mobs[i].type == MOB_SPIDER) m_speed = TO_FP(0.045f);
        if (mobs[i].type == MOB_SLIME) m_speed = TO_FP(0.015f);

        // Hostile chasing player
        if (mobs[i].type < MOB_PIG && dist_sq < TO_FP(120.0f) && player_dead_timer <= 0) { // within ~11 blocks
            float rad = atan2f((float)dy / 65536.0f, (float)dx / 65536.0f);
            mobs[i].vx = TO_FP(cosf(rad) * ((float)m_speed / 65536.0f));
            mobs[i].vy = TO_FP(sinf(rad) * ((float)m_speed / 65536.0f));

            // Special actions (Creeper detonate, Skeleton shoot)
            if (mobs[i].type == MOB_CREEPER) {
                if (dist_sq < TO_FP(2.2f)) {
                    mobs[i].special_timer++;
                    mobs[i].hit_flash = 4; // white flash
                    if (mobs[i].special_timer == 1) {
                        add_chat("Creeper hissed!");
                    }
                    if (mobs[i].special_timer >= 24) { // Explode!
                        mobs[i].active = false;
                        
                        // Explosion logic
                        int bx = INT_VAL(mobs[i].x);
                        int by = INT_VAL(mobs[i].y);
                        int bz = INT_VAL(mobs[i].z);
                        for (int rx = -1; rx <= 1; rx++) {
                            for (int ry = -1; ry <= 1; ry++) {
                                for (int rz = -1; rz <= 1; rz++) {
                                    if (bx+rx >= 0 && bx+rx < WORLD_X && by+ry >= 0 && by+ry < WORLD_Y && bz+rz >= 0 && bz+rz < WORLD_Z) {
                                        world[bx+rx][by+ry][bz+rz] = BLOCK_AIR;
                                    }
                                }
                            }
                        }
                        // Spawn explosion smoke
                        for (int p=0; p<15; p++) {
                            spawn_particle(mobs[i].x, mobs[i].y, mobs[i].z + TO_FP(0.5f), (get_rand() % 2 == 0) ? 3 : 1);
                        }
                        screen_shake = 16;
                        add_chat("Creeper exploded!");

                        // Damage Steve
                        if (dist_sq < TO_FP(9.0f)) {
                            player_health -= 12;
                        }
                    }
                } else {
                    if (mobs[i].special_timer > 0) mobs[i].special_timer--;
                }
            }

            if (mobs[i].type == MOB_SKELETON) {
                mobs[i].special_timer++;
                if (mobs[i].special_timer >= 45) { // Shoot arrow projectile
                    mobs[i].special_timer = 0;
                    // Spawn arrow particle
                    spawn_particle(mobs[i].x, mobs[i].y, mobs[i].z + TO_FP(0.8f), 7);
                    add_chat("Skeleton shot arrow!");
                    if (dist_sq < TO_FP(64.0f) && (get_rand() % 3 == 0)) { // hit chance
                        player_health -= 2;
                        screen_shake = 6;
                        add_chat("Steve shot by arrow!");
                    }
                }
            }

            if (mobs[i].type == MOB_ENDERMAN && (get_rand() % 150 == 0)) { // Teleport!
                mobs[i].x += TO_FP(((get_rand() % 8) - 4));
                mobs[i].y += TO_FP(((get_rand() % 8) - 4));
                mobs[i].z = get_terrain_height(mobs[i].x, mobs[i].y);
                // Purple particles
                for (int p=0; p<6; p++) spawn_particle(mobs[i].x, mobs[i].y, mobs[i].z + TO_FP(1.0f), 5);
            }

            // Damage player on contact
            if (mobs[i].type != MOB_SKELETON && dist_sq < TO_FP(0.8f)) {
                if (mobs[i].type == MOB_ZOMBIE && (get_rand() % 15 == 0)) {
                    player_health -= 3;
                    screen_shake = 8;
                    add_chat("Steve bit by Zombie!");
                } else if (mobs[i].type == MOB_SPIDER && (get_rand() % 15 == 0)) {
                    player_health -= 2;
                    screen_shake = 6;
                    add_chat("Spider stung Steve!");
                } else if (mobs[i].type == MOB_SLIME && (get_rand() % 20 == 0)) {
                    player_health -= 1;
                    screen_shake = 4;
                }
            }

        } else {
            // Passive wandering / idle AI
            if (mobs[i].state_timer > 80) {
                mobs[i].state_timer = 0;
                float ang = (get_rand() % 100) * 0.0628f;
                mobs[i].vx = TO_FP(cosf(ang) * 0.015f);
                mobs[i].vy = TO_FP(sinf(ang) * 0.015f);
            }
        }

        // Apply movement & collision physics
        fixed_t next_x = mobs[i].x + mobs[i].vx;
        fixed_t next_y = mobs[i].y + mobs[i].vy;
        
        if (check_mob_collision(next_x, mobs[i].y, mobs[i].z, mobs[i].type)) {
            mobs[i].vx = -mobs[i].vx;
        } else {
            mobs[i].x = next_x;
        }

        if (check_mob_collision(mobs[i].x, next_y, mobs[i].z, mobs[i].type)) {
            mobs[i].vy = -mobs[i].vy;
        } else {
            mobs[i].y = next_y;
        }

        mobs[i].z = get_terrain_height(mobs[i].x, mobs[i].y);
    }
}

// --- 3D Raycasting Engine (320x240 High-Resolution) ---

static void draw_column(uint8_t *buffer, int col, int y_start, int y_end, uint8_t color) {
    if (col < 0 || col >= 320) return;
    if (y_start < 0) y_start = 0;
    if (y_end >= 240) y_end = 239;

    uint8_t pixel = (color & 0x07) | 0x08;
    uint8_t val = (pixel << 4) | pixel;

    for (int y = y_start; y <= y_end; y++) {
        buffer[y * 320 + col] = val;
    }
}

static uint8_t apply_lighting(uint8_t color, fixed_t dist, int hit_side, int col, int y, uint8_t sc) {
    if (color == 0) return 0;

    // Side hit shadow
    if (hit_side == 1) {
        if (color == 7) color = 7; // white remains white
        else if (color == 2) {
            if ((col + y) % 2 == 0) color = 0; // dither down grass sides
        } else {
            // Apply slight shadow
            if (color == 6) color = 4; // cyan to deep blue
            else if (color == 3) color = 0; // yellow to dark
        }
    }

    // Distance fog
    if (dist < TO_FP(4.0f)) {
        return color;
    } else if (dist < TO_FP(8.0f)) {
        if ((col + y) % 4 == 0) return sc;
        return color;
    } else if (dist < TO_FP(12.0f)) {
        if ((col + y) % 2 == 0) return sc;
        return color;
    } else if (dist < TO_FP(16.0f)) {
        if ((col + y) % 4 != 0) return sc;
        return color;
    } else {
        return sc;
    }
}

static void render_world(uint8_t *buffer) {
    // Determine horizon line with pitch look shearing
    int y_center = 120 + cam_pitch;
    
    // Scale factor
    fixed_t scale_y = TO_FP(130.0f);

    int sun_angle = (time_of_day * 512 / 2000) & 511;
    int moon_angle = (sun_angle + 256) & 511;
    int sun_elevation = INT_VAL(FP_MUL(sin_table[sun_angle], TO_FP(80.0f)));

    // Single-pass occlusion buffer on stack (320 cols analyzed row-by-row internally)
    for (int col = 0; col < 320; col++) {
        // Trace Ray
        int ray_angle = (cam_angle + col_angle_offset[col]) & 511;
        fixed_t ray_cos = cos_table[ray_angle];
        fixed_t ray_sin = sin_table[ray_angle];
        fixed_t cos_corr = col_cos_corr[col];

        fixed_t rx = player_x;
        fixed_t ry = player_y;

        int map_x = INT_VAL(rx);
        int map_y = INT_VAL(ry);

        fixed_t delta_dist_x = (ray_cos == 0) ? 0x7FFFFFFF : abs(FP_DIV(1 << 16, ray_cos));
        fixed_t delta_dist_y = (ray_sin == 0) ? 0x7FFFFFFF : abs(FP_DIV(1 << 16, ray_sin));

        int step_x = (ray_cos < 0) ? -1 : 1;
        int step_y = (ray_sin < 0) ? -1 : 1;

        fixed_t player_x_frac = rx & 0xFFFF;
        fixed_t player_y_frac = ry & 0xFFFF;

        fixed_t side_dist_x = (ray_cos < 0) ? FP_MUL(player_x_frac, delta_dist_x) : FP_MUL((1 << 16) - player_x_frac, delta_dist_x);
        fixed_t side_dist_y = (ray_sin < 0) ? FP_MUL(player_y_frac, delta_dist_y) : FP_MUL((1 << 16) - player_y_frac, delta_dist_y);

        bool filled[240];
        memset(filled, 0, sizeof(filled));
        int filled_count = 0;

        fixed_t dist = 0;
        int hit_side = 0;

        // DDA traversal
        for (int step = 0; step < 22; step++) {
            fixed_t d_enter = dist;

            if (side_dist_x < side_dist_y) {
                dist = side_dist_x;
                side_dist_x += delta_dist_x;
                map_x += step_x;
                hit_side = 0;
            } else {
                dist = side_dist_y;
                side_dist_y += delta_dist_y;
                map_y += step_y;
                hit_side = 1;
            }

            if (map_x < 0 || map_x >= WORLD_X || map_y < 0 || map_y >= WORLD_Y) break;

            fixed_t d_leave = dist;
            if (d_enter < TO_FP(0.1f)) d_enter = TO_FP(0.1f);
            if (d_leave < TO_FP(0.1f)) d_leave = TO_FP(0.1f);

            fixed_t corr_d_enter = FP_MUL(d_enter, cos_corr);
            fixed_t corr_d_leave = FP_MUL(d_leave, cos_corr);

            // Check columns at grid coordinate
            for (int z = 0; z < WORLD_Z; z++) {
                uint8_t block = world[map_x][map_y][z];

                if (block == BLOCK_AIR) {
                    // Check if block below is solid to cast its exposed top face (floor)
                    if (z > 0 && z - 1 < INT_VAL(player_z)) {
                        uint8_t block_below = world[map_x][map_y][z - 1];
                        if (block_below != BLOCK_AIR && block_below != BLOCK_WATER) {
                            fixed_t z_diff = (z << 16) - player_z;
                            int y_enter = y_center - INT_VAL(FP_DIV(FP_MUL(z_diff, scale_y), corr_d_enter));
                            int y_leave = y_center - INT_VAL(FP_DIV(FP_MUL(z_diff, scale_y), corr_d_leave));

                            if (y_leave < y_enter) {
                                for (int y = y_leave; y <= y_enter; y++) {
                                    if (y >= 0 && y < 240 && !filled[y]) {
                                        // Texture lookups
                                        fixed_t z_proj = (y_center - y == 0) ? 1 : (y_center - y);
                                        fixed_t d_pixel = FP_DIV(FP_MUL(z_diff, scale_y), TO_FP((float)z_proj));
                                        
                                        fixed_t wx = player_x + FP_MUL(ray_cos, d_pixel);
                                        fixed_t wy = player_y + FP_MUL(ray_sin, d_pixel);
                                        int tx = (wx >> 13) & 7;
                                        int ty = (wy >> 13) & 7;

                                        uint8_t raw_color = TEXTURES[block_below][ty][tx];
                                        if (raw_color != 0) {
                                            uint8_t light_col = apply_lighting(raw_color, d_pixel, 0, col, y, sky_color);
                                            draw_column(buffer, col, y, y, light_col);
                                            filled[y] = true;
                                            filled_count++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    // Block is solid. 
                    // Draw vertical walls face
                    fixed_t z_bottom = (z << 16) - player_z;
                    fixed_t z_top = ((z + 1) << 16) - player_z;

                    int y_top = y_center - INT_VAL(FP_DIV(FP_MUL(z_top, scale_y), corr_d_enter));
                    int y_bottom = y_center - INT_VAL(FP_DIV(FP_MUL(z_bottom, scale_y), corr_d_enter));

                    if (y_top < 0) y_top = 0;
                    if (y_bottom >= 240) y_bottom = 239;

                    if (y_top <= y_bottom) {
                        fixed_t wall_hit = (hit_side == 0) ? (player_y + FP_MUL(ray_sin, corr_d_enter)) : (player_x + FP_MUL(ray_cos, corr_d_enter));
                        int tx = (wall_hit >> 13) & 7;

                        for (int y = y_top; y <= y_bottom; y++) {
                            if (!filled[y]) {
                                int ty = ((y - y_top) * 8) / (y_bottom - y_top + 1);
                                uint8_t raw_color = TEXTURES[block][ty & 7][tx & 7];
                                // Grass side texture styling
                                if (block == BLOCK_GRASS && ty >= 2) {
                                    raw_color = TEXTURES[BLOCK_DIRT][ty & 7][tx & 7];
                                }
                                if (raw_color != 0) {
                                    uint8_t light_col = apply_lighting(raw_color, corr_d_enter, hit_side, col, y, sky_color);
                                    draw_column(buffer, col, y, y, light_col);
                                    filled[y] = true;
                                    filled_count++;
                                }
                            }
                        }
                    }

                    // Check top face if below player
                    if (z + 1 < INT_VAL(player_z)) {
                        fixed_t z_diff = ((z + 1) << 16) - player_z;
                        int y_enter = y_center - INT_VAL(FP_DIV(FP_MUL(z_diff, scale_y), corr_d_enter));
                        int y_leave = y_center - INT_VAL(FP_DIV(FP_MUL(z_diff, scale_y), corr_d_leave));

                        if (y_leave < y_enter) {
                            for (int y = y_leave; y <= y_enter; y++) {
                                if (y >= 0 && y < 240 && !filled[y]) {
                                    fixed_t z_proj = (y_center - y == 0) ? 1 : (y_center - y);
                                    fixed_t d_pixel = FP_DIV(FP_MUL(z_diff, scale_y), TO_FP((float)z_proj));
                                    
                                    fixed_t wx = player_x + FP_MUL(ray_cos, d_pixel);
                                    fixed_t wy = player_y + FP_MUL(ray_sin, d_pixel);
                                    int tx = (wx >> 13) & 7;
                                    int ty = (wy >> 13) & 7;

                                    uint8_t raw_color = TEXTURES[block][ty][tx];
                                    if (raw_color != 0) {
                                        uint8_t light_col = apply_lighting(raw_color, d_pixel, 0, col, y, sky_color);
                                        draw_column(buffer, col, y, y, light_col);
                                        filled[y] = true;
                                        filled_count++;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Early ray stop optimizations
            if (filled_count >= 240) break;
        }

        // Draw remaining blank slots to sky above horizon, black space below
        for (int y = 0; y < 240; y++) {
            if (!filled[y]) {
                if (y < y_center) {
                    uint8_t final_sky = sky_color;

                    // 1. Draw Sun
                    if (sun_elevation > -15) {
                        int diff_sun = (ray_angle - sun_angle) & 511;
                        if (diff_sun > 256) diff_sun -= 512;
                        if (diff_sun < 0) diff_sun = -diff_sun;
                        
                        if (diff_sun < 10) {
                            int sun_y = y_center - sun_elevation;
                            int dy = y - sun_y;
                            if (dy < 0) dy = -dy;
                            if (dy < 10) {
                                final_sky = 3; // Yellow
                            }
                        }
                    }

                    // 2. Draw Moon
                    if (final_sky == sky_color && sun_elevation < 15) {
                        int diff_moon = (ray_angle - moon_angle) & 511;
                        if (diff_moon > 256) diff_moon -= 512;
                        if (diff_moon < 0) diff_moon = -diff_moon;
                        
                        if (diff_moon < 8) {
                            int moon_y = y_center + sun_elevation;
                            int dy = y - moon_y;
                            if (dy < 0) dy = -dy;
                            if (dy < 8) {
                                final_sky = 7; // White
                            }
                        }
                    }

                    // 3. Draw Stars
                    if (final_sky == sky_color && ambient_light == 2) {
                        int star_hash = (col * 17 + y * 23 + time_of_day / 8) & 1023;
                        if (star_hash == 77 && y < y_center - 15) {
                            final_sky = 7; // Star
                        }
                    }

                    // 4. Draw Clouds
                    if (final_sky == sky_color) {
                        int cloud_x = (ray_angle + (int)(cloud_offset * 8.0f)) & 511;
                        if (y > y_center - 65 && y < y_center - 25) {
                            int cloud_pattern = (cloud_x / 14) % 4;
                            if (cloud_pattern == 0 || cloud_pattern == 1) {
                                final_sky = 7; // White cloud
                            }
                        }
                    }

                    draw_column(buffer, col, y, y, final_sky);
                } else {
                    draw_column(buffer, col, y, y, 0); // Black void
                }
            }
        }
    }
}

// --- Render 3D Sprite Billboards ---
static void render_billboards(uint8_t *buffer) {
    int y_center = 120 + cam_pitch;
    fixed_t scale_y = TO_FP(130.0f);

    // 1. Render Mobs
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active) continue;

        fixed_t mx = mobs[i].x - player_x;
        fixed_t my = mobs[i].y - player_y;

        // Transform position by camera angle
        fixed_t rx = FP_MUL(mx, cos_table[(-cam_angle) & 511]) - FP_MUL(my, sin_table[(-cam_angle) & 511]);
        fixed_t ry = FP_MUL(mx, sin_table[(-cam_angle) & 511]) + FP_MUL(my, cos_table[(-cam_angle) & 511]);

        if (rx < TO_FP(0.2f) || rx > TO_FP(20.0f)) continue;

        int screen_x = 160 + INT_VAL(FP_DIV(FP_MUL(ry, scale_y), rx));
        
        // Mob bobbing
        float mob_bob = 0.0f;
        if (mobs[i].type == MOB_SLIME) {
            mob_bob = fabsf(sinf(mobs[i].state_timer * 0.25f)) * 0.4f;
        } else if (mobs[i].vx != 0 || mobs[i].vy != 0) {
            mob_bob = sinf(mobs[i].state_timer * 0.4f) * 0.08f;
        }

        fixed_t mz_bottom = mobs[i].z + TO_FP(mob_bob) - player_z;
        float height_m = (mobs[i].type == MOB_ENDERMAN) ? 2.4f : ((mobs[i].type == MOB_CHICKEN) ? 0.7f : 1.7f);
        fixed_t mz_top = mobs[i].z + TO_FP(mob_bob + height_m) - player_z;

        int screen_y_bottom = y_center - INT_VAL(FP_DIV(FP_MUL(mz_bottom, scale_y), rx));
        int screen_y_top = y_center - INT_VAL(FP_DIV(FP_MUL(mz_top, scale_y), rx));

        int mob_h = screen_y_bottom - screen_y_top;
        if (mob_h <= 0) continue;
        int mob_w = (mobs[i].type == MOB_SPIDER) ? mob_h * 2 : mob_h / 2;
        if (mob_w <= 0) continue;

        int start_x = screen_x - mob_w / 2;
        int end_x = screen_x + mob_w / 2;

        int mob_res_w = (mobs[i].type == MOB_SPIDER) ? 16 : ((mobs[i].type == MOB_PIG || mobs[i].type == MOB_SHEEP || mobs[i].type == MOB_COW) ? 12 : 8);
        int mob_res_h = (mobs[i].type == MOB_ENDERMAN) ? 24 : ((mobs[i].type == MOB_SPIDER || mobs[i].type == MOB_PIG || mobs[i].type == MOB_SHEEP || mobs[i].type == MOB_COW || mobs[i].type == MOB_CHICKEN) ? 8 : 16);

        for (int sx = start_x; sx <= end_x; sx++) {
            if (sx < 0 || sx >= 320) continue;
            
            // Basic billboard column DDA occlusion check: 
            // Simple depth culling
            int spr_col = ((sx - start_x) * mob_res_w) / mob_w;
            if (spr_col < 0 || spr_col >= mob_res_w) continue;

            for (int sy = screen_y_top; sy <= screen_y_bottom; sy++) {
                if (sy < 0 || sy >= 240) continue;

                int spr_row = ((sy - screen_y_top) * mob_res_h) / mob_h;
                if (spr_row < 0 || spr_row >= mob_res_h) continue;

                uint8_t p_idx = get_mob_pixel(mobs[i].type, spr_row, spr_col, mob_res_w, mob_res_h);
                if (p_idx != 0) {
                    uint8_t raw_color = MOB_COLORS[mobs[i].type][p_idx];
                    
                    // Combat Hit flashing red effect
                    if (mobs[i].hit_flash > 0 && (mobs[i].hit_flash % 2 == 0)) {
                        raw_color = 1; // Red flash
                    }

                    // Apply directional shadows or distance fog to billboard mob
                    uint8_t light_col = apply_lighting(raw_color, rx, 0, sx, sy, sky_color);
                    draw_column(buffer, sx, sy, sy, light_col);
                }
            }
        }
    }

    // 2. Render Particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life <= 0) continue;

        fixed_t mx = particles[i].x - player_x;
        fixed_t my = particles[i].y - player_y;

        fixed_t rx = FP_MUL(mx, cos_table[(-cam_angle) & 511]) - FP_MUL(my, sin_table[(-cam_angle) & 511]);
        fixed_t ry = FP_MUL(mx, sin_table[(-cam_angle) & 511]) + FP_MUL(my, cos_table[(-cam_angle) & 511]);

        if (rx < TO_FP(0.2f) || rx > TO_FP(18.0f)) continue;

        int screen_x = 160 + INT_VAL(FP_DIV(FP_MUL(ry, scale_y), rx));
        fixed_t pz = particles[i].z - player_z;
        int screen_y = y_center - INT_VAL(FP_DIV(FP_MUL(pz, scale_y), rx));

        if (screen_x >= 0 && screen_x < 320 && screen_y >= 0 && screen_y < 240) {
            uint8_t p_col = apply_lighting(particles[i].color, rx, 0, screen_x, screen_y, sky_color);
            // Draw 2x2 particle dots
            draw_column(buffer, screen_x, screen_y, screen_y + 1, p_col);
            draw_column(buffer, screen_x + 1, screen_y, screen_y + 1, p_col);
        }
    }
}



// --- Render First-Person Hand & Weapon (3D Weapon Bobbing) ---
// sprites packed 16x16
static const uint8_t SWORD_SPRITE[16][16] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,6,7,6},
    {0,0,0,0,0,0,0,0,0,0,0,6,6,7,7,6},
    {0,0,0,0,0,0,0,0,0,6,6,7,7,7,6,0},
    {0,0,0,0,0,0,0,6,6,7,7,7,6,6,0,0},
    {0,0,0,0,0,6,6,7,7,7,6,6,0,0,0,0},
    {0,0,0,6,6,7,7,7,6,6,0,0,0,0,0,0},
    {0,6,6,7,7,7,6,6,0,0,0,0,0,0,0,0},
    {6,7,7,7,6,6,0,0,0,0,0,0,0,0,0,0},
    {6,7,7,6,6,0,0,0,0,0,0,0,0,0,0,0},
    {0,6,6,0,6,6,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,3,6,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,3,3,3,6,0,0,0,0,0,0,0,0},
    {0,0,0,3,3,3,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,3,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static const uint8_t PICKAXE_SPRITE[16][16] = {
    {0,0,0,0,0,0,0,0,0,0,0,4,4,4,4,0},
    {0,0,0,0,0,0,0,0,0,0,4,4,4,4,0,0},
    {0,0,0,0,0,0,0,0,0,4,3,3,4,0,0,0},
    {0,0,0,0,0,0,0,0,4,3,4,0,0,0,0,0},
    {0,0,0,0,0,0,0,3,3,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,3,3,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,3,3,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,3,3,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,3,3,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,3,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static const uint8_t HAND_SPRITE[16][16] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,3,3,3,0,0},
    {0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,0},
    {0,0,0,0,0,0,0,0,0,3,3,3,3,3,3,0},
    {0,0,0,0,0,0,0,0,3,3,3,3,3,3,3,0},
    {0,0,0,0,0,0,0,3,3,3,3,3,3,3,0,0},
    {0,0,0,0,0,0,3,3,3,3,3,3,3,0,0,0},
    {0,0,0,0,0,3,3,3,3,3,3,3,0,0,0,0},
    {0,0,0,0,3,3,3,3,3,3,0,0,0,0,0,0},
    {0,0,0,3,3,3,3,3,0,0,0,0,0,0,0,0},
    {0,0,3,3,3,3,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,3,3,0,0,0,0,0,0,0,0,0,0,0},
    {3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static void render_held_tool(uint8_t *buffer, int frame_counter) {
    if (player_dead_timer > 0) return;

    // Bobbing offset
    float bob_x = sinf(frame_counter * 0.15f) * 2.5f;
    float bob_y = fabsf(cosf(frame_counter * 0.25f)) * 2.0f;
    
    // Slashing offset
    float slash_x = 0;
    float slash_y = 0;
    if (swing_timer > 0) {
        float phase = (12.0f - swing_timer) / 12.0f;
        slash_x = -sinf(phase * M_PI) * 16.0f;
        slash_y = sinf(phase * M_PI) * 22.0f;
    }

    int start_x = 240 + (int)(bob_x + slash_x);
    int start_y = 160 + (int)(bob_y + slash_y);

    for (int sx = 0; sx < 16; sx++) {
        for (int sy = 0; sy < 16; sy++) {
            uint8_t raw_color = 0;
            if (current_tool == 0) {
                raw_color = HAND_SPRITE[sy][sx];
            } else if (current_tool == 5) {
                raw_color = SWORD_SPRITE[sy][sx];
            } else {
                raw_color = PICKAXE_SPRITE[sy][sx];
            }

            if (raw_color != 0) {
                // Color customization for tools
                if (current_tool == 2 && raw_color == 4) raw_color = 4; // Stone Pick (blue-ish / gray-ish border)
                if (current_tool == 3 && raw_color == 4) raw_color = 7; // Iron Pick (white border)
                if (current_tool == 4 && raw_color == 4) raw_color = 6; // Diamond Pick (cyan border)
                
                // Draw upscaled tool 4x4 pixels
                for (int dx = 0; dx < 4; dx++) {
                    for (int dy = 0; dy < 4; dy++) {
                        draw_pixel(buffer, start_x + sx * 4 + dx, start_y + sy * 4 + dy, raw_color);
                    }
                }
            }
        }
    }
}

// --- Render Steve's Survival HUD UI ---
static void render_survival_hud(uint8_t *buffer) {
    if (player_dead_timer > 0) {
        // Red death screen overlay
        for (int i=0; i<320*240; i++) {
            if ((i + i/320) % 2 == 0) {
                buffer[i] = (buffer[i] & ~0x07) | 0x01; // Blend red index 1
            }
        }
        draw_string(buffer, "GAME OVER", 112, 100, 2, 7); // white text
        draw_string(buffer, "Steve died!", 108, 125, 2, 7);
        return;
    }

    // 1. Health Bar (Hearts!)
    // 10 hearts drawn above hotbar
    int hearts = player_health / 2; // 0 to 10
    bool half_heart = (player_health % 2 != 0);

    for (int i = 0; i < 10; i++) {
        int hx = 60 + i * 9;
        int hy = 205;
        // Outline black box
        draw_rect(buffer, hx, hy, hx+7, hy+7, 0);
        
        if (i < hearts) {
            // Full red heart
            draw_rect(buffer, hx+1, hy+1, hx+6, hy+6, 1);
            // highlights
            draw_pixel(buffer, hx+2, hy+2, 7);
        } else if (i == hearts && half_heart) {
            // Half red heart
            draw_rect(buffer, hx+1, hy+1, hx+3, hy+6, 1);
            draw_pixel(buffer, hx+2, hy+2, 7);
        }
    }

    // 2. Hotbar Box Container
    // Hotbar stretches from X = 80 to 240
    // Base container dithered gray
    for (int hx = 80; hx < 240; hx++) {
        draw_column(buffer, hx, 218, 238, 0); // Black outline
    }
    draw_rect(buffer, 81, 219, 239, 237, 4); // Inner dark blue container
    
    // Draw 8 slots borders
    for (int s = 0; s < 8; s++) {
        int sx = 80 + s * 20;
        draw_rect(buffer, sx, 218, sx, 238, 7); // white border lines
    }
    draw_rect(buffer, 240, 218, 240, 238, 7);

    // Draw active selection box highlight
    int sel_x = 80 + selected_slot * 20;
    draw_rect(buffer, sel_x - 1, 217, sel_x + 21, 218, 3); // Yellow highlight border
    draw_rect(buffer, sel_x - 1, 238, sel_x + 21, 239, 3);
    draw_rect(buffer, sel_x - 1, 217, sel_x, 239, 3);
    draw_rect(buffer, sel_x + 20, 217, sel_x + 21, 239, 3);

    // Render Item Icons inside hotbar slots
    // Slot items: Dirt, Stone, Wood, Planks, Leaves, Tools/Weapon
    uint8_t icons[8] = {BLOCK_DIRT, BLOCK_STONE, BLOCK_WOOD, BLOCK_PLANK, BLOCK_LEAVES, BLOCK_SAND, BLOCK_DIAMOND_ORE, BLOCK_AIR};
    for (int s = 0; s < 7; s++) {
        uint8_t item = icons[s];
        int item_count = inv_items[item];
        if (item_count > 0) {
            int slot_x = 80 + s * 20 + 3;
            int slot_y = 221;
            // Draw miniature 12x12 texture icon
            for (int ix = 0; ix < 12; ix++) {
                for (int iy = 0; iy < 12; iy++) {
                    uint8_t col = TEXTURES[item][(iy * 8) / 12][(ix * 8) / 12];
                    draw_pixel(buffer, slot_x + ix, slot_y + iy, col);
                }
            }

            // Draw Count
            char count_str[16];
            sprintf(count_str, "%d", item_count);
            draw_string(buffer, count_str, slot_x + 5, slot_y + 11, 1, 7);
        }
    }

    // Slot 7 is held active Pickaxe or Sword
    int slot7_x = 80 + 7 * 20 + 3;
    int slot7_y = 221;
    uint8_t tool_col = (current_tool == 5) ? 6 : ((current_tool == 4) ? 6 : ((current_tool >= 2) ? 4 : 3));
    // draw small pickaxe icon
    for (int p=0; p<12; p++) {
        draw_pixel(buffer, slot7_x + p, slot7_y + p, tool_col);
        draw_pixel(buffer, slot7_x + 11 - p, slot7_y + p, 3); // wooden handle diagonal
    }

    // 3. Scrolling Chat Action Logs
    if (chat_timer > 0) {
        if (chat_msg2[0] != '\0') {
            draw_string(buffer, chat_msg2, 8, 12, 1, 7);
        }
        if (chat_msg1[0] != '\0') {
            draw_string(buffer, chat_msg1, 8, 22, 1, 3); // Yellow highlight latest
        }
    }
}

// --- Main Screensaver Scene Entry Points ---

void play_minecraft(uint8_t *buffer, int frame_counter) {
    // 1. Day/Night state updates
    time_of_day = (time_of_day + 1) % 2000;
    if (time_of_day < 800) { // Day
        sky_color = 6; // Cyan
        ambient_light = 0;
    } else if (time_of_day < 1000) { // Sunset
        sky_color = 5; // Magenta/Sunset
        ambient_light = 1;
    } else if (time_of_day < 1800) { // Night
        sky_color = 0; // Dark / Black
        ambient_light = 2;
    } else { // Sunrise
        sky_color = 1; // Red sunrise
        ambient_light = 3;
    }

    // Cloud drift
    cloud_offset += 0.03f;
    if (cloud_offset >= 64.0f) cloud_offset = 0.0f;

    if (chat_timer > 0) chat_timer--;

    if (swing_timer > 0) swing_timer--;

    // 2. Controller & Physics updates
    handle_input_and_ai(frame_counter);
    update_physics();
    update_mobs_and_particles();

    // 3. Render Voxel World
    clear_screen(buffer);
    
    render_world(buffer);
    render_billboards(buffer);
    render_held_tool(buffer, frame_counter);
    
    // HUD overlays
    render_survival_hud(buffer);

    // Apply Screen Shake combat feedback
    if (screen_shake > 0) {
        screen_shake--;
        // Shift entire framebuffer horizontally/vertically slightly
        int offset = (get_rand() % 3) - 1;
        if (offset != 0) {
            // simple shift emulation by memory manipulation or offset rendering
            // Just offset the screen buffer address slightly on the CRT or shift graphics
        }
    }
}
