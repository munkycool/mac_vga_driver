// splash.c — Random boot splash screen
// Draws a retro Mac monitor bezel with Adafruit Feather RP2040 VGA branding.
// Triggered randomly on startup (~1 in 3 chance).

#include "common.h"

// ---------------------------------------------------------------------------
// Color palette used (3-bit VGA, GP7-GP9 = R,G,B, GP10 = sync HIGH always)
//   0 = black   1 = red    2 = green  3 = yellow
//   4 = blue    5 = magenta 6 = cyan  7 = white
// ---------------------------------------------------------------------------

// --- Draw a filled rectangle (already in common.c as draw_rect) ---

// Draw the old-Mac-style monitor bezel and screen
static void draw_mac_monitor(uint8_t *buf, int ox, int oy) {
    // Outer bezel — warm grey (draw as white since no grey in 3-bit)
    // Use color 7 (white) as bezel base and 6 (cyan) for shadow trim
    int bw = 130, bh = 110;     // bezel width/height
    int sw = 96,  sh = 72;      // screen inset width/height
    int sx = ox + (bw - sw) / 2;
    int sy = oy + 14;

    // Outer bezel body — Ultra HD bezel with dithered depth
    art_shaded_rect(buf, ox, oy, ox + bw, oy + bh, 7, 7, 6);

    // Inner shadow border (dark edge effect)
    draw_rect(buf, ox + 2, oy + 2, ox + bw - 2, oy + bh - 2, 6);

    // Inner bezel body (lighter)
    draw_rect(buf, ox + 4, oy + 4, ox + bw - 4, oy + bh - 4, 7);

    // Screen background (classic Mac phosphor green-on-black)
    draw_rect(buf, sx, sy, sx + sw, sy + sh, 0);

    // Screen scanline tint — draw alternating faint green lines for CRT effect
    for (int row = sy + 2; row < sy + sh; row += 4) {
        for (int col = sx + 2; col <= sx + sw - 2; col++) {
            // draw_pixel clips automatically
            draw_rect(buf, col, row, col, row, 2);
        }
    }

    // ----- Screen content -----
    // "ADAFRUIT" top line
    draw_string(buf, "ADAFRUIT", sx + 6, sy + 6,  1, 2);
    // "FEATHER" second line
    draw_string(buf, "FEATHER",  sx + 6, sy + 14, 1, 2);
    // "RP2040" third line — slightly brighter
    draw_string(buf, "RP2040",   sx + 6, sy + 22, 1, 3);
    // "VGA" bold (scale 2) — centred on screen
    draw_string(buf, "VGA",      sx + 30, sy + 34, 2, 2);
    // small divider line
    draw_rect(buf, sx + 4, sy + 50, sx + sw - 4, sy + 51, 2);
    // "MAC VGA DRIVER" label below divider
    draw_string(buf, "MAC VGA", sx + 14, sy + 55, 1, 7);
    draw_string(buf, "DRIVER",  sx + 18, sy + 63, 1, 6);
    draw_string(buf, "4K UHD++", sx + 10, sy + 72, 1, 3);

    // Blinking cursor block at bottom-right of screen
    draw_rect(buf, sx + sw - 8, sy + sh - 8, sx + sw - 2, sy + sh - 2, 2);

    // Bottom disk drive slot (rectangle in lower bezel)
    int drive_x = ox + bw / 2 - 18;
    int drive_y = oy + bh - 18;
    draw_rect(buf, drive_x, drive_y, drive_x + 36, drive_y + 6,  0);
    draw_rect(buf, drive_x + 1, drive_y + 1, drive_x + 35, drive_y + 5, 6);

    // Floppy eject button
    draw_rect(buf, drive_x + 29, drive_y + 1, drive_x + 34, drive_y + 5, 7);

    // Small "power" LED dot (green, top right of bezel)
    draw_rect(buf, ox + bw - 10, oy + 8, ox + bw - 8, oy + 10, 2);

    // Stand / neck
    int neck_x = ox + bw / 2 - 6;
    int neck_y = oy + bh;
    draw_rect(buf, neck_x, neck_y, neck_x + 12, neck_y + 12, 7);
    draw_rect(buf, neck_x - 16, neck_y + 12, neck_x + 28, neck_y + 18, 7);
}

// Draw the Adafruit Feather RP2040 PCB (schematic/pixel art style)
static void draw_feather_pcb(uint8_t *buf, int ox, int oy) {
    // PCB body (green)
    int pw = 100, ph = 38;
    draw_rect(buf, ox, oy, ox + pw, oy + ph, 2);

    // PCB edge highlight
    draw_rect(buf, ox + 1, oy + 1, ox + pw - 1, oy + 1, 3);
    draw_rect(buf, ox + 1, oy + ph - 1, ox + pw - 1, oy + ph - 1, 3);

    // USB-C connector (left end, grey=white)
    draw_rect(buf, ox - 6, oy + ph/2 - 4, ox, oy + ph/2 + 4, 7);
    draw_rect(buf, ox - 5, oy + ph/2 - 3, ox - 1, oy + ph/2 + 3, 6);

    // RP2040 chip (black square, centred)
    int cx = ox + pw / 2 - 10;
    int cy = oy + ph / 2 - 8;
    draw_rect(buf, cx, cy, cx + 20, cy + 16, 0);
    draw_rect(buf, cx + 2, cy + 2, cx + 18, cy + 14, 5);
    // "RP" text on chip
    draw_string(buf, "RP", cx + 4, cy + 3, 1, 7);
    draw_string(buf, "2040", cx + 2, cy + 9, 1, 7);

    // NeoPixel LED (small colourful square, left of chip)
    draw_rect(buf, ox + 10, oy + ph/2 - 3, ox + 16, oy + ph/2 + 3, 4);
    draw_rect(buf, ox + 11, oy + ph/2 - 2, ox + 15, oy + ph/2 + 2, 5);

    // Pin header rows (top and bottom of PCB, yellow dots)
    for (int i = 0; i < 12; i++) {
        int px_t = ox + 6 + i * 8;
        draw_rect(buf, px_t, oy - 2, px_t + 3, oy, 3);        // top pins
        draw_rect(buf, px_t, oy + ph, px_t + 3, oy + ph + 2, 3); // bottom pins
    }

    // BOOT and RESET buttons (right side of PCB)
    draw_rect(buf, ox + pw - 10, oy + 6,  ox + pw - 4, oy + 10, 7);
    draw_rect(buf, ox + pw - 10, oy + 20, ox + pw - 4, oy + 24, 7);

    // Adafruit logo text on PCB
    draw_string(buf, "ADAFRUIT", ox + 2, oy + ph - 10, 1, 3);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void init_splash() {
    // Nothing to initialise — fully stateless
}

void play_splash(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);

    // --- Background: subtle starfield / dark gradient ---
    // Draw a few scattered white pixels as stars
    uint32_t star_seed = 42;
    for (int i = 0; i < 80; i++) {
        star_seed = star_seed * 1103515245 + 12345;
        int sx = (star_seed / 65536) % 320;
        star_seed = star_seed * 1103515245 + 12345;
        int sy = (star_seed / 65536) % 240;
        draw_pixel(buffer, sx, sy, 7);
    }

    // Title at very top
    draw_string(buffer, "ADAFRUIT FEATHER RP2040 VGA", 12, 8, 1, 3);
    draw_string(buffer, "MAC MONITOR DRIVER", 52, 18, 1, 6);

    // --- Draw old Mac monitor centred top area ---
    draw_mac_monitor(buffer, 90, 32);

    // --- Draw PCB schematic below monitor ---
    draw_feather_pcb(buffer, 108, 168);

    // --- Bottom credits ---
    // Blink the "RUNNING..." line using frame counter
    if ((frame_counter / 20) % 2 == 0) {
        draw_string(buffer, "BOOTING...", 110, 222, 1, 2);
    }
    draw_string(buffer, "RP2040 @ 151MHZ  VGA 640X480 67HZ", 8, 232, 1, 6);

    // Animated "progress bar" using frame_counter
    int bar_w = (frame_counter * 200) / 200; // grows over ~200 frames (~3s)
    if (bar_w > 200) bar_w = 200;
    draw_rect(buffer, 60, 214, 260, 219, 0);           // bar background
    draw_rect(buffer, 60, 214, 260, 219, 6);           // bar border (1px)
    if (bar_w > 0) {
        draw_rect(buffer, 61, 215, 60 + bar_w, 218, 2); // fill
    }
}
