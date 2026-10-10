#include <assert.h>
#include <string.h>
#include "lfc_data.h"
#include "lfc_schedule.h"

int main(void)
{
    // The snapshot now spans both competitions: 38 Premier League matchdays
    // plus 8 Champions League league-phase matches, merged by kick-off.
    assert(lfc_fixture_count == 46);
    assert(strcmp(lfc_season, "2026-27") == 0);

    unsigned pl_rounds[LFC_PL_MATCHDAYS] = {0};
    unsigned cl_rounds[LFC_CL_MATCHDAYS] = {0};
    for (size_t i = 0; i < lfc_fixture_count; i++) {
        const lfc_fixture_t *f = &lfc_fixtures[i];
        assert(f->event_id != 0U);
        assert(f->opponent[0] != '\0');
        assert(f->venue[0] != '\0');
        assert(strcmp(f->opponent, lfc_team_name) != 0);
        if (i) assert(lfc_fixtures[i - 1U].kickoff_utc < f->kickoff_utc);
        // An unplayed fixture carries no result; only the online refresh or a
        // newer snapshot may set one.
        if (!f->played) assert(f->goals_for == 0U && f->goals_against == 0U);
        // Each matchday appears exactly once within its own competition.
        if (f->competition == LFC_COMP_CL) {
            assert(f->round >= 1U && f->round <= LFC_CL_MATCHDAYS);
            cl_rounds[f->round - 1U]++;
        } else {
            assert(f->competition == LFC_COMP_PL);
            assert(f->round >= 1U && f->round <= LFC_PL_MATCHDAYS);
            pl_rounds[f->round - 1U]++;
        }
    }
    for (unsigned r = 0; r < LFC_PL_MATCHDAYS; r++) assert(pl_rounds[r] == 1U);
    for (unsigned r = 0; r < LFC_CL_MATCHDAYS; r++) assert(cl_rounds[r] == 1U);

    // The captured results stay attached to the right side of the fixture.
    // R1 was Newcastle away (2-2); the Champions League opener was Atletico
    // Madrid at Anfield (2-1).
    assert(lfc_fixtures[0].played && !lfc_fixtures[0].home);
    assert(lfc_fixtures[0].goals_for == 2U && lfc_fixtures[0].goals_against == 2U);
    assert(lfc_fixtures[3].competition == LFC_COMP_CL &&
           lfc_fixtures[3].played && lfc_fixtures[3].home);
    assert(lfc_fixtures[3].goals_for == 2U && lfc_fixtures[3].goals_against == 1U);

    // Selection uses the match window, not the raw kick-off. R1 and R2 are
    // played and both ahead of the first Champions League fixture, so the
    // "next" fixture walks the merged timeline by kick-off.
    const int64_t w = LFC_MATCH_WINDOW_SECONDS;
    assert(lfc_next_fixture(0) == 0U);
    assert(lfc_next_fixture(lfc_fixtures[0].kickoff_utc) == 0U);
    assert(lfc_next_fixture(lfc_fixtures[0].kickoff_utc + w - 1) == 0U);
    assert(lfc_next_fixture(lfc_fixtures[0].kickoff_utc + w) == 1U);
    // After the last fixture's window closes the season is complete.
    assert(lfc_next_fixture(
               lfc_fixtures[lfc_fixture_count - 1U].kickoff_utc + w) ==
           lfc_fixture_count);

    // Recheck boundaries: the kick-off while the fixture is still ahead, then
    // the earlier of the window end and the next Beijing midnight once it has
    // started.
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
