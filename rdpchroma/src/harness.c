#include <libdragon.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "rdp.h"

#define DPL_CAPACITY 128
#define DPL_TIMEOUT_MS 250

static uint64_t *dpl;
static uint32_t  dpl_count;
static dpl_state dpl_state_last;
static FILE     *sd_log;

void harness_init(void)
{
    dpl = (uint64_t *)malloc_uncached(sizeof(uint64_t) * DPL_CAPACITY);
    dpl_count = 0;

    MEMORY_BARRIER();
    *DP_STATUS = DP_WSTATUS_RESET_XBUS_DMEM_DMA |
                 DP_WSTATUS_RESET_FREEZE |
                 DP_WSTATUS_RESET_FLUSH;
    MEMORY_BARRIER();
}

int report_init_sd(const char *path)
{
    if (!debug_init_sdfs("sd:/", -1)) return 0;
    sd_log = fopen(path, "w");
    return sd_log != NULL;
}

void report_printf(const char *fmt, ...)
{
    char line[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    debugf("%s", line);
    if (sd_log) {
        fputs(line, sd_log);
        fflush(sd_log);
    }
}

void report_close(void)
{
    if (sd_log) {
        fflush(sd_log);
        fclose(sd_log);
        sd_log = NULL;
    }
}

void dpl_reset(void)
{
    dpl_count = 0;
}

void dpl_cmd(uint64_t cmd)
{
    assertf(dpl_count < DPL_CAPACITY, "display list overflow (%lu)",
            (unsigned long)dpl_count);
    dpl[dpl_count++] = cmd;
}

#define WAIT_UNTIL(cond, deadline)                                   \
    ({                                                               \
        int _ok = 1;                                                 \
        while (cond) {                                               \
            if (get_ticks() > (deadline)) { _ok = 0; break; }        \
        }                                                            \
        _ok;                                                         \
    })

void dpl_run(void)
{
    dpl_cmd(RDP_SYNC_FULL);

    const uint32_t start = PhysicalAddr(dpl);
    const uint32_t end   = PhysicalAddr(dpl + dpl_count);
    const uint64_t deadline = get_ticks() + TICKS_FROM_MS(DPL_TIMEOUT_MS);

    int ok = 1;

    ok &= WAIT_UNTIL(*DP_STATUS & (DP_STATUS_DMA_BUSY | DP_STATUS_START_VALID |
                                   DP_STATUS_END_VALID), deadline);

    MEMORY_BARRIER();
    *DP_START = start;
    MEMORY_BARRIER();
    *DP_END   = end;
    MEMORY_BARRIER();

    ok &= WAIT_UNTIL(*DP_STATUS & (DP_STATUS_START_VALID | DP_STATUS_END_VALID),
                     deadline);
    ok &= WAIT_UNTIL(*DP_CURRENT != end, deadline);
    ok &= WAIT_UNTIL(*DP_STATUS & (DP_STATUS_DMA_BUSY | DP_STATUS_PIPE_BUSY |
                                   DP_STATUS_BUSY), deadline);

    dpl_state_last.status    = *DP_STATUS;
    dpl_state_last.current   = *DP_CURRENT;
    dpl_state_last.end       = end;
    dpl_state_last.consumed  = (*DP_CURRENT == end);
    dpl_state_last.timed_out = !ok;

    dpl_count = 0;
}

dpl_state dpl_last_state(void)
{
    return dpl_state_last;
}

rdp_target target_create(uint32_t width, uint32_t height, int size)
{
    rdp_target t;
    const uint32_t bpp = (size == SIZ_32) ? 4u : 2u;
    t.width  = width;
    t.height = height;
    t.size   = size;
    t.pixels = malloc_uncached_aligned(64, width * height * bpp);
    return t;
}

void target_bind(const rdp_target *t)
{
    dpl_cmd(RDP_SYNC_PIPE);
    dpl_cmd(rdp_set_color_image(RDP_FMT_RGBA, t->size, t->width,
                                PhysicalAddr(t->pixels)));
    dpl_cmd(rdp_set_scissor(0, 0, t->width << 2, t->height << 2));
}

void target_clear(const rdp_target *t, uint32_t value)
{
    const uint32_t fill = (t->size == SIZ_32)
        ? value
        : ((value & 0xffff) | ((value & 0xffff) << 16));

    dpl_reset();
    target_bind(t);
    dpl_cmd(RDP_OM_BASE | RDP_OM_CYCLE(RDP_CYCLE_FILL));
    dpl_cmd(RDP_SET_FILL_COLOR(fill));
    target_draw_rect(t);
    dpl_run();
}

void target_draw_rect(const rdp_target *t)
{
    dpl_cmd(rdp_fill_rectangle(t->width << 2, t->height << 2, 0, 0));
}

void target_draw_texrect(const rdp_target *t, uint32_t tile, int32_t dsdx, int32_t dtdy)
{
    dpl_cmd(rdp_tex_rect_w0(tile, t->width << 2, t->height << 2, 0, 0));
    dpl_cmd(rdp_tex_rect_w1(0, 0, dsdx, dtdy));
}

uint32_t target_pixel_raw(const rdp_target *t, uint32_t x, uint32_t y)
{
    if (t->size == SIZ_32)
        return ((const uint32_t *)t->pixels)[y * t->width + x];
    return ((const uint16_t *)t->pixels)[y * t->width + x];
}

uint32_t target_pixel_rgba(const rdp_target *t, uint32_t x, uint32_t y)
{
    const uint32_t v = target_pixel_raw(t, x, y);
    if (t->size == SIZ_32) return v;

    const uint32_t r5 = (v >> 11) & 0x1f;
    const uint32_t g5 = (v >>  6) & 0x1f;
    const uint32_t b5 = (v >>  1) & 0x1f;
    const uint32_t a1 = v & 1;
    const uint32_t r = (r5 << 3) | (r5 >> 2);
    const uint32_t g = (g5 << 3) | (g5 >> 2);
    const uint32_t b = (b5 << 3) | (b5 >> 2);

    return (r << 24) | (g << 16) | (b << 8) | (a1 ? 0xff : 0);
}

uint32_t target_center_rgba(const rdp_target *t)
{
    return target_pixel_rgba(t, t->width / 2, t->height / 2);
}
