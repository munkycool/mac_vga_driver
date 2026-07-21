/* Tests firmware/core/main.c (source-included, to reach its static history-stack
 * helpers -- plain C visibility rules, the same trick emulator/Makefile already
 * uses via -Dmain=main_app, just scoped to this one #include instead of a
 * command-line flag) plus a crash-free / well-formed-output smoke pass over
 * every demo scene. Scene internals (ball positions, board state, particle
 * systems, ...) are uniformly `static` with no public accessors, so a
 * crash-free, well-formed, non-blank render is the right-sized check -- see
 * the plan file for the full per-scene rationale. */
#include "test.h"

#define main main_app
#include "../firmware/core/main.c"
#undef main

#define FB_SIZE (320 * 240)

static bool differs_from_clear(const uint8_t *buf) {
    for (int i = 0; i < FB_SIZE; i++) if (buf[i] != 0x88) return true;
    return false;
}

static void reset_input(void) {
    global_input.up = false; global_input.down = false;
    global_input.left = false; global_input.right = false;
    global_input.action1 = false; global_input.action2 = false;
    global_input.skip = false; global_input.back = false;
    global_input.interactive = false; global_input.menu = false;
    global_input.timeout_ticks = 0;
}

static void vary_input(int i) {
    reset_input();
    switch (i % 3) {
        case 0: break; /* no input held */
        case 1: global_input.up = true; global_input.right = true; global_input.action1 = true; break;
        case 2: global_input.down = true; global_input.left = true; global_input.action2 = true; break;
    }
}

static const int FRAME_VALUES[] = {
    0, 1, 2, 3, 5, 10, 25, 50, 67, 100, 133, 200, 269, 333, 400, 667, 1000, 1334
};
#define NUM_FRAME_VALUES ((int)(sizeof(FRAME_VALUES) / sizeof(FRAME_VALUES[0])))

/* ============================== dma_handler ============================== */

TEST(test_dma_handler_line_wrap_and_vblank) {
    dma_chan_a = 0;
    dma_chan_b = 1;
    current_line = 0;
    vblank_triggered = false;
    memset(scanline_buffers, 0, sizeof(scanline_buffers));

    bool saw_vblank = false;
    for (int i = 0; i < V_TOTAL; i++) {
        dma_hw_inst.ints0 = (1u << dma_chan_a);
        dma_handler();
        if (i + 1 == V_ACTIVE) {
            CHECK(vblank_triggered);
            saw_vblank = true;
            vblank_triggered = false;
        }
    }
    CHECK(saw_vblank);
    CHECK(current_line == 0); /* wraps back to 0 after exactly V_TOTAL interrupts */
}

TEST(test_dma_handler_sync_pulse_polarity) {
    dma_chan_a = 0;
    dma_chan_b = 1;

    current_line = V_ACTIVE + V_FRONT - 1; /* about to enter the vsync pulse */
    dma_hw_inst.ints0 = (1u << dma_chan_a);
    dma_handler();
    uint8_t *buf = (uint8_t *)scanline_buffers[0];
    CHECK(buf[0] == 0x00); /* sync-low during the pulse */

    current_line = V_ACTIVE; /* front porch: not in the pulse */
    dma_hw_inst.ints0 = (1u << dma_chan_a);
    dma_handler();
    CHECK(buf[0] == 0x88); /* sync-high outside the pulse */
}

TEST(test_dma_handler_no_pending_irq_is_noop) {
    dma_chan_a = 0;
    dma_chan_b = 1;
    dma_hw_inst.ints0 = 0;
    int before = current_line;
    dma_handler();
    CHECK(current_line == before);
}

/* ===================== state history stack (static helpers) ===================== */

TEST(test_state_history_back_and_forward) {
    state_history_count = 0;
    state_history_index = -1;

    push_state_history(STATE_PONG);
    push_state_history(STATE_TETRIS);
    push_state_history(STATE_PACMAN);

    CHECK(go_back_state(STATE_PACMAN) == STATE_TETRIS);
    CHECK(go_back_state(STATE_TETRIS) == STATE_PONG);
    CHECK(go_back_state(STATE_PONG) == STATE_PONG); /* already at the oldest entry */

    CHECK(go_forward_state(STATE_PONG) == STATE_TETRIS);
    CHECK(go_forward_state(STATE_TETRIS) == STATE_PACMAN);
}

TEST(test_state_history_truncates_on_branch) {
    state_history_count = 0;
    state_history_index = -1;
    push_state_history(STATE_PONG);
    push_state_history(STATE_TETRIS);
    push_state_history(STATE_PACMAN);

    go_back_state(STATE_PACMAN); /* now at TETRIS (index 1) */
    go_back_state(STATE_TETRIS); /* now at PONG (index 0) */

    push_state_history(STATE_SNAKE); /* new branch discards the PACMAN redo entry */
    CHECK(state_history_count == 2);
    CHECK(go_forward_state(STATE_SNAKE) != STATE_PACMAN);
}

TEST(test_state_history_ring_buffer_wraps) {
    state_history_count = 0;
    state_history_index = -1;
    for (int i = 0; i < STATE_HISTORY_SIZE + 10; i++) {
        push_state_history((enum ProgramState)(i % NUM_STATES));
    }
    CHECK(state_history_count == STATE_HISTORY_SIZE);
    CHECK(state_history_index == STATE_HISTORY_SIZE - 1);
}

/* ===================== init_screensaver dispatch table ===================== */

TEST(test_init_screensaver_dispatch_no_crash) {
    for (int i = 0; i <= (int)STATE_SPLASH; i++) {
        init_screensaver((enum ProgramState)i);
    }
}

/* ============================ scene smoke tests ============================ */

typedef void (*init_fn)(void);
typedef void (*play_frame_fn)(uint8_t *, int);
typedef void (*play_noframe_fn)(uint8_t *);

typedef struct { const char *name; init_fn init; play_frame_fn play; } scene_frame_t;
typedef struct { const char *name; init_fn init; play_noframe_fn play; } scene_noframe_t;

static const scene_frame_t SCENES_WITH_FRAME[] = {
    {"xbox_pride", init_xbox_pride, play_xbox_pride},
    {"pacman", init_pacman, play_pacman},
    {"invaders", init_invaders, play_invaders},
    {"mario_nes", init_mario_nes, play_mario_nes},
    {"metroid", init_metroid, play_metroid},
    {"synthwave", init_synthwave, play_synthwave},
    {"city_skyline", init_city_skyline, play_city_skyline},
    {"fireplace", init_fireplace, play_fireplace},
    {"cherry_blossom", init_cherry_blossom, play_cherry_blossom},
    {"cyberpunk", init_cyberpunk, play_cyberpunk},
    {"nebula", init_nebula, play_nebula},
    {"quotes", init_quotes, play_quotes},
    {"frogger", init_frogger, play_frogger},
    {"snake", init_snake, play_snake},
    {"flappy_bird", init_flappy_bird, play_flappy_bird},
    {"sanic", init_sanic, play_sanic},
    {"undertale", init_undertale, play_undertale},
    {"deltarune", init_deltarune, play_deltarune},
    {"nasa", init_nasa, play_nasa},
    {"mario_show", init_mario_show, play_mario_show},
    {"challenger", init_challenger, play_challenger},
    {"nine_eleven", init_nine_eleven, play_nine_eleven},
    {"pinup", init_pinup, play_pinup},
    {"pride", init_pride, play_pride},
    {"chan", init_chan, play_chan},
    {"revolution", init_revolution, play_revolution},
    /* "downloads" is deliberately not in this table: with the synthetic
     * zero-image asset (test_stubs.c), play_downloads() always clears to
     * background, so the "did it draw anything" check below would legitimately
     * fail. It gets its own guard-path test instead. */
    {"perkins", init_perkins, play_perkins},
    {"happy", init_happy, play_happy},
    {"happy_bd", init_happy_bd, play_happy_bd},
    {"happy_mdd", init_happy_mdd, play_happy_mdd},
    {"happy_ocd", init_happy_ocd, play_happy_ocd},
    {"happy_mania", init_happy_mania, play_happy_mania},
    {"happy_mania_sunrise", init_happy_mania_sunrise, play_happy_mania_sunrise},
    {"happy_mania_clouds", init_happy_mania_clouds, play_happy_mania_clouds},
    {"happy_mania_gems", init_happy_mania_gems, play_happy_mania_gems},
    {"happy_psychosis", init_happy_psychosis, play_happy_psychosis},
    {"celeste", init_celeste, play_celeste},
    {"splash", init_splash, play_splash},
    /* Disabled from init_screensaver's dispatch (see the comment above it), but
     * still has a real public init/play pair -- smoke coverage is the only
     * thing guarding this ~1850-line file from silently bitrotting. */
    {"minecraft", init_minecraft, play_minecraft},
};

static const scene_noframe_t SCENES_NO_FRAME[] = {
    {"pong", init_pong, play_pong},
    {"tetris", init_tetris, play_tetris},
    {"donkey_kong", init_donkey_kong, play_donkey_kong},
    {"breakout", init_breakout, play_breakout},
    {"asteroids", init_asteroids, play_asteroids},
    {"tron", init_tron, play_tron},
};

TEST(test_scenes_with_frame_counter_smoke) {
    uint8_t buf[FB_SIZE];
    for (size_t s = 0; s < sizeof(SCENES_WITH_FRAME) / sizeof(SCENES_WITH_FRAME[0]); s++) {
        current_test = SCENES_WITH_FRAME[s].name;
        memset(buf, 0x88, FB_SIZE); /* matches real firmware's boot-time clear_screen state */
        SCENES_WITH_FRAME[s].init();
        bool ever_differed = false;
        for (int i = 0; i < NUM_FRAME_VALUES; i++) {
            vary_input(i);
            SCENES_WITH_FRAME[s].play(buf, FRAME_VALUES[i]);
            CHECK(buffer_is_well_formed(buf, FB_SIZE));
            if (differs_from_clear(buf)) ever_differed = true;
        }
        CHECK(ever_differed);
    }
    reset_input();
}

TEST(test_scenes_no_frame_counter_smoke) {
    uint8_t buf[FB_SIZE];
    for (size_t s = 0; s < sizeof(SCENES_NO_FRAME) / sizeof(SCENES_NO_FRAME[0]); s++) {
        current_test = SCENES_NO_FRAME[s].name;
        memset(buf, 0x88, FB_SIZE);
        SCENES_NO_FRAME[s].init();
        bool ever_differed = false;
        for (int i = 0; i < NUM_FRAME_VALUES; i++) {
            vary_input(i);
            SCENES_NO_FRAME[s].play(buf);
            CHECK(buffer_is_well_formed(buf, FB_SIZE));
            if (differs_from_clear(buf)) ever_differed = true;
        }
        CHECK(ever_differed);
    }
    reset_input();
}

TEST(test_bouncing_box_smoke) {
    uint8_t buf[FB_SIZE];
    memset(buf, 0x88, FB_SIZE);
    init_bouncing_box();
    bool ever_differed = false;
    for (int i = 0; i < 60; i++) {
        play_dvd(buf);
        CHECK(buffer_is_well_formed(buf, FB_SIZE));
        if (differs_from_clear(buf)) ever_differed = true;
        tick_bouncing_box();
    }
    CHECK(ever_differed);
}

TEST(test_downloads_no_images_guard) {
    uint8_t buf[FB_SIZE];
    memset(buf, 0x42, FB_SIZE); /* poison: proves play_downloads() actually writes */
    init_downloads();
    play_downloads(buf, 0);
    /* downloads_image_count == 0 (test_stubs.c's synthetic asset): the
     * documented guard clears to background and returns early. */
    CHECK(buffer_is_well_formed(buf, FB_SIZE));
    CHECK(!differs_from_clear(buf));
}

int main(void) {
    RUN(test_dma_handler_line_wrap_and_vblank);
    RUN(test_dma_handler_sync_pulse_polarity);
    RUN(test_dma_handler_no_pending_irq_is_noop);
    RUN(test_state_history_back_and_forward);
    RUN(test_state_history_truncates_on_branch);
    RUN(test_state_history_ring_buffer_wraps);
    RUN(test_init_screensaver_dispatch_no_crash);
    RUN(test_scenes_with_frame_counter_smoke);
    RUN(test_scenes_no_frame_counter_smoke);
    RUN(test_bouncing_box_smoke);
    RUN(test_downloads_no_images_guard);
    return TEST_SUMMARY();
}
