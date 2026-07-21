#ifndef TEST_H
#define TEST_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

static int checks_run = 0;
static int checks_failed = 0;
static const char *current_test = "";

#define TEST(name) static void name(void)
#define RUN(name) do { current_test = #name; name(); } while (0)
#define CHECK(cond) do { \
    checks_run++; \
    if (!(cond)) { \
        checks_failed++; \
        fprintf(stderr, "FAIL %s:%d [%s] %s\n", __FILE__, __LINE__, current_test, #cond); \
    } \
} while (0)

/* Every draw_pixel() write is (c<<4)|c with c=(color&7)|8 -- true for any
 * byte a scene ever legitimately writes to a framebuffer. */
static inline bool buffer_is_well_formed(const uint8_t *buf, int n) {
    for (int i = 0; i < n; i++) {
        uint8_t b = buf[i];
        if ((b >> 4) != (b & 0x0F)) return false;
        if (!(b & 0x08)) return false;
    }
    return true;
}

/* Host-side fake clock, controlled by tests, implemented in test_stubs.c. */
void test_set_fake_time_us(uint64_t t);
void test_advance_fake_time_us(uint64_t dt);

#define TEST_SUMMARY() \
    (fprintf(stderr, "%d/%d checks failed\n", checks_failed, checks_run), checks_failed ? 1 : 0)

#endif
