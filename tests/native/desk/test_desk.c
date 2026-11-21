/*
 * test_desk.c - desk_run session tests: startup failures, autostart.
 *
 * Startup contract: fb open/GET_INFO/CLAIM failures exit with code 3
 * and never RELEASE a claim that is not ours; the success path holds
 * the claim across the autostart spawn and releases exactly once.
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

static void test_no_fb(void) {
    struct desk_config c;
    const char *want[] = {"DESK_EXIT 3"};

    fake_install();
    fake_config(&c, 2);
    F.open_fail = 1;
    CHECK(desk_run(&c, 0, 0) == 3);
    CHECK(markers_exact(want, 1));
    CHECK(F.opens == 1 && F.closes == 0 && F.n_ioctls == 0);
}

static void test_no_backend(void) {
    const char *want[] = {"DESK_EXIT 3"};

    fake_reset();
    desk_set_io(0);
    desk_set_marker_sink(fake_sink);
    CHECK(desk_run(0, 0, 0) == 3);
    CHECK(markers_exact(want, 1));
    CHECK(F.opens == 0);
}

static void test_getinfo_fails(void) {
    struct desk_config c;
    const char *want[] = {"DESK_EXIT 3"};

    fake_install();
    fake_config(&c, 2);
    F.getinfo_fail = 1;
    CHECK(desk_run(&c, 0, 0) == 3);
    CHECK(markers_exact(want, 1));
    CHECK(F.claims == 0 && F.releases == 0 && F.closes == 1);
}

static void test_bad_geometry(void) {
    struct desk_config c;
    const char *want[] = {"DESK_EXIT 3"};

    fake_install();
    fake_config(&c, 2);
    F.info.bpp = 8; /* unsupported depth */
    CHECK(desk_run(&c, 0, 0) == 3);
    CHECK(markers_exact(want, 1));
    CHECK(F.claims == 0 && F.releases == 0 && F.closes == 1);
}

static void test_claim_refused(void) {
    struct desk_config c;
    const char *want[] = {"DESK_EXIT 3"};

    fake_install();
    fake_config(&c, 2);
    F.claim_fail = 1;
    CHECK(desk_run(&c, 0, 0) == 3);
    CHECK(markers_exact(want, 1));
    CHECK(F.releases == 0 && F.closes == 1 && F.n_ioctls == 2);
}

static void test_autostart_once(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "APP_STARTED osnake 7",
                          "APP_RETURN 0", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    CHECK(desk_run(&c, "osnake", 1) == 0);
    CHECK(markers_exact(want, 4));
    CHECK(strcmp(F.spawn_path, "/bin/osnake") == 0);
    CHECK(F.wait_calls == 1 && F.closes == 1);
    /* Claim held across the spawn: GET_INFO, CLAIM, RELEASE only. */
    CHECK(F.n_ioctls == 3 && release_is_last_ioctl());
    /* One repaint of the 4-row frame before the launch. */
    CHECK(F.writes == 4 && F.lseeks == 4);
}

static void test_autostart_name_too_long(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, "name-that-overflows-the-desk-name-max", 1) == 0);
    CHECK(markers_exact(want, 2));
    CHECK(F.spawn_calls == 0 && F.wait_calls == 0);
    CHECK(release_is_last_ioctl());
}

static void test_alloc_fails_after_claim(void) {
    struct desk_config c;
    const char *want[] = {"DESK_EXIT 3"};

    fake_install();
    fake_config(&c, 2);
    F.alloc_fail = 1;
    CHECK(desk_run(&c, 0, 0) == 3);
    CHECK(markers_exact(want, 1)); /* no DESK_READY: no frame to draw on */
    /* The claim was ours: released exactly once, then closed. */
    CHECK(F.releases == 1 && F.closes == 1);
    CHECK(release_is_last_ioctl());
    CHECK(F.allocs == 1 && F.frees == 0);
}

static void test_present_write_fails(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 3"};

    fake_install();
    fake_config(&c, 2);
    F.write_fail = 1;
    CHECK(desk_run(&c, 0, 0) == 3);
    CHECK(markers_exact(want, 2));
    /* The claim is ours: released exactly once, fb closed. */
    CHECK(F.releases == 1 && F.closes == 1);
    CHECK(release_is_last_ioctl());
}

static void test_present_lseek_fails(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 3"};

    fake_install();
    fake_config(&c, 2);
    F.lseek_fail = 1;
    CHECK(desk_run(&c, 0, 0) == 3);
    CHECK(markers_exact(want, 2));
    CHECK(F.releases == 1 && F.closes == 1);
    CHECK(release_is_last_ioctl());
}

static void test_autostart_spawn_fail_once(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    F.spawn_pid = -1;
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, "osnake", 1) == 0);
    CHECK(markers_exact(want, 2)); /* once, but the app never ran */
    CHECK(F.spawn_calls == 1 && F.wait_calls == 0);
    CHECK(F.writes == 8); /* fell back to the launcher and repainted */
    CHECK(release_is_last_ioctl());
}

static void test_wait_retry_parked_waiter(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "APP_STARTED osnake 7",
                          "APP_RETURN 5", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    F.wait_status = 5;
    F.wait_retry_first = 1; /* first waitpid answers -ENOENT */
    CHECK(desk_run(&c, "osnake", 1) == 0);
    CHECK(markers_exact(want, 4)); /* APP_RETURN carries the real code */
    CHECK(F.wait_calls == 2);      /* the parked waiter was retried */
    CHECK(release_is_last_ioctl());
}

int main(void) {
    test_no_fb();
    test_no_backend();
    test_getinfo_fails();
    test_bad_geometry();
    test_claim_refused();
    test_autostart_once();
    test_wait_retry_parked_waiter();
    test_autostart_name_too_long();
    test_alloc_fails_after_claim();
    test_autostart_spawn_fail_once();
    test_present_write_fails();
    test_present_lseek_fails();
    printf("\ntest_desk: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
