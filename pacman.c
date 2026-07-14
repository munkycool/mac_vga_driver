#include "common.h"

// --- Deep Pacman Maze Map ---
static const uint8_t PAC_MAZE_MAP[15][20] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,2,2,2,2,2,2,2,1,1,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,2,1,1,1,2,1,1,2,1,1,1,2,1,1,2,1},
    {1,3,1,1,2,1,1,1,2,1,1,2,1,1,1,2,1,1,3,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,2,1,2,1,1,1,1,1,1,2,1,2,1,1,2,1},
    {1,2,2,2,2,1,2,2,2,1,1,2,2,2,1,2,2,2,2,1},
    {1,1,1,1,2,1,1,1,0,1,1,0,1,1,1,2,1,1,1,1},
    {0,0,0,1,2,1,0,0,0,0,0,0,0,0,1,2,1,0,0,0},
    {1,1,1,1,2,1,0,1,1,0,0,1,1,0,1,2,1,1,1,1},
    {1,2,2,2,2,0,0,1,0,0,0,0,1,0,0,2,2,2,2,1},
    {1,2,1,1,2,1,0,1,1,1,1,1,1,0,1,2,1,1,2,1},
    {1,3,2,1,2,1,2,2,2,2,2,2,2,2,1,2,1,2,3,1},
    {1,1,2,1,2,1,2,1,1,1,1,1,1,2,1,2,1,2,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static uint8_t pac_grid[15][20];
static int pac_cx = 1, pac_cy = 1;
static int pac_dx = 1, pac_dy = 0;
static int pac_px = 40 + 12, pac_py = 30 + 12;
static Ghost ghosts[4];
static int pac_score = 0;
static int frightened_timer = 0;

bool is_walkable(int cx, int cy) {
    if (cx < 0 || cx >= 20 || cy < 0 || cy >= 15) return false;
    return (pac_grid[cy][cx] != 1);
}

void draw_hi_res_pacman(uint8_t *buffer, int cx, int cy, int r, int dx, int dy, bool mouth_open) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                if (mouth_open) {
                    if (dx > 0 && x > 0 && abs(y) < x) continue;
                    if (dx < 0 && x < 0 && abs(y) < -x) continue;
                    if (dy > 0 && y > 0 && abs(x) < y) continue;
                    if (dy < 0 && y < 0 && abs(x) < -y) continue;
                }
                draw_pixel(buffer, cx + x, cy + y, 3); // Yellow
            }
        }
    }
    int eye_x = (dx != 0) ? cx : cx + 2;
    int eye_y = (dy != 0) ? cy : cy - 3;
    draw_pixel(buffer, eye_x, eye_y, 0);
}

void draw_hi_res_ghost(uint8_t *buffer, int cx, int cy, int r, uint8_t color, int frame_counter) {
    for (int y = -r; y <= 0; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) draw_pixel(buffer, cx + x, cy + y, color);
        }
    }
    draw_rect(buffer, cx - r, cy + 1, cx + r, cy + r - 3, color);
    
    int animate_wave = (frame_counter / 8) % 2;
    for (int x = -r; x <= r; x++) {
        int base_y = cy + r - 3;
        if (((x + r) % 6 < 3) == (animate_wave == 0)) {
            draw_pixel(buffer, cx + x, base_y + 1, color);
            draw_pixel(buffer, cx + x, base_y + 2, color);
        }
    }
    
    draw_circle(buffer, cx - 3, cy - 2, 2, 7);
    draw_circle(buffer, cx + 3, cy - 2, 2, 7);
    draw_pixel(buffer, cx - 2, cy - 2, 4); 
    draw_pixel(buffer, cx + 4, cy - 2, 4);
    art_specular(buffer, cx - 3, cy - 3, 7);
    art_specular(buffer, cx + 3, cy - 3, 7);
}

void update_ghost_ai(Ghost *g, int target_cx, int target_cy) {
    if (g->dead) return;
    int dirs[4][2] = {{0,-1}, {1,0}, {0,1}, {-1,0}};
    int best_dir = -1;
    int min_dist = 999999;
    
    for (int i = 0; i < 4; i++) {
        int nx = g->cx + dirs[i][0];
        int ny = g->cy + dirs[i][1];
        if (dirs[i][0] == -g->dx && dirs[i][1] == -g->dy) continue; 
        if (is_walkable(nx, ny)) {
            int dist;
            if (frightened_timer > 0) {
                dist = -((nx - target_cx)*(nx - target_cx) + (ny - target_cy)*(ny - target_cy));
            } else {
                dist = (nx - target_cx)*(nx - target_cx) + (ny - target_cy)*(ny - target_cy);
            }
            if (dist < min_dist) {
                min_dist = dist;
                best_dir = i;
            }
        }
    }
    if (best_dir == -1) {
        for (int i = 0; i < 4; i++) {
            int nx = g->cx + dirs[i][0];
            int ny = g->cy + dirs[i][1];
            if (is_walkable(nx, ny)) {
                int dist = (nx - target_cx)*(nx - target_cx) + (ny - target_cy)*(ny - target_cy);
                if (dist < min_dist) {
                    min_dist = dist;
                    best_dir = i;
                }
            }
        }
    }
    if (best_dir != -1) {
        g->dx = dirs[best_dir][0];
        g->dy = dirs[best_dir][1];
    }
}

void init_pacman() {
    memcpy(pac_grid, PAC_MAZE_MAP, sizeof(PAC_MAZE_MAP));
    pac_px = 40 + 12; pac_py = 30 + 12;
    pac_cx = 1; pac_cy = 1;
    frightened_timer = 0;
    
    int m_x = 40; int m_y = 30;
    ghosts[0] = (Ghost){18, 13, -1, 0, m_x + 18*12, m_y + 13*12, 1, false}; 
    ghosts[1] = (Ghost){1, 13, 1, 0, m_x + 1*12, m_y + 13*12, 5, false};   
    ghosts[2] = (Ghost){18, 1, -1, 0, m_x + 18*12, m_y + 1*12, 6, false};  
    ghosts[3] = (Ghost){9, 10, 0, -1, m_x + 9*12, m_y + 10*12, 7, false};  
    pac_score = 0;
}

void play_pacman(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);
    draw_score(buffer, pac_score, 10, 10, 2, 7);
    
    int m_x = 40;
    int m_y = 30;
    
    for (int y = 0; y < 15; y++) {
        for (int x = 0; x < 20; x++) {
            int px = m_x + x * 12;
            int py = m_y + y * 12;
            if (pac_grid[y][x] == 1) {
                draw_rect(buffer, px, py, px + 11, py + 11, 4);
                draw_rect(buffer, px + 2, py + 2, px + 9, py + 9, 0);
            } else if (pac_grid[y][x] == 2) {
                draw_rect(buffer, px + 5, py + 5, px + 6, py + 6, 3);
            } else if (pac_grid[y][x] == 3) {
                if (frame_counter % 16 < 8) draw_circle(buffer, px + 6, py + 6, 3, 3);
            }
        }
    }
    
    if (frightened_timer > 0) frightened_timer--;
    
    int p_tx = m_x + pac_cx * 12;
    int p_ty = m_y + pac_cy * 12;
    if (pac_px == p_tx && pac_py == p_ty) {
        if (pac_grid[pac_cy][pac_cx] == 2) { pac_grid[pac_cy][pac_cx] = 0; pac_score += 10; }
        else if (pac_grid[pac_cy][pac_cx] == 3) { 
            pac_grid[pac_cy][pac_cx] = 0; 
            pac_score += 50; 
            frightened_timer = 240; 
        }
        
        int dirs[4][2] = {{0,-1}, {1,0}, {0,1}, {-1,0}};
        int best_dir = -1;
        int best_weight = -999999;
        
        for (int i = 0; i < 4; i++) {
            int nx = pac_cx + dirs[i][0];
            int ny = pac_cy + dirs[i][1];
            if (!is_walkable(nx, ny)) continue;
            
            int weight = 0;
            int min_pellet_dist = 9999;
            for (int py = 0; py < 15; py++) {
                for (int px = 0; px < 20; px++) {
                    if (pac_grid[py][px] == 2 || pac_grid[py][px] == 3) {
                        int dist = abs(nx - px) + abs(ny - py);
                        if (dist < min_pellet_dist) min_pellet_dist = dist;
                    }
                }
            }
            weight -= min_pellet_dist * 10; 
            
            for (int g = 0; g < 4; g++) {
                if (ghosts[g].dead) continue;
                int dist = abs(nx - ghosts[g].cx) + abs(ny - ghosts[g].cy);
                if (frightened_timer > 0) {
                    weight -= dist * 40; 
                } else {
                    if (dist < 5) weight += dist * 250; 
                }
            }
            if (dirs[i][0] == -pac_dx && dirs[i][1] == -pac_dy) weight -= 50;
            
            if (weight > best_weight) {
                best_weight = weight;
                best_dir = i;
            }
        }
        if (best_dir != -1) {
            pac_dx = dirs[best_dir][0];
            pac_dy = dirs[best_dir][1];
        }
        pac_cx += pac_dx; pac_cy += pac_dy;
    }
    if (pac_px < p_tx) pac_px++; else if (pac_px > p_tx) pac_px--;
    if (pac_py < p_ty) pac_py++; else if (pac_py > p_ty) pac_py--;
    
    for (int i = 0; i < 4; i++) {
        Ghost *g = &ghosts[i];
        int g_tx = m_x + g->cx * 12;
        int g_ty = m_y + g->cy * 12;
        
        bool update_step = true;
        if (frightened_timer > 0) {
            update_step = (frame_counter % 2 == 0); 
        }
        
        if (update_step && g->px == g_tx && g->py == g_ty) {
            if (g->dead) {
                g->cx = 9; g->cy = 7;
                g->dead = false;
            } else {
                int t_cx = pac_cx, t_cy = pac_cy;
                if (i == 1) { t_cx = pac_cx + pac_dx*2; t_cy = pac_cy + pac_dy*2; } 
                else if (i == 2) { t_cx = pac_cx - pac_dx*2; t_cy = pac_cy - pac_dy*2; } 
                else if (i == 3) { t_cx = get_rand() % 20; t_cy = get_rand() % 15; } 
                
                update_ghost_ai(g, t_cx, t_cy);
                g->cx += g->dx; g->cy += g->dy;
            }
        }
        if (g->px < g_tx) g->px++; else if (g->px > g_tx) g->px--;
        if (g->py < g_ty) g->py++; else if (g->py > g_ty) g->py--;
        
        uint8_t final_color = g->color;
        if (frightened_timer > 0) {
            final_color = (frightened_timer < 60 && (frightened_timer % 16 < 8)) ? g->color : 6; 
        }
        draw_hi_res_ghost(buffer, g->px + 6, g->py + 6, 5, final_color, frame_counter);
        
        if (abs(pac_px - g->px) < 8 && abs(pac_py - g->py) < 8) {
            if (frightened_timer > 0) {
                g->dead = true;
                g->px = m_x + 9*12; g->py = m_y + 10*12;
                g->cx = 9; g->cy = 10;
                pac_score += 200;
            } else {
                init_pacman();
                return;
            }
        }
    }
    bool mouth_open = (frame_counter % 12 < 6);
    draw_hi_res_pacman(buffer, pac_px + 6, pac_py + 6, 5, pac_dx, pac_dy, mouth_open);
}
