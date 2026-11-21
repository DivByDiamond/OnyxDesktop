/*
 * test_desk_events.c - desk_run mouse-loop tests.
 *
 * The fake event queue drives the launcher: a front-edge click maps
 * through ui_hit_test to the configured app, spawn failures fall back
 * to the launcher without APP_STARTED, button 2 and the stop request
 * (signal path) exit cleanly, and the RELEASE discipline holds.
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

static void test_click_launch_and_return(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "APP_STARTED osnake 9",
                          "APP_RETURN 0", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    F.spawn_pid = 9;
    fake_event(0, 3, DESK_BTN_CLICK); /* row 0: y=3 is under panel 2 */
    fake_event(0, 3, 0);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(markers_exact(want, 4));
    CHECK(strcmp(F.spawn_path, "/bin/osnake") == 0);
    CHECK(F.wait_calls == 1);
    /* Initial paint + repaint after the app returned to the loop. */
    CHECK(F.writes == 8 && F.lseeks == 8);
    CHECK(release_is_last_ioctl());
}

static void test_click_second_app(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "APP_STARTED ochat-gui 7",
                          "APP_RETURN 0", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    F.info.height = 32;
    F.info.size = 8u * 32u * 2u;
    fake_event(0, 20, DESK_BTN_CLICK); /* row 1 */
    fake_event(0, 20, 0);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(markers_exact(want, 4));
    CHECK(strcmp(F.spawn_path, "/bin/ochat-gui") == 0);
}

static void test_spawn_fail_returns_to_launcher(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    F.spawn_pid = -1;
    fake_event(0, 3, DESK_BTN_CLICK);
    fake_event(0, 3, 0);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(markers_exact(want, 2)); /* no APP_STARTED: the app never ran */
    CHECK(F.spawn_calls == 1 && F.wait_calls == 0);
    CHECK(F.writes == 8); /* fell back to the launcher and repainted */
    CHECK(release_is_last_ioctl());
}

static void test_panel_click_ignored(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    fake_event(0, 0, DESK_BTN_CLICK); /* y=0 is inside the panel strip */
    fake_event(0, 0, 0);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(markers_exact(want, 2));
    CHECK(F.spawn_calls == 0);
}

static void test_button2_exits(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(markers_exact(want, 2));
    CHECK(F.releases == 1 && F.closes == 1);
    CHECK(F.writes == 4); /* launcher painted once, never overdrawn */
}

static void test_mouse_error_exits(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    F.mouse_fail = 1;
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(markers_exact(want, 2));
    CHECK(release_is_last_ioctl());
}

static void test_stop_request_exits(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    F.stop_on_poll = 1; /* what the SIGTERM/SIGINT handler does */
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(markers_exact(want, 2));
    CHECK(release_is_last_ioctl());
}

static void test_default_config_built_in_list(void) {
    const char *want[] = {"DESK_READY", "APP_STARTED osnake 7",
                          "APP_RETURN 0", "DESK_EXIT 0"};

    fake_install();
    /* NULL config: built-in defaults, and the built-in app list is
     * live: a click below the default 24 px panel launches osnake. */
    F.info.height = 32;
    F.info.size = 8u * 32u * 2u;
    fake_event(0, 28, DESK_BTN_CLICK); /* row 0 under the 24 px panel */
    fake_event(0, 28, 0);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(0, 0, 0) == 0);
    CHECK(markers_exact(want, 4));
    CHECK(strcmp(F.spawn_path, "/bin/osnake") == 0);
    CHECK(release_is_last_ioctl());
}

static void test_click_once_exits_after_app(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "APP_STARTED osnake 7",
                          "APP_RETURN 0", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    fake_event(0, 3, DESK_BTN_CLICK);
    fake_event(0, 3, 0); /* release: never reached, once exits first */
    CHECK(desk_run(&c, 0, 1) == 0);
    CHECK(markers_exact(want, 4));
    CHECK(strcmp(F.spawn_path, "/bin/osnake") == 0);
    CHECK(F.wait_calls == 1);
    CHECK(F.writes == 4); /* launcher painted once, no repaint after */
    CHECK(release_is_last_ioctl());
}

static void test_spawn_fail_once_returns_to_launcher(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 0"};

    fake_install();
    fake_config(&c, 2);
    F.spawn_pid = -1;
    fake_event(0, 3, DESK_BTN_CLICK);
    fake_event(0, 3, 0);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 1) == 0);
    CHECK(markers_exact(want, 2)); /* no APP_STARTED, no early exit */
    CHECK(F.spawn_calls == 1 && F.wait_calls == 0);
    CHECK(F.writes == 8); /* back to the launcher and repainted */
    CHECK(release_is_last_ioctl());
}

int main(void) {
    test_click_launch_and_return();
    test_click_second_app();
    test_spawn_fail_returns_to_launcher();
    test_click_once_exits_after_app();
    test_spawn_fail_once_returns_to_launcher();
    test_panel_click_ignored();
    test_button2_exits();
    test_mouse_error_exits();
    test_stop_request_exits();
    test_default_config_built_in_list();
    printf("\ntest_desk_events: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
