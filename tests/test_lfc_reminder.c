#include <assert.h>
#include <stdint.h>
#include "lfc_data.h"
#include "lfc_reminder_core.h"

int main(void)
{
    const int64_t lead = LFC_REMINDER_LEAD_SECONDS;
    const int64_t first = lfc_fixtures[0].kickoff_utc;
    const int64_t sixth = lfc_fixtures[5].kickoff_utc;

    // Gates: the ALERTS switch, a synchronized clock and a trustworthy date all
    // suppress the alert, so a restored estimate can never fire it early.
    assert(lfc_reminder_due_at(first - 600, false, true, 0) == lfc_fixture_count);
    assert(lfc_reminder_due_at(first - 600, true, false, 0) == lfc_fixture_count);
    assert(lfc_reminder_due_at(100, true, true, 0) == lfc_fixture_count);

    // The window opens exactly `lead` seconds before the kick-off.
    assert(lfc_reminder_due_at(first - lead - 1, true, true, 0) == lfc_fixture_count);
    assert(lfc_reminder_due_at(first - lead, true, true, 0) == 0U);
    assert(lfc_reminder_due_at(first - 1, true, true, 0) == 0U);
    // At kick-off the fixture is no longer "upcoming" and is not announced.
    assert(lfc_reminder_due_at(first, true, true, 0) == lfc_fixture_count);
    assert(lfc_reminder_due_at(first + 60, true, true, 0) == lfc_fixture_count);

    // A fixture already announced is skipped, and the next one is far away, so
    // no second alert is produced inside the same window.
    assert(lfc_reminder_due_at(first - lead, true, true, first) == lfc_fixture_count);

    // The snapshot's earlier results are in the past and never alert.
    assert(lfc_reminder_due_at(sixth - 300, true, true, 0) == 5U);

    // Wait: the time until the window opens, then capped at one day.
    assert(lfc_reminder_wait_until(first - 1000, true, true, 0) ==
           (uint32_t)((1000 - lead) * 1000));
    assert(lfc_reminder_wait_until(first - lead, true, true, 0) == 0U);
    assert(lfc_reminder_wait_until(first - 100000, true, true, 0) == 86400000U);
    assert(lfc_reminder_wait_until(first - lead, false, true, 0) == UINT32_MAX);
    assert(lfc_reminder_wait_until(first - lead, true, false, 0) == UINT32_MAX);

    // Once the announced fixture is remembered, the wait jumps to the next one
    // and stays bounded rather than returning zero forever.
    uint32_t after = lfc_reminder_wait_until(first - lead, true, true, first);
    assert(after > 0U && after <= 86400000U);

    // Nothing left to announce at the end of the season.
    assert(lfc_reminder_wait_until(lfc_fixtures[lfc_fixture_count - 1U].kickoff_utc + 1,
                                   true, true, 0) == UINT32_MAX);
    assert(lfc_reminder_due_at(lfc_fixtures[lfc_fixture_count - 1U].kickoff_utc + 1,
                               true, true, 0) == lfc_fixture_count);
    return 0;
}
