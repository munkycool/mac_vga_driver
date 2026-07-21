/* Tests the ported PICO-8 Celeste Classic engine through its public
 * Celeste_P8_* API. The fake callback and tilemap reuse mirror
 * tools/celeste_train.c's existing pattern (MGET/FGET backed by the real
 * tilemap.h asset), not a new seam invented for this test. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include "../firmware/scenes/games/celeste_classic.h"
#include "../firmware/scenes/games/tilemap.h"
#include "test.h"

static bool sim_buttons[6] = {false};

static int fake_p8_callback(CELESTE_P8_CALLBACK_TYPE calltype, ...) {
    va_list args;
    va_start(args, calltype);
    int ret = 0;
    switch (calltype) {
        case CELESTE_P8_BTN: {
            int b = va_arg(args, int);
            if (b >= 0 && b < 6) ret = sim_buttons[b];
            break;
        }
        case CELESTE_P8_MGET: {
            int tx = va_arg(args, int);
            int ty = va_arg(args, int);
            if (tx >= 0 && tx < 128 && ty >= 0 && ty < 64)
                ret = tilemap_data[tx + ty * 128];
            break;
        }
        case CELESTE_P8_FGET: {
            int tile = va_arg(args, int);
            int flag = va_arg(args, int);
            ret = !!(tile_flags[tile] & (1 << flag));
            break;
        }
        default: break;
    }
    va_end(args);
    return ret;
}

TEST(test_settles_on_ground_no_crash) {
    Celeste_P8_set_call_func(fake_p8_callback);
    Celeste_P8_set_rndseed(1);
    memset(sim_buttons, 0, sizeof(sim_buttons));
    Celeste_P8_init();
    /* Celeste_P8_init() lands on the title screen (room 7,3), where no player
     * object exists until a button press starts the game. debug_load_room is
     * the public seam tools/celeste_train.c already uses to jump straight into
     * gameplay for exactly this reason. */
    Celeste_P8_debug_load_room(0, 0);

    float px, py, vx, vy;
    int djump, room_x, room_y;
    bool on_ground, wall_left, wall_right;
    bool settled = false;

    for (int i = 0; i < 2000; i++) {
        Celeste_P8_update();
        get_celeste_player_state(&px, &py, &vx, &vy, &djump,
                                  &on_ground, &wall_left, &wall_right, &room_x, &room_y);
        CHECK(room_x >= 0 && room_x < 8);
        CHECK(room_y >= 0 && room_y < 8);
        if (i > 200 && on_ground) settled = true;
    }
    /* Spawn room has floor tiles, so with no input the player should reach solid
     * ground at some point after settling in; the death/respawn cycle can leave
     * it briefly airborne at any single sampled instant, so check over the window
     * rather than requiring on_ground at the exact final frame. */
    CHECK(settled);
}

TEST(test_draw_no_crash) {
    Celeste_P8_set_call_func(fake_p8_callback);
    Celeste_P8_set_rndseed(1);
    memset(sim_buttons, 0, sizeof(sim_buttons));
    Celeste_P8_init();
    Celeste_P8_debug_load_room(0, 0);
    for (int i = 0; i < 30; i++) Celeste_P8_update();
    Celeste_P8_draw(); /* must not crash; draws via the callback, not a VGA buffer */
}

TEST(test_save_load_round_trip) {
    Celeste_P8_set_call_func(fake_p8_callback);
    Celeste_P8_set_rndseed(1);
    memset(sim_buttons, 0, sizeof(sim_buttons));
    Celeste_P8_init();
    Celeste_P8_debug_load_room(0, 0);
    for (int i = 0; i < 100; i++) Celeste_P8_update();

    size_t sz = Celeste_P8_get_state_size();
    CHECK(sz > 0);
    void *state_a = malloc(sz);
    void *state_b = malloc(sz);
    CHECK(state_a && state_b);

    Celeste_P8_save_state(state_a);

    for (int i = 0; i < 200; i++) Celeste_P8_update(); /* diverge state */

    Celeste_P8_load_state(state_a);
    Celeste_P8_save_state(state_b);

    CHECK(memcmp(state_a, state_b, sz) == 0);

    free(state_a);
    free(state_b);
}

int main(void) {
    RUN(test_settles_on_ground_no_crash);
    RUN(test_draw_no_crash);
    RUN(test_save_load_round_trip);
    return TEST_SUMMARY();
}
