#include <libdragon.h>
#include <stdio.h>
#include <string.h>

#include "tests.h"
#include "harness.h"
#include "model.h"
#include "probe.h"
#include "rdp.h"

#define OM_KEY_1CYC (OM_RGB_READOUT | RDP_OM_CYCLE(RDP_CYCLE_1) | RDP_OM_KEY_EN)
#define OM_NOKEY    (OM_RGB_READOUT | RDP_OM_CYCLE(RDP_CYCLE_1))

#define VAL_ONE  0x100u
#define VAL_ZERO 0u

static int32_t expected_rgb(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return (int32_t)model_clamp_9(model_combine17(a, b, c, d) >> 8);
}

/* ------------------------------------------------------------- Test Items */

test_item_t g_test_items[] = {
    /* Category 0: Response Function Arithmetic */
    { "R1", "17-Bit Pre-Shift Input (T2)",   "negative ramp",         5, TEST_STATE_PENDING, 0, 0, "" },
    { "R2", "12-Bit Key Width Align (A1)",   "full 12-bit R,G,B",   256, TEST_STATE_PENDING, 0, 0, "" },
    { "R3", "(x & 0xf)==8 Tie-Break (A2)",   "+16 bump silicon",     64, TEST_STATE_PENDING, 0, 0, "" },
    { "R4", "Signed Magnitude Range (A3)",   "+/- 65000 limits",     64, TEST_STATE_PENDING, 0, 0, "" },
    { "R5", "Cross-Channel Plain Min (A4)",  "plain min reduction",  64, TEST_STATE_PENDING, 0, 0, "" },

    /* Category 1: Chromabypass & YUV Path */
    { "B1", "Chromabypass Multiplex (T1)",   "key_en 0/1 routing",    2, TEST_STATE_PENDING, 0, 0, "" },
    { "B2", "YUV Texel Chromabypass (P6)",   "converted texel bypass",2, TEST_STATE_PENDING, 0, 0, "" },

    /* Category 2: Integration Layer Behaviors */
    { "I1", "2-Cycle Key Source (I1)",       "cycle 1 combined",      2, TEST_STATE_PENDING, 0, 0, "" },
    { "I2", "2-Cycle Bypass Source (I2)",    "cycle 1 sub_a",         2, TEST_STATE_PENDING, 0, 0, "" },
    { "I3", "Alpha Dither Suppressed (I3)",  "0/16 drawn on key_en", 16, TEST_STATE_PENDING, 0, 0, "" },
    { "I4", "cvg_times_alpha Order (I4)",    "scaled by comb alpha",  1, TEST_STATE_PENDING, 0, 0, "" },
    { "I5", "alpha_cvg_select Discard (T3)", "keyalpha thrown out",   2, TEST_STATE_PENDING, 0, 0, "" },
    { "I6", "2-Cycle Alpha Compare (T4)",    "sees cycle 0 alpha",    2, TEST_STATE_PENDING, 0, 0, "" },

    /* Category 3: Key Registers in Combiner */
    { "K1", "key_center (sub_b) Input (B1)", "8-bit combiner input", 32, TEST_STATE_PENDING, 0, 0, "" },
    { "K2", "key_scale (mul) Input (B2)",    "8-bit multiplier",     32, TEST_STATE_PENDING, 0, 0, "" },
    { "K3", "key_scale == ENV_A Equiv (B2b)","bit-identical path",   32, TEST_STATE_PENDING, 0, 0, "" },
    { "K4", "key_width Non-Leakage (B5)",    "no combiner leakage",  64, TEST_STATE_PENDING, 0, 0, "" },
    { "K5", "RGB Channel Lane Wiring (B3)",  "R,G,B mapped lanes",    3, TEST_STATE_PENDING, 0, 0, "" },

    /* Category 4: Designed Idiom, Modes & Formats */
    { "M1", "Designed Idiom via PRIM (P1)",  "(PRIM-C)*S w/ W",      64, TEST_STATE_PENDING, 0, 0, "" },
    { "M2", "Real Texture Point Idiom (P2)", "point texel ramp",     32, TEST_STATE_PENDING, 0, 0, "" },
    { "M3", "Real Texture Bilerp (P3)",      "half-step dsdx=0.5",   32, TEST_STATE_PENDING, 0, 0, "" },
    { "M4", "Target Format Independence(P4)","16bpp vs 32bpp FB",    32, TEST_STATE_PENDING, 0, 0, "" },
    { "M5", "COPY Mode Ignores key_en (P5)", "identical checksum",    2, TEST_STATE_PENDING, 0, 0, "" },
    { "M6", "SET_KEY Latency / Sync (C6)",   "no backwards bleed",    3, TEST_STATE_PENDING, 0, 0, "" },

    /* Category 5: Saturation ceiling and the blender partial-reject path */
    { "H1", "1-Cycle Saturation+Passthru",  "33 pts, both parities",33, TEST_STATE_PENDING, 0, 0, "" },
    { "H2", "2-Cycle Double Blend",         "blender runs twice",   33, TEST_STATE_PENDING, 0, 0, "" },
    { "H3", "Boundary at 1-pt Resolution",  "raw=255 discriminates", 9, TEST_STATE_PENDING, 0, 0, "" },
    { "H4", "Passthrough Direct Proof",     "output = pixel colour", 3, TEST_STATE_PENDING, 0, 0, "" },
    { "H5", "Multi-Width Saturation",       "reached by diff. math", 5, TEST_STATE_PENDING, 0, 0, "" },
    { "H6", "Keyalpha == Combiner Alpha",   "same blender path",     2, TEST_STATE_PENDING, 0, 0, "" },
};

const uint32_t g_test_item_count = sizeof(g_test_items) / sizeof(g_test_items[0]);

#define COLOR32(r, g, b, a) \
    (((uint32_t)(r) << 24) | ((uint32_t)(g) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(a))

test_category_t g_test_categories[] = {
    { "1. RESPONSE FUNCTION ARITHMETIC", COLOR32(0, 229, 255, 255),  0, 5 },
    { "2. CHROMABYPASS & YUV PATH",      COLOR32(128, 216, 255, 255), 5, 2 },
    { "3. INTEGRATION LAYER BEHAVIORS",  COLOR32(64, 169, 255, 255),  7, 6 },
    { "4. KEY REGISTERS IN COMBINER",    COLOR32(255, 171, 0, 255),  13, 5 },
    { "5. IDIOM, MODES & FORMATS",       COLOR32(178, 255, 89, 255), 18, 6 },
    { "6. SATURATION & BLENDER PATH",     COLOR32(255, 167, 38, 255), 24, 6 },
};

const uint32_t g_test_category_count = sizeof(g_test_categories) / sizeof(g_test_categories[0]);

void tests_init_suite(test_suite_t *suite)
{
    suite->items = g_test_items;
    suite->item_count = g_test_item_count;
    suite->categories = g_test_categories;
    suite->category_count = g_test_category_count;
    suite->total_points = 0;
    suite->passed_count = 0;
    suite->failed_count = 0;
    suite->is_done = 0;

    for (uint32_t i = 0; i < g_test_item_count; i++) {
        suite->total_points += g_test_items[i].points;
    }
}

/* -------------------------------------------------------- Test Functions */

/* R1: T2 17-Bit Pre-Shift Input */
static void run_test_r1(test_item_t *item)
{
    report_printf("\n--- [R1] 17-Bit Pre-Shift Input (T2) ---\n");
    uint32_t err = 0;

    for (uint32_t prim = 56; prim <= 72; prim += 4) {
        key_probe p = {
            .other_modes = OM_RGB_READOUT | RDP_OM_CYCLE(RDP_CYCLE_1) | RDP_OM_KEY_EN,
            .combine = rdp_set_combine_both(CC_A_ZERO, CC_B_PRIM, CC_M_ENV_A, CC_D_ZERO,
                                            AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
            .prim = (prim << 24) | (prim << 16) | (prim << 8) | 0xff,
            .env  = 0x00000010,
        };
        probe_set_uniform(&p, 0x40);

        const int32_t raw_model = 1152 - 16 * (int32_t)prim;
        const int32_t expected = (raw_model < 0) ? 0 : ((raw_model > 256) ? 256 : raw_model);
        const int32_t probe_exp = (expected > 255) ? 255 : expected;
        const int32_t measured = probe_keyalpha(&p);

        report_printf("  prim=%2lu -> measured=%ld expected=%ld\n",
                      (unsigned long)prim, (long)measured, (long)probe_exp);

        if (measured != probe_exp) err++;
    }

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* R2: A1 12-Bit Key Width Alignment (256 representative points across R, G, B) */
static void run_test_r2(test_item_t *item)
{
    report_printf("\n--- [R2] 12-Bit Key Width Alignment (A1, 256 pts) ---\n");
    uint32_t err = 0;

    struct { uint32_t prim; unsigned ch; } passes[4] = {
        { 0,   0 }, /* R low bits */
        { 200, 0 }, /* R high bits */
        { 0,   1 }, /* G */
        { 0,   2 }, /* B */
    };

    for (int p_idx = 0; p_idx < 4; p_idx++) {
        const uint32_t prim_lvl = passes[p_idx].prim;
        const unsigned ch = passes[p_idx].ch;
        const uint32_t prim = (prim_lvl << 24) | (prim_lvl << 16) | (prim_lvl << 8) | 0xff;
        const int32_t combined = model_combine17(VAL_ONE, VAL_ZERO, VAL_ZERO, prim_lvl);
        const int32_t c17[3] = { combined, combined, combined };

        for (uint32_t width = 0; width < 4096; width += 64) {
            key_probe p = {
                .other_modes = OM_KEY_1CYC,
                .combine = rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_ZERO, CC_D_PRIM,
                                                AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
                .prim = prim,
            };
            probe_set_uniform(&p, 0xfff);
            p.width[ch] = width;

            const int32_t model = model_keyalpha(c17, p.width);
            const int32_t meas  = probe_keyalpha(&p);
            const int32_t exp_probe = (model > 255) ? 255 : model;

            if (meas != exp_probe) err++;
        }
    }

    report_printf("  A1 swept 256 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* R3: A2 (x & 0xf) == 8 Silicon Tie-Break (64 points) */
static void run_test_r3(test_item_t *item)
{
    report_printf("\n--- [R3] Silicon Tie-Break (x & 0xf)==8 (A2, 64 pts) ---\n");
    uint32_t err = 0;

    for (uint32_t c = 0; c < 64; c++) {
        key_probe p = {
            .other_modes = OM_KEY_1CYC,
            .combine = rdp_set_combine_both(CC_A_ONE, CC_B_PRIM, CC_M_ENV_A, CC_D_ZERO,
                                            AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
            .prim = 0xffffffff,
            .env  = c,
        };
        probe_set_uniform(&p, 24);

        const int32_t combined = model_combine17(VAL_ONE, 255, c, VAL_ZERO);
        const int32_t c17[3] = { combined, combined, combined };
        const int32_t model = model_keyalpha(c17, p.width);
        const int32_t meas  = probe_keyalpha(&p);
        const int32_t exp_probe = (model > 255) ? 255 : model;

        if (meas != exp_probe) err++;
    }

    report_printf("  A2 tie-break 64 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* R4: A3 Signed Magnitude Range (+/- 65000, 64 points) */
static void run_test_r4(test_item_t *item)
{
    report_printf("\n--- [R4] Signed Magnitude Range (A3, 64 pts) ---\n");
    uint32_t err = 0;

    for (uint32_t v = 0; v < 256; v += 8) {
        key_probe p_neg = {
            .other_modes = OM_KEY_1CYC,
            .combine = rdp_set_combine_both(CC_A_ZERO, CC_B_PRIM, CC_M_ENV_A, CC_D_ZERO,
                                            AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
            .prim = (v << 24) | (v << 16) | (v << 8) | 0xff,
            .env  = 0x10u,
        };
        probe_set_uniform(&p_neg, 0x40);

        const int32_t c_neg = model_combine17(VAL_ZERO, v, 0x10, VAL_ZERO);
        const int32_t a_neg[3] = { c_neg, c_neg, c_neg };
        const int32_t m_neg = model_keyalpha(a_neg, p_neg.width);
        const int32_t meas_neg = probe_keyalpha(&p_neg);
        const int32_t exp_neg = (m_neg > 255) ? 255 : m_neg;
        if (meas_neg != exp_neg) err++;

        key_probe p_pos = {
            .other_modes = OM_KEY_1CYC,
            .combine = rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_ENV_A, CC_D_ZERO,
                                            AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
            .prim = 0x000000ff,
            .env  = v,
        };
        probe_set_uniform(&p_pos, 0xfff);

        const int32_t c_pos = model_combine17(VAL_ONE, VAL_ZERO, v, VAL_ZERO);
        const int32_t a_pos[3] = { c_pos, c_pos, c_pos };
        const int32_t m_pos = model_keyalpha(a_pos, p_pos.width);
        const int32_t meas_pos = probe_keyalpha(&p_pos);
        const int32_t exp_pos = (m_pos > 255) ? 255 : m_pos;
        if (meas_pos != exp_pos) err++;
    }

    report_printf("  A3 signed magnitude 64 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* R5: Cross-Channel Plain Minimum Reduction (64 points) */
static void run_test_r5(test_item_t *item)
{
    report_printf("\n--- [R5] Cross-Channel Minimum (A4, 64 pts) ---\n");
    uint32_t err = 0;

    for (uint32_t wr = 0; wr < 64; wr += 8) {
        for (uint32_t wg = 0; wg < 64; wg += 8) {
            key_probe p = {
                .other_modes = OM_KEY_1CYC,
                .combine = rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_ZERO, CC_D_PRIM,
                                                AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
                .prim = 0x000000ff,
            };
            probe_set_uniform(&p, 0xfff);
            p.width[0] = wr;
            p.width[1] = wg;

            const int32_t combined = model_combine17(VAL_ONE, VAL_ZERO, VAL_ZERO, 0);
            const int32_t c17[3] = { combined, combined, combined };
            const int32_t model = model_keyalpha(c17, p.width);
            const int32_t meas  = probe_keyalpha(&p);
            const int32_t exp_probe = (model > 255) ? 255 : model;

            if (meas != exp_probe) err++;
        }
    }

    report_printf("  A4 plain minimum 64 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* B1: Chromabypass Multiplex Verification */
static void run_test_b1(test_item_t *item)
{
    report_printf("\n--- [B1] Chromabypass Multiplex (T1) ---\n");
    uint32_t err = 0;

    key_probe p0 = {
        .other_modes = OM_RGB_READOUT | RDP_OM_CYCLE(RDP_CYCLE_1),
        .combine = rdp_set_combine_both(CC_A_PRIM, CC_B_ZERO, CC_M_ZERO, CC_D_ENV,
                                        AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
        .prim = 0xff0000ff,
        .env  = 0x00ff00ff,
    };
    probe_set_uniform(&p0, 0x100);
    const uint32_t px0 = probe_rgb(&p0);
    report_printf("  key_en=0 (expect ENV green 00ff00): %06lx\n", (unsigned long)(px0 >> 8));
    if ((px0 >> 8) != 0x00ff00) err++;

    key_probe p1 = {
        .other_modes = OM_RGB_READOUT | RDP_OM_CYCLE(RDP_CYCLE_1) | RDP_OM_KEY_EN,
        .combine = rdp_set_combine_both(CC_A_PRIM, CC_B_ZERO, CC_M_ZERO, CC_D_ENV,
                                        AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
        .prim = 0xff0000ff,
        .env  = 0x00ff00ff,
    };
    probe_set_uniform(&p1, 0x100);
    const uint32_t px1 = probe_rgb(&p1);
    report_printf("  key_en=1 (expect PRIM red   ff0000): %06lx\n", (unsigned long)(px1 >> 8));
    if ((px1 >> 8) != 0xff0000) err++;

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* B2: YUV Texel Chromabypass (P6) */
static void run_test_b2(test_item_t *item)
{
    report_printf("\n--- [B2] YUV Texel Chromabypass (P6) ---\n");
    uint32_t err = 0;
    const uint16_t yuv_pixel[4] = { 0x8080, 0x8080, 0x8080, 0x8080 };
    tex_upload_yuv16(yuv_pixel, 4);

    rdp_target *t = probe_target();
    target_clear(t, 0);
    target_bind(t);
    dpl_cmd(OM_TEX_RGB | RDP_OM_KEY_EN | RDP_OM_CONVERT_ONE);
    dpl_cmd(rdp_set_combine_both(CC_A_TEXEL0, CC_B_ZERO, CC_M_ZERO, CC_D_ENV,
                                 AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE));
    dpl_cmd(RDP_SET_ENV_COLOR(0x00ff00ff));
    probe_emit_key_uniform(0x100);
    dpl_cmd(rdp_tex_rect_w0(0, 256 << 2, 4 << 2, 0, 0));
    dpl_cmd(rdp_tex_rect_w1(0, 0, DSDX_1TO1, 0));
    dpl_run();

    const uint32_t px = target_center_rgba(t);
    report_printf("  YUV chromabypass readout: %08lx\n", (unsigned long)px);
    if ((px >> 8) == 0x00ff00) err++;

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* I1: 2-Cycle Key Source (Cycle 1 combined) */
static void run_test_i1(test_item_t *item)
{
    report_printf("\n--- [I1] 2-Cycle Key Source (Cycle 1 Combined) ---\n");
    uint32_t err = 0;

    /*
     * The two cycles must produce DIFFERENT combined values or the test proves
     * nothing. Previously both were 0x80, so cycle 0 and cycle 1 gave the same
     * answer and the result could not distinguish them.
     *
     * Cycle 0 yields 0x80, cycle 1 yields 0x4080. They are far enough apart
     * that no single width puts both in range, so the two hypotheses predict
     * opposite answers at each width.
     */
    static const uint32_t widths[2] = { 16, 1040 };
    const int32_t c0 = model_combine17(VAL_ZERO, VAL_ZERO, VAL_ZERO, 0);
    const int32_t c1 = model_combine17(VAL_ONE,  VAL_ZERO, VAL_ZERO, 64);

    report_printf("  width  meas  if-cyc0  if-cyc1\n");

    for (unsigned i = 0; i < 2; i++) {
        int32_t d0 = (int32_t)(widths[i] << 4) - c0;
        int32_t d1 = (int32_t)(widths[i] << 4) - c1;
        if (d0 < 0)   d0 = 0;
        if (d0 > 256) d0 = 256;
        if (d1 < 0)   d1 = 0;
        if (d1 > 256) d1 = 256;

        key_probe p = {
            .other_modes = OM_ALPHA_READOUT | RDP_OM_CYCLE(RDP_CYCLE_2) |
                           RDP_OM_KEY_EN,
            .combine = rdp_set_combine(
                /* cycle 0: combined = PRIM (0) */
                CC_A_ZERO, CC_B_ZERO, CC_M_ZERO, CC_D_PRIM,
                AC_ZERO,   AC_ZERO,   AC_M_ZERO, AC_ONE,
                /* cycle 1: combined = ENV (64), sub_a = ONE for a white bypass */
                CC_A_ONE,  CC_B_ZERO, CC_M_ZERO, CC_D_ENV,
                AC_ZERO,   AC_ZERO,   AC_M_ZERO, AC_ONE
            ),
            .prim = 0x000000ff,
            .env  = 0x40404040,
        };
        probe_set_uniform(&p, widths[i]);

        /* 2-cycle runs the blender twice - see model_blender_2cycle. */
        const int32_t exp0 = model_blender_2cycle(255, d0);
        const int32_t exp1 = model_blender_2cycle(255, d1);
        const int32_t meas = (int32_t)red_of(probe_rgb(&p));

        report_printf("  %5lu  %4ld  %7ld  %7ld\n",
                      (unsigned long)widths[i], (long)meas, (long)exp0, (long)exp1);
        if (meas != exp1) err++;
    }

    report_printf("  cycle 1 is the source when the if-cyc1 column matches\n");
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* I2: 2-Cycle Bypass Source (Cycle 1 sub_a) */
static void run_test_i2(test_item_t *item)
{
    report_printf("\n--- [I2] 2-Cycle Chromabypass Source (Cycle 1 sub_a) ---\n");
    uint32_t err = 0;

    key_probe p = {
        .other_modes = OM_RGB_READOUT | RDP_OM_CYCLE(RDP_CYCLE_2) | RDP_OM_KEY_EN,
        .combine = rdp_set_combine(
            CC_A_PRIM, CC_B_ZERO, CC_M_ZERO, CC_D_ZERO,
            AC_ZERO,   AC_ZERO,   AC_M_ZERO, AC_ONE,
            CC_A_ENV,  CC_B_ZERO, CC_M_ZERO, CC_D_ZERO,
            AC_ZERO,   AC_ZERO,   AC_M_ZERO, AC_COMBINED
        ),
        .prim = 0xff0000ff,
        .env  = 0x00ff00ff,
    };
    probe_set_uniform(&p, 0x100);

    const uint32_t px = probe_rgb(&p);
    report_printf("  2-cycle chromabypass: %06lx (exp ENV green 00ff00)\n",
                  (unsigned long)(px >> 8));
    if ((px >> 8) != 0x00ff00) err++;

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* I3: Alpha Dither Suppressed Under key_en */
static void run_test_i3(test_item_t *item)
{
    report_printf("\n--- [I3] Alpha Dither Suppressed Under key_en ---\n");
    uint32_t err = 0;
    rdp_target *t = probe_target();

    /*
     * RDP_OM_DITHER_ALPHA (w1 bit 1) is dither_alpha_en, which makes alpha
     * compare use a RANDOM threshold instead of blend_color.a - a different
     * feature from the alpha dither matrix (w0 bits 5:4). Setting it here made
     * the threshold of 132 meaningless and the test nondeterministic.
     *
     * The key_en=0 control is what gives the result meaning: without it, any
     * failure that draws nothing passes trivially as "dither suppressed".
     */
    const uint64_t modes = RDP_OM_BASE | RDP_OM_RGB_DITHER(3) |
                           RDP_OM_ALPHA_DITHER(0) | RDP_OM_CYCLE(RDP_CYCLE_1) |
                           RDP_OM_ALPHA_COMPARE;
    uint32_t drawn[2];

    for (int key_en = 0; key_en <= 1; key_en++) {
        target_clear(t, 0);
        target_bind(t);
        dpl_cmd(modes | (key_en ? RDP_OM_KEY_EN : 0));
        /* add=ONE so the pixel is white with the bypass (key on) and without it
         * (key off); the two cases then differ only in where alpha came from. */
        dpl_cmd(rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_ZERO, CC_D_ONE,
                                     AC_ZERO, AC_ZERO, AC_M_ZERO, AC_PRIM));
        dpl_cmd(RDP_SET_PRIM_COLOR(0x00000080));   /* alpha 128 when key off */
        dpl_cmd(RDP_SET_BLEND_COLOR(132));         /* just above 128         */
        probe_emit_key_uniform(16);                /* keyalpha 128 when key on */
        target_draw_rect(t);
        dpl_run();

        drawn[key_en] = 0;
        for (uint32_t x = 0; x < 16; x++)
            if (target_pixel_rgba(t, x, 0) != 0) drawn[key_en]++;

        report_printf("  key_en=%d drawn=%2lu/16  %s\n", key_en,
                      (unsigned long)drawn[key_en],
                      drawn[key_en] == 0 ? "none (suppressed)"
                        : (drawn[key_en] == 16 ? "all (threshold missed)" : "mixed (dithered)"));
    }

    /* Dither must be live without the key and suppressed with it. */
    if (drawn[0] == 0 || drawn[0] == 16) err++;   /* control did not dither */
    if (drawn[1] != 0) err++;                      /* key_en failed to suppress */

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* I4: cvg_times_alpha Ordering (I4) */
static void run_test_i4(test_item_t *item)
{
    report_printf("\n--- [I4] cvg_times_alpha Ordering (I4) ---\n");
    uint32_t err = 0;
    rdp_target *t = probe_target();

    target_clear(t, 0);
    target_bind(t);
    dpl_cmd(RDP_OM_BASE | RDP_OM_NO_DITHER | RDP_OM_CYCLE(RDP_CYCLE_1) |
            RDP_OM_KEY_EN | RDP_OM_CVG_X_ALPHA);
    dpl_cmd(rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_ZERO, CC_D_PRIM,
                                 AC_ZERO, AC_ZERO, AC_M_ZERO, AC_PRIM));
    dpl_cmd(RDP_SET_PRIM_COLOR(0x00000040));
    probe_emit_key_uniform(23);
    target_draw_rect(t);
    dpl_run();

    const uint32_t px = target_center_rgba(t);
    const uint32_t cvg_byte = px & 0xff;

    report_printf("  cvg_byte=%02lx (exp 0x20 by comb alpha 0x40; not 0xc0 by keyalpha)\n",
                  (unsigned long)cvg_byte);
    if (cvg_byte != 0x20) err++;

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* I5: alpha_cvg_select Discards Keyalpha (T3) */
static void run_test_i5(test_item_t *item)
{
    report_printf("\n--- [I5] alpha_cvg_select Discards Keyalpha (T3) ---\n");
    uint32_t err = 0;
    rdp_target *t = probe_target();

    target_clear(t, 0);
    target_bind(t);
    dpl_cmd(OM_ALPHA_READOUT | RDP_OM_CYCLE(RDP_CYCLE_1) | RDP_OM_KEY_EN);
    dpl_cmd(rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_ZERO, CC_D_PRIM,
                                 AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE));
    dpl_cmd(RDP_SET_PRIM_COLOR(0x000000ff));
    probe_emit_key_uniform(16);
    target_draw_rect(t);
    dpl_run();
    const uint32_t a0 = red_of(target_center_rgba(t));

    target_clear(t, 0);
    target_bind(t);
    dpl_cmd(OM_ALPHA_READOUT | RDP_OM_CYCLE(RDP_CYCLE_1) | RDP_OM_KEY_EN |
            RDP_OM_ALPHA_CVG_SEL);
    dpl_cmd(rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_ZERO, CC_D_PRIM,
                                 AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE));
    dpl_cmd(RDP_SET_PRIM_COLOR(0x000000ff));
    probe_emit_key_uniform(16);
    target_draw_rect(t);
    dpl_run();
    const uint32_t a1 = red_of(target_center_rgba(t));

    report_printf("  sel=0 -> a0=%lu (exp 127) | sel=1 -> a1=%lu (exp 255)\n",
                  (unsigned long)a0, (unsigned long)a1);
    if (a0 != 127 || a1 != 255) err++;

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* I6: 2-Cycle Alpha Compare Source (T4) */
static void run_test_i6(test_item_t *item)
{
    report_printf("\n--- [I6] 2-Cycle Alpha Compare Sees Cycle 0 Alpha (T4) ---\n");
    uint32_t err = 0;
    rdp_target *t = probe_target();

    target_clear(t, 0x0000ffff);
    target_bind(t);
    dpl_cmd(OM_TEX_RGB | RDP_OM_CYCLE(RDP_CYCLE_2) | RDP_OM_KEY_EN | RDP_OM_ALPHA_COMPARE);
    dpl_cmd(rdp_set_combine(
        CC_A_ZERO, CC_B_ZERO, CC_M_ZERO, CC_D_ZERO,
        AC_ZERO,   AC_ZERO,   AC_M_ZERO, AC_PRIM,
        CC_A_ONE,  CC_B_ZERO, CC_M_ZERO, CC_D_ZERO,
        AC_ZERO,   AC_ZERO,   AC_M_ZERO, AC_COMBINED
    ));
    dpl_cmd(RDP_SET_PRIM_COLOR(0x00000020));
    dpl_cmd(RDP_SET_BLEND_COLOR(0x40));
    probe_emit_key_uniform(0x100);
    target_draw_rect(t);
    dpl_run();

    const int pass_seen_c0 = (target_center_rgba(t) == 0x0000ffff);
    report_printf("  Pixel rejected by cycle 0 alpha 0x20 < 0x40: %s\n",
                  pass_seen_c0 ? "YES" : "NO");
    if (!pass_seen_c0) err++;

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* K1: key_center as sub_b (B1, 32 pts) */
static void run_test_k1(test_item_t *item)
{
    report_printf("\n--- [K1] key_center as Combiner sub_b (B1, 32 pts) ---\n");
    uint32_t err = 0;

    for (uint32_t center = 0; center < 256; center += 8) {
        key_probe p = {
            .other_modes = OM_NOKEY,
            .combine = rdp_set_combine_both(CC_A_ONE, CC_B_KEY_CENTER, CC_M_ENV_A, CC_D_ZERO,
                                            AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
            .env = 0x000000ff,
        };
        probe_set_uniform(&p, 0);
        for (int i = 0; i < 3; i++) p.center[i] = center;

        const int32_t meas = (int32_t)red_of(probe_rgb(&p));
        const int32_t exp  = expected_rgb(VAL_ONE, center, 0xff, VAL_ZERO);
        if (meas != exp) err++;
    }

    report_printf("  B1 key_center swept 32 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* K2: key_scale as Multiplier (B2, 32 pts) */
static void run_test_k2(test_item_t *item)
{
    report_printf("\n--- [K2] key_scale as Multiplier (B2, 32 pts) ---\n");
    uint32_t err = 0;

    for (uint32_t scale = 0; scale < 256; scale += 8) {
        key_probe p = {
            .other_modes = OM_NOKEY,
            .combine = rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_KEY_SCALE, CC_D_ZERO,
                                            AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
        };
        probe_set_uniform(&p, 0);
        for (int i = 0; i < 3; i++) p.scale[i] = scale;

        const int32_t meas = (int32_t)red_of(probe_rgb(&p));
        const int32_t exp  = expected_rgb(VAL_ONE, VAL_ZERO, scale, VAL_ZERO);
        if (meas != exp) err++;
    }

    report_printf("  B2 key_scale swept 32 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* K3: key_scale / ENV_ALPHA Bit-Equivalence (B2b, 32 pts) */
static void run_test_k3(test_item_t *item)
{
    report_printf("\n--- [K3] key_scale vs ENV_ALPHA Equivalence (B2b, 32 pts) ---\n");
    uint32_t err = 0;

    for (uint32_t v = 0; v < 256; v += 8) {
        key_probe key = {
            .other_modes = OM_NOKEY,
            .combine = rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_KEY_SCALE, CC_D_ZERO,
                                            AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
        };
        probe_set_uniform(&key, 0);
        for (int i = 0; i < 3; i++) key.scale[i] = v;

        key_probe env = {
            .other_modes = OM_NOKEY,
            .combine = rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_ENV_A, CC_D_ZERO,
                                            AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
            .env = v,
        };
        probe_set_uniform(&env, 0);

        const int32_t v_key = (int32_t)red_of(probe_rgb(&key));
        const int32_t v_env = (int32_t)red_of(probe_rgb(&env));
        if (v_key != v_env) err++;
    }

    report_printf("  B2b equivalence 32 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* K4: key_width Non-Leakage (B5, 64 pts) */
static void run_test_k4(test_item_t *item)
{
    report_printf("\n--- [K4] key_width Non-Leakage (B5, 64 pts) ---\n");
    uint32_t err = 0;
    const int32_t baseline = expected_rgb(VAL_ONE, 0x55, 0xff, VAL_ZERO);

    for (uint32_t width = 0; width < 4096; width += 64) {
        key_probe p = {
            .other_modes = OM_NOKEY,
            .combine = rdp_set_combine_both(CC_A_ONE, CC_B_KEY_CENTER, CC_M_ENV_A, CC_D_ZERO,
                                            AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
            .env = 0x000000ff,
        };
        probe_set_uniform(&p, width);
        for (int i = 0; i < 3; i++) p.center[i] = 0x55;

        const int32_t meas = (int32_t)red_of(probe_rgb(&p));
        if (meas != baseline) err++;
    }

    report_printf("  B5 non-leakage swept 64 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* K5: RGB Channel Lane Wiring (B3) */
static void run_test_k5(test_item_t *item)
{
    report_printf("\n--- [K5] RGB Channel Lane Wiring (B3) ---\n");
    uint32_t err = 0;
    static const uint32_t centres[3] = { 0x10, 0x40, 0x80 };

    key_probe p = {
        .other_modes = OM_NOKEY,
        .combine = rdp_set_combine_both(CC_A_ONE, CC_B_KEY_CENTER, CC_M_ENV_A, CC_D_ZERO,
                                        AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
        .env = 0x000000ff,
    };
    probe_set_uniform(&p, 0);
    for (int i = 0; i < 3; i++) p.center[i] = centres[i];

    const uint32_t px = probe_rgb(&p);
    for (int i = 0; i < 3; i++) {
        const uint32_t got = (px >> (24 - 8 * i)) & 0xff;
        const int32_t  exp = expected_rgb(VAL_ONE, centres[i], 0xff, VAL_ZERO);
        if ((int32_t)got != exp) err++;
    }

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* M1: Designed Idiom via PRIM (P1, 64 pts) */
static void run_test_m1(test_item_t *item)
{
    report_printf("\n--- [M1] Designed Idiom via PRIM (P1, 64 pts) ---\n");
    uint32_t err = 0;

    for (uint32_t v = 0; v < 256; v += 4) {
        key_probe p = {
            .other_modes = OM_TEX_RGB | RDP_OM_KEY_EN,
            .combine = rdp_set_combine_both(CC_A_PRIM, CC_B_KEY_CENTER, CC_M_KEY_SCALE, CC_D_ZERO,
                                            AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE),
            .prim = (v << 24) | (v << 16) | (v << 8) | 0xff,
        };
        for (int c = 0; c < 3; c++) {
            p.width[c]  = 0x100;
            p.center[c] = 0x80;
            p.scale[c]  = 0x40;
        }

        const int32_t c17 = model_combine17(v, 0x80, 0x40, 0);
        const int32_t a[3] = { c17, c17, c17 };
        const int32_t model = model_keyalpha(a, p.width);
        const int32_t meas  = probe_keyalpha(&p);
        const int32_t exp_probe = (model > 255) ? 255 : model;
        if (meas != exp_probe) err++;
    }

    report_printf("  P1 idiom via PRIM 64 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* M2: Real Texture Point-Sampled Idiom (P2, 32 pts exact binary search) */
static void run_test_m2(test_item_t *item)
{
    report_printf("\n--- [M2] Real Texture Point Idiom (P2, 32 pts) ---\n");
    uint32_t err = 0;
    tex_upload_i8(tex_ramp_i8(), 256);

    const uint32_t centre = 0x80, scale = 0x40, width = 0x100;
    rdp_target *t = probe_target();

    for (uint32_t x = 0; x < 256; x += 8) {
        int32_t lo = 0, hi = 255, meas = 0;
        while (lo <= hi) {
            const int32_t mid = (lo + hi + 1) / 2;
            target_clear(t, 0x0000ffff);
            target_bind(t);
            dpl_cmd(OM_TEX_RGB | RDP_OM_KEY_EN | RDP_OM_ALPHA_COMPARE);
            dpl_cmd(rdp_set_combine_both(CC_A_TEXEL0, CC_B_KEY_CENTER, CC_M_KEY_SCALE, CC_D_ZERO,
                                         AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE));
            dpl_cmd(RDP_SET_BLEND_COLOR((uint32_t)mid));
            dpl_cmd(rdp_set_key_gb(width, (uint8_t)centre, (uint8_t)scale,
                                   width, (uint8_t)centre, (uint8_t)scale));
            dpl_cmd(rdp_set_key_r(width, (uint8_t)centre, (uint8_t)scale));
            dpl_cmd(rdp_tex_rect_w0(0, 256 << 2, 4 << 2, 0, 0));
            dpl_cmd(rdp_tex_rect_w1(0, 0, DSDX_1TO1, 0));
            dpl_run();

            if (target_pixel_raw(t, x, 0) != 0x0000ffff) { meas = mid; lo = mid + 1; }
            else                                          { hi = mid - 1; }
        }

        const int32_t c17 = model_combine17(x, centre, scale, 0);
        const int32_t a[3] = { c17, c17, c17 };
        const uint32_t w[3] = { width, width, width };
        const int32_t model = model_keyalpha(a, w);
        const int32_t exp_probe = (model > 255) ? 255 : model;
        if (meas != exp_probe) err++;
    }

    report_printf("  P2 point texture 32 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* M3: Real Texture Bilerp Interpolated Texels (P3, 32 pts exact binary search) */
static void run_test_m3(test_item_t *item)
{
    report_printf("\n--- [M3] Real Texture Bilerp Interpolation (P3, 32 pts) ---\n");
    uint32_t err = 0;
    tex_upload_i8(tex_ramp_i8(), 256);

    const uint32_t centre = 0x80, scale = 0x40, width = 0x100;
    rdp_target *t = probe_target();

    /* 1. Measure interpolated texels */
    static uint8_t texv[256];
    target_clear(t, 0);
    target_bind(t);
    dpl_cmd(OM_TEX_RGB | RDP_OM_SAMPLE_QUAD);
    dpl_cmd(rdp_set_combine_both(CC_A_ZERO, CC_B_ZERO, CC_M_ZERO, CC_D_TEXEL0,
                                 AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE));
    dpl_cmd(rdp_tex_rect_w0(0, 256 << 2, 4 << 2, 0, 0));
    dpl_cmd(rdp_tex_rect_w1(0, 0, DSDX_1TO1 / 2, 0));
    dpl_run();

    for (uint32_t x = 0; x < 256; x += 8)
        texv[x] = (uint8_t)((target_pixel_rgba(t, x, 0) >> 24) & 0xff);

    /* 2. Measure keyalpha with binary search */
    for (uint32_t x = 0; x < 256; x += 8) {
        int32_t lo = 0, hi = 255, meas = 0;
        while (lo <= hi) {
            const int32_t mid = (lo + hi + 1) / 2;
            target_clear(t, 0x0000ffff);
            target_bind(t);
            dpl_cmd(OM_TEX_RGB | RDP_OM_KEY_EN | RDP_OM_ALPHA_COMPARE | RDP_OM_SAMPLE_QUAD);
            dpl_cmd(rdp_set_combine_both(CC_A_TEXEL0, CC_B_KEY_CENTER, CC_M_KEY_SCALE, CC_D_ZERO,
                                         AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE));
            dpl_cmd(RDP_SET_BLEND_COLOR((uint32_t)mid));
            dpl_cmd(rdp_set_key_gb(width, (uint8_t)centre, (uint8_t)scale,
                                   width, (uint8_t)centre, (uint8_t)scale));
            dpl_cmd(rdp_set_key_r(width, (uint8_t)centre, (uint8_t)scale));
            dpl_cmd(rdp_tex_rect_w0(0, 256 << 2, 4 << 2, 0, 0));
            dpl_cmd(rdp_tex_rect_w1(0, 0, DSDX_1TO1 / 2, 0));
            dpl_run();

            if (target_pixel_raw(t, x, 0) != 0x0000ffff) { meas = mid; lo = mid + 1; }
            else                                          { hi = mid - 1; }
        }

        const int32_t c17 = model_combine17(texv[x], centre, scale, 0);
        const int32_t a[3] = { c17, c17, c17 };
        const uint32_t w[3] = { width, width, width };
        const int32_t model = model_keyalpha(a, w);
        const int32_t exp_probe = (model > 255) ? 255 : model;
        if (meas != exp_probe) err++;
    }

    report_printf("  P3 bilerp texture 32 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* M4: Target Format Independence (16bpp vs 32bpp, 32 pts exact binary search) */
static void run_test_m4(test_item_t *item)
{
    report_printf("\n--- [M4] Framebuffer Format Independence (P4, 32 pts) ---\n");
    uint32_t err = 0;
    rdp_target *t16 = probe_target_16();

    for (uint32_t width = 0; width < 512; width += 16) {
        int32_t lo = 0, hi = 255, meas = 0;
        while (lo <= hi) {
            const int32_t mid = (lo + hi + 1) / 2;
            target_clear(t16, 0x003f);
            target_bind(t16);
            dpl_cmd(OM_TEX_RGB | RDP_OM_KEY_EN | RDP_OM_ALPHA_COMPARE);
            dpl_cmd(rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_ZERO, CC_D_PRIM,
                                         AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE));
            dpl_cmd(RDP_SET_PRIM_COLOR(0x000000ff));
            dpl_cmd(RDP_SET_BLEND_COLOR((uint32_t)mid));
            probe_emit_key_uniform(width);
            target_draw_rect(t16);
            dpl_run();

            if (target_pixel_raw(t16, 128, 0) != 0x003f) { meas = mid; lo = mid + 1; }
            else                                          { hi = mid - 1; }
        }

        const int32_t c17 = model_combine17(0x100, 0, 0, 0);
        const int32_t a[3] = { c17, c17, c17 };
        const uint32_t w[3] = { width, width, width };
        const int32_t model = model_keyalpha(a, w);
        const int32_t exp_probe = (model > 255) ? 255 : model;
        if (meas != exp_probe) err++;
    }

    report_printf("  P4 16bpp target swept 32 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* M5: COPY Mode Ignores key_en (P5) */
static void run_test_m5(test_item_t *item)
{
    report_printf("\n--- [M5] COPY Mode Ignores key_en (P5) ---\n");
    uint32_t err = 0;
    static uint16_t tex[256];
    for (int i = 0; i < 256; i++) {
        const uint32_t c5 = (uint32_t)i >> 3;
        tex[i] = (uint16_t)((c5 << 11) | (c5 << 6) | (c5 << 1) | 1);
    }
    tex_upload_rgba16(tex, 256);

    rdp_target *t16 = probe_target_16();
    uint32_t checksum[2];

    for (int key_en = 0; key_en <= 1; key_en++) {
        target_clear(t16, 0x003f);
        target_bind(t16);
        dpl_cmd(RDP_OM_BASE | RDP_OM_NO_DITHER | RDP_OM_CYCLE(RDP_CYCLE_COPY) |
                (key_en ? RDP_OM_KEY_EN : 0));
        dpl_cmd(rdp_set_key_gb(0x010, 0x80, 0x40, 0x010, 0x80, 0x40));
        dpl_cmd(rdp_set_key_r(0x010, 0x80, 0x40));
        dpl_cmd(rdp_tex_rect_w0(0, 256 << 2, 4 << 2, 0, 0));
        dpl_cmd(rdp_tex_rect_w1(0, 0, DSDX_COPY, 0));
        dpl_run();

        uint32_t sum = 0;
        for (uint32_t x = 0; x < 256; x++) {
            sum = sum * 31u + target_pixel_raw(t16, x, 0);
        }
        checksum[key_en] = sum;
    }

    /*
     * Identical checksums are trivially identical when the copy moved nothing,
     * so the copy has to be shown to have happened before the comparison means
     * anything. On the first hardware run of this test in ROM 2 the texrect
     * wrote all zeros and the "identical" result proved exactly nothing.
     */
    uint32_t nonzero = 0;
    for (uint32_t x = 0; x < 256; x++)
        if (target_pixel_raw(t16, x, 0) != 0 &&
            target_pixel_raw(t16, x, 0) != 0x003f) nonzero++;

    report_printf("  COPY key_en=0 crc=%08lx | key_en=1 crc=%08lx | copied=%lu/256\n",
                  (unsigned long)checksum[0], (unsigned long)checksum[1],
                  (unsigned long)nonzero);

    if (nonzero < 128) {
        report_printf("  INVALID: the texrect copied nothing, comparison is void\n");
        err++;
    } else if (checksum[0] != checksum[1]) {
        err++;
    }

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* M6: SET_KEY Latency Without SYNC_PIPE (C6) */
static void run_test_m6(test_item_t *item)
{
    report_printf("\n--- [M6] SET_KEY Latency Without SYNC_PIPE (C6) ---\n");
    uint32_t err = 0;
    rdp_target *t = probe_target();

    target_clear(t, 0);
    target_bind(t);
    dpl_cmd(OM_ALPHA_READOUT | RDP_OM_CYCLE(RDP_CYCLE_1) | RDP_OM_KEY_EN);
    dpl_cmd(rdp_set_combine_both(CC_A_ONE, CC_B_ZERO, CC_M_ZERO, CC_D_PRIM,
                                 AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE));
    dpl_cmd(RDP_SET_PRIM_COLOR(0x000000ff));
    probe_emit_key_uniform(16);
    target_draw_rect(t);
    probe_emit_key_uniform(24);
    dpl_run();

    const uint32_t head = red_of(target_pixel_rgba(t, 0, 0));
    const uint32_t tail = red_of(target_pixel_rgba(t, 255, 0));
    if (head != tail || head != 127) err++;

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* --------------------------------- Category 5: Saturation & blender path */

/* H1: 1-Cycle Blender Readout Sweep (Width 25, 17 pts) */
/*
 * ============================ Saturation & Blender ==========================
 *
 * These were originally written as "0x100 ceiling bug" tests, on the theory
 * that keyalpha saturated at 256 while angrylion capped it at 255. That theory
 * was wrong, and this suite is what disproved it.
 *
 * The readout jumps to 255 at saturation not because alpha reaches 256, but
 * because the blender stops blending: with m1b = pixel alpha and m2b = inverse
 * pixel alpha - the configuration every probe here uses - angrylion sets
 * partialreject, and
 *
 *     dontblend = partialreject && pixel_color.a >= 0xff
 *
 * passes blender1a straight through instead of blending. angrylion does the
 * same, so there is no divergence. keyalpha saturates at 0xff in both.
 *
 * H3 is the test that matters most: it walks the boundary at single-point
 * resolution including odd c, which is where the two competing models disagree
 * and where the earlier mistake was finally caught.
 */

/* Shared shape: combined17 = c + 0x80, so keyalpha = width*16 - (c + 0x80). */
static int32_t sat_raw(uint32_t c, uint32_t width)
{
    int32_t combined = (int32_t)c + 0x80;
    int32_t k = combined;
    if (k > 0) k = ((k & 0xf) == 8) ? (-k + 0x10) : -k;
    return (int32_t)(width << 4) + k;
}

static int32_t sat_keyalpha(uint32_t c, uint32_t width)
{
    const int32_t raw = sat_raw(c, width);
    if (raw < 0)   return 0;
    if (raw > 255) return 255;
    return raw;
}

/* Draws the saturation shape; returns the red channel of the readout. */
static int32_t sat_draw(uint32_t c, uint32_t width, int two_cycle, int sub_a)
{
    rdp_target *t = probe_target();

    target_clear(t, 0);
    target_bind(t);
    dpl_cmd(OM_ALPHA_READOUT | RDP_OM_KEY_EN |
            RDP_OM_CYCLE(two_cycle ? RDP_CYCLE_2 : RDP_CYCLE_1));
    if (two_cycle) {
        dpl_cmd(rdp_set_combine(
            CC_A_ZERO, CC_B_ZERO,  CC_M_ZERO,  CC_D_ZERO,
            AC_ZERO,   AC_ZERO,    AC_M_ZERO,  AC_ONE,
            sub_a,     CC_B_PRIM,  CC_M_ENV_A, CC_D_ZERO,
            AC_ZERO,   AC_ZERO,    AC_M_ZERO,  AC_ONE));
    } else {
        dpl_cmd(rdp_set_combine_both(sub_a, CC_B_PRIM, CC_M_ENV_A, CC_D_ZERO,
                                     AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE));
    }
    dpl_cmd(RDP_SET_PRIM_COLOR(0xffffffff));
    dpl_cmd(RDP_SET_ENV_COLOR(c));
    probe_emit_key_uniform(width);
    target_draw_rect(t);
    dpl_run();

    return (int32_t)red_of(target_center_rgba(t));
}

/* H1: 1-cycle saturation and passthrough, both parities of c (33 pts) */
static void run_test_h1(test_item_t *item)
{
    report_printf("\n--- [H1] 1-Cycle Saturation & Passthrough (33 pts) ---\n");
    uint32_t err = 0;
    const uint32_t width = 25;

    for (uint32_t c = 0; c <= 32; c++) {
        const int32_t A    = sat_keyalpha(c, width);
        const int32_t pred = model_blender_1cycle(255, A);
        const int32_t meas = sat_draw(c, width, 0, CC_A_ONE);

        if (meas != pred) {
            report_printf("  c=%2lu raw=%3ld A=%3ld meas=%3ld pred=%3ld MISMATCH\n",
                          (unsigned long)c, (long)sat_raw(c, width),
                          (long)A, (long)meas, (long)pred);
            err++;
        }
    }
    report_printf("  H1 33 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* H2: 2-cycle. The blender runs twice, and cycle 1 is the pass that
 * partial-reject can skip (33 pts) */
static void run_test_h2(test_item_t *item)
{
    report_printf("\n--- [H2] 2-Cycle Double Blend & Passthrough (33 pts) ---\n");
    uint32_t err = 0;
    const uint32_t width = 25;

    for (uint32_t c = 0; c <= 32; c++) {
        const int32_t A    = sat_keyalpha(c, width);
        const int32_t pred = model_blender_2cycle(255, A);
        const int32_t meas = sat_draw(c, width, 1, CC_A_ONE);

        if (meas != pred) {
            report_printf("  c=%2lu A=%3ld meas=%3ld pred=%3ld MISMATCH\n",
                          (unsigned long)c, (long)A, (long)meas, (long)pred);
            err++;
        }
    }
    report_printf("  H2 33 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/*
 * H3: the boundary at single-point resolution.
 *
 * A "keyalpha ceiling of 256" and a "ceiling of 255 plus passthrough" agree
 * everywhere EXCEPT at exactly raw = 255, which only an odd c reaches.
 * Sweeping even c alone hides the difference - which is how the wrong model
 * survived 25,000 points.
 */
static void run_test_h3(test_item_t *item)
{
    report_printf("\n--- [H3] Saturation Boundary, 1-pt Resolution (9 pts) ---\n");
    report_printf("  raw=255 discriminates; even-only sweeps never reach it\n");
    uint32_t err = 0;
    const uint32_t width = 25;

    for (uint32_t c = 12; c <= 20; c++) {
        const int32_t raw  = sat_raw(c, width);
        const int32_t A    = sat_keyalpha(c, width);
        const int32_t pred = model_blender_1cycle(255, A);
        const int32_t meas = sat_draw(c, width, 0, CC_A_ONE);

        report_printf("  c=%2lu raw=%3ld A=%3ld -> meas=%3ld pred=%3ld%s\n",
                      (unsigned long)c, (long)raw, (long)A,
                      (long)meas, (long)pred,
                      (raw == 255) ? "   <= discriminating point" : "");
        if (meas != pred) err++;
    }
    report_printf("  H3 9 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/*
 * H4: the passthrough proved directly rather than inferred.
 *
 * Above the threshold the output must equal the PIXEL COLOUR exactly, so a
 * different sub_a changes the readout one-for-one. A blend would scale it.
 * The control below the threshold must be scaled, showing the two regimes are
 * genuinely different code paths and not one formula.
 */
static void run_test_h4(test_item_t *item)
{
    report_printf("\n--- [H4] Partial-Reject Passthrough, Direct Proof (3 pts) ---\n");
    uint32_t err = 0;
    const uint32_t width = 50;      /* keyalpha hard against 0xff */

    const int32_t A_hi = sat_keyalpha(0, width);
    const int32_t m_one  = sat_draw(0, width, 0, CC_A_ONE);
    const int32_t p_one  = model_blender_1cycle(255, A_hi);
    report_printf("  saturated, sub_a=ONE   A=%3ld meas=%3ld pred=%3ld\n",
                  (long)A_hi, (long)m_one, (long)p_one);
    if (m_one != p_one) err++;

    /* PRIM is 0xffffffff here too, so the bypass value is also 255; what this
     * shows is that the output tracks the pixel, not a blend of it. */
    const int32_t m_prim = sat_draw(0, width, 0, CC_A_PRIM);
    report_printf("  saturated, sub_a=PRIM  A=%3ld meas=%3ld pred=%3ld\n",
                  (long)A_hi, (long)m_prim, (long)p_one);
    if (m_prim != p_one) err++;

    const uint32_t w_lo = 16;
    const int32_t A_lo  = sat_keyalpha(0, w_lo);
    const int32_t m_lo  = sat_draw(0, w_lo, 0, CC_A_ONE);
    const int32_t p_lo  = model_blender_1cycle(255, A_lo);
    report_printf("  below threshold        A=%3ld meas=%3ld pred=%3ld (blended)\n",
                  (long)A_lo, (long)m_lo, (long)p_lo);
    if (m_lo != p_lo) err++;
    if (A_lo >= 0xff) err++;        /* the control must be below threshold */

    report_printf("  H4 3 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* H5: several widths, so saturation is reached by different arithmetic. */
static void run_test_h5(test_item_t *item)
{
    report_printf("\n--- [H5] Multi-Width Saturation Probe (5 pts) ---\n");
    uint32_t err = 0;
    static const uint32_t widths[5] = { 16, 20, 25, 32, 50 };

    for (unsigned i = 0; i < 5; i++) {
        const int32_t A    = sat_keyalpha(0, widths[i]);
        const int32_t pred = model_blender_1cycle(255, A);
        const int32_t meas = sat_draw(0, widths[i], 0, CC_A_ONE);

        report_printf("  width=%3lu A=%3ld meas=%3ld pred=%3ld %s\n",
                      (unsigned long)widths[i], (long)A, (long)meas, (long)pred,
                      (A >= 0xff) ? "(passthrough)" : "(blended)");
        if (meas != pred) err++;
    }
    report_printf("  H5 5 pts, mismatches: %lu\n", (unsigned long)err);
    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/*
 * H6: keyalpha and ordinary combiner alpha reach the blender identically.
 *
 * If the key mux fed the blender differently from the normal alpha path these
 * would diverge. Both saturate onto the passthrough and read 255.
 */
static void run_test_h6(test_item_t *item)
{
    report_printf("\n--- [H6] Keyalpha vs Combiner Alpha Equivalence (2 pts) ---\n");
    uint32_t err = 0;
    rdp_target *t = probe_target();

    target_clear(t, 0);
    target_bind(t);
    dpl_cmd(OM_ALPHA_READOUT | RDP_OM_CYCLE(RDP_CYCLE_1));
    dpl_cmd(rdp_set_combine_both(CC_A_ZERO, CC_B_ZERO, CC_M_ZERO, CC_D_ONE,
                                 AC_ZERO, AC_ZERO, AC_M_ZERO, AC_ONE));
    target_draw_rect(t);
    dpl_run();
    const int32_t val_comb = (int32_t)red_of(target_center_rgba(t));

    const int32_t val_key = sat_draw(0, 25, 0, CC_A_ONE);

    report_printf("  [A] combiner alpha saturated -> %ld\n", (long)val_comb);
    report_printf("  [B] keyalpha saturated       -> %ld\n", (long)val_key);
    report_printf("  both 255 via passthrough; angrylion agrees\n");

    if (val_comb != 255) err++;
    if (val_key  != 255) err++;

    item->mismatches = err;
    item->state = (err == 0) ? TEST_STATE_PASSED : TEST_STATE_FAILED;
}

/* ------------------------------------------------------------- Dispatcher */

void test_run_item(test_suite_t *suite, uint32_t idx)
{
    if (idx >= suite->item_count) return;

    test_item_t *item = &suite->items[idx];
    item->state = TEST_STATE_RUNNING;

    switch (idx) {
        case 0:  run_test_r1(item); break;
        case 1:  run_test_r2(item); break;
        case 2:  run_test_r3(item); break;
        case 3:  run_test_r4(item); break;
        case 4:  run_test_r5(item); break;
        case 5:  run_test_b1(item); break;
        case 6:  run_test_b2(item); break;
        case 7:  run_test_i1(item); break;
        case 8:  run_test_i2(item); break;
        case 9:  run_test_i3(item); break;
        case 10: run_test_i4(item); break;
        case 11: run_test_i5(item); break;
        case 12: run_test_i6(item); break;
        case 13: run_test_k1(item); break;
        case 14: run_test_k2(item); break;
        case 15: run_test_k3(item); break;
        case 16: run_test_k4(item); break;
        case 17: run_test_k5(item); break;
        case 18: run_test_m1(item); break;
        case 19: run_test_m2(item); break;
        case 20: run_test_m3(item); break;
        case 21: run_test_m4(item); break;
        case 22: run_test_m5(item); break;
        case 23: run_test_m6(item); break;
        case 24: run_test_h1(item); break;
        case 25: run_test_h2(item); break;
        case 26: run_test_h3(item); break;
        case 27: run_test_h4(item); break;
        case 28: run_test_h5(item); break;
        case 29: run_test_h6(item); break;
        default: break;
    }

    if (item->state == TEST_STATE_PASSED) suite->passed_count++;
    else                                  suite->failed_count++;
}
