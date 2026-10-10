#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

// Keeps the Liverpool fixture table current. One worker owns the HTTP
// transaction, so a refresh never races the season or results services for the
// radio; updated rows are persisted and published without a restart.
//
// Call lfc_sync_start() once, after the cache partition and the network service
// exist. The worker stays parked until lfc_sync_set_network() reports usable
// connectivity with a synchronized clock.
esp_err_t lfc_sync_start(void);

// Network transitions. Reaching online-with-time arms a refresh; there is no
// attempt while offline or while the clock is only the estimate restored from
// NVS, because the fixture windows are decided by absolute time.
void lfc_sync_set_network(bool online, bool time_valid);

// Ask for a refresh as soon as the clock allows. Worker-safe.
void lfc_sync_request(void);

// The fixture detail page refreshes with OK, so it has to be able to say what
// happened: a key that quietly does nothing reads as a broken key. Every
// outcome advances the generation, which lets the footer notice a repeat of the
// same word and keep showing it for a moment.
typedef enum {
    LFC_REFRESH_IDLE = 0,
    LFC_REFRESH_UNAVAILABLE, // no radio, or no synchronized clock yet
    LFC_REFRESH_RUNNING,
    LFC_REFRESH_UPDATED,
    LFC_REFRESH_UNCHANGED,   // ESPN answered, but nothing had changed
    LFC_REFRESH_FAILED,      // the pass could not complete
} lfc_refresh_state_t;

typedef struct {
    unsigned generation;
    lfc_refresh_state_t state;
} lfc_refresh_status_t;

// False until the first manual request of this boot.
bool lfc_sync_refresh_status(lfc_refresh_status_t *status);

// Unix time of the last successful refresh, or 0 when nothing has landed yet.
int64_t lfc_sync_last_success(void);
