#include "common.h"

// --- Constants ---
#define LEVEL_WIDTH     600
#define CAMERA_MAX      (LEVEL_WIDTH - 320)
#define NUM_BUBBLES     12
#define BOMB_TICK_MAX   25

// Ground / floor height
#define FLOOR_Y         210
// Lava surface height (top of lava pool)
#define LAVA_TOP_Y      218
// Platform 1 (x 260–290) top surface
#define PLAT1_TOP_Y     179
#define PLAT1_X1        260
#define PLAT1_X2        290
// Platform 2 (x 310–350) top surface
#define PLAT2_TOP_Y     159
#define PLAT2_X1        308
#define PLAT2_X2        352
// Lava pool horizontal extents (world coords)
#define LAVA_X1         240
#define LAVA_X2         380
// X where Samus should stand up from morph ball
#define STANDUP_X       305
// Approx y of Samus feet when standing on platform surface
// Samus sprite draws from y upward ~18px, so "feet" are at y+8
#define SAMUS_FOOT_OFFSET 8

// --- Structs ---
typedef struct {
    int x, y;
    bool active;
    int speed;
} Bubble;

typedef struct {
    int x, y;
    bool active;
    int vx;
    bool is_missile;
} Projectile;

// --- State Variables ---
static int samus_x = 20;
static int samus_y = FLOOR_Y;
static int samus_vx = 2;
static int samus_vy = 0;
static int samus_score = 0;
static int camera_x = 0;

static enum {
    STATE_RUN_ZOOMER,
    STATE_JUMP_PLATFORM,
    STATE_DODGE_SKREE,
    STATE_ROLL_TUNNEL,
    STATE_BOMB_DROP,
    STATE_CROSS_LAVA,
    STATE_BOSS_INTRO,
    STATE_BOSS_FIGHT,
    STATE_GET_ENERGY,
    STATE_FADE_RESET
} metroid_state;

// Sub-states and Timers
static int metroid_timer = 0;
static bool samus_is_morph = false;
static int samus_anim_frame = 0;
static int samus_jump_timer = 0;

// Enemies
static int zoomer_x = 110;
static bool zoomer_alive = true;
static int skree_x = 200;
static int skree_y = 50;
static enum { SKREE_CEILING, SKREE_DIVING, SKREE_DEAD } skree_state;

// Boss: Giant Metroid
static int boss_x = 480;
static int boss_y = 150;
static int boss_hp = 5;
static int boss_flash_timer = 0;
static int boss_swoop_timer = 0;
static bool boss_alive = true;
static bool energy_tank_spawned = false;
static int energy_tank_x = 480;
static int energy_tank_y = FLOOR_Y;

// Projectiles
static Projectile projs[3];
static Bubble bubbles[NUM_BUBBLES];

// Bomb
static int bomb_x = 0;
static int bomb_y = 0;
static int bomb_timer = 0; // 0 = inactive, >0 ticking
static bool brick_destroyed = false;

// Explode animations
static int explode_x = 0;
static int explode_y = 0;
static int explode_timer = 0;

// Visor light flicker
static int visor_timer = 0;

// Lava heat shimmer counter
static int lava_shimmer = 0;

void init_metroid() {
    samus_x = 20;
    samus_y = FLOOR_Y;
    samus_vx = 2;
    samus_vy = 0;
    samus_score = 0;
    camera_x = 0;
    
    metroid_state = STATE_RUN_ZOOMER;
    metroid_timer = 0;
    samus_is_morph = false;
    samus_anim_frame = 0;
    samus_jump_timer = 0;

    zoomer_x = 110;
    zoomer_alive = true;

    skree_x = 200;
    skree_y = 50;
    skree_state = SKREE_CEILING;

    boss_x = 480;
    boss_y = 150;
    boss_hp = 5;
    boss_flash_timer = 0;
    boss_swoop_timer = 0;
    boss_alive = true;
    energy_tank_spawned = false;

    bomb_timer = 0;
    brick_destroyed = false;
    explode_timer = 0;

    lava_shimmer = 0;
    visor_timer = 0;

    for (int i = 0; i < 3; i++) projs[i].active = false;
    
    // Bubble particles seeded across the lava pool
    for (int i = 0; i < NUM_BUBBLES; i++) {
        bubbles[i].x = LAVA_X1 + 5 + (get_rand() % (LAVA_X2 - LAVA_X1 - 10));
        bubbles[i].y = 222 + (get_rand() % 14);
        bubbles[i].speed = 1 + (get_rand() % 2);
        bubbles[i].active = true;
    }
}

// -----------------------------------------------------------------------
// SNES-style cavern rock tile: dark multi-layer rock with highlighted top
// -----------------------------------------------------------------------
static void draw_cavern_block(uint8_t *buffer, int x1, int y1, int x2, int y2) {
    // Base deep rock: dark purple/black mix with subtle column lines
    draw_rect(buffer, x1, y1, x2, y2, 5);            // Dark purple base

    // Deep shadow layer (1px indent)
    if (y2 - y1 > 3) {
        draw_rect(buffer, x1 + 1, y2 - 1, x2 - 1, y2, 0); // Dark bottom
        draw_rect(buffer, x1, y1 + 2, x1 + 1, y2 - 2, 0); // Dark left edge
    }

    // Crack/grout vertical lines (every 14px)
    for (int x = x1 + 8; x <= x2; x += 14) {
        draw_pixel(buffer, x, y1 + 2, 0);
        if (y2 - y1 > 6) {
            draw_pixel(buffer, x, y1 + 4, 0);
            draw_pixel(buffer, x + 1, y1 + 6, 0);
        }
    }

    // Glowing cyan lichen highlight on top surface (SNES Brinstar look)
    draw_rect(buffer, x1, y1, x2, y1, 6);             // Bright cyan top edge
    draw_rect(buffer, x1 + 1, y1 + 1, x2 - 1, y1 + 1, 7); // White inner highlight
    draw_rect(buffer, x1 + 2, y1 + 2, x2 - 2, y1 + 2, 6); // Second cyan row
}

// -----------------------------------------------------------------------
// More detailed SNES-style Samus Varia Suit
// -----------------------------------------------------------------------
static void draw_varia_samus(uint8_t *buffer, int x, int y, int frame_counter) {
    if (samus_is_morph) {
        // --- Morph Ball: orange/yellow layered sphere with rotating stripe ---
        int rotation = (frame_counter / 2) % 8;

        art_circle_shaded(buffer, x, y + 4, 7, 1, 3);
        art_circle_shaded(buffer, x, y + 4, 5, 1, 3);
        art_circle_shaded(buffer, x, y + 4, 3, 1, 3);
        draw_circle(buffer, x, y + 4, 1, 3);  // Yellow nucleus

        // Rotating black stripe lines
        if (rotation == 0 || rotation == 4) {
            draw_rect(buffer, x - 6, y + 4, x + 6, y + 4, 0);
        } else if (rotation == 1 || rotation == 5) {
            draw_pixel(buffer, x - 5, y + 2, 0);
            draw_pixel(buffer, x + 5, y + 6, 0);
            draw_pixel(buffer, x - 3, y + 1, 0);
            draw_pixel(buffer, x + 3, y + 7, 0);
        } else if (rotation == 2 || rotation == 6) {
            draw_rect(buffer, x, y - 2, x, y + 10, 0);
        } else {
            draw_pixel(buffer, x + 5, y + 2, 0);
            draw_pixel(buffer, x - 5, y + 6, 0);
            draw_pixel(buffer, x + 3, y + 1, 0);
            draw_pixel(buffer, x - 3, y + 7, 0);
        }
        return;
    }

    // --- Standing/Running Samus (SNES Varia Suit) ---
    // Leg animation: 3-frame walk cycle
    int walk = 0;
    if (samus_vx > 0) walk = (frame_counter / 5) % 3;
    if (samus_vy != 0) walk = 2; // Jump/fall frame

    // Legs (orange-red boots, green knee joints)
    if (walk == 0) {
        // Idle stance
        draw_rect(buffer, x - 3, y + 1, x - 1, y + 8, 1); // Left leg
        draw_rect(buffer, x + 1, y + 1, x + 3, y + 8, 1); // Right leg
        draw_rect(buffer, x - 3, y + 6, x - 1, y + 8, 1); // Left boot
        draw_rect(buffer, x + 1, y + 6, x + 3, y + 8, 1); // Right boot
    } else if (walk == 1) {
        draw_rect(buffer, x - 4, y + 1, x - 2, y + 7, 1);
        draw_rect(buffer, x, y + 2, x + 2, y + 6, 1);
        draw_rect(buffer, x - 5, y + 7, x - 2, y + 8, 1); // Extended boot
        draw_rect(buffer, x, y + 6, x + 2, y + 8, 1);
    } else { // walk == 2
        draw_rect(buffer, x - 2, y + 1, x, y + 7, 1);
        draw_rect(buffer, x + 2, y + 1, x + 4, y + 7, 1);
        draw_rect(buffer, x - 2, y + 6, x, y + 8, 1);
        draw_rect(buffer, x + 2, y + 7, x + 5, y + 8, 1); // Extended boot
    }

    // Green knee-joint dots
    draw_pixel(buffer, x - 2, y + 3, 2);
    draw_pixel(buffer, x + 2, y + 3, 2);

    // Torso / Varia Breastplate (red with yellow centre stripe)
    draw_rect(buffer, x - 3, y - 4, x + 3, y, 1);         // Red torso
    draw_rect(buffer, x - 1, y - 3, x + 1, y - 1, 3);     // Yellow chest energy cell
    draw_pixel(buffer, x,    y - 2, 7);                    // Bright white highlight

    // Belt / waist (dark line)
    draw_rect(buffer, x - 3, y, x + 3, y, 0);

    // Shoulder pads (large rounded "bubbles" — SNES Varia suit)
    // Left pad
    draw_circle(buffer, x - 5, y - 4, 3, 1);  // Orange-red pad
    draw_pixel(buffer, x - 5, y - 6, 3);      // Yellow highlight dot
    draw_pixel(buffer, x - 7, y - 4, 3);
    // Right pad
    draw_circle(buffer, x + 5, y - 4, 3, 1);
    draw_pixel(buffer, x + 5, y - 6, 3);
    draw_pixel(buffer, x + 7, y - 4, 3);

    // Helmet (round, orange, with dark visor band)
    draw_circle(buffer, x, y - 9, 5, 1);            // Orange helmet dome
    draw_rect(buffer, x - 3, y - 10, x + 3, y - 12, 0); // Dark top of helmet
    draw_pixel(buffer, x - 2, y - 11, 1);            // Orange on dark top

    // Visor (glowing neon green/cyan with flicker, SNES style)
    visor_timer++;
    uint8_t visor_c = (visor_timer % 40 < 32) ? 2 : 6; // Green mostly, flash cyan
    draw_rect(buffer, x + 1, y - 10, x + 3, y - 8, visor_c); // Visor window
    draw_pixel(buffer, x + 4, y - 9, 7); // Visor shine pixel

    // Arm Cannon (barrel pointing right — longer, SNES style)
    draw_rect(buffer, x + 3, y - 5, x + 10, y - 3, 2);  // Green cannon body
    draw_rect(buffer, x + 9, y - 6, x + 10, y - 2, 2);  // Muzzle flare guard
    draw_pixel(buffer, x + 10, y - 4, 3);               // Cannon tip yellow glow

    // Dark arm connecting shoulder to cannon
    draw_rect(buffer, x + 3, y - 4, x + 5, y - 3, 5);   // Dark purple joint
}

// -----------------------------------------------------------------------
// Draw the Giant Metroid Boss – more SNES-faithful detail
// -----------------------------------------------------------------------
static void draw_metroid_boss(uint8_t *buffer, int x, int y) {
    if (!boss_alive) return;

    // Flash white when hit, else green dome
    uint8_t dome_c = (boss_flash_timer > 0 && (boss_flash_timer % 4 < 2)) ? 7 : 2;
    uint8_t nuke_c = (boss_flash_timer > 0 && (boss_flash_timer % 4 < 2)) ? 7 : 1;

    // Outer translucent dome (large circle outline + fill)
    draw_circle(buffer, x, y - 4, 14, dome_c); // Outer green dome
    draw_circle(buffer, x, y - 4, 12, 0);      // Hollow interior (black cutout)
    draw_circle(buffer, x, y - 4, 10, dome_c); // Inner ring rim
    draw_circle(buffer, x, y - 4,  8, 0);      // Second hollow

    // Four red nuclei (SNES Metroid has 4 inner spheres)
    draw_circle(buffer, x - 4, y - 8, 3, nuke_c);
    draw_circle(buffer, x + 4, y - 8, 3, nuke_c);
    draw_circle(buffer, x - 4, y,     3, nuke_c);
    draw_circle(buffer, x + 4, y,     3, nuke_c);

    // Yellow nucleus highlights
    draw_pixel(buffer, x - 3, y - 9, 3);
    draw_pixel(buffer, x + 3, y - 9, 3);
    draw_pixel(buffer, x - 3, y - 1, 3);
    draw_pixel(buffer, x + 3, y - 1, 3);

    // Hanging tentacles (gray, wavy)
    int wave = (boss_flash_timer > 0) ? 0 : (visor_timer / 8) % 3;
    draw_rect(buffer, x - 7 + wave, y + 8,  x - 6 + wave, y + 18, 7);
    draw_rect(buffer, x - 1,        y + 10, x + 1,         y + 20, 7);
    draw_rect(buffer, x + 6 - wave, y + 8,  x + 7 - wave,  y + 18, 7);
    // Claw tips
    draw_pixel(buffer, x - 8 + wave, y + 17, 7);
    draw_pixel(buffer, x - 2,        y + 20, 7);
    draw_pixel(buffer, x + 2,        y + 20, 7);
    draw_pixel(buffer, x + 8 - wave, y + 17, 7);
}

// -----------------------------------------------------------------------
// Draw animated lava pool (multiple layers + heat shimmer)
// -----------------------------------------------------------------------
static void draw_lava_pool(uint8_t *buffer, int cam_x, int shimmer) {
    int lx1 = LAVA_X1 - cam_x;
    int lx2 = LAVA_X2 - cam_x;

    // Deep lava body (dark red/orange)
    draw_rect(buffer, lx1, LAVA_TOP_Y + 2, lx2, 239, 1);

    // Bright orange mid layer
    draw_rect(buffer, lx1 + 1, LAVA_TOP_Y + 1, lx2 - 1, LAVA_TOP_Y + 3, 1);

    // Glowing yellow crust top (boiling skin)
    draw_rect(buffer, lx1, LAVA_TOP_Y, lx2, LAVA_TOP_Y, 3);

    // Shimmer pixels along crust (alternating yellow/white)
    for (int sx = lx1; sx <= lx2; sx += 4) {
        int offset = ((shimmer + sx) / 3) % 3;
        draw_pixel(buffer, sx + offset, LAVA_TOP_Y - 1, 3);
    }

    // Highlight edge glow just below ceiling
    draw_rect(buffer, lx1, LAVA_TOP_Y + 4, lx1 + 1, 232, 1); // Left wall glow
    draw_rect(buffer, lx2 - 1, LAVA_TOP_Y + 4, lx2, 232, 1); // Right wall glow
}

void play_metroid(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);
    lava_shimmer++;
    
    // Black void backdrop
    draw_rect(buffer, 0, 0, 319, 239, 0);

    // --- SCROLLING CAMERA ---
    camera_x = samus_x - 120;
    if (camera_x < 0) camera_x = 0;
    if (camera_x > CAMERA_MAX) camera_x = CAMERA_MAX;

    // --- DRAW LEVEL GEOMETRY ---
    // Rocky Ceiling (full width)
    draw_cavern_block(buffer, 0 - camera_x, 0, 600 - camera_x, 32);

    // Rocky Ground Left Segment (0 to 240)
    draw_cavern_block(buffer, 0 - camera_x, 219, 240 - camera_x, 239);
    // Rocky Ground Right Segment (380 to 600)
    draw_cavern_block(buffer, 380 - camera_x, 219, 600 - camera_x, 239);

    // Tunnel low-ceiling block (x=250–320, y=175–190)
    draw_cavern_block(buffer, 250 - camera_x, 175, 320 - camera_x, 190);

    // Explodable brick wall at tunnel path (x=280, y=191–218)
    if (!brick_destroyed) {
        int bx = 280 - camera_x;
        // Blue/purple layered bricks with red mortar
        draw_rect(buffer, bx, 191, bx + 15, 218, 4);      // Blue barrier bricks
        draw_rect(buffer, bx + 1, 192, bx + 14, 217, 5);  // Purple mortar fill
        // Brick grout lines
        for (int by = 191; by <= 218; by += 6)
            draw_rect(buffer, bx, by, bx + 15, by, 1);    // Red mortar row lines
        for (int bxl = bx + 4; bxl <= bx + 14; bxl += 5)
            draw_rect(buffer, bxl, 191, bxl, 218, 1);     // Red mortar column lines
    }

    // Floating Platforms over the Lava Pool
    draw_cavern_block(buffer, PLAT1_X1 - camera_x, PLAT1_TOP_Y, PLAT1_X2 - camera_x, PLAT1_TOP_Y + 7);
    draw_cavern_block(buffer, PLAT2_X1 - camera_x, PLAT2_TOP_Y, PLAT2_X2 - camera_x, PLAT2_TOP_Y + 7);

    // --- LAVA POOL (animated) ---
    draw_lava_pool(buffer, camera_x, lava_shimmer);

    // Animate bubble particles
    for (int i = 0; i < NUM_BUBBLES; i++) {
        if (bubbles[i].active) {
            bubbles[i].y -= bubbles[i].speed;
            if (bubbles[i].y < LAVA_TOP_Y) {
                bubbles[i].y = 224 + (get_rand() % 12);
                bubbles[i].x = LAVA_X1 + 3 + (get_rand() % (LAVA_X2 - LAVA_X1 - 6));
            }
            int bx = bubbles[i].x - camera_x;
            if (bx >= 0 && bx < 320) {
                uint8_t bcol = (bubbles[i].y < LAVA_TOP_Y + 4) ? 7 : 3; // white at top, yellow below
                draw_pixel(buffer, bx, bubbles[i].y, bcol);
            }
        }
    }

    // --- SCRIPTED SAMUS GAMEPLAY TIMELINE ---
    metroid_timer++;

    if (metroid_state == STATE_RUN_ZOOMER) {
        samus_x += samus_vx;
        // Shoot yellow beam to kill Zoomer
        if (zoomer_alive && zoomer_x - samus_x < 90 && frame_counter % 20 == 0) {
            for (int i = 0; i < 3; i++) {
                if (!projs[i].active) {
                    projs[i] = (Projectile){samus_x + 10, samus_y - 3, 4, true, false};
                    break;
                }
            }
        }
        if (samus_x >= 120) {
            metroid_state = STATE_JUMP_PLATFORM;
        }

    } else if (metroid_state == STATE_JUMP_PLATFORM) {
        samus_x += samus_vx;
        if (samus_x >= 130 && samus_vy == 0 && samus_y >= FLOOR_Y) {
            samus_vy = -8; // Jump!
        }
        samus_y += samus_vy;
        samus_vy += 1;
        if (samus_y >= FLOOR_Y) {
            samus_y = FLOOR_Y;
            samus_vy = 0;
            if (samus_x >= 165) {
                metroid_state = STATE_DODGE_SKREE;
            }
        }

    } else if (metroid_state == STATE_DODGE_SKREE) {
        samus_x += samus_vx;
        // Skree dives when Samus is near x=180
        if (skree_state == SKREE_CEILING && samus_x >= 170) {
            skree_state = SKREE_DIVING;
        }
        
        if (skree_state == SKREE_DIVING) {
            skree_y += 4;
            skree_x += (frame_counter % 4 < 2) ? 1 : -1; // spin wiggle
            
            if (skree_y >= FLOOR_Y) {
                skree_state = SKREE_DEAD;
                explode_x = skree_x;
                explode_y = FLOOR_Y - 5;
                explode_timer = 12;
            }
        }

        // Shoot at Skree
        if (skree_state == SKREE_DIVING && frame_counter % 15 == 0) {
            for (int i = 0; i < 3; i++) {
                if (!projs[i].active) {
                    projs[i] = (Projectile){samus_x + 10, samus_y - 3, 4, true, false};
                    break;
                }
            }
        }

        if (samus_x >= 240) {
            samus_x = 240;
            metroid_state = STATE_ROLL_TUNNEL;
            samus_is_morph = true; // Morph ball mode!
            samus_vx = 1;
        }

    } else if (metroid_state == STATE_ROLL_TUNNEL) {
        // Roll through low tunnel passage
        samus_x += samus_vx;
        // Keep Samus on tunnel floor (y=190 – ball radius ~8)
        samus_y = 202;
        if (samus_x >= 270) {
            samus_x = 270;
            samus_vx = 0;
            metroid_state = STATE_BOMB_DROP;
            bomb_x = samus_x + 6;
            bomb_y = samus_y + 4;
            bomb_timer = BOMB_TICK_MAX;
        }

    } else if (metroid_state == STATE_BOMB_DROP) {
        // Roll back slightly to escape blast radius
        if (samus_x > 252) {
            samus_x -= 1;
        }
        
        // Bomb ticking down
        if (bomb_timer > 0) {
            bomb_timer--;
            if (bomb_timer == 0) {
                brick_destroyed = true;
                samus_score += 100;
                explode_x = bomb_x;
                explode_y = bomb_y;
                explode_timer = 15;
                // Resume rolling forward, stand up after clearing tunnel
                metroid_state = STATE_CROSS_LAVA;
                samus_vx = 2;
            }
        }

    } else if (metroid_state == STATE_CROSS_LAVA) {
        // ------------------------------------------------------------------
        // Phase 1: still in morph ball, rolling forward out of tunnel
        // Phase 2: stand up, jump across platforms above lava
        // ------------------------------------------------------------------
        samus_x += samus_vx;

        // Stand up once past tunnel exit
        if (samus_is_morph && samus_x >= STANDUP_X) {
            samus_is_morph = false;
            samus_y = FLOOR_Y;
            samus_vy = 0;
        }

        if (!samus_is_morph) {
            // Apply gravity
            samus_y += samus_vy;
            samus_vy += 1;

            // --- Jump triggers (range-based, not exact-pixel) ---
            // Jump 1: from ground onto Platform 1 (x ~305-308, y == FLOOR_Y)
            if (samus_vy == 0 && samus_y >= FLOOR_Y &&
                samus_x >= 306 && samus_x <= 310) {
                samus_vy = -9;
            }
            // Jump 2: from Platform 1 onto Platform 2 (x ~285-290, on plat1)
            if (samus_vy == 0 && samus_y <= PLAT1_TOP_Y + 2 &&
                samus_x >= 280 && samus_x <= 292) {
                samus_vy = -10;
            }

            // --- Platform landing checks ---
            // Land on Platform 1
            if (samus_vy > 0 &&
                samus_x >= PLAT1_X1 && samus_x <= PLAT1_X2 &&
                samus_y + SAMUS_FOOT_OFFSET >= PLAT1_TOP_Y &&
                samus_y + SAMUS_FOOT_OFFSET <= PLAT1_TOP_Y + 6) {
                samus_y  = PLAT1_TOP_Y - SAMUS_FOOT_OFFSET;
                samus_vy = 0;
            }
            // Land on Platform 2
            if (samus_vy > 0 &&
                samus_x >= PLAT2_X1 && samus_x <= PLAT2_X2 &&
                samus_y + SAMUS_FOOT_OFFSET >= PLAT2_TOP_Y &&
                samus_y + SAMUS_FOOT_OFFSET <= PLAT2_TOP_Y + 6) {
                samus_y  = PLAT2_TOP_Y - SAMUS_FOOT_OFFSET;
                samus_vy = 0;
            }
            // Land on right-side ground (past lava pool)
            if (samus_y >= FLOOR_Y) {
                samus_y  = FLOOR_Y;
                samus_vy = 0;
            }

            // Advance to boss intro once Samus safely reaches right ground
            if (samus_y >= FLOOR_Y && samus_x >= 400) {
                metroid_state = STATE_BOSS_INTRO;
                samus_vx = 0;
            }
        } else {
            // Still morphed — stay on tunnel floor
            samus_y = 202;
        }

    } else if (metroid_state == STATE_BOSS_INTRO) {
        samus_x = 400;
        metroid_state = STATE_BOSS_FIGHT;

    } else if (metroid_state == STATE_BOSS_FIGHT) {
        samus_x = 400;
        samus_vx = 0;

        // Metroid AI: hover sinusoidally
        boss_y = 140 + (int)(15 * isqrt(abs((frame_counter % 60) - 30)));
        if (frame_counter % 80 == 0 && boss_alive) {
            boss_swoop_timer = 20;
        }
        if (boss_swoop_timer > 0) {
            boss_swoop_timer--;
            boss_x -= 3;
            boss_y += 2;
            if (boss_x < 420) {
                boss_x = 480;
                boss_y = 150;
            }
        } else {
            if (boss_x < 480) boss_x += 1;
        }

        // Shoot missiles
        if (boss_alive && frame_counter % 30 == 0) {
            for (int i = 0; i < 3; i++) {
                if (!projs[i].active) {
                    projs[i] = (Projectile){samus_x + 10, samus_y - 3, 5, true, true};
                    break;
                }
            }
        }

        if (!boss_alive) {
            metroid_state = STATE_GET_ENERGY;
            samus_vx = 2;
        }

    } else if (metroid_state == STATE_GET_ENERGY) {
        samus_x += samus_vx;
        if (energy_tank_spawned && abs(samus_x - energy_tank_x) < 10) {
            energy_tank_spawned = false;
            samus_score += 500;
            metroid_state = STATE_FADE_RESET;
            metroid_timer = 0;
        }

    } else if (metroid_state == STATE_FADE_RESET) {
        samus_vx = 0;
        if (metroid_timer >= 45) {
            init_metroid();
        }
    }

    // --- UPDATE PROJECTILES & EXPLOSIONS ---
    for (int i = 0; i < 3; i++) {
        if (projs[i].active) {
            projs[i].x += projs[i].vx;
            
            int px = projs[i].x - camera_x;
            if (px >= 0 && px < 320) {
                if (projs[i].is_missile) {
                    // Red rocket missile with white/yellow exhaust trail
                    draw_rect(buffer, px - 7, projs[i].y - 2, px, projs[i].y + 2, 1);  // Red body
                    draw_rect(buffer, px - 9, projs[i].y - 1, px - 8, projs[i].y + 1, 3); // Yellow trail
                    draw_rect(buffer, px - 11, projs[i].y, px - 10, projs[i].y, 7);   // White hot tip
                } else {
                    // Yellow/cyan energy beam with bright tip
                    draw_rect(buffer, px - 5, projs[i].y - 1, px - 1, projs[i].y + 1, 3); // Yellow beam
                    draw_pixel(buffer, px, projs[i].y, 7);   // White front
                    draw_pixel(buffer, px - 6, projs[i].y, 6); // Cyan tail
                }
            }

            // Hit: Zoomer
            if (zoomer_alive && !projs[i].is_missile && abs(projs[i].x - zoomer_x) < 12) {
                zoomer_alive = false;
                projs[i].active = false;
                samus_score += 100;
                explode_x = zoomer_x;
                explode_y = FLOOR_Y;
                explode_timer = 10;
            }

            // Hit: Skree
            if (skree_state == SKREE_DIVING && !projs[i].is_missile &&
                abs(projs[i].x - skree_x) < 12 && abs(projs[i].y - skree_y) < 12) {
                skree_state = SKREE_DEAD;
                projs[i].active = false;
                samus_score += 150;
                explode_x = skree_x;
                explode_y = skree_y;
                explode_timer = 10;
            }

            // Hit: Metroid Boss
            if (boss_alive && projs[i].is_missile &&
                abs(projs[i].x - boss_x) < 16 && abs(projs[i].y - boss_y) < 16) {
                projs[i].active = false;
                boss_hp--;
                boss_flash_timer = 12;
                if (boss_hp <= 0) {
                    boss_alive = false;
                    energy_tank_spawned = true;
                    energy_tank_x = boss_x;
                    energy_tank_y = FLOOR_Y;
                    samus_score += 1000;
                    explode_x = boss_x;
                    explode_y = boss_y;
                    explode_timer = 30;
                }
            }

            // Off-screen cull
            if (projs[i].x > samus_x + 200) projs[i].active = false;
        }
    }

    // Boss flash ticker
    if (boss_flash_timer > 0) boss_flash_timer--;

    // --- DRAW ENEMIES ---
    // Crawling Zoomer (spiky crab on the floor)
    if (zoomer_alive) {
        zoomer_x -= 1;
        int zx = zoomer_x - camera_x;
        if (zx >= -12 && zx < 320) {
            draw_circle(buffer, zx, 213, 6, 1);   // Red main shell
            draw_circle(buffer, zx, 213, 4, 3);   // Yellow inner
            draw_circle(buffer, zx, 213, 2, 1);   // Red core
            // Spikes
            draw_pixel(buffer, zx - 6, 208, 3);
            draw_pixel(buffer, zx + 6, 208, 3);
            draw_pixel(buffer, zx, 206, 3);
            draw_pixel(buffer, zx - 4, 210, 3);
            draw_pixel(buffer, zx + 4, 210, 3);
            // Legs
            draw_rect(buffer, zx - 5, 217, zx - 3, 219, 7);
            draw_rect(buffer, zx + 3, 217, zx + 5, 219, 7);
        }
    }

    // Diving Skree (hanging green bat-alien)
    if (skree_state == SKREE_CEILING || skree_state == SKREE_DIVING) {
        int sx = skree_x - camera_x;
        if (sx >= -12 && sx < 320) {
            draw_rect(buffer, sx - 5, skree_y - 10, sx + 5, skree_y, 2);  // Green body
            draw_rect(buffer, sx - 3, skree_y - 7,  sx + 3, skree_y - 4, 0); // Dark hollow
            // Wing tips
            draw_pixel(buffer, sx - 7, skree_y - 8, 2);
            draw_pixel(buffer, sx + 7, skree_y - 8, 2);
            // Yellow belly
            draw_rect(buffer, sx - 2, skree_y - 5, sx + 2, skree_y - 2, 3);
            // Spike feet
            draw_pixel(buffer, sx - 3, skree_y + 1, 3);
            draw_pixel(buffer, sx + 3, skree_y + 1, 3);
            draw_pixel(buffer, sx, skree_y + 2, 3);
        }
    }

    // Metroid Boss
    if (boss_alive) {
        draw_metroid_boss(buffer, boss_x - camera_x, boss_y);
    }

    // Energy Tank drop
    if (energy_tank_spawned) {
        int tx = energy_tank_x - camera_x;
        if (tx >= -16 && tx < 320) {
            draw_rect(buffer, tx - 8, energy_tank_y - 6, tx + 8, energy_tank_y + 4, 2);  // Green container
            draw_rect(buffer, tx - 6, energy_tank_y - 4, tx + 6, energy_tank_y + 2, 0);  // Black inner
            draw_rect(buffer, tx - 4, energy_tank_y - 3, tx + 4, energy_tank_y + 1, 6);  // Cyan charge
            draw_string(buffer, "ET", tx - 5, energy_tank_y - 3, 1, 7);
        }
    }

    // --- DRAW BOMB & EXPLOSIONS ---
    if (bomb_timer > 0) {
        int bx = bomb_x - camera_x;
        if (bx >= -10 && bx < 320) {
            uint8_t color = (bomb_timer % 4 < 2) ? 7 : 1; // flicker white/red
            draw_circle(buffer, bx, bomb_y, 3, color);
            draw_circle(buffer, bx, bomb_y, 1, 3); // yellow core
        }
    }

    if (explode_timer > 0) {
        explode_timer--;
        int ex = explode_x - camera_x;
        if (ex >= -30 && ex < 320) {
            int r = 14 - explode_timer / 2;
            if (r > 0) {
                draw_circle(buffer, ex, explode_y, r,     3); // yellow outer burst
                draw_circle(buffer, ex, explode_y, r - 2, 1); // red inner
                if (r > 4) draw_circle(buffer, ex, explode_y, r - 4, 7); // white hot core
            }
        }
    }

    // --- DRAW SAMUS ---
    int sx = samus_x - camera_x;
    draw_varia_samus(buffer, sx, samus_y, frame_counter);

    // --- HUD: Score ---
    draw_string(buffer, "ENERGY", 2, 2, 1, 6);
    draw_score(buffer, samus_score, 48, 2, 1, 3);

    // Boss HP bar
    if (metroid_state == STATE_BOSS_FIGHT && boss_alive) {
        draw_string(buffer, "METROID", 200, 2, 1, 2);
        draw_rect(buffer, 200, 10, 200 + boss_hp * 10, 13, 2);
        draw_rect(buffer, 200 + boss_hp * 10, 10, 250, 13, 0);
        draw_rect(buffer, 200, 10, 250, 13, 7); // white border
        draw_rect(buffer, 201, 11, 249, 12, 0); // reset interior
        draw_rect(buffer, 201, 11, 201 + boss_hp * 9, 12, 2); // green hp
    }

    // Screen Fadeout effect
    if (metroid_state == STATE_FADE_RESET) {
        for (int y = 0; y < 240; y += 2) {
            int block_sz = metroid_timer * 8;
            if (block_sz > 320) block_sz = 320;
            draw_rect(buffer, 0, y, block_sz, y, 0);
            draw_rect(buffer, 320 - block_sz, y + 1, 319, y + 1, 0);
        }
    }
}
