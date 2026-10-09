#include "pdkpass_model.h"

static size_t wrap_previous(size_t value, size_t count)
{
    return count == 0 ? 0 : (value + count - 1) % count;
}

static size_t wrap_next(size_t value, size_t count)
{
    return count == 0 ? 0 : (value + 1) % count;
}

static pdkpass_session_kind_t visible_session(uint32_t mask,
                                               size_t start, bool previous)
{
    for (size_t i = 0; i < PDKPASS_SESSION_COUNT; i++) {
        size_t kind = previous
            ? (start + PDKPASS_SESSION_COUNT - i) % PDKPASS_SESSION_COUNT
            : (start + i) % PDKPASS_SESSION_COUNT;
        if (mask & (1U << kind)) return (pdkpass_session_kind_t)kind;
    }
    return PDKPASS_SESSION_COUNT;
}

void pdkpass_state_set_sessions(pdkpass_state_t *state, uint32_t mask)
{
    state->session_mask = mask & ((1U << PDKPASS_SESSION_COUNT) - 1U);
    state->selected_session = visible_session(state->session_mask,
                                               state->selected_session, false);
}

size_t pdkpass_state_detail_index(const pdkpass_state_t *state)
{
    for (size_t i = 0; i < state->detail_count; i++)
        if (state->detail_sessions[i] == state->detail_selected_session) return i;
    return state->detail_count;
}

void pdkpass_state_set_detail_sessions(pdkpass_state_t *state, uint32_t mask,
    const pdkpass_detail_session_t sessions[PDKPASS_SESSION_COUNT],
    bool choose_default, bool time_valid, int64_t now_utc)
{
    state->detail_count = 0;
    for (unsigned kind = 0; kind < PDKPASS_SESSION_COUNT; kind++)
        if (mask & (1U << kind))
            state->detail_sessions[state->detail_count++] = (pdkpass_session_kind_t)kind;
    // Only compare known times. Missing times retain their conventional slots
    // rather than inventing a date or a duration to sort them.
    for (size_t i = 0; i < state->detail_count; i++) {
        for (size_t j = i + 1; j < state->detail_count; j++) {
            pdkpass_session_kind_t a = state->detail_sessions[i];
            pdkpass_session_kind_t b = state->detail_sessions[j];
            if (sessions[a].start_utc > 0 && sessions[b].start_utc > 0 &&
                sessions[a].start_utc > sessions[b].start_utc) {
                state->detail_sessions[i] = b;
                state->detail_sessions[j] = a;
            }
        }
    }
    if (!choose_default && pdkpass_state_detail_index(state) < state->detail_count)
        return;

    pdkpass_session_kind_t completed = PDKPASS_SESSION_COUNT;
    pdkpass_session_kind_t upcoming = PDKPASS_SESSION_COUNT;
    pdkpass_session_kind_t fallback = PDKPASS_SESSION_COUNT;
    for (size_t i = 0; i < state->detail_count; i++) {
        pdkpass_session_kind_t kind = state->detail_sessions[i];
        const pdkpass_detail_session_t *session = &sessions[kind];
        if (session->cancelled) continue;
        if (fallback == PDKPASS_SESSION_COUNT) fallback = kind;
        bool ended = time_valid && session->end_utc > 0 && session->end_utc <= now_utc;
        if (ended || session->result_ready) {
            // Prefer actual end times. Legacy result caches can prove completion
            // without preserving an end time; use their schedule order then.
            if (completed == PDKPASS_SESSION_COUNT || session->end_utc == 0 ||
                sessions[completed].end_utc == 0 ||
                session->end_utc >= sessions[completed].end_utc)
                completed = kind;
        } else if (time_valid && session->start_utc >= now_utc &&
                   session->start_utc > 0 &&
                   (upcoming == PDKPASS_SESSION_COUNT ||
                    session->start_utc < sessions[upcoming].start_utc)) {
            upcoming = kind;
        }
    }
    state->detail_selected_session = completed != PDKPASS_SESSION_COUNT ? completed :
        upcoming != PDKPASS_SESSION_COUNT ? upcoming : fallback;
    if (state->detail_selected_session == PDKPASS_SESSION_COUNT && state->detail_count)
        state->detail_selected_session = state->detail_sessions[0];
}

void pdkpass_state_init(pdkpass_state_t *state)
{
    state->page = PDKPASS_PAGE_HOME;
    state->detail_origin = PDKPASS_PAGE_HOME;
    state->selected_race = 0;
    state->selected_driver = 0;
    state->selected_team = 0;
    state->selected_session = PDKPASS_SESSION_FP1;
    state->session_mask = (1U << PDKPASS_SESSION_COUNT) - 1U;
    state->detail_count = 0;
    state->detail_selected_session = PDKPASS_SESSION_COUNT;
    for (unsigned i = 0; i < PDKPASS_SESSION_COUNT; i++)
        state->detail_sessions[i] = PDKPASS_SESSION_COUNT;
    state->home_card = 0;
    state->home_browsing = false;
    state->deck_complete = false;
    state->network_selection = 0;
}

void pdkpass_state_set_home_card(pdkpass_state_t *state, size_t card,
                                 size_t card_count)
{
    state->deck_complete = card_count > 0U && card >= card_count;
    state->home_card = state->deck_complete
                           ? (card_count > 0U ? card_count - 1U : 0U)
                           : card;
    state->home_browsing = false;
}

void pdkpass_state_reset_home(pdkpass_state_t *state)
{
    state->home_browsing = false;
}

// Pure navigation state machine. UI rendering and hardware access intentionally
// stay out of this function so every button path can be covered by host tests.
void pdkpass_state_handle(pdkpass_state_t *state, pdkpass_input_t input,
                          size_t race_count, size_t driver_count,
                          size_t team_count, size_t card_count)
{
    switch (state->page) {
    case PDKPASS_PAGE_HOME:
        if (input == PDKPASS_INPUT_BACK) state->page = PDKPASS_PAGE_NETWORK;
        if (input == PDKPASS_INPUT_UP_LONG) state->page = PDKPASS_PAGE_STANDINGS;
        if (input == PDKPASS_INPUT_DOWN_LONG)
            state->page = PDKPASS_PAGE_TEAM_STANDINGS;
        if ((input == PDKPASS_INPUT_UP || input == PDKPASS_INPUT_DOWN) &&
            card_count > 0U) {
            // One flat walk over the merged timeline; the Formula 1 rounds and
            // the Liverpool fixtures wrap together exactly like the rounds did.
            size_t current = state->deck_complete && !state->home_browsing
                                 ? card_count - 1U : state->home_card;
            state->home_card = input == PDKPASS_INPUT_UP
                                   ? wrap_previous(current, card_count)
                                   : wrap_next(current, card_count);
            state->home_browsing = true;
        }
        // OK opens the round detail. A Liverpool card is intercepted by the UI,
        // which owns the fixture selection, so it never reaches this branch.
        if (input == PDKPASS_INPUT_OK && card_count > 0U &&
            (!state->deck_complete || state->home_browsing)) {
            state->detail_origin = PDKPASS_PAGE_HOME;
            state->page = PDKPASS_PAGE_RACE_DETAIL;
        }
        break;

    case PDKPASS_PAGE_NETWORK:
        if (input == PDKPASS_INPUT_UP)
            state->network_selection =
                (state->network_selection + PDKPASS_NETWORK_ITEMS - 1U) % PDKPASS_NETWORK_ITEMS;
        if (input == PDKPASS_INPUT_DOWN)
            state->network_selection = (state->network_selection + 1U) % PDKPASS_NETWORK_ITEMS;
        if (input == PDKPASS_INPUT_BACK) state->page = PDKPASS_PAGE_HOME;
        if (input == PDKPASS_INPUT_OK &&
            state->network_selection == PDKPASS_NETWORK_ITEMS - 1U)
            state->page = PDKPASS_PAGE_HOME;
        break;
    case PDKPASS_PAGE_NETWORK_CONFIRM:
    case PDKPASS_PAGE_NETWORK_PROGRESS:
        if (input == PDKPASS_INPUT_BACK) state->page = PDKPASS_PAGE_NETWORK;
        break;
    case PDKPASS_PAGE_CALENDAR:
        if (input == PDKPASS_INPUT_UP) {
            state->selected_race = wrap_previous(state->selected_race, race_count);
        } else if (input == PDKPASS_INPUT_DOWN) {
            state->selected_race = wrap_next(state->selected_race, race_count);
        } else if (input == PDKPASS_INPUT_OK && race_count > 0) {
            state->detail_origin = PDKPASS_PAGE_CALENDAR;
            state->page = PDKPASS_PAGE_RACE_DETAIL;
        } else if (input == PDKPASS_INPUT_BACK) {
            state->page = PDKPASS_PAGE_HOME;
        }
        break;

    case PDKPASS_PAGE_STANDINGS:
        if (input == PDKPASS_INPUT_UP) {
            state->selected_driver = wrap_previous(state->selected_driver, driver_count);
        } else if (input == PDKPASS_INPUT_DOWN) {
            state->selected_driver = wrap_next(state->selected_driver, driver_count);
        } else if (input == PDKPASS_INPUT_BACK) {
            state->page = PDKPASS_PAGE_HOME;
        }
        break;

    case PDKPASS_PAGE_TEAM_STANDINGS:
        if (input == PDKPASS_INPUT_UP)
            state->selected_team = wrap_previous(state->selected_team, team_count);
        else if (input == PDKPASS_INPUT_DOWN)
            state->selected_team = wrap_next(state->selected_team, team_count);
        else if (input == PDKPASS_INPUT_BACK)
            state->page = PDKPASS_PAGE_HOME;
        break;

    case PDKPASS_PAGE_RACE_DETAIL:
        if (input == PDKPASS_INPUT_UP || input == PDKPASS_INPUT_DOWN) {
            size_t index = pdkpass_state_detail_index(state);
            if (input == PDKPASS_INPUT_UP && index > 0 && index < state->detail_count)
                state->detail_selected_session = state->detail_sessions[index - 1];
            if (input == PDKPASS_INPUT_DOWN && index + 1 < state->detail_count)
                state->detail_selected_session = state->detail_sessions[index + 1];
        } else if (input == PDKPASS_INPUT_OK && state->detail_count) {
            state->selected_session = state->detail_selected_session;
            state->page = PDKPASS_PAGE_RESULTS;
        } else if (input == PDKPASS_INPUT_BACK) {
            state->page = state->detail_origin;
        }
        break;

    case PDKPASS_PAGE_RESULTS:
        if (input == PDKPASS_INPUT_UP) {
            state->selected_session = visible_session(state->session_mask,
                wrap_previous(state->selected_session, PDKPASS_SESSION_COUNT), true);
        } else if (input == PDKPASS_INPUT_DOWN) {
            state->selected_session = visible_session(state->session_mask,
                state->selected_session < PDKPASS_SESSION_COUNT
                    ? wrap_next(state->selected_session, PDKPASS_SESSION_COUNT) : 0,
                false);
        } else if (input == PDKPASS_INPUT_BACK) {
            state->page = PDKPASS_PAGE_RACE_DETAIL;
        }
        break;

    case PDKPASS_PAGE_LFC_DETAIL:
        // The fixture detail page is self-contained: its selection lives in the
        // UI layer and lfc_handle_input() dispatches before this function runs,
        // so the model stores no extra state and keeps the F1 round untouched.
        break;
    }
    // Coming back to the dashboard keeps the card the user was on, exactly as
    // the round detail round-trip did before the deck was merged. The live event
    // is reclaimed only when the screen sleeps (pdkpass_state_reset_home) or a
    // refreshed season snapshot arrives.
}
