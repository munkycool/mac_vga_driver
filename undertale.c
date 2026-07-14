#include "common.h"

// Game States
static enum {
    UT_MENU_DIALOGUE,
    UT_MENU_ACTION,
    UT_ATTACK_DODGE
} ut_state = UT_MENU_DIALOGUE;

static int soul_x = 160;
static int soul_y = 145;
static int player_hp = 92;
static int menu_select = 0; // 0: FIGHT, 1: ACT, 2: ITEM, 3: MERCY

// Dialog Typewriter
static char ut_dialogue[128];
static int ut_char_count = 0;
static int ut_type_timer = 0;
static int ut_action_timer = 0;

// Battle variables
static int attack_timer = 0;
static int sans_offset_x = 0;
static int sans_target_offset = 0;
static int slash_timer = 0;
static bool show_miss = false;

// Attack Hazards
static int bone_x1 = 220;
static int bone_h1 = 20;
static int bone_x2 = 300;
static int bone_h2 = 30;

static int blaster_timer = 0; // Ticking for blaster charge & fire
static bool blaster_active = false;
static int blaster_y = 145;

void init_undertale() {
    ut_state = UT_MENU_DIALOGUE;
    soul_x = 160;
    soul_y = 145;
    player_hp = 92;
    menu_select = 0;
    
    strcpy(ut_dialogue, "* Sans blocks the way.");
    ut_char_count = 0;
    ut_type_timer = 0;
    ut_action_timer = 0;
    
    attack_timer = 0;
    sans_offset_x = 0;
    sans_target_offset = 0;
    slash_timer = 0;
    show_miss = false;
    
    bone_x1 = 220;
    bone_h1 = 15 + (get_rand() % 15);
    bone_x2 = 280;
    bone_h2 = 15 + (get_rand() % 15);
    blaster_timer = 0;
    blaster_active = false;
}

// Procedural drawing of Sans the Skeleton
static void draw_sans(uint8_t *buffer, int frame_counter) {
    int cx = 160 + sans_offset_x;
    
    // Head (White) — Ultra HD shaded skull
    art_circle_shaded(buffer, cx, 55, 10, 6, 7);
    art_shaded_rect(buffer, cx - 8, 58, cx + 8, 62, 7, 7, 6);
    
    // Eyes (Black eye sockets)
    draw_circle(buffer, cx - 4, 53, 2, 0);
    draw_circle(buffer, cx + 4, 53, 2, 0);
    
    // Flashing left eye (Sans' bad time mode)
    if (ut_state == UT_ATTACK_DODGE) {
        if (frame_counter % 12 < 6) {
            draw_pixel(buffer, cx - 4, 53, 6); // Blue/cyan eye glow
        } else {
            draw_pixel(buffer, cx - 4, 53, 3); // Yellow eye glow
        }
        draw_pixel(buffer, cx + 4, 53, 7);     // White pupil
    } else {
        // Normal pupils
        draw_pixel(buffer, cx - 4, 53, 7);
        draw_pixel(buffer, cx + 4, 53, 7);
    }
    
    // Nose cavity (Black triangle)
    draw_pixel(buffer, cx, 58, 0);
    
    // Smile (Black mouth outline)
    draw_rect(buffer, cx - 5, 61, cx + 5, 62, 0);
    
    // Jacket (Blue body, White fur hood/collar)
    draw_rect(buffer, cx - 14, 66, cx + 14, 90, 4); // Blue jacket body
    draw_rect(buffer, cx - 8, 66, cx + 8, 70, 7);   // White fur collar
    draw_rect(buffer, cx - 2, 71, cx + 2, 90, 7);   // White undershirt slice
    
    // Shorts (Black with white stripes)
    draw_rect(buffer, cx - 10, 91, cx + 10, 99, 0); // Black shorts
    draw_rect(buffer, cx - 8, 91, cx - 7, 99, 7);   // Left white stripe
    draw_rect(buffer, cx + 7, 91, cx + 8, 99, 7);   // Right white stripe
    
    // Legs (White bones)
    draw_rect(buffer, cx - 5, 100, cx - 4, 104, 7);
    draw_rect(buffer, cx + 4, 100, cx + 5, 104, 7);
    
    // Slippers (Pink)
    draw_rect(buffer, cx - 9, 104, cx - 2, 107, 5); // Left slipper
    draw_rect(buffer, cx + 2, 104, cx + 9, 107, 5); // Right slipper

    // Ultra HD jacket sheen + slipper highlights
    art_specular(buffer, cx - 12, 68, 7);
    art_dither_pixel(buffer, cx - 13, 78, 0, 2);
    art_dither_pixel(buffer, cx + 13, 78, 0, 2);
    art_specular(buffer, cx - 6, 104, 7);
    art_specular(buffer, cx + 6, 104, 7);
}

static void draw_soul(uint8_t *buffer, int x, int y) {
    draw_rect(buffer, x - 2, y - 2, x + 2, y - 2, 1);
    draw_rect(buffer, x - 3, y - 1, x + 3, y - 1, 1);
    draw_rect(buffer, x - 3, y, x + 3, y, 1);
    draw_rect(buffer, x - 2, y + 1, x + 2, y + 1, 1);
    draw_rect(buffer, x - 1, y + 2, x + 1, y + 2, 1);
    draw_pixel(buffer, x, y + 3, 1);
    draw_pixel(buffer, x, y - 2, 0); // heart top center black gap

    art_specular(buffer, x - 2, y - 1, 7);
    art_specular(buffer, x + 2, y - 1, 7);
}

static void draw_ut_hud(uint8_t *buffer) {
    // Player Name & LV
    draw_string(buffer, "FRISK   LV 19", 30, 180, 1, 7);
    
    // HP label
    draw_string(buffer, "HP", 125, 180, 1, 7);
    
    // HP Bar (Yellow is current, Red is missing)
    int max_bar_w = 40;
    int current_bar_w = (player_hp * max_bar_w) / 92;
    if (current_bar_w < 0) current_bar_w = 0;
    
    draw_rect(buffer, 142, 179, 142 + current_bar_w - 1, 186, 3); // Yellow bar
    draw_rect(buffer, 142 + current_bar_w, 179, 142 + max_bar_w - 1, 186, 1); // Red bar
    
    // HP Text
    char hp_str[16];
    sprintf(hp_str, "%02d / 92", player_hp);
    draw_string(buffer, hp_str, 190, 180, 1, 7);
}

static void draw_ut_buttons(uint8_t *buffer, int selected) {
    const char *labels[4] = {"FIGHT", "ACT", "ITEM", "MERCY"};
    int bx[4] = {32, 102, 172, 242};
    
    for (int i = 0; i < 4; i++) {
        uint8_t color = (selected == i) ? 3 : 1; // Highlight yellow/orange, otherwise red
        draw_rect(buffer, bx[i], 194, bx[i] + 48, 206, color);
        draw_rect(buffer, bx[i] + 2, 196, bx[i] + 46, 204, 0); // Black fill
        
        // Draw label text
        draw_string(buffer, labels[i], bx[i] + 8, 197, 1, color);
    }
}

void play_undertale(uint8_t *buffer, int frame_counter) {
    // 1. Clear Screen to Black
    clear_screen(buffer);
    
    // 2. Animate Sans slide/dodge
    if (sans_offset_x < sans_target_offset) sans_offset_x += 2;
    else if (sans_offset_x > sans_target_offset) sans_offset_x -= 2;
    
    draw_sans(buffer, frame_counter);
    
    // 3. Game State Machine
    if (ut_state == UT_MENU_DIALOGUE) {
        // Draw Dialogue Box (Wide)
        draw_rect(buffer, 28, 115, 292, 173, 7); // White border
        draw_rect(buffer, 32, 119, 288, 169, 0); // Black inner
        
        // Dialogue Typewriter
        char visible_text[128] = {0};
        strncpy(visible_text, ut_dialogue, ut_char_count);
        draw_string(buffer, visible_text, 44, 126, 1, 7);
        
        if (ut_char_count < strlen(ut_dialogue)) {
            ut_type_timer++;
            if (ut_type_timer >= 2) {
                ut_type_timer = 0;
                ut_char_count++;
            }
        }
        
        draw_ut_hud(buffer);
        draw_ut_buttons(buffer, menu_select);
        
        // Soul heart next to selected button
        int heart_x = 38 + menu_select * 70;
        draw_soul(buffer, heart_x, 200);
        
        // Menu input navigation
        if (global_input.interactive) {
            if (global_input.left) {
                menu_select = (menu_select + 3) % 4;
            }
            if (global_input.right) {
                menu_select = (menu_select + 1) % 4;
            }
            if (global_input.action1) {
                // Confirm action select
                ut_state = UT_MENU_ACTION;
                ut_action_timer = 0;
                
                if (menu_select == 0) { // FIGHT
                    slash_timer = 15;
                    show_miss = false;
                    sans_target_offset = 50; // Sans slides right to dodge!
                } else if (menu_select == 1) { // ACT
                    strcpy(ut_dialogue, "* You told Sans a skeleton joke.\n* He laughed bone-drily.");
                    ut_char_count = 0;
                } else if (menu_select == 2) { // ITEM
                    strcpy(ut_dialogue, "* You ate the Legendary Hero.\n* HP fully restored!");
                    ut_char_count = 0;
                    player_hp = 92;
                } else if (menu_select == 3) { // MERCY
                    strcpy(ut_dialogue, "* Sans spares you.\n* (You can skip game with Q)");
                    ut_char_count = 0;
                }
            }
        } else {
            // Screensaver autoplay timer to progress dialogue
            ut_action_timer++;
            if (ut_action_timer > 100) {
                ut_state = UT_MENU_ACTION;
                ut_action_timer = 0;
                
                // Random choice
                menu_select = get_rand() % 4;
                if (menu_select == 0) {
                    slash_timer = 15;
                    show_miss = false;
                    sans_target_offset = 50;
                } else if (menu_select == 1) {
                    strcpy(ut_dialogue, "* You check Sans.\n* SANS 1 ATK 1 DEF\n* Easiest enemy.");
                    ut_char_count = 0;
                } else if (menu_select == 2) {
                    strcpy(ut_dialogue, "* You ate a Butterscotch Pie.\n* HP fully restored!");
                    ut_char_count = 0;
                    player_hp = 92;
                } else {
                    strcpy(ut_dialogue, "* You SPARE Sans.\n* He is sleeping.");
                    ut_char_count = 0;
                }
            }
        }
        
    } else if (ut_state == UT_MENU_ACTION) {
        // Dialogue box remains
        draw_rect(buffer, 28, 115, 292, 173, 7);
        draw_rect(buffer, 32, 119, 288, 169, 0);
        
        draw_ut_hud(buffer);
        draw_ut_buttons(buffer, menu_select);
        
        if (menu_select == 0) { // FIGHT animations
            if (slash_timer > 0) {
                slash_timer--;
                // Draw white diagonal slash mark across Sans
                int sy = 55 + (15 - slash_timer) * 3;
                int sx = 145 + (15 - slash_timer) * 2;
                draw_rect(buffer, sx - 2, sy - 2, sx + 2, sy + 2, 7); // slash spark
                
                if (slash_timer == 0) {
                    show_miss = true;
                    ut_action_timer = 0;
                }
            }
            
            if (show_miss) {
                // Draw "MISS" yellow text floating above Sans
                draw_string(buffer, "MISS", 145 + sans_offset_x, 30, 2, 3);
                
                ut_action_timer++;
                if (ut_action_timer > 45) {
                    // Transition to ATTACK phase
                    ut_state = UT_ATTACK_DODGE;
                    attack_timer = 0;
                    soul_x = 160;
                    soul_y = 145;
                    sans_target_offset = 0; // Sans returns to center
                    
                    // Setup attack variables
                    bone_x1 = 210;
                    bone_x2 = 280;
                    blaster_timer = 0;
                    blaster_active = false;
                }
            }
        } else {
            // Text box for ACT/ITEM/MERCY
            char visible_text[128] = {0};
            strncpy(visible_text, ut_dialogue, ut_char_count);
            draw_string(buffer, visible_text, 44, 126, 1, 7);
            
            if (ut_char_count < strlen(ut_dialogue)) {
                ut_type_timer++;
                if (ut_type_timer >= 2) {
                    ut_type_timer = 0;
                    ut_char_count++;
                }
            }
            
            ut_action_timer++;
            // Transition to ATTACK mode
            if (ut_action_timer > 100 || (global_input.interactive && global_input.action1 && ut_char_count >= strlen(ut_dialogue))) {
                ut_state = UT_ATTACK_DODGE;
                attack_timer = 0;
                soul_x = 160;
                soul_y = 145;
                sans_target_offset = 0;
                
                // Reset attack hazards
                bone_x1 = 210;
                bone_x2 = 280;
                blaster_timer = 0;
                blaster_active = false;
            }
        }
        
    } else if (ut_state == UT_ATTACK_DODGE) {
        // Bullet box (Small)
        draw_rect(buffer, 108, 115, 212, 175, 7);
        draw_rect(buffer, 112, 119, 208, 171, 0);
        
        draw_ut_hud(buffer);
        
        // Attack Ticker
        attack_timer++;
        
        // --- Attack Hazards Physics ---
        // 1. Bone 1 (From right, vertical on bottom)
        bone_x1 -= 2;
        if (bone_x1 < 100) {
            bone_x1 = 210;
            bone_h1 = 15 + (get_rand() % 20);
        }
        // Draw Bone 1
        int bx1 = bone_x1;
        if (bx1 >= 112 && bx1 <= 208) {
            draw_rect(buffer, bx1 - 2, 171 - bone_h1, bx1 + 2, 171, 7);
            draw_rect(buffer, bx1 - 4, 171 - bone_h1, bx1 + 4, 171 - bone_h1 + 2, 7); // bone top caps
        }
        
        // 2. Bone 2 (Fast sliding, from right)
        bone_x2 -= 3;
        if (bone_x2 < 100) {
            bone_x2 = 240;
            bone_h2 = 12 + (get_rand() % 15);
        }
        int bx2 = bone_x2;
        if (bx2 >= 112 && bx2 <= 208) {
            draw_rect(buffer, bx2 - 2, 171 - bone_h2, bx2 + 2, 171, 7);
            draw_rect(buffer, bx2 - 4, 171 - bone_h2, bx2 + 4, 171 - bone_h2 + 2, 7);
        }
        
        // 3. Gaster Blaster
        blaster_timer++;
        if (blaster_timer > 80 && blaster_timer < 140) {
            blaster_active = true;
            // Draw Gaster Blaster charging skull on the left edge
            int blast_x = 90;
            // Draw Blaster Skull (Cyan/white glowing block)
            draw_circle(buffer, blast_x, blaster_y, 8, 7);
            draw_rect(buffer, blast_x - 4, blaster_y + 4, blast_x + 4, blaster_y + 8, 7);
            draw_rect(buffer, blast_x - 3, blaster_y - 2, blast_x - 1, blaster_y + 1, 0); // black eyes
            draw_rect(buffer, blast_x + 1, blaster_y - 2, blast_x + 3, blaster_y + 1, 0);
            
            // Flashing eyes charging indicator
            if (blaster_timer % 6 < 3) {
                draw_pixel(buffer, blast_x - 2, blaster_y, 6); // blue eye glow
                draw_pixel(buffer, blast_x + 2, blaster_y, 6);
            }
            
            // Firing beam
            if (blaster_timer > 105) {
                // Shoot horizontal cyan laser beam!
                draw_rect(buffer, 112, blaster_y - 6, 208, blaster_y + 6, 6); // cyan core
                draw_rect(buffer, 112, blaster_y - 3, 208, blaster_y + 3, 7); // white inner
            }
        } else {
            blaster_active = false;
            if (blaster_timer >= 140) {
                blaster_timer = 0;
                blaster_y = 125 + (get_rand() % 40); // random laser height next time
            }
        }
        
        // --- SOUL Controls ---
        if (global_input.interactive) {
            if (global_input.left) soul_x -= 2;
            if (global_input.right) soul_x += 2;
            if (global_input.up) soul_y -= 2;
            if (global_input.down) soul_y += 2;
        } else {
            // Autoplay AI: Dodges incoming bones/lasers
            // Target safe coordinates
            int target_sx = 160;
            int target_sy = 145;
            
            // Move away from bone 1
            if (abs(soul_x - bone_x1) < 20) {
                target_sy = 171 - bone_h1 - 10;
            }
            // Move away from bone 2
            if (abs(soul_x - bone_x2) < 20) {
                if (171 - bone_h2 < target_sy) {
                    target_sy = 171 - bone_h2 - 10;
                }
            }
            // Move away from laser
            if (blaster_active && blaster_timer > 105 && abs(soul_y - blaster_y) < 15) {
                if (blaster_y > 145) {
                    target_sy = 122; // stay high
                } else {
                    target_sy = 165; // stay low
                }
            }
            
            if (soul_x < target_sx) soul_x++;
            else if (soul_x > target_sx) soul_x--;
            
            if (soul_y < target_sy) soul_y += 2;
            else if (soul_y > target_sy) soul_y -= 2;
        }
        
        // Bounding limits keep SOUL in bullet box
        if (soul_x < 117) soul_x = 117;
        if (soul_x > 203) soul_x = 203;
        if (soul_y < 124) soul_y = 124;
        if (soul_y > 166) soul_y = 166;
        
        // Draw SOUL heart
        draw_soul(buffer, soul_x, soul_y);
        
        // --- Collision checks ---
        bool hit = false;
        // Bone 1 collision
        if (abs(soul_x - bone_x1) < 6 && soul_y >= 171 - bone_h1) {
            hit = true;
        }
        // Bone 2 collision
        if (abs(soul_x - bone_x2) < 6 && soul_y >= 171 - bone_h2) {
            hit = true;
        }
        // Blaster laser beam collision
        if (blaster_active && blaster_timer > 105 && abs(soul_y - blaster_y) < 8) {
            hit = true;
        }
        
        if (hit) {
            if (frame_counter % 6 == 0) {
                player_hp--;
                if (player_hp < 1) player_hp = 1; // don't die completely, keep game running
            }
            // Flash screen red border
            draw_rect(buffer, 0, 0, 319, 4, 1);
            draw_rect(buffer, 0, 235, 319, 239, 1);
            draw_rect(buffer, 0, 0, 4, 239, 1);
            draw_rect(buffer, 315, 0, 319, 239, 1);
        }
        
        // Transition back to Menu Dialogue
        if (attack_timer >= 150) {
            ut_state = UT_MENU_DIALOGUE;
            ut_action_timer = 0;
            
            // Select random message
            int msg = get_rand() % 4;
            if (msg == 0) strcpy(ut_dialogue, "* You feel your sins crawling\n  on your back.");
            else if (msg == 1) strcpy(ut_dialogue, "* Sans is sparing you.");
            else if (msg == 2) strcpy(ut_dialogue, "* Sans is sweating a bit.");
            else strcpy(ut_dialogue, "* Sans is looking tired.");
            
            ut_char_count = 0;
        }
    }
}
