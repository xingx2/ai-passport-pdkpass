#pragma once

#include "lfc_data.h"
#include "pdkpass_data.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The dashboard is one timeline rather than two separate seasons: every Formula
// 1 round and every Liverpool fixture is a card, and the arrow keys walk them in
// chronological order. This module owns the merge and the landing rule so both
// can be host-tested without LVGL or ESP-IDF (see tests/test_pdkpass_deck.c).

typedef enum {
    PDKPASS_DECK_RACE = 0,  // Formula 1 round; index addresses the season races.
    PDKPASS_DECK_MATCH,     // Liverpool fixture; index addresses the fixture table.
} pdkpass_deck_kind_t;

typedef struct {
    int64_t when_utc;  // Sort key: the round's switch boundary or the kick-off.
    size_t index;
    pdkpass_deck_kind_t kind;
} pdkpass_deck_entry_t;

// A 24-round Formula 1 season next to a 46-fixture season (38 Premier League
// plus 8 Champions League) fits with room to spare.
#define PDKPASS_DECK_MAX 80U

typedef struct {
    pdkpass_deck_entry_t entries[PDKPASS_DECK_MAX];
    size_t count;
} pdkpass_deck_t;

// How long a card that has already started keeps the dashboard before the next
// one takes over. It matches the fixture match window, so a match that kicks off
// is not replaced by the next fixture on the same afternoon. Applying the same
// hold to every card is what keeps the landing rule from skipping a card: a
// round whose boundary falls inside a match window still gets its own turn.
#define PDKPASS_DECK_GRACE_SECONDS (3LL * 60LL * 60LL)

// Merge both schedules into one ascending timeline. The merge is stable, so two
// cards that share a timestamp keep the caller's order (races are inserted
// first). Cards past PDKPASS_DECK_MAX are dropped rather than overrunning.
void pdkpass_deck_build(pdkpass_deck_t *deck,
                        const pdkpass_race_t *races, size_t race_count,
                        const lfc_fixture_t *fixtures, size_t fixture_count);

// Position of the card the dashboard opens on: the first card that has not
// started, unless the card just before it is still inside `grace_seconds`.
// `count` means every card is in the past, so the deck is exhausted.
size_t pdkpass_deck_next(const pdkpass_deck_t *deck, int64_t now_utc,
                         int64_t grace_seconds);
