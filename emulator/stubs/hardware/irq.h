#ifndef HARDWARE_IRQ_H
#define HARDWARE_IRQ_H

#include <stdint.h>
#include <stdbool.h>

#define DMA_IRQ_0 0

typedef void (*irq_handler_t)(void);

static inline void irq_set_exclusive_handler(uint num, irq_handler_t handler) { (void)num; (void)handler; }
static inline void irq_set_priority(uint num, uint8_t priority) { (void)num; (void)priority; }
static inline void irq_set_enabled(uint num, bool enabled) { (void)num; (void)enabled; }

#endif // HARDWARE_IRQ_H
