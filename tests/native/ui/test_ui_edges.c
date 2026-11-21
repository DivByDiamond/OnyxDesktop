/*
 * test_ui_edges.c - host tests for ui_hit_test geometry.
 *
 *   make -C tests/native test
 *
 * Hit testing maps a click inside a launcher row to the app index and
 * returns -1 for the panel strip, the empty area below the last row and
 * any coordinate outside the frame.
 */
#include <stdio.h>
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

static struct fb_info make_info(unsigned w, unsigned h, int bpp) {
    struct fb_info i;

    i.width = w;
    i.height = h;
    i.bpp = (unsigned)bpp;
    i.pitch = w * (unsigned)(bpp / 8);
    i.size = i.pitch * h;
    return i;
}

static void test_rows(void) {
    struct fb_info i = make_info(40, 60, 16);
    const int top = 10;

    printf("launcher rows:\n");
    CHECK(ui_hit_test(&i, top, 3, 0, top) == 0);
    CHECK(ui_hit_test(&i, top, 3, 39, top + UI_ROW_H - 1) == 0);
    CHECK(ui_hit_test(&i, top, 3, 0, top + UI_ROW_H) == 1);
    CHECK(ui_hit_test(&i, top, 3, 0, top + 2 * UI_ROW_H) == 2);
    CHECK(ui_hit_test(&i, top, 3, 0, top + 3 * UI_ROW_H - 1) == 2);
    CHECK(ui_hit_test(&i, top, 3, 0, top + 3 * UI_ROW_H) == -1);
    /* a partially visible row is still clickable */
    CHECK(ui_hit_test(&i, top, 8, 0, 59) == 4);
}

static void test_panel_region(void) {
    struct fb_info i = make_info(40, 60, 32);

    printf("panel region:\n");
    CHECK(ui_hit_test(&i, 10, 3, 0, 0) == -1);
    CHECK(ui_hit_test(&i, 10, 3, 0, 9) == -1);
    CHECK(ui_hit_test(&i, 0, 3, 0, 0) == 0);   /* no panel: row 0 at y=0 */
}

static void test_out_of_frame(void) {
    struct fb_info i = make_info(40, 60, 16);

    printf("out of frame:\n");
    CHECK(ui_hit_test(&i, 10, 3, -1, 12) == -1);
    CHECK(ui_hit_test(&i, 10, 3, 40, 12) == -1);
    CHECK(ui_hit_test(&i, 10, 3, 0, -1) == -1);
    CHECK(ui_hit_test(&i, 10, 3, 0, 60) == -1);
    CHECK(ui_hit_test(&i, -5, 3, 0, 0) == 0);  /* negative panel clamps to 0 */
}

static void test_bad_counts(void) {
    struct fb_info i = make_info(40, 60, 16);
    /* Tall enough that rows 7 and 8 are inside the frame, so the two
     * clamp checks below exercise the n_apps limit, not the frame
     * bounds (out-of-frame y is covered by test_out_of_frame). */
    struct fb_info tall = make_info(40, 200, 16);

    printf("app counts:\n");
    CHECK(ui_hit_test(&i, 10, 0, 0, 12) == -1);
    CHECK(ui_hit_test(&i, 10, -1, 0, 12) == -1);
    CHECK(ui_hit_test(&tall, 10, 1000, 0, 10 + DESK_MAX_APPS * UI_ROW_H) == -1);
    CHECK(ui_hit_test(&tall, 10, 1000, 0, 10 + (DESK_MAX_APPS - 1) * UI_ROW_H) ==
          DESK_MAX_APPS - 1);
}

static void test_zero_geometry(void) {
    struct fb_info i = make_info(0, 60, 16);
    struct fb_info j = make_info(40, 0, 16);

    printf("zero geometry:\n");
    CHECK(ui_hit_test(&i, 10, 3, 0, 12) == -1);
    CHECK(ui_hit_test(&j, 10, 3, 0, 12) == -1);
    CHECK(ui_hit_test(&i, 10, 3, 0, 0) == -1);
}

int main(void) {
    test_rows();
    test_panel_region();
    test_out_of_frame();
    test_bad_counts();
    test_zero_geometry();
    printf("\ntest_ui_edges: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
