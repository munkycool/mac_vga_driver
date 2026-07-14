#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

// Forward declarations from common.h — we don't include common.h here to avoid
// type conflicts on bad_apple_bin (array vs pointer).
typedef struct {
    bool up, down, left, right, action1, action2, skip, interactive;
    int timeout_ticks;
} InputState;
extern volatile InputState global_input;

// --- Global variables defined in stub headers or required by the screensaver ---
dma_hw_t dma_hw_inst = {0};
const pio_program_t vga_program = {0};

// bad_apple_bin is declared as const uint8_t[] in common.h;
// we store dynamically-loaded data behind a pointer cast.
static uint8_t *bad_apple_data = NULL;
const uint8_t *bad_apple_bin = NULL;
const uint8_t *bad_apple_bin_end = NULL;

// --- Framebuffers and DMA variables from main.c ---
extern uint8_t framebuffer_A[320 * 240];
extern uint8_t framebuffer_B[320 * 240];
extern volatile uint8_t *front_buffer;
extern volatile uint8_t *back_buffer;
extern volatile int current_line;
extern volatile bool vblank_triggered;

extern void dma_handler();
#ifdef main
#undef main
#endif

extern int main_app(void);


// --- Keyboard Input Queue ---
#define KEY_QUEUE_SIZE 256
static int key_queue[KEY_QUEUE_SIZE];
static int key_queue_head = 0;
static int key_queue_tail = 0;

static void queue_char(int c) {
    int next_tail = (key_queue_tail + 1) % KEY_QUEUE_SIZE;
    if (next_tail != key_queue_head) {
        key_queue[key_queue_tail] = c;
        key_queue_tail = next_tail;
    }
}

int getchar_timeout_us(uint32_t timeout_us) {
    (void)timeout_us;
    if (key_queue_head == key_queue_tail) {
        return PICO_ERROR_TIMEOUT;
    }
    int c = key_queue[key_queue_head];
    key_queue_head = (key_queue_head + 1) % KEY_QUEUE_SIZE;
    return c;
}

// --- Time stub ---
uint64_t time_us_64(void) {
    // Return time in microseconds using SDL performance counter
    static uint64_t start_perf = 0;
    static uint64_t perf_freq = 0;
    if (perf_freq == 0) {
        start_perf = SDL_GetPerformanceCounter();
        perf_freq = SDL_GetPerformanceFrequency();
    }
    uint64_t now = SDL_GetPerformanceCounter();
    return ((now - start_perf) * 1000000) / perf_freq;
}

// --- SDL2 State ---
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static uint32_t last_frame_time_ms = 0;

static uint32_t sdl_palette[8] = {
    0xFF000000, // 0: Black
    0xFFFF3B30, // 1: Red
    0xFF34C759, // 2: Green
    0xFFFFCC00, // 3: Yellow
    0xFF007AFF, // 4: Blue
    0xFFFF2D55, // 5: Magenta (Pinky / Mario hair)
    0xFF5AC8FA, // 6: Cyan
    0xFFFFFFFF  // 7: White
};

// Queue key characters matching the Pico input expectations in common.c.
// is_repeat: true when SDL is auto-repeating a held key.
static void handle_key_down(SDL_Keycode sym, bool is_repeat) {
    // Skip auto-repeated events. On real hardware the serial terminal never
    // auto-repeats, and repeat events cause skip to fire multiple times.
    if (is_repeat) return;
    switch (sym) {
        case SDLK_w: case SDLK_UP:
            queue_char(27); queue_char('['); queue_char('A'); // Escape sequence for Up arrow
            queue_char('w');
            break;
        case SDLK_s: case SDLK_DOWN:
            queue_char(27); queue_char('['); queue_char('B'); // Down arrow
            queue_char('s');
            break;
        case SDLK_d: case SDLK_RIGHT:
            queue_char(27); queue_char('['); queue_char('C'); // Right arrow
            queue_char('d');
            break;
        case SDLK_a: case SDLK_LEFT:
            queue_char(27); queue_char('['); queue_char('D'); // Left arrow
            queue_char('a');
            break;
        case SDLK_SPACE: case SDLK_z: case SDLK_RETURN:
            queue_char(' ');
            break;
        case SDLK_x: case SDLK_c:
            queue_char('x');
            break;
        case SDLK_q:
            queue_char('q');
            break;
        case SDLK_ESCAPE:
            // Escape = skip to next screensaver. Queue exactly one 'q' so
            // decay_skip = 1 (fires once, not 8 times).
            queue_char('q');
            break;
        default:
            break;
    }
}

// Render the 320x240 back_buffer (just drawn into by the game) to the SDL Window
static void render_framebuffer() {
    uint32_t pixels[320 * 240];
    // At the point tight_loop_contents is called, the game has just finished writing
    // to back_buffer. Rendering it directly shows the freshest frame.
    const uint8_t *fb = (const uint8_t *)back_buffer;
    
    for (int i = 0; i < 320 * 240; i++) {
        // Each byte stores two 4-bit nibbles: (color|sync) << 4 | (color|sync)
        // The color is in bits [2:0]; bit 3 is the sync bit.
        uint8_t color_idx = fb[i] & 0x07;
        pixels[i] = sdl_palette[color_idx];
    }
    
    SDL_UpdateTexture(texture, NULL, pixels, 320 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

// Process SDL2 events and frame pacing
void tight_loop_contents(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            SDL_DestroyTexture(texture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            exit(0);
        } else if (event.type == SDL_KEYDOWN) {
            handle_key_down(event.key.keysym.sym, event.key.repeat != 0);
        }
    }
    
    uint32_t now = SDL_GetTicks();
    // Pace to roughly 66.7Hz (~15ms per frame)
    if (now - last_frame_time_ms >= 15) {
        last_frame_time_ms = now;
        
        // Render the back_buffer (just filled by the game) to the SDL window.
        render_framebuffer();
        
        // Signal vblank directly.  The Pico's real dma_handler() early-exits
        // in the emulator because dma_hw->ints0 is never set by the stub DMA,
        // so we bypass it and drive the flag ourselves.  This unblocks the
        // "while (!vblank_triggered)" wait in main.c so the page-swap and
        // next game-logic tick can proceed.
        vblank_triggered = true;
    } else {
        // Sleep a tiny bit to reduce CPU usage without burning the whole core.
        SDL_Delay(1);
    }
}

// --- update_input() wrapper (--wrap linker flag) ---
//
// Problem 1: decay_skip = 8 in common.c → one 'q' keeps skip=true for 8 frames
//   → 8 state changes per keypress.  Suppressed here.
//
// Problem 2: pressing any key sets global_input.interactive = true, which
//   prevents state_frame_counter from incrementing in main.c (see the
//   "else if (!global_input.interactive)" branch).  On the Pico this is
//   intentional — it stops the 20-second auto-advance while the user is playing
//   a game.  But in the emulator it causes all frame-animated scenes (synthwave,
//   city skyline, etc.) to freeze at frame 0 after any keypress, because the
//   new scene is entered while interactive=true and frame_counter never advances.
//
//   Fix: after a skip is processed, immediately clear interactive mode and reset
//   the timeout so the new scene starts animating straight away.  Also snap back
//   to non-interactive if no direction/action keys are currently active.
extern void __real_update_input(void);
static int skip_suppress_frames = 0;

void __wrap_update_input(void) {
    __real_update_input();

    if (skip_suppress_frames > 0) {
        // Decay is still counting; force skip false so main.c ignores it.
        global_input.skip = false;
        skip_suppress_frames--;
    } else if (global_input.skip) {
        // First frame where skip is true: allow it, then suppress the rest.
        // Also reset interactive mode immediately so the *incoming* new scene
        // has its frame_counter running from frame 0.
        skip_suppress_frames = 8;   // decay_skip starts at 8 in common.c
        global_input.interactive = false;
        global_input.timeout_ticks = 0;
    }

    // If no direction or action key is actively held (all decays have expired),
    // snap back to non-interactive so screensaver scenes keep animating even
    // after the user briefly presses a key.
    if (global_input.interactive &&
        !global_input.up && !global_input.down &&
        !global_input.left && !global_input.right &&
        !global_input.action1 && !global_input.action2 &&
        !global_input.skip) {
        global_input.interactive = false;
        global_input.timeout_ticks = 0;
    }
}

// Load bad_apple.bin from disk
static void load_bad_apple() {
    FILE *f = fopen("bad_apple.bin", "rb");
    if (!f) {
        f = fopen("../bad_apple.bin", "rb");
    }
    if (!f) {
        printf("Info: bad_apple.bin not found, video state will be blank/static.\n");
        bad_apple_data = calloc(1536 * 100, 1);
        bad_apple_bin = bad_apple_data;
        bad_apple_bin_end = bad_apple_data + (1536 * 100);
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    bad_apple_data = malloc(size);
    if (!bad_apple_data) {
        printf("Error: failed to allocate memory for bad_apple.bin\n");
        fclose(f);
        exit(1);
    }
    size_t read_bytes = fread(bad_apple_data, 1, size, f);
    bad_apple_bin = bad_apple_data;
    bad_apple_bin_end = bad_apple_data + read_bytes;
    fclose(f);
    printf("Loaded bad_apple.bin successfully (%ld bytes).\n", (long)read_bytes);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    
    printf("Starting Pico VGA screensaver emulator...\n");
    load_bad_apple();
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create an upscaled 960x720 window for a crisp 3x pixel display
    window = SDL_CreateWindow("Mac VGA screensaver Emulator",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              960, 720, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, 320, 240);
    if (!texture) {
        fprintf(stderr, "Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    last_frame_time_ms = SDL_GetTicks();
    
    // Start the original screensaver application main loop
    main_app();
    
    return 0;
}
