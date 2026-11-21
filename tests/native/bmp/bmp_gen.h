/*
 * bmp_gen.h - guard-frame fixtures and BMP generators.
 *
 * Pulls in the ui guard-frame discipline (test_ui_util.h): the
 * frame is followed by a guard area every test checks, so an
 * out-of-bounds write fails the suite. BMP files are generated in
 * the test from plain RGB triples; nothing binary ships in the
 * repo.
 */
#ifndef TEST_BMP_GEN_H
#define TEST_BMP_GEN_H

#include <string.h>
#include "../ui/test_ui_util.h"
#include "widgets/bmp.h"

/* Little-endian field writers, the inverse of the decoder readers. */
static inline void put16(unsigned char *p, unsigned v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
}

static inline void put32(unsigned char *p, unsigned long v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

#define BMP_GEN_MAX 512

/* Serialise a 24/32-bit uncompressed BMP from w*h top-down RGB
 * triples. A bottom-up file stores the last image row first; a
 * top-down file stores a negative height and the rows in order.
 * Returns the file length. */
static inline unsigned long bmp_make(unsigned char *out, unsigned w,
                                     unsigned h, int bpp, int top_down,
                                     const unsigned char *rgb) {
    unsigned long px = (unsigned long)w * (unsigned)(bpp / 8);
    unsigned long row = bpp == 32 ? px : (px + 3) & ~3UL;
    unsigned long len = 54 + row * h;
    unsigned x, y;

    memset(out, 0, len);
    out[0] = 'B';
    out[1] = 'M';
    put32(out + 2, len);
    put32(out + 10, 54);
    put32(out + 14, 40);
    put32(out + 18, w);
    put32(out + 22, top_down ? 0UL - h : h); /* low 32 bits: -h */
    put16(out + 26, 1);
    put16(out + 28, (unsigned)bpp);
    for (y = 0; y < h; y++) {
        unsigned char *dst = out + 54 + row * (top_down ? y : h - 1 - y);
        for (x = 0; x < w; x++) {
            const unsigned char *s = rgb + ((size_t)y * w + x) * 3;
            dst[0] = s[2];
            dst[1] = s[1];
            dst[2] = s[0];
            dst += bpp / 8;
        }
    }
    return len;
}

/* Distinct colour per source pixel, so orientation, clipping and
 * channel order are all visible in the frame. */
static inline unsigned char chan_r(int x, int y) {
    return (unsigned char)(20 + x * 40);
}

static inline unsigned char chan_g(int x, int y) {
    return (unsigned char)(30 + y * 50);
}

static inline unsigned char chan_b(int x, int y) {
    return (unsigned char)(x * 17 + y * 9);
}

static inline unsigned short chan565(int x, int y) {
    return ui_rgb(chan_r(x, y), chan_g(x, y), chan_b(x, y));
}

static inline void rgb_fill(unsigned char *rgb, unsigned w, unsigned h) {
    unsigned x, y;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            unsigned char *s = rgb + ((size_t)y * w + x) * 3;
            s[0] = chan_r((int)x, (int)y);
            s[1] = chan_g((int)x, (int)y);
            s[2] = chan_b((int)x, (int)y);
        }
    }
}

#endif /* TEST_BMP_GEN_H */
