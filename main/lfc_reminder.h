#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Kick-off alert service for the Liverpool card. It shares the ALERTS switch
// and the synchronized-clock requirement with the Formula 1 reminders, and is
// delivered from the same worker so it also wakes a dark display.

// Restores the last announced kick-off from NVS. Call once during boot.
esp_err_t lfc_reminder_init(void);

// True once when a kick-off enters its lead window; `*fixture_index` receives
// the announced fixture. The kick-off is persisted before the alert is
// returned, so a restart inside the same window cannot repeat it.
bool lfc_reminder_poll(int64_t now_utc, size_t *fixture_index);

// Milliseconds until the next kick-off could become due, for the worker wait.
uint32_t lfc_reminder_wait_ms(int64_t now_utc);
