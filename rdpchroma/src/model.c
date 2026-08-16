#include "model.h"

int32_t model_extend_9(uint32_t value)
{
    value &= 0x1ff;
    return (value & 0x180) == 0x180 ? (int32_t)(value | ~0x1ffu) : (int32_t)value;
}

int32_t model_mul_9(uint32_t value)
{
    value &= 0x1ff;
    return (value & 0x100) ? (int32_t)value - 0x200 : (int32_t)value;
}

uint32_t model_clamp_9(int32_t value)
{
    const uint32_t v = (uint32_t)value & 0x1ff;
    if ((v & 0x180) == 0x180) return 0u;    /* negative */
    if (v & 0x100)            return 255u;  /* >= 256 */
    return v;
}

int32_t model_combine17(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    const int32_t ea = model_extend_9(a);
    const int32_t eb = model_extend_9(b);
    const int32_t ec = model_mul_9(c);
    const int32_t ed = model_extend_9(d);

    return (int32_t)((uint32_t)((ea - eb) * ec + (ed * 256) + 0x80) & 0x1ffff);
}

int32_t model_sign_17(int32_t value)
{
    const uint32_t v = (uint32_t)value & 0x1ffff;
    return (v & 0x10000) ? (int32_t)(v | ~0x1ffffu) : (int32_t)v;
}

/*
 * Per-channel distance.
 * (x & 0xf) == 8 tie-break correction adds +16 (+0x10).
 */
static int32_t channel_distance(int32_t combined17, uint32_t width)
{
    int32_t k = model_sign_17(combined17);
    if (k > 0) k = ((k & 0xf) == 8) ? (-k + 0x10) : -k;
    return (int32_t)((width & 0xfff) << 4) + k;
}

/* Saturates at 0xff, as angrylion does. The 255 seen at saturation through the
 * blender is the partial-reject passthrough, not a higher ceiling. */
int32_t model_keyalpha_1ch(int32_t combined17, uint32_t width)
{
    int32_t k = channel_distance(combined17, width);
    if (k < 0)   k = 0;
    if (k > 255) k = 255;
    return k;
}

int32_t model_keyalpha(const int32_t combined17[3], const uint32_t width[3])
{
    int32_t k = channel_distance(combined17[0], width[0]);

    for (int i = 1; i < 3; i++) {
        const int32_t c = channel_distance(combined17[i], width[i]);
        if (c < k) k = c;
    }

    if (k < 0)   k = 0;
    if (k > 255) k = 255;
    return k;
}

/* One blender pass: P * A over black memory, alpha quantized to five bits. */
int32_t model_blend_step(int32_t pixel, int32_t alpha)
{
    const int32_t a = (alpha < 0) ? 0 : ((alpha > 256) ? 256 : alpha);
    return (((pixel & 0xff) * (a >> 3)) >> 5) & 0xff;
}

int32_t model_blender_1cycle(int32_t pixel, int32_t alpha)
{
    /* Partial-reject: at or above 0xff the blend is skipped entirely and the
     * pixel colour passes straight through. */
    if (alpha >= 0xff) return pixel & 0xff;
    return model_blend_step(pixel, alpha);
}

int32_t model_blender_2cycle(int32_t pixel, int32_t alpha)
{
    /* Cycle 0 always blends; cycle 1 takes cycle 0's output as its P input and
     * is the one that can be skipped by partial-reject. */
    const int32_t first = model_blend_step(pixel, alpha);
    if (alpha >= 0xff) return first;
    return model_blend_step(first, alpha);
}
