#!/usr/bin/env python3
"""Generate main/lfc_data.c from the curated Liverpool 2026-27 fixture snapshot.

Input: a JSON array (see tools/lfc/README for provenance) with one object per
Premier League fixture: id, kickoff (ISO-8601 UTC), home, away, venue, played,
hs, as, lfc_home.

Usage: python3 tools/lfc/generate_lfc_data.py <raw.json> <out.c>
"""
import json
import sys
from datetime import datetime, timezone

OPPONENT_LEN = 24
VENUE_LEN = 34
SEASON = "2026-27"
TEAM = "LIVERPOOL"


def epoch(iso: str) -> int:
    # ESPN emits "2026-08-23T15:30Z".
    dt = datetime.strptime(iso, "%Y-%m-%dT%H:%MZ").replace(tzinfo=timezone.utc)
    return int(dt.timestamp())


def escape(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"')


def main() -> int:
    raw_path, out_path = sys.argv[1], sys.argv[2]
    rows = json.load(open(raw_path, encoding="utf-8"))
    rows.sort(key=lambda m: m["kickoff"])

    lines = [
        '#include "lfc_data.h"',
        "",
        "// Liverpool FC - %s English Premier League fixture snapshot." % SEASON,
        "// Captured 2026-10-09 from ESPN's public soccer API",
        "//   sports.core.api.espn.com/v2/sports/soccer/leagues/eng.1/",
        "//   seasons/2026/types/1/teams/364/events   (Liverpool = team 364)",
        "// Kick-off times are UTC epoch seconds; the UI renders Beijing time (UTC+8).",
        "// goals_for/against stay zero until ESPN reports the match complete.",
        "const lfc_fixture_t lfc_fixtures[] = {",
    ]

    max_opp = max_ven = 0
    for i, m in enumerate(rows, 1):
        opp = m["away"] if m["lfc_home"] else m["home"]
        venue = m["venue"] or ""
        max_opp = max(max_opp, len(opp))
        max_ven = max(max_ven, len(venue))
        assert len(opp) + 1 <= OPPONENT_LEN, "opponent too long: %r" % opp
        assert len(venue) + 1 <= VENUE_LEN, "venue too long: %r" % venue
        played = "true" if m["played"] else "false"
        home = "true" if m["lfc_home"] else "false"
        gf = m["hs"] if m["lfc_home"] else m["as"]
        ga = m["as"] if m["lfc_home"] else m["hs"]
        gf = gf if m["played"] and gf is not None else 0
        ga = ga if m["played"] and ga is not None else 0
        lines.append(
            '    { %10dU, %11dLL, %2dU, %-5s, %-5s, %2dU, %2dU, "%s", "%s" },'
            % (int(m["id"]), epoch(m["kickoff"]), i, home, played, gf, ga,
               escape(opp), escape(venue))
        )

    lines += [
        "};",
        "const size_t lfc_fixture_count = sizeof(lfc_fixtures) / sizeof(lfc_fixtures[0]);",
        'const char lfc_season[] = "%s";' % SEASON,
        'const char lfc_team_name[] = "%s";' % TEAM,
        "",
    ]
    open(out_path, "w", encoding="utf-8").write("\n".join(lines))
    print("wrote %s (%d fixtures, max opponent %d, max venue %d)"
          % (out_path, len(rows), max_opp, max_ven))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
