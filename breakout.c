#include "common.h"

#define PADDLE_W        40
#define PADDLE_H        6
#define PADDLE_Y        215

#define BRICK_ROWS      5
#define BRICK_COLS      10
#define BRICK_W         28
#define BRICK_H         8
#define BRICK_GAP       2
#define BRICK_START_X   11
#define BRICK_START_Y   40

static bool bricks[BRICK_ROWS][BRICK_COLS];
static int paddle_x;
static int ball_x, ball_y;
static int ball_dx, ball_dy;
static int breakout_score;

static void reset_bricks() {
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            bricks[r][c] = true;
        }
    }
}

void init_breakout() {
    paddle_x = 140;
    ball_x = 160;
    ball_y = 120;
    ball_dx = (get_rand() % 2 == 0) ? 2 : -2;
    ball_dy = 2;
    breakout_score = 0;
    reset_bricks();
}

void play_breakout(uint8_t *buffer) {
    clear_screen(buffer);

    // 1. Draw UI Score
    draw_score(buffer, breakout_score, 10, 10, 2, 7);

    // 2. Draw Borders (White 7)
    draw_rect(buffer, 5, 25, 10, 239, 7); // left wall
    draw_rect(buffer, 310, 25, 315, 239, 7); // right wall
    draw_rect(buffer, 5, 25, 315, 30, 7); // top wall

    // 3. AI Paddle Movement
    int target_x = ball_x - (PADDLE_W / 2);
    int p_speed = 3; // speed limit to make it look smooth
    if (paddle_x < target_x) {
        paddle_x += p_speed;
        if (paddle_x > target_x) paddle_x = target_x;
    } else if (paddle_x > target_x) {
        paddle_x -= p_speed;
        if (paddle_x < target_x) paddle_x = target_x;
    }

    // Keep paddle within walls
    if (paddle_x < 11) paddle_x = 11;
    if (paddle_x > 310 - PADDLE_W) paddle_x = 310 - PADDLE_W;

    // 4. Ball Physics & Movement
    ball_x += ball_dx;
    ball_y += ball_dy;

    // Wall bounces
    if (ball_x <= 12) {
        ball_x = 13;
        ball_dx = -ball_dx;
    }
    if (ball_x >= 308) {
        ball_x = 307;
        ball_dx = -ball_dx;
    }
    if (ball_y <= 32) {
        ball_y = 33;
        ball_dy = -ball_dy;
    }

    // Bottom out (Death)
    if (ball_y >= 238) {
        ball_x = 160;
        ball_y = 120;
        ball_dx = (get_rand() % 2 == 0) ? 2 : -2;
        ball_dy = -2;
        // slightly reduce score on death, don't go below 0
        breakout_score = (breakout_score >= 100) ? breakout_score - 100 : 0;
    }

    // Paddle collision
    if (ball_y >= PADDLE_Y - 3 && ball_y <= PADDLE_Y + 1 && ball_dy > 0) {
        if (ball_x >= paddle_x && ball_x <= paddle_x + PADDLE_W) {
            ball_dy = -ball_dy;
            ball_y = PADDLE_Y - 4;
            // Angle change based on hit position
            int hit_pos = ball_x - (paddle_x + PADDLE_W / 2);
            ball_dx = hit_pos / 4;
            if (ball_dx == 0) ball_dx = (get_rand() % 2 == 0) ? 1 : -1;
            if (ball_dx > 4) ball_dx = 4;
            if (ball_dx < -4) ball_dx = -4;
        }
    }

    // Brick collisions
    // Brick area y limits: BRICK_START_Y to BRICK_START_Y + BRICK_ROWS * 10
    if (ball_y >= BRICK_START_Y && ball_y < BRICK_START_Y + BRICK_ROWS * (BRICK_H + BRICK_GAP)) {
        int r = (ball_y - BRICK_START_Y) / (BRICK_H + BRICK_GAP);
        int c = (ball_x - BRICK_START_X) / (BRICK_W + BRICK_GAP);
        if (r >= 0 && r < BRICK_ROWS && c >= 0 && c < BRICK_COLS) {
            if (bricks[r][c]) {
                bricks[r][c] = false;
                ball_dy = -ball_dy;
                breakout_score += (BRICK_ROWS - r) * 20;

                // Check if all bricks are cleared
                bool any_left = false;
                for (int row = 0; row < BRICK_ROWS; row++) {
                    for (int col = 0; col < BRICK_COLS; col++) {
                        if (bricks[row][col]) any_left = true;
                    }
                }
                if (!any_left) {
                    reset_bricks();
                }
            }
        }
    }

    // 5. Draw Bricks
    // Row colors: 0=Red(1), 1=Magenta(5), 2=Yellow(3), 3=Green(2), 4=Cyan(6)
    uint8_t row_colors[BRICK_ROWS] = {1, 5, 3, 2, 6};
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (bricks[r][c]) {
                int bx = BRICK_START_X + c * (BRICK_W + BRICK_GAP);
                int by = BRICK_START_Y + r * (BRICK_H + BRICK_GAP);
                draw_rect(buffer, bx, by, bx + BRICK_W - 1, by + BRICK_H - 1, row_colors[r]);
                // Highlight top/left
                draw_rect(buffer, bx, by, bx + BRICK_W - 1, by, 7);
                draw_rect(buffer, bx, by, bx, by + BRICK_H - 1, 7);
            }
        }
    }

    // 6. Draw Paddle (White 7)
    draw_rect(buffer, paddle_x, PADDLE_Y, paddle_x + PADDLE_W, PADDLE_Y + PADDLE_H - 1, 7);

    // 7. Draw Ball (White 7, 3x3)
    draw_rect(buffer, ball_x - 1, ball_y - 1, ball_x + 1, ball_y + 1, 7);
}
