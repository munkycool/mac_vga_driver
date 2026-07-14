#include "common.h"

#define DOWNLOADS_FRAME_INTERVAL 67

static int current_second = -1;
static size_t current_image_index = 0;

void init_downloads() {
    current_second = -1;
    current_image_index = 0;
}

static size_t pick_next_image_index() {
    if (downloads_image_count <= 1) {
        return 0;
    }

    size_t next_index = get_rand() % downloads_image_count;
    if (next_index == current_image_index) {
        next_index = (next_index + 1 + (get_rand() % (downloads_image_count - 1))) % downloads_image_count;
    }

    return next_index;
}

void play_downloads(uint8_t *buffer, int frame_counter) {
    if (downloads_image_count == 0) {
        clear_screen(buffer);
        return;
    }

    int second_bucket = frame_counter / DOWNLOADS_FRAME_INTERVAL;
    if (second_bucket != current_second) {
        current_second = second_bucket;
        current_image_index = pick_next_image_index();
    }

    memcpy(buffer, downloads_images[current_image_index], DOWNLOADS_FRAME_BYTES);
}