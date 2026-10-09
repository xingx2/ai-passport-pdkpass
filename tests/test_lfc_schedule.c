#include <assert.h>
#include <string.h>
#include "lfc_data.h"
#include "lfc_schedule.h"

int main(void)
{
    assert(lfc_fixture_count == 38);
    assert(strcmp(lfc_season, "2026-27") == 0);
    for (size_t i = 0; i < lfc_fixture_count; i++) {
        const lfc_fixture_t *f = &lfc_fixtures[i];
        assert(f->round == i + 1U);
        assert(f->event_id != 0U);
        assert(f->opponent[0] != '\0');
        assert(f->venue[0] != '\0');
        assert(strcmp(f->opponent, lfc_team_name) != 0);
        if (i) assert(lfc_fixtures[i - 1U].kickoff_utc < f->kickoff_utc);
        // An unplayed fixture carries no result; only the online refresh or a
        // newer snapshot may set one.
        if (!f->played) assert(f->goals_for == 0U && f->goals_against == 0U);
    }

    // The captured results stay attached to the right side of the fixture.
    assert(lfc_fixtures[0].played && !lfc_fixtures[0].home);
    assert(lfc_fixtures[0].goals_for == 2U && lfc_fixtures[0].goals_against == 2U);
    assert(lfc_fixtures[4].played && !lfc_fixtures[4].home);
    assert(lfc_fixtures[4].goals_for == 1U && lfc_fixtures[4].goals_against == 0U);
    assert(!lfc_fixtures[5].played);

    // Selection uses the match window, not the raw kick-off.
    const int64_t w = LFC_MATCH_WINDOW_SECONDS;
    assert(lfc_next_fixture(0) == 0U);
    assert(lfc_next_fixture(lfc_fixtures[0].kickoff_utc) == 0U);
    assert(lfc_next_fixture(lfc_fixtures[0].kickoff_utc + w - 1) == 0U);
    assert(lfc_next_fixture(lfc_fixtures[0].kickoff_utc + w) == 1U);
    assert(lfc_next_fixture(lfc_fixtures[5].kickoff_utc + w) == 6U);
    assert(lfc_next_fixture(lfc_fixtures[37].kickoff_utc + w) ==
           lfc_fixture_count);

    // Recheck boundaries: the kick-off while the fixture is still ahead, then
    // the earlier of the window end and the next Beijing midnight once it has
    // started. R02 kicks off at 19:30 Beijing, so its window end comes first.
    assert(lfc_next_check(lfc_fixtures[0].kickoff_utc - 3600) ==
           lfc_fixtures[0].kickoff_utc);
    assert(lfc_next_check(lfc_fixtures[1].kickoff_utc + 60) ==
           lfc_fixtures[1].kickoff_utc + w);
    int64_t midweek = lfc_fixtures[5].kickoff_utc - 5 * 86400;
    int64_t check = lfc_next_check(midweek);
    assert(check > midweek);
    assert(check <= lfc_fixtures[5].kickoff_utc);
    return 0;
}
