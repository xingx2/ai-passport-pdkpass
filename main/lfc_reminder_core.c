#include "lfc_reminder_core.h"

#include "lfc_table.h"

// Same lower bound the Formula 1 reminder core uses: a clock earlier than the
// 2026 season is not trusted to place a match.
#define LFC_MIN_VALID_UTC 1767225600LL
#define LFC_WAIT_CAP_SECONDS 86400LL

size_t lfc_reminder_due_at(int64_t now_utc, bool alerts_enabled, bool time_valid,
                           int64_t last_alerted_kickoff)
{
    if (!alerts_enabled || !time_valid || now_utc < LFC_MIN_VALID_UTC)
        return lfc_table_count();
    const lfc_fixture_t *fixtures = lfc_table();
    size_t count = lfc_table_count();
    for (size_t i = 0; i < count; i++) {
        int64_t kick = fixtures[i].kickoff_utc;
        if (kick <= now_utc || kick == last_alerted_kickoff) continue;
        // Fixtures are stored in kick-off order, so the first future match is
        // also the earliest; only that one can be inside the lead window.
        return kick - now_utc <= LFC_REMINDER_LEAD_SECONDS ? i : count;
    }
    return count;
}

uint32_t lfc_reminder_wait_until(int64_t now_utc, bool alerts_enabled,
                                 bool time_valid, int64_t last_alerted_kickoff)
{
    if (!alerts_enabled || !time_valid || now_utc < LFC_MIN_VALID_UTC)
        return UINT32_MAX;
    const lfc_fixture_t *fixtures = lfc_table();
    size_t count = lfc_table_count();
    for (size_t i = 0; i < count; i++) {
        int64_t kick = fixtures[i].kickoff_utc;
        if (kick <= now_utc || kick == last_alerted_kickoff) continue;
        int64_t seconds = kick - LFC_REMINDER_LEAD_SECONDS - now_utc;
        if (seconds <= 0) return 0;
        if (seconds > LFC_WAIT_CAP_SECONDS) seconds = LFC_WAIT_CAP_SECONDS;
        return (uint32_t)seconds * 1000U;
    }
    return UINT32_MAX;
}
