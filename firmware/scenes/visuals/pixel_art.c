#include "common.h"
#include <math.h>

// =============================================================================
//  PIXEL ART — 4K ULTRA HD PRO PLUS ULTRA EDITION
//  Color palette: 0=Black  1=Red  2=Green  3=Yellow  4=Blue  5=Magenta  6=Cyan  7=White
//  Screen: 320×240  Buffer format: nibble-packed (draw_pixel handles it)
//  Rendering: ordered Bayer dithering + block upscaling via art_* engine
// =============================================================================

// ---------------------------------------------------------------------------
//  Twinkling Stars (extended to 40 stars, multi-size)
// ---------------------------------------------------------------------------
static const int16_t STAR_X[40] = {
    25,45,80,110,130,155,185,210,240,275,295,310,15,60,95,120,200,225,260,290,
    35,55,75,100,145,170,195,215,250,285,305,10,40,65,105,140,180,235,265,300
};
static const int16_t STAR_Y[40] = {
    20,45,30,25,60,15,40,20,50,35,55,20,70,90,85,75,80,95,65,85,
    12,38,55,48,22,70,32,58,42,18,62,44,28,72,52,36,68,26,78,14
};
// Size tier: 0=dot, 1=cross (+), 2=fat
static const uint8_t STAR_SZ[40] = {
    0,1,0,0,1,0,2,0,1,0,0,1,0,0,1,0,1,0,0,2,
    0,0,1,0,0,1,0,0,1,0,0,0,1,0,0,1,0,1,0,0
};

static void draw_stars(uint8_t *buf, int frame) {
    for (int i = 0; i < 40; i++) {
        int phase = (frame + i * 13) % 48;
        if (phase >= 40) continue; // off period
        uint8_t col = (phase < 36) ? 7 : 6; // white or dim cyan
        int x = STAR_X[i], y = STAR_Y[i];
        draw_pixel(buf, x, y, col);
        if (STAR_SZ[i] >= 1) {
            draw_pixel(buf, x-1, y, col);
            draw_pixel(buf, x+1, y, col);
            draw_pixel(buf, x, y-1, col);
            draw_pixel(buf, x, y+1, col);
        }
        if (STAR_SZ[i] >= 2) {
            draw_pixel(buf, x-1, y-1, 6);
            draw_pixel(buf, x+1, y-1, 6);
            draw_pixel(buf, x-1, y+1, 6);
            draw_pixel(buf, x+1, y+1, 6);
        }
    }
}

// ---------------------------------------------------------------------------
//  Lamborghini Countach — completely redrawn at scale 4, much more detail
// ---------------------------------------------------------------------------
#define CAR_SCALE 5

static void draw_car_pixel(uint8_t *buf, int cx, int cy, int dx, int dy, uint8_t col, bool right) {
    int rx = right ? dx : -dx;
    int px = cx + rx * CAR_SCALE;
    int py = cy + dy * CAR_SCALE;
    draw_rect(buf, px, py, px + CAR_SCALE - 1, py + CAR_SCALE - 1, col);
}

static void draw_car_rect(uint8_t *buf, int cx, int cy, int dx1, int dy1, int dx2, int dy2,
                           uint8_t col, bool right) {
    int sdx = dx1 < dx2 ? dx1 : dx2;
    int edx = dx1 > dx2 ? dx1 : dx2;
    for (int dy = dy1; dy <= dy2; dy++)
        for (int dx = sdx; dx <= edx; dx++)
            draw_car_pixel(buf, cx, cy, dx, dy, col, right);
}

// Shadow pixel: a darker version via dithering with black
static void draw_car_shadow(uint8_t *buf, int cx, int cy, int dx, int dy, bool right) {
    int rx = right ? dx : -dx;
    int px = cx + rx * CAR_SCALE;
    int py = cy + dy * CAR_SCALE;
    for (int y = py; y < py + CAR_SCALE; y++)
        for (int x = px; x < px + CAR_SCALE; x++)
            art_dither_pixel(buf, x, y, 0, 2); // 50% black over body color
}

static void draw_rear_countach(uint8_t *buf, int cx, int cy, int frame) {
    // Left & Right Tires (Black 0)
    draw_rect(buf, cx - 42, cy - 14, cx - 30, cy, 0);
    draw_rect(buf, cx + 30, cy - 14, cx + 42, cy, 0);

    // Rear bumper & diffusers (Dark Blue 4 / Black 0)
    draw_rect(buf, cx - 38, cy - 6, cx + 38, cy, 4);
    draw_rect(buf, cx - 35, cy - 4, cx + 35, cy, 0);

    // Quad Exhaust Pipes (Silver 7 / Orange 3 glow)
    draw_rect(buf, cx - 26, cy - 3, cx - 23, cy - 1, 7);
    draw_rect(buf, cx - 21, cy - 3, cx - 18, cy - 1, 7);
    draw_pixel(buf, cx - 25, cy - 2, 0);
    draw_pixel(buf, cx - 20, cy - 2, 0);
    draw_rect(buf, cx + 18, cy - 3, cx + 21, cy - 1, 7);
    draw_rect(buf, cx + 23, cy - 3, cx + 26, cy - 1, 7);
    draw_pixel(buf, cx + 20, cy - 2, 0);
    draw_pixel(buf, cx + 25, cy - 2, 0);

    // Main rear body wedge (Red 1 body dithered with Magenta 5)
    for (int y = cy - 24; y <= cy - 6; y++) {
        int w = 40 - (cy - 6 - y) * 2 / 5; // taper outwards to bottom
        draw_rect(buf, cx - w, y, cx + w, y, 1);
        // Shadow/dither edges
        art_dither_pixel(buf, cx - w, y, 5, 2);
        art_dither_pixel(buf, cx + w, y, 5, 2);
    }

    // Engine Cover Louvers (Black slots on the engine deck)
    for (int y = cy - 22; y <= cy - 16; y += 2) {
        draw_rect(buf, cx - 24, y, cx + 24, y, 0);
    }

    // Taillight assembly panel (Black 0 backing)
    draw_rect(buf, cx - 35, cy - 14, cx + 35, cy - 8, 0);

    // Glowing red taillights (red 1 with yellow 3 cores)
    draw_rect(buf, cx - 32, cy - 13, cx - 16, cy - 9, 1);
    draw_rect(buf, cx - 28, cy - 12, cx - 20, cy - 10, 3);
    draw_rect(buf, cx + 16, cy - 13, cx + 32, cy - 9, 1);
    draw_rect(buf, cx + 20, cy - 12, cx + 28, cy - 10, 3);

    // License Plate (White 7) - Cycles every 120 frames (~2 seconds)
    const char *plates[] = {
        "COUNTACH",
        "NPP",
        "PNL",
        "I SEE YOU"
    };
    const char *plate_text = plates[(frame / 120) % 4];
    int len = 0;
    while (plate_text[len] != '\0') len++;
    int text_width = len * 4 - 1;
    int half_w = (text_width + 1) / 2;
    int plate_w = half_w > 10 ? half_w : 10;

    draw_rect(buf, cx - plate_w, cy - 13, cx + plate_w, cy - 9, 7);
    draw_string(buf, plate_text, cx - text_width / 2, cy - 12, 1, 0); // black text

    // Huge Rear Spoiler/Wing (Red 1)
    draw_rect(buf, cx - 46, cy - 32, cx + 46, cy - 28, 1);
    draw_rect(buf, cx - 47, cy - 34, cx - 45, cy - 22, 1);
    draw_rect(buf, cx + 45, cy - 34, cx + 47, cy - 22, 1);
    draw_rect(buf, cx - 20, cy - 28, cx - 18, cy - 24, 1);
    draw_rect(buf, cx + 18, cy - 28, cx + 20, cy - 24, 1);

    // Specular body sheen highlights (White 7 / Cyan 6)
    draw_rect(buf, cx - 32, cy - 24, cx + 32, cy - 24, 6);
    draw_pixel(buf, cx - 30, cy - 24, 7);
    draw_pixel(buf, cx + 30, cy - 24, 7);

    // Rear cockpit glass (Black 0 with cyan 6 highlights)
    draw_rect(buf, cx - 18, cy - 32, cx + 18, cy - 26, 0);
    draw_rect(buf, cx - 15, cy - 31, cx + 15, cy - 30, 6);
}

static void draw_art_line(uint8_t *buf, int x0, int y0, int x1, int y1, uint8_t col) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        draw_pixel(buf, x0, y0, col);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void draw_countach_palm(uint8_t *buf, int cx, int ground_y, int trunk_h,
                               int crown_size, int sway, bool right_side) {
    int lean_dir = right_side ? 1 : -1;
    int trunk_w = trunk_h / 10 + 1;
    if (trunk_w < 1) trunk_w = 1;

    // Draw trunk with segments and tapering
    for (int i = 0; i < trunk_h; i++) {
        float t = (float)i / (float)trunk_h;
        int y = ground_y - i;
        // Quadratic bend for a more natural look
        int bend = (int)(t * t * 18.0f * lean_dir);
        // Subtle wind sway on the trunk too
        int wind = (int)(sinf(sway * 0.05f + i * 0.1f) * (t * 2.0f));
        int x = cx + bend + wind;

        int current_w = trunk_w - (i * trunk_w) / (trunk_h + 8);
        if (current_w < 1) current_w = 1;

        // Trunk body: Black (0)
        draw_rect(buf, x - current_w, y, x + current_w, y, 0);
        
        // Bark texture/segments: use Yellow (3) for highlights instead of just Blue (4)
        if (i % 5 == 0) {
            draw_rect(buf, x - current_w, y, x + current_w, y, 4); // Blue segment base
            draw_pixel(buf, x, y, 3); // Yellow knot detail
        }
        
        // Highlights on the outside of the curve
        if (right_side) {
            draw_pixel(buf, x + current_w, y, 6); // Cyan edge
        } else {
            draw_pixel(buf, x - current_w, y, 6); // Cyan edge
        }
    }

    int crown_cx = cx + (int)(18.0f * lean_dir);
    int crown_y = ground_y - trunk_h;

    // Coconuts (Red cluster)
    draw_pixel(buf, crown_cx, crown_y + 1, 1);
    draw_pixel(buf, crown_cx + 1, crown_y + 1, 1);
    draw_pixel(buf, crown_cx, crown_y + 2, 5);

    // Draw 9 detailed, curved fronds (increased from 7)
    for (int f = 0; f < 9; f++) {
        // Spread fronds around the top
        float base_angle = (float)f * (3.14159f * 2.0f / 9.0f);
        // Shift angles so they aren't perfectly symmetrical
        base_angle += (f % 3) * 0.15f;
        
        float sway_angle = sinf(sway * 0.05f + f) * 0.15f;
        float angle = base_angle + sway_angle;
        
        int len = crown_size + 8 + (f % 5);
        int last_x = crown_cx;
        int last_y = crown_y;

        for (int step = 1; step <= len; step++) {
            float st = (float)step / (float)len;
            // Fronds curve downwards more as they get longer
            float spread = 1.2f + (float)(f % 3) * 0.1f;
            int dx = (int)(cosf(angle) * step * spread);
            // More dramatic downward curve
            int dy = (int)(sinf(angle) * step + st * st * 15.0f);
            
            int px = crown_cx + dx;
            int py = crown_y + dy;

            if (px >= 0 && px < 320 && py >= 0 && py < 240) {
                // Main spine: Green (2)
                draw_art_line(buf, last_x, last_y, px, py, 2); 
                
                // Detailed leaflets
                if (step % 2 == 0) {
                    // Draw small perpendicular ticks for leaves
                    int lx = px + (int)(sinf(angle) * 2.0f);
                    int ly = py - (int)(cosf(angle) * 2.0f);
                    draw_pixel(buf, lx, ly, 2);
                    
                    // Sun highlight on the upper part of the frond
                    if (py < crown_y + 4) {
                        draw_pixel(buf, px, py - 1, 6); // Cyan
                    } else if (st < 0.5f) {
                        draw_pixel(buf, px, py - 1, 3); // Yellow
                    }
                }
            }
            last_x = px;
            last_y = py;
        }
        // Little leaf tips: White (7)
        draw_pixel(buf, last_x, last_y, 7);
    }
    
    // Crown center: use a small magenta nut
    draw_circle(buf, crown_cx, crown_y, 2, 0);
    draw_pixel(buf, crown_cx, crown_y, 5);
}

// ============================================================================
//  STATE 1: Synthwave Sunset — ENHANCED
//  New: multi-layer dithered sky, hex grid floor, scanline glow, rear-view car
// ============================================================================
void init_synthwave() {}

void play_synthwave(uint8_t *buf, int frame) {
    // ── SKY GRADIENT: top=black, mid=blue, horizon=magenta ───────────────────
    for (int y = 0; y < 100; y++) {
        // Upper sky: black -> blue
        int d = (y * 15) / 100;
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            draw_pixel(buf, x, y, (d >= t) ? 4 : 0);
        }
    }
    for (int y = 100; y < 140; y++) {
        // Lower sky: blue -> magenta
        int d = ((y - 100) * 15) / 40;
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            draw_pixel(buf, x, y, (d >= t) ? 5 : 4);
        }
    }

    // ── TWINKLING STARS ───────────────────────────────────────────────────────
    draw_stars(buf, frame);

    // ── SUN — dithered vertical gradient and retro widening gaps ─────────────
    int sun_cx = 160, sun_cy = 105, sun_r = 42;
    for (int y = sun_cy - sun_r; y <= sun_cy + sun_r; y++) {
        int dy = y - sun_cy;
        
        // Widening horizontal scanline gaps in the lower half
        if (dy > 0) {
            if (dy % 8 < (dy / 9) + 1) {
                continue;
            }
        }
        
        int dx_max = isqrt(sun_r * sun_r - dy * dy);
        int y_rel = y - (sun_cy - sun_r); // 0 at top, 84 at bottom
        
        for (int x = sun_cx - dx_max; x <= sun_cx + dx_max; x++) {
            int t = art_bayer4_at(x, y);
            uint8_t col;
            if (y_rel < 20) {
                // White (7) to Yellow (3)
                int val = y_rel * 16 / 20;
                col = (val >= t) ? 3 : 7;
            } else if (y_rel < 50) {
                // Yellow (3) to Red (1)
                int val = (y_rel - 20) * 16 / 30;
                col = (val >= t) ? 1 : 3;
            } else {
                // Red (1) to Magenta (5)
                int val = (y_rel - 50) * 16 / 34;
                col = (val >= t) ? 5 : 1;
            }
            draw_pixel(buf, x, y, col);
        }
    }

    // ── HORIZON GLOW ─────────────────────────────────────────────────────────
    draw_rect(buf, 0, 138, 319, 138, 7);
    draw_rect(buf, 0, 139, 319, 139, 5);
    draw_rect(buf, 0, 140, 319, 141, 5);

    // ── PERSPECTIVE GRID FLOOR ────────────────────────────────────────────────
    int horizon_y = 141;
    int grid_scroll = (frame * 4) % 20;

    // Floor base color
    for (int y = horizon_y; y < 240; y++) {
        int dy = y - horizon_y;
        int d = (dy * 15) / 99;
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            draw_pixel(buf, x, y, (d >= t) ? 0 : 0); // black floor
        }
    }

    // Horizontal grid lines (perspective foreshortened)
    for (int row = 0; row < 14; row++) {
        float t = (float)(row * 20 + grid_scroll) / 200.0f;
        if (t <= 0.0f) continue;
        int py = horizon_y + (int)(99 * t * t);
        if (py >= 240) break;
        draw_rect(buf, 0, py, 319, py, 6);     // cyan line
        // Glow above/below
        for (int x = 0; x < 320; x++) art_dither_pixel(buf, x, py - 1, 6, 2);
        for (int x = 0; x < 320; x++) art_dither_pixel(buf, x, py + 1, 6, 2);
    }

    // Vertical perspective lines converging to vanishing point
    for (int cx_off = -160; cx_off <= 160; cx_off += 20) {
        for (int y = horizon_y; y < 240; y++) {
            int px = 160 + (cx_off * (y - horizon_y)) / 99;
            draw_pixel(buf, px, y, 5); // magenta verticals
        }
    }


    // ── PARALLAX SCROLLING 3D PALM TREES ──────────────────────────────────────
    for (int side = 0; side < 2; side++) { // 0 = Left, 1 = Right
        for (int i = 0; i < 3; i++) {
            int t_frame = (frame * 2 + i * 80) % 240;
            float t = t_frame / 240.0f;
            int py = 141 + (int)(99 * t * t);
            int px = 160 + (side == 0 ? -1 : 1) * (int)(155 * t);
            int scale = (int)(t * 32.0f);
            if (scale > 2) {
                int trunk_h = 10 + scale * 2 / 3;
                int crown_size = 6 + scale / 3;
                draw_countach_palm(buf, px, py, trunk_h, crown_size,
                                   frame + i * 17 + side * 31, side == 1);
            }
        }
    }

    // ── REAR-VIEW LAMBORGHINI COUNTACH (Swaying/steering in center-foreground) ──
    int car_cx = 160 + (int)(22.0f * sinf(frame * 0.04f));
    int car_cy = 230;
    draw_rear_countach(buf, car_cx, car_cy, frame);

    // ── MOON ─────────────────────────────────────────────────────────────────
    draw_circle(buf, 280, 28, 10, 7);
    // Crescent cutout
    draw_circle(buf, 285, 25, 8, 4);
    // Crater details
    draw_circle(buf, 272, 24, 2, 6);
    draw_circle(buf, 278, 32, 1, 6);
}

// ============================================================================
//  STATE 2: City Skyline at Night — ENHANCED
// ============================================================================
static void draw_city_skyline(uint8_t *buf, int frame) {
    // ── SKY gradient top-dark to mid-blue ────────────────────────────────────
    for (int y = 0; y < 140; y++) {
        int d = (y * 15) / 140;
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            draw_pixel(buf, x, y, (d >= t) ? 4 : 0);
        }
    }
    draw_stars(buf, frame);

    // ── CRESCENT MOON with craters ────────────────────────────────────────────
    draw_circle(buf, 60, 35, 14, 3);
    draw_circle(buf, 66, 32, 12, 4); // bite out
    // Craters
    draw_circle(buf, 53, 28, 2, 6);
    draw_circle(buf, 58, 38, 3, 6);
    draw_circle(buf, 50, 38, 1, 6);

    // ── SKYSCRAPERS: 8 buildings with varying heights ─────────────────────────
    static const int bh[8]  = {100, 125, 80, 110, 88, 130, 95, 115};
    static const int bx[8]  = {  0,  40,  85, 125, 165, 205, 255, 295};
    static const int bw[8]  = { 35,  40,  35,  35,  35,  45,  35,  25};

    for (int i = 0; i < 8; i++) {
        int top = 140 - bh[i];
        // Building body: dither shadow on right side
        draw_rect(buf, bx[i], top, bx[i] + bw[i], 140, 0);
        // Right-side shading strip
        for (int y = top; y < 140; y++) {
            draw_pixel(buf, bx[i] + bw[i], y, 4);
            art_dither_pixel(buf, bx[i] + bw[i] - 1, y, 4, 2);
        }
        // Roof details
        if (i % 3 == 0) {
            // Water tank
            draw_rect(buf, bx[i] + bw[i]/2 - 3, top - 8, bx[i] + bw[i]/2 + 3, top, 4);
        }
        // Antenna
        draw_rect(buf, bx[i] + bw[i]/2, top - 16, bx[i] + bw[i]/2, top, 0);
        // Blinking light
        if (frame % 30 < 15) {
            draw_pixel(buf, bx[i] + bw[i]/2, top - 17, 1);
            draw_pixel(buf, bx[i] + bw[i]/2 - 1, top - 16, 1);
            draw_pixel(buf, bx[i] + bw[i]/2 + 1, top - 16, 1);
        }
    }

    // ── WINDOWS (multi-frame animated) ────────────────────────────────────────
    for (int i = 0; i < 8; i++) {
        int top = 140 - bh[i];
        for (int wx = bx[i] + 4; wx < bx[i] + bw[i] - 5; wx += 7) {
            for (int wy = top + 6; wy < 135; wy += 10) {
                int state = (wx * 11 + wy * 7 + frame / 60) % 6;
                uint8_t wc = (state < 3) ? 3 : ((state == 3) ? 6 : 0);
                if (wc != 0)
                    draw_rect(buf, wx, wy, wx + 3, wy + 5, wc);
            }
        }
    }

    // ── RIVER with multi-color reflections ────────────────────────────────────
    // Water base — dither between dark blue and black
    for (int y = 141; y < 240; y++) {
        int d = ((y - 141) * 15) / 99;
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            draw_pixel(buf, x, y, (d >= t) ? 0 : 4);
        }
    }

    // Reflections: wavy columns for each building
    for (int i = 0; i < 8; i++) {
        int rx = bx[i] + bw[i]/2;
        for (int y = 145; y < 235; y++) {
            int wave = ((frame / 3 + y * 2) % 10) - 5;
            int spread = 2 + (y - 145) / 15;
            int rrx = rx + wave;
            // Yellow-ish reflection
            draw_rect(buf, rrx - spread, y, rrx + spread, y, 3);
            // Fade edges with dither
            for (int x = rrx - spread - 3; x < rrx - spread; x++)
                art_dither_pixel(buf, x, y, 3, 2);
            for (int x = rrx + spread + 1; x < rrx + spread + 4; x++)
                art_dither_pixel(buf, x, y, 3, 2);
        }
    }

    // Ripple lines across water
    for (int y = 148; y < 235; y += 8) {
        int wave = ((frame / 5 + y) % 12) - 6;
        draw_rect(buf, wave + 40, y, wave + 60, y, 6);
        draw_rect(buf, wave + 140, y, wave + 170, y, 6);
        draw_rect(buf, wave + 240, y, wave + 260, y, 6);
    }

    // ── BRIDGE foreground ─────────────────────────────────────────────────────
    draw_rect(buf, 0, 148, 319, 152, 0);  // main span
    draw_rect(buf, 0, 152, 319, 152, 4);  // bottom edge highlight
    for (int px = 10; px < 320; px += 55) {
        // Suspension cables from tower
        draw_rect(buf, px + 10, 138, px + 14, 185, 0);  // Tower
        for (int cx = -20; cx <= 20; cx += 4) {
            int cy_off = cx * cx / 10;
            draw_pixel(buf, px + 12 + cx, 152 - cy_off, 4);  // cable
        }
    }

    // ── Flying objects (aircraft lights) ─────────────────────────────────────
    int plane_x = (frame * 2) % 380 - 30;
    int plane_y = 45;
    if (plane_x > 0 && plane_x < 320) {
        draw_rect(buf, plane_x - 5, plane_y, plane_x + 5, plane_y + 1, 0);  // body
        draw_rect(buf, plane_x - 8, plane_y + 1, plane_x + 8, plane_y + 1, 0);  // wings
        // Blinking strobes
        if (frame % 8 < 4) {
            draw_pixel(buf, plane_x - 8, plane_y + 1, 1);
            draw_pixel(buf, plane_x + 8, plane_y + 1, 6);
        }
    }
}

void init_city_skyline() {}
void play_city_skyline(uint8_t *buf, int frame) {
    draw_city_skyline(buf, frame);
}

// ============================================================================
//  STATE 3: Cozy Fireplace — ENHANCED
// ============================================================================
static void draw_cozy_fireplace(uint8_t *buf, int frame) {
    // ── ROOM background (dark) ────────────────────────────────────────────────
    // Wall with dithered warm gradient from firelight
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            int dist = (x - 160) * (x - 160) / 200 + (y - 150) * (y - 150) / 300;
            int d = 15 - dist / 20;
            if (d < 0) d = 0;
            if (d > 15) d = 15;
            int t = art_bayer4_at(x, y);
            uint8_t col = (d >= t) ? 1 : 0; // warm red glow -> black
            draw_pixel(buf, x, y, col);
        }
    }

    // ── FIREPLACE SURROUND ────────────────────────────────────────────────────
    // Ornate mantel
    draw_rect(buf, 60, 52, 260, 68, 6);   // cyan shelf top
    draw_rect(buf, 62, 68, 258, 72, 7);   // white front face
    draw_rect(buf, 60, 52, 260, 53, 7);   // top highlight
    draw_rect(buf, 60, 52, 61, 72, 7);    // left highlight
    draw_rect(buf, 258, 52, 260, 72, 4);  // right shadow

    // Mantel decorations
    draw_circle(buf, 160, 50, 8, 3);       // trophy/orb
    draw_circle(buf, 160, 50, 5, 7);       // highlight
    draw_rect(buf, 157, 50, 163, 54, 3);   // base
    draw_rect(buf, 105, 55, 118, 67, 4);   // left candle holder
    draw_rect(buf, 111, 42, 113, 56, 7);   // candle
    draw_circle(buf, 112, 40, 3, 3);        // flame
    draw_circle(buf, 112, 38, 1, 7);
    draw_rect(buf, 202, 55, 215, 67, 4);   // right candle holder
    draw_rect(buf, 208, 42, 210, 56, 7);
    draw_circle(buf, 209, 40, 3, 3);
    draw_circle(buf, 209, 38, 1, 7);

    // Stone columns (left)
    draw_rect(buf, 72, 72, 115, 215, 7);
    // Stone column texture — horizontal mortar lines
    for (int y = 80; y < 215; y += 18) {
        draw_rect(buf, 72, y, 115, y + 1, 6);
    }
    // Column right shadow
    for (int y = 72; y < 215; y++) {
        draw_pixel(buf, 114, y, 6);
        draw_pixel(buf, 113, y, 6);
        art_dither_pixel(buf, 112, y, 6, 2);
    }

    // Stone columns (right)
    draw_rect(buf, 205, 72, 248, 215, 7);
    for (int y = 80; y < 215; y += 18) {
        draw_rect(buf, 205, y, 248, y + 1, 6);
    }
    for (int y = 72; y < 215; y++) {
        draw_pixel(buf, 206, y, 6);
        draw_pixel(buf, 207, y, 6);
        art_dither_pixel(buf, 208, y, 6, 2);
    }

    // Hearth base (stone)
    draw_rect(buf, 60, 210, 260, 232, 7);
    for (int x = 80; x < 260; x += 24) {
        draw_rect(buf, x, 210, x + 1, 232, 6);
    }
    draw_rect(buf, 60, 232, 260, 232, 6);

    // ── FIREBOX background - glowing back wall ────────────────────────────────
    for (int y = 73; y < 210; y++) {
        int glow = 6 - (y - 73) / 22;
        if (glow < 0) glow = 0;
        for (int x = 116; x < 205; x++) {
            int dist = (x - 160) * (x - 160) / 40;
            int d = glow - dist / 30;
            if (d < 0) d = 0;
            int t = art_bayer4_at(x, y);
            draw_pixel(buf, x, y, (d >= t) ? 1 : 0);
        }
    }

    // ── FIREWOOD ─────────────────────────────────────────────────────────────
    // Log 1 (main log horizontal)
    draw_rect(buf, 125, 187, 195, 197, 1);
    // Log texture rings
    for (int x = 132; x < 190; x += 12) {
        draw_circle(buf, x, 192, 4, 5);
        draw_circle(buf, x, 192, 2, 3);
    }
    // Log ends (cross-section)
    draw_circle(buf, 126, 192, 5, 5);
    draw_circle(buf, 126, 192, 3, 3);
    draw_pixel(buf, 126, 192, 1);
    draw_circle(buf, 194, 192, 5, 5);
    draw_circle(buf, 194, 192, 3, 3);
    draw_pixel(buf, 194, 192, 1);
    // Log 2 (angled top)
    draw_rect(buf, 133, 182, 185, 190, 1);
    draw_pixel(buf, 133, 182, 5);
    draw_pixel(buf, 185, 182, 5);

    // ── GLOWING COALS ─────────────────────────────────────────────────────────
    for (int cx = 128; cx <= 192; cx += 5) {
        int phase = (frame + cx * 3) % 24;
        uint8_t col = (phase < 12) ? 1 : ((phase < 18) ? 3 : 5);
        int cr = 2 + (phase < 6 ? 1 : 0);
        draw_circle(buf, cx, 199, cr, col);
        // Ember glow
        art_dither_pixel(buf, cx - cr - 1, 199, col, 2);
        art_dither_pixel(buf, cx + cr + 1, 199, col, 2);
    }

    // ── FLAME — layered circles, detailed ─────────────────────────────────────
    for (int i = 0; i < 9; i++) {
        int seed = i * 31 + frame;
        int fx = 138 + i * 8 + (seed % 9) - 4;
        int fy = 183 - (seed % 38);
        int base_r = 9 - (183 - fy) / 5;
        if (base_r <= 0) continue;

        uint8_t col_outer, col_inner;
        if (fy > 173) { col_outer = 3; col_inner = 7; }
        else if (fy > 158) { col_outer = 1; col_inner = 3; }
        else { col_outer = 5; col_inner = 1; }

        draw_circle(buf, fx, fy, base_r, col_outer);
        if (base_r > 2) draw_circle(buf, fx, fy, base_r - 2, col_inner);
        if (base_r > 4) draw_circle(buf, fx, fy, base_r - 4, 7);
    }

    // ── RISING EMBERS ─────────────────────────────────────────────────────────
    for (int i = 0; i < 12; i++) {
        int seed = i * 53 + frame;
        int sx = 130 + i * 13 + (seed % 14) - 7;
        int sy = 155 - (seed % 110);
        if (sy > 78 && sy < 200) {
            uint8_t ec = ((seed / 7) % 3 == 0) ? 3 : ((seed / 7) % 3 == 1) ? 1 : 7;
            draw_pixel(buf, sx, sy, ec);
            if (i % 3 < 2) draw_pixel(buf, sx + 1, sy, ec);
        }
    }

    // ── SMOKE WISPS from top of fire ──────────────────────────────────────────
    for (int i = 0; i < 5; i++) {
        int seed = i * 41 + frame / 2;
        int wx = 148 + i * 10 + (seed % 10) - 5;
        int wy = 74 + (seed % 18);
        art_dither_pixel(buf, wx, wy, 7, 2);
        art_dither_pixel(buf, wx + 1, wy, 6, 2);
    }

    // ── RUG on floor ─────────────────────────────────────────────────────────
    draw_rect(buf, 70, 233, 250, 239, 1);
    // Rug pattern
    for (int x = 85; x < 250; x += 20) {
        draw_rect(buf, x, 233, x + 8, 239, 5);
    }
    draw_rect(buf, 70, 233, 70, 239, 5);
    draw_rect(buf, 250, 233, 250, 239, 5);

    // ── WINDOW glow on left wall ──────────────────────────────────────────────
    draw_rect(buf, 5, 80, 45, 140, 3);
    // Window pane divisions
    draw_rect(buf, 25, 80, 26, 140, 0);
    draw_rect(buf, 5, 110, 45, 111, 0);
    // Star outside window
    draw_pixel(buf, 15, 95, 7);
    draw_pixel(buf, 35, 90, 7);
}

void init_fireplace() {}
void play_fireplace(uint8_t *buf, int frame) {
    draw_cozy_fireplace(buf, frame);
}

// ============================================================================
//  STATE 4: Cherry Blossom Pagoda — ENHANCED
// ============================================================================
#define MAX_PETALS 90
struct Petal { float x, y; float speed_y, speed_x; int phase; };
static struct Petal petals[MAX_PETALS];

void init_cherry_blossom() {
    for (int i = 0; i < MAX_PETALS; i++) {
        petals[i].x = (float)(get_rand() % 320);
        petals[i].y = (float)(get_rand() % 240);
        petals[i].speed_y = 0.8f + (get_rand() % 100) / 100.0f;
        petals[i].speed_x = -1.2f - (get_rand() % 100) / 100.0f;
        petals[i].phase = get_rand() % 64;
    }
}

static void draw_blossom_cluster(uint8_t *buf, int cx, int cy, int r) {
    draw_circle(buf, cx, cy, r, 5); // magenta
    draw_circle(buf, cx - r/3, cy - r/3, r/2, 7); // white highlights
    draw_circle(buf, cx + r/2, cy + r/3, r/3, 5); // magenta overlap
    draw_circle(buf, cx, cy, r/4, 1); // dark center

    for (int i = 0; i < r * 2; i++) {
        int angle = get_rand() % 360;
        float rad = (float)angle * 3.14159f / 180.0f;
        int dist = r + (get_rand() % 4);
        int px = cx + (int)(cosf(rad) * dist);
        int py = cy + (int)(sinf(rad) * dist);
        if (px >= 0 && px < 320 && py >= 0 && py < 240) {
            draw_pixel(buf, px, py, (get_rand() % 2 == 0) ? 5 : 7);
        }
    }
}

void play_cherry_blossom(uint8_t *buf, int frame) {
    // ── SKY GRADIENT (Indigo -> Crimson -> Gold) ──────────────────────────────
    for (int y = 0; y < 200; y++) {
        int d = (y * 15) / 200;
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            draw_pixel(buf, x, y, (d >= t) ? 1 : 4);
        }
    }

    // ── LARGE SUN ────────────────────────────────────────────────────────────
    int sun_cx = 105, sun_cy = 130;
    draw_circle(buf, sun_cx, sun_cy, 40, 1); // red
    draw_circle(buf, sun_cx, sun_cy, 30, 3); // yellow
    draw_circle(buf, sun_cx, sun_cy, 16, 7); // white hot core

    // ── DETAILED 5-STORY PAGODA (Right side) ──────────────────────────────────
    draw_rect(buf, 205, 185, 295, 200, 0);
    draw_rect(buf, 205, 185, 295, 186, 5); // roof outline bottom

    int tier_widths[5] = {65, 55, 47, 40, 32};
    int tier_heights[5] = {20, 18, 16, 14, 12};
    int current_y = 185;
    for (int t = 0; t < 5; t++) {
        int w = tier_widths[t];
        int h = tier_heights[t];
        int next_y = current_y - h;
        // Pagoda body
        draw_rect(buf, 250 - w/3, next_y, 250 + w/3, current_y, 0);
        // Roof eave
        draw_rect(buf, 250 - w/2, next_y, 250 + w/2, next_y + 3, 0);
        draw_pixel(buf, 250 - w/2 - 1, next_y, 5);
        draw_pixel(buf, 250 + w/2 + 1, next_y, 5);
        // Glowing windows
        if (t < 4) {
            uint8_t wc = (frame % 40 < 30) ? 3 : 7;
            draw_rect(buf, 248, next_y + h/2 - 2, 252, next_y + h/2 + 2, wc);
        }
        current_y = next_y;
    }
    // Spire
    draw_rect(buf, 249, current_y - 25, 251, current_y, 0);
    draw_circle(buf, 250, current_y - 25, 2, 3);

    // ── GNARLED CHERRY BLOSSOM BRANCHES (Left & Right) ────────────────────────
    // Left trunk & branch
    for (int y = 200; y > 80; y--) {
        int tx = 30 + isqrt(200 - y) * 3;
        draw_rect(buf, tx - 4, y, tx + 4, y, 0);
        if (y % 6 == 0) draw_pixel(buf, tx - 2, y, 5);
    }
    for (int x = 40; x < 120; x++) {
        int by = 130 + (x - 40) * (x - 40) / 45;
        draw_rect(buf, x, by - 2, x, by + 2, 0);
    }
    for (int x = 30; x > 5; x--) {
        int by = 150 + (30 - x) * (30 - x) / 20;
        draw_rect(buf, x, by - 1, x, by + 1, 0);
    }

    // Right branch framing top-right corner
    for (int y = 0; y < 90; y++) {
        int tx = 310 - isqrt(y) * 4;
        draw_rect(buf, tx - 3, y, tx + 3, y, 0);
    }
    for (int x = 280; x > 180; x--) {
        int by = 60 - (280 - x) * (280 - x) / 80;
        draw_rect(buf, x, by - 2, x, by + 2, 0);
    }

    // ── MASSIVE BLOSSOM CLUSTERS ──────────────────────────────────────────────
    // Left side clusters
    draw_blossom_cluster(buf, 20, 140, 14);
    draw_blossom_cluster(buf, 35, 130, 16);
    draw_blossom_cluster(buf, 55, 120, 18);
    draw_blossom_cluster(buf, 75, 125, 15);
    draw_blossom_cluster(buf, 95, 135, 12);
    // Right side clusters
    draw_blossom_cluster(buf, 300, 30, 16);
    draw_blossom_cluster(buf, 275, 45, 18);
    draw_blossom_cluster(buf, 250, 40, 15);
    draw_blossom_cluster(buf, 215, 30, 13);
    draw_blossom_cluster(buf, 190, 25, 10);

    // ── PETAL COVERED GROUND (Replaces Pond) ──────────────────────────────────
    draw_rect(buf, 0, 200, 319, 239, 0);
    for (int y = 200; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            int noise = (x * 17 + y * 23 + frame) % 19;
            if (noise < 5) {
                draw_pixel(buf, x, y, 5); // fallen petal
            } else if (noise < 7) {
                draw_pixel(buf, x, y, 1); // shadow
            } else if (noise < 8) {
                draw_pixel(buf, x, y, 7); // highlight
            }
        }
    }

    // ── HEAVY DYNAMIC FALLING PETALS ──────────────────────────────────────────
    for (int i = 0; i < MAX_PETALS; i++) {
        uint8_t col = (i % 3 == 0) ? 5 : ((i % 3 == 1) ? 7 : 1);
        int wave = (int)(8.0f * sinf(frame * 0.05f + petals[i].phase));
        int px = (int)petals[i].x + wave;
        int py = (int)petals[i].y;

        if (px >= 0 && px < 320 && py >= 0 && py < 240) {
            draw_pixel(buf, px, py, col);
            draw_pixel(buf, px + 1, py, col);
            draw_pixel(buf, px, py + 1, col);
        }

        petals[i].y += petals[i].speed_y;
        petals[i].x += petals[i].speed_x;

        // Reset if offscreen or ground contact
        if (petals[i].y >= 200 + (get_rand() % 35)) {
            petals[i].y = 0;
            petals[i].x = (float)(get_rand() % 320);
        }
        if (petals[i].x < 0) {
            petals[i].x = 319.0f;
            petals[i].y = (float)(get_rand() % 160);
        }
    }
}

// ============================================================================
//  STATE 5: Cyberpunk Street — ENHANCED
// ============================================================================
#define MAX_RAIN 55
struct Raindrop { int x, y, speed; };
static struct Raindrop raindrops[MAX_RAIN];

void init_cyberpunk() {
    for (int i = 0; i < MAX_RAIN; i++) {
        raindrops[i].x = get_rand() % 320;
        raindrops[i].y = get_rand() % 240;
        raindrops[i].speed = 5 + (get_rand() % 5);
    }
}

void play_cyberpunk(uint8_t *buf, int frame) {
    // ── SKY gradient (dark purple -> black) ───────────────────────────────────
    for (int y = 0; y < 180; y++) {
        int d = ((180 - y) * 15) / 180;
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            draw_pixel(buf, x, y, (d >= t) ? 5 : 0);
        }
    }

    // ── BACKGROUND SKYLINE (far buildings, blue) ──────────────────────────────
    static const int fbh[7] = {60, 45, 70, 55, 65, 50, 40};
    static const int fbx[7] = {-5, 30, 70, 120, 175, 230, 275};
    static const int fbw[7] = {35, 40, 48, 50, 52, 45, 50};
    for (int i = 0; i < 7; i++) {
        // Far building silhouette — dithered blue
        for (int y = 180 - fbh[i]; y < 180; y++) {
            for (int x = fbx[i]; x < fbx[i] + fbw[i]; x++) {
                art_dither_pixel(buf, x, y, 4, 2);
            }
        }
    }

    // ── FOREGROUND SKYSCRAPERS ────────────────────────────────────────────────
    // Building 1 (left)
    draw_rect(buf, 5, 25, 68, 179, 0);
    // Right face lit by neon
    for (int y = 25; y < 179; y++) {
        art_dither_pixel(buf, 67, y, 6, 3);
        art_dither_pixel(buf, 66, y, 6, 1);
    }
    // Windows building 1
    for (int wy = 35; wy < 170; wy += 14) {
        for (int wx = 14; wx < 58; wx += 10) {
            int s = (wx * 7 + wy * 11 + frame / 45) % 7;
            uint8_t wc = (s < 3) ? 3 : ((s < 5) ? 6 : 0);
            if (wc != 0) draw_rect(buf, wx, wy, wx + 5, wy + 7, wc);
        }
    }

    // Building 2 (left-center)
    draw_rect(buf, 78, 42, 140, 179, 0);
    for (int y = 42; y < 179; y++) {
        art_dither_pixel(buf, 139, y, 5, 2);
        art_dither_pixel(buf, 138, y, 5, 1);
    }
    for (int wy = 52; wy < 170; wy += 14) {
        for (int wx = 88; wx < 133; wx += 10) {
            int s = (wx * 13 + wy * 7 + frame / 50) % 7;
            uint8_t wc = (s < 2) ? 6 : ((s < 4) ? 3 : 0);
            if (wc != 0) draw_rect(buf, wx, wy, wx + 5, wy + 7, wc);
        }
    }

    // Building 3 (center — tallest)
    draw_rect(buf, 150, 12, 222, 179, 0);
    for (int y = 12; y < 179; y++) {
        art_dither_pixel(buf, 221, y, 6, 3);
        art_dither_pixel(buf, 220, y, 6, 2);
    }
    for (int wy = 22; wy < 170; wy += 14) {
        for (int wx = 160; wx < 216; wx += 10) {
            int s = (wx * 11 + wy * 13 + frame / 40) % 8;
            uint8_t wc = (s < 3) ? 6 : ((s < 5) ? 3 : ((s < 6) ? 5 : 0));
            if (wc != 0) draw_rect(buf, wx, wy, wx + 5, wy + 7, wc);
        }
    }

    // Building 4 (right)
    draw_rect(buf, 240, 52, 308, 179, 0);
    for (int y = 52; y < 179; y++) {
        art_dither_pixel(buf, 307, y, 5, 2);
    }
    for (int wy = 62; wy < 170; wy += 14) {
        for (int wx = 252; wx < 302; wx += 10) {
            int s = (wx * 7 + wy * 17 + frame / 55) % 7;
            uint8_t wc = (s < 3) ? 3 : ((s < 5) ? 5 : 0);
            if (wc != 0) draw_rect(buf, wx, wy, wx + 5, wy + 7, wc);
        }
    }

    // ── NEON SIGNS ────────────────────────────────────────────────────────────
    uint8_t blink1 = (frame % 24 < 12) ? 6 : 5;
    // PICO billboard (building 2)
    draw_rect(buf, 84, 58, 132, 92, 0);
    // Neon border frame, flickering
    draw_rect(buf, 84, 58, 132, 59, blink1);
    draw_rect(buf, 84, 91, 132, 92, blink1);
    draw_rect(buf, 84, 58, 85, 92, blink1);
    draw_rect(buf, 131, 58, 132, 92, blink1);
    // Glow inside
    for (int y2 = 60; y2 < 91; y2++)
        for (int x2 = 86; x2 < 131; x2++)
            art_dither_pixel(buf, x2, y2, blink1, 1);
    draw_string(buf, "PICO", 92, 70, 2, blink1);

    // VGA vertical sign (building 3)
    uint8_t blink2 = (frame % 36 < 18) ? 5 : 6;
    draw_rect(buf, 178, 35, 192, 115, blink2);
    draw_rect(buf, 180, 37, 190, 113, 0);
    for (int y2 = 38; y2 < 113; y2 += 2) art_dither_pixel(buf, 185, y2, blink2, 2);
    draw_char(buf, 'V', 183, 45, 2, blink2);
    draw_char(buf, 'G', 183, 67, 2, blink2);
    draw_char(buf, 'A', 183, 89, 2, blink2);

    // Holographic logo over center
    uint8_t blink3 = (frame % 16 < 8) ? 6 : 7;
    draw_string(buf, "2077", 155, 13, 1, blink3);

    // ── ROAD ─────────────────────────────────────────────────────────────────
    // Road surface with wet reflection
    for (int y = 180; y < 240; y++) {
        int d = ((y - 180) * 15) / 60;
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            draw_pixel(buf, x, y, (d >= t) ? 0 : 0);
        }
    }

    // Center divider lines (moving)
    int lane_scroll = (frame * 4) % 40;
    for (int x = 0; x < 320; x += 40) {
        int lx = (x + lane_scroll) % 320;
        draw_rect(buf, lx, 208, lx + 20, 210, 3);
    }

    // ── ROAD REFLECTIONS ─────────────────────────────────────────────────────
    // Neon reflection ripples
    for (int y = 180; y < 240; y++) {
        int wave = ((frame / 2 + y * 3) % 8) - 4;
        if ((y - 180) % 2 == 0) {
            // Building 1 cyan reflection
            int rx = 36 + wave;
            draw_rect(buf, rx - 8, y, rx + 8, y, 6);
            art_dither_pixel(buf, rx - 10, y, 6, 2);
            art_dither_pixel(buf, rx + 10, y, 6, 2);
            // Building 2 magenta
            int rx2 = 109 + wave;
            draw_rect(buf, rx2 - 6, y, rx2 + 6, y, 5);
            // Center magenta/cyan
            int rx3 = 186 + wave;
            draw_rect(buf, rx3 - 10, y, rx3 + 10, y, blink2);
            // Building 4
            int rx4 = 274 + wave;
            draw_rect(buf, rx4 - 7, y, rx4 + 7, y, 5);
        }
    }

    // ── FLYING SPINNER CAR ────────────────────────────────────────────────────
    int spinner_x = (frame * 4) % 420 - 60;
    int spinner_y = 22 + ((frame / 8) % 6);
    if (spinner_x >= -20 && spinner_x < 320) {
        // Body
        draw_rect(buf, spinner_x, spinner_y, spinner_x + 14, spinner_y + 3, 0);
        draw_rect(buf, spinner_x + 2, spinner_y - 1, spinner_x + 10, spinner_y, 0);
        draw_rect(buf, spinner_x - 2, spinner_y + 2, spinner_x + 16, spinner_y + 3, 0); // wings
        // Lights
        uint8_t sblink = (frame % 6 < 3) ? 6 : 0;
        draw_pixel(buf, spinner_x + 14, spinner_y + 1, sblink);
        draw_pixel(buf, spinner_x - 1, spinner_y + 1, 1);
        // Thruster glow
        for (int g = 0; g < 8; g++) art_dither_pixel(buf, spinner_x - 1 - g, spinner_y + 2, 1, 3 - g/3);
    }

    // ── DIAGONAL RAIN ─────────────────────────────────────────────────────────
    for (int i = 0; i < MAX_RAIN; i++) {
        int rx = raindrops[i].x, ry = raindrops[i].y;
        // Rain streak (2px length)
        for (int l = 0; l < 4; l++) {
            int rx2 = rx + l, ry2 = ry - l * 2;
            draw_pixel(buf, rx2, ry2, 6);
            art_dither_pixel(buf, rx2 - 1, ry2, 4, 2);
        }
        // Splash on road
        if (ry >= 179 && ry < 184) {
            draw_pixel(buf, rx - 3, 180, 6);
            draw_pixel(buf, rx + 3, 180, 6);
            art_dither_pixel(buf, rx - 5, 180, 6, 2);
            art_dither_pixel(buf, rx + 5, 180, 6, 2);
        }
        raindrops[i].y += raindrops[i].speed;
        raindrops[i].x -= 3;
        if (raindrops[i].y >= 240) { raindrops[i].y = get_rand() % 40; raindrops[i].x = get_rand() % 320; }
        if (raindrops[i].x < 0) { raindrops[i].x = 319; raindrops[i].y = get_rand() % 240; }
    }
}

// ============================================================================
//  STATE 6: Space Nebula — ENHANCED
// ============================================================================
void init_nebula() {}

void play_nebula(uint8_t *buf, int frame) {
    // ── BLACK DEEP SPACE ──────────────────────────────────────────────────────
    clear_screen(buf);

    // ── MILKY WAY band — dithered diagonal stripe ─────────────────────────────
    for (int y = 10; y < 230; y++) {
        for (int x = 0; x < 320; x++) {
            int band = (x + y) - 160; // diagonal offset
            if (band > -80 && band < 80) {
                int density = 15 - (band * band) / 430;
                if (density > 0) art_dither_pixel(buf, x, y, 4, density/4);
            }
        }
    }

    // ── TWINKLING STARS (extended set) ────────────────────────────────────────
    draw_stars(buf, frame);
    // Extra star field
    for (int i = 0; i < 60; i++) {
        int sx2 = (i * 97 + 23) % 320;
        int sy2 = (i * 53 + 41) % 240;
        int phase = (frame + i * 17) % 48;
        if (phase < 40) {
            draw_pixel(buf, sx2, sy2, 7);
            if (i % 5 == 0) {
                draw_pixel(buf, sx2 - 1, sy2, 6);
                draw_pixel(buf, sx2 + 1, sy2, 6);
                draw_pixel(buf, sx2, sy2 - 1, 6);
                draw_pixel(buf, sx2, sy2 + 1, 6);
            }
        }
    }

    // ── NEBULA CLOUDS (multi-color, layered) ──────────────────────────────────
    // Cloud 1: large magenta-blue cloud (center)
    for (int y = 25; y < 215; y += 2) {
        for (int x = 55; x < 285; x += 2) {
            int dx2 = x - 175;
            int dy2 = y - 105;
            int dist2 = dx2*dx2 + dy2*dy2;
            int noise = (isqrt(dist2) * 13) + (x * 7) + (y * 11) + frame/20;
            if (dist2 < 9500 + (noise % 1200)) {
                int pattern = (x * 11 + y * 7 + frame/18) % 14;
                uint8_t col;
                if (pattern < 3) col = 4;
                else if (pattern < 6) col = 5;
                else if (pattern < 8) col = 6;
                else if (pattern < 9) col = 7;
                else continue;
                art_dither_pixel(buf, x, y, col, 3);
                art_dither_pixel(buf, x + 1, y, col, 1);
            }
        }
    }
    // Cloud 2: smaller red-yellow cloud (upper left)
    for (int y = 15; y < 100; y += 2) {
        for (int x = 15; x < 130; x += 2) {
            int dx2 = x - 65;
            int dy2 = y - 55;
            int dist2 = dx2*dx2 + dy2*dy2;
            if (dist2 < 2500) {
                int pattern = (x * 7 + y * 13 + frame/25) % 8;
                uint8_t col = (pattern < 3) ? 1 : ((pattern < 6) ? 3 : 7);
                art_dither_pixel(buf, x, y, col, 2);
            }
        }
    }

    // ── SATURN-LIKE PLANET ────────────────────────────────────────────────────
    int pcx = 248, pcy = 62;
    // Ring shadow behind planet
    for (int rx = pcx - 40; rx < pcx; rx++) {
        int ry_off = (rx - pcx) / 2;
        for (int r2 = -2; r2 <= 2; r2++) {
            int ry = pcy + ry_off + r2;
            draw_pixel(buf, rx, ry, 6);
            if (r2 == -2 || r2 == 2) draw_pixel(buf, rx, ry, 3);
        }
    }
    // Planet body — shaded sphere
    art_circle_shaded(buf, pcx, pcy, 18, 4, 6);
    // Surface bands
    draw_rect(buf, pcx - 17, pcy - 6, pcx + 17, pcy - 4, 5);
    draw_rect(buf, pcx - 17, pcy + 4, pcx + 17, pcy + 6, 5);
    draw_rect(buf, pcx - 10, pcy - 2, pcx + 10, pcy + 2, 4);
    // Specular highlight
    draw_circle(buf, pcx - 7, pcy - 7, 4, 7);
    draw_circle(buf, pcx - 7, pcy - 7, 2, 7);
    // Ring front
    for (int rx = pcx; rx <= pcx + 40; rx++) {
        int ry_off = (rx - pcx) / 2;
        for (int r2 = -2; r2 <= 2; r2++) {
            int ry = pcy + ry_off + r2;
            draw_pixel(buf, rx, ry, 6);
            if (r2 == -2 || r2 == 2) draw_pixel(buf, rx, ry, 3);
        }
    }
    // Ring glow
    for (int rx = pcx - 40; rx <= pcx + 40; rx++) {
        int ry = pcy + (rx - pcx) / 2;
        art_dither_pixel(buf, rx, ry - 3, 6, 2);
        art_dither_pixel(buf, rx, ry + 3, 6, 2);
    }

    // Small moon
    draw_circle(buf, pcx + 55, pcy - 20, 5, 7);
    draw_circle(buf, pcx + 55, pcy - 20, 3, 6);
    draw_pixel(buf, pcx + 53, pcy - 22, 7);

    // ── FLOATING ASTRONAUT ────────────────────────────────────────────────────
    int t = frame % 360;
    // Smooth elliptical orbit using integer trig approximation
    int ax, ay;
    if (t < 90)  { ax = 100 + t/2;          ay = 110 - t/3; }
    else if (t < 180) { ax = 145 + (t-90)/2;   ay = 80  + (t-90)/3; }
    else if (t < 270) { ax = 190 - (t-180)/2;  ay = 110 + (t-180)/3; }
    else              { ax = 145 - (t-270)/2;  ay = 140 - (t-270)/3; }

    // Detailed spacesuit
    // Backpack (PLSS)
    draw_rect(buf, ax - 6, ay - 6, ax - 3, ay + 5, 6);
    draw_rect(buf, ax - 6, ay - 4, ax - 5, ay + 3, 7);  // pack highlight
    // Torso
    draw_rect(buf, ax - 2, ay - 7, ax + 5, ay + 6, 7);
    // Chest details
    draw_rect(buf, ax + 1, ay - 4, ax + 4, ay - 2, 6);  // blue PLSS panel
    draw_pixel(buf, ax + 2, ay - 3, 3);                   // status light
    // Helmet
    draw_circle(buf, ax + 1, ay - 9, 5, 7);
    draw_circle(buf, ax + 2, ay - 10, 3, 6);  // visor
    draw_pixel(buf, ax + 1, ay - 11, 7);       // helmet top
    // Visor reflection
    draw_pixel(buf, ax + 1, ay - 10, 7);
    draw_pixel(buf, ax + 3, ay - 9, 7);
    // Arms
    draw_rect(buf, ax - 2, ay - 5, ax - 1, ay + 1, 7);  // left arm
    draw_rect(buf, ax + 6, ay - 5, ax + 7, ay + 1, 7);  // right arm
    // Gloves
    draw_rect(buf, ax - 3, ay + 1, ax - 1, ay + 2, 3);
    draw_rect(buf, ax + 6, ay + 1, ax + 8, ay + 2, 3);
    // Legs
    draw_rect(buf, ax - 1, ay + 6, ax + 1, ay + 10, 7);
    draw_rect(buf, ax + 3, ay + 6, ax + 5, ay + 10, 7);
    // Boots
    draw_rect(buf, ax - 2, ay + 10, ax + 2, ay + 11, 6);
    draw_rect(buf, ax + 3, ay + 10, ax + 7, ay + 11, 6);
    // Waving hand
    int wave = (frame / 12) % 2;
    if (wave == 0) { draw_pixel(buf, ax + 8, ay - 3, 7); draw_pixel(buf, ax + 9, ay - 4, 7); }
    else           { draw_pixel(buf, ax + 8, ay - 1, 7); draw_pixel(buf, ax + 9, ay - 2, 7); }
    // Tether
    for (int tl = 0; tl < 18; tl++) {
        draw_pixel(buf, ax - 4 - tl + (tl % 3 - 1), ay, 6);
    }
}

// ============================================================================
//  STATE 7: NASA Space Shuttle Launch — ENHANCED
// ============================================================================
void init_nasa() {}

void play_nasa(uint8_t *buf, int frame) {
    // ── SPACE / ATMOSPHERE background ─────────────────────────────────────────
    // Top = black, bottom = dark blue atmosphere
    for (int y = 0; y < 240; y++) {
        int d = (y * 15) / 239;
        for (int x = 0; x < 320; x++) {
            int t = art_bayer4_at(x, y);
            draw_pixel(buf, x, y, (d >= t) ? 4 : 0);
        }
    }

    // ── STARS ─────────────────────────────────────────────────────────────────
    draw_stars(buf, frame);
    for (int i = 0; i < 20; i++) {
        int sx2 = (STAR_X[i] * 13 + 37) % 320;
        int sy2 = 80 + (STAR_Y[i] * 7 + 59) % 140;
        int phase = (frame + i * 11) % 32;
        if (phase < 24) draw_pixel(buf, sx2, sy2, 7);
        else draw_pixel(buf, sx2, sy2, 4);
    }

    // ── NEBULA WISP ───────────────────────────────────────────────────────────
    for (int y = 40; y < 200; y += 3) {
        for (int x = 30; x < 290; x += 3) {
            int dx2 = x - 160, dy2 = y - 115;
            int dist2 = dx2*dx2 + dy2*dy2;
            int noise = (isqrt(dist2) * 5) + x*3 + y*7;
            if (dist2 < 7000 + (noise % 1000)) {
                int pattern = (x*7 + y*13 + frame/20) % 10;
                if (pattern < 3) art_dither_pixel(buf, x, y, 5, 2);
                else if (pattern < 5) art_dither_pixel(buf, x, y, 4, 2);
            }
        }
    }

    // ── NASA WORM LOGO (top, large) ───────────────────────────────────────────
    // N — smooth curved worm strokes
    draw_rect(buf, 90, 25, 93, 48, 1);
    for (int i = 0; i <= 18; i++) draw_rect(buf, 92 + i, 22 + i, 95 + i, 25 + i, 1);
    draw_rect(buf, 108, 22, 111, 45, 1);

    // A — Rounded inverted U arch (No crossbar, authentic NASA worm)
    draw_rect(buf, 118, 30, 121, 48, 1);
    draw_rect(buf, 130, 30, 133, 48, 1);
    draw_rect(buf, 122, 22, 129, 25, 1);
    draw_rect(buf, 120, 24, 123, 29, 1);
    draw_rect(buf, 128, 24, 131, 29, 1);

    // S — smooth curves using flats and segments
    draw_rect(buf, 148, 22, 161, 25, 1); // top flat
    draw_rect(buf, 144, 24, 147, 34, 1); // top-left stem
    draw_rect(buf, 148, 33, 161, 36, 1); // middle flat
    draw_rect(buf, 161, 35, 164, 46, 1); // bottom-right stem
    draw_rect(buf, 147, 45, 160, 48, 1); // bottom flat
    draw_rect(buf, 160, 23, 162, 27, 1); // top-right bend
    draw_rect(buf, 145, 43, 148, 47, 1); // bottom-left bend

    // A — Second A (Rounded arch, no crossbar)
    draw_rect(buf, 170, 30, 173, 48, 1);
    draw_rect(buf, 182, 30, 185, 48, 1);
    draw_rect(buf, 174, 22, 181, 25, 1);
    draw_rect(buf, 172, 24, 175, 29, 1);
    draw_rect(buf, 180, 24, 183, 29, 1);

    // ── LAUNCH PAD / GROUND (only at bottom if shuttle is near) ───────────────
    int cycle = frame % 240;
    int sy = 215 - (cycle * 180) / 240;
    int sx = 160;

    // Detachment physics for boosters (SRBs)
    int srb_left_x = sx - 12;
    int srb_right_x = sx + 12;
    int srb_y_offset = 0;
    bool detached = (cycle >= 130);
    int t_detach = cycle - 130;
    if (detached) {
        srb_left_x = sx - 12 - t_detach * 2;
        srb_right_x = sx + 12 + t_detach * 2;
        srb_y_offset = t_detach * 2;
    }

    // Launchpad base (only visible early in cycle)
    if (cycle < 60) {
        draw_rect(buf, sx - 30, 230, sx + 30, 239, 7);  // pad concrete
        // Pad detail
        for (int lp = sx - 25; lp < sx + 25; lp += 8) {
            draw_rect(buf, lp, 230, lp + 1, 235, 6);
        }
        // Flame trench
        draw_rect(buf, sx - 15, 224, sx + 15, 230, 0);
    }

    // ── EXHAUST PLUMES / SMOKE ────────────────────────────────────────────────
    if (sy < 235) {
        // Smoke billowing from pad
        for (int bx2 = sx - 50; bx2 <= sx + 50; bx2 += 18) {
            int smoke_off = (frame + bx2) % 16;
            int smoke_y2 = 235 - smoke_off/3;
            int smoke_r2 = 14 + (bx2 % 9) + cycle / 12;
            if (smoke_r2 > 35) smoke_r2 = 35;
            draw_circle(buf, bx2, smoke_y2, smoke_r2, 7);
            draw_circle(buf, bx2 + 5, smoke_y2 + 3, smoke_r2 - 5, 6);
            // Shadow
            art_dither_pixel(buf, bx2 + smoke_r2, smoke_y2, 4, 2);
        }

        // Rising contrail/exhaust column
        for (int ty = sy + 50; ty < 240; ty += 2) {
            int spread = (ty - (sy + 50)) / 4 + 3;
            uint8_t pc = ((ty - sy) % 12 < 6) ? 7 : 6;
            draw_rect(buf, sx - spread, ty, sx + spread, ty + 1, pc);
            // Contrail edge fade
            art_dither_pixel(buf, sx - spread - 2, ty, 6, 2);
            art_dither_pixel(buf, sx + spread + 2, ty, 6, 2);
        }

        // Engine flames — large, detailed
        int base_y = sy + 52;
        // Left SRB
        int lf = 4 + (cycle % 4);
        int lsrb_flame_x = srb_left_x;
        int lsrb_flame_y = base_y + srb_y_offset;
        if (!detached) {
            draw_circle(buf, lsrb_flame_x, lsrb_flame_y, lf, 3);
            draw_circle(buf, lsrb_flame_x, lsrb_flame_y + 4, lf - 1, 1);
            draw_circle(buf, lsrb_flame_x, lsrb_flame_y + 7, lf - 2, 5);
        } else if (t_detach < 15) {
            draw_circle(buf, lsrb_flame_x, lsrb_flame_y, lf / 2, 3);
            draw_circle(buf, lsrb_flame_x, lsrb_flame_y + 2, lf / 2, 1);
        }
        // Right SRB
        int rf = 4 + ((cycle + 1) % 4);
        int rsrb_flame_x = srb_right_x;
        int rsrb_flame_y = base_y + srb_y_offset;
        if (!detached) {
            draw_circle(buf, rsrb_flame_x, rsrb_flame_y, rf, 3);
            draw_circle(buf, rsrb_flame_x, rsrb_flame_y + 4, rf - 1, 1);
            draw_circle(buf, rsrb_flame_x, rsrb_flame_y + 7, rf - 2, 5);
        } else if (t_detach < 15) {
            draw_circle(buf, rsrb_flame_x, rsrb_flame_y, rf / 2, 3);
            draw_circle(buf, rsrb_flame_x, rsrb_flame_y + 2, rf / 2, 1);
        }
        // SSME (main engine)
        int mf = 5 + ((cycle + 2) % 4);
        draw_circle(buf, sx, base_y, mf, 7);
        draw_circle(buf, sx, base_y + 3, mf, 3);
        draw_circle(buf, sx, base_y + 7, mf - 1, 1);
        draw_circle(buf, sx, base_y + 10, mf - 2, 5);
        // Mach diamonds
        for (int md = 1; md <= 3; md++) {
            int mdy = base_y + 14 + md * 8;
            int mdw = mf - md * 2;
            if (mdw > 0) draw_circle(buf, sx, mdy, mdw, 3);
        }
    }

    // ── SPACE SHUTTLE STACK — detailed ────────────────────────────────────────
    // External Tank (ET) — orange/yellow
    draw_rect(buf, sx - 7, sy - 22, sx + 7, sy + 28, 3);
    // ET shading
    for (int ty = sy - 22; ty < sy + 28; ty++) {
        draw_pixel(buf, sx - 7, ty, 1);      // left shadow
        art_dither_pixel(buf, sx - 6, ty, 1, 2); // left shadow blend
        draw_pixel(buf, sx + 6, ty, 1);      // right shadow
    }
    // ET nose cone
    draw_rect(buf, sx - 5, sy - 27, sx + 5, sy - 23, 3);
    draw_rect(buf, sx - 3, sy - 31, sx + 3, sy - 28, 3);
    draw_rect(buf, sx - 1, sy - 34, sx + 1, sy - 32, 1);  // red tip

    // Left SRB (staged / detachable)
    int ls_x = srb_left_x;
    int ls_y = sy + srb_y_offset;
    draw_rect(buf, ls_x - 3, ls_y - 18, ls_x + 2, ls_y + 28, 7);
    draw_rect(buf, ls_x - 2, ls_y - 22, ls_x + 1, ls_y - 18, 7);  // nose
    draw_rect(buf, ls_x - 3, ls_y - 22, ls_x - 2, ls_y - 18, 6);
    draw_rect(buf, ls_x - 3, ls_y - 18, ls_x - 3, ls_y + 28, 5);  // shadow
    draw_pixel(buf, ls_x - 1, ls_y - 23, 7);  // tip
    if (!detached) {
        draw_rect(buf, ls_x - 3, ls_y, ls_x + 2, ls_y + 2, 6); // ET attach ring
    }

    // Right SRB (staged / detachable)
    int rs_x = srb_right_x;
    int rs_y = sy + srb_y_offset;
    draw_rect(buf, rs_x - 2, rs_y - 18, rs_x + 3, rs_y + 28, 7);
    draw_rect(buf, rs_x - 1, rs_y - 22, rs_x + 2, rs_y - 18, 7);
    draw_rect(buf, rs_x + 2, rs_y - 22, rs_x + 3, rs_y - 18, 6);
    draw_rect(buf, rs_x + 3, rs_y - 18, rs_x + 3, rs_y + 28, 5);
    draw_pixel(buf, rs_x + 1, rs_y - 23, 7);
    if (!detached) {
        draw_rect(buf, rs_x - 2, rs_y, rs_x + 3, rs_y + 2, 6);
    }

    // Shuttle Orbiter (mounted on ET)
    // Fuselage
    draw_rect(buf, sx - 4, sy - 10, sx + 4, sy + 8, 7);
    // Cockpit windows
    draw_rect(buf, sx - 3, sy - 8, sx + 3, sy - 6, 0);
    draw_pixel(buf, sx - 2, sy - 7, 6);
    draw_pixel(buf, sx + 1, sy - 7, 6);
    // Orbiter nose
    draw_rect(buf, sx - 2, sy - 13, sx + 2, sy - 11, 7);
    draw_pixel(buf, sx, sy - 14, 0);  // nose cap
    // Left delta wing
    draw_rect(buf, sx - 8, sy + 5, sx - 4, sy + 18, 7);
    draw_rect(buf, sx - 12, sy + 12, sx - 8, sy + 18, 7);
    draw_rect(buf, sx - 14, sy + 15, sx - 12, sy + 18, 7);
    // Wing leading edge shadow
    for (int wy2 = sy + 5; wy2 <= sy + 18; wy2++) draw_pixel(buf, sx - 14, wy2, 5);
    // Right delta wing
    draw_rect(buf, sx + 4, sy + 5, sx + 8, sy + 18, 7);
    draw_rect(buf, sx + 8, sy + 12, sx + 12, sy + 18, 7);
    draw_rect(buf, sx + 12, sy + 15, sx + 14, sy + 18, 7);
    for (int wy2 = sy + 5; wy2 <= sy + 18; wy2++) draw_pixel(buf, sx + 14, wy2, 5);
    // Vertical stabilizer / tail fin
    draw_rect(buf, sx, sy + 8, sx + 2, sy + 24, 7);
    draw_rect(buf, sx + 1, sy + 6, sx + 4, sy + 22, 7);
    // RCS thrusters on nose
    draw_pixel(buf, sx - 3, sy - 10, 6);
    draw_pixel(buf, sx + 3, sy - 10, 6);
    // Payload bay door seam
    draw_rect(buf, sx - 1, sy - 5, sx + 1, sy + 4, 6);
}

// ============================================================================
//  STATE: Space Shuttle Challenger — ENHANCED
// ============================================================================
static void draw_challenger_stack(uint8_t *buf, int sx, int sy) {
    // ET
    draw_rect(buf, sx - 7, sy, sx + 7, sy + 48, 3);
    for (int ty = sy; ty < sy + 48; ty++) {
        draw_pixel(buf, sx - 7, ty, 1);
        draw_pixel(buf, sx + 6, ty, 1);
    }
    draw_rect(buf, sx - 5, sy - 5, sx + 5, sy, 3);
    draw_rect(buf, sx - 2, sy - 9, sx + 2, sy - 5, 1);
    draw_pixel(buf, sx, sy - 10, 7);

    // Left SRB
    draw_rect(buf, sx - 16, sy + 4, sx - 11, sy + 48, 7);
    draw_rect(buf, sx - 15, sy - 2, sx - 12, sy + 4, 7);
    draw_pixel(buf, sx - 14, sy - 4, 7);
    draw_rect(buf, sx - 16, sy + 4, sx - 16, sy + 48, 5);  // shadow
    draw_rect(buf, sx - 16, sy + 18, sx - 11, sy + 20, 6); // attach ring

    // Right SRB
    draw_rect(buf, sx + 11, sy + 4, sx + 16, sy + 48, 7);
    draw_rect(buf, sx + 12, sy - 2, sx + 15, sy + 4, 7);
    draw_pixel(buf, sx + 14, sy - 4, 7);
    draw_rect(buf, sx + 16, sy + 4, sx + 16, sy + 48, 5);
    draw_rect(buf, sx + 11, sy + 18, sx + 16, sy + 20, 6);

    // Orbiter
    draw_rect(buf, sx + 7, sy + 4, sx + 16, sy + 33, 7);   // body
    draw_rect(buf, sx + 7, sy + 31, sx + 22, sy + 42, 7);  // wing
    draw_rect(buf, sx + 7, sy + 39, sx + 12, sy + 48, 7);  // tail
    // Tail fin
    draw_rect(buf, sx + 8, sy + 36, sx + 10, sy + 48, 7);
    // Cockpit
    draw_rect(buf, sx + 7, sy + 5, sx + 9, sy + 9, 0);
    draw_pixel(buf, sx + 8, sy + 6, 6);
    // Wing shadow
    for (int wy2 = sy + 31; wy2 <= sy + 42; wy2++) draw_pixel(buf, sx + 22, wy2, 5);
}

void init_challenger() {}

void play_challenger(uint8_t *buf, int frame) {
    int cycle = frame % 400;

    // ── PHASE 1: Liftoff & ascent ──────────────────────────────────────────────
    if (cycle < 120) {
        // Sky gradient (blue atmosphere)
        for (int y = 0; y < 240; y++) {
            int d = ((240 - y) * 15) / 240;
            for (int x = 0; x < 320; x++) {
                int t = art_bayer4_at(x, y);
                draw_pixel(buf, x, y, (d >= t) ? 4 : 0);
            }
        }
        // Stars (early frames only)
        if (cycle < 50) draw_stars(buf, frame);

        // Horizon (earth's edge)
        draw_rect(buf, 0, 192, 319, 195, 4);
        art_dither_rect_v(buf, 0, 196, 319, 215, 4, 5, 20);
        draw_rect(buf, 0, 215, 319, 239, 0); // dark earth

        // Earth surface texture
        for (int ex = 0; ex < 320; ex += 30) {
            draw_circle(buf, ex + 10, 228, 6, 4);  // landmasses
        }

        int alt = (cycle * 160) / 119;
        int sy = 200 - alt;
        int sx = 160;

        // Launchpad (only visible early)
        if (cycle < 30) {
            draw_rect(buf, sx - 35, 232, sx + 35, 239, 7);
            // Service structure arms
            draw_rect(buf, sx - 35, 195, sx - 33, 232, 7);
            draw_rect(buf, sx + 33, 195, sx + 35, 232, 7);
            for (int arm = 200; arm < 230; arm += 15) {
                draw_rect(buf, sx - 33, arm, sx - 18, arm + 2, 7);
                draw_rect(buf, sx + 18, arm, sx + 33, arm + 2, 7);
            }
        }

        // Smoke cloud at pad (grows over time)
        for (int bx2 = sx - 60; bx2 <= sx + 60; bx2 += 20) {
            int sr = 12 + (bx2 % 11) + cycle/5;
            if (sr > 30) sr = 30;
            draw_circle(buf, bx2, 235, sr, 7);
            draw_circle(buf, bx2 + 6, 232, sr - 4, 6);
        }

        // Rising contrail
        for (int ty = sy + 48; ty < 240; ty += 2) {
            int spread = (ty - (sy + 48)) / 5 + 2;
            draw_rect(buf, sx - spread - 2, ty, sx + spread + 2, ty + 1,
                      ((ty - sy) % 12 < 6) ? 7 : 6);
        }

        // Engine flames
        int base_y = sy + 51;
        for (int e = -1; e <= 1; e++) {
            int ef = 4 + (cycle + e) % 4;
            draw_circle(buf, sx + e * 13, base_y, ef, 7);
            draw_circle(buf, sx + e * 13, base_y + 3, ef, 3);
            draw_circle(buf, sx + e * 13, base_y + 6, ef - 1, 1);
            draw_circle(buf, sx + e * 13, base_y + 9, ef - 2, 5);
        }

        draw_challenger_stack(buf, sx, sy);

        // Title text
        draw_string(buf, "CHALLENGER", 85, 6, 1, 7);
        draw_string(buf, "STS-51-L", 100, 16, 1, 6);
        draw_string(buf, "JAN 28 1986", 88, 26, 1, 6);

    // ── PHASE 2: Breakup ─────────────────────────────────────────────────────
    } else if (cycle < 200) {
        int t = cycle - 120;  // 0..79

        // Flash bright at start
        if (t < 5) {
            // White flash
            for (int y = 0; y < 240; y++)
                for (int x = 0; x < 320; x++)
                    draw_pixel(buf, x, y, 7);
        } else if (t < 12) {
            // Yellow flash
            for (int y = 0; y < 240; y++)
                for (int x = 0; x < 320; x++)
                    draw_pixel(buf, x, y, 3);
        } else {
            // Blue sky
            for (int y = 0; y < 240; y++) {
                int d = ((240 - y) * 15) / 240;
                for (int x = 0; x < 320; x++) {
                    int t2 = art_bayer4_at(x, y);
                    draw_pixel(buf, x, y, (d >= t2) ? 4 : 0);
                }
            }
        }

        int ex = 160, ey = 60 - t / 4;
        int cr = 8 + t / 4;
        if (cr > 28) cr = 28;

        // Explosion cloud
        draw_circle(buf, ex, ey, cr + 4, 6);     // outer cyan rim
        draw_circle(buf, ex, ey, cr, 7);          // white
        draw_circle(buf, ex, ey, cr - 4, 3);      // yellow
        draw_circle(buf, ex, ey, cr - 8, 1);      // red
        draw_circle(buf, ex, ey, cr - 11, 7);     // white hot center

        // Smoke billowing outward
        static const int cos8[8] = { 10, 7, 0,-7,-10,-7, 0, 7};
        static const int sin8[8] = {  0, 7,10, 7,  0,-7,-10,-7};
        for (int i = 0; i < 8; i++) {
            int dist = 14 + t / 3;
            int bx2 = ex + (cos8[i] * dist) / 10;
            int by2 = ey + (sin8[i] * dist) / 10;
            int br = 5 + t / 7;
            if (br > 14) br = 14;
            draw_circle(buf, bx2, by2, br, 7);
            // Black shadow inside each puff
            draw_circle(buf, bx2 + 2, by2 + 2, br - 3, 0);
        }

        // SRBs diverging
        int sep = t * 2;
        // Left SRB
        draw_rect(buf, ex - 14 - sep, ey + 12, ex - 10 - sep, ey + 50, 7);
        draw_rect(buf, ex - 14 - sep, ey + 12, ex - 14 - sep, ey + 50, 5);
        draw_circle(buf, ex - 12 - sep, ey + 53, 4 + t/18, 3);
        draw_circle(buf, ex - 12 - sep, ey + 57, 3 + t/20, 1);
        // Right SRB
        draw_rect(buf, ex + 10 + sep, ey + 12, ex + 14 + sep, ey + 50, 7);
        draw_rect(buf, ex + 14 + sep, ey + 12, ex + 14 + sep, ey + 50, 5);
        draw_circle(buf, ex + 12 + sep, ey + 53, 4 + t/18, 3);
        draw_circle(buf, ex + 12 + sep, ey + 57, 3 + t/20, 1);

        // Debris
        for (int i = 0; i < 10; i++) {
            int seed = i * 41 + t;
            int ddx = (i % 2 == 0 ? 1 : -1) * (8 + i * 7 + seed % 18);
            int ddy = 25 + i * 10 + t / 2;
            draw_rect(buf, ex + ddx, ey + ddy, ex + ddx + 2, ey + ddy + 6, 1);
            draw_pixel(buf, ex + ddx, ey + ddy + 7, 3);
        }

        draw_string(buf, "CHALLENGER", 85, 6, 1, 7);
        draw_string(buf, "JAN 28 1986", 88, 16, 1, 1);

    // ── PHASE 3: Iconic Y-cloud ───────────────────────────────────────────────
    } else {
        int t = cycle - 200;  // 0..199

        // Blue sky
        for (int y = 0; y < 240; y++) {
            int d = ((240 - y) * 15) / 240;
            for (int x = 0; x < 320; x++) {
                int t2 = art_bayer4_at(x, y);
                draw_pixel(buf, x, y, (d >= t2) ? 4 : 0);
            }
        }

        int ccx = 160;
        int stem_base = 225;
        int stem_top = 100 + t / 8;
        if (stem_top > 160) stem_top = 160;

        // Stem: wide billowing column
        for (int y2 = stem_top; y2 < stem_base; y2 += 1) {
            int spread = 8 + (stem_base - y2) / 10;
            // Layered colors: white outside, cyan/gray inside shadow
            draw_rect(buf, ccx - spread, y2, ccx + spread, y2, 7);
            // Shadow inside
            art_dither_pixel(buf, ccx - spread + 2, y2, 6, 2);
            art_dither_pixel(buf, ccx + spread - 2, y2, 6, 2);
        }
        // Stem glow core
        for (int y2 = stem_top + 20; y2 < stem_base; y2++) {
            art_dither_pixel(buf, ccx, y2, 3, 2);
        }

        // Left arm of Y
        {
            int arm_len = 55 + t / 3;
            if (arm_len > 125) arm_len = 125;
            for (int i = 0; i < arm_len; i += 2) {
                int ax2 = ccx - 6 - i - i * i / 55;
                int ay2 = stem_top - 8 - i + i * i / 75;
                if (ay2 < 8) ay2 = 8;
                int ar = 6 + i / 18;
                if (ar > 16) ar = 16;
                uint8_t ac = (i < 25) ? 3 : ((i < 60) ? 7 : 6);
                draw_circle(buf, ax2, ay2, ar, ac);
                // Dark shadow on each puff
                draw_circle(buf, ax2 + ar/3, ay2 + ar/3, ar/3, 0);
            }
            // SRB at tip
            int ax_tip = ccx - 6 - arm_len - arm_len*arm_len/55;
            int ay_tip = stem_top - 8 - arm_len + arm_len*arm_len/75;
            if (ay_tip < 8) ay_tip = 8;
            if (ax_tip > 8) {
                draw_rect(buf, ax_tip - 4, ay_tip, ax_tip + 4, ay_tip + 22, 7);
                draw_rect(buf, ax_tip - 4, ay_tip, ax_tip - 4, ay_tip + 22, 5);
                draw_circle(buf, ax_tip, ay_tip + 25, 5, 3);
                draw_circle(buf, ax_tip, ay_tip + 29, 3, 1);
            }
        }

        // Right arm of Y
        {
            int arm_len = 55 + t / 3;
            if (arm_len > 125) arm_len = 125;
            for (int i = 0; i < arm_len; i += 2) {
                int ax2 = ccx + 6 + i + i * i / 55;
                int ay2 = stem_top - 8 - i + i * i / 75;
                if (ay2 < 8) ay2 = 8;
                int ar = 6 + i / 18;
                if (ar > 16) ar = 16;
                uint8_t ac = (i < 25) ? 3 : ((i < 60) ? 7 : 6);
                draw_circle(buf, ax2, ay2, ar, ac);
                draw_circle(buf, ax2 - ar/3, ay2 + ar/3, ar/3, 0);
            }
            int ax_tip = ccx + 6 + arm_len + arm_len*arm_len/55;
            int ay_tip = stem_top - 8 - arm_len + arm_len*arm_len/75;
            if (ay_tip < 8) ay_tip = 8;
            if (ax_tip < 312) {
                draw_rect(buf, ax_tip - 4, ay_tip, ax_tip + 4, ay_tip + 22, 7);
                draw_rect(buf, ax_tip + 4, ay_tip, ax_tip + 4, ay_tip + 22, 5);
                draw_circle(buf, ax_tip, ay_tip + 25, 5, 3);
                draw_circle(buf, ax_tip, ay_tip + 29, 3, 1);
            }
        }

        // Main explosion cloud (mushroom top)
        {
            int cr = 22 + t / 6;
            if (cr > 50) cr = 50;
            draw_circle(buf, ccx,      stem_top - 12, cr,      7);  // white
            draw_circle(buf, ccx - 18, stem_top - 6,  cr - 8,  7);  // left puff
            draw_circle(buf, ccx + 18, stem_top - 6,  cr - 8,  7);  // right puff
            draw_circle(buf, ccx,      stem_top - 22, cr - 5,  6);  // cyan shadow
            draw_circle(buf, ccx,      stem_top - 12, cr / 2,  3);  // yellow core
            draw_circle(buf, ccx,      stem_top - 12, cr / 4,  1);  // red center
            draw_circle(buf, ccx,      stem_top - 12, cr / 8,  7);  // white hot
            // Shadow on each large puff
            draw_circle(buf, ccx + cr/2, stem_top - 5, cr/3, 0);
        }

        // Falling debris trails
        for (int i = 0; i < 10; i++) {
            int seed = i * 59;
            int ddx = (i % 2 == 0 ? 1 : -1) * (18 + i * 16 + seed % 25);
            int dy_s = stem_top + 30;
            int dy_e = dy_s + 50 + t / 3 + i * 10;
            if (dy_e > 232) dy_e = 232;
            if (t > i * 7) {
                draw_rect(buf, ccx + ddx, dy_s, ccx + ddx + 2, dy_e, 1);
                // Burning tip
                draw_circle(buf, ccx + ddx + 1, dy_e + 1, 2, 3);
                draw_pixel(buf, ccx + ddx + 1, dy_e + 3, 1);
            }
        }

        // Launchpad smoke at base
        for (int bx2 = 90; bx2 <= 240; bx2 += 18) {
            int sr = 10 + (bx2 % 11) + t / 12;
            if (sr > 28) sr = 28;
            draw_circle(buf, bx2, 232, sr, 7);
            draw_circle(buf, bx2 + 4, 229, sr - 5, 6);
        }

        // Big Bird Memorial Grave
        draw_rect(buf, 260, 205, 290, 235, 4); // dark blue/grey stone
        draw_rect(buf, 262, 207, 288, 209, 7); // white top border
        draw_string(buf, "RIP", 269, 212, 1, 7);
        draw_string(buf, "B.BIRD", 264, 222, 1, 7);
        // Draw a dithered yellow feather next to the grave
        draw_rect(buf, 245, 225, 252, 235, 3); // yellow feather block
        draw_pixel(buf, 248, 223, 7); // white stem
        draw_pixel(buf, 247, 224, 7);

        // Caption
        draw_string(buf, "CHALLENGER", 85, 6, 1, 7);
        draw_string(buf, "STS-51-L", 100, 16, 1, 6);
        draw_string(buf, "JAN 28 1986", 88, 26, 1, 1);
        // Memorial note
        if (t > 100) {
            draw_string(buf, "IN MEMORIAM", 90, 36, 1, 5);
        }
    }
}

void init_nine_eleven() {}

void play_nine_eleven(uint8_t *buf, int frame) {
    int cycle = frame % 450;

    if (cycle < 220) {
        // --- MORNING SKY (Blue gradient) ---
        for (int y = 0; y < 180; y++) {
            int d = (y * 15) / 180;
            for (int x = 0; x < 320; x++) {
                int t = art_bayer4_at(x, y);
                draw_pixel(buf, x, y, (d >= t) ? 6 : 4);
            }
        }

        // --- LOWER MANHATTAN SKYLINE (Silhouette) ---
        static const int bh[8]  = {50, 70, 85, 60, 90, 75, 65, 50};
        static const int bx[8]  = {20, 45, 75, 110, 185, 215, 250, 275};
        static const int bw[8]  = {20, 25, 30, 20, 25, 30, 20, 30};

        for (int i = 0; i < 8; i++) {
            draw_rect(buf, bx[i], 180 - bh[i], bx[i] + bw[i], 180, 0);
        }

        // --- TWIN TOWERS ---
        // North Tower (1 WTC)
        draw_rect(buf, 135, 70, 150, 180, 7); // white body
        for (int y = 70; y < 180; y += 4) {
            draw_rect(buf, 138, y, 139, y + 2, 0);
            draw_rect(buf, 142, y, 143, y + 2, 0);
            draw_rect(buf, 146, y, 147, y + 2, 0);
        }
        // Antenna on North Tower
        draw_rect(buf, 142, 45, 143, 69, 0);

        // South Tower (2 WTC)
        draw_rect(buf, 160, 72, 175, 180, 7);
        for (int y = 72; y < 180; y += 4) {
            draw_rect(buf, 163, y, 164, y + 2, 0);
            draw_rect(buf, 167, y, 168, y + 2, 0);
            draw_rect(buf, 171, y, 172, y + 2, 0);
        }

        // --- HUDSON RIVER ---
        for (int y = 181; y < 240; y++) {
            int d = ((y - 181) * 15) / 59;
            for (int x = 0; x < 320; x++) {
                int t = art_bayer4_at(x, y);
                draw_pixel(buf, x, y, (d >= t) ? 0 : 4);
            }
        }

        // --- PLANE 1: Flight 11 (Approaches North Tower) ---
        if (cycle < 70) {
            int px = -30 + (cycle * 172) / 70;
            int py = 85;
            draw_rect(buf, px - 12, py, px + 12, py, 0); // body
            draw_rect(buf, px - 3, py - 5, px + 3, py + 5, 0); // wings
            draw_pixel(buf, px - 11, py - 2, 0);
            draw_pixel(buf, px - 11, py + 2, 0);
        }

        // --- PLANE 2: Flight 175 (Approaches South Tower, banked) ---
        if (cycle >= 70 && cycle < 140) {
            int t = cycle - 70;
            int px = 350 - (t * 183) / 70;
            int py = 75 + (t * 20) / 70;
            // Banked plane silhouette (drawn slightly angled)
            draw_rect(buf, px - 12, py, px + 12, py, 0);
            draw_rect(buf, px - 5, py - 7, px - 1, py + 7, 0); // banked wings
            draw_pixel(buf, px + 11, py - 2, 0);
            draw_pixel(buf, px + 11, py + 2, 0);
        }

        // --- EXPLOSIONS & SMOKE ---
        // North Tower (WTC 1) hit at cycle = 70
        if (cycle >= 70) {
            int t1 = cycle - 70;
            int ex = 142, ey = 85;
            // Explosion flash
            if (t1 < 15) {
                int r = t1 / 2 + 2;
                draw_circle(buf, ex, ey, r + 6, 7);
                draw_circle(buf, ex, ey, r + 2, 3);
                draw_circle(buf, ex, ey, r, 1);
            }
            // Billowing smoke column
            for (int i = 0; i < 6; i++) {
                int sy_pos = ey - (t1 * 2) / 3 - i * 12;
                int sx_pos = ex + (t1 / 6) + (i * 4) % 15 - 7;
                int sr = 7 + i + t1 / 12;
                if (sr > 16) sr = 16;
                if (sy_pos > 20) {
                    draw_circle(buf, sx_pos, sy_pos, sr, 0); // black smoke
                    draw_circle(buf, sx_pos + 1, sy_pos - 1, sr - 3, 4); // grey highlight
                }
            }
        }

        // South Tower (WTC 2) hit at cycle = 140
        if (cycle >= 140) {
            int t2 = cycle - 140;
            int ex = 167, ey = 95;
            // Explosion flash
            if (t2 < 15) {
                int r = t2 / 2 + 2;
                draw_circle(buf, ex, ey, r + 8, 7);
                draw_circle(buf, ex, ey, r + 3, 3);
                draw_circle(buf, ex, ey, r, 1);
            }
            // Billowing smoke column
            for (int i = 0; i < 6; i++) {
                int sy_pos = ey - (t2 * 2) / 3 - i * 12;
                int sx_pos = ex + (t2 / 6) + (i * 3) % 13 - 6;
                int sr = 7 + i + t2 / 12;
                if (sr > 16) sr = 16;
                if (sy_pos > 20) {
                    draw_circle(buf, sx_pos, sy_pos, sr, 0); // black smoke
                    draw_circle(buf, sx_pos + 1, sy_pos - 1, sr - 3, 4); // grey highlight
                }
            }
        }

        draw_string(buf, "NEW YORK CITY", 105, 10, 1, 7);
        draw_string(buf, "SEPTEMBER 11, 2001", 85, 22, 1, 6);

    } else {
        // --- PHASE 3: Tribute in Light (Night) ---
        for (int y = 0; y < 180; y++) {
            for (int x = 0; x < 320; x++) {
                draw_pixel(buf, x, y, 0);
            }
        }
        draw_stars(buf, frame);

        static const int bh[8]  = {50, 70, 85, 60, 90, 75, 65, 50};
        static const int bx[8]  = {20, 45, 75, 110, 185, 215, 250, 275};
        static const int bw[8]  = {20, 25, 30, 20, 25, 30, 20, 30};

        for (int i = 0; i < 8; i++) {
            draw_rect(buf, bx[i], 180 - bh[i], bx[i] + bw[i], 180, 4);
        }

        for (int y = 0; y < 180; y++) {
            int density = 4 - (y * 4) / 180;
            if (density < 1) density = 1;
            
            for (int x = 138; x <= 147; x++) {
                art_dither_pixel(buf, x, y, 6, density);
                art_dither_pixel(buf, x, y, 7, density - 1);
            }
            for (int x = 163; x <= 172; x++) {
                art_dither_pixel(buf, x, y, 6, density);
                art_dither_pixel(buf, x, y, 7, density - 1);
            }
        }

        for (int y = 181; y < 240; y++) {
            int d = ((y - 181) * 15) / 59;
            for (int x = 0; x < 320; x++) {
                int t = art_bayer4_at(x, y);
                draw_pixel(buf, x, y, (d >= t) ? 0 : 4);
            }
            int wave = ((frame / 3 + y * 2) % 10) - 5;
            int rrx1 = 142 + wave;
            int rrx2 = 167 + wave;
            for (int x = rrx1 - 4; x <= rrx1 + 4; x++) {
                art_dither_pixel(buf, x, y, 6, 2);
            }
            for (int x = rrx2 - 4; x <= rrx2 + 4; x++) {
                art_dither_pixel(buf, x, y, 6, 2);
            }
        }

        draw_string(buf, "9/11 MEMORIAL", 98, 15, 1, 7);
        draw_string(buf, "WE WILL NEVER FORGET", 80, 27, 1, 3);
    }
}

// ============================================================================
//  STATE 30: Computer Pinup
// ============================================================================
void init_pinup() {}

void play_pinup(uint8_t *buf, int frame) {
    int mode = (frame / 500) % 2;
    if (mode == 0) {
        // ── Scene 1: Retro Computer Pinup Girl (Original, Enhanced) ──────────────
        // Ambient background: deep blue to magenta vertical gradient scanlines
        for (int y = 0; y < 240; y++) {
            uint8_t bg_color = 0; // Black base
            if (y < 80) bg_color = (y % 4 == 0) ? 5 : 0; // Magenta stars
            else bg_color = (y % 8 == 0) ? 4 : 0; // Blue grid lines
            for (int x = 0; x < 320; x++) {
                draw_pixel(buf, x, y, bg_color);
            }
        }

        // Draw a beautiful synthwave grid floor with converging perspective
        for (int x = 0; x < 320; x += 20) {
            for (int y = 160; y < 240; y++) {
                int px = 160 + ((x - 160) * (y - 130)) / 30;
                if (px >= 0 && px < 320) {
                    draw_pixel(buf, px, y, 6); // Cyan grid lines
                }
            }
        }
        // Scrolling horizontal floor lines
        int scroll = (frame / 2) % 12;
        for (int y = 160 + scroll; y < 240; y += 12) {
            draw_rect(buf, 0, y, 319, y, 6);
        }

        // ── DETAILED CRT MONITOR & WORKSTATION ──────────────────────────────────
        // Outer monitor casing with 3D dithered shading
        art_shaded_rect(buf, 25, 75, 125, 175, 7, 7, 0); // main grey casing
        // Back CRT tube bulge/vents
        draw_rect(buf, 35, 65, 115, 75, 0);
        for (int vx = 40; vx < 110; vx += 6) {
            draw_rect(buf, vx, 68, vx + 2, 73, 7); // cooling vents
        }
        // Bezel shadow and inner bezel
        draw_rect(buf, 30, 80, 120, 170, 0);
        draw_rect(buf, 32, 82, 118, 168, 4); // blue inner bezel

        // CRT Screen (Green phosphor glow)
        draw_rect(buf, 38, 88, 112, 142, 2); // Screen face
        // Raster scanline overlay
        for (int y = 88; y <= 142; y += 2) {
            draw_rect(buf, 38, y, 112, y, 0);
        }
        // Screen text & cursor
        draw_string(buf, "C:\\> RUN PINUP", 42, 92, 1, 3); // Yellow prompt
        draw_string(buf, "STATUS: OK", 42, 102, 1, 7); // White text
        draw_string(buf, "VRAM: 640KB", 42, 112, 1, 6); // Cyan info
        draw_string(buf, "MODE: MAX-RES", 42, 122, 1, 7);
        if (frame % 20 < 10) {
            draw_rect(buf, 100, 122, 104, 129, 3); // blinking cursor
        }

        // Monitor controls & LED
        draw_rect(buf, 35, 155, 115, 165, 0); // control strip
        draw_circle(buf, 45, 160, 2, 7); // dials
        draw_circle(buf, 55, 160, 2, 7);
        // Power LED (Green / Orange blinking)
        uint8_t led_col = (frame % 30 < 15) ? 2 : 3;
        draw_circle(buf, 105, 160, 2, led_col);

        // Monitor swivel stand
        art_shaded_rect(buf, 60, 175, 90, 190, 7, 7, 0);

        // Keyboard & desk accessories
        draw_rect(buf, 15, 190, 115, 202, 7); // Keyboard base
        draw_rect(buf, 18, 193, 112, 199, 0); // Key well
        for (int kx = 21; kx < 110; kx += 4) {
            draw_pixel(buf, kx, 195, 7); // individual keys
            draw_pixel(buf, kx + 1, 197, 7);
        }
        // Floppy Disks
        draw_rect(buf, 122, 192, 137, 202, 1); // Red 3.5" floppy
        draw_rect(buf, 125, 189, 130, 196, 7); // white label
        draw_rect(buf, 132, 192, 147, 202, 4); // Blue 3.5" floppy

        // ── HIGH-RESOLUTION PINUP GIRL (Sitting on CRT) ─────────────────────────
        // Detailed hair (Magenta and Red locks flowing over shoulders)
        int h_center = 125;
        // Ponytail/back hair with highlight shading
        draw_circle(buf, h_center + 10, 48, 16, 5); // Main hair mass
        draw_circle(buf, h_center + 18, 55, 12, 1); // Red hair highlights
        draw_rect(buf, h_center + 4, 52, h_center + 14, 85, 5); // Flowing lock
        draw_rect(buf, h_center + 8, 60, h_center + 18, 90, 1);

        // Face & head profile
        draw_circle(buf, h_center, 46, 9, 3); // Face skin base
        draw_circle(buf, h_center - 2, 48, 8, 3);
        // Hair bangs and front fringe
        draw_circle(buf, h_center - 4, 40, 5, 5);
        draw_rect(buf, h_center - 6, 38, h_center + 4, 44, 5);
        // Face features
        draw_pixel(buf, h_center - 5, 46, 0); // Detailed eye
        draw_pixel(buf, h_center - 6, 47, 0); // eye lash
        draw_pixel(buf, h_center - 3, 50, 1); // Lips (Red lipstick)
        draw_pixel(buf, h_center - 4, 50, 1);
        draw_pixel(buf, h_center - 1, 48, 5); // Blushing cheek

        // Elegant Neck & Shoulders
        draw_rect(buf, h_center - 1, 55, h_center + 4, 63, 3); // Neck
        draw_pixel(buf, h_center - 2, 57, 3);

        // Torso / Bustier (Blue corset with Yellow/Magenta stitching and laces)
        draw_rect(buf, 120, 63, 142, 92, 4); // Blue corset body
        draw_rect(buf, 124, 60, 138, 63, 3); // Exposed chest/collarbone
        draw_rect(buf, 128, 67, 134, 88, 5); // Magenta center panel
        // Lacing details
        for (int ly = 70; ly <= 84; ly += 4) {
            draw_pixel(buf, 130, ly, 3); // Golden laces
            draw_pixel(buf, 132, ly, 3);
        }

        // Arms: Leaning back support
        // Left arm resting back on the monitor
        draw_rect(buf, 138, 65, 155, 71, 3); // Upper arm
        draw_rect(buf, 148, 71, 154, 98, 3); // Forearm
        draw_rect(buf, 146, 98, 152, 102, 3); // Hand resting
        // Right arm resting on leg
        draw_rect(buf, 116, 65, 122, 75, 3); // Shoulder/upper
        draw_rect(buf, 110, 75, 118, 92, 3); // Forearm
        draw_rect(buf, 108, 92, 114, 96, 3); // Hand

        // Legs: Detailed white thigh-high boots with red details, crossed elegantly
        // Thighs (Skin color 3)
        draw_rect(buf, 120, 92, 138, 110, 3);
        art_dither_pixel(buf, 138, 100, 5, 2); // shadow under buttocks

        // Crossed leg (Over)
        draw_rect(buf, 116, 110, 140, 124, 3); // Thigh
        // Thigh-high top rim (Red lace band)
        draw_rect(buf, 114, 124, 134, 127, 1);
        // Left Boot (White with Red laces/details)
        draw_rect(buf, 112, 128, 130, 165, 7); // boot leg
        for (int lly = 132; lly <= 160; lly += 6) {
            draw_pixel(buf, 120, lly, 1); // Red laces
            draw_pixel(buf, 121, lly + 1, 1);
        }
        draw_rect(buf, 108, 160, 126, 167, 7); // boot foot pointing down

        // Hanging leg (Under)
        draw_rect(buf, 132, 110, 146, 135, 3); // Thigh
        draw_rect(buf, 134, 135, 144, 138, 1); // Red top rim
        draw_rect(buf, 133, 139, 143, 180, 7); // Boot leg
        for (int rly = 142; rly <= 176; rly += 6) {
            draw_pixel(buf, 137, rly, 1); // laces
        }
        draw_rect(buf, 133, 178, 148, 184, 7); // boot foot

        // Dynamic glowing Neon Hearts next to her (Heart beating animation)
        int beat = (frame / 6) % 3;
        int hx = 175, hy = 55;
        draw_circle(buf, hx - 5, hy, 5 + beat, 1);
        draw_circle(buf, hx + 5, hy, 5 + beat, 1);
        draw_rect(buf, hx - 9, hy, hx + 9, hy + 5, 1);
        for (int px = hx - 9; px <= hx + 9; px++) {
            int p_depth = hy + 5 + (px < hx ? (px - (hx - 9)) : ((hx + 9) - px));
            draw_rect(buf, px, hy + 5, px, p_depth, 1);
        }
        // Inner glowing core of the heart
        draw_circle(buf, hx - 4, hy + 1, 2, 7);
        draw_circle(buf, hx + 4, hy + 1, 2, 7);

        // Title Overlay text
        draw_string(buf, "RETRO PINUP V2.0", 175, 215, 1, 5);
        draw_string(buf, "MAX PIXELS ACTIVE", 175, 225, 1, 6);
    } else {
        // ── Scene 2: Queer Synthwave Pinup (Enhanced & Waving Flag) ──────────────
        // Clear background with neon grid and stars
        for (int y = 0; y < 240; y++) {
            int ambient = (y % 12 == 0) ? 6 : 0; // Cyan grid lines
            for (int x = 0; x < 320; x++) {
                draw_pixel(buf, x, y, ambient);
            }
        }
        draw_stars(buf, frame);

        // Giant Synthwave Sunset Sun (Trans Flag Theme)
        int sun_cx = 210, sun_cy = 90, sun_r = 60;
        for (int y = sun_cy - sun_r; y <= sun_cy + sun_r; y++) {
            // Synthwave horizontal gaps cutting the sun
            if (y > sun_cy && (y % 8 == 0 || y % 8 == 1 || y % 8 == 2)) continue;
            
            int dy = y - sun_cy;
            int dx_max = isqrt(sun_r * sun_r - dy * dy);
            for (int x = sun_cx - dx_max; x <= sun_cx + dx_max; x++) {
                // Trans pride colors in stripes
                int stripe = (y - (sun_cy - sun_r)) * 5 / (sun_r * 2);
                uint8_t col = 6; // Cyan
                if (stripe == 1 || stripe == 3) col = 5; // Magenta
                else if (stripe == 2) col = 7; // White
                draw_pixel(buf, x, y, col);
            }
        }

        // ── Retro Workstation (Left Side) ───────────────────────────────────────
        art_shaded_rect(buf, 15, 85, 105, 175, 7, 7, 0); // Casing
        draw_rect(buf, 20, 90, 100, 150, 0); // Bezel
        draw_rect(buf, 24, 94, 96, 146, 6); // Screen
        // Raster lines
        for (int y = 94; y <= 146; y += 2) {
            draw_rect(buf, 24, y, 96, y, 0);
        }
        draw_string(buf, "SYS: ACTIVE", 28, 98, 1, 5);
        draw_string(buf, "QUEER_GFX", 28, 110, 1, 7);
        draw_string(buf, "VGA_MODE", 28, 122, 1, 3);

        // Keyboard & desk
        draw_rect(buf, 10, 185, 110, 195, 7);
        draw_rect(buf, 5, 195, 150, 205, 0); // table outline

        // ── Queer Butch Pinup Model (Center-Right) ──────────────────────────────
        // Asymmetric pixie cut hair in Magenta and Cyan
        int px_c = 165, py_c = 65;
        draw_circle(buf, px_c, py_c, 15, 5); // Magenta base
        draw_rect(buf, px_c - 12, py_c - 10, px_c + 6, py_c + 2, 6); // Cyan side-shave / undercut
        for (int hx = px_c - 10; hx <= px_c + 10; hx += 3) {
            draw_pixel(buf, hx, py_c - 12, 7); // White hair spikes/highlights
        }

        // Face & features
        draw_circle(buf, px_c - 4, py_c + 4, 9, 3); // Face skin
        draw_pixel(buf, px_c - 9, py_c + 4, 0); // eye
        draw_pixel(buf, px_c - 10, py_c + 5, 0); // eyebrow
        draw_pixel(buf, px_c - 8, py_c + 9, 1); // Red lipstick
        draw_pixel(buf, px_c - 7, py_c + 9, 1);

        // Leather Jacket (Black with White/Silver zippers and lapels)
        draw_rect(buf, px_c - 12, py_c + 14, px_c + 22, py_c + 60, 0); // Jacket body
        draw_rect(buf, px_c - 6, py_c + 14, px_c, py_c + 30, 7); // Silver collar zipper
        draw_pixel(buf, px_c - 10, py_c + 18, 7); // Shoulder studs
        draw_pixel(buf, px_c + 18, py_c + 18, 7);

        // Tank top (Trans pride stripes under jacket)
        draw_rect(buf, px_c - 2, py_c + 22, px_c + 12, py_c + 40, 6); // Cyan stripe
        draw_rect(buf, px_c - 2, py_c + 27, px_c + 12, py_c + 32, 7); // White stripe
        draw_rect(buf, px_c - 2, py_c + 33, px_c + 12, py_c + 38, 5); // Magenta stripe

        // Left arm resting on hip
        draw_rect(buf, px_c - 24, py_c + 20, px_c - 12, py_c + 26, 3); // Upper arm
        draw_rect(buf, px_c - 26, py_c + 26, px_c - 20, py_c + 42, 3); // Forearm
        draw_rect(buf, px_c - 28, py_c + 42, px_c - 22, py_c + 46, 3); // Hand on hip

        // Right arm holding flagpole
        draw_rect(buf, px_c + 18, py_c + 22, px_c + 36, py_c + 28, 3); // Arm stretching out

        // ── DYNAMICALLY WAVING PRIDE FLAG ─────────────────────────────────────
        int pole_x = px_c + 36;
        draw_rect(buf, pole_x, py_c - 10, pole_x + 2, py_c + 70, 7); // Flagpole
        draw_circle(buf, pole_x + 1, py_c - 13, 3, 3); // Gold tip
        
        // Waving Trans Pride Flag
        int flag_start_x = pole_x + 3;
        int flag_end_x = flag_start_x + 40;
        int flag_width = flag_end_x - flag_start_x;
        for (int fx = flag_start_x; fx <= flag_end_x; fx++) {
            // Wave offset calculated via sine wave
            float angle = (float)fx * 0.15f - (float)frame * 0.2f;
            int wave_offset = (int)(sinf(angle) * 5.0f);
            
            // Draw flag column
            int col_y = py_c - 5 + wave_offset;
            for (int fy = 0; fy < 30; fy++) {
                int stripe = fy * 5 / 30;
                uint8_t col = 6; // Cyan (outer)
                if (stripe == 1 || stripe == 3) col = 5; // Magenta (middle)
                else if (stripe == 2) col = 7; // White (core)
                draw_pixel(buf, fx, col_y + fy, col);
            }
        }

        // Legs: Tight Blue jeans & detailed boots
        draw_rect(buf, px_c - 8, py_c + 60, px_c + 18, py_c + 115, 4); // Jeans
        draw_rect(buf, px_c - 10, py_c + 115, px_c + 2, py_c + 140, 7); // Left Boot
        draw_rect(buf, px_c + 8, py_c + 115, px_c + 20, py_c + 140, 7); // Right Boot
        // Boot soles & heels (Black)
        draw_rect(buf, px_c - 12, py_c + 138, px_c + 4, py_c + 142, 0);
        draw_rect(buf, px_c + 6, py_c + 138, px_c + 22, py_c + 142, 0);

        // Title Overlay text
        draw_string(buf, "QUEER RETRO PINUP V2.0", 140, 215, 1, 6);
        draw_string(buf, "MAX DETAIL PINUP ACTIVE", 140, 225, 1, 7);
    }
}

// ============================================================================
//  STATE 31: Pride Flags
// ============================================================================
void init_pride() {}

void play_pride(uint8_t *buf, int frame) {
    // Clear screen to black
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            draw_pixel(buf, x, y, 0);
        }
    }
    // Draw background stars
    draw_stars(buf, frame);

    // Active flag cycles every 300 frames (about 4.5 seconds)
    int flag_type = (frame / 300) % 5; // 0: Trans, 1: Lesbian, 2: Gay, 3: Queer, 4: Genderfluid

    // Draw flagpole
    draw_rect(buf, 55, 35, 58, 220, 7); // silver pole
    draw_circle(buf, 56, 30, 4, 3);     // gold sphere

    // Flag dimensions
    int start_x = 59;
    int end_x = 270;
    int flag_h = 100;
    int base_y = 40;

    // Render the flag with a sine-wave wave effect
    for (int x = start_x; x <= end_x; x++) {
        float angle = (float)x * 0.04f - (float)frame * 0.12f;
        int wave = (int)(sinf(angle) * 7.0f);
        
        // Amplitude dampening: flag waves more at the free end
        float factor = (float)(x - start_x) / (float)(end_x - start_x);
        wave = (int)((float)wave * factor);

        for (int y = 0; y < flag_h; y++) {
            int py = base_y + y + wave;
            if (py < 0 || py >= 240) continue;

            uint8_t color = 0;
            if (flag_type == 0) {
                // ── TRANS PRIDE FLAG ──
                int stripe = y / 20;
                if (stripe == 0 || stripe == 4) color = 6; // Cyan (light blue)
                else if (stripe == 1 || stripe == 3) color = 5; // Magenta (pink)
                else color = 7; // White
            }
            else if (flag_type == 1) {
                // ── LESBIAN PRIDE FLAG ──
                int stripe = y / 20;
                if (stripe == 0) {
                    color = 1; // Red
                } else if (stripe == 1) {
                    color = ((x + py) % 2 == 0) ? 1 : 3; // Orange
                } else if (stripe == 2) {
                    color = 7; // White
                } else if (stripe == 3) {
                    color = ((x + py) % 2 == 0) ? 5 : 7; // Pink
                } else {
                    color = 5; // Magenta
                }
            }
            else if (flag_type == 2) {
                // ── GAY PRIDE ──
                int stripe = y / 16;
                if (stripe >= 6) stripe = 5;
                
                if (stripe == 0) color = 1; // Red
                else if (stripe == 1) color = ((x + py) % 2 == 0) ? 1 : 3; // Orange
                else if (stripe == 2) color = 3; // Yellow
                else if (stripe == 3) color = 2; // Green
                else if (stripe == 4) color = 4; // Blue
                else color = 5; // Violet
            }
            else if (flag_type == 3) {
                // ── QUEER PRIDE ──
                int stripe = y / 33;
                if (stripe >= 3) stripe = 2;
                
                if (stripe == 0) {
                    color = ((x + py) % 2 == 0) ? 5 : 7; // Lavender
                } else if (stripe == 1) {
                    color = 7; // White
                } else {
                    color = 2; // Green
                }
            }
            else {
                // ── GENDERFLUID PRIDE ──
                int stripe = y / 20;
                if (stripe >= 5) stripe = 4;

                if (stripe == 0) {
                    color = 5; // Pink (Magenta)
                } else if (stripe == 1) {
                    color = 7; // White
                } else if (stripe == 2) {
                    color = ((x + py) % 2 == 0) ? 5 : 4; // Purple (dithered Magenta and Blue)
                } else if (stripe == 3) {
                    color = 0; // Black
                } else {
                    color = 4; // Blue
                }
            }

            // Flag shadow shading for folds
            float slope = cosf(angle);
            if (slope < -0.6f) {
                if ((x + py) % 3 == 0) {
                    color = 0; // Shadow
                }
            }

            draw_pixel(buf, x, py, color);
        }
    }

    // Title text
    if (flag_type == 0) draw_string(buf, "TRANS PRIDE", 120, 200, 1, 6);
    else if (flag_type == 1) draw_string(buf, "LESBIAN PRIDE", 110, 200, 1, 1);
    else if (flag_type == 2) draw_string(buf, "GAY PRIDE", 130, 200, 1, 3);
    else if (flag_type == 3) draw_string(buf, "QUEER PRIDE", 120, 200, 1, 5);
    else draw_string(buf, "GENDERFLUID PRIDE", 100, 200, 1, 5);
}

// ============================================================================
//  STATE 32: Microsoft Xbox Pride Flag Mashup
// ============================================================================
void init_xbox_pride() {}

void play_xbox_pride(uint8_t *buf, int frame) {
    // 1. Draw a beautiful dark space background with neon grid lines and stars
    for (int y = 0; y < 240; y++) {
        uint8_t bg_col = (y % 16 == 0) ? 4 : 0; // blue lines
        for (int x = 0; x < 320; x++) {
            draw_pixel(buf, x, y, bg_col);
        }
    }
    
    // Draw a synthwave perspective grid on the floor
    for (int x = 0; x < 320; x += 30) {
        for (int y = 180; y < 240; y++) {
            int px = 160 + ((x - 160) * (y - 150)) / 30;
            if (px >= 0 && px < 320) {
                draw_pixel(buf, px, y, 6); // cyan grid
            }
        }
    }
    int grid_scroll = (frame / 2) % 12;
    for (int y = 180 + grid_scroll; y < 240; y += 12) {
        draw_rect(buf, 0, y, 319, y, 6);
    }
    
    draw_stars(buf, frame);

    // 2. Draw Xbox Pride logo sphere (pulse effect)
    int pulse = (int)(4.0f * sinf((float)frame * 0.08f));
    int radius = 55 + pulse;
    int cx = 160;
    int cy = 110;

    // Draw solid black circular outline
    draw_circle(buf, cx, cy, radius + 2, 0);
    draw_circle(buf, cx, cy, radius + 1, 0);

    for (int y = cy - radius; y <= cy + radius; y++) {
        for (int x = cx - radius; x <= cx + radius; x++) {
            int dx = x - cx;
            int dy = y - cy;
            int dist2 = dx*dx + dy*dy;
            if (dist2 <= radius*radius) {
                // Check if we are inside the colored segments (four quadrants)
                bool in_top = (dy < -6) && (abs(dx) < -dy * 0.75f);
                bool in_bottom = (dy > 6) && (abs(dx) < dy * 0.65f);
                bool in_left = (dx < -6) && (abs(dy) < -dx * 0.85f);
                bool in_right = (dx > 6) && (abs(dy) < dx * 0.85f);

                if (in_top || in_bottom || in_left || in_right) {
                    int stripe = (y - (cy - radius)) * 14 / (radius * 2);
                    uint8_t col = 7;
                    switch (stripe % 8) {
                        case 0: col = 1; break; // Red
                        case 1: col = 3; break; // Yellow
                        case 2: col = 2; break; // Green
                        case 3: col = 6; break; // Cyan
                        case 4: col = 4; break; // Blue
                        case 5: col = 5; break; // Magenta
                        case 6: col = 7; break; // White
                        case 7: col = 1; break; // Red/orange
                    }
                    
                    // Add 3D shading based on sphere surface normals
                    int shade = (dx * -3 + dy * -3) / (radius / 3);
                    int bayer = art_bayer4_at(x, y);
                    if (shade < -4 && bayer < 4) {
                        col = 0; // shadow edge
                    } else if (shade < -2 && bayer < 8) {
                        if (col == 7) col = 6;
                        else if (col == 3) col = 2;
                        else col = 0;
                    } else if (shade > 4 && bayer < 6) {
                        col = 7; // white specular highlight
                    }

                    draw_pixel(buf, x, y, col);
                } else {
                    // Inner "X" groove (Glossy white/silver/grey button casing)
                    int shade = (dx * -2 + dy * -2) / (radius / 4);
                    uint8_t gcol = 7; // Silver base
                    if (shade < -2) gcol = 6; // Cyan shadow
                    if (shade < -4) gcol = 0; // Black groove depth
                    draw_pixel(buf, x, y, gcol);
                }
            }
        }
    }

    // 3. Neon green/cyan outer glow around the Xbox sphere
    for (int r = radius + 3; r < radius + 8; r++) {
        for (int angle = 0; angle < 360; angle += 3) {
            float rad = (float)angle * 3.14159f / 180.0f;
            int px = cx + (int)(cosf(rad) * (float)r);
            int py = cy + (int)(sinf(rad) * (float)r);
            if (px >= 0 && px < 320 && py >= 0 && py < 240) {
                int dist_edge = r - radius;
                int bayer = art_bayer4_at(px, py);
                if (bayer > dist_edge * 3) {
                    draw_pixel(buf, px, py, 6); // glowing cyan halo
                }
            }
        }
    }

    // Title / overlay text
    draw_string(buf, "XBOX PRIDE MASHUP", 100, 195, 1, 3);
    draw_string(buf, "34 PRIDE FLAGS INCLUDED", 75, 207, 1, 7);
}
