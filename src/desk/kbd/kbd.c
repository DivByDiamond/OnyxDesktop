/*
 * kbd.c - the desk keyboard pass: poll, read, dispatch.
 *
 * Everything here goes through the desk_io seams, so host tests
 * inject bytes without a tty and the target table (io.c) talks to
 * the real one. The tty mode itself is the session's business
 * (desk.c); this module only turns bytes into actions.
 */
#include <stdio.h>
#include "kbd.h"

/* poll(0): the desk pass is already paced by io->delay() (20 ms on
 * target, io.c), so waiting here would only slow the mouse path; a
 * key pressed mid-pass is picked up at the next pass. */
#define KBD_POLL_MS 0

static void (*g_sink)(desk_action_t);

void kbd_set_action_sink(void (*sink)(desk_action_t action)) {
    g_sink = sink;
}

desk_action_t kbd_dispatch(const struct desk_config *c, int ch) {
    if (c == NULL) return DESK_ACT_NONE;
    if (c->bind_key[DESK_BIND_LAUNCHER] != 0 &&
        ch == (int)(unsigned char)c->bind_key[DESK_BIND_LAUNCHER])
        return DESK_ACT_LAUNCHER;
    if (c->bind_key[DESK_BIND_CONSOLE] != 0 &&
        ch == (int)(unsigned char)c->bind_key[DESK_BIND_CONSOLE])
        return DESK_ACT_CONSOLE;
    return DESK_ACT_NONE;
}

/* The M2 stub: the overlays are task 6; until then every bound key
 * is noted on stderr so the bind path stays observable in a smoke
 * log. One line per key, nothing on the unbound path. */
static const char *action_name(desk_action_t a) {
    if (a == DESK_ACT_LAUNCHER) return "launcher";
    if (a == DESK_ACT_CONSOLE) return "console";
    return "none";
}

static void default_sink(desk_action_t a) {
    fprintf(stderr, "desk: key action %s\n", action_name(a));
}

void kbd_pass(const struct desk_config *c, const struct desk_io *io) {
    desk_action_t a = DESK_ACT_NONE;

    if (io->tty_poll(KBD_POLL_MS) > 0) a = kbd_dispatch(c, io->kbd_read());
    if (a == DESK_ACT_NONE) return;
    if (g_sink) g_sink(a);
    else default_sink(a);
}

/* The session helpers below are the whole tty lifecycle: the desk
 * owns the tty except while a launched child runs. */
int kbd_arm(const struct desk_io *io) {
    if (io->tty_raw() != 0) return 1;
    fprintf(stderr, "desk: tty raw refused, keybinds off\n");
    return 0;
}

void kbd_yield(const struct desk_io *io, int kbd) {
    if (kbd) io->tty_restore();
}

void kbd_reclaim(const struct desk_io *io, int kbd) {
    if (kbd) io->tty_raw();
}
