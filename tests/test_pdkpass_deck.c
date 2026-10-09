#include <assert.h>
#include <string.h>

#include "pdkpass_deck.h"

static pdkpass_race_t race_at(int64_t when, uint8_t round)
{
    pdkpass_race_t race;
    memset(&race, 0, sizeof(race));
    race.switch_at_utc = when;
    race.round = round;
    return race;
}

static lfc_fixture_t match_at(int64_t kickoff, uint8_t round)
{
    lfc_fixture_t fixture;
    memset(&fixture, 0, sizeof(fixture));
    fixture.kickoff_utc = kickoff;
    fixture.round = round;
    return fixture;
}

// Interleaving is by time, and each entry still points back at its own source.
static void test_merge_order(void)
{
    pdkpass_race_t races[] = {race_at(100, 1), race_at(300, 2), race_at(500, 3)};
    lfc_fixture_t fixtures[] = {match_at(50, 1), match_at(200, 2),
                                match_at(400, 3), match_at(600, 4)};
    pdkpass_deck_t deck;
    pdkpass_deck_build(&deck, races, 3U, fixtures, 4U);
    assert(deck.count == 7U);

    const pdkpass_deck_kind_t kinds[] = {
        PDKPASS_DECK_MATCH, PDKPASS_DECK_RACE, PDKPASS_DECK_MATCH,
        PDKPASS_DECK_RACE, PDKPASS_DECK_MATCH, PDKPASS_DECK_RACE,
        PDKPASS_DECK_MATCH,
    };
    const size_t indices[] = {0U, 0U, 1U, 1U, 2U, 2U, 3U};
    for (size_t i = 0; i < deck.count; i++) {
        assert(deck.entries[i].kind == kinds[i]);
        assert(deck.entries[i].index == indices[i]);
    }
    assert(deck.entries[0].when_utc == 50);
    assert(deck.entries[6].when_utc == 600);
}

// A round and a fixture on the same timestamp keep the round in front, so a race
// weekend never hides a match kicking off beside it.
static void test_shared_timestamp_prefers_race(void)
{
    pdkpass_race_t races[] = {race_at(200, 7)};
    lfc_fixture_t fixtures[] = {match_at(200, 9)};
    pdkpass_deck_t deck;
    pdkpass_deck_build(&deck, races, 1U, fixtures, 1U);
    assert(deck.count == 2U);
    assert(deck.entries[0].kind == PDKPASS_DECK_RACE && deck.entries[0].index == 0U);
    assert(deck.entries[1].kind == PDKPASS_DECK_MATCH && deck.entries[1].index == 0U);
}

// A season without fixtures still drives the former Formula 1-only dashboard,
// and an empty deck never overruns the cursor.
static void test_single_source_and_empty(void)
{
    pdkpass_race_t races[] = {race_at(10, 1), race_at(20, 2)};
    pdkpass_deck_t deck;
    pdkpass_deck_build(&deck, races, 2U, NULL, 0U);
    assert(deck.count == 2U);
    assert(deck.entries[0].kind == PDKPASS_DECK_RACE);
    assert(deck.entries[1].when_utc == 20);

    pdkpass_deck_build(&deck, NULL, 0U, NULL, 0U);
    assert(deck.count == 0U);
    assert(pdkpass_deck_next(&deck, 0, 1) == 0U);
    assert(pdkpass_deck_next(NULL, 0, 1) == 0U);
}

// An oversized season is truncated instead of writing past the array.
static void test_capacity_is_bounded(void)
{
    pdkpass_race_t races[PDKPASS_DECK_MAX + 8U];
    for (size_t i = 0; i < sizeof(races) / sizeof(races[0]); i++)
        races[i] = race_at((int64_t)i * 10, (uint8_t)(i + 1U));
    lfc_fixture_t fixtures[8];
    for (size_t i = 0; i < 8U; i++) fixtures[i] = match_at((int64_t)i * 10 + 5, 1);
    pdkpass_deck_t deck;
    pdkpass_deck_build(&deck, races, sizeof(races) / sizeof(races[0]), fixtures, 8U);
    assert(deck.count == PDKPASS_DECK_MAX);
    for (size_t i = 1U; i < deck.count; i++)
        assert(deck.entries[i - 1U].when_utc <= deck.entries[i].when_utc);
}

// The landing card is the next one to start, with the card that just started
// holding on for the grace window. A short grace keeps the boundaries visible.
static void test_next_card(void)
{
    pdkpass_race_t races[] = {race_at(1000, 1), race_at(2000, 2)};
    lfc_fixture_t fixtures[] = {match_at(100, 1), match_at(500, 2)};
    pdkpass_deck_t deck;
    pdkpass_deck_build(&deck, races, 2U, fixtures, 2U);
    assert(deck.count == 4U);
    const int64_t grace = 10;

    // Before anything has started the first card is the one to show.
    assert(pdkpass_deck_next(&deck, 0, grace) == 0U);
    assert(pdkpass_deck_next(&deck, 99, grace) == 0U);
    // It starts exactly on its own timestamp...
    assert(pdkpass_deck_next(&deck, 100, grace) == 0U);
    // ...and holds the dashboard until its window runs out.
    assert(pdkpass_deck_next(&deck, 100 + grace - 1, grace) == 0U);
    assert(pdkpass_deck_next(&deck, 100 + grace, grace) == 1U);
    // Mid-season the landing simply tracks the next kick-off.
    assert(pdkpass_deck_next(&deck, 700, grace) == 2U);
    assert(pdkpass_deck_next(&deck, 999, grace) == 2U);
    assert(pdkpass_deck_next(&deck, 1000, grace) == 2U);
    assert(pdkpass_deck_next(&deck, 1000 + grace - 1, grace) == 2U);
    assert(pdkpass_deck_next(&deck, 1000 + grace, grace) == 3U);
    // Exhausted: every card is in the past, so the caller sees `count`.
    assert(pdkpass_deck_next(&deck, 2000 + grace, grace) == deck.count);
    assert(pdkpass_deck_next(&deck, 99999, grace) == deck.count);
}

// A race boundary landing inside a live match window must still get its own turn
// once the match has been shown. This is the case a naive "first card that has
// not expired" rule gets wrong: it would jump past the round entirely.
static void test_grace_never_skips_a_card(void)
{
    const int64_t grace = PDKPASS_DECK_GRACE_SECONDS;
    pdkpass_race_t races[] = {race_at(1000, 1)};
    lfc_fixture_t fixtures[] = {match_at(800, 1)};  // Kicks off inside the hold.
    pdkpass_deck_t deck;
    pdkpass_deck_build(&deck, races, 1U, fixtures, 1U);
    assert(deck.count == 2U);

    // Without a hold the round takes over the moment it starts.
    assert(pdkpass_deck_next(&deck, 1000, 0) == 1U);
    // With the hold the match keeps the dashboard first...
    assert(pdkpass_deck_next(&deck, 1000, grace) == 0U);
    // ...but the round is still shown for its own window afterwards.
    assert(pdkpass_deck_next(&deck, 800 + grace, grace) == 1U);
    assert(pdkpass_deck_next(&deck, 1000 + grace, grace) == deck.count);
}

int main(void)
{
    test_merge_order();
    test_shared_timestamp_prefers_race();
    test_single_source_and_empty();
    test_capacity_is_bounded();
    test_next_card();
    test_grace_never_skips_a_card();
    return 0;
}
