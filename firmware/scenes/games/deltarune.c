#include "common.h"

// Game States
static enum {
    DR_MENU,
    DR_PLAYER_ATTACK,
    DR_ENEMY_ATTACK
} dr_state = DR_MENU;

// Squad HP
static int kris_hp = 90;
static int susie_hp = 110;
static int ralsei_hp = 70;
static int tp_percent = 40;

// Battle Flow Variables
static int dr_timer = 0;
static int active_char = 0; // 0: Kris, 1: Susie, 2: Ralsei
static int menu_select = 0; // 0: FIGHT, 1: ACT, 2: ITEM, 3: SPARE
static int kris_action = 0;
static int susie_action = 0;
static int ralsei_action = 0;

static int kris_offset_x = 0;
static int susie_offset_x = 0;
static int ralsei_offset_x = 0;

static int slash_x = 0;
static int slash_y = 0;
static int slash_timer = 0;
static int heal_timer = 0;
static int damage_amount = 0;

// SOUL
static int soul_x = 160;
static int soul_y = 145;

// Attack Hazards
static int hazard_x[4];
static int hazard_y[4];
static uint8_t hazard_color[4];

void init_deltarune() {
    dr_state = DR_MENU;
    dr_timer = 0;
    active_char = 0;
    menu_select = 0;
    
    kris_hp = 90;
    susie_hp = 110;
    ralsei_hp = 70;
    tp_percent = 35;
    
    kris_offset_x = 0;
    susie_offset_x = 0;
    ralsei_offset_x = 0;
    
    slash_timer = 0;
    heal_timer = 0;
    damage_amount = 0;
    
    soul_x = 160;
    soul_y = 145;
    
    for (int i = 0; i < 4; i++) {
        hazard_x[i] = 120 + i * 20;
        hazard_y[i] = 80 + (get_rand() % 40);
        hazard_color[i] = (i % 2 == 0) ? 4 : 3; // Pipis (blue) or Money (yellow)
    }
}

// ============================================================
//  DELTARUNE SPRITES (Authentic Dark World Art Matrices)
//  0=Black  1=Red  2=Green  3=Yellow  4=Blue  5=Magenta  6=Cyan  7=White  .=Transparent
// ============================================================

static const char *kris_sprite[21] = {
    ".....00000..........",
    "....0000000.........",
    "...000000000........",
    "...000000000........",
    "...006666600........", // cyan skin / face
    "...066666660........",
    "...000000000........",
    "....0666660.........",
    "....0055500.........", // pink scarf
    "...055777550....7...", // sword tip
    "..05577777550..77...",
    "..05777777750.77....",
    "..0577333775077.....",
    "..055777775577......",
    "...0555555577.......", // sword body
    "....00000033........", // guard
    "....00...000........", // hilt
    "....06...600........",
    "....06...60.........",
    "....07...70.........",
    "....00...00........."
};

static const char *susie_sprite[24] = {
    "......00000000......",
    ".....0555555550.....", // wild magenta hair
    "....055555555550....",
    "....055555555550....",
    "....055555555550....",
    "....055111111550....", // plum skin
    "....051771177150....", // eyes
    "....051111111150....",
    "....051177771150....", // grin
    ".....011111110......",
    "......0000000.......",
    ".....055000550......", // black vest
    "....05550005550.....",
    "....05550005550.....",
    "....05550005550.....",
    "....05553335550.....", // gold stud belt
    ".....055555550......",
    "......0333330.......", // yellow pants
    "......033.330.......",
    "......033.330.......",
    "......033.330.......",
    "......000.000.......", // heavy boots
    "......000.000.......",
    "......000.000......."
};

static const char *ralsei_sprite[27] = {
    ".......020..........", // witch hat green
    "......02220.........",
    "......02220.........",
    ".....0222220........",
    ".....0222220........",
    "....022222220.......",
    "....022222220.......",
    "....077777770.......", // white ribbon
    "...00000000000......", // black brim
    "..0000000000000.....",
    "....000000000.......",
    "...00077007700......", // round glasses
    "..0007770777000.....",
    "..0010070700100.....", // pink cheeks (1)
    "...0000000000.......", // black fur
    "....00000000........",
    "....07777770........", // pink/white scarf
    "...0777777770.......",
    "..022222222220......", // green robes
    ".02222000022220.....", // black print
    "0222200000022220....",
    "0222200000022220....",
    "0222220000222220....",
    "0222222222222220....",
    "0222222222222220....",
    "0022222222222200....",
    ".00000000000000....."
};

static const char *spamton_sprite[28] = {
    "......07770.........", // spiky hair white
    "....077777770.......",
    "...00077777000......",
    "..0777777777770.....", // head outline
    ".077777777777770....",
    ".077555577333370....", // mismatched lenses
    ".075777757377730....",
    ".075777757377730....",
    ".077555577333370....",
    "..0777777777770.....",
    "...07777777770......", // nose pointing out
    "....00000000........",
    "....07777770........", // smile
    "....07070700........",
    "....07777770........",
    ".....000000.........",
    "....00111100........", // red bow tie
    "...0000000000.......", // black suit
    "..000077770000......", // white undershirt
    "..000077770000......",
    "..000077770000......",
    "..000000000000......",
    "...0000000000.......",
    "....000..000........", // skinny legs
    "....000..000........",
    "....000..000........",
    "....000..000........",
    "....070..070........"  // white shoe highlights
};

static void draw_sprite_matrix(uint8_t *buffer, int start_x, int start_y, const char **sprite, int rows, int cols) {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            char color_char = sprite[r][c];
            if (color_char == '.') continue;
            uint8_t color = 0;
            switch (color_char) {
                case '0': color = 0; break;
                case '1': color = 1; break;
                case '2': color = 2; break;
                case '3': color = 3; break;
                case '4': color = 4; break;
                case '5': color = 5; break;
                case '6': color = 6; break;
                case '7': color = 7; break;
                default: continue;
            }
            // Draw upscaled 2x2 blocks
            draw_rect(buffer, start_x + c * 2, start_y + r * 2, start_x + c * 2 + 1, start_y + r * 2 + 1, color);
        }
    }
}

static void draw_kris(uint8_t *buffer, int ox) {
    int px = 58 + ox;
    int py = 60;
    // Render matrix scaled by 2 (offset centers)
    draw_sprite_matrix(buffer, px - 16, py - 20, kris_sprite, 21, 20);
}

static void draw_susie(uint8_t *buffer, int ox) {
    int px = 47 + ox;
    int py = 115;
    
    // Draw axe handle/head overlays on top of the matrix
    draw_rect(buffer, px - 14, py - 18, px - 13, py + 5, 6); // handle
    draw_rect(buffer, px - 20, py - 22, px - 12, py - 13, 5); // head
    draw_rect(buffer, px - 19, py - 21, px - 13, py - 14, 7); // shine
    draw_pixel(buffer, px - 21, py - 20, 6);
    draw_pixel(buffer, px - 21, py - 16, 6);
    draw_rect(buffer, px - 16, py - 13, px - 13, py - 10, 5);

    draw_sprite_matrix(buffer, px - 18, py - 22, susie_sprite, 24, 20);
}

static void draw_ralsei(uint8_t *buffer, int ox) {
    int px = 58 + ox;
    int py = 160;

    // Draw staff overlays
    draw_rect(buffer, px + 11, py - 15, px + 12, py + 10, 6); // shaft
    draw_circle(buffer, px + 12, py - 16, 3, 2); // green orb
    draw_pixel(buffer, px + 11, py - 17, 7); // orb shine

    draw_sprite_matrix(buffer, px - 18, py - 26, ralsei_sprite, 27, 20);
}

static void draw_spamton(uint8_t *buffer) {
    int px = 248;
    int py = 108;

    // Draw puppet strings
    draw_rect(buffer, px - 5, 40, px - 5, py - 20, 7);
    draw_rect(buffer, px + 5, 40, px + 5, py - 20, 7);
    draw_rect(buffer, px - 12, 40, px - 12, py,    7);
    draw_rect(buffer, px + 12, 40, px + 12, py,    7);

    draw_sprite_matrix(buffer, px - 16, py - 24, spamton_sprite, 28, 20);
}

static void draw_soul(uint8_t *buffer, int x, int y) {
    // Classic Undertale/Deltarune SOUL - red heart shape
    // Row 1: two bumps
    draw_pixel(buffer, x - 3, y - 3, 1);
    draw_pixel(buffer, x - 2, y - 4, 1);
    draw_pixel(buffer, x - 1, y - 3, 1);
    draw_pixel(buffer, x + 1, y - 3, 1);
    draw_pixel(buffer, x + 2, y - 4, 1);
    draw_pixel(buffer, x + 3, y - 3, 1);
    // Row 2: wide top
    draw_rect(buffer, x - 3, y - 2, x + 3, y - 2, 1);
    // Row 3: wide middle
    draw_rect(buffer, x - 3, y - 1, x + 3, y - 1, 1);
    draw_rect(buffer, x - 3, y,     x + 3, y,     1);
    // Row 4: narrows
    draw_rect(buffer, x - 2, y + 1, x + 2, y + 1, 1);
    // Row 5: tip
    draw_rect(buffer, x - 1, y + 2, x + 1, y + 2, 1);
    draw_pixel(buffer, x, y + 3, 1);
    // Dark inner for dimension
    draw_pixel(buffer, x - 1, y - 1, 0);
    draw_pixel(buffer, x + 1, y - 1, 0);

    // Ultra HD heart gleam
    art_specular(buffer, x - 2, y - 3, 7);
    art_specular(buffer, x + 2, y - 3, 7);
    art_dither_pixel(buffer, x, y + 1, 1, 2);
}

// Draw Deltarune Dark World HUD
static void draw_dr_hud(uint8_t *buffer) {
    // 1. Vertical TP Bar on left
    draw_rect(buffer, 8, 40, 16, 170, 7);
    draw_rect(buffer, 10, 42, 14, 168, 0); // black inner
    
    int tp_h = (tp_percent * (168 - 42)) / 100;
    draw_rect(buffer, 10, 168 - tp_h, 14, 168, 3); // Orange TP fill
    
    char tp_str[8];
    sprintf(tp_str, "%d%%", tp_percent);
    draw_string(buffer, tp_str, 5, 26, 1, 3);
    draw_string(buffer, "TP", 8, 174, 1, 3);
    
    // 2. Squad Name & Health Bars at bottom
    // Kris HUD
    draw_string(buffer, "KRIS", 30, 192, 1, 6);
    draw_rect(buffer, 30, 201, 80, 206, 0);
    int kw = (kris_hp * 50) / 90;
    if (kw > 0) draw_rect(buffer, 30, 201, 30 + kw, 206, 6); // Cyan health
    char khp[12]; sprintf(khp, "%d/90", kris_hp);
    draw_string(buffer, khp, 30, 209, 1, 7);
    
    // Susie HUD
    draw_string(buffer, "SUSIE", 120, 192, 1, 5);
    draw_rect(buffer, 120, 201, 170, 206, 0);
    int sw = (susie_hp * 50) / 110;
    if (sw > 0) draw_rect(buffer, 120, 201, 120 + sw, 206, 5); // Purple health
    char shp[12]; sprintf(shp, "%d/110", susie_hp);
    draw_string(buffer, shp, 120, 209, 1, 7);
    
    // Ralsei HUD
    draw_string(buffer, "RALSEI", 210, 192, 1, 2);
    draw_rect(buffer, 210, 201, 260, 206, 0);
    int rw = (ralsei_hp * 50) / 70;
    if (rw > 0) draw_rect(buffer, 210, 201, 210 + rw, 206, 2); // Green health
    char rhp[12]; sprintf(rhp, "%d/70", ralsei_hp);
    draw_string(buffer, rhp, 210, 209, 1, 7);
}

static void draw_dr_buttons(uint8_t *buffer, int selected) {
    const char *labels[4] = {"FIGHT", "ACT", "ITEM", "SPARE"};
    int bx[4] = {32, 92, 152, 212};
    
    for (int i = 0; i < 4; i++) {
        uint8_t color = (selected == i) ? 3 : 7;
        draw_rect(buffer, bx[i], 175, bx[i] + 48, 186, color);
        draw_rect(buffer, bx[i] + 1, 176, bx[i] + 47, 185, 0); // Black fill
        
        draw_string(buffer, labels[i], bx[i] + 6, 178, 1, color);
    }
}

void play_deltarune(uint8_t *buffer, int frame_counter) {
    // Clear screen
    clear_screen(buffer);
    
    // Draw battlefield ground grid
    for (int x = 20; x < 320; x += 40) {
        for (int y = 50; y < 190; y += 2) {
            draw_pixel(buffer, x + (y - 50)/6, y, 5); // perspective blue/purple dots
        }
    }
    
    // Draw Squad
    draw_kris(buffer, kris_offset_x);
    draw_susie(buffer, susie_offset_x);
    draw_ralsei(buffer, ralsei_offset_x);
    
    // Draw Boss
    draw_spamton(buffer);
    
    draw_dr_hud(buffer);
    
    if (dr_state == DR_MENU) {
        // Draw Battle Board (Dialog Box)
        draw_rect(buffer, 28, 115, 292, 170, 7);
        draw_rect(buffer, 32, 119, 288, 166, 0);
        
        draw_dr_buttons(buffer, menu_select);
        
        // Show whose turn it is
        if (active_char == 0) {
            draw_string(buffer, "* Kris: Choose action.", 40, 126, 1, 6);
            draw_soul(buffer, 38, 180);
        } else if (active_char == 1) {
            draw_string(buffer, "* Susie: Choose action.", 40, 126, 1, 5);
            draw_soul(buffer, 98, 180);
        } else {
            draw_string(buffer, "* Ralsei: Choose action.", 40, 126, 1, 2);
            draw_soul(buffer, 158, 180);
        }
        
        // Menu navigation
        if (global_input.interactive) {
            if (global_input.left) {
                menu_select = (menu_select + 3) % 4;
            }
            if (global_input.right) {
                menu_select = (menu_select + 1) % 4;
            }
            if (global_input.action1) {
                // Confirm action
                if (active_char == 0) {
                    kris_action = menu_select;
                    active_char = 1;
                    menu_select = 0;
                } else if (active_char == 1) {
                    susie_action = menu_select;
                    active_char = 2;
                    menu_select = 0;
                } else {
                    ralsei_action = menu_select;
                    // Transition to attacks executing!
                    dr_state = DR_PLAYER_ATTACK;
                    dr_timer = 0;
                }
            }
        } else {
            // Autoplay screensaver turn progression
            dr_timer++;
            if (dr_timer > 60) {
                dr_timer = 0;
                if (active_char == 0) {
                    kris_action = get_rand() % 4;
                    active_char = 1;
                } else if (active_char == 1) {
                    susie_action = get_rand() % 4;
                    active_char = 2;
                } else {
                    ralsei_action = get_rand() % 4;
                    dr_state = DR_PLAYER_ATTACK;
                    dr_timer = 0;
                }
            }
        }
        
    } else if (dr_state == DR_PLAYER_ATTACK) {
        dr_timer++;
        
        // Stage 1: Kris Attacks
        if (dr_timer < 30) {
            if (dr_timer == 1) kris_offset_x = 40; // Dash forward
            if (dr_timer == 8) {
                kris_offset_x = 0;
                slash_timer = 10;
                slash_x = 240;
                slash_y = 110;
                damage_amount = (kris_action == 0) ? 140 : 0;
            }
        }
        // Stage 2: Susie Attacks
        else if (dr_timer < 60) {
            if (dr_timer == 31) susie_offset_x = 50; // Dash forward
            if (dr_timer == 38) {
                susie_offset_x = 0;
                slash_timer = 10;
                slash_x = 245;
                slash_y = 120;
                damage_amount = (susie_action == 0) ? 190 : 0;
            }
        }
        // Stage 3: Ralsei casts Heal Prayer
        else if (dr_timer < 90) {
            if (dr_timer == 61 && ralsei_action == 2) {
                heal_timer = 20;
                kris_hp = (kris_hp + 30 > 90) ? 90 : kris_hp + 30;
                susie_hp = (susie_hp + 40 > 110) ? 110 : susie_hp + 40;
                ralsei_hp = (ralsei_hp + 20 > 70) ? 70 : ralsei_hp + 20;
            }
        } else {
            // Transition to Enemy Attack dodge phase
            dr_state = DR_ENEMY_ATTACK;
            dr_timer = 0;
            soul_x = 160;
            soul_y = 145;
            
            // Setup hazards
            for (int i = 0; i < 4; i++) {
                hazard_x[i] = 120 + i * 24;
                hazard_y[i] = 119 - (get_rand() % 40);
            }
        }
        
        // Draw slash animations
        if (slash_timer > 0) {
            slash_timer--;
            // Draw a yellow sword slash across Spamton
            draw_rect(buffer, slash_x - 10, slash_y - 2 + (10 - slash_timer), slash_x + 10, slash_y + 2 + (10 - slash_timer), 3);
            if (damage_amount > 0) {
                char dmg[8]; sprintf(dmg, "%d", damage_amount);
                draw_string(buffer, dmg, slash_x - 10, slash_y - 25, 2, 3); // Yellow dmg text
            } else {
                draw_string(buffer, "SPARE", slash_x - 12, slash_y - 25, 1, 2); // Green spared text
            }
        }
        
        // Draw heal animations (green sparkles rising)
        if (heal_timer > 0) {
            heal_timer--;
            for (int i = 0; i < 3; i++) {
                int sx = 40 + i * 12;
                int sy = 90 + i * 30 - (20 - heal_timer);
                draw_circle(buffer, sx, sy, 3, 2); // Green sparkle
            }
        }
        
    } else if (dr_state == DR_ENEMY_ATTACK) {
        // Bullet box (Cyan outline)
        draw_rect(buffer, 108, 115, 212, 175, 6);
        draw_rect(buffer, 112, 119, 208, 171, 0);
        
        dr_timer++;
        
        // --- Hazards Physics ---
        for (int i = 0; i < 4; i++) {
            hazard_y[i] += 2; // fall down
            if (hazard_y[i] > 168) {
                hazard_y[i] = 119;
                hazard_x[i] = 116 + (get_rand() % 80);
                hazard_color[i] = (get_rand() % 2 == 0) ? 4 : 3;
            }
            
            // Draw hazard (Pipis = Blue circle, Money = Yellow $)
            if (hazard_color[i] == 4) {
                draw_circle(buffer, hazard_x[i], hazard_y[i], 3, 4); // blue pipis
            } else {
                draw_string(buffer, "$", hazard_x[i] - 2, hazard_y[i] - 3, 1, 3); // yellow dollar
            }
        }
        
        // --- SOUL Controls ---
        if (global_input.interactive) {
            if (global_input.left) soul_x -= 2;
            if (global_input.right) soul_x += 2;
            if (global_input.up) soul_y -= 2;
            if (global_input.down) soul_y += 2;
        } else {
            // Autoplay AI: Dodges falling items
            int target_sx = 160;
            int target_sy = 145;
            
            for (int i = 0; i < 4; i++) {
                if (abs(soul_x - hazard_x[i]) < 16 && abs(soul_y - hazard_y[i]) < 16) {
                    if (soul_x < hazard_x[i]) target_sx = soul_x - 12;
                    else target_sx = soul_x + 12;
                }
            }
            
            if (soul_x < target_sx) soul_x++;
            else if (soul_x > target_sx) soul_x--;
            
            if (soul_y < target_sy) soul_y++;
            else if (soul_y > target_sy) soul_y--;
        }
        
        // Clamp inside box limits
        if (soul_x < 117) soul_x = 117;
        if (soul_x > 203) soul_x = 203;
        if (soul_y < 124) soul_y = 124;
        if (soul_y > 166) soul_y = 166;
        
        // Draw SOUL
        draw_soul(buffer, soul_x, soul_y);
        
        // --- Collision checks ---
        bool hit = false;
        for (int i = 0; i < 4; i++) {
            if (abs(soul_x - hazard_x[i]) < 6 && abs(soul_y - hazard_y[i]) < 6) {
                hit = true;
            }
        }
        
        if (hit && (frame_counter % 8 == 0)) {
            // Deal damage to random squad member
            int target_victim = get_rand() % 3;
            if (target_victim == 0 && kris_hp > 5) kris_hp -= 4;
            else if (target_victim == 1 && susie_hp > 5) susie_hp -= 6;
            else if (ralsei_hp > 5) ralsei_hp -= 3;
            
            // Flashing box outline red
            draw_rect(buffer, 108, 115, 212, 175, 1);
        }
        
        // End of enemy attack turn
        if (dr_timer >= 150) {
            dr_state = DR_MENU;
            dr_timer = 0;
            active_char = 0;
            menu_select = 0;
            
            // Gain TP% for surviving turn
            tp_percent += 15;
            if (tp_percent > 100) tp_percent = 100;
        }
    }
}
