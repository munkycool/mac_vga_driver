#include "common.h"

// 16x16 Green 4chan Clover Sprite
// 2: Green, 8: Transparent
static const uint8_t CLOVER_SPRITE[16][16] = {
    {8,8,8,2,2,8,8,8,8,8,2,2,8,8,8,8},
    {8,8,2,2,2,2,8,8,8,2,2,2,2,8,8,8},
    {8,2,2,2,2,2,2,8,2,2,2,2,2,2,8,8},
    {8,2,2,2,2,2,2,8,2,2,2,2,2,2,8,8},
    {8,8,2,2,2,2,8,8,8,2,2,2,2,8,8,8},
    {8,8,8,2,2,8,8,8,8,8,2,2,8,8,8,8},
    {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
    {8,8,8,2,2,8,8,8,8,8,2,2,8,8,8,8},
    {8,8,2,2,2,2,8,8,8,2,2,2,2,8,8,8},
    {8,2,2,2,2,2,2,8,2,2,2,2,2,2,8,8},
    {8,2,2,2,2,2,2,8,2,2,2,2,2,2,8,8},
    {8,8,2,2,2,2,8,8,8,2,2,2,2,8,8,8},
    {8,8,8,2,2,8,8,8,8,8,2,2,8,8,8,8},
    {8,8,8,8,8,8,8,8,2,2,8,8,8,8,8,8},
    {8,8,8,8,8,8,8,2,2,8,8,8,8,8,8,8},
    {8,8,8,8,8,8,8,2,8,8,8,8,8,8,8,8}
};

// 16x16 Derpy Trollface Sprite
// 0: Black, 7: White, 8: Transparent
static const uint8_t TROLLFACE_SPRITE[16][16] = {
    {8,8,8,0,0,0,0,0,0,0,8,8,8,8,8,8},
    {8,8,0,7,7,7,7,7,7,7,0,8,8,8,8,8},
    {8,0,7,7,7,7,7,7,7,7,7,0,8,8,8,8},
    {0,7,7,0,7,7,7,7,0,7,7,7,0,8,8,8},
    {0,7,0,0,0,7,7,0,0,0,7,7,0,8,8,8},
    {0,7,7,0,7,7,7,7,0,7,7,7,0,8,8,8},
    {0,7,7,7,7,7,7,7,7,7,7,7,0,8,8,8},
    {8,0,7,7,0,0,0,0,0,7,7,0,8,8,8,8},
    {8,8,0,7,7,7,7,7,7,7,0,8,8,8,8,8},
    {8,8,8,0,0,0,0,0,0,0,8,8,8,8,8,8},
    {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
    {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
    {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
    {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
    {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8},
    {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8}
};

static const char *STORIES[4][10] = {
    {
        "> Be me",
        "> Writing Pico VGA code",
        "> Compile took 2 seconds",
        "> Flash took 1 second",
        "> Runs at 60 FPS",
        "> Meanwhile modern web apps",
        "> Need 16GB RAM to show a button",
        "> mfw embedded C wins",
        NULL
    },
    {
        "> Be hacker on /vga/",
        "> Plug in Pico USB",
        "> Screen begins to sync",
        "> VGA monitor starts buzzing",
        "> \"I'm in.\"",
        "> Screen displays: \"Pool's Closed\"",
        "> mfw hacked by a $4 chip",
        NULL
    },
    {
        "> Be me",
        "> Overclocking RP2040 to 250MHz",
        "> Core temperature: 80C",
        "> Coffee cup placed on chip",
        "> Keeps coffee perfectly warm",
        "> No radiator needed",
        "> mfw feature not bug",
        NULL
    },
    {
        "> Be me, retro gamer",
        "> Play breakout on VGA",
        "> Paddle is 4 pixels wide",
        "> Ball moves at Mach 3",
        "> Miss the ball instantly",
        "> Game over in 1 second",
        "> \"10/10 master class design\"",
        NULL
    }
};

static int selected_story = 0;
static int logo_x = 100, logo_y = 150;
static int logo_dx = 2, logo_dy = 1;
static int story_type_timer = 0;
static int story_line_count = 0;
static int story_char_count = 0;
static bool logo_is_troll = false;

void init_chan() {
    selected_story = get_rand() % 4;
    logo_x = 20 + (get_rand() % 240);
    logo_y = 120 + (get_rand() % 80);
    logo_dx = (get_rand() % 2 == 0) ? 2 : -2;
    logo_dy = (get_rand() % 2 == 0) ? 1 : -1;
    logo_is_troll = (get_rand() % 2 == 0);
    story_type_timer = 0;
    story_line_count = 0;
    story_char_count = 0;
}

static void fill_beige_bg(uint8_t *buffer) {
    for (int y = 0; y < 240; y++) {
        uint8_t *row = &buffer[y * 320];
        for (int x = 0; x < 320; x++) {
            // Fast checkerboard pattern of 3 (Yellow) and 7 (White)
            uint8_t color = ((x + y) & 1) ? 7 : 3;
            uint8_t pixel = (color & 0x07) | 0x08;
            row[x] = (pixel << 4) | pixel;
        }
    }
}

void play_chan(uint8_t *buffer, int frame_counter) {
    // 1. Fill background with dithered beige
    fill_beige_bg(buffer);

    // 2. Draw Top Bar (Header)
    draw_rect(buffer, 0, 0, 319, 14, 2); // Green header bar
    draw_string(buffer, "/vga/ - Video Graphics Array", 6, 4, 1, 7);

    // 3. Draw Post container
    // Outer border: Blue (4)
    // Fill: Solid White (7)
    draw_rect(buffer, 8, 22, 311, 230, 4);
    draw_rect(buffer, 9, 23, 310, 229, 7);

    // Post header: Name (Green/Sage - 2), Date/Time, Post ID (Red - 1)
    draw_string(buffer, "Anonymous", 14, 28, 1, 2);
    draw_string(buffer, "07/14/26(Tue)03:20:00 No.", 75, 28, 1, 0);
    draw_string(buffer, "9001337", 235, 28, 1, 1);

    // Divider line
    draw_rect(buffer, 12, 38, 307, 39, 4);

    // 4. Draw typing greentext story
    int draw_y = 46;
    for (int i = 0; i <= story_line_count; i++) {
        const char *line = STORIES[selected_story][i];
        if (!line) break;

        if (i == story_line_count) {
            // Draw currently typing line partially
            char temp_buf[80];
            int len = strlen(line);
            int chars_to_draw = (story_char_count < len) ? story_char_count : len;
            strncpy(temp_buf, line, chars_to_draw);
            temp_buf[chars_to_draw] = '\0';
            draw_string(buffer, temp_buf, 16, draw_y, 1, 2); // Greentext (2)
        } else {
            // Draw complete lines
            draw_string(buffer, line, 16, draw_y, 1, 2);
        }
        draw_y += 10;
    }

    // Story typing physics
    story_type_timer++;
    if (story_type_timer >= 2) {
        story_type_timer = 0;
        const char *curr_line = STORIES[selected_story][story_line_count];
        if (curr_line) {
            int len = strlen(curr_line);
            if (story_char_count < len) {
                story_char_count++;
            } else {
                // Move to next line if there is one
                if (STORIES[selected_story][story_line_count + 1] != NULL) {
                    story_line_count++;
                    story_char_count = 0;
                }
            }
        }
    }

    // 5. Bouncing Logo Physics & Render
    logo_x += logo_dx;
    logo_y += logo_dy;

    // Bounce off post box inner bounds
    if (logo_x <= 10 || logo_x >= 284) {
        logo_dx = -logo_dx;
    }
    if (logo_y <= 24 || logo_y >= 212) {
        logo_dy = -logo_dy;
    }

    // Draw the logo (scaled 2x to be 32x32)
    if (logo_is_troll) {
        art_sprite(buffer, (const uint8_t *)TROLLFACE_SPRITE, 16, 16, logo_x, logo_y, 2, 8);
    } else {
        art_sprite(buffer, (const uint8_t *)CLOVER_SPRITE, 16, 16, logo_x, logo_y, 2, 8);
    }
}
