#include "lfc_sync_core.h"

#include <string.h>

// ESPN's core API answers with deeply nested documents whose leaves are mostly
// "$ref" pointers. The refresh only ever needs four short fields, so instead of
// pulling in cJSON (and the heap churn that costs) this file walks the text
// with a handful of quote-aware scanners.

static bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Advance past whitespace, never past `end`.
static const char *skip(const char *p, const char *end)
{
    while (p < end && is_space(*p)) p++;
    return p;
}

// Locate the value that follows an exact `"key":` pair. Matching the closing
// quote keeps "score" from matching "scores" and "id" from matching "uid".
static const char *find_key(const char *p, const char *end, const char *key)
{
    size_t klen = strlen(key);
    for (; p + klen + 2 <= end; p++) {
        if (*p != '"' || memcmp(p + 1, key, klen) != 0 || p[1 + klen] != '"')
            continue;
        const char *q = skip(p + klen + 2, end);
        if (q < end && *q == ':') return skip(q + 1, end);
    }
    return NULL;
}

// Copy a JSON string value starting at `p`. False when it is not a string.
static bool read_string(const char *p, const char *end, char *out, size_t cap)
{
    if (p >= end || *p != '"' || cap == 0) return false;
    p++;
    size_t n = 0;
    while (p < end && *p != '"') {
        char c = *p;
        if (c == '\\') { // Only the escapes ESPN actually emits matter here.
            if (++p >= end) return false;
            c = *p;
        }
        if (n + 1 < cap) out[n++] = c;
        p++;
    }
    if (p >= end) return false;
    out[n] = '\0';
    return true;
}

static bool read_long(const char *p, const char *end, long *out)
{
    p = skip(p, end);
    bool negative = p < end && *p == '-';
    if (negative) p++;
    if (p >= end || *p < '0' || *p > '9') return false;
    long value = 0;
    while (p < end && *p >= '0' && *p <= '9') value = value * 10 + (*p++ - '0');
    *out = negative ? -value : value;
    return true;
}

// ESPN quotes its numbers ("id":"364", "displayValue":"1") on most documents
// but not on all of them, so accept either spelling.
static bool read_number(const char *p, const char *end, long *out)
{
    p = skip(p, end);
    if (p < end && *p == '"') p++;
    return read_long(p, end, out);
}

static bool read_bool(const char *p, const char *end, bool *out)
{
    p = skip(p, end);
    if (end - p >= 4 && memcmp(p, "true", 4) == 0) { *out = true; return true; }
    if (end - p >= 5 && memcmp(p, "false", 5) == 0) { *out = false; return true; }
    return false;
}

// Return the address just past the object that starts at `p` ({ ... }).
static const char *skip_object(const char *p, const char *end)
{
    if (p >= end || *p != '{') return NULL;
    unsigned depth = 0;
    bool quoted = false, escaped = false;
    for (; p < end; p++) {
        char c = *p;
        if (quoted) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
            continue;
        }
        if (c == '"') quoted = true;
        else if (c == '{' || c == '[') {
            if (++depth > 16U) return NULL;
        } else if (c == '}' || c == ']') {
            if (depth == 0 || --depth == 0) return p + 1;
        }
    }
    return NULL;
}

// Walks the top-level objects of the array held by `key`.
typedef struct {
    const char *p, *end;
} object_cursor_t;

static bool cursor_init(object_cursor_t *cursor, const char *json,
                        size_t length, const char *key)
{
    if (!json || !cursor || length < 2U) return false;
    const char *end = json + length;
    const char *p = find_key(json, end, key);
    if (!p) return false;
    while (p < end && *p != '[') {
        if (!is_space(*p)) return false;
        p++;
    }
    if (p >= end) return false;
    cursor->p = p + 1;
    cursor->end = end;
    return true;
}

static bool cursor_next(object_cursor_t *cursor, const char **start,
                        const char **end)
{
    while (cursor->p < cursor->end &&
           (is_space(*cursor->p) || *cursor->p == ','))
        cursor->p++;
    if (cursor->p >= cursor->end || *cursor->p != '{') return false;
    const char *object_end = skip_object(cursor->p, cursor->end);
    if (!object_end) return false;
    *start = cursor->p;
    *end = object_end;
    cursor->p = object_end;
    return true;
}

bool lfc_sync_iso8601(const char *text, size_t length, int64_t *epoch)
{
    if (!text || !epoch || length < 16) return false;
    // "YYYY-MM-DDTHH:MM" with an optional ":SS" and a trailing 'Z'.
    const char *p = text;
    const char *end = text + length;
    long year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    for (unsigned i = 0; i < 4U; i++) { if (p >= end || *p < '0' || *p > '9') return false; year = year * 10 + (*p++ - '0'); }
    if (p >= end || *p++ != '-') return false;
    for (unsigned i = 0; i < 2U; i++) { if (p >= end || *p < '0' || *p > '9') return false; month = month * 10 + (*p++ - '0'); }
    if (p >= end || *p++ != '-') return false;
    for (unsigned i = 0; i < 2U; i++) { if (p >= end || *p < '0' || *p > '9') return false; day = day * 10 + (*p++ - '0'); }
    if (p >= end || (*p != 'T' && *p != 't')) return false;
    p++;
    for (unsigned i = 0; i < 2U; i++) { if (p >= end || *p < '0' || *p > '9') return false; hour = hour * 10 + (*p++ - '0'); }
    if (p >= end || *p++ != ':') return false;
    for (unsigned i = 0; i < 2U; i++) { if (p >= end || *p < '0' || *p > '9') return false; minute = minute * 10 + (*p++ - '0'); }
    if (p < end && *p == ':') {
        p++;
        for (unsigned i = 0; i < 2U; i++) { if (p >= end || *p < '0' || *p > '9') return false; second = second * 10 + (*p++ - '0'); }
    }
    if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 || second > 60)
        return false;

    // Days since the Unix epoch (Howard Hinnant's civil-from-days, inverted).
    long y = year - (month <= 2);
    long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (unsigned)((153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1);
    unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    int64_t days = (int64_t)era * 146097 + (int64_t)doe - 719468;
    *epoch = days * 86400 + hour * 3600 + minute * 60 + second;
    return true;
}

bool lfc_sync_parse_date(const char *json, size_t length, int64_t *kickoff_utc)
{
    if (!json || !kickoff_utc || length < 2U) return false;
    const char *end = json + length;
    // The event document's own "date" precedes every nested one.
    const char *date = find_key(json, end, "date");
    char stamp[32];
    if (!date || !read_string(date, end, stamp, sizeof(stamp))) return false;
    return lfc_sync_iso8601(stamp, strlen(stamp), kickoff_utc);
}

bool lfc_sync_parse_competitors(const char *json, size_t length,
                                uint32_t *home_team, uint32_t *away_team)
{
    if (!json || !home_team || !away_team) return false;
    *home_team = 0U;
    *away_team = 0U;
    object_cursor_t cursor;
    if (!cursor_init(&cursor, json, length, "items")) return false;
    const char *start = NULL, *end = NULL;
    while (cursor_next(&cursor, &start, &end)) {
        char side[8];
        const char *home_away = find_key(start, end, "homeAway");
        const char *id = find_key(start, end, "id");
        long team = 0;
        if (!home_away || !id ||
            !read_string(home_away, end, side, sizeof(side)) ||
            !read_number(id, end, &team))
            continue;
        if (strcmp(side, "home") == 0) *home_team = (uint32_t)team;
        else if (strcmp(side, "away") == 0) *away_team = (uint32_t)team;
    }
    // A half-answered page is not a result: leave nothing behind for the
    // caller to mistake for one side of the fixture.
    if (*home_team == 0U || *away_team == 0U) {
        *home_team = 0U;
        *away_team = 0U;
        return false;
    }
    return true;
}

bool lfc_sync_parse_status(const char *json, size_t length, bool *completed)
{
    if (!json || !completed || length < 2U) return false;
    const char *end = json + length;
    const char *flag = find_key(json, end, "completed");
    return flag && read_bool(flag, end, completed);
}

bool lfc_sync_parse_score(const char *json, size_t length, int *goals)
{
    if (!json || !goals || length < 2U) return false;
    const char *end = json + length;
    // "displayValue" is the score as printed; "value" is the same number. A
    // score document carries both, so either can back up the other.
    char text[16];
    const char *display = find_key(json, end, "displayValue");
    if (display && read_string(display, end, text, sizeof(text))) {
        long value = 0;
        if (read_long(text, text + strlen(text), &value)) {
            *goals = (int)value;
            return true;
        }
    }
    long value = 0;
    const char *raw = find_key(json, end, "value");
    if (raw && read_number(raw, end, &value)) {
        *goals = (int)value;
        return true;
    }
    return false;
}

bool lfc_sync_apply(lfc_fixture_t *fixtures, size_t count,
                    const lfc_sync_event_t *event)
{
    if (!fixtures || !event) return false;
    for (size_t i = 0; i < count; i++) {
        lfc_fixture_t *fixture = &fixtures[i];
        if (fixture->event_id != event->event_id) continue;

        bool changed = false;
        if (event->kickoff_utc > 0 && event->kickoff_utc != fixture->kickoff_utc) {
            fixture->kickoff_utc = event->kickoff_utc;
            changed = true;
        }
        if (event->completed && event->home_goals >= 0 && event->away_goals >= 0) {
            int for_goals = fixture->home ? event->home_goals : event->away_goals;
            int against = fixture->home ? event->away_goals : event->home_goals;
            if (for_goals <= 255 && against <= 255 &&
                (!fixture->played || fixture->goals_for != (uint8_t)for_goals ||
                 fixture->goals_against != (uint8_t)against)) {
                fixture->goals_for = (uint8_t)for_goals;
                fixture->goals_against = (uint8_t)against;
                fixture->played = true;
                changed = true;
            }
        }
        return changed;
    }
    return false;
}
