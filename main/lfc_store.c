#include "lfc_store.h"

#include "lfc_table.h"
#include "pdkpass_cache.h"
#include "esp_log.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define LFC_NAMESPACE "pdk_lfc"
#define LFC_FIXTURES_MAGIC 0x50444C58U // "PDLX"
// Bumped to 2 when lfc_fixture_t gained the `competition` field, so a cached
// Premier-League-only snapshot from an older firmware is rejected and the
// bundled (merged) snapshot takes over instead of being misread.
#define LFC_FIXTURES_VERSION 2U
#define LFC_FIXTURES_KEY "fixtures"

static const char *TAG = "pdkpass_lfc";

// The refreshed table is stored whole. One blob keeps the write atomic: a power
// cut either leaves the previous snapshot or the new one, never a mixture.
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t count;
    lfc_fixture_t fixtures[LFC_MAX_FIXTURES];
} lfc_fixtures_blob_t;

// Static so neither startup (which runs on the main task's small stack) nor a
// background refresh has to put three kilobytes on the stack.
static lfc_fixtures_blob_t s_blob;

void lfc_fixtures_start(void)
{
    size_t size = sizeof(s_blob);
    bool valid =
        pdkpass_cache_read_blob(LFC_NAMESPACE, LFC_FIXTURES_KEY, &s_blob, &size) == ESP_OK &&
        size == sizeof(s_blob) && s_blob.magic == LFC_FIXTURES_MAGIC &&
        s_blob.version == LFC_FIXTURES_VERSION && s_blob.count > 0U &&
        s_blob.count <= LFC_MAX_FIXTURES;
    if (!valid) return; // Keep the bundled snapshot compiled into the firmware.
    // Nothing has read the table yet, so no LVGL lock is needed here.
    if (!lfc_table_publish(s_blob.fixtures, s_blob.count))
        ESP_LOGW(TAG, "Stored fixture snapshot rejected");
}

bool lfc_fixtures_save(const lfc_fixture_t *fixtures, size_t count)
{
    if (!fixtures || count == 0U || count > LFC_MAX_FIXTURES) return false;
    s_blob.magic = LFC_FIXTURES_MAGIC;
    s_blob.version = LFC_FIXTURES_VERSION;
    s_blob.count = (uint16_t)count;
    memset(s_blob.fixtures, 0, sizeof(s_blob.fixtures));
    memcpy(s_blob.fixtures, fixtures, count * sizeof(*fixtures));
    if (pdkpass_cache_write_blob(LFC_NAMESPACE, LFC_FIXTURES_KEY, &s_blob,
                                 sizeof(s_blob)) != ESP_OK) {
        ESP_LOGW(TAG, "Refreshed fixtures not saved");
        return false;
    }
    return true;
}
