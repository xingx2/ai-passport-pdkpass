#pragma once

#include "lfc_data.h"
#include <stddef.h>
#include <stdint.h>

// Seconds a match stays featured after kick-off before the home card advances to
// the next fixture (regulation + half-time + stoppage, with slack).
#define LFC_MATCH_WINDOW_SECONDS (3LL * 60LL * 60LL)

// Fixture to feature for `now_utc`: the first match whose window has not yet
// elapsed, or lfc_table_count() once the season is complete.
size_t lfc_next_fixture(int64_t now_utc);

// Earliest moment the featured fixture could change: the current fixture's
// kick-off or window end when still ahead, otherwise the next Beijing midnight.
int64_t lfc_next_check(int64_t now_utc);
