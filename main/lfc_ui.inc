// Liverpool views. Included by pdkpass_ui.c so the renderers reuse the same
// label helpers, palette and status bar as the Formula 1 dashboard. The theme
// of every view is the colour of the ground the fixture is played at: Anfield
// stays Liverpool red, away fixtures take the host club's signature colour.

// Fixture shown on the detail page. It is opened straight from the dashboard
// card, so there is no season list in between to keep in step.
static size_t s_lfc_selected;

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
// complete or the Beijing kick-off while it is still ahead. `position` is the
// card's place in the shared dashboard deck, not the matchday, so the counter
// and the progress bar read the same on both kinds of card.
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
    make_center_label(s_content, fixture->opponent, 0, 73, INNER_W,
                      &lv_font_unscii_16, top_ink);

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
    make_fit_zoom_label(s_content, fixture->opponent, 0, 24, INNER_W, top_ink);
    make_fit_body_label(s_content, fixture->venue, 0, 78, INNER_W, top_ink);

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
    set_hint("UP/DN:MATCH HOLD:BACK");
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
    } else if (input == PDKPASS_INPUT_OK || input == PDKPASS_INPUT_BACK) {
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
