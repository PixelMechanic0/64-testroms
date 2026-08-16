#ifndef RDPCHROMA_UI_H
#define RDPCHROMA_UI_H

#include <stdint.h>
#include <libdragon.h>

#define SCREEN_W 640
#define SCREEN_H 480

typedef enum {
    TEST_STATE_PENDING = 0,
    TEST_STATE_RUNNING,
    TEST_STATE_PASSED,
    TEST_STATE_FAILED
} test_state_t;

typedef struct {
    const char  *id;
    const char  *name;
    const char  *evidence;
    uint32_t     points;
    test_state_t state;
    uint32_t     mismatches;
    int32_t      divergent_val;
    char         summary[64];
} test_item_t;

typedef struct {
    const char  *title;
    uint32_t     header_color;
    uint32_t     test_start_idx;
    uint32_t     test_count;
} test_category_t;

typedef struct {
    test_item_t     *items;
    uint32_t         item_count;
    test_category_t *categories;
    uint32_t         category_count;
    uint32_t         total_points;
    uint32_t         passed_count;
    uint32_t         failed_count;
    int              is_done;
    uint64_t         elapsed_ticks;
} test_suite_t;

void ui_init(void);
void ui_draw_screen(const test_suite_t *suite, uint32_t current_running_idx);

#endif /* RDPCHROMA_UI_H */
