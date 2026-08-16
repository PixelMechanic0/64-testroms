#ifndef RDPCHROMA_PROBE_H
#define RDPCHROMA_PROBE_H

#include <stdint.h>
#include "harness.h"
#include "rdp.h"

#define OM_ALPHA_READOUT                                              \
    (RDP_OM_BASE | RDP_OM_NO_DITHER | RDP_OM_FORCE_BLEND |            \
     RDP_OM_IMAGE_READ | RDP_BLEND_ALPHA_READOUT)

#define OM_RGB_READOUT (RDP_OM_BASE | RDP_OM_NO_DITHER)

#define OM_TEX_RGB \
    (RDP_OM_BASE | RDP_OM_NO_DITHER | RDP_OM_CYCLE(RDP_CYCLE_1) | RDP_OM_BILERP0 | RDP_OM_BILERP1)

#define OM_TEX_CONVERT \
    (RDP_OM_BASE | RDP_OM_NO_DITHER | RDP_OM_CYCLE(RDP_CYCLE_1))

#define DSDX_1TO1   (1 << 10)
#define DSDX_COPY   (4 << 10)

typedef struct {
    uint64_t other_modes;
    uint64_t combine;
    uint32_t prim;
    uint32_t env;
    uint32_t width[3];      /* r, g, b */
    uint32_t center[3];
    uint32_t scale[3];
} key_probe;

rdp_target *probe_target(void);
rdp_target *probe_target_16(void);

void probe_set_uniform(key_probe *p, uint32_t width);
void probe_emit_key(const key_probe *p);
void probe_emit_key_uniform(uint32_t width);

int     probe_draws(const key_probe *p, uint32_t threshold);
int32_t probe_keyalpha(const key_probe *p);
uint32_t probe_rgb(const key_probe *p);

static inline uint32_t red_of(uint32_t px)   { return (px >> 24) & 0xff; }
static inline uint32_t green_of(uint32_t px) { return (px >> 16) & 0xff; }
static inline uint32_t blue_of(uint32_t px)  { return (px >>  8) & 0xff; }

/* TMEM Uploads */
const uint8_t *tex_ramp_i8(void);
void tex_upload_i8(const uint8_t *texels, uint32_t width);
void tex_upload_rgba16(const uint16_t *texels, uint32_t width);
void tex_upload_yuv16(const uint16_t *texels, uint32_t width);

#endif /* RDPCHROMA_PROBE_H */
