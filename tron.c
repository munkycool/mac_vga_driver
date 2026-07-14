#include "common.h"

#define GRID_W  160
#define GRID_H  120

// Game State Machine
typedef enum {
    TRON_STATE_READY,
    TRON_STATE_PLAYING,
    TRON_STATE_CRASH
} TronState;

static TronState current_state = TRON_STATE_READY;
static int state_timer = 0;
static int win_player = 0; // 0 = Tie, 1 = P1 wins, 2 = P2 wins

// Grid and Bike states
static uint8_t tron_grid[GRID_H][GRID_W];
static int p1_x, p1_y, p1_dir;
static int p2_x, p2_y, p2_dir;
static int tron_score_p1, tron_score_p2;

// Boost and speed state
static int p1_boost = 100;
static int p2_boost = 100;
static bool p1_is_boosting = false;
static bool p2_is_boosting = false;
static int p1_move_timer = 0;
static int p2_move_timer = 0;

// Directions: 0 = Up, 1 = Right, 2 = Down, 3 = Left
static const int DIRS_X[4] = {0, 1, 0, -1};
static const int DIRS_Y[4] = {-1, 0, 1, 0};

// Explosion Particle System
#define MAX_PARTICLES 40
typedef struct {
    float x, y;
    float vx, vy;
    uint8_t color;
    int lifetime;
} Particle;

static Particle particles[MAX_PARTICLES];
static int particle_count = 0;

static void spawn_explosion(int x, int y, uint8_t base_color) {
    particle_count = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].x = x;
        particles[i].y = y;
        // Random velocities in [-2.0, 2.0]
        particles[i].vx = ((get_rand() % 401) - 200) / 100.0f;
        particles[i].vy = ((get_rand() % 401) - 200) / 100.0f;
        
        int r = get_rand() % 3;
        if (r == 0) particles[i].color = base_color;
        else if (r == 1) particles[i].color = 3; // Yellow
        else particles[i].color = 7; // White
        
        particles[i].lifetime = 30 + (get_rand() % 30);
        particle_count++;
    }
}

static void update_particles() {
    for (int i = 0; i < particle_count; i++) {
        if (particles[i].lifetime > 0) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vx *= 0.96f;
            particles[i].vy *= 0.96f;
            particles[i].lifetime--;
        }
    }
}

static void draw_particles(uint8_t *buffer) {
    for (int i = 0; i < particle_count; i++) {
        if (particles[i].lifetime > 0) {
            draw_pixel(buffer, (int)particles[i].x, (int)particles[i].y, particles[i].color);
        }
    }
}

static void reset_round() {
    memset(tron_grid, 0, sizeof(tron_grid));

    // Player 1 (Blue) starts left moving right
    p1_x = 20;
    p1_y = 60;
    p1_dir = 1; 
    tron_grid[p1_y][p1_x] = 1;

    // Player 2 (Red) starts right moving left
    p2_x = 140;
    p2_y = 60;
    p2_dir = 3;
    tron_grid[p2_y][p2_x] = 2;

    p1_boost = 100;
    p2_boost = 100;
    p1_is_boosting = false;
    p2_is_boosting = false;
    p1_move_timer = 0;
    p2_move_timer = 0;

    current_state = TRON_STATE_READY;
    state_timer = 90;
    win_player = 0;
    particle_count = 0;
}

void init_tron() {
    tron_score_p1 = 0;
    tron_score_p2 = 0;
    reset_round();
}

static bool is_blocked(int x, int y) {
    if (x < 2 || x >= GRID_W - 2 || y < 2 || y >= GRID_H - 2) return true;
    return (tron_grid[y][x] != 0);
}

// Simple AI to look ahead and count open spaces
static int lookahead_space(int sx, int sy, int sdir) {
    int count = 0;
    int cx = sx + DIRS_X[sdir];
    int cy = sy + DIRS_Y[sdir];
    while (cx >= 0 && cx < GRID_W && cy >= 0 && cy < GRID_H && tron_grid[cy][cx] == 0) {
        count++;
        cx += DIRS_X[sdir];
        cy += DIRS_Y[sdir];
        if (count >= 25) break; // Look ahead max 25 steps
    }
    return count;
}

static void update_bike_ai(int *px, int *py, int *pdir, bool *is_boosting, int *boost_val) {
    int cx = *px;
    int cy = *py;
    int dir = *pdir;

    int next_x = cx + DIRS_X[dir];
    int next_y = cy + DIRS_Y[dir];

    // Check if we need to turn (either blocked ahead, or small random steer)
    bool need_turn = is_blocked(next_x, next_y) || (get_rand() % 100 < 3);

    if (need_turn) {
        int left_dir = (dir + 3) % 4;
        int right_dir = (dir + 1) % 4;

        int left_space = lookahead_space(cx, cy, left_dir);
        int right_space = lookahead_space(cx, cy, right_dir);
        int straight_space = lookahead_space(cx, cy, dir);

        if (need_turn && is_blocked(next_x, next_y)) {
            // Must turn! Choose side with most space
            if (left_space > right_space) {
                *pdir = left_dir;
            } else if (right_space > left_space) {
                *pdir = right_dir;
            } else {
                // equal space or both blocked, turn to whichever is open
                if (left_space > 0) *pdir = left_dir;
                else if (right_space > 0) *pdir = right_dir;
            }
        } else {
            // Optional strategic turn: turn if side is much more open than straight ahead
            if (left_space > straight_space + 6 && left_space >= right_space) {
                *pdir = left_dir;
            } else if (right_space > straight_space + 6 && right_space >= left_space) {
                *pdir = right_dir;
            }
        }
    }

    // AI Boost logic
    int straight_line = lookahead_space(cx, cy, *pdir);
    if (straight_line > 20 && *boost_val > 20 && (get_rand() % 100 < 10)) {
        *is_boosting = true;
    }

    if (*is_boosting) {
        if (*boost_val > 0) {
            (*boost_val)--;
            if (straight_line < 5) {
                *is_boosting = false;
            }
        } else {
            *is_boosting = false;
        }
    } else {
        if (*boost_val < 100) {
            (*boost_val)++;
        }
    }
}

static void draw_background_grid(uint8_t *buffer) {
    for (int y = 4; y < GRID_H - 4; y += 8) {
        for (int x = 4; x < GRID_W - 4; x += 8) {
            draw_pixel(buffer, x * 2, y * 2, 4); // Blue dot
        }
    }
}

void play_tron(uint8_t *buffer) {
    clear_screen(buffer);

    // 1. Draw Background Grid
    draw_background_grid(buffer);

    // 2. Draw Scores and Boost Bars
    draw_score(buffer, tron_score_p1, 100, 10, 2, 4);
    draw_score(buffer, tron_score_p2, 180, 10, 2, 1);

    // P1 boost bar
    draw_rect(buffer, 100, 24, 130, 26, 7); // frame
    if (p1_boost > 0) {
        draw_rect(buffer, 101, 25, 101 + (p1_boost * 28 / 100), 25, 4); // fill Blue
    }

    // P2 boost bar
    draw_rect(buffer, 180, 24, 210, 26, 7); // frame
    if (p2_boost > 0) {
        draw_rect(buffer, 181, 25, 181 + (p2_boost * 28 / 100), 25, 1); // fill Red
    }

    // 3. Draw Arena Borders (White 7)
    draw_rect(buffer, 0, 0, 319, 3, 7); // top
    draw_rect(buffer, 0, 236, 319, 239, 7); // bottom
    draw_rect(buffer, 0, 0, 3, 239, 7); // left
    draw_rect(buffer, 316, 0, 319, 239, 7); // right

    // 4. Update Game States
    if (current_state == TRON_STATE_READY) {
        state_timer--;
        if (state_timer > 30) {
            draw_string(buffer, "READY", 130, 100, 3, 3); // Yellow
        } else {
            draw_string(buffer, "GO!", 142, 100, 3, 2); // Green
        }
        if (state_timer == 0) {
            current_state = TRON_STATE_PLAYING;
            p1_move_timer = 0;
            p2_move_timer = 0;
        }
    } else if (current_state == TRON_STATE_CRASH) {
        state_timer--;
        update_particles();
        draw_particles(buffer);

        if (win_player == 1) {
            draw_string(buffer, "PLAYER 1 WINS!", 76, 100, 2, 6); // Cyan
        } else if (win_player == 2) {
            draw_string(buffer, "PLAYER 2 WINS!", 76, 100, 2, 3); // Yellow
        } else {
            draw_string(buffer, "TIE GAME!", 104, 100, 2, 7); // White
        }

        if (state_timer == 0) {
            reset_round();
            return;
        }
    } else if (current_state == TRON_STATE_PLAYING) {
        // Player Input / AI updates
        if (global_input.interactive) {
            if (global_input.up && p1_dir != 2) p1_dir = 0;
            else if (global_input.right && p1_dir != 3) p1_dir = 1;
            else if (global_input.down && p1_dir != 0) p1_dir = 2;
            else if (global_input.left && p1_dir != 1) p1_dir = 3;

            if (global_input.action1 && p1_boost > 0) {
                p1_is_boosting = true;
                p1_boost--;
            } else {
                p1_is_boosting = false;
                if (p1_boost < 100) p1_boost++;
            }
        } else {
            p1_is_boosting = false;
            if (p1_boost < 100) p1_boost++;
            update_bike_ai(&p1_x, &p1_y, &p1_dir, &p1_is_boosting, &p1_boost);
        }

        // P2 is always AI
        update_bike_ai(&p2_x, &p2_y, &p2_dir, &p2_is_boosting, &p2_boost);

        // Move timers
        p1_move_timer++;
        p2_move_timer++;

        int p1_threshold = p1_is_boosting ? 2 : 4;
        int p2_threshold = p2_is_boosting ? 2 : 4;

        bool p1_should_move = (p1_move_timer >= p1_threshold);
        bool p2_should_move = (p2_move_timer >= p2_threshold);

        if (p1_should_move) {
            p1_x += DIRS_X[p1_dir];
            p1_y += DIRS_Y[p1_dir];
            p1_move_timer = 0;
        }
        if (p2_should_move) {
            p2_x += DIRS_X[p2_dir];
            p2_y += DIRS_Y[p2_dir];
            p2_move_timer = 0;
        }

        // Collision Check
        bool p1_died = false;
        bool p2_died = false;

        if (p1_should_move && is_blocked(p1_x, p1_y)) p1_died = true;
        if (p2_should_move && is_blocked(p2_x, p2_y)) p2_died = true;

        if (p1_x == p2_x && p1_y == p2_y) {
            p1_died = true;
            p2_died = true;
        }

        if (p1_died || p2_died) {
            current_state = TRON_STATE_CRASH;
            state_timer = 90;
            if (p1_died && p2_died) {
                win_player = 0;
                spawn_explosion(p1_x * 2, p1_y * 2, 4);
                spawn_explosion(p2_x * 2, p2_y * 2, 1);
            } else if (p1_died) {
                win_player = 2;
                tron_score_p2++;
                spawn_explosion(p1_x * 2, p1_y * 2, 4);
            } else {
                win_player = 1;
                tron_score_p1++;
                spawn_explosion(p2_x * 2, p2_y * 2, 1);
            }
            return;
        }

        // Write trail cells
        if (p1_should_move) {
            tron_grid[p1_y][p1_x] = 1;
        }
        if (p2_should_move) {
            tron_grid[p2_y][p2_x] = 2;
        }
    }

    // 5. Draw Trails from Grid
    for (int y = 2; y < GRID_H - 2; y++) {
        for (int x = 2; x < GRID_W - 2; x++) {
            uint8_t cell = tron_grid[y][x];
            if (cell == 1) {
                draw_rect(buffer, x * 2, y * 2, x * 2 + 1, y * 2 + 1, 4); // Blue 4
            } else if (cell == 2) {
                draw_rect(buffer, x * 2, y * 2, x * 2 + 1, y * 2 + 1, 1); // Red 1
            }
        }
    }

    // 6. Draw Cycle Heads
    // Only draw heads if they are alive (especially in crash state)
    bool draw_p1 = true;
    bool draw_p2 = true;
    if (current_state == TRON_STATE_CRASH) {
        if (win_player == 0) {
            draw_p1 = false;
            draw_p2 = false;
        } else if (win_player == 2) {
            draw_p1 = false; // P1 crashed
        } else if (win_player == 1) {
            draw_p2 = false; // P2 crashed
        }
    }

    if (draw_p1) {
        draw_rect(buffer, p1_x * 2 - 1, p1_y * 2 - 1, p1_x * 2 + 2, p1_y * 2 + 2, 6); // Cyan
        draw_rect(buffer, p1_x * 2, p1_y * 2, p1_x * 2 + 1, p1_y * 2 + 1, 7); // White
    }

    if (draw_p2) {
        draw_rect(buffer, p2_x * 2 - 1, p2_y * 2 - 1, p2_x * 2 + 2, p2_y * 2 + 2, 3); // Yellow
        draw_rect(buffer, p2_x * 2, p2_y * 2, p2_x * 2 + 1, p2_y * 2 + 1, 7); // White
    }
}
