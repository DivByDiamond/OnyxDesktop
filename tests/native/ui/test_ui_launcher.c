/*
 * test_ui_launcher.c - launcher rows, selection highlight and clipping.
 *
 *   make -C tests/native test
 *
 * The launcher clears the area below the panel, draws one UI_ROW_H row
 * per configured app, highlights only a valid selected index, and clips
 * rows that run past the bottom of the frame.
 */
#include "test_ui_util.h"

static void test_launcher_rows(int bpp) {
    struct fb_info i;
    struct desk_config c;

    info_init(&i, 24, 40, bpp);
    config_init(&c, 8);
    reset();
    ui_draw_launcher(frame, &i, &c, 1);
    CHECK(raw_is(&i, 0, 7));                        /* panel rows untouched */
    /* Glyph rows are 10..16 and 22..28, so the strips below the text are
     * pure row background across the full frame width. */
    CHECK(rect_is(&i, 0, 17, 24, 3, UI_BG));        /* row 0 */
    CHECK(rect_is(&i, 0, 29, 24, 3, UI_SELECTED_BG)); /* row 1 selected */
    CHECK(rect_is(&i, 0, 32, 24, 8, UI_BG));        /* empty area cleared */
    CHECK(has_px(&i, 0, 8, 24, UI_ROW_H, UI_TEXT));
    CHECK(has_px(&i, 0, 20, 24, UI_ROW_H, UI_SELECTED_TEXT));
    CHECK(guard_ok(frame_len(&i)));

    reset();                   /* selected out of range: no highlight */
    ui_draw_launcher(frame, &i, &c, -1);
    CHECK(rect_is(&i, 0, 17, 24, 3, UI_BG));
    CHECK(rect_is(&i, 0, 29, 24, 3, UI_BG));
    CHECK(!has_px(&i, 0, 8, 24, 32, UI_SELECTED_BG));
    CHECK(has_px(&i, 0, 8, 24, UI_ROW_H, UI_TEXT));
    CHECK(guard_ok(frame_len(&i)));
    ui_draw_launcher(frame, &i, &c, 7);
    CHECK(rect_is(&i, 0, 17, 24, 3, UI_BG));
    CHECK(!has_px(&i, 0, 8, 24, 32, UI_SELECTED_BG));
    CHECK(guard_ok(frame_len(&i)));
}

static void test_launcher_clips(void) {
    struct fb_info i;
    struct desk_config c;
    int k;

    info_init(&i, 20, 16, 16);
    config_init(&c, 14);
    c.n_apps = 8;
    for (k = 2; k < 8; k++) {
        strcpy(c.apps[k], "app");
    }
    reset();
    ui_draw_launcher(frame, &i, &c, 0);
    CHECK(raw_is(&i, 0, 13));                  /* panel boundary respected */
    CHECK(rect_is(&i, 0, 14, 20, 2, UI_SELECTED_BG)); /* only 2 rows visible */
    CHECK(guard_ok(frame_len(&i)));

    reset();
    ui_draw_launcher(frame, &i, &c, 7);        /* offscreen selection */
    CHECK(rect_is(&i, 0, 14, 20, 2, UI_BG));
    CHECK(guard_ok(frame_len(&i)));
}

static void test_launcher_empty_list(void) {
    struct fb_info i;
    struct desk_config c;

    info_init(&i, 20, 30, 16);
    config_init(&c, 6);
    c.n_apps = 0;
    reset();
    ui_draw_launcher(frame, &i, &c, 0);
    CHECK(rect_is(&i, 0, 6, 20, 24, UI_BG));
    CHECK(raw_is(&i, 0, 5));
    CHECK(guard_ok(frame_len(&i)));
}

int main(void) {
    test_launcher_rows(16);
    test_launcher_rows(32);
    test_launcher_clips();
    test_launcher_empty_list();
    printf("\ntest_ui_launcher: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
