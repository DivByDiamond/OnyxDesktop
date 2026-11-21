/*
 * test_photo.c - photo widget layer tests.
 *
 * photo_draw reads every configured photo through the desk_io
 * read_file seam and blits it: a valid file lands pixel-exact; a
 * missing, oversized or malformed file is skipped with one stderr
 * note and the remaining photos still draw.
 */
#include <unistd.h>
#include "bmp_fake.h"

static unsigned char file[BMP_GEN_MAX];
static unsigned char rgb[4 * 3 * 3];
static unsigned long file_len;

static struct fb_info gi;
static struct desk_config gc;

/* kbd test recipe: run one photo_draw with stderr captured. */
static size_t grabbed(char *out, size_t cap) {
    FILE *capf;
    size_t n;
    int saved;

    fflush(stderr);
    saved = dup(2);
    capf = tmpfile();
    dup2(fileno(capf), 2);
    photo_draw(frame, &gi, &gc, &PHOTO_IO);
    fflush(stderr);
    dup2(saved, 2);
    close(saved);
    rewind(capf);
    n = fread(out, 1, cap - 1, capf);
    out[n] = '\0';
    fclose(capf);
    return n;
}

static void photo_setup(void) {
    rgb_fill(rgb, 4, 3);
    info_init(&gi, 8, 4, 16);
    file_len = bmp_make(file, 4, 3, 24, 0, rgb);
    memset(&gc, 0, sizeof gc);
    reset();
    files_reset();
}

static void add_photo(int slot, const char *path, int x, int y) {
    gc.n_photos++;
    snprintf(gc.photos[slot], DESK_PATH_MAX, "%s", path);
    gc.photo_x[slot] = x;
    gc.photo_y[slot] = y;
}

static void test_valid_file_blits(void) {
    photo_setup();
    CHECK(file_add("/etc/p.bmp", file, file_len) == 0);
    add_photo(0, "/etc/p.bmp", 2, 1);
    photo_draw(frame, &gi, &gc, &PHOTO_IO);
    CHECK(px_is(&gi, 2, 1, chan565(0, 0)));
    CHECK(px_is(&gi, 5, 3, chan565(3, 2)));
    CHECK(raw_is(&gi, 1, 1)); /* left of the photo */
    CHECK(P.calls[0] == 1);   /* read exactly once */
    CHECK(guard_ok(frame_len(&gi)));
}

static void test_missing_file_warns_and_skips(void) {
    char buf[256];

    photo_setup();
    CHECK(file_add("/etc/q.bmp", file, file_len) == 0);
    add_photo(0, "/no/such.bmp", 0, 0);
    add_photo(1, "/etc/q.bmp", 4, 2);
    CHECK(grabbed(buf, sizeof buf) > 0);
    CHECK(strstr(buf, "desk: photo /no/such.bmp unreadable") != NULL);
    CHECK(raw_is(&gi, 0, 0)); /* nothing where the missing photo goes */
    CHECK(px_is(&gi, 4, 2, chan565(0, 0))); /* the next photo drew */
    CHECK(px_is(&gi, 7, 3, chan565(3, 1)));
    CHECK(P.calls[0] == 1);
    CHECK(guard_ok(frame_len(&gi)));
}

static void test_failed_read_warns_and_skips(void) {
    char buf[256];

    photo_setup();
    CHECK(file_add("/etc/bad.bmp", file, file_len) == 0);
    P.fail[0] = 1; /* open or read refused */
    add_photo(0, "/etc/bad.bmp", 0, 0);
    CHECK(grabbed(buf, sizeof buf) > 0);
    CHECK(strstr(buf, "desk: photo /etc/bad.bmp unreadable") != NULL);
    CHECK(raw_is(&gi, 0, 0));
    CHECK(P.calls[0] == 1);
    CHECK(guard_ok(frame_len(&gi)));
}

static void test_oversized_file_skips(void) {
    char buf[256];

    photo_setup();
    CHECK(file_add("/etc/big.bmp", NULL, (unsigned long)PHOTO_FILE_CAP + 1) == 0);
    add_photo(0, "/etc/big.bmp", 0, 0);
    CHECK(grabbed(buf, sizeof buf) > 0);
    CHECK(strstr(buf, "desk: photo /etc/big.bmp too large") != NULL);
    CHECK(raw_is(&gi, 0, 0)); /* never blitted */
    CHECK(P.calls[0] == 1);
    CHECK(guard_ok(frame_len(&gi)));
}

static void test_invalid_bmp_warns_and_skips(void) {
    char buf[256];

    photo_setup();
    memcpy(file, "BM", 2);
    memset(file + 2, 0, 128); /* plausible size, broken header */
    CHECK(file_add("/etc/junk.bmp", file, 200) == 0);
    add_photo(0, "/etc/junk.bmp", 0, 0);
    CHECK(grabbed(buf, sizeof buf) > 0);
    CHECK(strstr(buf, "desk: photo /etc/junk.bmp invalid") != NULL);
    CHECK(raw_is(&gi, 0, 0));
    CHECK(P.calls[0] == 1); /* read once, then refused by the decoder */
    CHECK(guard_ok(frame_len(&gi)));
}

int main(void) {
    test_valid_file_blits();
    test_missing_file_warns_and_skips();
    test_failed_read_warns_and_skips();
    test_oversized_file_skips();
    test_invalid_bmp_warns_and_skips();
    printf("\ntest_photo: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
