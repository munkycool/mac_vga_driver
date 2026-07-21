#include "../firmware/core/common.h"
#include "test.h"

#define FB_SIZE (320 * 240)

typedef struct {
    uint8_t before[16];
    uint8_t fb[FB_SIZE];
    uint8_t after[16];
} canary_buf_t;

static void set_canaries(canary_buf_t *c) {
    memset(c->before, 0xCC, sizeof(c->before));
    memset(c->after, 0xCC, sizeof(c->after));
}

static bool canaries_intact(const canary_buf_t *c) {
    for (size_t i = 0; i < sizeof(c->before); i++) {
        if (c->before[i] != 0xCC) return false;
        if (c->after[i] != 0xCC) return false;
    }
    return true;
}

/* ---- common.c: isqrt ---- */
TEST(test_isqrt) {
    CHECK(isqrt(0) == 0);
    CHECK(isqrt(1) == 1);
    CHECK(isqrt(4) == 2);
    CHECK(isqrt(9) == 3);
    CHECK(isqrt(15) == 3);
    CHECK(isqrt(16) == 4);
    CHECK(isqrt(1000000) == 1000);
    CHECK(isqrt(-5) == 0);
    int prev = 0;
    for (int v = 0; v <= 2000; v++) {
        int r = isqrt(v);
        CHECK(r >= prev);
        CHECK(r * r <= v);
        prev = r;
    }
}

/* ---- common.c: get_rand (deterministic LCG, never seeded in tests) ---- */
TEST(test_get_rand) {
    /* Must run before anything else touches get_rand() (e.g. pick_next_state)
     * so the predicted sequence below matches the real one from rng_state==0. */
    uint32_t predicted_state = 0;
    for (int i = 0; i < 20; i++) {
        predicted_state = predicted_state * 1103515245u + 12345u;
        uint32_t predicted = (predicted_state / 65536) % 32768;
        uint32_t actual = get_rand();
        CHECK(actual == predicted);
        CHECK(actual < 32768);
    }
}

/* ---- common.c: pick_next_state shuffle-bag ---- */
TEST(test_pick_next_state) {
    int counts[NUM_STATES] = {0};
    enum ProgramState cur = (enum ProgramState)0;
    for (int i = 0; i < NUM_STATES; i++) {
        cur = pick_next_state(cur);
        CHECK(cur >= 0 && cur < NUM_STATES);
        counts[cur]++;
    }
    CHECK(counts[STATE_MINECRAFT] == 0);
    CHECK(counts[STATE_SYNTHWAVE] == 2);
    for (int s = 0; s < NUM_STATES; s++) {
        if (s == STATE_MINECRAFT || s == STATE_SYNTHWAVE) continue;
        CHECK(counts[s] == 1);
    }
}

/* ---- common.c: get_char_idx ---- */
TEST(test_get_char_idx) {
    for (char c = '0'; c <= '9'; c++) CHECK(get_char_idx(c) == c - '0');
    for (char c = 'a'; c <= 'z'; c++) CHECK(get_char_idx(c) == get_char_idx(c - 32));
    for (char c = 'A'; c <= 'Z'; c++) CHECK(get_char_idx(c) == c - 'A' + 10);
    CHECK(get_char_idx(' ') == 36);
    CHECK(get_char_idx(':') == 37);
    CHECK(get_char_idx('!') == 38);
    CHECK(get_char_idx('-') == 39);
    CHECK(get_char_idx('?') == 40);
    CHECK(get_char_idx('@') == 36); /* unknown -> space */
}

/* ---- common.c: draw_pixel ---- */
TEST(test_draw_pixel) {
    uint8_t buf[FB_SIZE];
    memset(buf, 0xAA, sizeof(buf));

    draw_pixel(buf, -1, 0, 7);
    draw_pixel(buf, 320, 0, 7);
    draw_pixel(buf, 0, -1, 7);
    draw_pixel(buf, 0, 240, 7);
    CHECK(buf[0] == 0xAA); /* all out-of-bounds writes are no-ops */

    draw_pixel(buf, 0, 0, 7);
    uint8_t expected = ((7 & 7) | 8) << 4 | ((7 & 7) | 8);
    CHECK(buf[0] == expected);

    draw_pixel(buf, 319, 239, 3);
    uint8_t expected2 = ((3 & 7) | 8) << 4 | ((3 & 7) | 8);
    CHECK(buf[239 * 320 + 319] == expected2);
}

/* ---- common.c: clear_screen ---- */
TEST(test_clear_screen) {
    uint8_t buf[FB_SIZE];
    memset(buf, 0, sizeof(buf));
    clear_screen(buf);
    CHECK(buf[0] == 0x88);
    CHECK(buf[FB_SIZE / 2] == 0x88);
    CHECK(buf[FB_SIZE - 1] == 0x88);
    CHECK(buffer_is_well_formed(buf, FB_SIZE));
}

/* ---- common.c: draw_rect / draw_circle out-of-bounds safety ---- */
TEST(test_draw_rect_oob) {
    canary_buf_t c;
    set_canaries(&c);
    memset(c.fb, 0x88, FB_SIZE);
    draw_rect(c.fb, -1000, -1000, 1000, 1000, 5);
    CHECK(canaries_intact(&c));
    CHECK(buffer_is_well_formed(c.fb, FB_SIZE));
    CHECK(c.fb[0] != 0x88); /* fully covers the buffer, so it did get painted */
}

TEST(test_draw_circle_oob) {
    canary_buf_t c;
    set_canaries(&c);
    memset(c.fb, 0x88, FB_SIZE);
    draw_circle(c.fb, 160, 120, 100000, 5);
    CHECK(canaries_intact(&c));
    CHECK(buffer_is_well_formed(c.fb, FB_SIZE));
}

/* ---- common.c: art_bayer4_at ---- */
TEST(test_art_bayer4_at) {
    static const uint8_t expected[4][4] = {
        { 0,  8,  2, 10},
        {12,  4, 14,  6},
        { 3, 11,  1,  9},
        {15,  7, 13,  5}
    };
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            CHECK(art_bayer4_at(x, y) == expected[y][x]);
    /* periodic with period 4 */
    CHECK(art_bayer4_at(0, 0) == art_bayer4_at(4, 8));
    CHECK(art_bayer4_at(5, 1) == art_bayer4_at(9, 5));
}

/* ---- common.c: art_dither_pixel threshold behavior ---- */
TEST(test_art_dither_pixel) {
    uint8_t buf[FB_SIZE];

    /* density4 <= 0: never draws */
    memset(buf, 0x88, FB_SIZE);
    art_dither_pixel(buf, 0, 0, 7, 0);
    CHECK(buf[0] == 0x88);

    /* density4 >= 4: always draws, same as draw_pixel */
    memset(buf, 0x88, FB_SIZE);
    art_dither_pixel(buf, 0, 0, 7, 4);
    uint8_t expected = ((7 & 7) | 8) << 4 | ((7 & 7) | 8);
    CHECK(buf[0] == expected);

    /* density4 == 2: ART_BAYER2 = {{0,2},{3,1}}, drawn where value < 2 */
    memset(buf, 0x88, FB_SIZE);
    art_dither_pixel(buf, 0, 0, 7, 2); /* bayer=0 -> drawn */
    art_dither_pixel(buf, 1, 0, 7, 2); /* bayer=2 -> not drawn */
    art_dither_pixel(buf, 0, 1, 7, 2); /* bayer=3 -> not drawn */
    art_dither_pixel(buf, 1, 1, 7, 2); /* bayer=1 -> drawn */
    CHECK(buf[0] == expected);
    CHECK(buf[1] == 0x88);
    CHECK(buf[320] == 0x88);
    CHECK(buf[321] == expected);
}

/* ---- common.c: draw_string / draw_char smoke ---- */
TEST(test_draw_string_smoke) {
    uint8_t buf[FB_SIZE];
    memset(buf, 0x88, FB_SIZE);
    draw_string(buf, "HELLO: -!?", 10, 10, 2, 7);
    CHECK(buffer_is_well_formed(buf, FB_SIZE));
}

/* ---- common.c: draw_score zero-pads to 4 digits via sprintf("%04d", ...) ---- */
TEST(test_draw_score_zero_pad) {
    uint8_t buf_score[FB_SIZE], buf_string[FB_SIZE];

    memset(buf_score, 0x88, FB_SIZE);
    memset(buf_string, 0x88, FB_SIZE);
    draw_score(buf_score, 7, 10, 10, 1, 7);
    draw_string(buf_string, "0007", 10, 10, 1, 7);
    CHECK(memcmp(buf_score, buf_string, FB_SIZE) == 0);

    memset(buf_score, 0x88, FB_SIZE);
    memset(buf_string, 0x88, FB_SIZE);
    draw_score(buf_score, 12345, 10, 10, 1, 7); /* longer than 4 digits: not truncated */
    draw_string(buf_string, "12345", 10, 10, 1, 7);
    CHECK(memcmp(buf_score, buf_string, FB_SIZE) == 0);
}

/* ---- video.c: init_bad_apple/play_video/tick_video guard paths ----
 * No bad_apple.bin data on the host (see test_stubs.c), so total_frames == 0
 * and these should behave as documented no-ops without crashing. */
TEST(test_video_no_data_guard) {
    test_set_fake_time_us(0);
    init_bad_apple();

    uint8_t buf[FB_SIZE];
    memset(buf, 0x42, FB_SIZE);
    play_video(buf);
    CHECK(buf[0] == 0x42); /* untouched: total_frames == 0 guard */

    for (int i = 0; i < 100; i++) {
        test_advance_fake_time_us(50000);
        tick_video();
    }
    play_video(buf);
    CHECK(buf[0] == 0x42);
}

int main(void) {
    RUN(test_isqrt);
    RUN(test_get_rand);
    RUN(test_pick_next_state);
    RUN(test_get_char_idx);
    RUN(test_draw_pixel);
    RUN(test_clear_screen);
    RUN(test_draw_rect_oob);
    RUN(test_draw_circle_oob);
    RUN(test_art_bayer4_at);
    RUN(test_art_dither_pixel);
    RUN(test_draw_string_smoke);
    RUN(test_draw_score_zero_pad);
    RUN(test_video_no_data_guard);
    return TEST_SUMMARY();
}
