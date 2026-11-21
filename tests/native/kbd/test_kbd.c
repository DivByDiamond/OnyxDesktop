/*
 * test_kbd.c - keybind dispatch and tty discipline tests.
 *
 * kbd_dispatch maps one raw byte against the config binds; through
 * desk_run the key fake proves the whole contract: a bound key
 * reaches the action sink, an unbound byte is dropped, the tty is
 * raw right after CLAIM, restored before spawn and re-armed after
 * wait, no byte is read while a child runs, and a refused raw leaves
 * a mouse-only desk whose launches still work.
 */
#include <stdio.h>
#include <unistd.h>
#include "fake_kbd.h"

static int checks;
static int failures;

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (cond) {                                                      \
            printf("  ok  %s\n", #cond);                                 \
        } else {                                                        \
            printf("  FAIL %s (%s:%d)\n", #cond, __FILE__, __LINE__);    \
            failures++;                                                  \
        }                                                                \
    } while (0)

static void test_dispatch(void) {
    struct desk_config c;

    desk_config_default(&c);
    CHECK(kbd_dispatch(&c, DESK_KEY_ESC) == DESK_ACT_LAUNCHER);
    CHECK(kbd_dispatch(&c, DESK_KEY_TAB) == DESK_ACT_CONSOLE);
    CHECK(kbd_dispatch(&c, 'z') == DESK_ACT_NONE);
    CHECK(kbd_dispatch(&c, -1) == DESK_ACT_NONE);
    CHECK(kbd_dispatch(0, DESK_KEY_ESC) == DESK_ACT_NONE);
    CHECK(desk_config_parse("bind=launcher:q\nbind=console:x\n", &c) == 0);
    CHECK(kbd_dispatch(&c, 'q') == DESK_ACT_LAUNCHER);
    CHECK(kbd_dispatch(&c, 'x') == DESK_ACT_CONSOLE);
    CHECK(kbd_dispatch(&c, DESK_KEY_ESC) == DESK_ACT_NONE);
    memset(&c, 0, sizeof c);
    CHECK(kbd_dispatch(&c, 0) == DESK_ACT_NONE); /* zero slot: unbound */
}

static void test_launcher_key(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "DESK_EXIT 0"};

    kbd_install();
    kbd_config(&c, 2);
    kbd_key(DESK_KEY_ESC);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(markers_exact(want, 2)); /* a bound key changes no markers */
    CHECK(K.n_actions == 1 && K.actions[0] == DESK_ACT_LAUNCHER);
    CHECK(K.reads == 1 && F.spawn_calls == 0);
    CHECK(K.raw_calls == 1 && K.restore_calls == 1);
}

static void test_console_key(void) {
    struct desk_config c;

    kbd_install();
    kbd_config(&c, 2);
    kbd_key(DESK_KEY_TAB);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(K.n_actions == 1 && K.actions[0] == DESK_ACT_CONSOLE);
    CHECK(K.reads == 1 && F.spawn_calls == 0);
}

static void test_unbound_key(void) {
    struct desk_config c;

    kbd_install();
    kbd_config(&c, 2);
    kbd_key('z');
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(K.n_actions == 0 && K.reads == 1);
    CHECK(K.polls == 1 && F.spawn_calls == 0);
}

static void test_custom_bind_key(void) {
    struct desk_config c;

    kbd_install();
    CHECK(desk_config_parse("panel_height=2\nbind=launcher:q\n", &c) == 0);
    kbd_key('q');
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(K.n_actions == 1 && K.actions[0] == DESK_ACT_LAUNCHER);
    CHECK(K.reads == 1);
}

static void test_tty_discipline(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "APP_STARTED osnake 7",
                          "APP_RETURN 0", "DESK_EXIT 0"};

    kbd_install();
    kbd_config(&c, 2);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, "osnake", 0) == 0);
    CHECK(markers_exact(want, 4));
    /* raw at claim, restore before spawn, raw after wait, restore at
     * the way out: two switches each. */
    CHECK(K.raw_calls == 2 && K.restore_calls == 2);
    CHECK(K.raw_marker[0] == 0);    /* armed before DESK_READY */
    CHECK(K.restore_marker[0] == 1); /* released before APP_STARTED */
    CHECK(K.raw_marker[1] == 2);     /* re-armed before APP_RETURN */
    CHECK(K.reads_in_child == 0 && K.reads == 0);
    CHECK(K.polls == 1);
}

static void test_raw_refused_degrades(void) {
    struct desk_config c;
    const char *want[] = {"DESK_READY", "APP_STARTED osnake 7",
                          "APP_RETURN 0", "DESK_EXIT 0"};

    kbd_install();
    kbd_config(&c, 2);
    K.raw_fail = 1;
    kbd_key(DESK_KEY_ESC); /* the bind path is off: never read */
    fake_event(0, 3, DESK_BTN_CLICK);
    fake_event(0, 3, 0);
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(markers_exact(want, 4)); /* the mouse launch is untouched */
    CHECK(K.polls == 0 && K.reads == 0 && K.n_actions == 0);
    CHECK(K.restore_calls == 0); /* never raw, never restored */
    CHECK(release_is_last_ioctl());
}

static void test_spawn_fail_keeps_raw(void) {
    struct desk_config c;

    kbd_install();
    kbd_config(&c, 2);
    F.spawn_pid = -1;
    fake_event(0, 3, DESK_BTN_CLICK); /* launch fails, desk stays up */
    fake_event(0, 3, 0);
    kbd_key(DESK_KEY_ESC); /* binds must still work afterwards */
    fake_event(0, 0, DESK_BTN_EXIT);
    CHECK(desk_run(&c, 0, 0) == 0);
    CHECK(K.n_actions == 1 && K.actions[0] == DESK_ACT_LAUNCHER);
    CHECK(K.raw_calls == 2 && K.restore_calls == 2);
    CHECK(K.reads == 1 && K.reads_in_child == 0);
}

static void test_default_sink_notes_stderr(void) {
    struct desk_config c;
    char buf[128];
    FILE *cap;
    int saved;
    size_t n;

    kbd_install();
    kbd_config(&c, 2);
    kbd_set_action_sink(0); /* the shipped default: a stderr note */
    kbd_key(DESK_KEY_ESC);
    fake_event(0, 0, DESK_BTN_EXIT);
    fflush(stderr);
    saved = dup(2);
    cap = tmpfile();
    dup2(fileno(cap), 2);
    CHECK(desk_run(&c, 0, 0) == 0);
    fflush(stderr);
    dup2(saved, 2);
    close(saved);
    rewind(cap); /* the writes advanced the shared offset */
    n = fread(buf, 1, sizeof buf - 1, cap);
    buf[n] = '\0';
    fclose(cap);
    CHECK(n > 0);
    CHECK(strstr(buf, "desk: key action launcher") != NULL);
}

int main(void) {
    test_dispatch();
    test_launcher_key();
    test_console_key();
    test_unbound_key();
    test_custom_bind_key();
    test_tty_discipline();
    test_raw_refused_degrades();
    test_spawn_fail_keeps_raw();
    test_default_sink_notes_stderr();
    printf("\ntest_kbd: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
