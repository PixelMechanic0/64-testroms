#include <libdragon.h>
#include <string.h>

#include "probe.h"

#define TARGET_W 256
#define TARGET_H 4
#define PROBE_MARKER 0x0000ffff

static rdp_target fb32, fb16;
static int        fb_ready;

rdp_target *probe_target(void)
{
    if (!fb_ready) {
        fb32 = target_create(TARGET_W, TARGET_H, SIZ_32);
        fb16 = target_create(TARGET_W, TARGET_H, SIZ_16);
        fb_ready = 1;
    }
    return &fb32;
}

rdp_target *probe_target_16(void)
{
    if (!fb_ready) {
        fb32 = target_create(TARGET_W, TARGET_H, SIZ_32);
        fb16 = target_create(TARGET_W, TARGET_H, SIZ_16);
        fb_ready = 1;
    }
    return &fb16;
}

void probe_set_uniform(key_probe *p, uint32_t width)
{
    for (int i = 0; i < 3; i++) {
        p->width[i]  = width;
        p->center[i] = 0;
        p->scale[i]  = 0;
    }
}

void probe_emit_key(const key_probe *p)
{
    dpl_cmd(rdp_set_key_gb(p->width[1], (uint8_t)p->center[1], (uint8_t)p->scale[1],
                           p->width[2], (uint8_t)p->center[2], (uint8_t)p->scale[2]));
    dpl_cmd(rdp_set_key_r(p->width[0], (uint8_t)p->center[0], (uint8_t)p->scale[0]));
}

void probe_emit_key_uniform(uint32_t width)
{
    dpl_cmd(rdp_set_key_gb(width, 0, 0, width, 0, 0));
    dpl_cmd(rdp_set_key_r(width, 0, 0));
}

static void probe_emit_common(const key_probe *p, uint64_t extra_modes)
{
    target_bind(probe_target());
    dpl_cmd(p->other_modes | extra_modes);
    dpl_cmd(p->combine);
    dpl_cmd(RDP_SET_PRIM_COLOR(p->prim));
    dpl_cmd(RDP_SET_ENV_COLOR(p->env));
    probe_emit_key(p);
}

int probe_draws(const key_probe *p, uint32_t threshold)
{
    rdp_target *t = probe_target();

    target_clear(t, PROBE_MARKER);
    probe_emit_common(p, RDP_OM_ALPHA_COMPARE);
    dpl_cmd(RDP_SET_BLEND_COLOR(threshold & 0xff));
    target_draw_rect(t);
    dpl_run();

    return target_center_rgba(t) != PROBE_MARKER;
}

int32_t probe_keyalpha(const key_probe *p)
{
    uint32_t lo = 0, hi = 255;

    if (!probe_draws(p, 0)) return -1;

    while (lo < hi) {
        const uint32_t mid = (lo + hi + 1) / 2;
        if (probe_draws(p, mid)) lo = mid;
        else                     hi = mid - 1;
    }
    return (int32_t)lo;
}

uint32_t probe_rgb(const key_probe *p)
{
    rdp_target *t = probe_target();

    target_clear(t, PROBE_MARKER);
    probe_emit_common(p, 0);
    target_draw_rect(t);
    dpl_run();

    return target_center_rgba(t);
}

/* ------------------------------------------------------------------ TMEM */

static uint8_t *tex_stage;
static uint8_t  ramp[256];
static int      ramp_ready;

const uint8_t *tex_ramp_i8(void)
{
    if (!ramp_ready) {
        for (int i = 0; i < 256; i++) ramp[i] = (uint8_t)i;
        ramp_ready = 1;
    }
    return ramp;
}

static uint8_t *stage(const void *src, uint32_t bytes)
{
    if (!tex_stage) tex_stage = (uint8_t *)malloc_uncached_aligned(64, 4096);
    memcpy(tex_stage, src, bytes);
    return tex_stage;
}

static void upload(const void *texels, uint32_t width, int fmt, int siz,
                   uint32_t bytes_per_texel)
{
    const uint32_t bytes = width * bytes_per_texel;
    uint8_t *buf = stage(texels, bytes);
    const uint32_t line = (bytes + 7) / 8;

    dpl_reset();
    dpl_cmd(RDP_SYNC_PIPE);
    dpl_cmd(rdp_set_texture_image(fmt, siz, width, PhysicalAddr(buf)));
    dpl_cmd(RDP_SYNC_LOAD);
    dpl_cmd(rdp_set_tile(fmt, siz, line, 0, 7, 0, 0));
    dpl_cmd(rdp_load_tile(7, 0, 0, (width - 1) << 2, 0));
    dpl_cmd(RDP_SYNC_TILE);
    dpl_cmd(rdp_set_tile(fmt, siz, line, 0, 0, 0, 0));
    dpl_cmd(rdp_set_tile_size(0, 0, 0, (width - 1) << 2, 0));
    dpl_cmd(RDP_SYNC_TILE);
    dpl_run();
}

void tex_upload_i8(const uint8_t *texels, uint32_t width)
{
    upload(texels, width, FMT_I, SIZ_8, 1);
}

void tex_upload_rgba16(const uint16_t *texels, uint32_t width)
{
    upload(texels, width, FMT_RGBA, SIZ_16, 2);
}

void tex_upload_yuv16(const uint16_t *texels, uint32_t width)
{
    upload(texels, width, FMT_YUV, SIZ_16, 2);
}
