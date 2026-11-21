/*
 * fake_desk.h - scripted fake desk_io plus marker capture.
 *
 * Every OnyxOS contact desk_run makes goes through this table, and
 * every smoke marker lands in F.markers, so a test asserts the exact
 * session transcript (ioctl order, marker sequence, release count)
 * without any OS. The claim discipline is checked structurally: the
 * 0x46F1 RELEASE ioctl must appear exactly once and must be the last
 * ioctl of the session (never around a spawn).
 */
#ifndef FAKE_DESK_H
#define FAKE_DESK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "desk/desk.h"
#include "ui.h"

#define FAKE_MAX_MARKERS 16
#define FAKE_MAX_IOCTLS 16
#define FAKE_MAX_EVENTS 16

struct fake_state {
    int open_fail, getinfo_fail, claim_fail, mouse_fail, stop_on_poll;
    int lseek_fail, write_fail, alloc_fail;
    int allocs, frees;
    struct fb_info info;
    long spawn_pid;
    int wait_status, wait_calls, spawn_calls, wait_retry_first;
    struct desk_pointer events[FAKE_MAX_EVENTS];
    int n_events, next_event;
    char markers[FAKE_MAX_MARKERS][64];
    int n_markers;
    unsigned long ioctls[FAKE_MAX_IOCTLS];
    int n_ioctls, opens, closes, claims, releases;
    long writes, lseeks;
    char spawn_path[64];
};

static struct fake_state F;

/* Fresh session: 8x4 RGB565 fb, spawn pid 7, exit status 0. */
static inline void fake_reset(void) {
    memset(&F, 0, sizeof F);
    F.spawn_pid = 7;
    F.info.width = 8;
    F.info.height = 4;
    F.info.bpp = 16;
    F.info.pitch = 16;
    F.info.size = 64;
}

static inline void fake_event(int x, int y, int buttons) {
    struct desk_pointer *e;

    if (F.n_events >= FAKE_MAX_EVENTS) return;
    e = &F.events[F.n_events++];
    e->x = (unsigned short)x;
    e->y = (unsigned short)y;
    e->buttons = (unsigned char)buttons;
}

static inline int fake_open(const char *path, int flags) {
    (void)flags;
    F.opens++;
    return F.open_fail || strcmp(path, DESK_FB_DEV) != 0 ? -1 : 3;
}

static inline int fake_close(int fd) { (void)fd; F.closes++; return 0; }

static inline long fake_ioctl(int fd, unsigned long request, unsigned long arg) {
    (void)fd;
    if (F.n_ioctls < FAKE_MAX_IOCTLS) F.ioctls[F.n_ioctls] = request;
    F.n_ioctls++;
    if (request == DESK_FB_GET_INFO) {
        if (F.getinfo_fail) return -1;
        memcpy((void *)arg, &F.info, sizeof F.info);
        return 0;
    }
    if (request == DESK_FB_CLAIM) {
        F.claims++;
        return F.claim_fail ? -1 : 0;
    }
    if (request == DESK_FB_RELEASE) {
        F.releases++;
        return 0;
    }
    return -1;
}

static inline long fake_lseek(int fd, long offset, int whence) {
    (void)fd; (void)whence;
    F.lseeks++;
    return F.lseek_fail ? -1 : offset;
}

static inline long fake_write(int fd, const void *buf, unsigned long count) {
    (void)fd;
    (void)buf;
    F.writes++;
    return F.write_fail ? -1 : (long)count;
}

static inline long fake_spawn(const char *path, char *const argv[], int ring) {
    (void)argv;
    (void)ring;
    F.spawn_calls++;
    snprintf(F.spawn_path, sizeof F.spawn_path, "%s", path);
    return F.spawn_pid;
}

static inline long fake_waitpid(int pid, int *status, int options) {
    (void)options;
    F.wait_calls++;
    /* The kernel can park a waiter and answer -ENOENT (runtime.c); the
     * desk must retry instead of treating it as the app's exit code. */
    if (F.wait_calls == 1 && F.wait_retry_first) return -DESK_ENOENT;
    *status = F.wait_status;
    return pid;
}

static inline long fake_mouse_read(void *out) {
    struct desk_pointer *p = out;

    memset(p, 0, sizeof *p);
    if (F.stop_on_poll) {
        F.stop_on_poll = 0;
        desk_request_stop();  /* the SIGTERM/SIGINT path, simulated */
        return 0;
    }
    if (F.mouse_fail) return -1;
    if (F.next_event < F.n_events) *p = F.events[F.next_event++];
    return 0;
}

static inline void fake_catch_signals(void) {}
static inline void fake_delay(void) {}

/* kbd seams: the desk fake stays raw-ok with no keys queued; the
 * key-injecting table for the kbd tests lives in kbd/fake_kbd.h. */
static inline int fake_tty_raw(void) { return 1; }
static inline void fake_tty_restore(void) {}
static inline int fake_tty_poll(int t) { (void)t; return 0; }
static inline int fake_kbd_read(void) { return -1; }

static inline void *fake_frame_alloc(unsigned long bytes) {
    F.allocs++;
    return F.alloc_fail ? NULL : malloc((size_t)bytes);
}

static inline void fake_frame_free(void *frame) {
    F.frees++;
    free(frame);
}

/* No file is registered by the desk fakes: every read answers -1.
 * The photo tests (bmp/bmp_util.h) bring a registered-file table. */
static inline long fake_read_file(const char *path, unsigned char *buf,
                                  unsigned long cap) {
    (void)path;
    (void)buf;
    (void)cap;
    return -1;
}

/* No desk test drives time yet: a quiet refusal. The bar tests bring
 * their own scripted table (bar/fake_bar.h). */
static inline long fake_clock_gettime(int clk_id, struct desk_timespec *ts) {
    (void)clk_id;
    (void)ts;
    return -1;
}

static const struct desk_io FAKE_IO = {
    fake_open,       fake_close, fake_ioctl,     fake_lseek,
    fake_write,      fake_spawn, fake_waitpid,   fake_mouse_read,
    fake_tty_raw,    fake_tty_restore, fake_tty_poll, fake_kbd_read,
    fake_catch_signals, fake_delay, fake_frame_alloc, fake_frame_free,
    fake_read_file,  fake_clock_gettime,
};

static inline void fake_sink(const char *marker) {
    if (F.n_markers < FAKE_MAX_MARKERS)
        snprintf(F.markers[F.n_markers], 64, "%s", marker);
    F.n_markers++;
}

/* Install the fake table and sink over a fresh session state. */
static inline void fake_install(void) {
    fake_reset();
    desk_set_io(&FAKE_IO);
    desk_set_marker_sink(fake_sink);
}

static inline void fake_config(struct desk_config *c, int panel_h) {
    memset(c, 0, sizeof *c);
    c->panel_height = panel_h;
    c->n_apps = 2;
    strcpy(c->apps[0], "osnake");
    strcpy(c->apps[1], "ochat-gui");
}

static inline int markers_exact(const char *const *want, int n) {
    int k;

    if (F.n_markers != n) return 0;
    for (k = 0; k < n; k++)
        if (strcmp(F.markers[k], want[k]) != 0) return 0;
    return 1;
}

/* Claim discipline: exactly one RELEASE, and it is the last ioctl. */
static inline int release_is_last_ioctl(void) {
    return F.n_ioctls >= 3 && F.releases == 1 &&
           F.ioctls[F.n_ioctls - 1] == DESK_FB_RELEASE;
}

#endif /* FAKE_DESK_H */
