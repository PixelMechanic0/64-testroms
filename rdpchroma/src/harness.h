#ifndef RDPCHROMA_HARNESS_H
#define RDPCHROMA_HARNESS_H

#include <stdint.h>
#include "rdp.h"

typedef struct {
    void     *pixels;   /* uncached RDRAM buffer */
    uint32_t  width;
    uint32_t  height;
    int       size;     /* SIZ_16 or SIZ_32 */
} rdp_target;

typedef struct {
    uint32_t status;
    uint32_t current;
    uint32_t end;
    int      consumed;
    int      timed_out;
} dpl_state;

void harness_init(void);

void dpl_reset(void);
void dpl_cmd(uint64_t cmd);
void dpl_run(void);
dpl_state dpl_last_state(void);

rdp_target target_create(uint32_t width, uint32_t height, int size);
void target_bind(const rdp_target *t);
void target_clear(const rdp_target *t, uint32_t value);
void target_draw_rect(const rdp_target *t);
void target_draw_texrect(const rdp_target *t, uint32_t tile, int32_t dsdx, int32_t dtdy);

uint32_t target_pixel_raw(const rdp_target *t, uint32_t x, uint32_t y);
uint32_t target_pixel_rgba(const rdp_target *t, uint32_t x, uint32_t y);
uint32_t target_center_rgba(const rdp_target *t);

/* Logging & Diagnostics */
int  report_init_sd(const char *path);
void report_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void report_close(void);

#endif /* RDPCHROMA_HARNESS_H */
