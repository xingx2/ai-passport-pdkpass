#include "lfc_data.h"

// Liverpool FC - 2026-27 English Premier League fixture snapshot.
// Captured 2026-10-09 from ESPN's public soccer API
//   sports.core.api.espn.com/v2/sports/soccer/leagues/eng.1/
//   seasons/2026/types/1/teams/364/events   (Liverpool = team 364)
// Kick-off times are UTC epoch seconds; the UI renders Beijing time (UTC+8).
// goals_for/against stay zero until ESPN reports the match complete.
const lfc_fixture_t lfc_fixtures[] = {
    {  401879319U,  1787499000LL,  1U, false, true ,  2U,  2U, "Newcastle United", "St. James' Park" },
    {  401879314U,  1788003000LL,  2U, true , true ,  2U,  2U, "Nottingham Forest", "Anfield" },
    {  401879288U,  1788548400LL,  3U, false, true ,  2U,  0U, "Ipswich Town", "Portman Road" },
    {  401879279U,  1789221600LL,  4U, true , true ,  0U,  0U, "Fulham", "Anfield" },
    {  401879276U,  1789909200LL,  5U, false, true ,  1U,  0U, "AFC Bournemouth", "Vitality Stadium" },
    {  401879264U,  1791732600LL,  6U, true , false,  0U,  0U, "Manchester City", "Anfield" },
    {  401879262U,  1792245600LL,  7U, false, false,  0U,  0U, "Brentford", "Gtech Community Stadium" },
    {  401878765U,  1792936800LL,  8U, true , false,  0U,  0U, "Brighton & Hove Albion", "Anfield" },
    {  401879246U,  1793550600LL,  9U, true , false,  0U,  0U, "Arsenal", "Anfield" },
    {  401879243U,  1794146400LL, 10U, false, false,  0U,  0U, "Crystal Palace", "Selhurst Park" },
    {  401878755U,  1795365000LL, 11U, true , false,  0U,  0U, "Manchester United", "Anfield" },
    {  401879229U,  1795953600LL, 12U, false, false,  0U,  0U, "Everton", "Hill Dickinson Stadium" },
    {  401879219U,  1796241600LL, 13U, true , false,  0U,  0U, "Sunderland", "Anfield" },
    {  401879213U,  1796482800LL, 14U, false, false,  0U,  0U, "Chelsea", "Stamford Bridge" },
    {  401878743U,  1797087600LL, 15U, true , false,  0U,  0U, "Leeds United", "Anfield" },
    {  401879200U,  1797692400LL, 16U, true , false,  0U,  0U, "Tottenham Hotspur", "Anfield" },
    {  401878733U,  1798306200LL, 17U, false, false,  0U,  0U, "Hull City", "The MKM Stadium" },
    {  401879194U,  1798661700LL, 18U, false, false,  0U,  0U, "Aston Villa", "Villa Park" },
    {  401879181U,  1798902000LL, 19U, true , false,  0U,  0U, "Coventry City", "Anfield" },
    {  401878727U,  1799180100LL, 20U, false, false,  0U,  0U, "Sunderland", "Stadium of Light" },
    {  401879166U,  1800111600LL, 21U, true , false,  0U,  0U, "Crystal Palace", "Anfield" },
    {  401878718U,  1800716400LL, 22U, false, false,  0U,  0U, "Manchester United", "Old Trafford" },
    {  401879155U,  1801321200LL, 23U, true , false,  0U,  0U, "Everton", "Anfield" },
    {  401878712U,  1801926000LL, 24U, false, false,  0U,  0U, "Arsenal", "Emirates Stadium" },
    {  401879145U,  1802289600LL, 25U, false, false,  0U,  0U, "Coventry City", "Coventry Building Society Arena" },
    {  401878703U,  1803135600LL, 26U, true , false,  0U,  0U, "Hull City", "Anfield" },
    {  401879129U,  1803740400LL, 27U, false, false,  0U,  0U, "Tottenham Hotspur", "Tottenham Hotspur Stadium" },
    {  401879125U,  1804104000LL, 28U, true , false,  0U,  0U, "Aston Villa", "Anfield" },
    {  401878690U,  1804950000LL, 29U, true , false,  0U,  0U, "Ipswich Town", "Anfield" },
    {  401879111U,  1805554800LL, 30U, false, false,  0U,  0U, "Fulham", "Craven Cottage" },
    {  401879101U,  1807365600LL, 31U, true , false,  0U,  0U, "Newcastle United", "Anfield" },
    {  401878681U,  1807970400LL, 32U, false, false,  0U,  0U, "Nottingham Forest", "The City Ground" },
    {  401879092U,  1808575200LL, 33U, false, false,  0U,  0U, "Leeds United", "Elland Road" },
    {  401878674U,  1809180000LL, 34U, true , false,  0U,  0U, "Chelsea", "Anfield" },
    {  401878668U,  1809784800LL, 35U, false, false,  0U,  0U, "Manchester City", "Etihad Stadium" },
    {  401878663U,  1810389600LL, 36U, true , false,  0U,  0U, "Brentford", "Anfield" },
    {  401878662U,  1811080800LL, 37U, false, false,  0U,  0U, "Brighton & Hove Albion", "American Express Stadium" },
    {  401879059U,  1811689200LL, 38U, true , false,  0U,  0U, "AFC Bournemouth", "Anfield" },
};
const size_t lfc_fixture_count = sizeof(lfc_fixtures) / sizeof(lfc_fixtures[0]);
const char lfc_season[] = "2026-27";
const char lfc_team_name[] = "LIVERPOOL";
