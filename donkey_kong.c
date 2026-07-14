#include "common.h"

static int dk_mario_x = 20, dk_mario_y = 210;
static int dk_mario_jump_y = 0;
static int dk_mario_jump_timer = 0;
static int dk_mario_state = 0; 
static int dk_barrel_x = 165, dk_barrel_y = 82;
static int dk_barrel_state = 0; 
static int dk_mario_score = 0;
static int dk_frame_counter = 0;

// Sprite Rendering helper
static void draw_sprite(uint8_t *buffer, int x_center, int y_bottom, const char *sprite[], int width, int height) {
    int start_x = x_center - width / 2;
    int start_y = y_bottom - height;
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            char color_char = sprite[r][c];
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

static void draw_sprite_flipped(uint8_t *buffer, int x_center, int y_bottom, const char *sprite[], int width, int height) {
    int start_x = x_center - width / 2;
    int start_y = y_bottom - height;
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            char color_char = sprite[r][width - 1 - c];
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

// Girders & Ladders (same as original, but with color customizable parameter)
void draw_steel_girder(uint8_t *buffer, int x1, int y, int x2, uint8_t color) {
    draw_rect(buffer, x1, y, x2, y + 1, color);
    draw_rect(buffer, x1, y + 6, x2, y + 7, color);
    for (int x = x1; x < x2 - 6; x += 8) {
        for (int offset = 0; x + offset < x2 && offset < 6; offset++) {
            draw_pixel(buffer, x + offset, y + 1 + offset, color);
            draw_pixel(buffer, x + offset, y + 6 - offset, color);
        }
    }
}

void draw_steel_ladder(uint8_t *buffer, int x, int y1, int y2, uint8_t color) {
    draw_rect(buffer, x, y1, x + 1, y2, color);
    draw_rect(buffer, x + 8, y1, x + 9, y2, color);
    for (int y = y1; y < y2; y += 4) {
        draw_rect(buffer, x + 2, y, x + 7, y, color);
    }
}

// Sprites
static const char *barrel_0[16] = {
    "....YYYYYYYY....",
    "..YYKYYYYYYKYY..",
    ".YRRKRRRRRRKRRY.",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YRRRKRRRRRRKRRRY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YRRRKRRRRRRKRRRY",
    ".YYYKYYYYYYKYYY.",
    "..YYKYYYYYYKYY..",
    "....YYYYYYYY...."
};

static const char *barrel_1[16] = {
    "....YYYYYYYY....",
    "..YYKYYYYYYKYY..",
    ".YYYKYYYYYYKYYY.",
    "YRRRKRRRRRRKRRRY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YRRRKRRRRRRKRRRY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YRRRKRRRRRRKRRRY",
    ".YYYKYYYYYYKYYY.",
    "..YYKYYYYYYKYY..",
    "....YYYYYYYY...."
};

static const char *barrel_2[16] = {
    "....YYYYYYYY....",
    "..YYKYYYYYYKYY..",
    ".YYYKYYYYYYKYYY.",
    "YYYYKYYYYYYKYYYY",
    "YRRRKRRRRRRKRRRY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YRRRKRRRRRRKRRRY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    ".YRRKRRRRRRKRRY.",
    "..YYKYYYYYYKYY..",
    "....YYYYYYYY...."
};

static const char *barrel_3[16] = {
    "....YYYYYYYY....",
    "..YYKYYYYYYKYY..",
    ".YYYKYYYYYYKYYY.",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YRRRKRRRRRRKRRRY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    "YRRRKRRRRRRKRRRY",
    "YYYYKYYYYYYKYYYY",
    "YYYYKYYYYYYKYYYY",
    ".YYYKYYYYYYKYYY.",
    "..YRKRRRRRRKRY..",
    "....YYYYYYYY...."
};

static const char *barrel_front[16] = {
    "....KKKKKKKK....",
    "..KKYYYYYYYYKK..",
    ".KYYYYYYYYYYYYK.",
    "KYYYYKKKKKKYYYYK",
    "KYYYKYYYYYYKYYYK",
    "KYYKYYYYYYYYKYYK",
    "KYYKYYYYYYYYKYYK",
    "KYYKYYYYYYYYKYYK",
    "KYYKYYYYYYYYKYYK",
    "KYYKYYYYYYYYKYYK",
    "KYYKYYYYYYYYKYYK",
    "KYYYKYYYYYYKYYYK",
    "KYYYYKKKKKKYYYYK",
    ".KYYYYYYYYYYYYK.",
    "..KKYYYYYYYYKK..",
    "....KKKKKKKK...."
};

static const char *barrel_vertical[16] = {
    "..KKKKKKKK..",
    ".KYYYYYYYYK.",
    "KYYYYYYYYYYK",
    "KKKKKKKKKKKK",
    "KYYYYYYYYYYK",
    "KYYYYYYYYYYK",
    "KYYYYYYYYYYK",
    "KKKKKKKKKKKK",
    "KYYYYYYYYYYK",
    "KYYYYYYYYYYK",
    "KYYYYYYYYYYK",
    "KKKKKKKKKKKK",
    "KYYYYYYYYYYK",
    "KYYYYYYYYYYK",
    ".KYYYYYYYYK.",
    "..KKKKKKKK.."
};

static const char *dk_chest_beat_1[20] = {
    "......RRRRRRRR..........",
    ".....RRYYYYYYRR.........",
    "....RRYWKYWKYRRY........",
    "....RRYKKYKKYRRY........",
    "....RRYYYYYYYYRR........",
    ".....RRYYYYYYRR.........",
    "......RRRRRRRR..........",
    "....RRYYRRRRRRYY........",
    "...RRYYRRRRRRYYRR.......",
    "..RRYYRRRRRRRRYYRR......",
    "..RRRRRYYYYYYRRRRR......",
    ".RRYYRRRYYYYRRRYYRR.....",
    ".RRYYRRRRRRRRRRYYRR.....",
    "..RRRRRRRRRRRRRRRR......",
    "...RRRRRRRRRRRRRR.......",
    "....RRRRRRRRRRRR........",
    ".....RRR....RRR.........",
    ".....RRR....RRR.........",
    ".....YYY....YYY.........",
    "....YYYY....YYYY........"
};

static const char *dk_chest_beat_2[20] = {
    "......RRRRRRRR..........",
    ".....RRYYYYYYRR.........",
    "....RRYWKYWKYRRY........",
    "....RRYKKYKKYRRY........",
    "....RRYYYYYYYYRR........",
    ".....RRYYYYYYRR.........",
    "......RRRRRRRR..........",
    "...RRYY.RRRRRR.YYR......",
    "..RRYY..RRRRRR..YYR.....",
    "..RRY...RRRRRR...YR.....",
    "..RRRRRYYYYYYRRRRR......",
    ".RRYYRRRYYYYRRRYYRR.....",
    ".RRYYRRRRRRRRRRYYRR.....",
    "..RRRRRRRRRRRRRRRR......",
    "...RRRRRRRRRRRRRR.......",
    "....RRRRRRRRRRRR........",
    ".....RRR....RRR.........",
    ".....RRR....RRR.........",
    ".....YYY....YYY.........",
    "....YYYY....YYYY........"
};

static const char *dk_throw[20] = {
    "......RRRRRRRR..........",
    ".....RRYYYYYYRR.........",
    "....RRYWKYWKYRRY........",
    "....RRYKKYKKYRRY........",
    "....RRYYYYYYYYRR........",
    ".....RRYYYYYYRR.........",
    "......RRRRRRRR..........",
    "....RRYYRRRRRR..........",
    "...RRYYRRRRRRRRRRYY.....",
    "..RRYYRRRRRRRRRRRRYY....",
    "..RRRRRYYYYYYRRRRYYR....",
    ".RRYYRRRYYYYRRRRYYR.....",
    ".RRYYRRRRRRRRRRYYR......",
    "..RRRRRRRRRRRRRR........",
    "...RRRRRRRRRRRR.........",
    "....RRRRRRRRRR..........",
    ".....RRR....RRR.........",
    ".....RRR....RRR.........",
    ".....YYY....YYY.........",
    "....YYYY....YYYY........"
};

static const char *mario_walk_0[13] = {
    "....RRRRR...",
    "....RRRRRRR.",
    "....KKKYYKY.",
    "....KYKYYYY.",
    "....KYKKYRR.",
    "....KKYYYY..",
    ".....YYYY...",
    "...BBRBRB...",
    "..BBBRRRB...",
    ".BBBBRRBB...",
    "....RRRRR...",
    "...RRR.RR...",
    "..KKK...KKK."
};

static const char *mario_walk_1[13] = {
    "....RRRRR...",
    "....RRRRRRR.",
    "....KKKYYKY.",
    "....KYKYYYY.",
    "....KYKKYRR.",
    "....KKYYYY..",
    ".....YYYY...",
    "...BBRBRB...",
    "..BBBRRRB...",
    ".BBBBRRBB...",
    "....RRRRR...",
    "....RR.RRR..",
    "....KK..KKK."
};

static const char *mario_jump_sprite[13] = {
    "....RRRRR...",
    "....RRRRRRR.",
    "....KKKYYKY.",
    "....KYKYYYY.",
    "....KYKKYRR.",
    "....KKYYYY..",
    "....BYYYYB..",
    "...BBRBRBB..",
    "..BBBRRRBBB.",
    "....RRRRR...",
    "....RR.RR...",
    "...RR...RR..",
    "..KK.....KK."
};

static const char *mario_climb_0_sprite[13] = {
    "....RRRRR...",
    "...RRRRRRR..",
    "..KKKYYYKKK.",
    "..KKKKKKKKK.",
    "...RRRRRRR..",
    "...BBRRRBB..",
    "..BBBRRRBBB.",
    "..BBRRRRRBB.",
    "...RRRRRRR..",
    "...RRR.RRR..",
    "...RR...RR..",
    "..KK.....KK.",
    "..KK.....KK."
};

static const char *mario_climb_1_sprite[13] = {
    "....RRRRR...",
    "...RRRRRRR..",
    "..KKKYYYKKK.",
    "..KKKKKKKKK.",
    "...RRRRRRR..",
    "..BBBRRRB...",
    "..BBRRRBBB..",
    "..BBBRRRRBB.",
    "...RRRRRRR..",
    "...RRRR.RR..",
    "....RR..RR..",
    "....KK..KK..",
    "....KK..KKK."
};

static const char *pauline_wave_0[16] = {
    ".....YYY....",
    "....YYYYY...",
    "....WYWYW...",
    "....YYYYY...",
    ".....WWW....",
    "....MMMMM...",
    "...MMMMMMM..",
    "..M.MMMMM.M.",
    "..M.MMMMM.M.",
    "....MMMMM...",
    "....MMMMM...",
    "....MMMMM...",
    "....MMMMM...",
    "....MMMMM...",
    "....Y...Y...",
    "....K...K..."
};

static const char *pauline_wave_1[16] = {
    ".....YYY....",
    "....YYYYY...",
    "....WYWYW...",
    "....YYYYY...",
    ".....WWW....",
    "....MMMMM...",
    "...MMMMMMM..",
    "..M.MMMMM.M.",
    "..M.MMMMM...",
    "....MMMMM...",
    "....MMMMM...",
    "....MMMMM...",
    "....MMMMM...",
    "....MMMMM...",
    "....Y...Y...",
    "....K...K..."
};

static const char *heart_sprite[5] = {
    ".R.R.",
    "RRRRR",
    "RRRRR",
    ".RRR.",
    "..R.."
};

static void draw_mario_animated(uint8_t *buffer, int x, int y_bottom) {
    const char **mario_sprite;
    bool is_climbing = (dk_mario_state == 1 || dk_mario_state == 3);
    bool is_jumping = (dk_mario_jump_y != 0);

    if (is_jumping) {
        mario_sprite = mario_jump_sprite;
        if (dk_mario_state == 2) {
            draw_sprite_flipped(buffer, x, y_bottom, mario_sprite, 12, 13);
        } else {
            draw_sprite(buffer, x, y_bottom, mario_sprite, 12, 13);
        }
    } else if (is_climbing) {
        mario_sprite = ((y_bottom / 4) % 2 == 0) ? mario_climb_0_sprite : mario_climb_1_sprite;
        draw_sprite(buffer, x, y_bottom, mario_sprite, 12, 13);
    } else {
        mario_sprite = ((x / 4) % 2 == 0) ? mario_walk_0 : mario_walk_1;
        if (dk_mario_state == 2) {
            draw_sprite_flipped(buffer, x, y_bottom, mario_sprite, 12, 13);
        } else {
            draw_sprite(buffer, x, y_bottom, mario_sprite, 12, 13);
        }
    }
}

void init_donkey_kong() {
    dk_mario_x = 20; dk_mario_y = 210;
    dk_mario_jump_y = 0; dk_mario_jump_timer = 0;
    dk_mario_state = 0;
    dk_barrel_x = 165; dk_barrel_y = 82;
    dk_barrel_state = 0;
    dk_mario_score = 0;
    dk_frame_counter = 0;
}

void play_donkey_kong(uint8_t *buffer) {
    clear_screen(buffer);
    dk_frame_counter++;

    // UI elements
    draw_score(buffer, dk_mario_score, 10, 10, 2, 7);
    draw_string(buffer, "DONKEY KONG", 120, 10, 2, 1);

    // Lattice Girders (color 5: Magenta)
    draw_steel_girder(buffer, 10, 218, 309, 5); 
    draw_steel_girder(buffer, 10, 158, 250, 5); 
    draw_steel_girder(buffer, 70, 98, 309, 5);  
    draw_steel_girder(buffer, 100, 55, 140, 5); // Pauline's platform

    // Ladders (color 6: Cyan)
    draw_steel_ladder(buffer, 230, 158, 218, 6); 
    draw_steel_ladder(buffer, 80, 98, 158, 6);   
    draw_steel_ladder(buffer, 120, 55, 98, 6);  // Pauline's ladder

    // Draw Pauline waving at the top
    const char **pauline_sprite = (dk_frame_counter % 32 < 16) ? pauline_wave_0 : pauline_wave_1;
    draw_sprite(buffer, 110, 55, pauline_sprite, 12, 16);

    // Floating heart near Pauline
    if (dk_frame_counter % 32 < 16) {
        draw_sprite(buffer, 128, 35, heart_sprite, 5, 5);
    }

    // Static pile of barrels next to DK
    draw_sprite(buffer, 138, 98, barrel_vertical, 12, 16);
    draw_sprite(buffer, 148, 98, barrel_vertical, 12, 16);
    draw_sprite(buffer, 143, 82, barrel_vertical, 12, 16);

    // Draw Donkey Kong (chest beating vs throwing)
    const char **dk_sprite;
    if (dk_barrel_state == 0 && dk_barrel_x < 185) {
        dk_sprite = dk_throw;
    } else {
        dk_sprite = ((dk_frame_counter % 24) < 12) ? dk_chest_beat_1 : dk_chest_beat_2;
    }
    draw_sprite(buffer, 165, 98, dk_sprite, 24, 20);

    // Barrel movement logic (same as original, but resetting to 165)
    if (dk_barrel_state == 0) {
        dk_barrel_x += 2;
        if (dk_barrel_x >= 234) { dk_barrel_state = 1; }
    } else if (dk_barrel_state == 1) {
        dk_barrel_y += 2;
        if (dk_barrel_y >= 142) { dk_barrel_state = 2; dk_barrel_y = 142; }
    } else if (dk_barrel_state == 2) {
        dk_barrel_x -= 2;
        if (dk_barrel_x <= 84) { dk_barrel_state = 3; }
    } else if (dk_barrel_state == 3) {
        dk_barrel_y += 2;
        if (dk_barrel_y >= 202) { dk_barrel_state = 4; dk_barrel_y = 202; }
    } else if (dk_barrel_state == 4) {
        dk_barrel_x += 2;
        if (dk_barrel_x > 320) {
            dk_barrel_x = 165; dk_barrel_y = 82; dk_barrel_state = 0;
        }
    }

    // Draw active barrel (rotating for rolling, front for ladder)
    const char **barrel_sprite;
    if (dk_barrel_state == 1 || dk_barrel_state == 3) {
        barrel_sprite = barrel_front;
    } else {
        int frame = (dk_barrel_x / 4) % 4;
        if (frame == 0) barrel_sprite = barrel_0;
        else if (frame == 1) barrel_sprite = barrel_1;
        else if (frame == 2) barrel_sprite = barrel_2;
        else barrel_sprite = barrel_3;
    }
    draw_sprite(buffer, dk_barrel_x, dk_barrel_y + 16, barrel_sprite, 16, 16);

    // Mario movement physics (same as original)
    if (dk_mario_state == 0) {
        dk_mario_x++;
        if (dk_mario_x >= 234) { dk_mario_state = 1; }
    } else if (dk_mario_state == 1) {
        dk_mario_y--;
        if (dk_mario_y <= 150) { dk_mario_state = 2; dk_mario_y = 150; }
    } else if (dk_mario_state == 2) {
        dk_mario_x--;
        if (dk_mario_x <= 84) { dk_mario_state = 3; }
    } else if (dk_mario_state == 3) {
        dk_mario_y--;
        if (dk_mario_y <= 90) { dk_mario_state = 4; dk_mario_y = 90; }
    } else if (dk_mario_state == 4) {
        dk_mario_x++;
        if (dk_mario_x >= 120) {
            dk_mario_score += 1000;
            dk_mario_x = 20; dk_mario_y = 210; dk_mario_state = 0;
        }
    }

    // Mario jump physics (same as original)
    if (dk_mario_jump_timer > 0) {
        dk_mario_jump_timer--;
        int jump_h = dk_mario_jump_timer - 10;
        dk_mario_jump_y = (jump_h*jump_h) / 2 - 50; 
    } else {
        dk_mario_jump_y = 0;
        if (abs(dk_mario_x - dk_barrel_x) < 32 && abs(dk_mario_y - dk_barrel_y) < 16 && dk_mario_state != 1 && dk_mario_state != 3) {
            dk_mario_jump_timer = 20;
            dk_mario_score += 100;
        }
    }

    // Draw Mario
    draw_mario_animated(buffer, dk_mario_x, dk_mario_y + dk_mario_jump_y + 4); 

    // Mario collision with barrel (same as original)
    if (dk_mario_jump_y == 0 && abs(dk_mario_x - dk_barrel_x) < 8 && abs(dk_mario_y - dk_barrel_y) < 8) {
        dk_mario_x = 20; dk_mario_y = 210; dk_mario_state = 0;
        dk_mario_score = (dk_mario_score > 200) ? dk_mario_score - 200 : 0;
    }
}
