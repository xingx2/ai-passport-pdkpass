#pragma once

#include "lfc_data.h"
#include <stdbool.h>
#include <stddef.h>

// The live Liverpool fixture table. It starts as the bundled snapshot compiled
// into lfc_data.c and is replaced whenever a refresh brings newer results or a
// moved kick-off. Kept in its own translation unit, free of ESP-IDF, so the
// schedule and reminder cores stay host-testable.

const lfc_fixture_t *lfc_table(void);
size_t lfc_table_count(void);

// Copy the live table into `out`. False when `capacity` cannot hold it.
bool lfc_table_copy(lfc_fixture_t *out, size_t capacity, size_t *count);

// Replace the live table. The caller must hold the BSP LVGL lock: every UI
// reader holds it too, so a rendered card never sees a half-written row.
bool lfc_table_publish(const lfc_fixture_t *fixtures, size_t count);
