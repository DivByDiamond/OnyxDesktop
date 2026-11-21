/*
 * kbd.h - keybind dispatch for the desk loop.
 *
 * kbd_dispatch maps one raw tty byte to the action bound to it in
 * desk.conf (bind=launcher:<key>, bind=console:<key>, see
 * config/config.h). kbd_pass is the per-pass loop helper: poll the
 * tty, read one byte, dispatch it and report through the action
 * sink; the shipped sink notes the action on stderr until the
 * overlays land (task 6), tests install a counter instead.
 *
 * The desk owns the tty lifecycle (desk.c): raw right after a
 * successful CLAIM, restored before every spawn, raw again when the
 * child returns. When the raw call refuses, the desk degrades
 * quietly: one stderr note, the binds stay off for the whole
 * session and the mouse loop runs unchanged. Quit is not a bind:
 * leaving the desk stays on mouse button 2, as in v1.
 */
#ifndef ONYXDESK_KBD_H
#define ONYXDESK_KBD_H

#include "../desk.h"

typedef enum {
    DESK_ACT_NONE = 0,
    DESK_ACT_LAUNCHER,
    DESK_ACT_CONSOLE
} desk_action_t;

/* Map one byte to its bound action; DESK_ACT_NONE when the byte is
 * unbound, the config is NULL or the slot is empty (zero). When both
 * binds share one key, the launcher wins (checked first). */
desk_action_t kbd_dispatch(const struct desk_config *c, int ch);

/* Session tty discipline for desk.c: arm raw right after a successful
 * CLAIM, yield the tty before a spawn, reclaim it when the child has
 * returned (or never started). kbd_arm prints one stderr note on a
 * refusal and the session then runs without binds. */
int kbd_arm(const struct desk_io *io);
void kbd_yield(const struct desk_io *io, int kbd);
void kbd_reclaim(const struct desk_io *io, int kbd);

/* One keyboard pass for the desk loop: poll the tty, read at most
 * one byte and report the bound action through the sink. Reads
 * nothing while the desk waits for a child (the caller only runs
 * this between launches). */
void kbd_pass(const struct desk_config *c, const struct desk_io *io);

/* NULL restores the shipped stderr sink. */
void kbd_set_action_sink(void (*sink)(desk_action_t action));

#endif /* ONYXDESK_KBD_H */
