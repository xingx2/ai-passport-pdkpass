#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Liverpool FC support for the PDKPASS firmware. The names here are specific to
// this club module and do not depend on ESP-IDF or LVGL, so host tests can
// compile them directly (see tests/test_lfc_schedule.c).

#define LFC_MAX_FIXTURES 40
#define LFC_OPPONENT_LEN 24
#define LFC_VENUE_LEN 34

// One Premier League fixture. Scores are only meaningful while `played` is set;
// the bundled snapshot records a result only after ESPN reports the match
// complete, and the online refresh replaces the snapshot in place.
typedef struct {
    uint32_t event_id;              // ESPN event id, stable across refreshes.
    int64_t kickoff_utc;            // Kick-off, UTC epoch seconds.
    uint8_t round;                  // Premier League matchday, 1..LFC_MAX_FIXTURES.
    bool home;                      // true when Liverpool plays at Anfield.
    bool played;                    // Match finished; goals_for/against are valid.
    uint8_t goals_for;              // Liverpool goals (valid when `played`).
    uint8_t goals_against;          // Opponent goals (valid when `played`).
    char opponent[LFC_OPPONENT_LEN];
    char venue[LFC_VENUE_LEN];
} lfc_fixture_t;

extern const lfc_fixture_t lfc_fixtures[];
extern const size_t lfc_fixture_count;
extern const char lfc_season[];     // "2026-27"
extern const char lfc_team_name[];  // "LIVERPOOL"
