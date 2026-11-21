/*
 * fake_bar.h - clock-scripting desk_io fake for the bar tests.
 *
 * Wraps the desk fake (fake_desk.h): every entry stays quiet and only
 * clock_gettime is scripted, with a separate refusal flag per clock,
 * so a test drives the clock/uptime choice and both failure paths.
 */
#ifndef FAKE_BAR_H
#define FAKE_BAR_H

#include "../desk/fake_desk.h"
#include "widgets/bar/bar.h"

struct bar_clock {
    long real_sec, mono_sec;
    int fail_real, fail_mono;
    int real_calls, mono_calls;
};

static struct bar_clock T;

static inline long b_clock(int clk_id, struct desk_timespec *ts) {
    if (clk_id == DESK_CLOCK_REALTIME) {
        T.real_calls++;
        if (T.fail_real) return -1;
        ts->tv_sec = T.real_sec;
    } else if (clk_id == DESK_CLOCK_MONOTONIC) {
        T.mono_calls++;
        if (T.fail_mono) return -1;
        ts->tv_sec = T.mono_sec;
    } else {
        return -1;
    }
    ts->tv_nsec = 0;
    return 0;
}

static const struct desk_io BAR_IO = {
    fake_open,      fake_close,     fake_ioctl,     fake_lseek,
    fake_write,     fake_spawn,     fake_waitpid,   fake_mouse_read,
    fake_tty_raw,   fake_tty_restore, fake_tty_poll, fake_kbd_read,
    fake_catch_signals, fake_delay, fake_frame_alloc, fake_frame_free,
    fake_read_file,
    b_clock,
};

/* A provider table with no clock seam at all: the NULL-seam case. */
static const struct desk_io NO_CLOCK_IO = {0};

#endif /* FAKE_BAR_H */
