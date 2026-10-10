// Liverpool views. Included by pdkpass_ui.c so the renderers reuse the same
// label helpers, palette and status bar as the Formula 1 dashboard. The theme
// of every view is the colour of the ground the fixture is played at: Anfield
// stays Liverpool red, away fixtures take the host club's signature colour.

// Fixture shown on the detail page. It is opened straight from the dashboard
// card, so there is no season list in between to keep in step.
static size_t s_lfc_selected;

// Long values keep their face and break between words rather than being squeezed
// with a transform. A scaled label is drawn through a render layer of its own,
// and the window between the label and the refresh is the trap: the detail page
// asked for `Coventry Building Society Arena` as one 328 x 30 ARGB8888 buffer,
// 39 KB in a single piece, which is more than a 64 KB pool can be relied on to
// hand over once the page around it is up. The refresh has no way back from that
// -- it retries the same allocation for ever instead of settling -- so the label
// simply stops asking, and breaks the text instead. A plain label needs no layer
// at all.
//
// The break is measured here rather than delegated to LVGL's long-mode wrapper:
// a wrapping label left the same refresh spinning, and the measured break lands
// in the same place either way. Returns the number of lines produced.
static int split_words(const char *text, const lv_font_t *font, int available,
                       char *output, size_t capacity)
{
    char probe[LFC_VENUE_LEN];
    lv_point_t size;
    size_t used = 0;
    size_t line_start = 0;
    int lines = 1;
    output[0] = '\0';

    for (size_t i = 0; text[i] != '\0';) {
        size_t word_end = i;
        while (text[word_end] != '\0' && text[word_end] != ' ') word_end++;

        // The candidate is the run from the start of this line through this
        // word, so it is a prefix of the input: it cannot outgrow `probe` and
        // the measurement never sees a truncated word.
        size_t length = word_end - line_start;
        if (length > sizeof(probe) - 1U) length = sizeof(probe) - 1U;
        memcpy(probe, text + line_start, length);
        probe[length] = '\0';
        lv_text_get_size(&size, probe, font, 0, 0, LV_COORD_MAX,
                         LV_TEXT_FLAG_NONE);

        if (line_start != i && size.x > available) {
            // Keep the word whole: close the line here, start the next one. The
            // separator space before this word belongs to neither line, so the
            // closed line stops short of it -- a trailing space would shift a
            // centred name off balance for no visible reason.
            size_t close = i;
            while (close > line_start && text[close - 1U] == ' ') close--;
            if (capacity - used < close - line_start + 2U) break;
            memcpy(output + used, text + line_start, close - line_start);
            used += close - line_start;
            output[used++] = '\n';
            lines++;
            line_start = i;
        }
        i = text[word_end] == '\0' ? word_end : word_end + 1;
    }

    const size_t tail = strlen(text + line_start);
    if (tail < capacity - used) {
        memcpy(output + used, text + line_start, tail + 1U);
    }
    return lines;
}

// The opponent line, shared by the dashboard card and the fixture detail page.
// Club names run from "Fulham" to "Brighton & Hove Albion", and the row leaves
// 204 px for the name, which holds twelve characters of the 16-pixel face. Every
// club keeps that face -- shrinking a longer name into the narrower body face
// reads as a different size class next to the double-height `PL Rn` and
// `HOME`/`AWAY` rows -- and a name that overruns one line breaks onto a second
// one between words, still centred. Two lines hold every Premier League club and
// clear whatever sits below.
static void render_fixture_opponent(const char *name, int y, uint32_t ink)
{
    char text[LFC_OPPONENT_LEN + 4];
    split_words(name, &lv_font_unscii_16, INNER_W - 6, text, sizeof(text));
    make_center_label(s_content, text, 0, y, INNER_W, &lv_font_unscii_16, ink);
}

static const char *lfc_month(unsigned month)
{
    static const char *months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    return month < 12U ? months[month] : "---";
}

// Beijing wall-clock breakdown for a fixture kick-off (UTC epoch seconds).
static void lfc_beijing_parts(int64_t kickoff_utc, struct tm *parts)
{
    time_t local = (time_t)(kickoff_utc + BEIJING_OFFSET_SECONDS);
    gmtime_r(&local, parts);
}

// Status text mirrors the Formula 1 dashboard: network word plus the Beijing
// date, prefixed with `~` while the clock is only the NVS-restored estimate.
static void lfc_set_status(uint32_t background)
{
    char text[32];
    if (s_time_valid || s_time_estimated) {
        struct tm parts;
        time_t local = (time_t)((int64_t)time(NULL) + BEIJING_OFFSET_SECONDS);
        gmtime_r(&local, &parts);
        snprintf(text, sizeof(text), "%s | %s%02d.%02d", network_word(),
                 s_time_valid ? "" : "~", parts.tm_mon + 1, parts.tm_mday);
    } else {
        snprintf(text, sizeof(text), "%s | --.--", network_word());
    }
    set_status(text, s_network_state == PDKPASS_NETWORK_OFFLINE ? UI_RED
                                                                : background);
}

// One dashboard card for a Liverpool fixture, laid out exactly like the Formula
// 1 card: matchday and side, the opponent, then the result once the match is
// complete or the Beijing kick-off while it is still ahead. `position` and
// `total` count within the Premier League season alone, so this card reads
// "6 / 38" instead of borrowing the merged deck's extent.
static void render_home_match(size_t index, size_t position, size_t total)
{
    if (index >= lfc_table_count()) return;
    const lfc_fixture_t *fixture = &lfc_table()[index];
    pdkpass_theme_t theme = lfc_fixture_theme(fixture);
    uint32_t top_ink = contrast_color(theme.top);
    uint32_t base_ink = contrast_color(theme.bottom);
    set_title("LIVERPOOL");
    lfc_set_status(theme.top);
    ui_pixel_screen_set_theme(s_screen, theme.top, theme.bottom);
    content_reset(theme.top, theme.bottom);

    char round[10];
    snprintf(round, sizeof(round), "PL R%u", fixture->round);
    make_zoom_label(s_content, round, 0, 3, INNER_W, top_ink);
    make_fit_zoom_label(s_content, fixture->home ? "HOME" : "AWAY", 0, 35,
                        INNER_W, top_ink);
    render_fixture_opponent(fixture->opponent, 73, top_ink);

    lv_obj_t *card = make_card(s_content, 8, 113, 194, 27, theme.bottom, 3);
    char line[20];
    if (fixture->played) {
        snprintf(line, sizeof(line), "%u - %u", fixture->goals_for,
                 fixture->goals_against);
    } else {
        struct tm parts;
        lfc_beijing_parts(fixture->kickoff_utc, &parts);
        snprintf(line, sizeof(line), "%02d %s %02d:%02d", parts.tm_mday,
                 lfc_month((unsigned)parts.tm_mon), parts.tm_hour,
                 parts.tm_min);
    }
    snprintf(s_home_session_text, sizeof(s_home_session_text), "%s", line);
    make_medium_label(card, line, 0, -2, 188, base_ink);

    char page[20];
    snprintf(page, sizeof(page), "%u / %u", (unsigned)(position + 1U),
             (unsigned)total);
    make_center_label(s_content, page, 0, 146, INNER_W, &lv_font_unscii_16,
                      base_ink);
    make_progress(position, total);
    set_hint("UP/DN:CARD OK:MATCH");
}

// Fixture detail, opened directly from the dashboard card. Up/down step through
// the season without going back through the dashboard.
static void render_lfc_detail(void)
{
    if (lfc_table_count() == 0U) return;
    if (s_lfc_selected >= lfc_table_count()) s_lfc_selected = lfc_table_count() - 1U;
    const lfc_fixture_t *fixture = &lfc_table()[s_lfc_selected];
    pdkpass_theme_t theme = lfc_fixture_theme(fixture);
    uint32_t top_ink = contrast_color(theme.top);
    uint32_t base_ink = contrast_color(theme.bottom);
    char title[12];
    snprintf(title, sizeof(title), "PL R%02u", fixture->round);
    set_title(title);
    lfc_set_status(theme.top);
    ui_pixel_screen_set_theme(s_screen, theme.top, theme.bottom);
    content_reset(theme.top, theme.bottom);

    make_center_label(s_content, fixture->home ? "HOME" : "AWAY", 0, 6,
                      INNER_W, &lv_font_unscii_8, top_ink);
    // The same rule as the dashboard card: every club keeps the 16-pixel face
    // and wraps onto a second line rather than shrinking. Fitting the name with
    // a transform made short clubs double size and long ones small, so `Arsenal`
    // and `Crystal Palace` read as two different ranks of information on the
    // same page.
    render_fixture_opponent(fixture->opponent, 24, top_ink);
    // The venue follows the same rule. `Coventry Building Society Arena` is the
    // longest ground in the season at 31 characters against the 21 the body face
    // fits, and it is the one that used to be squeezed into a 39 KB layer; a
    // second line costs 14 px instead, so the block moves up to keep the kick-off
    // card clear and every other ground keeps the row it has always had.
    char venue[LFC_VENUE_LEN + 4];
    const int venue_lines = split_words(fixture->venue, &pdkpass_body_font,
                                        INNER_W, venue, sizeof(venue));
    make_label(s_content, venue, 0, venue_lines > 1 ? 64 : 78, INNER_W,
               &pdkpass_body_font, top_ink);

    struct tm parts;
    lfc_beijing_parts(fixture->kickoff_utc, &parts);
    char kickoff[28];
    snprintf(kickoff, sizeof(kickoff), "%02d %s %02d:%02d CST", parts.tm_mday,
             lfc_month((unsigned)parts.tm_mon), parts.tm_hour, parts.tm_min);
    lv_obj_t *when = make_card(s_content, 1, 100, 208, 26, theme.bottom, 3);
    make_medium_label(when, kickoff, 0, -2, 202, base_ink);

    lv_obj_t *state = make_card(s_content, 1, 131, 208, 46, UI_PAPER, 3);
    if (fixture->played) {
        char score[24];
        snprintf(score, sizeof(score), "LIVERPOOL %u - %u", fixture->goals_for,
                 fixture->goals_against);
        make_center_label(state, "FULL TIME", 0, 6, 202, &lv_font_unscii_8,
                          UI_INK);
        make_center_label(state, score, 0, 25, 202, &lv_font_unscii_8, UI_INK);
    } else {
        make_center_label(state, "KICK-OFF", 0, 6, 202, &lv_font_unscii_8,
                          UI_INK);
        make_center_label(state, "BEIJING TIME", 0, 25, 202, &lv_font_unscii_8,
                          UI_INK);
    }
    // Always advertise the refresh, exactly like the Formula 1 detail pages
    // advertise their sync: hiding it whenever the clock was still settling
    // made the key look absent rather than unavailable, and the press reports
    // its own outcome in the footer below.
    set_hint("UP/DN OK:REFRESH HOLD:BACK");
}

// Footer feedback for the manual refresh, driven by the same 250 ms tick as the
// Formula 1 manual sync. The word replaces the key hints for a moment after each
// request so a press is never silent; the generation counter keeps a repeat of
// the same word visible.
static unsigned s_lfc_refresh_generation;
static uint32_t s_lfc_refresh_notice_until;

static void lfc_manual_hint(void)
{
    if (s_state.page != PDKPASS_PAGE_LFC_DETAIL) return;
    lfc_refresh_status_t status;
    if (!lfc_sync_refresh_status(&status)) return;
    const char *word = NULL;
    switch (status.state) {
    case LFC_REFRESH_UNAVAILABLE: word = "NEED WIFI + CLOCK"; break;
    case LFC_REFRESH_RUNNING: word = "REFRESHING..."; break;
    case LFC_REFRESH_UPDATED: word = "FIXTURES UPDATED"; break;
    case LFC_REFRESH_UNCHANGED: word = "NO NEW DATA"; break;
    case LFC_REFRESH_FAILED: word = "REFRESH FAILED"; break;
    default: return;
    }
    if (status.generation != s_lfc_refresh_generation) {
        s_lfc_refresh_generation = status.generation;
        s_lfc_refresh_notice_until = status.state == LFC_REFRESH_RUNNING
            ? 0U : lv_tick_get() + 3000U;
    }
    if (status.state == LFC_REFRESH_RUNNING ||
        sync_notice_active(s_lfc_refresh_notice_until)) {
        set_hint(word);
        return;
    }
    set_hint("UP/DN OK:REFRESH HOLD:BACK");
}

// Button handling for the fixture detail page. The dashboard card opens it and
// the model keeps no Liverpool state, so the whole page lives here; returning to
// the dashboard hands it back to the live event, exactly as a round detail does.
// Returns true when the selection or the page changed, so the caller repaints.
static bool lfc_handle_input(pdkpass_input_t input)
{
    if (lfc_table_count() == 0U) return false;
    if (s_state.page != PDKPASS_PAGE_LFC_DETAIL) return false;
    size_t previous_selection = s_lfc_selected;
    pdkpass_page_t previous_page = s_state.page;

    if (input == PDKPASS_INPUT_UP) {
        s_lfc_selected =
            (s_lfc_selected + lfc_table_count() - 1U) % lfc_table_count();
    } else if (input == PDKPASS_INPUT_DOWN) {
        s_lfc_selected = (s_lfc_selected + 1U) % lfc_table_count();
    } else if (input == PDKPASS_INPUT_OK) {
        // Ask the fixture service for a fresh result. It re-reads the next
        // unplayed kick-off and any finished match, then repaints this page
        // through pdkpass_ui_lfc_update(). Offline the request waits for
        // connectivity, so the card is simply left as it is.
        lfc_sync_request();
    } else if (input == PDKPASS_INPUT_BACK) {
        // Keep the card the user was on, exactly like the round detail.
        s_state.page = PDKPASS_PAGE_HOME;
    } else {
        return false;
    }
    return s_lfc_selected != previous_selection ||
           s_state.page != previous_page;
}

// Full-screen kick-off notice. It mirrors the Formula 1 race alert layout and
// shares its layer, countdown timer and dismissal, but takes the Liverpool red
// accent so the two alerts stay visually distinct. Only the fixture index is
// needed: the snapshot already carries the opponent, venue and kick-off.
static void render_lfc_reminder(void)
{
    if (s_lfc_reminder_index >= lfc_table_count()) return;
    const lfc_fixture_t *fixture = &lfc_table()[s_lfc_reminder_index];
    pdkpass_theme_t theme = lfc_fixture_theme(fixture);
    uint32_t background = theme.top;
    uint32_t panel_color = theme.bottom;
    // At Anfield the page background is already Liverpool red, so the middle
    // card is lightened to keep the countdown readable against it.
    uint32_t card_color = background == LFC_RED ? color_mix(LFC_RED, UI_PAPER, 55)
                                                : LFC_RED;

    // Free the page content before the full-screen layer, as the F1 alert does.
    content_reset(theme.top, theme.bottom);
    if (s_reminder_layer) lv_obj_delete(s_reminder_layer);
    s_reminder_layer = make_block(s_screen, 0, 0, 240, 320, background);
    set_gradient(s_reminder_layer, background, theme.bottom);

    make_block(s_reminder_layer, 4, 4, 232, 48, UI_INK);
    make_block(s_reminder_layer, 6, 7, 228, 44, background);
    make_block(s_reminder_layer, 10, 11, 220, 36, UI_RED);
    reminder_label(s_reminder_layer, "MATCH ALERT", 29, 204,
                   &lv_font_unscii_16, 384, UI_PAPER);

    reminder_label(s_reminder_layer, fixture->opponent, 75, 206,
                   &lv_font_unscii_16, 448, readable_text_color(background));
    char line[40];
    snprintf(line, sizeof(line), "MATCHDAY %u", fixture->round);
    reminder_label(s_reminder_layer, line, 101, 204,
                   &pdkpass_body_font, 256, readable_text_color(background));

    make_block(s_reminder_layer, 6, 115, 228, 114, UI_INK);
    make_block(s_reminder_layer, 10, 119, 216, 106, card_color);
    uint32_t card_ink = readable_text_color(card_color);
    reminder_label(s_reminder_layer, "KICK-OFF IN", 134, 188,
                   &lv_font_unscii_16, 256, card_ink);
    make_block(s_reminder_layer, 31, 145, 174, 2, card_ink);
    int64_t remaining = fixture->kickoff_utc - (int64_t)time(NULL);
    if (remaining < 0) remaining = 0;
    snprintf(line, sizeof(line), "%ld", (long)((remaining + 59) / 60));
    reminder_label(s_reminder_layer, line, 177, 190,
                   &lv_font_unscii_16, 896, card_ink);
    reminder_label(s_reminder_layer, "MINUTES", 211, 188,
                   &lv_font_unscii_16, 256, card_ink);

    make_block(s_reminder_layer, 6, 232, 228, 52, UI_INK);
    make_block(s_reminder_layer, 10, 236, 220, 44, panel_color);
    uint32_t panel_ink = readable_text_color(panel_color);
    reminder_label(s_reminder_layer, fixture->home ? "HOME" : "AWAY", 250, 204,
                   &lv_font_unscii_16, 352, panel_ink);
    struct tm parts;
    lfc_beijing_parts(fixture->kickoff_utc, &parts);
    char date_line[40];
    snprintf(date_line, sizeof(date_line), "%02d %s %02d:%02d CST",
             parts.tm_mday, lfc_month((unsigned)parts.tm_mon), parts.tm_hour,
             parts.tm_min);
    reminder_label(s_reminder_layer, date_line, 270, 204,
                   &pdkpass_body_font, 256, panel_ink);

    make_block(s_reminder_layer, 4, 287, 232, 28, UI_INK);
    make_block(s_reminder_layer, 10, 291, 220, 20, UI_PAPER);
    reminder_label(s_reminder_layer, "ANY KEY TO DISMISS", 301, 206,
                   &pdkpass_body_font, 256, UI_INK);
}

// A background refresh replaced the fixture table. Repaint only the Liverpool
// views; on any other page the next natural render picks the change up. The
// dashboard cache is cleared so a result that just landed can move the carousel
// on to the next card.
void pdkpass_ui_lfc_update(void)
{
    if (!s_screen) return;
    s_home_shown = (size_t)-1;
    if (s_state.page == PDKPASS_PAGE_HOME ||
        s_state.page == PDKPASS_PAGE_LFC_DETAIL)
        render();
}
