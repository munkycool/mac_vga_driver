#include "common.h"
#include <math.h>

// --- Minecraft Definitions ---
#define WORLD_X 32
#define WORLD_Y 32
#define WORLD_Z 8

static uint8_t world[WORLD_X][WORLD_Y][WORLD_Z];
static float player_x = 16.0f;
static float player_y = 16.0f;
static float player_z = 4.0f;
static float player_vz = 0.0f;
static float cam_angle = 0.0f;

// AI Target
static float target_x = 16.0f;
static float target_y = 16.0f;
static int target_timer = 0;

// Mob definition
typedef struct {
    float x, y, z;
    float vx, vy;
    int type; // 0: Herobrine, 1: Creeper, 2: Zombie
    bool active;
    int state_timer;
} Mob;

#define MAX_MOBS 5
static Mob mobs[MAX_MOBS];

// Sprites (16x8 pixels)
static const uint8_t HEROBRINE_SPRITE[16][8] = {
    {6,6,6,6,6,6,6,6},
    {6,6,6,6,6,6,6,6},
    {6,6,6,6,6,6,6,6},
    {6,7,0,6,6,0,7,6}, // White eyes!
    {6,6,6,6,6,6,6,6},
    {6,6,0,0,0,0,6,6},
    {3,3,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,3},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
};

static const uint8_t CREEPER_SPRITE[16][8] = {
    {2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2},
    {2,0,0,2,2,0,0,2},
    {2,0,0,2,2,0,0,2},
    {2,2,2,0,0,2,2,2},
    {2,2,0,0,0,0,2,2},
    {2,2,0,0,0,0,2,2},
    {2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2}
};

static const uint8_t ZOMBIE_SPRITE[16][8] = {
    {6,6,6,6,6,6,6,6},
    {2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2},
    {2,0,0,2,2,0,0,2},
    {2,2,2,2,2,2,2,2},
    {2,2,0,0,0,0,2,2},
    {3,3,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,3},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
};

// Z-depth buffer for sprite occlusion
static float z_depth[160];

static float get_terrain_height(float x, float y) {
    if (x < 0) x = 0; if (x >= WORLD_X) x = WORLD_X - 1;
    if (y < 0) y = 0; if (y >= WORLD_Y) y = WORLD_Y - 1;
    
    // Find the highest solid block
    for (int z = WORLD_Z - 1; z >= 0; z--) {
        if (world[(int)x][(int)y][z] != 0 && world[(int)x][(int)y][z] != 1) { // 1 is water
            return (float)z + 1.0f;
        }
    }
    return 0.0f;
}

void init_minecraft() {
    // Generate randomized terrain
    for (int x = 0; x < WORLD_X; x++) {
        for (int y = 0; y < WORLD_Y; y++) {
            // Build terrain height using sine waves
            float fx = (float)x * 0.2f;
            float fy = (float)y * 0.2f;
            int h = 3 + (int)(sinf(fx) * 1.5f + cosf(fy) * 1.5f);
            if (h < 1) h = 1;
            if (h >= WORLD_Z - 2) h = WORLD_Z - 3;
            
            for (int z = 0; z < WORLD_Z; z++) {
                if (z < h - 2) {
                    world[x][y][z] = 5; // Stone
                } else if (z < h - 1) {
                    world[x][y][z] = 6; // Dirt
                } else if (z < h) {
                    world[x][y][z] = 2; // Grass
                } else if (z <= 2) {
                    world[x][y][z] = 1; // Water
                } else {
                    world[x][y][z] = 0; // Air
                }
            }
        }
    }
    
    // Add randomized trees
    for (int i = 0; i < 15; i++) {
        int tx = 2 + (get_rand() % (WORLD_X - 4));
        int ty = 2 + (get_rand() % (WORLD_Y - 4));
        int th = (int)get_terrain_height((float)tx, (float)ty);
        if (th > 2 && world[tx][ty][th-1] == 2) { // Grass
            // Trunk
            for (int tz = th; tz < th + 3 && tz < WORLD_Z; tz++) {
                world[tx][ty][tz] = 6; // Wood
            }
            // Leaves
            for (int lx = tx - 1; lx <= tx + 1; lx++) {
                for (int ly = ty - 1; ly <= ty + 1; ly++) {
                    for (int lz = th + 2; lz < th + 4 && lz < WORLD_Z; lz++) {
                        if (lx == tx && ly == ty && lz == th + 2) continue; // Trunk
                        world[lx][ly][lz] = 2; // Leaves (green)
                    }
                }
            }
        }
    }
    
    // Spawn mobs
    for (int i = 0; i < MAX_MOBS; i++) {
        mobs[i].active = true;
        mobs[i].type = i % 3; // Herobrine, Creeper, Zombie
        mobs[i].x = 5.0f + (get_rand() % (WORLD_X - 10));
        mobs[i].y = 5.0f + (get_rand() % (WORLD_Y - 10));
        mobs[i].z = get_terrain_height(mobs[i].x, mobs[i].y);
        mobs[i].vx = 0.0f;
        mobs[i].vy = 0.0f;
        mobs[i].state_timer = 0;
    }
    
    // Initial Player position
    player_x = 16.0f;
    player_y = 16.0f;
    player_z = get_terrain_height(player_x, player_y) + 1.6f;
    player_vz = 0.0f;
    cam_angle = 0.0f;
    
    // Initial AI Target
    target_x = 16.0f;
    target_y = 16.0f;
    target_timer = 0;
}

// Simple fast column renderer
static void draw_retro_column(uint8_t *buffer, int col, int y_start, int y_end, uint8_t color, int shading) {
    if (col < 0 || col >= 160) return;
    if (y_start < 0) y_start = 0;
    if (y_end >= 120) y_end = 119;
    
    uint8_t final_color = color;
    
    for (int y = y_start; y <= y_end; y++) {
        // Apply Shading
        if (shading == 1) { // Medium
            if ((col + y) % 2 == 0) final_color = 0; // Blend to black
        } else if (shading == 2) { // Dark
            if ((col + y) % 4 != 0) final_color = 0;
        } else if (shading == 3) { // Fog / Sky blend
            if ((col + y) % 2 == 0) final_color = 3; // Blend to Sky (Cyan)
        } else if (shading == 4) { // Heavy Fog
            final_color = 3;
        }
        
        uint8_t pixel = (final_color & 0x07) | 0x08;
        uint8_t val = (pixel << 4) | pixel;
        
        int fb_y1 = y * 2;
        int fb_y2 = y * 2 + 1;
        buffer[fb_y1 * 320 + col * 2] = val;
        buffer[fb_y1 * 320 + col * 2 + 1] = val;
        buffer[fb_y2 * 320 + col * 2] = val;
        buffer[fb_y2 * 320 + col * 2 + 1] = val;
    }
}

void play_minecraft(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);
    
    // --- 1. UPDATE AI CAMERA ---
    target_timer++;
    float dx = target_x - player_x;
    float dy = target_y - player_y;
    float dist_to_target = sqrtf(dx*dx + dy*dy);
    
    if (dist_to_target < 2.0f || target_timer > 200) {
        // Pick new random target
        target_x = 4.0f + (get_rand() % (WORLD_X - 8));
        target_y = 4.0f + (get_rand() % (WORLD_Y - 8));
        target_timer = 0;
    }
    
    // Rotate camera towards target
    float target_angle = atan2f(target_y - player_y, target_x - player_x);
    float angle_diff = target_angle - cam_angle;
    while (angle_diff < -M_PI) angle_diff += 2.0f * M_PI;
    while (angle_diff > M_PI) angle_diff -= 2.0f * M_PI;
    
    cam_angle += angle_diff * 0.05f;
    
    // Move forward
    float move_speed = 0.06f;
    float next_x = player_x + cosf(cam_angle) * move_speed;
    float next_y = player_y + sinf(cam_angle) * move_speed;
    
    // Collision detection
    float floor_z = get_terrain_height(next_x, next_y);
    float current_floor_z = get_terrain_height(player_x, player_y);
    
    if (floor_z - current_floor_z <= 1.0f) {
        // Auto-jump if walking up a slab
        if (floor_z > current_floor_z && player_vz == 0.0f) {
            player_vz = 0.18f;
        }
        player_x = next_x;
        player_y = next_y;
    } else {
        // High obstacle: force picking a new target
        target_timer = 999;
    }
    
    // Apply gravity
    player_z += player_vz;
    player_vz -= 0.02f;
    
    float p_floor = get_terrain_height(player_x, player_y);
    if (player_z < p_floor + 1.6f) {
        player_z = p_floor + 1.6f;
        player_vz = 0.0f;
    }
    
    // Interaction AI: occasionally break block in front
    if ((get_rand() % 150) == 0) {
        int bx = (int)(player_x + cosf(cam_angle) * 1.5f);
        int by = (int)(player_y + sinf(cam_angle) * 1.5f);
        int bz = (int)(player_z - 0.5f);
        if (bx >= 0 && bx < WORLD_X && by >= 0 && by < WORLD_Y && bz >= 0 && bz < WORLD_Z) {
            if (world[bx][by][bz] != 0 && world[bx][by][bz] != 1) { // Break solid block
                world[bx][by][bz] = 0;
            }
        }
    }
    
    // Interaction AI: occasionally place block
    if ((get_rand() % 250) == 0) {
        int bx = (int)(player_x + cosf(cam_angle) * 2.0f);
        int by = (int)(player_y + sinf(cam_angle) * 2.0f);
        int bz = (int)(player_z - 1.0f);
        if (bx >= 0 && bx < WORLD_X && by >= 0 && by < WORLD_Y && bz >= 0 && bz < WORLD_Z) {
            if (world[bx][by][bz] == 0) {
                world[bx][by][bz] = 6; // Place dirt
            }
        }
    }
    
    // --- 2. UPDATE MOBS ---
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active) continue;
        
        mobs[i].state_timer++;
        
        if (mobs[i].type == 0) { // Herobrine Stalking AI
            float mdx = player_x - mobs[i].x;
            float mdy = player_y - mobs[i].y;
            float mdist = sqrtf(mdx*mdx + mdy*mdy);
            
            if (mdist < 3.0f || mobs[i].state_timer > 300) {
                // Teleport away out of direct sight
                float tp_ang = cam_angle + M_PI + ((get_rand() % 100) - 50) * 0.01f;
                float tp_dist = 6.0f + (get_rand() % 8);
                mobs[i].x = player_x + cosf(tp_ang) * tp_dist;
                mobs[i].y = player_y + sinf(tp_ang) * tp_dist;
                if (mobs[i].x < 1) mobs[i].x = 1; if (mobs[i].x >= WORLD_X - 1) mobs[i].x = WORLD_X - 2;
                mobs[i].y = player_y + sinf(tp_ang) * tp_dist;
                if (mobs[i].y < 1) mobs[i].y = 1; if (mobs[i].y >= WORLD_Y - 1) mobs[i].y = WORLD_Y - 2;
                mobs[i].state_timer = 0;
            } else {
                // Stand still and face player view
                mobs[i].vx = 0;
                mobs[i].vy = 0;
            }
        } else { // Creeper / Zombie wandering
            if (mobs[i].state_timer > 60) {
                float wander_ang = (get_rand() % 628) * 0.01f;
                mobs[i].vx = cosf(wander_ang) * 0.03f;
                mobs[i].vy = sinf(wander_ang) * 0.03f;
                mobs[i].state_timer = 0;
            }
            
            mobs[i].x += mobs[i].vx;
            mobs[i].y += mobs[i].vy;
            
            // Stay inside world bounds
            if (mobs[i].x < 1.0f) { mobs[i].x = 1.0f; mobs[i].vx = -mobs[i].vx; }
            if (mobs[i].x >= WORLD_X - 1.0f) { mobs[i].x = WORLD_X - 1.0f; mobs[i].vx = -mobs[i].vx; }
            if (mobs[i].y < 1.0f) { mobs[i].y = 1.0f; mobs[i].vy = -mobs[i].vy; }
            if (mobs[i].y >= WORLD_Y - 1.0f) { mobs[i].y = WORLD_Y - 1.0f; mobs[i].vy = -mobs[i].vy; }
        }
        
        mobs[i].z = get_terrain_height(mobs[i].x, mobs[i].y);
    }
    
    // --- 3. RENDER VOXEL WORLD ---
    // Initialize depth buffer
    for (int col = 0; col < 160; col++) {
        z_depth[col] = 999.0f;
    }
    
    // Render column by column
    for (int col = 0; col < 160; col++) {
        float fov = 1.0f; // field of view
        float ray_angle = cam_angle + ((float)col - 80.0f) * (fov / 160.0f);
        float cos_r = cosf(ray_angle);
        float sin_r = sinf(ray_angle);
        
        // Raycasting variables
        float rx = player_x;
        float ry = player_y;
        
        int map_x = (int)rx;
        int map_y = (int)ry;
        
        float delta_dist_x = (cos_r == 0) ? 1e30f : fabsf(1.0f / cos_r);
        float delta_dist_y = (sin_r == 0) ? 1e30f : fabsf(1.0f / sin_r);
        
        int step_x, step_y;
        float side_dist_x, side_dist_y;
        
        if (cos_r < 0) {
            step_x = -1;
            side_dist_x = (rx - map_x) * delta_dist_x;
        } else {
            step_x = 1;
            side_dist_x = (map_x + 1.0f - rx) * delta_dist_x;
        }
        
        if (sin_r < 0) {
            step_y = -1;
            side_dist_y = (ry - map_y) * delta_dist_y;
        } else {
            step_y = 1;
            side_dist_y = (map_y + 1.0f - ry) * delta_dist_y;
        }
        
        // Front-to-back occlusion
        bool filled[120];
        memset(filled, 0, sizeof(filled));
        int filled_count = 0;
        
        float dist = 0;
        int hit_side = 0;
        
        for (int step = 0; step < 26; step++) {
            if (side_dist_x < side_dist_y) {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                hit_side = 0;
            } else {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                hit_side = 1;
            }
            
            if (map_x < 0 || map_x >= WORLD_X || map_y < 0 || map_y >= WORLD_Y) break;
            
            if (hit_side == 0) {
                dist = side_dist_x - delta_dist_x;
            } else {
                dist = side_dist_y - delta_dist_y;
            }
            if (dist < 0.1f) dist = 0.1f;
            
            // Fish-eye correction
            float corr_dist = dist * cosf(ray_angle - cam_angle);
            
            // Check all blocks in this column
            for (int z = 0; z < WORLD_Z; z++) {
                uint8_t block = world[map_x][map_y][z];
                if (block != 0) {
                    // Project vertical bounds of block
                    float scale_y = 65.0f;
                    int y_bottom = 60 - (int)(((float)z - player_z) * scale_y / corr_dist);
                    int y_top = 60 - (int)(((float)z + 1.0f - player_z) * scale_y / corr_dist);
                    
                    if (y_top < 0) y_top = 0;
                    if (y_bottom >= 120) y_bottom = 119;
                    
                    if (y_top <= y_bottom) {
                        // Store the depth of the closest solid block in this column
                        if (z_depth[col] > dist) {
                            z_depth[col] = dist;
                        }
                        
                        // Shading based on side hit and distance fog
                        int shading = hit_side; // 0 or 1
                        if (corr_dist > 12.0f) {
                            shading = 4; // Heavy fog
                        } else if (corr_dist > 7.0f) {
                            shading = 3; // Light fog
                        }
                        
                        // Draw column slice
                        int slice_start = -1;
                        for (int y = y_top; y <= y_bottom; y++) {
                            if (!filled[y]) {
                                if (slice_start == -1) slice_start = y;
                                filled[y] = true;
                                filled_count++;
                            } else {
                                if (slice_start != -1) {
                                    draw_retro_column(buffer, col, slice_start, y - 1, block, shading);
                                    slice_start = -1;
                                }
                            }
                        }
                        if (slice_start != -1) {
                            draw_retro_column(buffer, col, slice_start, y_bottom, block, shading);
                        }
                    }
                }
            }
            
            if (filled_count >= 120) break; // Entire column is filled
        }
        
        // Draw sky for any unfilled pixels
        int sky_start = -1;
        for (int y = 0; y < 120; y++) {
            if (!filled[y]) {
                if (sky_start == -1) sky_start = y;
            } else {
                if (sky_start != -1) {
                    draw_retro_column(buffer, col, sky_start, y - 1, 3, 0); // Cyan Sky
                    sky_start = -1;
                }
            }
        }
        if (sky_start != -1) {
            draw_retro_column(buffer, col, sky_start, 119, 3, 0);
        }
    }
    
    // --- 4. RENDER BILLBOARD MOBS ---
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active) continue;
        
        // Vector from camera to mob
        float mx = mobs[i].x - player_x;
        float my = mobs[i].y - player_y;
        
        // Rotate into camera space
        float rx = mx * cosf(-cam_angle) - my * sinf(-cam_angle); // Depth
        float ry = mx * sinf(-cam_angle) + my * cosf(-cam_angle); // Horizontal offset
        
        if (rx < 0.2f || rx > 20.0f) continue; // Behind camera or too far
        
        // Screen projection
        float scale = 65.0f;
        int screen_x = 80 + (int)(ry * scale / rx);
        int screen_y_bottom = 60 - (int)((mobs[i].z - player_z) * scale / rx);
        int screen_y_top = 60 - (int)((mobs[i].z + 1.8f - player_z) * scale / rx);
        
        int mob_h = screen_y_bottom - screen_y_top;
        if (mob_h <= 0) continue;
        int mob_w = mob_h / 2;
        if (mob_w <= 0) continue;
        
        int start_x = screen_x - mob_w / 2;
        int end_x = screen_x + mob_w / 2;
        
        // Draw scaled sprite
        for (int sx = start_x; sx <= end_x; sx++) {
            if (sx < 0 || sx >= 160) continue;
            
            // Check depth buffer
            if (rx > z_depth[sx]) continue; // Occluded by terrain
            
            int sprite_col = ((sx - start_x) * 8) / mob_w;
            if (sprite_col < 0 || sprite_col >= 8) continue;
            
            for (int sy = screen_y_top; sy <= screen_y_bottom; sy++) {
                if (sy < 0 || sy >= 120) continue;
                
                int sprite_row = ((sy - screen_y_top) * 16) / mob_h;
                if (sprite_row < 0 || sprite_row >= 16) continue;
                
                uint8_t pixel_color = 0;
                if (mobs[i].type == 0) {
                    pixel_color = HEROBRINE_SPRITE[sprite_row][sprite_col];
                } else if (mobs[i].type == 1) {
                    pixel_color = CREEPER_SPRITE[sprite_row][sprite_col];
                } else if (mobs[i].type == 2) {
                    pixel_color = ZOMBIE_SPRITE[sprite_row][sprite_col];
                }
                
                draw_retro_column(buffer, sx, sy, sy, pixel_color, 0);
            }
        }
    }
}
