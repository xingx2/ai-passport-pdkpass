#include "lfc_data.h"

// Liverpool FC - 2026-27 fixture snapshot: the Premier League season and the
// Champions League league phase merged into one kick-off-ordered timeline.
// Captured 2026-10-09/10 from ESPN's public soccer API:
//   sports.core.api.espn.com/v2/sports/soccer/leagues/eng.1/
//   sports.core.api.espn.com/v2/sports/soccer/leagues/uefa.champions/
//     seasons/2026/types/1/teams/364/events   (Liverpool = team 364)
// Kick-off times are UTC epoch seconds; the UI renders Beijing time (UTC+8).
// goals_for/against stay zero until ESPN reports the match complete. The
// `competition` field distinguishes the two; Champions League rows are marked
// LFC_COMP_CL and carry the league-phase matchday in `round`.
const lfc_fixture_t lfc_fixtures[] = {
    {  401879319U,   1787499000LL,  1U, false, true , 2U, 2U, "Newcastle United", "St. James' Park", LFC_COMP_PL },
    {  401879314U,   1788003000LL,  2U, true , true , 2U, 2U, "Nottingham Forest", "Anfield", LFC_COMP_PL },
    {  401879288U,   1788548400LL,  3U, false, true , 2U, 0U, "Ipswich Town", "Portman Road", LFC_COMP_PL },
    {  401915446U,   1788951600LL,  1U, true , true , 2U, 1U, "Atletico Madrid", "Anfield", LFC_COMP_CL },
    {  401879279U,   1789221600LL,  4U, true , true , 0U, 0U, "Fulham", "Anfield", LFC_COMP_PL },
    {  401879276U,   1789909200LL,  5U, false, true , 1U, 0U, "AFC Bournemouth", "Vitality Stadium", LFC_COMP_PL },
    {  401879264U,   1791732600LL,  6U, true , false, 0U, 0U, "Manchester City", "Anfield", LFC_COMP_PL },
    {  401915415U,   1791967500LL,  2U, false, false, 0U, 0U, "LASK Linz", "Raiffeisen Arena (Linz)", LFC_COMP_CL },
    {  401879262U,   1792245600LL,  7U, false, false, 0U, 0U, "Brentford", "Gtech Community Stadium", LFC_COMP_PL },
    {  401915397U,   1792494000LL,  3U, true , false, 0U, 0U, "Villarreal", "Anfield", LFC_COMP_CL },
    {  401878765U,   1792936800LL,  8U, true , false, 0U, 0U, "Brighton & Hove Albion", "Anfield", LFC_COMP_PL },
    {  401879246U,   1793550600LL,  9U, true , false, 0U, 0U, "Arsenal", "Anfield", LFC_COMP_PL },
    {  401915382U,   1793785500LL,  4U, false, false, 0U, 0U, "Fenerbahce", "Sukru Saracoglu Stadium", LFC_COMP_CL },
    {  401879243U,   1794146400LL, 10U, false, false, 0U, 0U, "Crystal Palace", "Selhurst Park", LFC_COMP_PL },
    {  401878755U,   1795365000LL, 11U, true , false, 0U, 0U, "Manchester United", "Anfield", LFC_COMP_PL },
    {  401915292U,   1795608000LL,  5U, false, false, 0U, 0U, "Club Brugge", "Jan Breydel Stadium", LFC_COMP_CL },
    {  401879229U,   1795953600LL, 12U, false, false, 0U, 0U, "Everton", "Hill Dickinson Stadium", LFC_COMP_PL },
    {  401879219U,   1796241600LL, 13U, true , false, 0U, 0U, "Sunderland", "Anfield", LFC_COMP_PL },
    {  401879213U,   1796482800LL, 14U, false, false, 0U, 0U, "Chelsea", "Stamford Bridge", LFC_COMP_PL },
    {  401915369U,   1796817600LL,  6U, true , false, 0U, 0U, "FC Porto", "Anfield", LFC_COMP_CL },
    {  401878743U,   1797087600LL, 15U, true , false, 0U, 0U, "Leeds United", "Anfield", LFC_COMP_PL },
    {  401879200U,   1797692400LL, 16U, true , false, 0U, 0U, "Tottenham Hotspur", "Anfield", LFC_COMP_PL },
    {  401878733U,   1798306200LL, 17U, false, false, 0U, 0U, "Hull City", "The MKM Stadium", LFC_COMP_PL },
    {  401879194U,   1798661700LL, 18U, false, false, 0U, 0U, "Aston Villa", "Villa Park", LFC_COMP_PL },
    {  401879181U,   1798902000LL, 19U, true , false, 0U, 0U, "Coventry City", "Anfield", LFC_COMP_PL },
    {  401878727U,   1799180100LL, 20U, false, false, 0U, 0U, "Sunderland", "Stadium of Light", LFC_COMP_PL },
    {  401879166U,   1800111600LL, 21U, true , false, 0U, 0U, "Crystal Palace", "Anfield", LFC_COMP_PL },
    {  401915360U,   1800360000LL,  7U, false, false, 0U, 0U, "Internazionale", "San Siro", LFC_COMP_CL },
    {  401878718U,   1800716400LL, 22U, false, false, 0U, 0U, "Manchester United", "Old Trafford", LFC_COMP_PL },
    {  401915353U,   1801051200LL,  8U, true , false, 0U, 0U, "Lens", "Anfield", LFC_COMP_CL },
    {  401879155U,   1801321200LL, 23U, true , false, 0U, 0U, "Everton", "Anfield", LFC_COMP_PL },
    {  401878712U,   1801926000LL, 24U, false, false, 0U, 0U, "Arsenal", "Emirates Stadium", LFC_COMP_PL },
    {  401879145U,   1802289600LL, 25U, false, false, 0U, 0U, "Coventry City", "Coventry Building Society Arena", LFC_COMP_PL },
    {  401878703U,   1803135600LL, 26U, true , false, 0U, 0U, "Hull City", "Anfield", LFC_COMP_PL },
    {  401879129U,   1803740400LL, 27U, false, false, 0U, 0U, "Tottenham Hotspur", "Tottenham Hotspur Stadium", LFC_COMP_PL },
    {  401879125U,   1804104000LL, 28U, true , false, 0U, 0U, "Aston Villa", "Anfield", LFC_COMP_PL },
    {  401878690U,   1804950000LL, 29U, true , false, 0U, 0U, "Ipswich Town", "Anfield", LFC_COMP_PL },
    {  401879111U,   1805554800LL, 30U, false, false, 0U, 0U, "Fulham", "Craven Cottage", LFC_COMP_PL },
    {  401879101U,   1807365600LL, 31U, true , false, 0U, 0U, "Newcastle United", "Anfield", LFC_COMP_PL },
    {  401878681U,   1807970400LL, 32U, false, false, 0U, 0U, "Nottingham Forest", "The City Ground", LFC_COMP_PL },
    {  401879092U,   1808575200LL, 33U, false, false, 0U, 0U, "Leeds United", "Elland Road", LFC_COMP_PL },
    {  401878674U,   1809180000LL, 34U, true , false, 0U, 0U, "Chelsea", "Anfield", LFC_COMP_PL },
    {  401878668U,   1809784800LL, 35U, false, false, 0U, 0U, "Manchester City", "Etihad Stadium", LFC_COMP_PL },
    {  401878663U,   1810389600LL, 36U, true , false, 0U, 0U, "Brentford", "Anfield", LFC_COMP_PL },
    {  401878662U,   1811080800LL, 37U, false, false, 0U, 0U, "Brighton & Hove Albion", "American Express Stadium", LFC_COMP_PL },
    {  401879059U,   1811689200LL, 38U, true , false, 0U, 0U, "AFC Bournemouth", "Anfield", LFC_COMP_PL },
};
const size_t lfc_fixture_count = sizeof(lfc_fixtures) / sizeof(lfc_fixtures[0]);
const char lfc_season[] = "2026-27";
const char lfc_team_name[] = "LIVERPOOL";
