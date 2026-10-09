#pragma once

#include "lfc_data.h"
#include <stdbool.h>
#include <stddef.h>

// Persisted fixture table. The bundled snapshot is the baseline; a refresh that
// changed something is written here and reloaded on the next boot, so results
// survive without another network round trip. Call after the cache partition is
// ready and before anything reads the table.
void lfc_fixtures_start(void);
bool lfc_fixtures_save(const lfc_fixture_t *fixtures, size_t count);
