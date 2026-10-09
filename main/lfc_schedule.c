#include "lfc_schedule.h"

#include "lfc_table.h"

#define SECONDS_PER_DAY 86400LL
#define BEIJING_UTC_OFFSET 28800LL

size_t lfc_next_fixture(int64_t now_utc)
{
    const lfc_fixture_t *fixtures = lfc_table();
    size_t count = lfc_table_count();
    for (size_t i = 0; i < count; i++) {
        if (now_utc < fixtures[i].kickoff_utc + LFC_MATCH_WINDOW_SECONDS)
            return i;
    }
    return count;
}

int64_t lfc_next_check(int64_t now_utc)
{
    int64_t beijing_day = (now_utc + BEIJING_UTC_OFFSET) / SECONDS_PER_DAY;
    int64_t next_midnight = (beijing_day + 1) * SECONDS_PER_DAY -
                            BEIJING_UTC_OFFSET;
    int64_t best = next_midnight;
    const lfc_fixture_t *fixtures = lfc_table();
    size_t count = lfc_table_count();
    size_t fixture = lfc_next_fixture(now_utc);
    if (fixture < count) {
        int64_t kick = fixtures[fixture].kickoff_utc;
        int64_t end = kick + LFC_MATCH_WINDOW_SECONDS;
        // A completed fixture only offers its remaining window; a future one
        // also offers its kick-off, so the card can flip to "in progress".
        if (kick > now_utc && kick < best) best = kick;
        if (end > now_utc && end < best) best = end;
    }
    return best;
}
