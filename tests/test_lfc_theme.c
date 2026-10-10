// Fixture theming stays readable for every club in the bundled snapshot.
//
// The Liverpool views take their background from the ground the fixture is
// played at: Anfield is Liverpool red, an away trip takes the host club's
// signature colour. That means a white-kit club (Fulham, Leeds United,
// Tottenham Hotspur) is never allowed to contribute its white shirt: it either
// sits behind Liverpool red at Anfield or contributes the dark accent from the
// same kit at its own ground, so the page text always has something to read
// against. This test pins that down for the whole season.
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "lfc_data.h"
#include "lfc_theme.h"

// Palette values copied from ui_pixel.h, which pulls in LVGL and cannot be
// included from a host test. Keep in step with the panel colours.
#define UI_PAPER 0xF4F4EAU
#define WHITE 0xFFFFFFU

// sRGB channel to linear light, as WCAG 2.1 defines it.
static double linear_channel(uint32_t channel)
{
    double value = (double)channel / 255.0;
    return value <= 0.03928 ? value / 12.92
                            : pow((value + 0.055) / 1.055, 2.4);
}

static double luminance(uint32_t color)
{
    return 0.2126 * linear_channel((color >> 16) & 0xffU) +
           0.7152 * linear_channel((color >> 8) & 0xffU) +
           0.0722 * linear_channel(color & 0xffU);
}

static double contrast(uint32_t first, uint32_t second)
{
    double a = luminance(first);
    double b = luminance(second);
    double lighter = a > b ? a : b;
    double darker = a > b ? b : a;
    return (lighter + 0.05) / (darker + 0.05);
}

// The club colours that exist only because their home kit is white. Each has to
// stay dark enough to carry the page text on its own ground.
static const char *const white_kit_clubs[] = {
    "Fulham",
    "Leeds United",
    "Tottenham Hotspur",
};

int main(void)
{
    assert(lfc_fixture_count > 0U);

    uint32_t colors[LFC_MAX_FIXTURES];
    size_t color_count = 0;

    // 1. Every opponent in the snapshot resolves to a club colour. An unknown
    //    name would silently fall back to Liverpool red at an away ground, which
    //    is exactly the bug this guards against.
    for (size_t i = 0; i < lfc_fixture_count; i++) {
        const lfc_fixture_t *fixture = &lfc_fixtures[i];
        uint32_t color = 0;
        assert(lfc_team_color(fixture->opponent, &color));
        assert(color != 0U);

        bool seen = false;
        for (size_t j = 0; j < color_count; j++) {
            if (colors[j] == color) seen = true;
        }
        if (!seen && color_count < LFC_MAX_FIXTURES) colors[color_count++] = color;

        // 2. The theme follows the ground, not the card's club. At Anfield it is
        //    Liverpool red whatever the opponent; away it is the host colour.
        pdkpass_theme_t theme = lfc_fixture_theme(fixture);
        assert(theme.top != 0U && theme.bottom != 0U);
        assert(theme.top != theme.bottom);
        if (fixture->home) {
            assert(theme.top == LFC_RED);
        } else {
            assert(theme.top == color);
        }
        // The bottom half is the same hue, darkened for the panel gradient.
        assert(luminance(theme.bottom) < luminance(theme.top));

        // 3. Every background keeps the page text readable. The pixel font is
        //    set large, so WCAG's 3:1 large-text ratio is the floor; the paper
        //    colour used for that text is the tighter of the two light inks.
        assert(contrast(theme.top, UI_PAPER) >= 3.0);
        assert(contrast(theme.bottom, UI_PAPER) >= 3.0);
    }

    // 4. The white-kit clubs contribute a dark accent, never their shirt: these
    //    are the grounds where a light background would have washed the text out.
    for (size_t i = 0; i < sizeof(white_kit_clubs) / sizeof(white_kit_clubs[0]);
         i++) {
        uint32_t color = 0;
        assert(lfc_team_color(white_kit_clubs[i], &color));
        assert(luminance(color) < 0.25);
        assert(contrast(color, UI_PAPER) >= 4.5);
        assert(contrast(color, WHITE) >= 4.5);
    }

    // 5. One club, two grounds, two themes: Tottenham Hotspur is red at Anfield
    //    because that is where the match is played, and its own colour when
    //    Liverpool travels there. A white primary colour never reaches a card.
    bool home_seen = false, away_seen = false;
    for (size_t i = 0; i < lfc_fixture_count; i++) {
        const lfc_fixture_t *fixture = &lfc_fixtures[i];
        if (strcmp(fixture->opponent, "Tottenham Hotspur") != 0) continue;
        pdkpass_theme_t theme = lfc_fixture_theme(fixture);
        if (fixture->home) {
            assert(theme.top == LFC_RED);
            home_seen = true;
        } else {
            assert(theme.top == 0x132257U);
            away_seen = true;
        }
    }
    assert(home_seen && away_seen);

    // 6. Liverpool's own fixture list never hands the theme to a light colour.
    uint32_t red = 0;
    assert(lfc_team_color("Liverpool", &red));
    assert(red == LFC_RED);
    assert(luminance(red) < 0.25);
    return 0;
}
