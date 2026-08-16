#include <libdragon.h>
#include <stdio.h>

#include "harness.h"
#include "tests.h"
#include "ui.h"

int main(void)
{
    debug_init_isviewer();
    debug_init_usblog();
    joypad_init();

    harness_init();
    ui_init();

    const int sd = report_init_sd("sd:/rdpchroma.txt");

    report_printf("===============================================================\n");
    report_printf("  RDP CHROMA KEY TEST ROM - SILICON HARDWARE VERIFICATION      \n");
    report_printf("  640x480 High-Res Suite (30,000+ points across all categories)\n");
    report_printf("  SD Log: %s\n", sd ? "sd:/rdpchroma.txt" : "unavailable (flashcart SD not found)");
    report_printf("===============================================================\n\n");

    test_suite_t suite;
    tests_init_suite(&suite);

    /* Render initial dashboard */
    ui_draw_screen(&suite, 0);

    /* Execute all tests sequentially while updating UI */
    for (uint32_t i = 0; i < suite.item_count; i++) {
        ui_draw_screen(&suite, i);
        test_run_item(&suite, i);
        ui_draw_screen(&suite, i);
    }

    suite.is_done = 1;

    report_printf("\n===============================================================\n");
    report_printf("  TEST EXECUTION COMPLETE: %lu/%lu PASSED (%lu FAILURES)\n",
                  (unsigned long)suite.passed_count, (unsigned long)suite.item_count,
                  (unsigned long)suite.failed_count);
    report_printf("  Total Points Swept: %lu\n", (unsigned long)suite.total_points);
    report_printf("===============================================================\n\n");

    report_close();

    /* Final display refresh loop */
    while (1) {
        ui_draw_screen(&suite, suite.item_count);
        joypad_poll();
    }
}
