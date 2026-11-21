/*
 * test_desk_fb.c - GET_INFO validation tests for the desk loop.
 *
 * present() writes row y at offset y*pitch, so a device whose last row
 * (height-1)*pitch + width*bytes overruns the reported size must be
 * rejected up front - the packed w*h*bytes metric alone is not enough.
 */
#include "fake_desk.h"

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

static void test_pitch_overruns_rejected(void) {
    struct desk_config c;
    const char *want[] = {"DESK_EXIT 3"};

    fake_install();
    fake_config(&c, 2);
    /* packed = 8*4*2 = 64 fits size, but the last row lands at
     * 3*32 + 16 = 112: the device cannot hold the final present row. */
    F.info.pitch = 32;
    F.info.size = 64;
    fake_event(0, 0, DESK_BTN_EXIT); /* exits if wrongly accepted */
    CHECK(desk_run(&c, 0, 0) == 3);
    CHECK(markers_exact(want, 1));
    CHECK(F.claims == 0 && F.releases == 0 && F.closes == 1);
    CHECK(F.writes == 0); /* never painted on an unusable device */
}

static void test_pitch_fits_accepted(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    /* Same wide pitch, but a device big enough for the last row. */
    F.info.pitch = 32;
    F.info.size = 128; /* 3*32 + 16 = 112 <= 128 */
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(markers_exact(want, 2));
    CHECK(F.writes == 4 && F.lseeks == 4);
    CHECK(release_is_last_ioctl());
}

int main(void) {
    test_pitch_overruns_rejected();
    test_pitch_fits_accepted();
    printf("\ntest_desk_fb: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
