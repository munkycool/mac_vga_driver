#include "common.h"

static int ball_x = 160, ball_y = 120;
static int ball_dx = 3, ball_dy = 2;
static int paddle1_y = 105, paddle2_y = 105;
static int pong_score_left = 0;
static int pong_score_right = 0;
static int paddle1_error = 0;
static int paddle2_error = 0;
static int pong_error_timer = 0;

void init_pong() {
    ball_x = 160; ball_y = 120;
    ball_dx = 3; ball_dy = 2;
    paddle1_y = 105; paddle2_y = 105;
    pong_score_left = 0; pong_score_right = 0;
    paddle1_error = 0; paddle2_error = 0;
    pong_error_timer = 0;
}

void play_pong(uint8_t *buffer) {
    clear_screen(buffer);
    draw_score(buffer, pong_score_left, 100, 10, 2, 7);
    draw_score(buffer, pong_score_right, 180, 10, 2, 7);
    
    for (int y = 0; y < 240; y += 12) {
        draw_rect(buffer, 159, y, 160, y + 6, 7);
    }
    
    ball_x += ball_dx;
    ball_y += ball_dy;
    
    if (ball_y <= 5 || ball_y >= 235) {
        ball_dy = -ball_dy;
    }
    
    pong_error_timer++;
    if (pong_error_timer > 45) {
        pong_error_timer = 0;
        paddle1_error = (get_rand() % 70) - 35;
        paddle2_error = (get_rand() % 70) - 35;
    }
    
    int target1 = ball_y - 15 + paddle1_error;
    if (paddle1_y < target1) paddle1_y += 2;
    else if (paddle1_y > target1) paddle1_y -= 2;
    
    int target2 = ball_y - 15 + paddle2_error;
    if (paddle2_y < target2) paddle2_y += 2;
    else if (paddle2_y > target2) paddle2_y -= 2;
    
    if (paddle1_y < 5) paddle1_y = 5;
    if (paddle1_y > 205) paddle1_y = 205;
    if (paddle2_y < 5) paddle2_y = 5;
    if (paddle2_y > 205) paddle2_y = 205;
    
    if (ball_x <= 20 && ball_x >= 14 && ball_y >= paddle1_y - 4 && ball_y <= paddle1_y + 34) {
        ball_dx = -ball_dx;
        ball_x = 21;
    }
    if (ball_x >= 294 && ball_x <= 300 && ball_y >= paddle2_y - 4 && ball_y <= paddle2_y + 34) {
        ball_dx = -ball_dx;
        ball_x = 293;
    }
    
    if (ball_x < 0) {
        pong_score_right++;
        ball_x = 160; ball_y = 120;
        ball_dx = 3;
    } else if (ball_x > 320) {
        pong_score_left++;
        ball_x = 160; ball_y = 120;
        ball_dx = -3;
    }
    
    draw_rect(buffer, 10, paddle1_y, 14, paddle1_y + 30, 7);
    draw_rect(buffer, 305, paddle2_y, 309, paddle2_y + 30, 7);
    draw_rect(buffer, ball_x - 2, ball_y - 2, ball_x + 2, ball_y + 2, 7);
}
