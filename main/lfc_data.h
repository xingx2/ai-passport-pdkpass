#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Liverpool FC support for the PDKPASS firmware. The names here are specific to
// this club module and do not depend on ESP-IDF or LVGL, so host tests can
// compile them directly (see tests/test_lfc_schedule.c).

#define LFC_MAX_FIXTURES 48
#define LFC_OPPONENT_LEN 24
#define LFC_VENUE_LEN 34

// Matchdays in each competition. The dashboard counter reads "n / total" per
// competition, so a Premier League card never borrows the Champions League
// extent (or the other way round).
#define LFC_PL_MATCHDAYS 38U
#define LFC_CL_MATCHDAYS 8U

// Which competition a fixture belongs to. The dashboard merges both into one
// timeline, but the card and the detail page name the competition, and the
// `round` field means something different in each.
typedef enum {
    LFC_COMP_PL = 0,  // Premier League; `round` is the matchday, 1..38.
    LFC_COMP_CL = 1,  // UEFA Champions League; `round` is the league-phase
                      // matchday, 1..8.
} lfc_competition_t;

// One Liverpool fixture. Scores are only meaningful while `played` is set; the
// bundled snapshot records a result only after ESPN reports the match complete,
// and the online refresh replaces the snapshot in place.
typedef struct {
    uint32_t event_id;              // ESPN event id, stable across refreshes.
    int64_t kickoff_utc;            // Kick-off, UTC epoch seconds.
    uint8_t round;                  // Matchday (see lfc_competition_t).
    bool home;                      // true when Liverpool plays at Anfield.
    bool played;                    // Match finished; goals_for/against are valid.
    uint8_t goals_for;              // Liverpool goals (valid when `played`).
    uint8_t goals_against;          // Opponent goals (valid when `played`).
    char opponent[LFC_OPPONENT_LEN];
    char venue[LFC_VENUE_LEN];
    // Last so a positional initializer that omits it stays Premier League
    // (the enum's zero value), keeping the bundled PL rows untouched.
    lfc_competition_t competition;
} lfc_fixture_t;

extern const lfc_fixture_t lfc_fixtures[];
extern const size_t lfc_fixture_count;
extern const char lfc_season[];     // "2026-27"
extern const char lfc_team_name[];  // "LIVERPOOL"
