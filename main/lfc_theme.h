#pragma once

#include "lfc_data.h"
#include "pdkpass_theme.h"
#include <stdbool.h>
#include <stdint.h>

// Liverpool red, used for every Anfield fixture.
#define LFC_RED 0xC8102EU

// Signature colour for a club. Returns false for an unknown name, so the
// fixture test can prove every bundled opponent is covered.
bool lfc_team_color(const char *team, uint32_t *color);

// Theme for a fixture: the home club's colour, so Anfield is Liverpool red and
// each away ground keeps its own identity (the venue the fixture is played at).
pdkpass_theme_t lfc_fixture_theme(const lfc_fixture_t *fixture);
