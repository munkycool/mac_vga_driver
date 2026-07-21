/* Host runtime for the test binaries: plays the same role emulator/sdl_emulator.c
 * plays for the SDL emulator, but with a controllable fake clock and no SDL/video
 * output. Reuses emulator/stubs/ (pico/hardware headers) unmodified. */
#include "../firmware/core/common.h"

dma_hw_t dma_hw_inst = {0};
const pio_program_t vga_program = {0};

static uint64_t fake_now_us = 0;

void test_set_fake_time_us(uint64_t t) { fake_now_us = t; }
void test_advance_fake_time_us(uint64_t dt) { fake_now_us += dt; }

uint64_t time_us_64(void) { return fake_now_us; }
int getchar_timeout_us(uint32_t timeout_us) { (void)timeout_us; return PICO_ERROR_TIMEOUT; }
void tight_loop_contents(void) {}

/* No bad_apple.bin on the host: alias bad_apple_bin_end onto bad_apple_bin so
 * total_frames = (bad_apple_bin_end - bad_apple_bin) / 1536 is deterministically
 * 0, exercising video.c's documented "no data" guard path instead of fabricating
 * frame bytes with no defined relative layout to a second array. */
const uint8_t bad_apple_bin[1] = {0};
extern const uint8_t bad_apple_bin_end[1] __attribute__((alias("bad_apple_bin")));

/* Synthetic "no photos found" downloads asset: exercises play_downloads()'s
 * count==0 guard deterministically, without touching the user's real
 * ~/Downloads folder or needing the Python asset generator. */
const size_t downloads_image_count = 0;
const uint8_t downloads_images[1][DOWNLOADS_FRAME_BYTES];
