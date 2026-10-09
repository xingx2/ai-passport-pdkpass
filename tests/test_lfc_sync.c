// Host test for the Liverpool fixture refresh. Every payload below keeps ESPN's
// real key order and nesting (captured from
// sports.core.api.espn.com/v2/sports/soccer/leagues/eng.1) with the long "$ref"
// URLs shortened: the parser never reads them, and the surrounding structure is
// what the scanners have to navigate.
#include "lfc_sync_core.h"

#include <assert.h>
#include <string.h>

// Bournemouth 0-1 Liverpool, 2026-09-20T13:00Z. Home side first, as ESPN emits.
static const char COMPETITORS[] =
    "{\"count\":2,\"pageIndex\":1,\"pageSize\":25,\"pageCount\":1,\"items\":["
    "{\"$ref\":\".../competitors/349\",\"id\":\"349\",\"uid\":\"s:600~t:349\","
    "\"type\":\"team\",\"order\":0,\"homeAway\":\"home\",\"winner\":false,"
    "\"team\":{\"$ref\":\".../seasons/2026/teams/349\"},"
    "\"score\":{\"$ref\":\".../competitors/349/score\"},"
    "\"linescores\":{\"$ref\":\".../competitors/349/linescores\"},"
    "\"record\":{\"$ref\":\".../competitors/349/records\"},"
    "\"uniform\":{\"type\":\"home\",\"color\":\"ff0000\"}},"
    "{\"$ref\":\".../competitors/364\",\"id\":\"364\",\"uid\":\"s:600~t:364\","
    "\"type\":\"team\",\"order\":1,\"homeAway\":\"away\",\"winner\":true,"
    "\"team\":{\"$ref\":\".../seasons/2026/teams/364\"},"
    "\"score\":{\"$ref\":\".../competitors/364/score\"},"
    "\"linescores\":{\"$ref\":\".../competitors/364/linescores\"},"
    "\"record\":{\"$ref\":\".../competitors/364/records\"},"
    "\"uniform\":{\"type\":\"away\",\"color\":\"aa0118\"}}"
    "]}";

// An event document: the match's own "date" is the first date in the document.
static const char EVENT[] =
    "{\"$ref\":\".../events/401879276\",\"id\":\"401879276\","
    "\"uid\":\"s:600~l:700~e:401879276\",\"date\":\"2026-09-20T13:00Z\","
    "\"name\":\"Liverpool at AFC Bournemouth\",\"shortName\":\"LIV @ BOU\","
    "\"season\":{\"$ref\":\".../seasons/2026\"},"
    "\"competitions\":[{\"$ref\":\".../competitions/401879276\"}]}";

static const char STATUS_FULL_TIME[] =
    "{\"$ref\":\".../competitions/401879276/status\",\"clock\":5400.0,"
    "\"addedClock\":300.0,\"displayClock\":\"90'+5'\",\"period\":2,"
    "\"type\":{\"id\":\"28\",\"name\":\"STATUS_FULL_TIME\",\"state\":\"post\","
    "\"completed\":true,\"description\":\"Full Time\",\"detail\":\"FT\","
    "\"shortDetail\":\"FT\"}}";

static const char STATUS_SCHEDULED[] =
    "{\"$ref\":\".../competitions/401879276/status\",\"clock\":0.0,"
    "\"displayClock\":\"0'\",\"period\":0,"
    "\"type\":{\"id\":\"1\",\"name\":\"STATUS_SCHEDULED\",\"state\":\"pre\","
    "\"completed\":false,\"description\":\"Scheduled\",\"detail\":\"Sun, "
    "October 11th at 9:00 AM EDT\",\"shortDetail\":\"Scheduled\"}}";

static const char SCORE_LIVERPOOL_ONE[] =
    "{\"$ref\":\".../competitors/364/scores/38\",\"value\":1.0,"
    "\"displayValue\":\"1\",\"winner\":true,"
    "\"source\":{\"id\":\"38\",\"description\":\"SA.ENVOY\"}}";

static const char SCORE_HOME_ZERO[] =
    "{\"$ref\":\".../competitors/349/scores/38\",\"value\":0.0,"
    "\"displayValue\":\"0\",\"winner\":false,"
    "\"source\":{\"id\":\"38\",\"description\":\"SA.ENVOY\"}}";

static lfc_fixture_t fixture(uint32_t id, int64_t kickoff, bool home)
{
    lfc_fixture_t f;
    memset(&f, 0, sizeof(f));
    f.event_id = id;
    f.kickoff_utc = kickoff;
    f.home = home;
    return f;
}

static void test_iso8601(void)
{
    int64_t epoch = -1;
    assert(lfc_sync_iso8601("2026-09-20T13:00Z", 17, &epoch));
    assert(epoch == 1789909200LL);
    assert(lfc_sync_iso8601("1970-01-01T00:00Z", 16, &epoch) && epoch == 0);
    assert(lfc_sync_iso8601("2027-01-02T19:45Z", 17, &epoch) && epoch == 1798919100LL);
    // Seconds are optional and a lower-case separator is tolerated.
    assert(lfc_sync_iso8601("2026-09-20T13:00:30Z", 20, &epoch) &&
           epoch == 1789909230LL);
    assert(lfc_sync_iso8601("2026-09-20t13:00Z", 17, &epoch) &&
           epoch == 1789909200LL);
    // Truncated and impossible values are rejected rather than guessed.
    assert(!lfc_sync_iso8601("2026-09-20", 10, &epoch));
    assert(!lfc_sync_iso8601("2026-13-40T13:00Z", 17, &epoch));
    assert(!lfc_sync_iso8601("2026-09-20X13:00Z", 17, &epoch));
}

static void test_parse_date(void)
{
    int64_t kickoff = 0;
    assert(lfc_sync_parse_date(EVENT, sizeof(EVENT) - 1U, &kickoff));
    assert(kickoff == 1789909200LL);
    // An error page or a truncated body must not invent a kick-off.
    assert(!lfc_sync_parse_date("{\"message\":\"Not Found\"}", 23U, &kickoff));
    assert(!lfc_sync_parse_date("", 0U, &kickoff));
}

static void test_parse_competitors(void)
{
    uint32_t home = 9U, away = 9U;
    assert(lfc_sync_parse_competitors(COMPETITORS, sizeof(COMPETITORS) - 1U,
                                      &home, &away));
    assert(home == 349U && away == 364U);

    // A page missing its away side reports nothing rather than a half answer.
    const char *single =
        "{\"count\":1,\"items\":[{\"id\":\"349\",\"homeAway\":\"home\"}]}";
    assert(!lfc_sync_parse_competitors(single, strlen(single), &home, &away));
    assert(home == 0U && away == 0U);
    assert(!lfc_sync_parse_competitors("{\"count\":0}", 11U, &home, &away));
}

static void test_parse_status(void)
{
    bool completed = false;
    assert(lfc_sync_parse_status(STATUS_FULL_TIME, sizeof(STATUS_FULL_TIME) - 1U,
                                 &completed));
    assert(completed);
    assert(lfc_sync_parse_status(STATUS_SCHEDULED, sizeof(STATUS_SCHEDULED) - 1U,
                                 &completed));
    assert(!completed);
    assert(!lfc_sync_parse_status("{\"clock\":0}", 10U, &completed));
}

static void test_parse_score(void)
{
    int goals = -1;
    assert(lfc_sync_parse_score(SCORE_LIVERPOOL_ONE,
                                sizeof(SCORE_LIVERPOOL_ONE) - 1U, &goals));
    assert(goals == 1);
    assert(lfc_sync_parse_score(SCORE_HOME_ZERO, sizeof(SCORE_HOME_ZERO) - 1U,
                                &goals));
    assert(goals == 0);
    // "displayValue" can be blank on an unplayed match; "value" backs it up.
    const char *blank = "{\"value\":2.0,\"displayValue\":\"\"}";
    assert(lfc_sync_parse_score(blank, strlen(blank), &goals) && goals == 2);
    assert(!lfc_sync_parse_score("{\"winner\":true}", 15U, &goals));
}

static void test_apply_result(void)
{
    lfc_fixture_t fixtures[] = {
        fixture(401879273U, 1789909200LL, false), // unrelated, must stay put
        fixture(401879276U, 1789909200LL, false), // Liverpool away
        fixture(401879279U, 1789130400LL, true),  // Liverpool home
    };
    size_t count = sizeof(fixtures) / sizeof(fixtures[0]);

    lfc_sync_event_t event = {.event_id = 401879276U,
                              .completed = true,
                              .kickoff_utc = 1789909200LL,
                              .home_goals = 0,
                              .away_goals = 1};
    assert(lfc_sync_apply(fixtures, count, &event));
    assert(fixtures[1].played);
    assert(fixtures[1].goals_for == 1U && fixtures[1].goals_against == 0U);
    assert(fixtures[1].kickoff_utc == 1789909200LL);
    // Untouched rows keep their bundled state.
    assert(!fixtures[0].played && !fixtures[2].played);
    // Re-applying an unchanged result reports no work, so the caller can skip
    // the flash write.
    assert(!lfc_sync_apply(fixtures, count, &event));

    // The same scoreline seen from the other side flips the goals.
    lfc_fixture_t home = fixture(401879276U, 1789909200LL, true);
    assert(lfc_sync_apply(&home, 1U, &event));
    assert(home.played && home.goals_for == 0U && home.goals_against == 1U);
    assert(!lfc_sync_apply(&home, 1U, &event));

    // An id the table does not carry is ignored.
    lfc_sync_event_t stranger = event;
    stranger.event_id = 999999U;
    assert(!lfc_sync_apply(fixtures, count, &stranger));
}

static void test_apply_reschedule(void)
{
    lfc_fixture_t fixtures[] = {fixture(401879276U, 1789909200LL, false)};
    // A kick-off that moved without a result yet: time only, never "played".
    lfc_sync_event_t moved = {.event_id = 401879276U,
                              .completed = false,
                              .kickoff_utc = 1790017200LL, // 2026-09-21T19:00Z
                              .home_goals = -1,
                              .away_goals = -1};
    assert(lfc_sync_apply(fixtures, 1U, &moved));
    assert(fixtures[0].kickoff_utc == 1790017200LL);
    assert(!fixtures[0].played);
    assert(!lfc_sync_apply(fixtures, 1U, &moved));

    // Scores reporting "not started" must not mark the match played.
    lfc_sync_event_t pending = {.event_id = 401879276U,
                                .completed = false,
                                .kickoff_utc = 1790017200LL,
                                .home_goals = 0,
                                .away_goals = 0};
    assert(!lfc_sync_apply(fixtures, 1U, &pending));
    assert(!fixtures[0].played);
}

int main(void)
{
    test_iso8601();
    test_parse_date();
    test_parse_competitors();
    test_parse_status();
    test_parse_score();
    test_apply_result();
    test_apply_reschedule();
    return 0;
}
