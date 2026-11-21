/*
 * test_bmp.c - BMP decoder tests on generated fixtures.
 *
 * Valid 24-bit (padded rows) and 32-bit files land pixel-exact in
 * both frame depths; bottom-up and top-down row order both come
 * out upright; every malformed header is refused with the frame
 * and its guard area untouched; a blit straddling an edge draws
 * exactly the intersection, and a blit fully outside is quiet.
 */
#include "bmp_gen.h"

static unsigned char file[BMP_GEN_MAX];
static unsigned char rgb[4 * 3 * 3];
static unsigned long file_len;
static struct fb_info gi;

/* 8x4 frame at the given depth, guard fill, triples ready. */
static void setup(int bpp) {
    rgb_fill(rgb, 4, 3);
    info_init(&gi, 8, 4, bpp);
    reset();
}

/* Rebuild the 4x3 bottom-up 24-bit fixture for a reject case. */
static unsigned long fresh(void) {
    return file_len = bmp_make(file, 4, 3, 24, 0, rgb);
}

/* One reject case: answer -1, frame and guard area untouched. */
static int rejected(void) {
    reset();
    if (bmp_blit(frame, &gi, file, file_len, 1, 1) != -1) return 0;
    if (!guard_ok(frame_len(&gi))) return 0;
    return raw_is(&gi, 1, 1);
}

static void test_valid_24_bottom_up(void) {
    unsigned long len;

    setup(16);
    len = bmp_make(file, 4, 3, 24, 0, rgb);
    CHECK(bmp_blit(frame, &gi, file, len, 1, 1) == 0);
    CHECK(px_is(&gi, 1, 1, chan565(0, 0))); /* top image row on top */
    CHECK(px_is(&gi, 2, 2, chan565(1, 1)));
    CHECK(px_is(&gi, 4, 3, chan565(3, 2))); /* bottom row at bottom */
    CHECK(raw_is(&gi, 0, 0));               /* outside the blit */
    CHECK(guard_ok(frame_len(&gi)));
}

/* Three pixels per row make a 9-byte row padded to 12: a decoder
 * that ignores the padding misreads every row after the first. */
static void test_valid_24_row_padding(void) {
    unsigned char small[3 * 2 * 3];
    unsigned long len;

    setup(16);
    rgb_fill(small, 3, 2);
    len = bmp_make(file, 3, 2, 24, 0, small);
    CHECK(bmp_blit(frame, &gi, file, len, 0, 0) == 0);
    CHECK(px_is(&gi, 0, 0, chan565(0, 0)));
    CHECK(px_is(&gi, 2, 0, chan565(2, 0)));
    CHECK(px_is(&gi, 0, 1, chan565(0, 1)));
    CHECK(px_is(&gi, 2, 1, chan565(2, 1)));
    CHECK(guard_ok(frame_len(&gi)));
}

static void test_valid_32(void) {
    unsigned long len;

    setup(16);
    len = bmp_make(file, 4, 3, 32, 0, rgb);
    CHECK(bmp_blit(frame, &gi, file, len, 2, 0) == 0);
    CHECK(px_is(&gi, 2, 0, chan565(0, 0)));
    CHECK(px_is(&gi, 5, 2, chan565(3, 2)));
    CHECK(raw_is(&gi, 1, 0));
    CHECK(guard_ok(frame_len(&gi)));
}

/* A 24-bit file onto a 32bpp frame exercises the expand path. */
static void test_valid_24_on_32bpp_frame(void) {
    unsigned long len;

    setup(32);
    len = bmp_make(file, 4, 3, 24, 0, rgb);
    CHECK(bmp_blit(frame, &gi, file, len, 0, 1) == 0);
    CHECK(px_is(&gi, 0, 1, chan565(0, 0)));
    CHECK(px_is(&gi, 3, 3, chan565(3, 2)));
    CHECK(guard_ok(frame_len(&gi)));
}

static void test_valid_32_top_down(void) {
    unsigned long len;

    setup(16);
    len = bmp_make(file, 4, 3, 32, 1, rgb);
    CHECK(bmp_blit(frame, &gi, file, len, 1, 0) == 0);
    CHECK(px_is(&gi, 1, 0, chan565(0, 0))); /* same upright image */
    CHECK(px_is(&gi, 4, 2, chan565(3, 2)));
    CHECK(guard_ok(frame_len(&gi)));
}

/* Every malformed header: refuse, touch neither frame nor guard. */
static void test_malformed_headers(void) {
    struct fb_info broken;

    setup(16);
    fresh();
    memcpy(file, "BX", 2);
    CHECK(rejected()); /* wrong magic */

    fresh();
    reset();
    CHECK(bmp_blit(frame, &gi, file, 53, 1, 1) == -1); /* short */
    CHECK(bmp_blit(frame, &gi, file, 0, 1, 1) == -1);
    CHECK(bmp_blit(frame, &gi, NULL, fresh(), 1, 1) == -1);
    CHECK(raw_is(&gi, 1, 1) && guard_ok(frame_len(&gi)));

    fresh(); put32(file + 30, 3);    CHECK(rejected()); /* BI_BITFIELDS */
    fresh(); put16(file + 28, 8);    CHECK(rejected()); /* bpp 8 */
    fresh(); put16(file + 28, 16);   CHECK(rejected()); /* bpp 16 */
    fresh(); put32(file + 22, 0);    CHECK(rejected()); /* zero height */
    fresh(); put32(file + 18, 0);    CHECK(rejected()); /* zero width */
    fresh(); put32(file + 18, 4097); CHECK(rejected()); /* width limit */
    fresh(); put32(file + 22, 4097); CHECK(rejected()); /* height limit */
    fresh(); put32(file + 22, 0x80000000UL); CHECK(rejected()); /* 2^31 rows */
    fresh(); put32(file + 14, 12);   CHECK(rejected()); /* tiny info hdr */
    fresh(); put32(file + 10, 40);   CHECK(rejected()); /* off_bits low */
    fresh(); put32(file + 2, file_len + 1); CHECK(rejected()); /* file size */
    fresh(); put32(file + 2, 60);    CHECK(rejected()); /* size under data */
    fresh(); put16(file + 26, 0);    CHECK(rejected()); /* biPlanes */

    fresh();
    reset();
    CHECK(bmp_blit(frame, &gi, file, file_len - 1, 1, 1) == -1);
    CHECK(raw_is(&gi, 1, 1) && guard_ok(frame_len(&gi))); /* truncated */

    CHECK(bmp_blit(NULL, &gi, file, fresh(), 1, 1) == -1);
    CHECK(bmp_blit(frame, NULL, file, fresh(), 1, 1) == -1);
    broken = gi;
    broken.bpp = 8; /* put_px depth guard: never draw to this */
    CHECK(bmp_blit(frame, &broken, file, file_len, 1, 1) == -1);
    CHECK(raw_is(&gi, 1, 1) && guard_ok(frame_len(&gi)));
}

static void test_clip_right_bottom(void) {
    unsigned long len;

    setup(16);
    len = bmp_make(file, 4, 3, 24, 0, rgb);
    CHECK(bmp_blit(frame, &gi, file, len, 6, 2) == 0);
    /* only source pixels (0..1, 0..1) land inside the 8x4 frame */
    CHECK(px_is(&gi, 6, 2, chan565(0, 0)));
    CHECK(px_is(&gi, 7, 2, chan565(1, 0)));
    CHECK(px_is(&gi, 6, 3, chan565(0, 1)));
    CHECK(px_is(&gi, 7, 3, chan565(1, 1)));
    CHECK(raw_is(&gi, 5, 2)); /* left of the blit */
    CHECK(raw_is(&gi, 7, 1)); /* above it */
    CHECK(guard_ok(frame_len(&gi)));
}

static void test_clip_negative_origin(void) {
    unsigned long len;

    setup(16);
    len = bmp_make(file, 4, 3, 24, 0, rgb);
    CHECK(bmp_blit(frame, &gi, file, len, -2, -1) == 0);
    /* source pixels (2..3, 1..2) shift into (0..1, 0..1) */
    CHECK(px_is(&gi, 0, 0, chan565(2, 1)));
    CHECK(px_is(&gi, 1, 1, chan565(3, 2)));
    CHECK(raw_is(&gi, 2, 0));
    CHECK(guard_ok(frame_len(&gi)));
}

/* A blit with an empty intersection is valid and draws nothing. */
static void test_fully_outside_is_quiet(void) {
    unsigned long len;

    setup(16);
    len = bmp_make(file, 4, 3, 24, 0, rgb);
    CHECK(bmp_blit(frame, &gi, file, len, 100, 100) == 0);
    CHECK(raw_is(&gi, 0, 0));
    CHECK(guard_ok(frame_len(&gi)));
}

int main(void) {
    test_valid_24_bottom_up();
    test_valid_24_row_padding();
    test_valid_32();
    test_valid_24_on_32bpp_frame();
    test_valid_32_top_down();
    test_malformed_headers();
    test_clip_right_bottom();
    test_clip_negative_origin();
    test_fully_outside_is_quiet();
    printf("\ntest_bmp: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
