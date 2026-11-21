/*
 * test_ui_util.h - shared fixtures for the ui drawing tests.
 *
 * The frame buffer is followed by a guard area: every drawing test
 * checks that the bytes after width*height*(bpp/8) are untouched, so an
 * out-of-bounds write fails the suite instead of silently passing.
 */
#ifndef TEST_UI_UTIL_H
#define TEST_UI_UTIL_H

#include <stdio.h>
#include <string.h>
#include "ui.h"

static int checks;
static int failures;

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (cond) {                                                      \
            printf("  ok  %s\n", #cond);                                 \
        } else {                                                         \
            printf("  FAIL %s (%s:%d)\n", #cond, __FILE__, __LINE__);    \
            failures++;                                                  \
        }                                                                \
    } while (0)

#define GUARD 256
#define GUARD_BYTE 0xA5

static unsigned char frame[8192 + GUARD];

static inline void reset(void) {
    memset(frame, GUARD_BYTE, sizeof frame);
}

static inline size_t frame_len(const struct fb_info *i) {
    return (size_t)i->width * i->height * (i->bpp / 8);
}

static inline int guard_ok(size_t used) {
    size_t k;

    for (k = used; k < sizeof frame; k++) {
        if (frame[k] != GUARD_BYTE) {
            return 0;
        }
    }
    return 1;
}

static inline unsigned read_px(const struct fb_info *i, int x, int y) {
    int bytes = (int)(i->bpp / 8);
    const unsigned char *p =
        frame + ((size_t)y * i->width + (size_t)x) * (size_t)bytes;
    unsigned v = (unsigned)p[0] | ((unsigned)p[1] << 8);

    if (bytes == 4) {
        v |= ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
    }
    return v;
}

/* Same expansion otablet uses for 32bpp (device/geometry.c). */
static inline unsigned expand(unsigned short c) {
    return ((((c >> 11) & 31) * 255 / 31) << 16) |
           ((((c >> 5) & 63) * 255 / 63) << 8) |
           ((c & 31) * 255 / 31);
}

static inline int px_is(const struct fb_info *i, int x, int y,
                        unsigned short c) {
    return read_px(i, x, y) == (i->bpp == 16 ? (unsigned)c : expand(c));
}

/* Untouched pixel: still the guard byte in every component. */
static inline int raw_is(const struct fb_info *i, int x, int y) {
    int bytes = (int)(i->bpp / 8);
    const unsigned char *p =
        frame + ((size_t)y * i->width + (size_t)x) * (size_t)bytes;
    int k;

    for (k = 0; k < bytes; k++) {
        if (p[k] != GUARD_BYTE) {
            return 0;
        }
    }
    return 1;
}

static inline int rect_is(const struct fb_info *i, int x0, int y0, int w,
                          int h, unsigned short c) {
    int x, y;

    for (y = y0; y < y0 + h; y++) {
        for (x = x0; x < x0 + w; x++) {
            if (!px_is(i, x, y, c)) {
                return 0;
            }
        }
    }
    return 1;
}

static inline int has_px(const struct fb_info *i, int x0, int y0, int w,
                         int h, unsigned short c) {
    int x, y;

    for (y = y0; y < y0 + h; y++) {
        for (x = x0; x < x0 + w; x++) {
            if (px_is(i, x, y, c)) {
                return 1;
            }
        }
    }
    return 0;
}

static inline void info_init(struct fb_info *i, unsigned w, unsigned h,
                             int bpp) {
    i->width = w;
    i->height = h;
    i->bpp = (unsigned)bpp;
    i->pitch = w * (unsigned)(bpp / 8);
    i->size = i->pitch * h;
}

static inline void config_init(struct desk_config *c, int panel_h) {
    memset(c, 0, sizeof *c);
    c->panel_height = panel_h;
    c->n_apps = 2;
    strcpy(c->apps[0], "osnake");
    strcpy(c->apps[1], "ochat-gui");
}

#endif /* TEST_UI_UTIL_H */
