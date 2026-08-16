#ifndef RDPCHROMA_TESTS_H
#define RDPCHROMA_TESTS_H

#include "ui.h"

extern test_item_t g_test_items[];
extern const uint32_t g_test_item_count;

extern test_category_t g_test_categories[];
extern const uint32_t g_test_category_count;

void tests_init_suite(test_suite_t *suite);
void test_run_item(test_suite_t *suite, uint32_t idx);

#endif /* RDPCHROMA_TESTS_H */
