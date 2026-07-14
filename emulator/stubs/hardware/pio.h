#ifndef HARDWARE_PIO_H
#define HARDWARE_PIO_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t txf[4];
} pio_hw_t;

typedef pio_hw_t* PIO;

#define pio0 ((PIO)0x1000)
#define pio1 ((PIO)0x2000)

typedef struct {
    int dummy;
} pio_sm_config;

typedef struct {
    int dummy;
} pio_program_t;

#define PIO_FIFO_JOIN_TX 1

static inline uint pio_add_program(PIO pio, const pio_program_t *program) { (void)pio; (void)program; return 0; }
static inline pio_sm_config pio_sm_config_get_default(uint offset) { (void)offset; pio_sm_config c; return c; }
static inline void sm_config_set_out_pins(pio_sm_config *c, uint pin_base, uint pin_count) { (void)c; (void)pin_base; (void)pin_count; }
static inline void pio_gpio_init(PIO pio, uint pin) { (void)pio; (void)pin; }
static inline void pio_sm_set_consecutive_pindirs(PIO pio, uint sm, uint pin_base, uint pin_count, bool is_out) { (void)pio; (void)sm; (void)pin_base; (void)pin_count; (void)is_out; }
static inline void sm_config_set_out_shift(pio_sm_config *c, bool shift_right, bool autopull, uint pull_threshold) { (void)c; (void)shift_right; (void)autopull; (void)pull_threshold; }
static inline void sm_config_set_fifo_join(pio_sm_config *c, uint join) { (void)c; (void)join; }
static inline void sm_config_set_clkdiv(pio_sm_config *c, float div) { (void)c; (void)div; }
static inline void pio_sm_init(PIO pio, uint sm, uint offset, const pio_sm_config *c) { (void)pio; (void)sm; (void)offset; (void)c; }
static inline void pio_sm_set_enabled(PIO pio, uint sm, bool enabled) { (void)pio; (void)sm; (void)enabled; }
static inline uint pio_get_dreq(PIO pio, uint sm, bool is_tx) { (void)pio; (void)sm; (void)is_tx; return 0; }

#endif // HARDWARE_PIO_H
