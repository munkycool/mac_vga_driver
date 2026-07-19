#include "common.h"

#define LANE_Y0     224 // Safe start
#define LANE_Y1     180 // Road 1
#define LANE_Y2     150 // Road 2
#define LANE_Y3     120 // Safe middle grass
#define LANE_Y4     90  // River 1
#define LANE_Y5     60  // River 2
#define LANE_Y6     30  // Destination Bays

static int frog_x = 160;
static int frog_y = LANE_Y0;
static int frog_score = 0;

static int car_x1[3] = {20, 130, 240}; // Road 1 (moves Left, vx = -2)
static int car_x2[3] = {10, 120, 230}; // Road 2 (moves Right, vx = 3)
static int log_x1[3] = {10, 120, 230}; // River 1 (moves Left, vx = -1)
static int log_x2[3] = {20, 140, 260}; // River 2 (moves Right, vx = 2)

static int ai_timer = 0;

void init_frogger() {
    frog_x = 160;
    frog_y = LANE_Y0;
    frog_score = 0;
    ai_timer = 0;
}

// Collide check helper
static bool check_car_collision(int fx, int fy) {
    if (fy == LANE_Y1) {
        for (int i = 0; i < 3; i++) {
            if (fx >= car_x1[i] - 6 && fx <= car_x1[i] + 24 + 6) return true;
        }
    } else if (fy == LANE_Y2) {
        for (int i = 0; i < 3; i++) {
            if (fx >= car_x2[i] - 6 && fx <= car_x2[i] + 24 + 6) return true;
        }
    }
    return false;
}

// Float check helper (returns true if on log, and gives log speed)
static bool check_log_floating(int fx, int fy, int *speed) {
    if (fy == LANE_Y4) {
        for (int i = 0; i < 3; i++) {
            if (fx >= log_x1[i] - 2 && fx <= log_x1[i] + 48 + 2) {
                *speed = -1;
                return true;
            }
        }
    } else if (fy == LANE_Y5) {
        for (int i = 0; i < 3; i++) {
            if (fx >= log_x2[i] - 2 && fx <= log_x2[i] + 48 + 2) {
                *speed = 2;
                return true;
            }
        }
    }
    return false;
}

static bool is_road_safe_predictive(int x, int y) {
    if (y == LANE_Y1) {
        for (int i = 0; i < 3; i++) {
            int cx = car_x1[i];
            for (int f = 0; f < 20; f++) {
                int proj_cx = cx - 2 * f;
                if (proj_cx < -30) proj_cx += 350;
                if (x >= proj_cx - 8 && x <= proj_cx + 32) {
                    return false;
                }
            }
        }
    } else if (y == LANE_Y2) {
        for (int i = 0; i < 3; i++) {
            int cx = car_x2[i];
            for (int f = 0; f < 20; f++) {
                int proj_cx = cx + 3 * f;
                if (proj_cx > 320) proj_cx -= 350;
                if (x >= proj_cx - 8 && x <= proj_cx + 32) {
                    return false;
                }
            }
        }
    }
    return true;
}

static int get_nearest_log_dist(int x, int river_id) {
    int min_d = 999;
    if (river_id == 1) {
        for (int i = 0; i < 3; i++) {
            int d = abs((log_x1[i] + 24) - x);
            if (d < min_d) min_d = d;
        }
    } else {
        for (int i = 0; i < 3; i++) {
            int d = abs((log_x2[i] + 24) - x);
            if (d < min_d) min_d = d;
        }
    }
    return min_d;
}

static void frogger_run_ai() {
    int sp = 0;
    if (frog_y == LANE_Y0) {
        // Safe to jump to Road 1?
        if (is_road_safe_predictive(frog_x, LANE_Y1)) {
            frog_y = LANE_Y1;
        } else {
            // step sideways to find gaps
            int step_dir = (frog_x > 160) ? -12 : 12;
            if (frog_x == 160) step_dir = (get_rand() % 2 == 0) ? 12 : -12;
            int next_x = frog_x + step_dir;
            if (next_x >= 10 && next_x <= 310) {
                frog_x = next_x;
            }
        }
    }
    else if (frog_y == LANE_Y1) {
        // Safe to jump to Road 2?
        if (is_road_safe_predictive(frog_x, LANE_Y2)) {
            frog_y = LANE_Y2;
        } else {
            // Dodge cars on current lane predictively if unsafe
            if (!is_road_safe_predictive(frog_x, LANE_Y1)) {
                bool left_safe = (frog_x - 12 >= 10) && is_road_safe_predictive(frog_x - 12, LANE_Y1);
                bool right_safe = (frog_x + 12 <= 310) && is_road_safe_predictive(frog_x + 12, LANE_Y1);
                
                if (left_safe && right_safe) {
                    if (abs(frog_x - 12 - 160) < abs(frog_x + 12 - 160)) {
                        frog_x -= 12;
                    } else {
                        frog_x += 12;
                    }
                } else if (left_safe) {
                    frog_x -= 12;
                } else if (right_safe) {
                    frog_x += 12;
                }
            }
        }
    }
    else if (frog_y == LANE_Y2) {
        // Safe to jump to Safe middle grass?
        frog_y = LANE_Y3;
    }
    else if (frog_y == LANE_Y3) {
        // Safe to jump to River 1 log directly?
        if (check_log_floating(frog_x, LANE_Y4, &sp)) {
            frog_y = LANE_Y4;
        } else {
            // Check if stepping left/right puts us in front of a log in River 1
            bool left_safe = (frog_x - 12 >= 10) && check_log_floating(frog_x - 12, LANE_Y4, &sp);
            bool right_safe = (frog_x + 12 <= 310) && check_log_floating(frog_x + 12, LANE_Y4, &sp);
            
            if (left_safe && right_safe) {
                frog_x += (get_rand() % 2 == 0) ? 12 : -12;
            } else if (left_safe) {
                frog_x -= 12;
            } else if (right_safe) {
                frog_x += 12;
            } else {
                // Otherwise, move towards the nearest log center in River 1
                int nearest_log_dist = 999;
                int step_dir = 1;
                for (int i = 0; i < 3; i++) {
                    int d = (log_x1[i] + 24) - frog_x;
                    if (abs(d) < abs(nearest_log_dist)) {
                        nearest_log_dist = d;
                        step_dir = (d > 0) ? 1 : -1;
                    }
                }
                int next_x = frog_x + step_dir * 12;
                if (next_x >= 10 && next_x <= 310) {
                    frog_x = next_x;
                }
            }
        }
    }
    else if (frog_y == LANE_Y4) {
        // Safe to jump to River 2 log directly?
        if (check_log_floating(frog_x, LANE_Y5, &sp)) {
            frog_y = LANE_Y5;
        } else {
            // Walk left/right along our River 1 log to align with a River 2 log
            bool left_valid = (frog_x - 12 >= 10) && check_log_floating(frog_x - 12, LANE_Y4, &sp);
            bool right_valid = (frog_x + 12 <= 310) && check_log_floating(frog_x + 12, LANE_Y4, &sp);
            
            int dist_curr = get_nearest_log_dist(frog_x, 2);
            int dist_left = left_valid ? get_nearest_log_dist(frog_x - 12, 2) : 999;
            int dist_right = right_valid ? get_nearest_log_dist(frog_x + 12, 2) : 999;
            
            if (left_valid && dist_left < dist_curr && (dist_left <= dist_right)) {
                frog_x -= 12;
            } else if (right_valid && dist_right < dist_curr && (dist_right <= dist_left)) {
                frog_x += 12;
            }
        }
    }
    else if (frog_y == LANE_Y5) {
        // Target destination bays
        int target_bays[5] = {40, 100, 160, 220, 280};
        int closest_bay = 160;
        int min_d = 999;
        for (int i = 0; i < 5; i++) {
            int d = abs(target_bays[i] - frog_x);
            if (d < min_d) {
                min_d = d;
                closest_bay = target_bays[i];
            }
        }

        // Jump to bay if aligned
        if (abs(frog_x - closest_bay) < 8) {
            frog_x = closest_bay;
            frog_y = LANE_Y6;
        } else {
            // Walk left/right along our River 2 log to align with the closest bay
            bool left_valid = (frog_x - 12 >= 10) && check_log_floating(frog_x - 12, LANE_Y5, &sp);
            bool right_valid = (frog_x + 12 <= 310) && check_log_floating(frog_x + 12, LANE_Y5, &sp);
            
            int dist_curr = abs(closest_bay - frog_x);
            int dist_left = left_valid ? abs(closest_bay - (frog_x - 12)) : 999;
            int dist_right = right_valid ? abs(closest_bay - (frog_x + 12)) : 999;
            
            if (left_valid && dist_left < dist_curr && (dist_left <= dist_right)) {
                frog_x -= 12;
            } else if (right_valid && dist_right < dist_curr && (dist_right <= dist_left)) {
                frog_x += 12;
            }
        }
    }

    // Keep frog within boundaries
    if (frog_x < 10) frog_x = 10;
    if (frog_x > 310) frog_x = 310;
}

static void draw_car(uint8_t *buffer, int x, int y, uint8_t body_color, bool moving_left) {
    // 1. Base body contour
    draw_rect(buffer, x + 2, y - 4, x + 22, y + 4, body_color);

    // 2. Tires (black rectangles that cut into the body)
    draw_rect(buffer, x + 4, y - 5, x + 7, y - 4, 0);
    draw_rect(buffer, x + 4, y + 4, x + 7, y + 5, 0);
    draw_rect(buffer, x + 17, y - 5, x + 20, y - 4, 0);
    draw_rect(buffer, x + 17, y + 4, x + 20, y + 5, 0);

    // 3. Windshield, windows, roof, lights
    if (moving_left) {
        // Front bumper (White 7)
        draw_rect(buffer, x + 1, y - 3, x + 1, y + 3, 7);
        // Rear bumper (Black 0)
        draw_rect(buffer, x + 23, y - 3, x + 23, y + 3, 0);

        // Headlights (Yellow 3) at front-left
        draw_pixel(buffer, x + 2, y - 3, 3);
        draw_pixel(buffer, x + 2, y + 3, 3);

        // Taillights (Red 1) at rear-right
        draw_pixel(buffer, x + 22, y - 3, 1);
        draw_pixel(buffer, x + 22, y + 3, 1);

        // Windows (Cyan 6)
        draw_rect(buffer, x + 7, y - 3, x + 8, y + 3, 6);   // Front windshield
        draw_rect(buffer, x + 9, y - 3, x + 14, y - 3, 6);  // Top side window
        draw_rect(buffer, x + 9, y + 3, x + 14, y + 3, 6);  // Bottom side window
        draw_rect(buffer, x + 15, y - 3, x + 16, y + 3, 6); // Rear window

        // Roof (Body color)
        draw_rect(buffer, x + 9, y - 2, x + 14, y + 2, body_color);
    } else {
        // Front bumper (White 7) on the right
        draw_rect(buffer, x + 23, y - 3, x + 23, y + 3, 7);
        // Rear bumper (Black 0) on the left
        draw_rect(buffer, x + 1, y - 3, x + 1, y + 3, 0);

        // Headlights (Yellow 3) at front-right
        draw_pixel(buffer, x + 22, y - 3, 3);
        draw_pixel(buffer, x + 22, y + 3, 3);

        // Taillights (Red 1) at rear-left
        draw_pixel(buffer, x + 2, y - 3, 1);
        draw_pixel(buffer, x + 2, y + 3, 1);

        // Windows (Cyan 6)
        draw_rect(buffer, x + 16, y - 3, x + 17, y + 3, 6); // Front windshield
        draw_rect(buffer, x + 10, y - 3, x + 15, y - 3, 6); // Top side window
        draw_rect(buffer, x + 10, y + 3, x + 15, y + 3, 6); // Bottom side window
        draw_rect(buffer, x + 8, y - 3, x + 9, y + 3, 6);   // Rear window

        // Roof (Body color)
        draw_rect(buffer, x + 10, y - 2, x + 15, y + 2, body_color);
    }

    // Ultra HD body sheen + headlight bloom
    art_dither_pixel(buffer, x + 12, y - 4, 7, 2);
    art_specular(buffer, moving_left ? x + 3 : x + 21, y - 2, 7);
}

void play_frogger(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);

    // Draw HUD Score
    draw_score(buffer, frog_score, 10, 10, 2, 7);

    // --- Lane Updates ---
    for (int i = 0; i < 3; i++) {
        // Road 1 (Left)
        car_x1[i] -= 2;
        if (car_x1[i] < -30) car_x1[i] = 320;
        // Road 2 (Right)
        car_x2[i] += 3;
        if (car_x2[i] > 320) car_x2[i] = -30;
        // River 1 (Left)
        log_x1[i] -= 1;
        if (log_x1[i] < -50) log_x1[i] = 320;
        // River 2 (Right)
        log_x2[i] += 2;
        if (log_x2[i] > 320) log_x2[i] = -50;
    }

    // --- Draw Lanes backgrounds ---
    // Safe Grass bottom
    draw_rect(buffer, 0, 218, 319, 239, 2); // Green
    // Safe Grass middle
    draw_rect(buffer, 0, 115, 319, 135, 2); // Green
    // River zones (Lanes 4 and 5)
    draw_rect(buffer, 0, 55, 319, 105, 4); // Blue river water

    // Top Bays Zone
    draw_rect(buffer, 0, 25, 319, 54, 2); // Green grass headers
    // Draw 5 open water bays (black bays)
    int target_bays[5] = {40, 100, 160, 220, 280};
    for (int i = 0; i < 5; i++) {
        draw_rect(buffer, target_bays[i] - 8, 25, target_bays[i] + 8, 45, 0); // Black bay cutouts
    }

    // --- Draw Game Objects ---
    // Draw Cars (Lanes 1 and 2)
    uint8_t road1_colors[3] = {1, 5, 7}; // Red, Magenta, White
    uint8_t road2_colors[3] = {3, 5, 1}; // Yellow, Magenta, Red
    for (int i = 0; i < 3; i++) {
        // Road 1: Cars moving left
        draw_car(buffer, car_x1[i], LANE_Y1, road1_colors[i], true);
        // Road 2: Cars moving right
        draw_car(buffer, car_x2[i], LANE_Y2, road2_colors[i], false);
    }

    // Draw Logs (Lanes 4 and 5)
    for (int i = 0; i < 3; i++) {
        // River 1 logs
        draw_rect(buffer, log_x1[i], LANE_Y4 - 6, log_x1[i] + 48, LANE_Y4 + 6, 1); // Brown logs
        draw_rect(buffer, log_x1[i] + 2, LANE_Y4 - 4, log_x1[i] + 46, LANE_Y4 - 2, 3); // log highlights
        // River 2 logs
        draw_rect(buffer, log_x2[i], LANE_Y5 - 6, log_x2[i] + 48, LANE_Y5 + 6, 1);
        draw_rect(buffer, log_x2[i] + 2, LANE_Y5 - 4, log_x2[i] + 46, LANE_Y5 - 2, 3);
    }

    // --- Frog Physics and State Check ---
    bool dead = false;
    
    // Floating checks
    int speed = 0;
    if (frog_y == LANE_Y4 || frog_y == LANE_Y5) {
        if (check_log_floating(frog_x, frog_y, &speed)) {
            frog_x += speed; // float with river
            if (frog_x < 5 || frog_x > 315) dead = true; // floated off-screen
        } else {
            dead = true; // fell in water!
        }
    }

    // Car Collision checks
    if (frog_y == LANE_Y1 || frog_y == LANE_Y2) {
        if (check_car_collision(frog_x, frog_y)) dead = true;
    }

    // Reach destination check
    if (frog_y == LANE_Y6) {
        // Check if landed in one of the bays
        bool scored = false;
        for (int i = 0; i < 5; i++) {
            if (frog_x >= target_bays[i] - 10 && frog_x <= target_bays[i] + 10) {
                scored = true;
                break;
            }
        }
        if (scored) {
            frog_score += 200;
            frog_x = 160;
            frog_y = LANE_Y0;
            ai_timer = 0;
            return;
        } else {
            dead = true; // hit destination walls
        }
    }

    if (dead) {
        frog_score = (frog_score > 50) ? frog_score - 50 : 0;
        frog_x = 160;
        frog_y = LANE_Y0;
        ai_timer = 0;
        return;
    }

    // --- AI Timer Actions ---
    ai_timer++;
    if (ai_timer >= 8) {
        ai_timer = 0;
        frogger_run_ai();
    }

    // --- Draw Frog (cyan circle) ---
    draw_circle(buffer, frog_x, frog_y, 5, 6); // Cyan body
    draw_circle(buffer, frog_x - 3, frog_y - 3, 2, 7); // Eyes
    draw_circle(buffer, frog_x + 3, frog_y - 3, 2, 7);
    draw_pixel(buffer, frog_x - 3, frog_y - 3, 0);
    draw_pixel(buffer, frog_x + 3, frog_y - 3, 0);
}
