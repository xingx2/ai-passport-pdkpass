#include "lfc_reminder.h"

#include "lfc_reminder_core.h"
#include "lfc_table.h"
#include "pdkpass_cache.h"
#include "pdkpass_reminder.h"
#include "esp_log.h"

#define LFC_ALERT_MAGIC 0x50444C41U // "PDLA"
#define LFC_ALERT_VERSION 1U
#define LFC_ALERT_NAMESPACE "pdk_lfc"
#define LFC_ALERT_KEY "alert"

static const char *TAG = "pdkpass_lfc";

// Only the announced kick-off is remembered: kick-off times are unique within a
// season, so a single value is enough to suppress a repeat without tracking a
// growing set of event ids.
typedef struct {
    uint32_t magic;
    uint16_t version, reserved;
    int64_t last_kickoff;
} lfc_alert_store_t;

static int64_t s_last_alerted;

esp_err_t lfc_reminder_init(void)
{
    lfc_alert_store_t rec = {0};
    size_t size = sizeof(rec);
    if (pdkpass_cache_read_blob(LFC_ALERT_NAMESPACE, LFC_ALERT_KEY, &rec, &size) == ESP_OK &&
        size == sizeof(rec) && rec.magic == LFC_ALERT_MAGIC &&
        rec.version == LFC_ALERT_VERSION)
        s_last_alerted = rec.last_kickoff;
    else
        s_last_alerted = 0;
    return ESP_OK;
}

bool lfc_reminder_poll(int64_t now_utc, size_t *fixture_index)
{
    // Kick-off notices ride the shared ALERTS setting. The Liverpool fixtures
    // are part of the single dashboard deck, so they are not opted into
    // separately and a Formula 1-only viewer simply turns ALERTS off.
    if (!fixture_index) return false;
    size_t index = lfc_reminder_due_at(now_utc, pdkpass_reminder_enabled(),
                                       pdkpass_reminder_time_valid(),
                                       s_last_alerted);
    if (index >= lfc_table_count()) return false;
    // Remember before announcing, so a crash or restart in the same window
    // cannot deliver the alert a second time.
    lfc_alert_store_t rec = {
        .magic = LFC_ALERT_MAGIC,
        .version = LFC_ALERT_VERSION,
        .reserved = 0,
        .last_kickoff = lfc_table()[index].kickoff_utc,
    };
    if (pdkpass_cache_write_blob(LFC_ALERT_NAMESPACE, LFC_ALERT_KEY, &rec, sizeof(rec)) != ESP_OK) {
        ESP_LOGW(TAG, "Kick-off alert not saved; skipping to avoid repeats");
        return false;
    }
    s_last_alerted = rec.last_kickoff;
    *fixture_index = index;
    return true;
}

uint32_t lfc_reminder_wait_ms(int64_t now_utc)
{
    return lfc_reminder_wait_until(now_utc, pdkpass_reminder_enabled(),
                                   pdkpass_reminder_time_valid(), s_last_alerted);
}
