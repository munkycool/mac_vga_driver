#include "common.h"

#define GRID_W      20
#define GRID_H      15
#define CELL_SZ     12
#define OFFSET_X    40
#define OFFSET_Y    30

static int snake_x[300];
static int snake_y[300];
static int snake_len = 0;
static int s_dx = 1, s_dy = 0;
static int apple_x = 0, apple_y = 0;
static int snake_score = 0;
static int move_delay_counter = 0;

static void spawn_apple() {
    bool valid = false;
    while (!valid) {
        apple_x = get_rand() % GRID_W;
        apple_y = get_rand() % GRID_H;
        valid = true;
        for (int i = 0; i < snake_len; i++) {
            if (snake_x[i] == apple_x && snake_y[i] == apple_y) {
                valid = false;
                break;
            }
        }
    }
}

void init_snake() {
    snake_len = 5;
    for (int i = 0; i < snake_len; i++) {
        snake_x[i] = 10 - i;
        snake_y[i] = 7;
    }
    s_dx = 1;
    s_dy = 0;
    snake_score = 0;
    move_delay_counter = 0;
    spawn_apple();
}

static void snake_run_ai() {
    int dirs[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}}; // Up, Right, Down, Left
    int best_dir = -1;
    int min_dist = 99999;

    for (int i = 0; i < 4; i++) {
        int nx = snake_x[0] + dirs[i][0];
        int ny = snake_y[0] + dirs[i][1];
        
        // Cannot reverse directly into neck segment
        if (dirs[i][0] == -s_dx && dirs[i][1] == -s_dy) continue;

        // Check wall collision
        if (nx < 0 || nx >= GRID_W || ny < 0 || ny >= GRID_H) continue;

        // Check self collision
        bool collides_self = false;
        for (int j = 0; j < snake_len - 1; j++) { // exclude tail end (it moves out of the way)
            if (snake_x[j] == nx && snake_y[j] == ny) {
                collides_self = true;
                break;
            }
        }
        if (collides_self) continue;

        // Calculate Manhattan distance to apple
        int dist = abs(nx - apple_x) + abs(ny - apple_y);
        if (dist < min_dist) {
            min_dist = dist;
            best_dir = i;
        }
    }

    // Backup: If no optimal direction, pick any valid direction
    if (best_dir == -1) {
        for (int i = 0; i < 4; i++) {
            int nx = snake_x[0] + dirs[i][0];
            int ny = snake_y[0] + dirs[i][1];
            if (nx >= 0 && nx < GRID_W && ny >= 0 && ny < GRID_H) {
                bool collides_self = false;
                for (int j = 0; j < snake_len; j++) {
                    if (snake_x[j] == nx && snake_y[j] == ny) {
                        collides_self = true;
                        break;
                    }
                }
                if (!collides_self) {
                    best_dir = i;
                    break;
                }
            }
        }
    }

    if (best_dir != -1) {
        s_dx = dirs[best_dir][0];
        s_dy = dirs[best_dir][1];
    }
}

void play_snake(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);

    // Draw score board
    draw_score(buffer, snake_score, 10, 10, 2, 7);

    // Draw boundary border box
    draw_rect(buffer, OFFSET_X - 2, OFFSET_Y - 2, OFFSET_X + GRID_W * CELL_SZ + 1, OFFSET_Y - 1, 7); // top
    draw_rect(buffer, OFFSET_X - 2, OFFSET_Y + GRID_H * CELL_SZ, OFFSET_X + GRID_W * CELL_SZ + 1, OFFSET_Y + GRID_H * CELL_SZ + 1, 7); // bottom
    draw_rect(buffer, OFFSET_X - 2, OFFSET_Y - 2, OFFSET_X - 1, OFFSET_Y + GRID_H * CELL_SZ + 1, 7); // left
    draw_rect(buffer, OFFSET_X + GRID_W * CELL_SZ, OFFSET_Y - 2, OFFSET_X + GRID_W * CELL_SZ + 1, OFFSET_Y + GRID_H * CELL_SZ + 1, 7); // right

    // Game speed tick (every 8 frames)
    move_delay_counter++;
    if (move_delay_counter >= 8) {
        move_delay_counter = 0;

        // Run pathfinder AI
        snake_run_ai();

        // Calculate next head position
        int next_x = snake_x[0] + s_dx;
        int next_y = snake_y[0] + s_dy;

        // Collision Check (Wall / Self)
        bool dead = false;
        if (next_x < 0 || next_x >= GRID_W || next_y < 0 || next_y >= GRID_H) dead = true;
        for (int i = 0; i < snake_len; i++) {
            if (snake_x[i] == next_x && snake_y[i] == next_y) dead = true;
        }

        if (dead) {
            init_snake();
            return;
        }

        // Check apple eating
        if (next_x == apple_x && next_y == apple_y) {
            snake_len++;
            snake_score += 10;
            // Shift snake segments back
            for (int i = snake_len - 1; i > 0; i--) {
                snake_x[i] = snake_x[i - 1];
                snake_y[i] = snake_y[i - 1];
            }
            snake_x[0] = next_x;
            snake_y[0] = next_y;
            spawn_apple();
        } else {
            // Normal shift movement
            for (int i = snake_len - 1; i > 0; i--) {
                snake_x[i] = snake_x[i - 1];
                snake_y[i] = snake_y[i - 1];
            }
            snake_x[0] = next_x;
            snake_y[0] = next_y;
        }
    }

    // Draw Apple (blinking glowing red)
    int ax = OFFSET_X + apple_x * CELL_SZ;
    int ay = OFFSET_Y + apple_y * CELL_SZ;
    uint8_t apple_color = (frame_counter % 16 < 8) ? 1 : 3; // Red <-> Yellow
    draw_rect(buffer, ax + 2, ay + 2, ax + CELL_SZ - 3, ay + CELL_SZ - 3, apple_color);
    draw_rect(buffer, ax + 4, ay + 1, ax + 5, ay + 2, 2); // green leaf

    // Draw Snake Body
    for (int i = 0; i < snake_len; i++) {
        int sx = OFFSET_X + snake_x[i] * CELL_SZ;
        int sy = OFFSET_Y + snake_y[i] * CELL_SZ;
        
        uint8_t color;
        if (i == 0) {
            color = 3; // Yellow Head
        } else {
            // Alternating green segments
            color = (i % 2 == 0) ? 2 : 6; // Green / Cyan body
        }
        draw_rect(buffer, sx + 1, sy + 1, sx + CELL_SZ - 2, sy + CELL_SZ - 2, color);
        // Head eyes
        if (i == 0) {
            draw_pixel(buffer, sx + 3, sy + 3, 0);
            draw_pixel(buffer, sx + 8, sy + 3, 0);
        }
    }
}
