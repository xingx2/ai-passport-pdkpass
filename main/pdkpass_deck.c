#include "pdkpass_deck.h"

void pdkpass_deck_build(pdkpass_deck_t *deck,
                        const pdkpass_race_t *races, size_t race_count,
                        const lfc_fixture_t *fixtures, size_t fixture_count)
{
    if (!deck) return;
    deck->count = 0U;
    // Races first: on a shared timestamp the stable sort below keeps them ahead
    // of a fixture kicking off at the same moment.
    for (size_t i = 0; i < race_count && deck->count < PDKPASS_DECK_MAX; i++) {
        deck->entries[deck->count].when_utc = races[i].switch_at_utc;
        deck->entries[deck->count].index = i;
        deck->entries[deck->count].kind = PDKPASS_DECK_RACE;
        deck->count++;
    }
    for (size_t i = 0; i < fixture_count && deck->count < PDKPASS_DECK_MAX; i++) {
        deck->entries[deck->count].when_utc = fixtures[i].kickoff_utc;
        deck->entries[deck->count].index = i;
        deck->entries[deck->count].kind = PDKPASS_DECK_MATCH;
        deck->count++;
    }
    // Stable insertion sort by time. Both sources arrive in order, so only the
    // fixtures have to travel and the pass costs about one comparison per card.
    for (size_t i = 1U; i < deck->count; i++) {
        pdkpass_deck_entry_t key = deck->entries[i];
        size_t j = i;
        while (j > 0U && deck->entries[j - 1U].when_utc > key.when_utc) {
            deck->entries[j] = deck->entries[j - 1U];
            j--;
        }
        deck->entries[j] = key;
    }
}

size_t pdkpass_deck_next(const pdkpass_deck_t *deck, int64_t now_utc,
                         int64_t grace_seconds)
{
    if (!deck || deck->count == 0U) return 0U;
    size_t index = deck->count;  // First card that has not started yet.
    for (size_t i = 0U; i < deck->count; i++) {
        if (deck->entries[i].when_utc >= now_utc) {
            index = i;
            break;
        }
    }
    if (index == 0U) return 0U;
    // `index - 1` is the card that started most recently: either the last card
    // of an exhausted deck, or the one just before the next card up. It is the
    // only candidate still holding the dashboard, and only for its grace.
    size_t previous = index - 1U;
    if (now_utc < deck->entries[previous].when_utc + grace_seconds)
        return previous;
    return index;
}
