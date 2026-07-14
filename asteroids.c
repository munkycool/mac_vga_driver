#include "common.h"

// 16-direction angle lookup tables (scaled by 128)
static const int COS16[16] = {128, 118, 90, 49, 0, -49, -90, -118, -128, -118, -90, -49, 0, 49, 90, 118};
static const int SIN16[16] = {0, 49, 90, 118, 128, 118, 90, 49, 0, -49, -90, -118, -128, -118, -90, -49};

#define MAX_ASTEROIDS   12
#define MAX_BULLETS     6
#define SHIP_X          160
#define SHIP_Y          120

typedef struct {
    bool active;
    int x, y;
    int dx, dy;
    int size;   // 3 = Large, 2 = Medium, 1 = Small
} Asteroid;

typedef struct {
    bool active;
    int x, y;
    int dx, dy;
    int life;
} Bullet;

static Asteroid asteroids[MAX_ASTEROIDS];
static Bullet bullets[MAX_BULLETS];
static int ship_angle;
static int shoot_cooldown;
static int asteroids_score;

// Bresenham's Line Drawing Helper
static void draw_line(uint8_t *buffer, int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        draw_pixel(buffer, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

static void spawn_asteroid(int x, int y, int size) {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) {
            asteroids[i].active = true;
            asteroids[i].x = x;
            asteroids[i].y = y;
            // Ensure non-zero speed
            asteroids[i].dx = (get_rand() % 3) - 1;
            asteroids[i].dy = (get_rand() % 3) - 1;
            if (asteroids[i].dx == 0 && asteroids[i].dy == 0) {
                asteroids[i].dx = (get_rand() % 2 == 0) ? 1 : -1;
                asteroids[i].dy = (get_rand() % 2 == 0) ? 1 : -1;
            }
            asteroids[i].size = size;
            break;
        }
    }
}

void init_asteroids() {
    ship_angle = 0;
    shoot_cooldown = 0;
    asteroids_score = 0;

    for (int i = 0; i < MAX_ASTEROIDS; i++) asteroids[i].active = false;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;

    // Spawn initial large asteroids far from ship
    spawn_asteroid(30, 30, 3);
    spawn_asteroid(290, 30, 3);
    spawn_asteroid(30, 210, 3);
    spawn_asteroid(290, 210, 3);
}

void play_asteroids(uint8_t *buffer) {
    clear_screen(buffer);

    // 1. Draw Score
    draw_score(buffer, asteroids_score, 10, 10, 2, 7);

    // 2. Find Nearest Asteroid for AI Targeting
    int nearest_idx = -1;
    int min_dist_sq = 9999999;
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            int dx = asteroids[i].x - SHIP_X;
            int dy = asteroids[i].y - SHIP_Y;
            int dist_sq = dx * dx + dy * dy;
            if (dist_sq < min_dist_sq) {
                min_dist_sq = dist_sq;
                nearest_idx = i;
            }
        }
    }

    // AI logic: turn towards the nearest asteroid
    if (nearest_idx != -1) {
        int target_x = asteroids[nearest_idx].x;
        int target_y = asteroids[nearest_idx].y;
        int to_ast_x = target_x - SHIP_X;
        int to_ast_y = target_y - SHIP_Y;

        // Find angle that maximizes dot product (cos_a * to_ast_x + sin_a * to_ast_y)
        int best_angle = 0;
        int max_dot = -9999999;
        for (int a = 0; a < 16; a++) {
            int dot = to_ast_x * COS16[a] + to_ast_y * SIN16[a];
            if (dot > max_dot) {
                max_dot = dot;
                best_angle = a;
            }
        }

        // Slowly turn towards target angle
        if (ship_angle != best_angle) {
            // Find shortest turn direction
            int diff = (best_angle - ship_angle + 16) % 16;
            if (diff < 8) {
                ship_angle = (ship_angle + 1) % 16;
            } else {
                ship_angle = (ship_angle - 1 + 16) % 16;
            }
        }

        // Shoot if facing target
        if (ship_angle == best_angle && shoot_cooldown <= 0) {
            // Spawn bullet
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = true;
                    bullets[i].x = SHIP_X + (COS16[ship_angle] * 6) / 128;
                    bullets[i].y = SHIP_Y + (SIN16[ship_angle] * 6) / 128;
                    bullets[i].dx = (COS16[ship_angle] * 4) / 128;
                    bullets[i].dy = (SIN16[ship_angle] * 4) / 128;
                    bullets[i].life = 45; // lives for 45 frames
                    shoot_cooldown = 15;  // cooldown
                    break;
                }
            }
        }
    }

    if (shoot_cooldown > 0) shoot_cooldown--;

    // 3. Draw & Update Ship (White 7)
    int tip_x = SHIP_X + (COS16[ship_angle] * 8) / 128;
    int tip_y = SHIP_Y + (SIN16[ship_angle] * 8) / 128;
    int left_x = SHIP_X + (COS16[(ship_angle + 6) % 16] * 6) / 128;
    int left_y = SHIP_Y + (SIN16[(ship_angle + 6) % 16] * 6) / 128;
    int right_x = SHIP_X + (COS16[(ship_angle + 10) % 16] * 6) / 128;
    int right_y = SHIP_Y + (SIN16[(ship_angle + 10) % 16] * 6) / 128;

    draw_line(buffer, tip_x, tip_y, left_x, left_y, 7);
    draw_line(buffer, tip_x, tip_y, right_x, right_y, 7);
    draw_line(buffer, left_x, left_y, right_x, right_y, 7);

    // 4. Update & Draw Bullets (White 7)
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].x += bullets[i].dx;
            bullets[i].y += bullets[i].dy;
            bullets[i].life--;

            // Screen Wrap
            if (bullets[i].x < 0) bullets[i].x += 320;
            if (bullets[i].x >= 320) bullets[i].x -= 320;
            if (bullets[i].y < 0) bullets[i].y += 240;
            if (bullets[i].y >= 240) bullets[i].y -= 240;

            if (bullets[i].life <= 0) {
                bullets[i].active = false;
                continue;
            }

            // Draw bullet as 2x2 square
            draw_rect(buffer, bullets[i].x, bullets[i].y, bullets[i].x + 1, bullets[i].y + 1, 7);
        }
    }

    // 5. Update & Draw Asteroids
    bool any_active = false;
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            any_active = true;
            asteroids[i].x += asteroids[i].dx;
            asteroids[i].y += asteroids[i].dy;

            // Screen Wrap
            if (asteroids[i].x < -20) asteroids[i].x += 360;
            if (asteroids[i].x >= 340) asteroids[i].x -= 360;
            if (asteroids[i].y < -20) asteroids[i].y += 280;
            if (asteroids[i].y >= 260) asteroids[i].y -= 280;

            int rad = asteroids[i].size * 5;
            
            // Draw Asteroid (Red 1 body outline)
            int num_pts = 6;
            for (int p = 0; p < num_pts; p++) {
                int angle1 = (p * 16) / num_pts;
                int angle2 = (((p + 1) * 16) / num_pts) % 16;
                int x1 = asteroids[i].x + (COS16[angle1] * rad) / 128;
                int y1 = asteroids[i].y + (SIN16[angle1] * rad) / 128;
                int x2 = asteroids[i].x + (COS16[angle2] * rad) / 128;
                int y2 = asteroids[i].y + (SIN16[angle2] * rad) / 128;
                draw_line(buffer, x1, y1, x2, y2, 1);
            }

            // Collision: Ship hits Asteroid
            int sd_x = asteroids[i].x - SHIP_X;
            int sd_y = asteroids[i].y - SHIP_Y;
            if (sd_x * sd_x + sd_y * sd_y < (rad + 5) * (rad + 5)) {
                init_asteroids();
                return;
            }

            // Collision: Bullet hits Asteroid
            for (int b = 0; b < MAX_BULLETS; b++) {
                if (bullets[b].active) {
                    int bd_x = asteroids[i].x - bullets[b].x;
                    int bd_y = asteroids[i].y - bullets[b].y;
                    if (bd_x * bd_x + bd_y * bd_y < rad * rad) {
                        // Deactivate bullet
                        bullets[b].active = false;
                        asteroids[i].active = false;
                        
                        // Scoring
                        asteroids_score += (4 - asteroids[i].size) * 50;

                        // Split Asteroid
                        if (asteroids[i].size > 1) {
                            spawn_asteroid(asteroids[i].x, asteroids[i].y, asteroids[i].size - 1);
                            spawn_asteroid(asteroids[i].x, asteroids[i].y, asteroids[i].size - 1);
                        }
                        break;
                    }
                }
            }
        }
    }

    // Respawn new asteroids if all cleared
    if (!any_active) {
        spawn_asteroid(30, 30, 3);
        spawn_asteroid(290, 30, 3);
        spawn_asteroid(30, 210, 3);
        spawn_asteroid(290, 210, 3);
    }
}
