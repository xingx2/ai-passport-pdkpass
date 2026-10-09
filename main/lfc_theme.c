#include "lfc_theme.h"

#include <string.h>

typedef struct {
    const char *team;
    uint32_t color;
} lfc_team_color_t;

// Signature colours for the 2026-27 Premier League clubs. `lfc_team_color`
// matches on the ESPN display name used by the bundled snapshot. Clubs whose
// home kit is white (Fulham, Tottenham, Leeds) use the dark accent from the
// same kit so the themed background keeps carrying the page text.
static const lfc_team_color_t s_team_colors[] = {
    { "Arsenal",                0xEF0107U },
    { "Aston Villa",            0x670E36U },
    { "AFC Bournemouth",        0xDA291CU },
    { "Brentford",              0xE30613U },
    { "Brighton & Hove Albion", 0x0057B8U },
    { "Chelsea",                0x034694U },
    { "Coventry City",          0x2E7FB8U },
    { "Crystal Palace",         0x1B458FU },
    { "Everton",                0x003399U },
    { "Fulham",                 0x1B1B1BU },
    { "Hull City",              0xC86A00U },
    { "Ipswich Town",           0x3A64A3U },
    { "Leeds United",           0x1D428AU },
    { "Liverpool",              LFC_RED },
    { "Manchester City",        0x2F87C2U },
    { "Manchester United",      0xDA020EU },
    { "Newcastle United",       0x241F20U },
    { "Nottingham Forest",      0xDD0000U },
    { "Sunderland",             0xEB172BU },
    { "Tottenham Hotspur",      0x132257U },
};

static uint32_t darken(uint32_t color)
{
    const uint32_t factor = 222U;
    uint32_t red = ((color >> 16) & 0xffU) * factor / 256U;
    uint32_t green = ((color >> 8) & 0xffU) * factor / 256U;
    uint32_t blue = (color & 0xffU) * factor / 256U;
    return (red << 16) | (green << 8) | blue;
}

bool lfc_team_color(const char *team, uint32_t *color)
{
    if (!team || !color) return false;
    for (size_t i = 0; i < sizeof(s_team_colors) / sizeof(s_team_colors[0]); i++) {
        if (strcmp(s_team_colors[i].team, team) == 0) {
            *color = s_team_colors[i].color;
            return true;
        }
    }
    return false;
}

pdkpass_theme_t lfc_fixture_theme(const lfc_fixture_t *fixture)
{
    uint32_t base = LFC_RED;
    // A home fixture is played at Anfield; away fixtures take the host club's
    // colour, so the theme always reflects the ground the match is at.
    if (fixture && !fixture->home && !lfc_team_color(fixture->opponent, &base))
        base = LFC_RED;
    pdkpass_theme_t theme = { .top = base, .bottom = darken(base) };
    return theme;
}
