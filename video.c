#include "common.h"

static uint32_t current_frame = 0;
static uint32_t total_frames = 0;
static uint64_t next_frame_time = 0;
static uint64_t frame_delay_us = 50000; // 50ms (20 FPS)

void draw_video_frame(uint8_t *buffer, const uint8_t *frame_data) {
    for (int y = 0; y < 240; y++) {
        int src_y = (y * 2) / 5; 
        const uint8_t *row_data = &frame_data[src_y * 16]; // 16 bytes per row for 1-bit color
        uint8_t *dest_row = &buffer[y * 320];
        
        for (int x = 0; x < 320; x++) {
            int src_x = (x * 2) / 5; 
            uint8_t b = row_data[src_x >> 3];
            int bit = (b >> (7 - (src_x & 7))) & 1;
            
            uint8_t color = bit ? 7 : 0;
            uint8_t pixel = color | 0x08; 
            dest_row[x] = (pixel << 4) | pixel;
        }
    }
}

void init_bad_apple() {
    frame_delay_us = 50000; // 20 FPS
    total_frames = (bad_apple_bin_end - bad_apple_bin) / 1536;
    if (total_frames > 0) {
        current_frame = get_rand() % total_frames;
    } else {
        current_frame = 0;
    }
    next_frame_time = time_us_64();
}

void play_video(uint8_t *buffer) {
    if (total_frames == 0) return;
    draw_video_frame(buffer, bad_apple_bin + ((current_frame % total_frames) * 1536));
}

void tick_video() {
    if (total_frames == 0) return;

    uint64_t now = time_us_64();
    if (now >= next_frame_time) {
        uint32_t frames_to_advance = (now - next_frame_time) / frame_delay_us + 1;
        current_frame = (current_frame + frames_to_advance) % total_frames;
        next_frame_time += frames_to_advance * frame_delay_us;
    }
}
