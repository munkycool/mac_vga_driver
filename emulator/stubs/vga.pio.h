#ifndef VGA_PIO_H
#define VGA_PIO_H

#include "hardware/pio.h"

extern const pio_program_t vga_program;

static inline pio_sm_config vga_program_get_default_config(uint offset) {
    (void)offset;
    pio_sm_config c;
    return c;
}

#endif // VGA_PIO_H
