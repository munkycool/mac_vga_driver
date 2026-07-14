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

void tetris_run_ai() {
    int best_score = -999999;
    ai_target_col = 3;
    ai_target_rot = 0;

    for (int r = 0; r < 4; r++) {
        for (int c = -2; c < 9; c++) {
            if (tetris_collide(curr_piece_type, r, 0, c)) continue; 
            int land_r = get_land_row(curr_piece_type, r, c);
            if (land_r < 0) continue; 
            
            uint8_t temp_grid[20][10];
            memcpy(temp_grid, tetris_grid, sizeof(tetris_grid));
            
            for (int py = 0; py < 4; py++) {
                for (int px = 0; px < 4; px++) {
                    if (TETROMINOES[curr_piece_type][r][py][px]) {
                        int gr = land_r + py;
                        int gc = c + px;
                        if (gr >= 0 && gr < 20 && gc >= 0 && gc < 10) {
                            temp_grid[gr][gc] = 1; 
                        }
                    }
                }
            }
            
            int landing_height = 20 - land_r; 
            int lines_cleared = 0;
            for (int ty = 0; ty < 20; ty++) {
                bool full = true;
                for (int tx = 0; tx < 10; tx++) {
                    if (temp_grid[ty][tx] == 0) { full = false; break; }
                }
                if (full) lines_cleared++;
            }
            
            int holes = 0;
            for (int tx = 0; tx < 10; tx++) {
                bool block_above = false;
                for (int ty = 0; ty < 20; ty++) {
                    if (temp_grid[ty][tx] != 0) block_above = true;
                    else if (block_above && temp_grid[ty][tx] == 0) holes++;
                }
            }
            
            int r_bumpiness = 0;
            int col_heights[10] = {0};
            for (int tx = 0; tx < 10; tx++) {
                for (int ty = 0; ty < 20; ty++) {
                    if (temp_grid[ty][tx] != 0) {
                        col_heights[tx] = 20 - ty;
                        break;
                    }
                }
            }
            for (int tx = 0; tx < 9; tx++) {
                r_bumpiness += abs(col_heights[tx] - col_heights[tx + 1]);
            }
            
            int score = (-5 * landing_height) - (35 * holes) - (18 * r_bumpiness);
            if (lines_cleared == 4) {
                score += 12000;
            } else if (lines_cleared > 0) {
                int max_height = 0;
                for (int tx = 0; tx < 10; tx++) {
                    if (col_heights[tx] > max_height) max_height = col_heights[tx];
                }
                if (max_height > 12) {
                    score += lines_cleared * 200;
                } else {
                    score += lines_cleared * 10; // stack up, do not clear small lines!
                }
            }

            // Reward keeping the well (column 9) empty
            bool well_empty = true;
            for (int ty = 6; ty < 20; ty++) {
                if (temp_grid[ty][9] != 0) {
                    well_empty = false;
                    break;
                }
            }
            if (well_empty) {
                score += 150;
            }

            if (score > best_score) {
                best_score = score;
                ai_target_col = c;
                ai_target_rot = r;
            }
        }
    }
}

void spawn_tetris_piece() {
    curr_piece_type = get_rand() % 7;
    curr_piece_row = -1;
    curr_piece_col = 3;
    curr_piece_rot = 0;
    tetris_run_ai();
}

void init_tetris() {
    memset(tetris_grid, 0, sizeof(tetris_grid));
    for (int r = 16; r < 20; r++) {
        for (int c = 0; c < 10; c++) {
            if ((c + r) % 3 != 0) {
                tetris_grid[r][c] = (c % 6) + 1;
            }
        }
    }
    tetris_score = 0;
    tetris_tick_counter = 0;
    line_clear_anim_ticks = 0;
    num_lines_to_clear = 0;
    spawn_tetris_piece();
}

void play_tetris(uint8_t *buffer) {
    clear_screen(buffer);
    draw_score(buffer, tetris_score, 230, 40, 2, 7);
    
    draw_rect(buffer, 118, 38, 120, 202, 7); 
    draw_rect(buffer, 200, 38, 202, 202, 7); 
    draw_rect(buffer, 118, 200, 202, 202, 7); 
    
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
                        draw_rect(buffer, 121 + c * 8, 40 + r * 8, 121 + c * 8 + 6, 40 + r * 8 + 6, 7);
                    } else if (!is_clearing) {
                        draw_rect(buffer, 121 + c * 8, 40 + r * 8, 121 + c * 8 + 6, 40 + r * 8 + 6, color);
                    }
                }
            }
        }
        if (line_clear_anim_ticks == 0) {
            for (int i = 0; i < num_lines_to_clear; i++) {
                int row_to_clear = lines_to_clear[i];
                for (int r = row_to_clear; r > 0; r--) {
                    memcpy(tetris_grid[r], tetris_grid[r - 1], 10);
                }
                memset(tetris_grid[0], 0, 10);
            }
            if (num_lines_to_clear == 1) tetris_score += 100;
            else if (num_lines_to_clear == 2) tetris_score += 300;
            else if (num_lines_to_clear == 3) tetris_score += 500;
            else if (num_lines_to_clear == 4) tetris_score += 800;
            num_lines_to_clear = 0;
            spawn_tetris_piece();
        }
        return; 
    }
    
    tetris_tick_counter++;
    if (tetris_tick_counter >= 3) { 
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
                }
                spawn_tetris_piece();
            }
        }
    }
    
    for (int r = 0; r < 20; r++) {
        for (int c = 0; c < 10; c++) {
            uint8_t color = tetris_grid[r][c];
            if (color != 0) {
                draw_rect(buffer, 121 + c * 8, 40 + r * 8, 121 + c * 8 + 6, 40 + r * 8 + 6, color);
            }
        }
    }
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (TETROMINOES[curr_piece_type][curr_piece_rot][y][x]) {
                int sx = 121 + (curr_piece_col + x) * 8;
                int sy = 40 + (curr_piece_row + y) * 8;
                if (sy >= 40) draw_rect(buffer, sx, sy, sx + 6, sy + 6, TETROMINO_COLORS[curr_piece_type]);
            }
        }
    }
}
