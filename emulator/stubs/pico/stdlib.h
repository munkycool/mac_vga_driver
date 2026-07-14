#ifndef PICO_STDLIB_H
#define PICO_STDLIB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PICO_ERROR_TIMEOUT -1

typedef unsigned int uint;

static inline void set_sys_clock_khz(uint32_t khz, bool required) { (void)khz; (void)required; }
void tight_loop_contents(void);
uint64_t time_us_64(void);
int getchar_timeout_us(uint32_t timeout_us);
static inline void sleep_ms(uint32_t ms) { (void)ms; }

#endif // PICO_STDLIB_H
