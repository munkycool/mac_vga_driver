#include "common.h"

// 16-direction angle lookup tables (scaled by 128)
static const int COS16[16] = {128, 118, 90, 49, 0, -49, -90, -118, -128, -118, -90, -49, 0, 49, 90, 118};
static const int SIN16[16] = {0, 49, 90, 118, 128, 118, 90, 49, 0, -49, -90, -118, -128, -118, -90, -49};

#define MAX_ASTEROIDS   12
#define MAX_BULLETS     6

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

static float ship_x, ship_y;
static float ship_vx, ship_vy;

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
    ship_x = 160.0f;
    ship_y = 120.0f;
    ship_vx = 0.0f;
    ship_vy = 0.0f;

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

    // Update ship physics
    ship_x += ship_vx;
    ship_y += ship_vy;
    ship_vx *= 0.95f;
    ship_vy *= 0.95f;

    // Screen Wrap
    if (ship_x < 0) ship_x += 320;
    if (ship_x >= 320) ship_x -= 320;
    if (ship_y < 0) ship_y += 240;
    if (ship_y >= 240) ship_y -= 240;

    int SHIP_X = (int)ship_x;
    int SHIP_Y = (int)ship_y;

    // 2. Find Nearest Asteroid for AI Targeting & Threat Avoidance
    int nearest_idx = -1;
    float min_dist = 999999.0f;
    float repel_x = 0.0f;
    float repel_y = 0.0f;
    bool under_threat = false;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            float dx = (float)(asteroids[i].x - SHIP_X);
            float dy = (float)(asteroids[i].y - SHIP_Y);
            
            // Screen wrap distance correction
            if (dx > 160) dx -= 320;
            if (dx < -160) dx += 320;
            if (dy > 120) dy -= 240;
            if (dy < -120) dy += 240;

            float dist = isqrt((int)(dx * dx + dy * dy));
            if (dist < 1.0f) dist = 1.0f;

            if (dist < min_dist) {
                min_dist = dist;
                nearest_idx = i;
            }

            // Threat repulsion: if within 55 pixels, push away
            if (dist < 55.0f) {
                under_threat = true;
                float force = (55.0f - dist) / dist;
                repel_x -= dx * force;
                repel_y -= dy * force;
            }
        }
    }

    // Determine target vector
    float target_dx = 0.0f;
    float target_dy = 0.0f;
    if (under_threat) {
        target_dx = repel_x;
        target_dy = repel_y;
    } else if (nearest_idx != -1) {
        // Hunt the nearest asteroid
        float dx = (float)(asteroids[nearest_idx].x - SHIP_X);
        float dy = (float)(asteroids[nearest_idx].y - SHIP_Y);
        if (dx > 160) dx -= 320;
        if (dx < -160) dx += 320;
        if (dy > 120) dy -= 240;
        if (dy < -120) dy += 240;
        
        target_dx = dx;
        target_dy = dy;
    }

    bool thrusting = false;

    if (nearest_idx != -1) {
        // Find angle that maximizes dot product with target vector
        int best_angle = 0;
        float max_dot = -9999999.0f;
        for (int a = 0; a < 16; a++) {
            float dot = target_dx * COS16[a] + target_dy * SIN16[a];
            if (dot > max_dot) {
                max_dot = dot;
                best_angle = a;
            }
        }

        // Rotate ship towards best_angle
        if (ship_angle != best_angle) {
            int diff = (best_angle - ship_angle + 16) % 16;
            if (diff < 8) {
                ship_angle = (ship_angle + 1) % 16;
            } else {
                ship_angle = (ship_angle - 1 + 16) % 16;
            }
        }

        // Apply thrust if aligned
        int angle_diff = abs(ship_angle - best_angle);
        if (angle_diff > 8) angle_diff = 16 - angle_diff;
        
        if (angle_diff <= 2) {
            if (under_threat) {
                // Flee!
                ship_vx += (COS16[ship_angle] / 128.0f) * 0.45f;
                ship_vy += (SIN16[ship_angle] / 128.0f) * 0.45f;
                thrusting = true;
            } else if (min_dist > 80.0f) {
                // Move closer to target
                ship_vx += (COS16[ship_angle] / 128.0f) * 0.25f;
                ship_vy += (SIN16[ship_angle] / 128.0f) * 0.25f;
                thrusting = true;
            }
        }

        // Shoot if aligned with the actual nearest asteroid
        float ast_dx = (float)(asteroids[nearest_idx].x - SHIP_X);
        float ast_dy = (float)(asteroids[nearest_idx].y - SHIP_Y);
        if (ast_dx > 160) ast_dx -= 320;
        if (ast_dx < -160) ast_dx += 320;
        if (ast_dy > 120) ast_dy -= 240;
        if (ast_dy < -120) ast_dy += 240;
        
        int ast_angle = 0;
        float max_ast_dot = -9999999.0f;
        for (int a = 0; a < 16; a++) {
            float dot = ast_dx * COS16[a] + ast_dy * SIN16[a];
            if (dot > max_ast_dot) {
                max_ast_dot = dot;
                ast_angle = a;
            }
        }

        int ast_angle_diff = abs(ship_angle - ast_angle);
        if (ast_angle_diff > 8) ast_angle_diff = 16 - ast_angle_diff;

        if (ast_angle_diff <= 1 && shoot_cooldown <= 0) {
            // Spawn bullet
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = true;
                    bullets[i].x = SHIP_X + (COS16[ship_angle] * 6) / 128;
                    bullets[i].y = SHIP_Y + (SIN16[ship_angle] * 6) / 128;
                    bullets[i].dx = (COS16[ship_angle] * 4) / 128 + (int)ship_vx;
                    bullets[i].dy = (SIN16[ship_angle] * 4) / 128 + (int)ship_vy;
                    bullets[i].life = 45;
                    shoot_cooldown = 12; // cooldown
                    break;
                }
            }
        }
    }

    if (shoot_cooldown > 0) shoot_cooldown--;

    // Draw flickering thrust flame
    if (thrusting && (get_rand() % 2 == 0)) {
        int back_angle = (ship_angle + 8) % 16;
        int fx = SHIP_X + (COS16[back_angle] * 6) / 128;
        int fy = SHIP_Y + (SIN16[back_angle] * 6) / 128;
        draw_rect(buffer, fx - 1, fy - 1, fx + 1, fy + 1, 3); // orange/yellow flame
    }

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
