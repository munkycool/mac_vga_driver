#ifndef HARDWARE_DMA_H
#define HARDWARE_DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    volatile uint32_t ints0;
} dma_hw_t;

extern dma_hw_t dma_hw_inst;
#define dma_hw (&dma_hw_inst)

typedef struct {
    int dummy;
} dma_channel_config;

#define DMA_SIZE_32 2

static inline int dma_claim_unused_channel(bool required) { (void)required; static int next_chan = 0; return next_chan++; }
static inline dma_channel_config dma_channel_get_default_config(uint channel) { (void)channel; dma_channel_config c; return c; }
static inline void channel_config_set_transfer_data_size(dma_channel_config *c, uint size) { (void)c; (void)size; }
static inline void channel_config_set_read_increment(dma_channel_config *c, bool incr) { (void)c; (void)incr; }
static inline void channel_config_set_write_increment(dma_channel_config *c, bool incr) { (void)c; (void)incr; }
static inline void channel_config_set_dreq(dma_channel_config *c, uint dreq) { (void)c; (void)dreq; }
static inline void channel_config_set_chain_to(dma_channel_config *c, uint chain_to) { (void)c; (void)chain_to; }
static inline void dma_channel_configure(uint channel, const dma_channel_config *c, void *write_addr, const void *read_addr, uint transfer_count, bool trigger) {
    (void)channel; (void)c; (void)write_addr; (void)read_addr; (void)transfer_count; (void)trigger;
}
static inline void dma_channel_set_irq0_enabled(uint channel, bool enabled) { (void)channel; (void)enabled; }
static inline void dma_channel_start(uint channel) { (void)channel; }
static inline void dma_channel_set_read_addr(uint channel, const void *read_addr, bool trigger) { (void)channel; (void)read_addr; (void)trigger; }

#endif // HARDWARE_DMA_H
