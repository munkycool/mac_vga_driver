#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "vga.pio.h"

// --- Mac M1212 67Hz Custom Timings ---
#define H_ACTIVE   640
#define H_FRONT    64
#define H_SYNC     64
#define H_BACK     96
#define H_TOTAL    864

#define V_ACTIVE   480
#define V_FRONT    3
#define V_SYNC     3
#define V_BACK     39
#define V_TOTAL    525

// --- Pin Definitions ---
#define PIN_BASE   7 

// --- Downloads Slideshow Assets ---
#define DOWNLOADS_FRAME_WIDTH 320
#define DOWNLOADS_FRAME_HEIGHT 240
#define DOWNLOADS_FRAME_BYTES (DOWNLOADS_FRAME_WIDTH * DOWNLOADS_FRAME_HEIGHT)

// --- Deterministic State Machine ---
#define NUM_STATES 41
#define SPLASH_PROB 3  // 1-in-SPLASH_PROB chance of boot splash
enum ProgramState {
    STATE_XBOX_PRIDE,
    STATE_BAD_APPLE,
    STATE_BOUNCING_BOX,
    STATE_PONG,
    STATE_TETRIS,
    STATE_PACMAN,
    STATE_INVADERS,
    STATE_DONKEY_KONG,
    STATE_MARIO_NES,
    STATE_METROID,
    STATE_SYNTHWAVE,
    STATE_CITY_SKYLINE,
    STATE_FIREPLACE,
    STATE_CHERRY_BLOSSOM,
    STATE_CYBERPUNK,
    STATE_NEBULA,
    STATE_QUOTES,
    STATE_FROGGER,
    STATE_SNAKE,
    STATE_FLAPPY_BIRD,
    STATE_BREAKOUT,
    STATE_ASTEROIDS,
    STATE_TRON,
    STATE_SANIC,
    STATE_UNDERTALE,
    STATE_DELTARUNE,
    STATE_NASA,
    STATE_MARIO_SHOW,
    STATE_CHALLENGER,
    STATE_NINE_ELEVEN,
    STATE_PINUP,
    STATE_PRIDE,
    STATE_MINECRAFT,
    STATE_CHAN,
    STATE_REVOLUTION,
    STATE_DOWNLOADS,
    STATE_PERKINS,
    STATE_HAPPY,
    STATE_HAPPY_BD,
    STATE_HAPPY_MDD,
    STATE_HAPPY_OCD,
    // NOTE: STATE_SPLASH is NOT in the shuffle bag – triggered separately
    STATE_SPLASH
};

typedef struct {
    int cx, cy;
    int dx, dy;
    int px, py;
    uint8_t color;
    bool dead;
} Ghost;

typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
    bool action1;
    bool action2;
    bool skip;
    bool back;
    bool interactive;
    int timeout_ticks;
} InputState;

extern volatile InputState global_input;
void update_input();

// --- External Assembly Symbols ---
extern const uint8_t bad_apple_bin[];
extern const uint8_t bad_apple_bin_end[];

// --- Alphanumeric Font Table ---
extern const uint16_t ALPHANUM_FONT[41];

// --- Utility Functions ---
void seed_rng();
uint32_t get_rand();
int isqrt(int value);
enum ProgramState pick_next_state(enum ProgramState current);

// --- Graphics Functions ---
void clear_screen(uint8_t *buffer);
void draw_pixel(uint8_t *buffer, int x, int y, uint8_t color);
void draw_rect(uint8_t *buffer, int x1, int y1, int x2, int y2, uint8_t color);
void draw_circle(uint8_t *buffer, int cx, int cy, int r, uint8_t color);

// Downloads slideshow frames generated from the local Downloads folder.
extern const size_t downloads_image_count;
extern const uint8_t downloads_images[][DOWNLOADS_FRAME_BYTES];

// 4K ULTRA HD PRO PLUS ULTRA rendering engine
// Ordered dithering + block upscaling for faux high-fidelity on 320x240
#define ART_HD_SCALE 2

static inline int art_bayer4_at(int x, int y) {
    static const uint8_t t[4][4] = {
        { 0,  8,  2, 10},
        {12,  4, 14,  6},
        { 3, 11,  1,  9},
        {15,  7, 13,  5}
    };
    return t[y & 3][x & 3];
}

void art_dither_pixel(uint8_t *buf, int x, int y, uint8_t color, int density4);
void art_dither_rect_v(uint8_t *buf, int x1, int y1, int x2, int y2,
                       uint8_t color_a, uint8_t color_b, int height);
void art_dither_rect_h(uint8_t *buf, int x1, int y1, int x2, int y2,
                       uint8_t color_a, uint8_t color_b, int width);
void art_block(uint8_t *buf, int x, int y, uint8_t color, int scale);
void art_rect(uint8_t *buf, int x1, int y1, int x2, int y2, uint8_t color, int scale);
void art_sprite(uint8_t *buf, const uint8_t *pixels, int w, int h,
                int ox, int oy, int scale, uint8_t transparent);
void art_shaded_rect(uint8_t *buf, int x1, int y1, int x2, int y2,
                     uint8_t base, uint8_t hi, uint8_t shadow);
void art_circle_shaded(uint8_t *buf, int cx, int cy, int r,
                       uint8_t color_dark, uint8_t color_light);
void art_specular(uint8_t *buf, int x, int y, uint8_t color);
int get_char_idx(char c);
void draw_char(uint8_t *buffer, char c, int x, int y, int scale, uint8_t color);
void draw_string(uint8_t *buffer, const char *str, int x, int y, int scale, uint8_t color);
void draw_score(uint8_t *buffer, int score, int x, int y, int scale, uint8_t color);
void draw_pro_mario(uint8_t *buffer, int x, int y);

// --- State/Screensaver Functions ---

// STATE_XBOX_PRIDE
void init_xbox_pride();
void play_xbox_pride(uint8_t *buffer, int frame);

// STATE_BAD_APPLE
void init_bad_apple();
void play_video(uint8_t *buffer);
void tick_video();

// STATE_BOUNCING_BOX
void init_bouncing_box();
void play_dvd(uint8_t *buffer);
void tick_bouncing_box();

// STATE_PONG
void init_pong();
void play_pong(uint8_t *buffer);

// STATE_TETRIS
void init_tetris();
void play_tetris(uint8_t *buffer);

// STATE_PACMAN
void init_pacman();
void play_pacman(uint8_t *buffer, int frame_counter);

// STATE_INVADERS
void init_invaders();
void play_invaders(uint8_t *buffer, int frame_counter);

// STATE_DONKEY_KONG
void init_donkey_kong();
void play_donkey_kong(uint8_t *buffer);

// STATE_MARIO_NES
void init_mario_nes();
void play_mario_nes(uint8_t *buffer, int frame_counter);

// STATE_METROID
void init_metroid();
void play_metroid(uint8_t *buffer, int frame_counter);

// STATE_SYNTHWAVE
void init_synthwave();
void play_synthwave(uint8_t *buffer, int frame_counter);

// STATE_CITY_SKYLINE
void init_city_skyline();
void play_city_skyline(uint8_t *buffer, int frame_counter);

// STATE_FIREPLACE
void init_fireplace();
void play_fireplace(uint8_t *buffer, int frame_counter);

// STATE_CHERRY_BLOSSOM
void init_cherry_blossom();
void play_cherry_blossom(uint8_t *buffer, int frame_counter);

// STATE_PINUP
void init_pinup();
void play_pinup(uint8_t *buffer, int frame_counter);

// STATE_PRIDE
void init_pride();
void play_pride(uint8_t *buffer, int frame_counter);

// STATE_CYBERPUNK
void init_cyberpunk();
void play_cyberpunk(uint8_t *buffer, int frame_counter);

// STATE_NEBULA
void init_nebula();
void play_nebula(uint8_t *buffer, int frame_counter);

// STATE_QUOTES
void init_quotes();
void play_quotes(uint8_t *buffer, int frame_counter);

// STATE_FROGGER
void init_frogger();
void play_frogger(uint8_t *buffer, int frame_counter);

// STATE_SNAKE
void init_snake();
void play_snake(uint8_t *buffer, int frame_counter);

// STATE_FLAPPY_BIRD
void init_flappy_bird();
void play_flappy_bird(uint8_t *buffer, int frame_counter);

// STATE_BREAKOUT
void init_breakout();
void play_breakout(uint8_t *buffer);

// STATE_ASTEROIDS
void init_asteroids();
void play_asteroids(uint8_t *buffer);

// STATE_TRON
void init_tron();
void play_tron(uint8_t *buffer);

// STATE_SANIC
void init_sanic();
void play_sanic(uint8_t *buffer, int frame_counter);

// STATE_UNDERTALE
void init_undertale();
void play_undertale(uint8_t *buffer, int frame_counter);

// STATE_DELTARUNE
void init_deltarune();
void play_deltarune(uint8_t *buffer, int frame_counter);

// STATE_NASA
void init_nasa();
void play_nasa(uint8_t *buffer, int frame_counter);

// STATE_MARIO_SHOW
void init_mario_show();
void play_mario_show(uint8_t *buffer, int frame_counter);

// STATE_CHALLENGER
void init_challenger();
void play_challenger(uint8_t *buffer, int frame_counter);

// STATE_NINE_ELEVEN
void init_nine_eleven();
void play_nine_eleven(uint8_t *buffer, int frame_counter);

// STATE_SPLASH (boot-time, not in shuffle bag)
void init_splash();
void play_splash(uint8_t *buffer, int frame_counter);

// STATE_MINECRAFT
void init_minecraft();
void play_minecraft(uint8_t *buffer, int frame_counter);

// STATE_CHAN
void init_chan();
void play_chan(uint8_t *buffer, int frame_counter);

// STATE_REVOLUTION
void init_revolution();
void play_revolution(uint8_t *buffer, int frame_counter);

// STATE_DOWNLOADS
void init_downloads();
void play_downloads(uint8_t *buffer, int frame_counter);

// STATE_PERKINS
void init_perkins();
void play_perkins(uint8_t *buffer, int frame_counter);

// STATE_HAPPY
void init_happy();
void play_happy(uint8_t *buffer, int frame_counter);

// STATE_HAPPY_BD
void init_happy_bd();
void play_happy_bd(uint8_t *buffer, int frame_counter);

// STATE_HAPPY_MDD
void init_happy_mdd();
void play_happy_mdd(uint8_t *buffer, int frame_counter);

// STATE_HAPPY_OCD
void init_happy_ocd();
void play_happy_ocd(uint8_t *buffer, int frame_counter);

#endif // COMMON_H
