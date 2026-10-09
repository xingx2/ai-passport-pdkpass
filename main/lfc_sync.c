#include "lfc_sync.h"

#include "bsp_display.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lfc_schedule.h"
#include "lfc_store.h"
#include "lfc_sync_core.h"
#include "lfc_table.h"
#include "pdkpass_http.h"
#include "pdkpass_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *TAG = "pdkpass_lfc";

// ESPN's public core API. Its "site" endpoints (scoreboard, summary) run to tens
// of kilobytes and the firmware's HTTP client cannot decompress gzip, so the
// refresh walks the per-resource documents instead: 250 B to 3 KB each, which
// keeps every allocation inside the budget the rest of the firmware uses.
#define LFC_CORE_API "https://sports.core.api.espn.com/v2/sports/soccer/leagues/eng.1"

// One pass clears a whole backlog; the worker then waits out the cooldown.
#define LFC_SYNC_MAX_FIXTURES 4U
#define LFC_SYNC_HTTP_DEADLINE_US (60LL * 1000000LL)
#define LFC_SYNC_IDLE_SECONDS (6LL * 60LL * 60LL)
#define LFC_SYNC_PRESSED_SECONDS 60LL
#define LFC_SYNC_WAKE_MS 60000U
#define LFC_SYNC_TASK_STACK 4096U
#define LFC_SYNC_URL_LEN 200U

// Body budgets. The competitors page is the largest document a refresh reads.
#define LFC_SYNC_COMPETITORS_LIMIT 4096U
#define LFC_SYNC_STATUS_LIMIT 1024U
#define LFC_SYNC_SCORE_LIMIT 1024U
#define LFC_SYNC_EVENT_LIMIT (16U * 1024U)

static TaskHandle_t s_task;
static volatile bool s_online, s_time_valid;
static volatile bool s_pressed;
static int64_t s_last_success;
static int64_t s_last_attempt;

// Only this worker touches the candidate copy, and it is three kilobytes, so it
// lives in .bss rather than on the worker's stack.
static lfc_fixture_t s_candidate[LFC_MAX_FIXTURES];

static bool sync_get(const char *url, size_t limit, char **body)
{
    *body = NULL;
    if (pdkpass_http_expired()) return false;
    return pdkpass_http_get(url, limit, body) == ESP_OK;
}

// The competition's competitors page names both sides, which is how the refresh
// learns the opponent's id without downloading the whole competition document.
static bool read_sides(const lfc_fixture_t *fixture, uint32_t *home, uint32_t *away)
{
    char url[LFC_SYNC_URL_LEN];
    snprintf(url, sizeof(url),
             LFC_CORE_API "/events/%u/competitions/%u/competitors",
             (unsigned)fixture->event_id, (unsigned)fixture->event_id);
    char *body = NULL;
    if (!sync_get(url, LFC_SYNC_COMPETITORS_LIMIT, &body)) return false;
    bool ok = lfc_sync_parse_competitors(body, strlen(body), home, away);
    free(body);
    return ok;
}

static bool read_score(uint32_t event_id, uint32_t team_id, int *goals)
{
    char url[LFC_SYNC_URL_LEN];
    snprintf(url, sizeof(url),
             LFC_CORE_API "/events/%u/competitions/%u/competitors/%u/score",
             (unsigned)event_id, (unsigned)event_id, (unsigned)team_id);
    char *body = NULL;
    if (!sync_get(url, LFC_SYNC_SCORE_LIMIT, &body)) return false;
    bool ok = lfc_sync_parse_score(body, strlen(body), goals);
    free(body);
    return ok;
}

// Fill in a finished match. False means "nothing usable", never "0-0": the
// caller must not write a result the API did not confirm.
static bool read_result(const lfc_fixture_t *fixture, lfc_sync_event_t *event)
{
    uint32_t home_team = 0, away_team = 0;
    if (!read_sides(fixture, &home_team, &away_team)) return false;

    char url[LFC_SYNC_URL_LEN];
    snprintf(url, sizeof(url), LFC_CORE_API "/events/%u/competitions/%u/status",
             (unsigned)fixture->event_id, (unsigned)fixture->event_id);
    char *body = NULL;
    if (!sync_get(url, LFC_SYNC_STATUS_LIMIT, &body)) return false;
    bool completed = false;
    bool ok = lfc_sync_parse_status(body, strlen(body), &completed);
    free(body);
    if (!ok || !completed) return false;

    uint32_t ours = fixture->home ? home_team : away_team;
    uint32_t theirs = fixture->home ? away_team : home_team;
    int our_goals = -1, their_goals = -1;
    if (!read_score(fixture->event_id, ours, &our_goals)) return false;
    if (!read_score(fixture->event_id, theirs, &their_goals)) return false;

    event->completed = true;
    event->home_goals = fixture->home ? our_goals : their_goals;
    event->away_goals = fixture->home ? their_goals : our_goals;
    return true;
}

// Kick-off times are only re-read once per pass, for the next unplayed match:
// every other fixture's time is either in the past or far enough away that
// re-reading it would only add latency.
static void read_kickoff(const lfc_fixture_t *fixture, lfc_sync_event_t *event)
{
    char url[LFC_SYNC_URL_LEN];
    snprintf(url, sizeof(url), LFC_CORE_API "/events/%u", (unsigned)fixture->event_id);
    char *body = NULL;
    if (!sync_get(url, LFC_SYNC_EVENT_LIMIT, &body)) return;
    int64_t kickoff = 0;
    if (lfc_sync_parse_date(body, strlen(body), &kickoff)) event->kickoff_utc = kickoff;
    free(body);
}

// A kick-off move is only visible before the match starts, so the next future
// fixture is re-read on every pass even though it has no result yet. Fixtures
// are stored in kick-off order, so the first one in the future is the next one.
static bool refresh_next_kickoff(size_t count, int64_t now)
{
    for (size_t i = 0; i < count; i++) {
        if (s_candidate[i].kickoff_utc <= now) continue;
        lfc_sync_event_t event = {
            .event_id = s_candidate[i].event_id,
            .completed = false,
            .kickoff_utc = 0,
            .home_goals = -1,
            .away_goals = -1,
        };
        read_kickoff(&s_candidate[i], &event);
        if (event.kickoff_utc <= 0) return false;
        return lfc_sync_apply(s_candidate, count, &event);
    }
    return false;
}

static void sync_once(int64_t now)
{
    size_t count = 0;
    if (!lfc_table_copy(s_candidate, LFC_MAX_FIXTURES, &count)) return;
    s_last_attempt = now;
    if (!pdkpass_http_begin_until(esp_timer_get_time() + LFC_SYNC_HTTP_DEADLINE_US))
        return;

    bool changed = false;
    unsigned checked = 0;
    // Kick-offs first: it is the largest document of the pass, so it runs while
    // the heap is still untouched by the smaller result reads.
    if (refresh_next_kickoff(count, now)) changed = true;
    for (size_t i = 0; i < count && checked < LFC_SYNC_MAX_FIXTURES; i++) {
        lfc_fixture_t *fixture = &s_candidate[i];
        // A result only exists once the match window has closed.
        if (fixture->kickoff_utc + LFC_MATCH_WINDOW_SECONDS > now) break;
        if (pdkpass_http_expired()) break;
        checked++;

        lfc_sync_event_t event = {
            .event_id = fixture->event_id,
            .completed = false,
            .kickoff_utc = 0,
            .home_goals = -1,
            .away_goals = -1,
        };
        if (!read_result(fixture, &event)) continue;
        if (lfc_sync_apply(s_candidate, count, &event)) changed = true;
    }
    pdkpass_http_end();
    if (!changed) return;

    // Flash first, then publish: a redraw must never wait on NVS, and the next
    // boot reads the same rows even if the publish is skipped.
    lfc_fixtures_save(s_candidate, count);
    if (bsp_lvgl_lock(1000)) {
        lfc_table_publish(s_candidate, count);
        pdkpass_ui_lfc_update();
        bsp_lvgl_unlock();
    } else {
        // The table is still correct for this boot; the next refresh retries.
        ESP_LOGW(TAG, "Fixture refresh published on the next pass");
    }
    s_last_success = now;
    ESP_LOGI(TAG, "Fixture refresh complete (%u match(es) checked)", checked);
}

static void sync_task(void *argument)
{
    (void)argument;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(LFC_SYNC_WAKE_MS));
        int64_t now = (int64_t)time(NULL);
        if (!s_online || !s_time_valid || now <= 0) continue;
        bool pressed = s_pressed;
        s_pressed = false;
        int64_t cooldown = pressed ? LFC_SYNC_PRESSED_SECONDS : LFC_SYNC_IDLE_SECONDS;
        if (s_last_attempt && now < s_last_attempt + cooldown) continue;
        sync_once(now);
    }
}

esp_err_t lfc_sync_start(void)
{
    if (s_task) return ESP_OK;
    if (xTaskCreate(sync_task, "pdk_lfc_net", LFC_SYNC_TASK_STACK, NULL, 2,
                    &s_task) != pdPASS) {
        s_task = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void lfc_sync_set_network(bool online, bool time_valid)
{
    bool was_usable = s_online && s_time_valid;
    s_online = online;
    s_time_valid = time_valid;
    // Arm a first pass on the transition into usable connectivity; the worker
    // re-evaluates its own cooldown either way.
    if (!was_usable && online && time_valid && s_task) xTaskNotifyGive(s_task);
}

void lfc_sync_request(void)
{
    s_pressed = true;
    if (s_task) xTaskNotifyGive(s_task);
}

int64_t lfc_sync_last_success(void)
{
    return s_last_success;
}
