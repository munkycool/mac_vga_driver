#include "common.h"

// --- DMA Video Core & Framebuffers ---
uint32_t scanline_buffers[2][108]; 
uint8_t framebuffer_A[320 * 240] __attribute__((aligned(4)));
uint8_t framebuffer_B[320 * 240] __attribute__((aligned(4)));

volatile uint8_t *front_buffer = framebuffer_A;
volatile uint8_t *back_buffer = framebuffer_B;

volatile int current_line = 0;
volatile bool vblank_triggered = false;
int dma_chan_a, dma_chan_b;

void dma_handler();

// RESTORED: Squeaky-clean game maps and timers setup [10]
void init_screensaver(enum ProgramState state) {
    if (state == STATE_VIDEO) {
        init_mario_movie();
    } else if (state == STATE_BAD_APPLE) {
        init_bad_apple();
    } else if (state == STATE_BOUNCING_BOX) {
        init_bouncing_box();
    } else if (state == STATE_PONG) {
        init_pong();
    } else if (state == STATE_TETRIS) {
        init_tetris();
    } else if (state == STATE_PACMAN) {
        init_pacman();
    } else if (state == STATE_INVADERS) {
        init_invaders();
    } else if (state == STATE_DONKEY_KONG) {
        init_donkey_kong();
    } else if (state == STATE_MARIO_NES) {
        init_mario_nes();
    } else if (state == STATE_METROID) {
        init_metroid();
    } else if (state == STATE_SYNTHWAVE) {
        init_synthwave();
    } else if (state == STATE_CITY_SKYLINE) {
        init_city_skyline();
    } else if (state == STATE_FIREPLACE) {
        init_fireplace();
    } else if (state == STATE_CHERRY_BLOSSOM) {
        init_cherry_blossom();
    } else if (state == STATE_CYBERPUNK) {
        init_cyberpunk();
    } else if (state == STATE_NEBULA) {
        init_nebula();
    } else if (state == STATE_QUOTES) {
        init_quotes();
    } else if (state == STATE_FROGGER) {
        init_frogger();
    } else if (state == STATE_SNAKE) {
        init_snake();
    } else if (state == STATE_FLAPPY_BIRD) {
        init_flappy_bird();
    } else if (state == STATE_BREAKOUT) {
        init_breakout();
    } else if (state == STATE_ASTEROIDS) {
        init_asteroids();
    } else if (state == STATE_TRON) {
        init_tron();
    } else if (state == STATE_SANIC) {
        init_sanic();
    } else if (state == STATE_UNDERTALE) {
        init_undertale();
    } else if (state == STATE_DELTARUNE) {
        init_deltarune();
    } else if (state == STATE_NASA) {
        init_nasa();
    } else if (state == STATE_MARIO_SHOW) {
        init_mario_show();
    } else if (state == STATE_CHALLENGER) {
        init_challenger();
    } else if (state == STATE_NINE_ELEVEN) {
        init_nine_eleven();
    } else if (state == STATE_PINUP) {
        init_pinup();
    } else if (state == STATE_PRIDE) {
        init_pride();
    } else if (state == STATE_SPLASH) {
        init_splash();
    }
}

// DMA Interrupt Handler
void dma_handler() {
    int target_buffer = -1;
    if (dma_hw->ints0 & (1u << dma_chan_a)) {
        dma_hw->ints0 = 1u << dma_chan_a;
        dma_channel_set_read_addr(dma_chan_a, scanline_buffers[0], false);
        target_buffer = 0;
    }
    if (dma_hw->ints0 & (1u << dma_chan_b)) {
        dma_hw->ints0 = 1u << dma_chan_b;
        dma_channel_set_read_addr(dma_chan_b, scanline_buffers[1], false);
        target_buffer = 1;
    }
    if (target_buffer == -1) return;

    current_line++;
    if (current_line >= V_TOTAL) current_line = 0;
    
    if (current_line == V_ACTIVE) {
        vblank_triggered = true;
    }
    
    bool v_sync_active = (current_line >= V_ACTIVE + V_FRONT) && (current_line < V_ACTIVE + V_FRONT + V_SYNC);
    bool is_active_v = (current_line < V_ACTIVE);
    
    uint8_t *buf = (uint8_t *)scanline_buffers[target_buffer];
    
    uint8_t sync_high = 0x08; 
    uint8_t sync_low  = 0x00;
    
    uint8_t active_sync = v_sync_active ? sync_low  : sync_high;
    uint8_t fp_sync     = v_sync_active ? sync_low  : sync_high;
    uint8_t pulse_sync  = v_sync_active ? sync_high : sync_low;
    uint8_t bp_sync     = v_sync_active ? sync_low  : sync_high;

    if (is_active_v) {
        uint8_t *fb_row = (uint8_t *)&front_buffer[(current_line / 2) * 320];
        memcpy(buf, fb_row, 320);
        buf += 320;
    } else {
        uint8_t val = (active_sync << 4) | active_sync;
        memset(buf, val, 320);
        buf += 320;
    }
    
    uint8_t val_fp = (fp_sync << 4) | fp_sync;
    memset(buf, val_fp, 32);
    buf += 32;
    
    uint8_t val_pulse = (pulse_sync << 4) | pulse_sync;
    memset(buf, val_pulse, 32);
    buf += 32;
    
    uint8_t val_bp = (bp_sync << 4) | bp_sync;
    memset(buf, val_bp, 48);
}

// Master execution block [10]
int main() {
    set_sys_clock_khz(151200, true);
    seed_rng();
    
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &vga_program);
    
    pio_sm_config c = vga_program_get_default_config(offset);
    sm_config_set_out_pins(&c, PIN_BASE, 4);
    
    for(int i = 0; i < 4; i++) {
        pio_gpio_init(pio, PIN_BASE + i);
    }
    pio_sm_set_consecutive_pindirs(pio, sm, PIN_BASE, 4, true);
    
    sm_config_set_out_shift(&c, true, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, 5.0f);
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
    
    dma_chan_a = dma_claim_unused_channel(true);
    dma_chan_b = dma_claim_unused_channel(true);
    
    dma_channel_config c_a = dma_channel_get_default_config(dma_chan_a);
    channel_config_set_transfer_data_size(&c_a, DMA_SIZE_32);
    channel_config_set_read_increment(&c_a, true);
    channel_config_set_write_increment(&c_a, false);
    channel_config_set_dreq(&c_a, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&c_a, dma_chan_b);
    
    dma_channel_config c_b = dma_channel_get_default_config(dma_chan_b);
    channel_config_set_transfer_data_size(&c_b, DMA_SIZE_32);
    channel_config_set_read_increment(&c_b, true);
    channel_config_set_write_increment(&c_b, false);
    channel_config_set_dreq(&c_b, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&c_b, dma_chan_a);
    
    dma_channel_configure(dma_chan_a, &c_a, &pio->txf[sm], scanline_buffers[0], 108, false);
    dma_channel_configure(dma_chan_b, &c_b, &pio->txf[sm], scanline_buffers[1], 108, false);
    
    dma_channel_set_irq0_enabled(dma_chan_a, true);
    dma_channel_set_irq0_enabled(dma_chan_b, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    
    irq_set_priority(DMA_IRQ_0, 0); 
    irq_set_enabled(DMA_IRQ_0, true);
    
    memset(scanline_buffers[0], 0x88, 108 * 4); 
    memset(scanline_buffers[1], 0x88, 108 * 4);
    
    clear_screen((uint8_t *)framebuffer_A);
    clear_screen((uint8_t *)framebuffer_B);
    
    dma_channel_start(dma_chan_a);
    
    // Randomly show splash screen on boot (~1 in SPLASH_PROB chance)
    enum ProgramState current_state;
    if ((get_rand() % SPLASH_PROB) == 0) {
        current_state = STATE_SPLASH;
        init_screensaver(current_state);
    } else {
        current_state = pick_next_state((enum ProgramState)-1);
        init_screensaver(current_state);
    }
    
    int state_frame_counter = 0;
    const int TIMEOUT_FRAMES = 1334; // 1334 frames = Exactly 20.0 seconds at 66.7Hz [10]
    
    while(1) {
        update_input();
        // --- 1. RENDER PHASE ---
        if (current_state == STATE_VIDEO || current_state == STATE_BAD_APPLE) {
            play_video((uint8_t *)back_buffer);
            
        } else if (current_state == STATE_BOUNCING_BOX) {
            play_dvd((uint8_t *)back_buffer);
            
        } else if (current_state == STATE_PONG) {
            play_pong((uint8_t *)back_buffer);
            
        } else if (current_state == STATE_TETRIS) {
            play_tetris((uint8_t *)back_buffer);
            
        } else if (current_state == STATE_PACMAN) {
            play_pacman((uint8_t *)back_buffer, state_frame_counter);
            
        } else if (current_state == STATE_INVADERS) {
            play_invaders((uint8_t *)back_buffer, state_frame_counter);
            
        } else if (current_state == STATE_DONKEY_KONG) {
            play_donkey_kong((uint8_t *)back_buffer);
            
        } else if (current_state == STATE_MARIO_NES) {
            play_mario_nes((uint8_t *)back_buffer, state_frame_counter);
            
        } else if (current_state == STATE_METROID) {
            play_metroid((uint8_t *)back_buffer, state_frame_counter);
            
        } else if (current_state == STATE_SYNTHWAVE) {
            play_synthwave((uint8_t *)back_buffer, state_frame_counter);
            
        } else if (current_state == STATE_CITY_SKYLINE) {
            play_city_skyline((uint8_t *)back_buffer, state_frame_counter);
            
        } else if (current_state == STATE_FIREPLACE) {
            play_fireplace((uint8_t *)back_buffer, state_frame_counter);
            
        } else if (current_state == STATE_CHERRY_BLOSSOM) {
            play_cherry_blossom((uint8_t *)back_buffer, state_frame_counter);
            
        } else if (current_state == STATE_CYBERPUNK) {
            play_cyberpunk((uint8_t *)back_buffer, state_frame_counter);
            
        } else if (current_state == STATE_NEBULA) {
            play_nebula((uint8_t *)back_buffer, state_frame_counter);
            
        } else if (current_state == STATE_QUOTES) {
            play_quotes((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_FROGGER) {
            play_frogger((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_SNAKE) {
            play_snake((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_FLAPPY_BIRD) {
            play_flappy_bird((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_BREAKOUT) {
            play_breakout((uint8_t *)back_buffer);
        } else if (current_state == STATE_ASTEROIDS) {
            play_asteroids((uint8_t *)back_buffer);
        } else if (current_state == STATE_TRON) {
            play_tron((uint8_t *)back_buffer);
        } else if (current_state == STATE_SANIC) {
            play_sanic((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_UNDERTALE) {
            play_undertale((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_DELTARUNE) {
            play_deltarune((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_NASA) {
            play_nasa((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_MARIO_SHOW) {
            play_mario_show((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_CHALLENGER) {
            play_challenger((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_NINE_ELEVEN) {
            play_nine_eleven((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_PINUP) {
            play_pinup((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_PRIDE) {
            play_pride((uint8_t *)back_buffer, state_frame_counter);
        } else if (current_state == STATE_SPLASH) {
            play_splash((uint8_t *)back_buffer, state_frame_counter);
        }
        
        // --- 2. VSYNC WAIT ---
        while (!vblank_triggered) {
            tight_loop_contents();
        }
        vblank_triggered = false;
        
        // --- 3. PAGE SWAP ---
        volatile uint8_t *temp = front_buffer;
        front_buffer = back_buffer;
        back_buffer = temp;
        
        // --- 4. STEP STATE PHYSICS & SCHEDULER ---
        if (current_state == STATE_VIDEO || current_state == STATE_BAD_APPLE) {
            tick_video();
        } else if (current_state == STATE_BOUNCING_BOX) {
            tick_bouncing_box();
        }
        
        // Master timer to swap state after 20 seconds [10]
        // Splash screen: always times out after ~3 seconds regardless of input mode
        if (current_state == STATE_SPLASH) {
            state_frame_counter++;
            if (state_frame_counter >= 200) { // ~3 seconds at 67Hz
                current_state = pick_next_state((enum ProgramState)-1);
                init_screensaver(current_state);
                state_frame_counter = 0;
            }
        } else if (global_input.interactive && global_input.skip) {
            current_state = pick_next_state(current_state);
            init_screensaver(current_state);
            state_frame_counter = 0;
            global_input.skip = false;
        } else if (!global_input.interactive) {
            state_frame_counter++;
            if (state_frame_counter >= TIMEOUT_FRAMES) {
                // Pick a completely random next state (No consecutive duplicates) [10]
                current_state = pick_next_state(current_state);
                init_screensaver(current_state);
                state_frame_counter = 0;
            }
        }
    }
}
