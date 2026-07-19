#include "common.h"

// --- Tetris Piece Shapes ---
static const uint8_t TETROMINOES[7][4][4][4] = {
    // 0: I (Cyan / Color 6)
    {
        {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
        {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}
    },
    // 1: O (Yellow / Color 3)
    {
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}
    },
    // 2: T (Purple / Color 5)
    {
        {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}
    },
    // 3: S (Green / Color 2)
    {
        {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
        {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}}
    },
    // 4: Z (Red / Color 1)
    {
        {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}}
    },
    // 5: J (Blue / Color 4)
    {
        {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}
    },
    // 6: L (White / Color 7)
    {
        {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
        {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}
    }
};

static const uint8_t TETROMINO_COLORS[7] = {6, 3, 5, 2, 1, 4, 7};

static uint8_t tetris_grid[20][10];
static int curr_piece_type = 0;
static int next_piece_type = 0;
static int curr_piece_row = 0;
static int curr_piece_col = 4;
static int curr_piece_rot = 0;
static int ai_target_col = 4;
static int ai_target_rot = 0;
static int tetris_tick_counter = 0;
static int line_clear_anim_ticks = 0;
static uint8_t lines_to_clear[20];
static int num_lines_to_clear = 0;
static int tetris_score = 0;
static int tetris_lines = 0;
static int tetris_popup_ticks = 0;

// Star field coordinates for twinkling sky aesthetics
static const int STAR_COORDS[12][2] = {
    {15, 25}, {95, 30}, {215, 20}, {305, 35},
    {20, 95}, {100, 100}, {212, 90}, {308, 98},
    {10, 160}, {98, 172}, {220, 165}, {310, 155}
};

bool tetris_collide(int type, int rot, int row, int col) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (TETROMINOES[type][rot][y][x]) {
                int gr = row + y;
                int gc = col + x;
                if (gc < 0 || gc >= 10 || gr >= 20) return true;
                if (gr >= 0 && tetris_grid[gr][gc] != 0) return true;
            }
        }
    }
    return false;
}

int get_land_row(int type, int rot, int col) {
    int r = -3;
    while (!tetris_collide(type, rot, r + 1, col)) {
        r++;
    }
    return r;
}

void draw_beveled_block(uint8_t *buffer, int x, int y, uint8_t color) {
    // 8x8 block with a thin black outline and top-left retro bevel highlight
    draw_rect(buffer, x, y, x + 7, y + 7, 0); // black border
    draw_rect(buffer, x + 1, y + 1, x + 6, y + 6, color); // solid fill
    // Top-left highlight
    draw_pixel(buffer, x + 1, y + 1, 7);
    draw_pixel(buffer, x + 2, y + 1, 7);
    draw_pixel(buffer, x + 3, y + 1, 7);
    draw_pixel(buffer, x + 1, y + 2, 7);
    draw_pixel(buffer, x + 1, y + 3, 7);
}

int evaluate_grid(uint8_t grid[20][10]) {
    int holes = 0;
    int row_transitions = 0;
    int col_transitions = 0;
    int well_sum = 0;
    
    // Calculate heights of each column
    int heights[10] = {0};
    int max_h = 0;
    int aggregate_height = 0;
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 20; y++) {
            if (grid[y][x] != 0) {
                heights[x] = 20 - y;
                break;
            }
        }
        if (x < 9) {
            aggregate_height += heights[x];
            if (heights[x] > max_h) max_h = heights[x];
        }
    }
    
    // Calculate holes
    for (int x = 0; x < 10; x++) {
        bool block_above = false;
        for (int y = 0; y < 20; y++) {
            if (grid[y][x] != 0) block_above = true;
            else if (block_above && grid[y][x] == 0) holes++;
        }
    }
    
    // Row transitions
    for (int y = 0; y < 20; y++) {
        int last = 1; // Left border is solid (1)
        for (int x = 0; x < 10; x++) {
            int current = (grid[y][x] != 0) ? 1 : 0;
            if (current != last) row_transitions++;
            last = current;
        }
        if (last != 1) row_transitions++; // Right border is solid (1)
    }
    
    // Column transitions
    for (int x = 0; x < 10; x++) {
        int last = 0; // Sky is empty (0)
        for (int y = 0; y < 20; y++) {
            int current = (grid[y][x] != 0) ? 1 : 0;
            if (current != last) col_transitions++;
            last = current;
        }
        if (last != 1) col_transitions++; // Floor is solid (1)
    }
    
    // Wells (columns 0 to 8 only; column 9 is the Tetris well, not penalized)
    for (int x = 0; x < 9; x++) {
        int depth = 0;
        for (int y = 0; y < 20; y++) {
            if (grid[y][x] == 0) {
                bool left_solid = (x == 0) || (grid[y][x - 1] != 0);
                bool right_solid = (grid[y][x + 1] != 0);
                if (left_solid && right_solid) {
                    depth++;
                } else {
                    if (depth > 0) {
                        well_sum += (depth * (depth + 1)) / 2;
                        depth = 0;
                    }
                }
            } else {
                if (depth > 0) {
                    well_sum += (depth * (depth + 1)) / 2;
                    depth = 0;
                }
            }
        }
        if (depth > 0) {
            well_sum += (depth * (depth + 1)) / 2;
        }
    }
    
    // Bumpiness (sum of absolute height differences between adjacent columns 0 to 8)
    int bumpiness = 0;
    for (int x = 0; x < 8; x++) {
        bumpiness += abs(heights[x] - heights[x + 1]);
    }

    // Height penalty adjustments to allow/encourage stacking for Tetris
    int height_penalty;
    if (max_h > 12) {
        height_penalty = 50 * aggregate_height + 500 * (max_h - 12);
    } else if (max_h < 4) {
        height_penalty = 10 * aggregate_height + 100 * (4 - max_h);
    } else {
        height_penalty = 15 * aggregate_height;
    }

    // Heavy penalty for blocking the Tetris well (column 9)
    int col9_penalty = heights[9] * 1000;

    return -height_penalty
           - (760 * holes)
           - (18 * row_transitions)
           - (18 * col_transitions)
           - (24 * well_sum)
           - (18 * bumpiness)
           - col9_penalty;
}

void tetris_run_ai() {
    int best_score = -9999999;
    ai_target_col = 3;
    ai_target_rot = 0;

    // Search all possible placements of the current piece (c1: -3 to 9)
    for (int r1 = 0; r1 < 4; r1++) {
        for (int c1 = -3; c1 < 10; c1++) {
            if (tetris_collide(curr_piece_type, r1, 0, c1)) continue; 
            int land_r1 = get_land_row(curr_piece_type, r1, c1);
            if (land_r1 < 0) continue; 
            
            // Place current piece on temp_grid1
            uint8_t temp_grid1[20][10];
            memcpy(temp_grid1, tetris_grid, sizeof(tetris_grid));
            
            for (int py = 0; py < 4; py++) {
                for (int px = 0; px < 4; px++) {
                    if (TETROMINOES[curr_piece_type][r1][py][px]) {
                        int gr = land_r1 + py;
                        int gc = c1 + px;
                        if (gr >= 0 && gr < 20 && gc >= 0 && gc < 10) {
                            temp_grid1[gr][gc] = 1; 
                        }
                    }
                }
            }
            
            // Collapse lines cleared by 1st piece
            int lines_cleared1 = 0;
            for (int ty = 19; ty >= 0; ty--) {
                bool full = true;
                for (int tx = 0; tx < 10; tx++) {
                    if (temp_grid1[ty][tx] == 0) { full = false; break; }
                }
                if (full) {
                    lines_cleared1++;
                    for (int r = ty; r > 0; r--) {
                        memcpy(temp_grid1[r], temp_grid1[r - 1], 10);
                    }
                    memset(temp_grid1[0], 0, 10);
                    ty++; // Check same row index again
                }
            }
            
            // Search all possible placements of the next piece on temp_grid1 (c2: -3 to 9)
            int best_score2 = -9999999;
            for (int r2 = 0; r2 < 4; r2++) {
                for (int c2 = -3; c2 < 10; c2++) {
                    // Collision check for the 2nd piece on temp_grid1
                    bool collide2 = false;
                    for (int y = 0; y < 4; y++) {
                        for (int x = 0; x < 4; x++) {
                            if (TETROMINOES[next_piece_type][r2][y][x]) {
                                int gr = y;
                                int gc = c2 + x;
                                if (gc < 0 || gc >= 10 || gr >= 20) { collide2 = true; break; }
                                if (gr >= 0 && temp_grid1[gr][gc] != 0) { collide2 = true; break; }
                            }
                        }
                        if (collide2) break;
                    }
                    if (collide2) continue;

                    // Find land row of the 2nd piece on temp_grid1
                    int land_r2 = -3;
                    while (true) {
                        bool collide_next = false;
                        int next_r = land_r2 + 1;
                        for (int y = 0; y < 4; y++) {
                            for (int x = 0; x < 4; x++) {
                                if (TETROMINOES[next_piece_type][r2][y][x]) {
                                    int gr = next_r + y;
                                    int gc = c2 + x;
                                    if (gc < 0 || gc >= 10 || gr >= 20) { collide_next = true; break; }
                                    if (gr >= 0 && temp_grid1[gr][gc] != 0) { collide_next = true; break; }
                                }
                            }
                            if (collide_next) break;
                        }
                        if (collide_next) break;
                        land_r2++;
                    }
                    if (land_r2 < 0) continue;

                    // Place next piece on temp_grid2
                    uint8_t temp_grid2[20][10];
                    memcpy(temp_grid2, temp_grid1, sizeof(temp_grid1));
                    for (int py = 0; py < 4; py++) {
                        for (int px = 0; px < 4; px++) {
                            if (TETROMINOES[next_piece_type][r2][py][px]) {
                                int gr = land_r2 + py;
                                int gc = c2 + px;
                                if (gr >= 0 && gr < 20 && gc >= 0 && gc < 10) {
                                    temp_grid2[gr][gc] = 1;
                                }
                            }
                        }
                    }

                    // Collapse lines cleared by 2nd piece
                    int lines_cleared2 = 0;
                    for (int ty = 19; ty >= 0; ty--) {
                        bool full = true;
                        for (int tx = 0; tx < 10; tx++) {
                            if (temp_grid2[ty][tx] == 0) { full = false; break; }
                        }
                        if (full) {
                            lines_cleared2++;
                            for (int r = ty; r > 0; r--) {
                                memcpy(temp_grid2[r], temp_grid2[r - 1], 10);
                            }
                            memset(temp_grid2[0], 0, 10);
                            ty++;
                        }
                    }

                    // Evaluate temp_grid2
                    int score2 = evaluate_grid(temp_grid2);

                    // Find max height of columns 0-8 on temp_grid2
                    int max_h2 = 0;
                    for (int x = 0; x < 9; x++) {
                        int h = 0;
                        for (int y = 0; y < 20; y++) {
                            if (temp_grid2[y][x] != 0) {
                                h = 20 - y;
                                break;
                            }
                        }
                        if (h > max_h2) max_h2 = h;
                    }

                    // Strongly reward Tetris (4-line clears)
                    if (lines_cleared1 == 4) score2 += 12000;
                    if (lines_cleared2 == 4) score2 += 12000;

                    // Discourage minor clears unless we are high up and need to survive
                    int total_lines_cleared = lines_cleared1 + lines_cleared2;
                    if (total_lines_cleared > 0 && total_lines_cleared < 4) {
                        if (max_h2 < 13) {
                            score2 -= 4000; // Penalize non-Tetris clears to keep the well open
                        } else {
                            score2 += total_lines_cleared * 1000; // Survive!
                        }
                    }

                    if (score2 > best_score2) best_score2 = score2;
                }
            }

            if (best_score2 > best_score) {
                best_score = best_score2;
                ai_target_col = c1;
                ai_target_rot = r1;
            }
        }
    }
}

void spawn_tetris_piece() {
    curr_piece_type = next_piece_type;
    next_piece_type = get_rand() % 7;
    curr_piece_row = -1;
    curr_piece_col = 3;
    curr_piece_rot = 0;
    tetris_run_ai();
}

void init_tetris() {
    memset(tetris_grid, 0, sizeof(tetris_grid));
    // Starts with a completely empty board
    tetris_score = 0;
    tetris_lines = 0;
    tetris_tick_counter = 0;
    line_clear_anim_ticks = 0;
    num_lines_to_clear = 0;
    next_piece_type = get_rand() % 7;
    spawn_tetris_piece();
}

void draw_striped_dome(uint8_t *buffer, int cx, int cy, int r, uint8_t color_a, uint8_t color_b, int pattern_type) {
    // Draw an onion dome shape: wider at lower-middle, tapering to a top spire
    for (int dy = -r - 4; dy <= r; dy++) {
        int y = cy + dy;
        if (y < 0 || y >= 240) continue;
        int w = 0;
        if (dy < -r) {
            w = 1;
        } else if (dy < 0) {
            // Tapering top half (cone shape)
            w = r + dy;
            if (w < 1) w = 1;
        } else {
            // Bulbous/circular bottom half
            w = isqrt(r * r - dy * dy);
        }
        
        for (int x = cx - w; x <= cx + w; x++) {
            if (x < 0 || x >= 320) continue;
            uint8_t color = color_a;
            if (pattern_type == 0) { // Diagonal stripes (/)
                color = ((x + y) / 3) % 2 == 0 ? color_a : color_b;
            } else if (pattern_type == 1) { // Diagonal stripes (\)
                color = ((x - y) / 3) % 2 == 0 ? color_a : color_b;
            } else if (pattern_type == 2) { // Vertical stripes
                color = (x / 3) % 2 == 0 ? color_a : color_b;
            } else if (pattern_type == 3) { // Horizontal stripes
                color = (y / 3) % 2 == 0 ? color_a : color_b;
            }
            draw_pixel(buffer, x, y, color);
        }
    }
}

void draw_kremlin_graphics(uint8_t *buffer) {
    // Ground/brick base across the bottom behind everything
    draw_rect(buffer, 5, 232, 110, 239, 1);   // Left base (Red bricks)
    draw_rect(buffer, 210, 232, 315, 239, 1); // Right base
    
    // Draw brick mortar lines for details
    for (int y = 234; y <= 238; y += 2) {
        draw_rect(buffer, 5, y, 110, y, 0);
        draw_rect(buffer, 210, y, 315, y, 0);
    }
    
    // --- Left Side Kremlin Towers ---
    
    // Tower 1 (Leftmost): Red/yellow brick tower
    draw_rect(buffer, 10, 195, 30, 232, 1);
    // Masonry offset brick pattern
    for (int y = 198; y < 232; y += 4) {
        draw_rect(buffer, 10, y, 30, y, 0);
        int shift = (y % 8 == 0) ? 3 : 0;
        for (int x = 10 + shift; x < 30; x += 5) draw_rect(buffer, x, y, x, y + 3, 0);
    }
    // Draw white arches on Tower 1
    draw_rect(buffer, 14, 202, 26, 210, 7);
    draw_rect(buffer, 16, 204, 24, 210, 0); // black window inside arch
    // Striped dome: Blue and Yellow, diagonal
    draw_striped_dome(buffer, 20, 185, 6, 4, 3, 0);
    // Gold Cross
    draw_rect(buffer, 20, 172, 20, 178, 3);
    draw_rect(buffer, 18, 174, 22, 174, 3);

    // Tower 2 (Middle-Left, Tallest): Detailed white/cyan tower
    draw_rect(buffer, 35, 175, 75, 232, 7); // White stone
    draw_rect(buffer, 45, 155, 65, 175, 1); // Red brick neck
    // Neck brick mortar lines
    for (int y = 158; y < 175; y += 3) {
        draw_rect(buffer, 45, y, 65, y, 0);
        int shift = (y % 6 == 0) ? 3 : 0;
        for (int x = 45 + shift; x < 65; x += 5) draw_rect(buffer, x, y, x, y + 2, 0);
    }
    // Draw decorative white stone trims
    draw_rect(buffer, 35, 175, 75, 177, 6); // Cyan trim
    draw_rect(buffer, 42, 190, 68, 215, 6); // Inner window pane
    draw_rect(buffer, 45, 193, 65, 215, 0); // Black window
    // Striped dome: Red and White, diagonal
    draw_striped_dome(buffer, 55, 143, 9, 1, 7, 1);
    // Gold Cross
    draw_rect(buffer, 55, 127, 55, 133, 3);
    draw_rect(buffer, 53, 129, 57, 129, 3);

    // Tower 3 (Inner-Left): Green tower
    draw_rect(buffer, 80, 190, 105, 232, 2);
    // Arch details
    draw_rect(buffer, 86, 200, 99, 210, 3);
    draw_rect(buffer, 88, 202, 97, 210, 0);
    // Striped dome: Cyan and Magenta, vertical
    draw_striped_dome(buffer, 92, 180, 7, 6, 5, 2);
    // Gold Cross
    draw_rect(buffer, 92, 166, 92, 172, 3);
    draw_rect(buffer, 90, 168, 94, 168, 3);

    // --- Right Side Kremlin Towers ---
    
    // Tower 4 (Inner-Right): Green tower
    draw_rect(buffer, 215, 190, 240, 232, 2);
    // Arch details
    draw_rect(buffer, 221, 200, 234, 210, 3);
    draw_rect(buffer, 223, 202, 232, 210, 0);
    // Striped dome: Cyan and Magenta, vertical
    draw_striped_dome(buffer, 228, 180, 7, 6, 5, 2);
    // Gold Cross
    draw_rect(buffer, 228, 166, 228, 172, 3);
    draw_rect(buffer, 226, 168, 230, 168, 3);

    // Tower 5 (Middle-Right, Tallest): Detailed white/cyan tower
    draw_rect(buffer, 245, 175, 285, 232, 7); // White stone
    draw_rect(buffer, 255, 155, 275, 175, 1); // Red brick neck
    // Neck brick mortar lines
    for (int y = 158; y < 175; y += 3) {
        draw_rect(buffer, 255, y, 275, y, 0);
        int shift = (y % 6 == 0) ? 3 : 0;
        for (int x = 255 + shift; x < 275; x += 5) draw_rect(buffer, x, y, x, y + 2, 0);
    }
    // Draw decorative white stone trims
    draw_rect(buffer, 245, 175, 285, 177, 6); // Cyan trim
    draw_rect(buffer, 252, 190, 278, 215, 6); // Inner window pane
    draw_rect(buffer, 255, 193, 275, 215, 0); // Black window
    // Striped dome: Red and White, diagonal
    draw_striped_dome(buffer, 265, 143, 9, 1, 7, 1);
    // Gold Cross
    draw_rect(buffer, 265, 127, 265, 133, 3);
    draw_rect(buffer, 263, 129, 267, 129, 3);

    // Tower 6 (Rightmost): Red/yellow brick tower
    draw_rect(buffer, 290, 195, 310, 232, 1);
    // Masonry offset brick pattern
    for (int y = 198; y < 232; y += 4) {
        draw_rect(buffer, 290, y, 310, y, 0);
        int shift = (y % 8 == 0) ? 3 : 0;
        for (int x = 290 + shift; x < 310; x += 5) draw_rect(buffer, x, y, x, y + 3, 0);
    }
    // Draw white arches on Tower 6
    draw_rect(buffer, 294, 202, 306, 210, 7);
    draw_rect(buffer, 296, 204, 304, 210, 0); // black window inside arch
    // Striped dome: Blue and Yellow, diagonal
    draw_striped_dome(buffer, 300, 185, 6, 4, 3, 0);
    // Gold Cross
    draw_rect(buffer, 300, 172, 300, 178, 3);
    draw_rect(buffer, 299, 174, 301, 174, 3);
}

void draw_nes_bezel(uint8_t *buffer) {
    // 1. Draw dithered blue background
    art_dither_rect_v(buffer, 0, 0, 319, 239, 4, 0, 240);

    // 2. Draw twinkling stars in the sky background
    for (int i = 0; i < 12; i++) {
        // Simple time/rand based twinkling using lines/score
        if ((tetris_lines + i) % 3 != 0) {
            draw_pixel(buffer, STAR_COORDS[i][0], STAR_COORDS[i][1], (i % 2 == 0) ? 7 : 3);
        }
    }

    // 3. Draw Saint Basil's Cathedral / Kremlin Towers
    draw_kremlin_graphics(buffer);

    // 4. Main title text
    draw_string(buffer, "NES TETRIS AI", 108, 12, 1, 7);

    // 5. Playfield Border & Bezel
    draw_rect(buffer, 117, 37, 202, 202, 7); // outer white
    draw_rect(buffer, 118, 38, 201, 201, 4); // inner blue
    draw_rect(buffer, 119, 39, 200, 200, 0); // black playfield interior

    // 6. LINES Box (Left)
    draw_rect(buffer, 30, 40, 100, 85, 7);
    draw_rect(buffer, 32, 42, 98, 83, 0);
    draw_string(buffer, "LINES", 48, 48, 1, 6);
    draw_score(buffer, tetris_lines, 42, 65, 1, 7);

    // 7. LEVEL Box (Left)
    draw_rect(buffer, 30, 105, 100, 150, 7);
    draw_rect(buffer, 32, 107, 98, 148, 0);
    draw_string(buffer, "LEVEL", 48, 113, 1, 6);
    draw_score(buffer, tetris_lines / 10, 42, 130, 1, 7);

    // 8. SCORE Box (Right)
    draw_rect(buffer, 220, 40, 290, 85, 7);
    draw_rect(buffer, 222, 42, 288, 83, 0);
    draw_string(buffer, "SCORE", 238, 48, 1, 6);
    draw_score(buffer, tetris_score, 226, 65, 1, 7);

    // 9. NEXT Box (Right)
    draw_rect(buffer, 220, 105, 290, 150, 7);
    draw_rect(buffer, 222, 107, 288, 148, 0);
    draw_string(buffer, "NEXT", 242, 113, 1, 6);

    // Draw Next Piece Centered in Next Box using beveled block style
    int ntype = next_piece_type;
    int px = 236;
    int py = 124;
    if (ntype == 0) { // I
        px -= 2;
        py -= 4;
    } else if (ntype == 1) { // O
        px += 2;
    } else { // T, S, Z, J, L
        px += 2;
    }
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (TETROMINOES[ntype][0][y][x]) {
                draw_beveled_block(buffer, px + x * 8, py + y * 8, TETROMINO_COLORS[ntype]);
            }
        }
    }
}

void play_tetris(uint8_t *buffer) {
    // Draw all background & NES bezel/boxes
    draw_nes_bezel(buffer);
    
    if (line_clear_anim_ticks > 0) {
        line_clear_anim_ticks--;
        for (int r = 0; r < 20; r++) {
            bool is_clearing = false;
            for (int i = 0; i < num_lines_to_clear; i++) {
                if (lines_to_clear[i] == r) { is_clearing = true; break; }
            }
            for (int c = 0; c < 10; c++) {
                uint8_t color = tetris_grid[r][c];
                if (color != 0) {
                    if (is_clearing && (line_clear_anim_ticks % 4 < 2)) {
                        // Flash white on clear
                        draw_rect(buffer, 120 + c * 8, 40 + r * 8, 120 + c * 8 + 7, 40 + r * 8 + 7, 7);
                    } else if (!is_clearing) {
                        draw_beveled_block(buffer, 120 + c * 8, 40 + r * 8, color);
                    }
                }
            }
        }
        if (line_clear_anim_ticks == 0) {
            for (int i = num_lines_to_clear - 1; i >= 0; i--) {
                int row_to_clear = lines_to_clear[i];
                for (int r = row_to_clear; r > 0; r--) {
                    memcpy(tetris_grid[r], tetris_grid[r - 1], 10);
                }
                memset(tetris_grid[0], 0, 10);
            }
            if (num_lines_to_clear == 1) tetris_score += 100;
            else if (num_lines_to_clear == 2) tetris_score += 300;
            else if (num_lines_to_clear == 3) tetris_score += 500;
            else if (num_lines_to_clear == 4) {
                tetris_score += 800;
                tetris_popup_ticks = 40;
            }
            
            tetris_lines += num_lines_to_clear;
            num_lines_to_clear = 0;
            spawn_tetris_piece();
        }
        return; 
    }
    
    tetris_tick_counter++;
    int tick_limit = 3;
    if (global_input.down || (curr_piece_col == ai_target_col && curr_piece_rot == ai_target_rot)) {
        tick_limit = 1;
    }
    if (tetris_tick_counter >= tick_limit) { 
        tetris_tick_counter = 0;
        
        if (curr_piece_rot != ai_target_rot) {
            int next_rot = (curr_piece_rot + 1) % 4;
            if (!tetris_collide(curr_piece_type, next_rot, curr_piece_row, curr_piece_col)) {
                curr_piece_rot = next_rot;
            }
        }
        if (curr_piece_col < ai_target_col) {
            if (!tetris_collide(curr_piece_type, curr_piece_rot, curr_piece_row, curr_piece_col + 1)) curr_piece_col++;
        } else if (curr_piece_col > ai_target_col) {
            if (!tetris_collide(curr_piece_type, curr_piece_rot, curr_piece_row, curr_piece_col - 1)) curr_piece_col--;
        }
        
        if (!tetris_collide(curr_piece_type, curr_piece_rot, curr_piece_row + 1, curr_piece_col)) {
            curr_piece_row++;
        } else {
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    if (TETROMINOES[curr_piece_type][curr_piece_rot][y][x]) {
                        int gr = curr_piece_row + y;
                        int gc = curr_piece_col + x;
                        if (gr >= 0 && gr < 20 && gc >= 0 && gc < 10) {
                            tetris_grid[gr][gc] = TETROMINO_COLORS[curr_piece_type];
                        }
                    }
                }
            }
            num_lines_to_clear = 0;
            for (int r = 19; r >= 0; r--) {
                bool row_full = true;
                for (int c = 0; c < 10; c++) {
                    if (tetris_grid[r][c] == 0) { row_full = false; break; }
                }
                if (row_full) lines_to_clear[num_lines_to_clear++] = r;
            }
            if (num_lines_to_clear > 0) {
                line_clear_anim_ticks = 16; 
            } else {
                bool game_over = false;
                for (int c = 0; c < 10; c++) {
                    if (tetris_grid[2][c] != 0) { game_over = true; break; }
                }
                if (game_over) {
                    memset(tetris_grid, 0, sizeof(tetris_grid));
                    tetris_score = 0;
                    tetris_lines = 0;
                }
                spawn_tetris_piece();
            }
        }
    }
    
    // Draw current board pieces as 3D beveled blocks
    for (int r = 0; r < 20; r++) {
        for (int c = 0; c < 10; c++) {
            uint8_t color = tetris_grid[r][c];
            if (color != 0) {
                draw_beveled_block(buffer, 120 + c * 8, 40 + r * 8, color);
            }
        }
    }
    
    // Draw falling piece as 3D beveled blocks
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (TETROMINOES[curr_piece_type][curr_piece_rot][y][x]) {
                int sx = 120 + (curr_piece_col + x) * 8;
                int sy = 40 + (curr_piece_row + y) * 8;
                if (sy >= 40) draw_beveled_block(buffer, sx, sy, TETROMINO_COLORS[curr_piece_type]);
            }
        }
    }
    
    // Draw flashing pop-up banner for Tetris clears
    if (tetris_popup_ticks > 0) {
        tetris_popup_ticks--;
        // Alternate colors of the banner and text for a flashing arcade look
        uint8_t bg_color = (tetris_popup_ticks % 8 < 4) ? 1 : 5;   // Flash between Red (1) and Magenta (5)
        uint8_t text_color = (tetris_popup_ticks % 4 < 2) ? 3 : 7; // Flash between Yellow (3) and White (7)
        
        // Draw banner box (centered inside the playfield, x: 120-199)
        draw_rect(buffer, 122, 110, 198, 130, 7); // white outer border
        draw_rect(buffer, 123, 111, 197, 129, bg_color);
        
        // Draw centered "TETRIS!" text
        draw_string(buffer, "TETRIS!", 132, 116, 1, text_color);
    }
}
