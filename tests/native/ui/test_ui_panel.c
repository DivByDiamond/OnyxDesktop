/*
 * test_ui_panel.c - panel drawing, text and defensive no-op tests.
 *
 *   make -C tests/native test
 *
 * The panel must fill exactly panel_h rows of the packed frame (16 and
 * 32 bpp), draw deterministic glyphs, and never write for empty
 * geometry, unsupported bpp or NULL inputs.
 */
#include "test_ui_util.h"

static void test_panel_fill(int bpp) {
    struct fb_info i;

    info_init(&i, 20, 24, bpp);
    reset();
    ui_draw_panel(frame, &i, 9, NULL);
    CHECK(rect_is(&i, 0, 0, 20, 9, UI_PANEL_BG));
    CHECK(raw_is(&i, 0, 9));   /* the row below the panel is untouched */
    CHECK(raw_is(&i, 19, 23));
    CHECK(guard_ok(frame_len(&i)));

    reset();                   /* panel_h taller than the screen clamps */
    ui_draw_panel(frame, &i, 500, NULL);
    CHECK(rect_is(&i, 0, 0, 20, 24, UI_PANEL_BG));
    CHECK(guard_ok(frame_len(&i)));
}

static void test_panel_text(int bpp) {
    struct fb_info i;
    int y = (UI_ROW_H - UI_GLYPH_H) / 2;

    info_init(&i, 24, UI_ROW_H, bpp);
    reset();
    ui_draw_panel(frame, &i, UI_ROW_H, "A");
    /* 'A' row 3 is "#####": five lit pixels at the glyph origin. */
    CHECK(px_is(&i, UI_PAD_X + 0, y + 3, UI_PANEL_TEXT));
    CHECK(px_is(&i, UI_PAD_X + 1, y + 3, UI_PANEL_TEXT));
    CHECK(px_is(&i, UI_PAD_X + 2, y + 3, UI_PANEL_TEXT));
    CHECK(px_is(&i, UI_PAD_X + 3, y + 3, UI_PANEL_TEXT));
    CHECK(px_is(&i, UI_PAD_X + 4, y + 3, UI_PANEL_TEXT));
    /* 'A' row 0 is ".###.": the outer columns stay background. */
    CHECK(px_is(&i, UI_PAD_X, y, UI_PANEL_BG));
    CHECK(px_is(&i, UI_PAD_X + 1, y, UI_PANEL_TEXT));
    CHECK(px_is(&i, UI_PAD_X + 2, y, UI_PANEL_TEXT));
    CHECK(px_is(&i, UI_PAD_X + 4, y, UI_PANEL_BG));
    CHECK(guard_ok(frame_len(&i)));

    reset();                   /* lowercase maps onto the same glyph */
    ui_draw_panel(frame, &i, UI_ROW_H, "a");
    CHECK(px_is(&i, UI_PAD_X + 2, y + 3, UI_PANEL_TEXT));
    CHECK(guard_ok(frame_len(&i)));
}

static void test_panel_long_name_clips(void) {
    struct fb_info i;
    char name[120];
    int k;

    info_init(&i, 12, UI_ROW_H, 16);
    for (k = 0; k < (int)sizeof name - 1; k++) {
        name[k] = 'x';
    }
    name[sizeof name - 1] = '\0';
    reset();
    ui_draw_panel(frame, &i, UI_ROW_H, name);
    CHECK(guard_ok(frame_len(&i)));
    CHECK(rect_is(&i, 0, 0, 12, 1, UI_PANEL_BG));
}

static void test_zero_geometry(void) {
    struct fb_info i;
    struct desk_config c;

    config_init(&c, 8);
    reset();
    info_init(&i, 0, 40, 16);
    ui_draw_panel(frame, &i, 8, "x");
    ui_draw_launcher(frame, &i, &c, 0);
    info_init(&i, 40, 0, 16);
    ui_draw_panel(frame, &i, 8, "x");
    ui_draw_launcher(frame, &i, &c, 0);
    info_init(&i, 0, 0, 32);
    ui_draw_panel(frame, &i, 8, "x");
    ui_draw_launcher(frame, &i, &c, 0);
    CHECK(guard_ok(0));        /* nothing was written anywhere */
}

static void test_unsupported_bpp(void) {
    struct fb_info i;
    struct desk_config c;

    config_init(&c, 8);
    reset();
    info_init(&i, 20, 20, 8);
    ui_draw_panel(frame, &i, 8, "x");
    ui_draw_launcher(frame, &i, &c, 0);
    info_init(&i, 20, 20, 24);
    ui_draw_panel(frame, &i, 8, "x");
    ui_draw_launcher(frame, &i, &c, 0);
    CHECK(guard_ok(0));
}

static void test_null_inputs(void) {
    struct fb_info i;
    struct desk_config c;

    info_init(&i, 20, 20, 16);
    config_init(&c, 8);
    reset();
    ui_draw_panel(NULL, &i, 8, "x");
    ui_draw_panel(frame, NULL, 8, "x");
    ui_draw_launcher(NULL, &i, &c, 0);
    ui_draw_launcher(frame, NULL, &c, 0);
    ui_draw_launcher(frame, &i, NULL, 0);
    CHECK(guard_ok(0));
    CHECK(ui_hit_test(NULL, 8, 2, 0, 10) == -1);
}

int main(void) {
    test_panel_fill(16);
    test_panel_fill(32);
    test_panel_text(16);
    test_panel_text(32);
    test_panel_long_name_clips();
    test_zero_geometry();
    test_unsupported_bpp();
    test_null_inputs();
    printf("\ntest_ui_panel: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
