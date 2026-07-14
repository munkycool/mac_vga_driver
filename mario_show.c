#include "common.h"

// --- Scrolling Lyrics ---
static const char *lyrics[] = {
    "DO THE MARIO!",
    "SWING YOUR ARMS FROM SIDE TO SIDE!",
    "COME ON, IT'S TIME TO GO!",
    "DO THE MARIO!",
    "TAKE ONE STEP, AND THEN AGAIN!",
    "LET'S DO THE MARIO, ALL TOGETHER NOW!",
    "YOU GOT IT!",
    "IT'S THE MARIO!"
};
#define NUM_LYRICS 8

static int lyric_scroll_x = 320;
static int current_lyric_idx = 0;

// --- Floating Music Notes ---
struct MusicNote {
    int x, y;
    int speed_y;
    int phase;
    int color;
    bool is_double;
};
#define MAX_NOTES 5
static struct MusicNote notes[MAX_NOTES];

void init_mario_show() {
    lyric_scroll_x = 320;
    current_lyric_idx = 0;
    for (int i = 0; i < MAX_NOTES; i++) {
        notes[i].x = 60 + (get_rand() % 200);
        notes[i].y = 100 + (get_rand() % 80);
        notes[i].speed_y = 1 + (get_rand() % 2);
        notes[i].phase = get_rand() % 360;
        notes[i].color = 2 + (get_rand() % 5); // 2 (Green) to 7 (White)
        notes[i].is_double = (get_rand() % 2 == 0);
    }
}

// Draw a pixel-art music note (♪ or ♫)
static void draw_music_note(uint8_t *buffer, int x, int y, int color, bool is_double) {
    if (x < 0 || x >= 320 || y < 10 || y >= 230) return;
    if (!is_double) {
        // Single note: ♪
        draw_rect(buffer, x, y, x + 3, y + 2, color); // Note head
        draw_rect(buffer, x + 3, y - 6, x + 3, y, color); // Stem
        draw_pixel(buffer, x + 4, y - 6, color); // Flag
        draw_pixel(buffer, x + 5, y - 5, color);
    } else {
        // Double note: ♫
        draw_rect(buffer, x, y, x + 3, y + 2, color); // Head 1
        draw_rect(buffer, x + 8, y - 2, x + 11, y, color); // Head 2 (slightly offset)
        draw_rect(buffer, x + 3, y - 8, x + 3, y, color); // Stem 1
        draw_rect(buffer, x + 11, y - 10, x + 11, y - 2, color); // Stem 2
        draw_rect(buffer, x + 3, y - 8, x + 11, y - 8, color); // Joining beam
    }
}

// Draw King Koopa peeking from curtains
static void draw_peeking_koopa(uint8_t *buffer, int kx, int ky, int pose) {
    int scale = 2;
    // Crown
    draw_rect(buffer, kx + 5*scale, ky, kx + 9*scale - 1, ky + 1*scale - 1, 3); // Gold crown
    draw_pixel(buffer, kx + 7*scale, ky - 1*scale, 1); // Red crown gem
    
    // Head
    draw_rect(buffer, kx + 4*scale, ky + 1*scale, kx + 10*scale - 1, ky + 5*scale - 1, 2); // Green head
    draw_rect(buffer, kx + 5*scale, ky + 3*scale, kx + 6*scale - 1, ky + 4*scale - 1, 7); // White eye
    draw_pixel(buffer, kx + 6*scale, ky + 3*scale, 0); // Black pupil
    
    // Snout
    draw_rect(buffer, kx + 3*scale, ky + 4*scale, kx + 7*scale - 1, ky + 6*scale - 1, 3); // Yellow snout
    draw_pixel(buffer, kx + 2*scale, ky + 5*scale, 1); // Red mouth line
    
    // Spiked shell on back
    draw_rect(buffer, kx + 9*scale, ky + 5*scale, kx + 12*scale - 1, ky + 11*scale - 1, 3); // Orange shell
    draw_pixel(buffer, kx + 11*scale, ky + 6*scale, 7); // White spike
    draw_pixel(buffer, kx + 10*scale, ky + 9*scale, 7); // White spike
    
    // Body (Green)
    draw_rect(buffer, kx + 5*scale, ky + 6*scale, kx + 9*scale - 1, ky + 12*scale - 1, 2);
    
    // Yellow belly
    draw_rect(buffer, kx + 4*scale, ky + 8*scale, kx + 7*scale - 1, ky + 12*scale - 1, 3);
    
    // Arm waving
    if (pose == 0) {
        // Arm pointing up/forward
        draw_rect(buffer, kx + 1*scale, ky + 6*scale, kx + 4*scale - 1, ky + 7*scale - 1, 2);
        draw_rect(buffer, kx, ky + 5*scale, kx + 1*scale, ky + 6*scale, 7); // Claws
    } else {
        // Arm pointing down
        draw_rect(buffer, kx + 1*scale, ky + 8*scale, kx + 4*scale - 1, ky + 9*scale - 1, 2);
        draw_rect(buffer, kx, ky + 9*scale, kx + 1*scale, ky + 10*scale, 7); // Claws
    }
}

// Draw a detailed dancing character (Mario or Luigi)
static void draw_dancing_character(uint8_t *buffer, int x, int y, bool is_luigi, int pose, int scale) {
    uint8_t hat_color = is_luigi ? 2 : 1;       // Green vs Red
    uint8_t shirt_color = is_luigi ? 2 : 1;     // Green vs Red
    uint8_t overalls_color = 4;                 // Blue overalls for both

    // --- Hat ---
    draw_rect(buffer, x - 3*scale, y - 9*scale, x + 2*scale, y - 8*scale, hat_color);
    draw_rect(buffer, x - 2*scale, y - 10*scale, x + 1*scale, y - 9*scale, hat_color);
    
    // --- Face ---
    draw_rect(buffer, x - 2*scale, y - 8*scale, x + 2*scale, y - 5*scale, 3); // Yellow skin
    draw_rect(buffer, x + 1*scale, y - 7*scale, x + 3*scale, y - 6*scale, 3); // Nose
    draw_pixel(buffer, x, y - 8*scale, 7); // White eye
    draw_pixel(buffer, x, y - 7*scale, 0); // Pupil
    draw_rect(buffer, x - 3*scale, y - 8*scale, x - 2*scale, y - 6*scale, 0); // Hair/Sideburn
    draw_rect(buffer, x, y - 6*scale, x + 3*scale, y - 6*scale, 0); // Mustache
    
    // --- Torso/Overalls ---
    draw_rect(buffer, x - 3*scale, y - 4*scale, x + 3*scale, y + 1*scale, overalls_color);
    draw_rect(buffer, x - 2*scale, y - 4*scale, x - 1*scale, y - 1*scale, overalls_color); // Straps
    draw_rect(buffer, x + 1*scale, y - 4*scale, x + 2*scale, y - 1*scale, overalls_color);
    draw_pixel(buffer, x - 2*scale, y - 1*scale, 3); // Yellow buttons
    draw_pixel(buffer, x + 1*scale, y - 1*scale, 3);
    draw_rect(buffer, x - 4*scale, y - 4*scale, x - 3*scale, y - 2*scale, shirt_color); // Shirt sides
    draw_rect(buffer, x + 3*scale, y - 4*scale, x + 4*scale, y - 2*scale, shirt_color);
    draw_rect(buffer, x - 2*scale, y - 5*scale, x + 2*scale, y - 5*scale, shirt_color);
    
    // --- Arms (depending on pose) ---
    if (pose == 0) {
        // Left arm pointing left/down
        draw_rect(buffer, x - 6*scale, y - 3*scale, x - 4*scale, y - 2*scale, shirt_color);
        draw_rect(buffer, x - 7*scale, y - 3*scale, x - 6*scale, y - 2*scale, 7); // Glove
        // Right arm pointing left/up
        draw_rect(buffer, x + 3*scale, y - 3*scale, x + 5*scale, y - 2*scale, shirt_color);
        draw_rect(buffer, x + 5*scale, y - 4*scale, x + 6*scale, y - 3*scale, shirt_color);
        draw_rect(buffer, x + 5*scale, y - 5*scale, x + 6*scale, y - 5*scale, 7);
    } else if (pose == 1) {
        // Both arms pointing up
        draw_rect(buffer, x - 5*scale, y - 6*scale, x - 4*scale, y - 4*scale, shirt_color);
        draw_rect(buffer, x - 6*scale, y - 8*scale, x - 5*scale, y - 6*scale, 7);
        draw_rect(buffer, x + 4*scale, y - 6*scale, x + 5*scale, y - 4*scale, shirt_color);
        draw_rect(buffer, x + 5*scale, y - 8*scale, x + 6*scale, y - 6*scale, 7);
    } else if (pose == 2) {
        // Left arm pointing right/up
        draw_rect(buffer, x - 5*scale, y - 3*scale, x - 4*scale, y - 2*scale, shirt_color);
        draw_rect(buffer, x - 7*scale, y - 4*scale, x - 6*scale, y - 3*scale, shirt_color);
        draw_rect(buffer, x - 7*scale, y - 5*scale, x - 6*scale, y - 5*scale, 7);
        // Right arm pointing right/down
        draw_rect(buffer, x + 4*scale, y - 3*scale, x + 6*scale, y - 2*scale, shirt_color);
        draw_rect(buffer, x + 6*scale, y - 3*scale, x + 7*scale, y - 2*scale, 7);
    } else {
        // Both arms pointing down
        draw_rect(buffer, x - 5*scale, y - 1*scale, x - 4*scale, y + 1*scale, shirt_color);
        draw_rect(buffer, x - 6*scale, y + 1*scale, x - 5*scale, y + 2*scale, 7);
        draw_rect(buffer, x + 4*scale, y - 1*scale, x + 5*scale, y + 1*scale, shirt_color);
        draw_rect(buffer, x + 5*scale, y + 1*scale, x + 6*scale, y + 2*scale, 7);
    }
    
    // --- Legs (depending on pose) ---
    if (pose == 0 || pose == 2) {
        // Standard legs
        draw_rect(buffer, x - 3*scale, y + 2*scale, x - 1*scale, y + 4*scale, overalls_color);
        draw_rect(buffer, x - 4*scale, y + 5*scale, x - 1*scale, y + 6*scale, 0); // Black shoes
        draw_rect(buffer, x + 1*scale, y + 2*scale, x + 3*scale, y + 4*scale, overalls_color);
        draw_rect(buffer, x + 1*scale, y + 5*scale, x + 4*scale, y + 6*scale, 0);
    } else {
        // Wide/kicking legs
        draw_rect(buffer, x - 4*scale, y + 2*scale, x - 2*scale, y + 4*scale, overalls_color);
        draw_rect(buffer, x - 5*scale, y + 4*scale, x - 3*scale, y + 5*scale, overalls_color);
        draw_rect(buffer, x - 6*scale, y + 5*scale, x - 4*scale, y + 6*scale, 0); // Shoe
        draw_rect(buffer, x + 2*scale, y + 2*scale, x + 4*scale, y + 4*scale, overalls_color);
        draw_rect(buffer, x + 3*scale, y + 4*scale, x + 5*scale, y + 5*scale, overalls_color);
        draw_rect(buffer, x + 4*scale, y + 5*scale, x + 6*scale, y + 6*scale, 0);
    }

    art_specular(buffer, x - 1*scale, y - 9*scale, 7);
    art_dither_pixel(buffer, x + 3*scale, y - 3*scale, 0, 2);
}

void play_mario_show(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);
    
    // --- 1. Draw Flashing Title Sign at Top ---
    // Alternate red/green/yellow coloring for a retro neon effect
    uint8_t title_color1 = (frame_counter % 20 < 10) ? 1 : 3; // Red <-> Yellow
    uint8_t title_color2 = (frame_counter % 20 < 10) ? 3 : 2; // Yellow <-> Green
    draw_string(buffer, "MARIO BROS", 80, 15, 3, title_color1);
    draw_string(buffer, "SUPER SHOW!", 70, 35, 3, title_color2);
    
    // Flashing star ornaments beside title
    if (frame_counter % 10 < 5) {
        draw_circle(buffer, 45, 25, 4, 3);
        draw_circle(buffer, 275, 25, 4, 3);
    } else {
        draw_pixel(buffer, 45, 25, 3);
        draw_pixel(buffer, 275, 25, 3);
    }

    // --- 2. Draw Pulsing Neon Disco Floor ---
    // Draw cells from x = 40 to 280, y = 190 to 240
    int col_width = 30;
    int row_height = 10;
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 8; c++) {
            // Cycle color per block
            uint8_t color_idx = (c + r + frame_counter / 8) % 5;
            uint8_t colors[] = {3, 1, 6, 4, 5}; // Yellow, Red, Cyan, Blue, Magenta
            uint8_t cell_color = colors[color_idx];
            
            int x1 = 40 + c * col_width;
            int x2 = x1 + col_width - 2;
            int y1 = 190 + r * row_height;
            int y2 = y1 + row_height - 2;
            
            draw_rect(buffer, x1, y1, x2, y2, cell_color);
        }
    }

    // --- 3. Draw Peeking King Koopa ---
    // Waving pose changes every 16 frames, peeks out behind left curtains
    int koopa_pose = (frame_counter / 16) % 2;
    int koopa_y = 80 + ((frame_counter / 32) % 2 == 0 ? 0 : 4); // subtle bobbing
    draw_peeking_koopa(buffer, 15, koopa_y, koopa_pose);

    // --- 4. Draw Floating Music Notes ---
    for (int i = 0; i < MAX_NOTES; i++) {
        // Floating path using sine-wave for wobble
        int wobble = (frame_counter + notes[i].phase) % 360;
        int note_x = notes[i].x + (wobble < 180 ? wobble - 90 : 270 - wobble) / 10;
        int note_y = notes[i].y;
        
        draw_music_note(buffer, note_x, note_y, notes[i].color, notes[i].is_double);
        
        // Float upwards
        notes[i].y -= notes[i].speed_y;
        if (notes[i].y < 55) {
            notes[i].y = 185;
            notes[i].x = 60 + (get_rand() % 200);
            notes[i].color = 2 + (get_rand() % 5);
        }
    }

    // --- 5. Draw Dancing Mario and Luigi ---
    int scale = 2;
    int bounce_y = (frame_counter / 6) % 2 == 0 ? 0 : 4;
    
    // Mario
    int mario_pose = (frame_counter / 12) % 4;
    draw_dancing_character(buffer, 110, 150 + bounce_y, false, mario_pose, scale);
    
    // Luigi (Dances out of phase)
    int luigi_pose = ((frame_counter / 12) + 2) % 4;
    int luigi_bounce_y = ((frame_counter / 6) + 1) % 2 == 0 ? 0 : 4;
    draw_dancing_character(buffer, 210, 150 + luigi_bounce_y, true, luigi_pose, scale);

    // --- 6. Draw Theater Curtains on Sides ---
    // Left curtain
    draw_rect(buffer, 0, 0, 12, 189, 1);  // Red main
    draw_rect(buffer, 13, 0, 17, 189, 5); // Shadow
    draw_rect(buffer, 18, 0, 28, 189, 1);
    draw_rect(buffer, 29, 0, 33, 189, 5);
    draw_rect(buffer, 34, 0, 39, 189, 1);
    draw_rect(buffer, 40, 0, 41, 189, 3); // Gold fringe/border
    
    // Right curtain
    draw_rect(buffer, 307, 0, 319, 189, 1);
    draw_rect(buffer, 302, 0, 306, 189, 5);
    draw_rect(buffer, 291, 0, 301, 189, 1);
    draw_rect(buffer, 286, 0, 290, 189, 5);
    draw_rect(buffer, 280, 0, 285, 189, 1);
    draw_rect(buffer, 278, 0, 279, 189, 3); // Gold fringe/border
    
    // Valance curtain at top
    for (int x = 0; x < 320; x++) {
        // Wavy fringe pattern
        int val_y = 12 + ((x % 40) < 20 ? (x % 20) / 2 : 10 - (x % 20) / 2);
        draw_rect(buffer, x, 0, x, val_y, 1);
        draw_pixel(buffer, x, val_y + 1, 3); // Gold valance fringe
    }

    // --- 7. Draw Scrolling Lyrics ticker at the bottom ---
    // Draw background ticker bar
    draw_rect(buffer, 0, 226, 319, 239, 0); // Black background
    draw_rect(buffer, 0, 225, 319, 225, 7); // White top border
    
    const char *current_lyric = lyrics[current_lyric_idx];
    draw_string(buffer, current_lyric, lyric_scroll_x, 229, 2, 7); // White lyric text
    
    // Update scroll position
    lyric_scroll_x -= 2;
    int text_width = strlen(current_lyric) * 8; // 8 pixels per char at scale 2
    if (lyric_scroll_x < -text_width) {
        lyric_scroll_x = 320;
        current_lyric_idx = (current_lyric_idx + 1) % NUM_LYRICS;
    }
}
