#include "lfc_table.h"

#include <string.h>

// One override buffer, not two: publication happens under the BSP LVGL lock and
// every reader holds that same lock, so there is never a concurrent reader to
// double-buffer against.
static lfc_fixture_t s_override[LFC_MAX_FIXTURES];
static const lfc_fixture_t *s_active;
static size_t s_active_count;

const lfc_fixture_t *lfc_table(void)
{
    return s_active ? s_active : lfc_fixtures;
}

size_t lfc_table_count(void)
{
    return s_active ? s_active_count : lfc_fixture_count;
}

bool lfc_table_copy(lfc_fixture_t *out, size_t capacity, size_t *count)
{
    size_t total = lfc_table_count();
    if (!out || total > capacity) return false;
    memcpy(out, lfc_table(), total * sizeof(*out));
    if (count) *count = total;
    return true;
}

bool lfc_table_publish(const lfc_fixture_t *fixtures, size_t count)
{
    if (!fixtures || count == 0U || count > LFC_MAX_FIXTURES) return false;
    memcpy(s_override, fixtures, count * sizeof(*fixtures));
    s_active_count = count;
    s_active = s_override;
    return true;
}
