/*
 * config.c - line-oriented parser for /etc/desk.conf.
 *
 * The input is never modified: every line is bounded by [start, end)
 * pointers, keys and values are trimmed in place, and only complete
 * names are copied into the fixed-size apps table. Unknown keys (and
 * lines without '=' at all) are ignored, so comments and future keys
 * keep working on older builds.
 */
#include <string.h>
#include "bind.h"
#include "config.h"

#define PANEL_KEY "panel_height"
#define PANEL_KEY_LEN 12
#define APP_KEY "app"
#define APP_KEY_LEN 3
#define BIND_KEY "bind"
#define BIND_KEY_LEN 4
#define PHOTO_KEY "photo"
#define PHOTO_KEY_LEN 5

/* 1..200, decimal digits only. Bails out as soon as the value exceeds
 * 200, so a long digit string cannot overflow the accumulator. */
static int parse_panel_height(const char *v, int len) {
    int value = 0;
    int i;

    if (len <= 0) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        if (v[i] < '0' || v[i] > '9') {
            return -1;
        }
        value = value * 10 + (v[i] - '0');
        if (value > 200) {
            return -1;
        }
    }
    return value >= 1 ? value : -1;
}

/* Validate and append one app name. A name that does not fit is an error,
 * never a truncation. */
static int add_app(struct desk_config *c, const char *v, int len) {
    int i;

    if (len < 1 || len > DESK_NAME_MAX - 1 || c->n_apps >= DESK_MAX_APPS) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        if (cfg_is_space(v[i])) {
            return -1;
        }
    }
    memcpy(c->apps[c->n_apps], v, (size_t)len);
    c->apps[c->n_apps][len] = '\0';
    c->n_apps++;
    return 0;
}

static int parse_line(const char *line, int len, struct desk_config *c,
                      int *apps_seen) {
    const char *start = cfg_skip_space_until(line, line + len);
    const char *end = cfg_trim_end(start, line + len);
    const char *eq;
    const char *key_end;
    const char *value;
    int value_len;

    if (start >= end) {
        return 0; /* blank line */
    }
    if (*start == '#') {
        return 0; /* full-line comment */
    }

    eq = start;
    while (eq < end && *eq != '=') {
        eq++;
    }
    if (eq == end) {
        return 0; /* not key=value: ignored like an unknown key */
    }

    key_end = cfg_trim_end(start, eq);
    value = cfg_skip_space_until(eq + 1, end);
    value_len = (int)(end - value);

    if (key_end - start == PANEL_KEY_LEN &&
        memcmp(start, PANEL_KEY, PANEL_KEY_LEN) == 0) {
        int h = parse_panel_height(value, value_len);
        if (h < 0) {
            return -1;
        }
        c->panel_height = h;
        return 0;
    }
    if (key_end - start == APP_KEY_LEN &&
        memcmp(start, APP_KEY, APP_KEY_LEN) == 0) {
        /* The first app= line replaces the built-in list; later lines
         * append to it. Entries past n_apps stay empty strings. */
        if (!*apps_seen) {
            memset(c->apps, 0, sizeof c->apps);
            c->n_apps = 0;
            *apps_seen = 1;
        }
        return add_app(c, value, value_len);
    }
    if (key_end - start == BIND_KEY_LEN &&
        memcmp(start, BIND_KEY, BIND_KEY_LEN) == 0) {
        return cfg_parse_bind(value, value_len, c);
    }
    if (key_end - start == PHOTO_KEY_LEN &&
        memcmp(start, PHOTO_KEY, PHOTO_KEY_LEN) == 0) {
        return cfg_parse_photo(value, value_len, c);
    }
    return 0; /* unknown key */
}

void desk_config_default(struct desk_config *out) {
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof *out);
    out->panel_height = DESK_DEFAULT_PANEL_HEIGHT;
    /* Default M2 keybinds: launcher on esc, console on tab. */
    out->bind_key[DESK_BIND_LAUNCHER] = DESK_KEY_ESC;
    out->bind_key[DESK_BIND_CONSOLE] = DESK_KEY_TAB;
    /* The built-in app list: the launcher is never empty, so a broken
     * or missing desk.conf still leaves the desktop usable. */
    strcpy(out->apps[0], "osnake");
    strcpy(out->apps[1], "ochat-gui");
    out->n_apps = 2;
}

int desk_config_parse(const char *text, struct desk_config *out) {
    const char *p;
    int apps_seen = 0;

    if (out == NULL) {
        return -1;
    }
    desk_config_default(out);
    if (text == NULL) {
        return 0;
    }

    p = text;
    while (*p != '\0') {
        const char *nl = strchr(p, '\n');
        int len = nl != NULL ? (int)(nl - p) : (int)strlen(p);
        if (parse_line(p, len, out, &apps_seen) != 0) {
            desk_config_default(out);
            return -1;
        }
        p += len;
        if (nl != NULL) {
            p++;
        }
    }
    return 0;
}
