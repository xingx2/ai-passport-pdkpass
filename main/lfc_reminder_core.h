#pragma once

#include "lfc_data.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Kick-off alerts. A fixture is announced once its kick-off enters the lead
// window below, so the reminder behaves like the Formula 1 alert but keyed on a
// single match instead of a race weekend's sessions.

#define LFC_REMINDER_LEAD_SECONDS 600LL

// Index of the fixture that should be announced now, or lfc_table_count() when
// nothing is due. `last_alerted_kickoff` is the kick-off already announced, so a
// match is never announced twice; `time_valid` gates on a synchronized clock so
// a restored estimate can never fire an alert early.
size_t lfc_reminder_due_at(int64_t now_utc, bool alerts_enabled, bool time_valid,
                           int64_t last_alerted_kickoff);

// Milliseconds until the next kick-off could become due, capped at one day to
// bound the caller's wait. UINT32_MAX means nothing is scheduled.
uint32_t lfc_reminder_wait_until(int64_t now_utc, bool alerts_enabled,
                                 bool time_valid, int64_t last_alerted_kickoff);
