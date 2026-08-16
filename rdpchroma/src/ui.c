#include <stdio.h>
#include <string.h>

#include "ui.h"

#define COLOR32(r, g, b, a) \
    (((uint32_t)(r) << 24) | ((uint32_t)(g) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(a))

#define C_BG           COLOR32(16, 20, 28, 255)
#define C_HEADER_BG    COLOR32(22, 28, 38, 255)
#define C_CARD_BG      COLOR32(20, 25, 34, 255)
#define C_CARD_BORDER  COLOR32(38, 48, 64, 255)
#define C_CARD_DIVIDER COLOR32(32, 40, 54, 255)

#define C_TEXT_WHITE   COLOR32(240, 244, 248, 255)
#define C_TEXT_LABEL   COLOR32(200, 210, 222, 255)
#define C_TEXT_MUTED   COLOR32(120, 136, 156, 255)
#define C_ACCENT_CYAN  COLOR32(0, 229, 255, 255)
#define C_ACCENT_BLUE  COLOR32(64, 169, 255, 255)
#define C_ACCENT_GOLD  COLOR32(255, 214, 0, 255)

#define C_PASS_TEXT    COLOR32(0, 230, 118, 255)
#define C_PASS_BG      COLOR32(12, 44, 28, 255)
#define C_PASS_BORDER  COLOR32(0, 200, 83, 255)

#define C_FAIL_TEXT    COLOR32(255, 82, 82, 255)
#define C_FAIL_BG      COLOR32(48, 16, 20, 255)
#define C_FAIL_BORDER  COLOR32(213, 0, 0, 255)

#define C_RUN_TEXT     COLOR32(255, 214, 0, 255)
#define C_RUN_BG       COLOR32(44, 38, 12, 255)
#define C_RUN_BORDER   COLOR32(255, 171, 0, 255)

#define C_PEND_TEXT    COLOR32(100, 115, 130, 255)
#define C_PEND_BG      COLOR32(24, 28, 36, 255)
#define C_PEND_BORDER  COLOR32(45, 55, 70, 255)

void ui_init(void)
{
    display_init(RESOLUTION_640x480, DEPTH_32_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
}

static void draw_box_outline(surface_t *surf, int x, int y, int w, int h,
                             uint32_t fill_color, uint32_t border_color)
{
    graphics_draw_box(surf, x, y, w, h, fill_color);
    graphics_draw_line(surf, x,         y,         x + w - 1, y,         border_color);
    graphics_draw_line(surf, x,         y + h - 1, x + w - 1, y + h - 1, border_color);
    graphics_draw_line(surf, x,         y,         x,         y + h - 1, border_color);
    graphics_draw_line(surf, x + w - 1, y,         x + w - 1, y + h - 1, border_color);
}

static void draw_card(surface_t *surf, int x, int y, int w, int h,
                      const char *title, uint32_t header_color)
{
    draw_box_outline(surf, x, y, w, h, C_CARD_BG, C_CARD_BORDER);

    graphics_draw_box(surf, x + 1, y + 1, w - 2, 17, C_HEADER_BG);
    graphics_draw_line(surf, x, y + 18, x + w - 1, y + 18, C_CARD_DIVIDER);

    graphics_set_color(header_color, 0);
    graphics_draw_text(surf, x + 8, y + 5, title);
}

static void draw_badge(surface_t *surf, int x, int y, test_state_t state)
{
    const char *text = "PEND";
    uint32_t fg = C_PEND_TEXT;
    uint32_t bg = C_PEND_BG;
    uint32_t bd = C_PEND_BORDER;

    switch (state) {
        case TEST_STATE_PASSED:
            text = "PASS";
            fg = C_PASS_TEXT;
            bg = C_PASS_BG;
            bd = C_PASS_BORDER;
            break;
        case TEST_STATE_FAILED:
            text = "FAIL";
            fg = C_FAIL_TEXT;
            bg = C_FAIL_BG;
            bd = C_FAIL_BORDER;
            break;
        case TEST_STATE_RUNNING:
            text = "RUN ";
            fg = C_RUN_TEXT;
            bg = C_RUN_BG;
            bd = C_RUN_BORDER;
            break;
        default:
            break;
    }

    draw_box_outline(surf, x, y, 36, 12, bg, bd);
    graphics_set_color(fg, 0);
    graphics_draw_text(surf, x + 3, y + 2, text);
}

static void draw_test_item(surface_t *surf, int x, int y, int w, const test_item_t *item)
{
    draw_badge(surf, x + 6, y, item->state);

    graphics_set_color(C_TEXT_LABEL, 0);
    graphics_draw_text(surf, x + 48, y + 2, item->name);

    if (item->state == TEST_STATE_PASSED) {
        char buf[32];
        if (item->points >= 1000)
            snprintf(buf, sizeof(buf), "%luk pts", (unsigned long)(item->points / 1000));
        else if (item->points > 1)
            snprintf(buf, sizeof(buf), "%lu pts", (unsigned long)item->points);
        else
            snprintf(buf, sizeof(buf), "MATCH");

        graphics_set_color(C_PASS_TEXT, 0);
        graphics_draw_text(surf, x + w - 64, y + 2, buf);
    } else if (item->state == TEST_STATE_FAILED) {
        char buf[32];
        snprintf(buf, sizeof(buf), "ERR:%lu", (unsigned long)item->mismatches);
        graphics_set_color(C_FAIL_TEXT, 0);
        graphics_draw_text(surf, x + w - 64, y + 2, buf);
    } else if (item->state == TEST_STATE_RUNNING) {
        graphics_set_color(C_RUN_TEXT, 0);
        graphics_draw_text(surf, x + w - 64, y + 2, "TESTING");
    } else {
        graphics_set_color(C_TEXT_MUTED, 0);
        graphics_draw_text(surf, x + w - 64, y + 2, "READY");
    }
}

void ui_draw_screen(const test_suite_t *suite, uint32_t current_running_idx)
{
    surface_t *surf = display_get();

    /* Clear screen */
    graphics_draw_box(surf, 0, 0, SCREEN_W, SCREEN_H, C_BG);

    /* ------------------------------------------------------------- Header */
    draw_box_outline(surf, 8, 8, SCREEN_W - 16, 40, C_HEADER_BG, C_CARD_BORDER);
    graphics_draw_line(surf, 8, 47, SCREEN_W - 8, 47, C_ACCENT_CYAN);

    graphics_set_color(C_TEXT_WHITE, 0);
    graphics_draw_text(surf, 18, 14, "N64 RDP CHROMA KEY HARDWARE VERIFICATION SUITE");

    graphics_set_color(C_TEXT_MUTED, 0);
    graphics_draw_text(surf, 18, 28, "640x480 High-Res Silicon Conformance & 0x100 Saturation Analysis (30 Tests)");

    char mode_buf[48];
    if (suite->is_done) {
        if (suite->failed_count == 0) {
            snprintf(mode_buf, sizeof(mode_buf), "30/30 PASS (100%% SILICON MATCH)");
            graphics_set_color(C_PASS_TEXT, 0);
        } else {
            snprintf(mode_buf, sizeof(mode_buf), "%lu FAILURES DETECTED", (unsigned long)suite->failed_count);
            graphics_set_color(C_FAIL_TEXT, 0);
        }
    } else {
        snprintf(mode_buf, sizeof(mode_buf), "RUNNING [%lu/%lu]",
                 (unsigned long)current_running_idx + 1, (unsigned long)suite->item_count);
        graphics_set_color(C_ACCENT_GOLD, 0);
    }
    graphics_draw_text(surf, SCREEN_W - 240, 20, mode_buf);

    /* -------------------------------------------------------- Cards Grid */
    const int col_w = 308;
    const int left_x = 8;
    const int right_x = 324;

    /* Category 0: Response Function Arithmetic (5 items) */
    {
        const test_category_t *cat = &suite->categories[0];
        const int cy = 56;
        const int ch = 116;
        draw_card(surf, left_x, cy, col_w, ch, cat->title, cat->header_color);
        for (uint32_t i = 0; i < cat->test_count; i++) {
            draw_test_item(surf, left_x, cy + 22 + (int)i * 18, col_w,
                           &suite->items[cat->test_start_idx + i]);
        }
    }

    /* Category 1: Chromabypass & YUV (2 items) */
    {
        const test_category_t *cat = &suite->categories[1];
        const int cy = 180;
        const int ch = 62;
        draw_card(surf, left_x, cy, col_w, ch, cat->title, cat->header_color);
        for (uint32_t i = 0; i < cat->test_count; i++) {
            draw_test_item(surf, left_x, cy + 22 + (int)i * 18, col_w,
                           &suite->items[cat->test_start_idx + i]);
        }
    }

    /* Category 2: Integration Layer (6 items) */
    {
        const test_category_t *cat = &suite->categories[2];
        const int cy = 250;
        const int ch = 134;
        draw_card(surf, left_x, cy, col_w, ch, cat->title, cat->header_color);
        for (uint32_t i = 0; i < cat->test_count; i++) {
            draw_test_item(surf, left_x, cy + 22 + (int)i * 18, col_w,
                           &suite->items[cat->test_start_idx + i]);
        }
    }

    /* Category 3: Key Registers in Combiner (5 items) */
    {
        const test_category_t *cat = &suite->categories[3];
        const int cy = 56;
        const int ch = 116;
        draw_card(surf, right_x, cy, col_w, ch, cat->title, cat->header_color);
        for (uint32_t i = 0; i < cat->test_count; i++) {
            draw_test_item(surf, right_x, cy + 22 + (int)i * 18, col_w,
                           &suite->items[cat->test_start_idx + i]);
        }
    }

    /* Category 4: Designed Idiom, Modes & Formats (6 items) */
    {
        const test_category_t *cat = &suite->categories[4];
        const int cy = 180;
        const int ch = 134;
        draw_card(surf, right_x, cy, col_w, ch, cat->title, cat->header_color);
        for (uint32_t i = 0; i < cat->test_count; i++) {
            draw_test_item(surf, right_x, cy + 22 + (int)i * 18, col_w,
                           &suite->items[cat->test_start_idx + i]);
        }
    }

    /* Category 5: Silicon 0x100 Ceiling Bug Analysis (6 items H1..H6) */
    {
        const test_category_t *cat = &suite->categories[5];
        const int cy = 322;
        const int ch = 134;
        draw_card(surf, right_x, cy, col_w, ch, cat->title, cat->header_color);
        for (uint32_t i = 0; i < cat->test_count; i++) {
            draw_test_item(surf, right_x, cy + 22 + (int)i * 18, col_w,
                           &suite->items[cat->test_start_idx + i]);
        }
    }

    display_show(surf);
}
