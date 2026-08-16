#ifndef RDPCHROMA_MODEL_H
#define RDPCHROMA_MODEL_H

#include <stdint.h>

/* Sign rule for combiner sub_a / sub_b / add inputs */
int32_t model_extend_9(uint32_t value);

/* Sign rule for multiplier input (plain 9-bit sign extension) */
int32_t model_mul_9(uint32_t value);

/* 9-bit clamp to 0..255 on combiner output */
uint32_t model_clamp_9(int32_t value);

/* 17-bit combiner equation: ((a - b) * c + (d << 8) + 0x80) & 0x1ffff */
int32_t model_combine17(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

/* Sign-extends raw 17-bit combiner output */
int32_t model_sign_17(int32_t value);

/*
 * Hardware-validated chroma keyer. keyalpha saturates at 0xff, exactly as
 * angrylion does.
 *
 * An earlier revision clamped at 0x100 to explain the 255 readout seen at
 * saturation. That was wrong: the 255 comes from the blender's partial-reject
 * passthrough, not from a higher ceiling. See model_blender_1cycle.
 */
int32_t model_keyalpha(const int32_t combined17[3], const uint32_t width[3]);
int32_t model_keyalpha_1ch(int32_t combined17, uint32_t width);

/*
 * Blender readout for the force_blend "pixel over black memory" configuration
 * every probe here uses, INCLUDING the partial-reject passthrough:
 *
 *   dontblend = partialreject && pixel_alpha >= 0xff  ->  pass blender1a through
 *
 * partialreject holds whenever m1b selects pixel alpha and m2b selects inverse
 * pixel alpha - which is precisely this configuration. Above the threshold the
 * blender therefore stops blending and emits the pixel colour unchanged, which
 * is why the readout jumps to the pixel value rather than continuing the
 * quantized ramp. Omitting this path is what produced a phantom "angrylion
 * bug" in an earlier revision of this suite.
 */
int32_t model_blend_step(int32_t pixel, int32_t alpha);
int32_t model_blender_1cycle(int32_t pixel, int32_t alpha);
int32_t model_blender_2cycle(int32_t pixel, int32_t alpha);

#endif /* RDPCHROMA_MODEL_H */
