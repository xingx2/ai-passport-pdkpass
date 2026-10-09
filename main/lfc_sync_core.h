#pragma once

// Liverpool fixture refresh logic, kept free of ESP-IDF and LVGL so the host
// tests can run it against real ESPN documents (tests/test_lfc_sync.c).
//
// The device never downloads a whole season. ESPN's "site" scoreboard and
// "summary" documents run to tens of kilobytes and the firmware's HTTP client
// has no gzip support, so the refresh uses the core API's per-resource
// documents instead: a few hundred bytes each, and every allocation stays
// inside the budget the rest of the firmware already works within.

#include "lfc_data.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// One Liverpool fixture as reported by ESPN.
typedef struct {
    uint32_t event_id;
    bool completed;       // ESPN reports the match finished.
    int64_t kickoff_utc;  // 0 when unknown; a positive value replaces the table.
    int home_goals;       // -1 when unknown.
    int away_goals;
} lfc_sync_event_t;

// ESPN core API readers. Each returns false when its field is absent, so a
// truncated or unexpected body can never masquerade as a real result.
//
// parse_date        "2026-09-20T13:00Z" out of an event document.
// parse_competitors both team ids and their sides out of the competitors page.
// parse_status      the "completed" flag out of a competition status document.
// parse_score       the goal count out of a competitor score document.
bool lfc_sync_parse_date(const char *json, size_t length, int64_t *kickoff_utc);
bool lfc_sync_parse_competitors(const char *json, size_t length,
                                uint32_t *home_team, uint32_t *away_team);
bool lfc_sync_parse_status(const char *json, size_t length, bool *completed);
bool lfc_sync_parse_score(const char *json, size_t length, int *goals);

// Fold a parsed event into the fixture table, matching on `event_id`.
// A completed match may set `played` and the goals; a moved kick-off is applied
// whenever the payload carries one. False means the table already matched, so
// the caller can skip the flash write.
bool lfc_sync_apply(lfc_fixture_t *fixtures, size_t count,
                    const lfc_sync_event_t *event);

// ESPN emits "2026-10-11T13:00Z" (seconds optional). Exposed for the tests.
bool lfc_sync_iso8601(const char *text, size_t length, int64_t *epoch);
