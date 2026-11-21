/*
 * fake_kbd.h - key-injecting desk_io fake for the kbd tests.
 *
 * Wraps the desk fake (fake_desk.h): the tty seams queue raw bytes,
 * count calls and record the marker index at every switch, so a test
 * proves WHEN the desk armed or released the tty. k_spawn/k_waitpid
 * track the child window: kbd_read answers are counted separately
 * while a child is between spawn and wait.
 */
#ifndef FAKE_KBD_H
#define FAKE_KBD_H

#include <string.h>
#include "../desk/fake_desk.h"
#include "desk/kbd/kbd.h"

#define KBD_MAX_KEYS 8
#define KBD_MAX_RAW 4
#define KBD_MAX_ACTIONS 4

struct kbd_fake {
    int raw_fail, raw_calls, restore_calls, polls;
    int reads, reads_in_child, child;
    int raw_marker[KBD_MAX_RAW];    /* F.n_markers at each tty_raw */
    int restore_marker[KBD_MAX_RAW]; /* F.n_markers at each restore */
    int keys[KBD_MAX_KEYS], n_keys, next_key;
    desk_action_t actions[KBD_MAX_ACTIONS];
    int n_actions;
};

static struct kbd_fake K;

static inline void kbd_key(int ch) {
    if (K.n_keys < KBD_MAX_KEYS) K.keys[K.n_keys++] = ch;
}

static inline void k_sink(desk_action_t a) {
    if (K.n_actions < KBD_MAX_ACTIONS) K.actions[K.n_actions++] = a;
}

static inline int k_raw(void) {
    if (K.raw_calls < KBD_MAX_RAW) K.raw_marker[K.raw_calls] = F.n_markers;
    K.raw_calls++;
    return K.raw_fail ? 0 : 1;
}

static inline void k_restore(void) {
    if (K.restore_calls < KBD_MAX_RAW)
        K.restore_marker[K.restore_calls] = F.n_markers;
    K.restore_calls++;
}

static inline int k_poll(int timeout_ms) {
    (void)timeout_ms;
    K.polls++;
    return K.next_key < K.n_keys;
}

static inline int k_read(void) {
    K.reads++;
    if (K.child) K.reads_in_child++;
    if (K.next_key >= K.n_keys) return -1;
    return K.keys[K.next_key++];
}

/* Only a real child opens the window: a failed spawn never ran. */
static inline long k_spawn(const char *path, char *const argv[], int ring) {
    long pid = fake_spawn(path, argv, ring);

    if (pid >= 0) K.child = 1;
    return pid;
}

static inline long k_waitpid(int pid, int *status, int options) {
    long r = fake_waitpid(pid, status, options);

    if (r >= 0) K.child = 0;
    return r;
}

static const struct desk_io KEY_IO = {
    fake_open,     fake_close,     fake_ioctl,    fake_lseek,
    fake_write,    k_spawn,        k_waitpid,     fake_mouse_read,
    k_raw,        k_restore,      k_poll,        k_read,
    fake_catch_signals, fake_delay, fake_frame_alloc, fake_frame_free,
    fake_read_file, fake_clock_gettime,
};

/* fake_config zeroes the binds; put the built-in ones back. */
static inline void kbd_config(struct desk_config *c, int panel_h) {
    fake_config(c, panel_h);
    c->bind_key[DESK_BIND_LAUNCHER] = DESK_KEY_ESC;
    c->bind_key[DESK_BIND_CONSOLE] = DESK_KEY_TAB;
}

/* Fresh session on the key fake: raw ok, no keys, counting sink. */
static inline void kbd_install(void) {
    fake_reset();
    memset(&K, 0, sizeof K);
    desk_set_io(&KEY_IO);
    desk_set_marker_sink(fake_sink);
    kbd_set_action_sink(k_sink);
}

#endif /* FAKE_KBD_H */
