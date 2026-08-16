/*
 * Raw RDP command encoding for Chroma Key Test ROM 3.
 *
 * Bit positions verified against silicon and reference implementations:
 *   SET_OTHER_MODES   src/core/n64video/rdp.c:624
 *   SET_COMBINE       src/core/n64video/rdp/combiner.c:522
 *   blender muxes     src/core/n64video/rdp/blender.c:7
 *   SET_KEY_GB/R      src/core/n64video/rdp/combiner.c:565
 *   FILL_RECTANGLE    src/core/n64video/rdp/rasterizer.c:2743
 *   TEXTURE_RECTANGLE src/core/n64video/rdp/rasterizer.c:2634
 *   SET_TILE          src/core/n64video/rdp/tex.c:979
 *   SET_TEXTURE_IMAGE src/core/n64video/rdp/tex.c:1000
 *   LOAD_TILE         src/core/n64video/rdp/tex.c:939
 *   SET_TILE_SIZE     src/core/n64video/rdp/tex.c:939
 *   SET_CONVERT       src/core/n64video/rdp/tex.c:1011
 *   SET_SCISSOR       src/core/n64video/rdp/rasterizer.c:2777
 */
#ifndef RDPCHROMA_RDP_H
#define RDPCHROMA_RDP_H

#include <stdint.h>

#define RDP_CMD(op) ((uint64_t)(op) << 56)

#define RDP_SYNC_FULL RDP_CMD(0x29)
#define RDP_SYNC_PIPE RDP_CMD(0x27)
#define RDP_SYNC_LOAD RDP_CMD(0x31)
#define RDP_SYNC_TILE RDP_CMD(0x28)

/* Cycle types */
#define RDP_CYCLE_1     0
#define RDP_CYCLE_2     1
#define RDP_CYCLE_COPY  2
#define RDP_CYCLE_FILL  3

/* Formats & Sizes */
#define FMT_RGBA 0
#define FMT_YUV  1
#define FMT_CI   2
#define FMT_IA   3
#define FMT_I    4

#define SIZ_4    0
#define SIZ_8    1
#define SIZ_16   2
#define SIZ_32   3

#define RDP_FMT_RGBA 0
#define RDP_SIZE_16  2
#define RDP_SIZE_32  3

/* SET_OTHER_MODES */
#define RDP_OM_BASE            RDP_CMD(0x2f)
#define RDP_OM_CYCLE(x)        ((uint64_t)(x) << 52)  /* w0 21:20 */
#define RDP_OM_TLUT_EN         (1ULL << 47)
#define RDP_OM_SAMPLE_QUAD     (1ULL << 45)           /* w0 bit 13 */
#define RDP_OM_BILERP0         (1ULL << 43)           /* w0 bit 11 */
#define RDP_OM_BILERP1         (1ULL << 42)           /* w0 bit 10 */
#define RDP_OM_CONVERT_ONE     (1ULL << 41)           /* w0 bit 9  */
#define RDP_OM_KEY_EN          (1ULL << 40)           /* w0 bit 8  */
#define RDP_OM_RGB_DITHER(x)   ((uint64_t)(x) << 38)  /* w0 7:6    */
#define RDP_OM_ALPHA_DITHER(x) ((uint64_t)(x) << 36)  /* w0 5:4    */
#define RDP_OM_FORCE_BLEND     (1ULL << 14)
#define RDP_OM_ALPHA_CVG_SEL   (1ULL << 13)
#define RDP_OM_CVG_X_ALPHA     (1ULL << 12)
#define RDP_OM_ZMODE(x)        ((uint64_t)(x) << 10)
#define RDP_OM_CVG_DEST(x)     ((uint64_t)(x) << 8)
#define RDP_OM_COLOR_ON_CVG    (1ULL << 7)
#define RDP_OM_IMAGE_READ      (1ULL << 6)
#define RDP_OM_Z_UPDATE        (1ULL << 5)
#define RDP_OM_Z_COMPARE       (1ULL << 4)
#define RDP_OM_AA              (1ULL << 3)
#define RDP_OM_DITHER_ALPHA    (1ULL << 1)
#define RDP_OM_ALPHA_COMPARE   (1ULL << 0)

#define RDP_OM_NO_DITHER (RDP_OM_RGB_DITHER(3) | RDP_OM_ALPHA_DITHER(3))

/* Blender Mux */
#define RDP_BLEND(a0, b0, c0, d0, a1, b1, c1, d1)                     \
    (((uint64_t)(a0) << 30) | ((uint64_t)(a1) << 28) |                \
     ((uint64_t)(b0) << 26) | ((uint64_t)(b1) << 24) |                \
     ((uint64_t)(c0) << 22) | ((uint64_t)(c1) << 20) |                \
     ((uint64_t)(d0) << 18) | ((uint64_t)(d1) << 16))

#define RDP_BLEND_ALPHA_READOUT RDP_BLEND(0, 0, 1, 0, 0, 0, 1, 0)

/* Combiner Selectors */
#define CC_A_COMBINED 0
#define CC_A_TEXEL0   1
#define CC_A_TEXEL1   2
#define CC_A_PRIM     3
#define CC_A_SHADE    4
#define CC_A_ENV      5
#define CC_A_ONE      6
#define CC_A_NOISE    7
#define CC_A_ZERO     8

#define CC_B_COMBINED   0
#define CC_B_TEXEL0     1
#define CC_B_TEXEL1     2
#define CC_B_PRIM       3
#define CC_B_SHADE      4
#define CC_B_ENV        5
#define CC_B_KEY_CENTER 6
#define CC_B_K4         7
#define CC_B_ZERO       8

#define CC_M_COMBINED   0
#define CC_M_TEXEL0     1
#define CC_M_TEXEL1     2
#define CC_M_PRIM       3
#define CC_M_SHADE      4
#define CC_M_ENV        5
#define CC_M_KEY_SCALE  6
#define CC_M_COMBINED_A 7
#define CC_M_TEXEL0_A   8
#define CC_M_TEXEL1_A   9
#define CC_M_PRIM_A     10
#define CC_M_SHADE_A    11
#define CC_M_ENV_A      12
#define CC_M_LOD_FRAC   13
#define CC_M_PRIM_LOD   14
#define CC_M_K5         15
#define CC_M_ZERO       16

#define CC_D_COMBINED 0
#define CC_D_TEXEL0   1
#define CC_D_TEXEL1   2
#define CC_D_PRIM     3
#define CC_D_SHADE    4
#define CC_D_ENV      5
#define CC_D_ONE      6
#define CC_D_ZERO     7

#define AC_COMBINED 0
#define AC_TEXEL0   1
#define AC_TEXEL1   2
#define AC_PRIM     3
#define AC_SHADE    4
#define AC_ENV      5
#define AC_ONE      6
#define AC_ZERO     7

#define AC_M_LOD_FRAC 0
#define AC_M_TEXEL0   1
#define AC_M_TEXEL1   2
#define AC_M_PRIM     3
#define AC_M_SHADE    4
#define AC_M_ENV      5
#define AC_M_PRIM_LOD 6
#define AC_M_ZERO     7

static inline uint64_t rdp_set_combine(
    int rgb_a0, int rgb_b0, int rgb_m0, int rgb_d0,
    int a_a0,   int a_b0,   int a_m0,   int a_d0,
    int rgb_a1, int rgb_b1, int rgb_m1, int rgb_d1,
    int a_a1,   int a_b1,   int a_m1,   int a_d1)
{
    const uint64_t w0 = ((uint64_t)rgb_a0 << 52) | ((uint64_t)rgb_m0 << 47) |
                        ((uint64_t)a_a0   << 44) | ((uint64_t)a_m0   << 41) |
                        ((uint64_t)rgb_a1 << 37) | ((uint64_t)rgb_m1 << 32);
    const uint64_t w1 = ((uint64_t)rgb_b0 << 28) | ((uint64_t)rgb_b1 << 24) |
                        ((uint64_t)a_a1   << 21) | ((uint64_t)a_m1   << 18) |
                        ((uint64_t)rgb_d0 << 15) | ((uint64_t)a_b0   << 12) |
                        ((uint64_t)a_d0   <<  9) | ((uint64_t)rgb_d1 <<  6) |
                        ((uint64_t)a_b1   <<  3) | ((uint64_t)a_d1);
    return RDP_CMD(0x3c) | w0 | w1;
}

static inline uint64_t rdp_set_combine_both(
    int rgb_a, int rgb_b, int rgb_m, int rgb_d,
    int a_a,   int a_b,   int a_m,   int a_d)
{
    return rdp_set_combine(rgb_a, rgb_b, rgb_m, rgb_d, a_a, a_b, a_m, a_d,
                           rgb_a, rgb_b, rgb_m, rgb_d, a_a, a_b, a_m, a_d);
}

/* Colors */
#define RDP_SET_PRIM_COLOR(rgba)  (RDP_CMD(0x3a) | (uint64_t)(uint32_t)(rgba))
#define RDP_SET_ENV_COLOR(rgba)   (RDP_CMD(0x3b) | (uint64_t)(uint32_t)(rgba))
#define RDP_SET_BLEND_COLOR(rgba) (RDP_CMD(0x39) | (uint64_t)(uint32_t)(rgba))
#define RDP_SET_FOG_COLOR(rgba)   (RDP_CMD(0x38) | (uint64_t)(uint32_t)(rgba))
#define RDP_SET_FILL_COLOR(v)     (RDP_CMD(0x37) | (uint64_t)(uint32_t)(v))

/* Chroma Key registers */
static inline uint64_t rdp_set_key_gb(uint32_t width_g, uint8_t center_g, uint8_t scale_g,
                                      uint32_t width_b, uint8_t center_b, uint8_t scale_b)
{
    return RDP_CMD(0x2a) |
           ((uint64_t)(width_g & 0xfff) << 44) | ((uint64_t)(width_b & 0xfff) << 32) |
           ((uint64_t)center_g << 24) | ((uint64_t)scale_g << 16) |
           ((uint64_t)center_b <<  8) | (uint64_t)scale_b;
}

static inline uint64_t rdp_set_key_r(uint32_t width_r, uint8_t center_r, uint8_t scale_r)
{
    return RDP_CMD(0x2b) |
           ((uint64_t)(width_r & 0xfff) << 16) |
           ((uint64_t)center_r << 8) | (uint64_t)scale_r;
}

/* Render targets & Scissor */
static inline uint64_t rdp_set_color_image(int fmt, int size, uint32_t width,
                                           uint32_t phys_addr)
{
    return RDP_CMD(0x3f) |
           ((uint64_t)fmt << 53) | ((uint64_t)size << 51) |
           ((uint64_t)(width - 1) << 32) | (phys_addr & 0x3ffffff);
}

static inline uint64_t rdp_set_scissor(uint32_t xh, uint32_t yh,
                                       uint32_t xl, uint32_t yl)
{
    return RDP_CMD(0x2d) |
           ((uint64_t)(xh & 0xfff) << 44) | ((uint64_t)(yh & 0xfff) << 32) |
           ((uint64_t)(xl & 0xfff) << 12) | ((uint64_t)(yl & 0xfff) <<  0);
}

static inline uint64_t rdp_fill_rectangle(uint32_t xl, uint32_t yl,
                                          uint32_t xh, uint32_t yh)
{
    return RDP_CMD(0x36) |
           ((uint64_t)(xl & 0xfff) << 44) | ((uint64_t)(yl & 0xfff) << 32) |
           ((uint64_t)(xh & 0xfff) << 12) | ((uint64_t)(yh & 0xfff) <<  0);
}

/* Texture & TMEM commands */
static inline uint64_t rdp_set_texture_image(int fmt, int siz, uint32_t width,
                                             uint32_t phys_addr)
{
    return RDP_CMD(0x3d) |
           ((uint64_t)fmt << 53) | ((uint64_t)siz << 51) |
           ((uint64_t)((width - 1) & 0x3ff) << 32) |
           (phys_addr & 0xffffff);
}

static inline uint64_t rdp_set_tile(int fmt, int siz, uint32_t line,
                                    uint32_t tmem, uint32_t tile,
                                    uint32_t mask_s, uint32_t mask_t)
{
    return RDP_CMD(0x35) |
           ((uint64_t)fmt << 53) | ((uint64_t)siz << 51) |
           ((uint64_t)(line & 0x1ff) << 41) | ((uint64_t)(tmem & 0x1ff) << 32) |
           ((uint64_t)(tile & 7) << 24) |
           ((uint64_t)(mask_t & 0xf) << 14) | ((uint64_t)(mask_s & 0xf) << 4);
}

static inline uint64_t rdp_load_tile(uint32_t tile, uint32_t sl, uint32_t tl,
                                     uint32_t sh, uint32_t th)
{
    return RDP_CMD(0x34) |
           ((uint64_t)(sl & 0xfff) << 44) | ((uint64_t)(tl & 0xfff) << 32) |
           ((uint64_t)(tile & 7) << 24) |
           ((uint64_t)(sh & 0xfff) << 12) | (uint64_t)(th & 0xfff);
}

static inline uint64_t rdp_set_tile_size(uint32_t tile, uint32_t sl, uint32_t tl,
                                         uint32_t sh, uint32_t th)
{
    return RDP_CMD(0x32) |
           ((uint64_t)(sl & 0xfff) << 44) | ((uint64_t)(tl & 0xfff) << 32) |
           ((uint64_t)(tile & 7) << 24) |
           ((uint64_t)(sh & 0xfff) << 12) | (uint64_t)(th & 0xfff);
}

static inline uint64_t rdp_tex_rect_w0(uint32_t tile, uint32_t xl, uint32_t yl,
                                       uint32_t xh, uint32_t yh)
{
    return RDP_CMD(0x24) |
           ((uint64_t)(xl & 0xfff) << 44) | ((uint64_t)(yl & 0xfff) << 32) |
           ((uint64_t)(tile & 7) << 24) |
           ((uint64_t)(xh & 0xfff) << 12) | (uint64_t)(yh & 0xfff);
}

static inline uint64_t rdp_tex_rect_w1(int32_t s, int32_t t,
                                       int32_t dsdx, int32_t dtdy)
{
    return ((uint64_t)(uint16_t)s    << 48) | ((uint64_t)(uint16_t)t    << 32) |
           ((uint64_t)(uint16_t)dsdx << 16) | (uint64_t)(uint16_t)dtdy;
}

static inline uint64_t rdp_set_convert(int32_t k0, int32_t k1, int32_t k2,
                                       int32_t k3, int32_t k4, int32_t k5)
{
    return RDP_CMD(0x2c) |
           ((uint64_t)(k0 & 0x1ff) << 45) | ((uint64_t)(k1 & 0x1ff) << 36) |
           ((uint64_t)(k2 & 0x1ff) << 27) | ((uint64_t)(k3 & 0x1ff) << 18) |
           ((uint64_t)(k4 & 0x1ff) <<  9) | (uint64_t)(k5 & 0x1ff);
}

#endif /* RDPCHROMA_RDP_H */
