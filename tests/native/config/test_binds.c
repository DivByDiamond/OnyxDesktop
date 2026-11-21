/*
 * test_binds.c - host tests for the M2 desk.conf extensions: the
 * bind= and photo= lines handled by src/config/bind.c.
 *
 *   make -C tests/native test
 *
 * bind=<action>:<key> maps launcher and console to esc, tab, enter or
 * a single printable ASCII key; unknown actions or keys, a missing
 * ':' and duplicates (the last line wins) only drop the line.
 * photo=<path>,<x>,<y> accepts up to DESK_MAX_PHOTOS entries with
 * non-negative decimal coordinates and a comma-free path of at most
 * DESK_PATH_MAX-1 characters; malformed lines are dropped too, and
 * none of them may fail the whole parse or disturb the v1 fields.
 */
#include <stdio.h>
#include <string.h>
#include "config/config.h"

static int failures = 0;
static int checks = 0;

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

/* The M2 defaults on top of the v1 defaults: launcher on esc,
 * console on tab, no photos. */
#define M2_DEFAULTS(c)                                                   \
    ((c).bind_key[DESK_BIND_LAUNCHER] == DESK_KEY_ESC &&                 \
     (c).bind_key[DESK_BIND_CONSOLE] == DESK_KEY_TAB &&                 \
     (c).n_photos == 0 && (c).photos[0][0] == '\0')

static void test_bind_defaults(void) {
    struct desk_config c;

    printf("bind defaults:\n");
    desk_config_default(&c);
    CHECK(c.bind_key[DESK_BIND_LAUNCHER] == DESK_KEY_ESC);
    CHECK(c.bind_key[DESK_BIND_CONSOLE] == DESK_KEY_TAB);
    CHECK(c.n_photos == 0 && c.photos[0][0] == '\0');
    CHECK(desk_config_parse("", &c) == 0);
    CHECK(M2_DEFAULTS(c));
}

static void test_bind_valid_keys(void) {
    struct desk_config c;

    printf("valid binds:\n");
    CHECK(desk_config_parse("bind=launcher:q\nbind=console:x\n", &c) == 0);
    CHECK(c.bind_key[DESK_BIND_LAUNCHER] == 'q');
    CHECK(c.bind_key[DESK_BIND_CONSOLE] == 'x');
    CHECK(desk_config_parse("bind=launcher:~\nbind=console:5\n", &c) == 0);
    CHECK(c.bind_key[DESK_BIND_LAUNCHER] == '~' &&
          c.bind_key[DESK_BIND_CONSOLE] == '5');
    CHECK(desk_config_parse("bind=launcher:enter\n", &c) == 0);
    CHECK(c.bind_key[DESK_BIND_LAUNCHER] == DESK_KEY_ENTER &&
          c.bind_key[DESK_BIND_CONSOLE] == DESK_KEY_TAB);
    /* Padding around '=' and ':' is tolerated, like in v1 lines. */
    CHECK(desk_config_parse("  bind = launcher : a  \r\n", &c) == 0);
    CHECK(c.bind_key[DESK_BIND_LAUNCHER] == 'a' &&
          c.bind_key[DESK_BIND_CONSOLE] == DESK_KEY_TAB);
}

static void test_bind_invalid_lines(void) {
    struct desk_config c;
    const char *bad = "bind=menu:esc\n"
                      "bind=launcher:meta\n"
                      "bind=launcher:ab\n"
                      "bind=launcher:\n"
                      "bind=launcher\n"
                      "bind=:esc\n";

    printf("invalid binds:\n");
    CHECK(desk_config_parse(bad, &c) == 0);
    CHECK(M2_DEFAULTS(c));
    /* A bad line must not clobber a bind set by an earlier line. */
    CHECK(desk_config_parse("bind=launcher:q\nbind=launcher:bad\n", &c) == 0);
    CHECK(c.bind_key[DESK_BIND_LAUNCHER] == 'q');
}

static void test_bind_duplicates(void) {
    struct desk_config c;

    printf("duplicate binds:\n");
    CHECK(desk_config_parse("bind=console:x\nbind=launcher:esc\n"
                            "bind=launcher:tab\n", &c) == 0);
    CHECK(c.bind_key[DESK_BIND_LAUNCHER] == DESK_KEY_TAB); /* last wins */
    CHECK(c.bind_key[DESK_BIND_CONSOLE] == 'x'); /* other action intact */
    CHECK(desk_config_parse("bind=console:a\nbind=console:z\n", &c) == 0);
    CHECK(c.bind_key[DESK_BIND_CONSOLE] == 'z' &&
          c.bind_key[DESK_BIND_LAUNCHER] == DESK_KEY_ESC);
}

static void test_photo_valid(void) {
    struct desk_config c;

    printf("valid photos:\n");
    CHECK(desk_config_parse("photo=/etc/desk-logo.bmp,540,560\n", &c) == 0);
    CHECK(c.n_photos == 1);
    CHECK(strcmp(c.photos[0], "/etc/desk-logo.bmp") == 0);
    CHECK(c.photo_x[0] == 540 && c.photo_y[0] == 560);
    CHECK(desk_config_parse("photo=/a.bmp,0,0\nphoto=/b.bmp,1023,767\n",
                            &c) == 0);
    CHECK(c.n_photos == 2 && strcmp(c.photos[1], "/b.bmp") == 0);
    CHECK(c.photo_x[1] == 1023 && c.photo_y[1] == 767);
    CHECK(desk_config_parse("  photo = /w.bmp , 7 , 9  \r\n", &c) == 0);
    CHECK(c.n_photos == 1 && c.photo_x[0] == 7 && c.photo_y[0] == 9);
}

static void test_photo_limit(void) {
    struct desk_config c;

    printf("photo limit:\n");
    CHECK(desk_config_parse("photo=/p1.bmp,1,1\nphoto=/p2.bmp,2,2\n"
                            "photo=/p3.bmp,3,3\nphoto=/p4.bmp,4,4\n"
                            "photo=/p5.bmp,5,5\n", &c) == 0);
    CHECK(c.n_photos == DESK_MAX_PHOTOS);
    CHECK(strcmp(c.photos[DESK_MAX_PHOTOS - 1], "/p4.bmp") == 0);
    CHECK(c.photo_x[DESK_MAX_PHOTOS - 1] == 4 &&
          c.photo_y[DESK_MAX_PHOTOS - 1] == 4);
}

static void test_photo_invalid_lines(void) {
    struct desk_config c;
    const char *bad = "photo=/a.bmp,-1,5\n"
                      "photo=/a.bmp,5,-2\n"
                      "photo=,10,10\n"
                      "photo=/a.bmp,10\n"
                      "photo=/a.bmp\n"
                      "photo=/a.bmp,abc,5\n"
                      "photo=/a.bmp,5,x\n"
                      "photo=/a.bmp,5,5,5\n"
                      "photo=/a.bmp,99999999999999999999,5\n";

    printf("invalid photos:\n");
    CHECK(desk_config_parse(bad, &c) == 0);
    CHECK(c.n_photos == 0); /* every line above was dropped whole */
}

static void test_photo_path_too_long(void) {
    struct desk_config c;
    char path[DESK_PATH_MAX + 2];
    char text[DESK_PATH_MAX + 32];

    printf("photo path limits:\n");
    memset(path, 'a', sizeof path);
    path[DESK_PATH_MAX] = '\0'; /* exactly DESK_PATH_MAX chars: too long */
    snprintf(text, sizeof text, "photo=%s,5,5\n", path);
    CHECK(desk_config_parse(text, &c) == 0);
    CHECK(c.n_photos == 0); /* ignored, never silently truncated */

    path[DESK_PATH_MAX - 1] = '\0'; /* longest accepted path */
    snprintf(text, sizeof text, "photo=%s,5,5\n", path);
    CHECK(desk_config_parse(text, &c) == 0);
    CHECK(c.n_photos == 1 &&
          (int)strlen(c.photos[0]) == DESK_PATH_MAX - 1);
}

static void test_m2_lines_coexist_with_v1(void) {
    struct desk_config c;
    const char *text = "panel_height=40\n"
                       "app=onyxcalc\n"
                       "bind=launcher:z\n"
                       "photo=/a.bmp,1,2\n"
                       "photo=broken\n";

    printf("m2 lines next to v1 lines:\n");
    CHECK(desk_config_parse(text, &c) == 0);
    CHECK(c.panel_height == 40 && c.n_apps == 1 &&
          strcmp(c.apps[0], "onyxcalc") == 0);
    CHECK(c.bind_key[DESK_BIND_LAUNCHER] == 'z');
    CHECK(c.n_photos == 1 && strcmp(c.photos[0], "/a.bmp") == 0);
    CHECK(c.photo_x[0] == 1 && c.photo_y[0] == 2);
    /* A malformed v1 value still fails the whole parse and resets
     * every m2 field the earlier lines had set. */
    CHECK(desk_config_parse("bind=launcher:q\nphoto=/a.bmp,1,2\n"
                            "panel_height=abc\n", &c) == -1);
    CHECK(M2_DEFAULTS(c));
}

int main(void) {
    test_bind_defaults();
    test_bind_valid_keys();
    test_bind_invalid_lines();
    test_bind_duplicates();
    test_photo_valid();
    test_photo_limit();
    test_photo_invalid_lines();
    test_photo_path_too_long();
    test_m2_lines_coexist_with_v1();
    printf("\ntest_binds: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
