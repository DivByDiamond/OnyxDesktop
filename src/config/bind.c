/*
 * bind.c - M2 desk.conf keys: bind= and photo=.
 *
 * Every handler validates the whole value first and only then
 * touches the config: an unknown action, an unsupported key, a bad
 * coordinate, an overlong path or an entry past DESK_MAX_PHOTOS
 * drops the line with a stderr note. The parse itself never fails,
 * so one broken line cannot make the desktop start without its
 * defaults.
 */
#include <stdio.h>
#include <string.h>
#include "bind.h"

/* Anything larger cannot be a frame coordinate; bailing out early
 * also keeps a long digit string from overflowing the accumulator. */
#define COORD_MAX 100000

/* Non-negative decimal digits only. */
static int parse_coord(const char *v, int len) {
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
        if (value > COORD_MAX) {
            return -1;
        }
    }
    return value;
}

/* esc/tab/enter by name, or one printable ASCII character by code. */
static int parse_key(const char *k, int len) {
    if (len == 3 && memcmp(k, "esc", 3) == 0) {
        return DESK_KEY_ESC;
    }
    if (len == 3 && memcmp(k, "tab", 3) == 0) {
        return DESK_KEY_TAB;
    }
    if (len == 5 && memcmp(k, "enter", 5) == 0) {
        return DESK_KEY_ENTER;
    }
    if (len == 1 && k[0] >= 32 && k[0] <= 126) {
        return k[0];
    }
    return -1;
}

static int action_index(const char *a, int len) {
    if (len == 8 && memcmp(a, "launcher", 8) == 0) {
        return DESK_BIND_LAUNCHER;
    }
    if (len == 7 && memcmp(a, "console", 7) == 0) {
        return DESK_BIND_CONSOLE;
    }
    return -1;
}

int cfg_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' ||
           c == '\v';
}

/* Advance p towards end past leading whitespace. */
const char *cfg_skip_space_until(const char *p, const char *end) {
    while (p < end && cfg_is_space(*p)) {
        p++;
    }
    return p;
}

/* Shrink [start, end) from the right. */
const char *cfg_trim_end(const char *start, const char *end) {
    while (end > start && cfg_is_space(end[-1])) {
        end--;
    }
    return end;
}

int cfg_parse_bind(const char *v, int len, struct desk_config *c) {
    const char *colon = memchr(v, ':', (size_t)len);
    const char *a;
    const char *k;
    int idx;
    int key;

    if (colon == NULL) {
        fprintf(stderr, "desk: config: bind= without ':' is ignored\n");
        return 0;
    }
    a = cfg_skip_space_until(v, colon);
    idx = action_index(a, (int)(cfg_trim_end(a, colon) - a));
    if (idx < 0) {
        fprintf(stderr, "desk: config: bind= action is ignored\n");
        return 0;
    }
    k = cfg_skip_space_until(colon + 1, v + len);
    key = parse_key(k, (int)(cfg_trim_end(k, v + len) - k));
    if (key < 0) {
        fprintf(stderr, "desk: config: bind= key is ignored\n");
        return 0;
    }
    /* A repeated action overwrites: the last line wins. */
    c->bind_key[idx] = (char)key;
    return 0;
}

int cfg_parse_photo(const char *v, int len, struct desk_config *c) {
    const char *c1 = memchr(v, ',', (size_t)len);
    const char *c2;
    const char *p;
    const char *t;
    int plen;
    int x;
    int y;

    if (c1 == NULL) {
        fprintf(stderr, "desk: config: photo= without ',' is ignored\n");
        return 0;
    }
    p = cfg_skip_space_until(v, c1);
    plen = (int)(cfg_trim_end(p, c1) - p);
    if (plen < 1 || plen > DESK_PATH_MAX - 1) {
        fprintf(stderr, "desk: config: photo= path is ignored\n");
        return 0;
    }
    c2 = memchr(c1 + 1, ',', (size_t)(v + len - c1 - 1));
    if (c2 == NULL) {
        fprintf(stderr, "desk: config: photo= without y is ignored\n");
        return 0;
    }
    t = cfg_skip_space_until(c1 + 1, c2);
    x = parse_coord(t, (int)(cfg_trim_end(t, c2) - t));
    t = cfg_skip_space_until(c2 + 1, v + len);
    y = parse_coord(t, (int)(cfg_trim_end(t, v + len) - t));
    if (x < 0 || y < 0) {
        fprintf(stderr, "desk: config: photo= coordinates are ignored\n");
        return 0;
    }
    if (c->n_photos >= DESK_MAX_PHOTOS) {
        fprintf(stderr, "desk: config: photo= past %d is ignored\n",
                DESK_MAX_PHOTOS);
        return 0;
    }
    memcpy(c->photos[c->n_photos], p, (size_t)plen);
    c->photos[c->n_photos][plen] = '\0';
    c->photo_x[c->n_photos] = x;
    c->photo_y[c->n_photos] = y;
    c->n_photos++;
    return 0;
}
