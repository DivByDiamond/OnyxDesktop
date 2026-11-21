/*
 * bar.c - the panel time widget string: clock or uptime.
 *
 * One realtime read picks the format, one monotonic read feeds the
 * uptime; both go through the desk_io clock_gettime seam, so the host
 * tests script the clocks and no libc time call happens here. The
 * fixed decisions (dot separator, year threshold, silence on refusal)
 * live in bar.h.
 */
#include <stdio.h>
#include "bar.h"
#include "desk/desk.h"

/* 2021-01-01T00:00:00Z: a realtime reading at or past this instant
 * belongs to a year beyond 2020. Anything earlier, a refused epoch
 * included, is uptime. */
#define BAR_CLOCK_MIN_SEC 1609459200L

#define BAR_DAY_SECS 86400L
#define BAR_MINUTE_SECS 60L
#define BAR_HOUR_SECS 3600L

static void bar_uptime(char *text_buf, int buf_len, long sec) {
    long mins = sec / BAR_MINUTE_SECS;

    if (sec < BAR_MINUTE_SECS) {
        snprintf(text_buf, (size_t)buf_len, "up 0.%02ld", sec);
    } else if (sec < BAR_HOUR_SECS) {
        snprintf(text_buf, (size_t)buf_len, "up %ld.%02ld", mins,
                 sec % BAR_MINUTE_SECS);
    } else {
        snprintf(text_buf, (size_t)buf_len, "up %ld.%02ld.%02ld",
                 sec / BAR_HOUR_SECS, mins % BAR_MINUTE_SECS,
                 sec % BAR_MINUTE_SECS);
    }
}

void bar_widgets(char *text_buf, int buf_len, const struct desk_io *io) {
    struct desk_timespec ts;
    long day;

    if (text_buf == NULL || buf_len <= 0) return;
    text_buf[0] = '\0'; /* the default widget: none */
    if (io == NULL || io->clock_gettime == NULL) return;
    if (io->clock_gettime(DESK_CLOCK_REALTIME, &ts) != 0) return;
    if (ts.tv_sec >= BAR_CLOCK_MIN_SEC) {
        day = ts.tv_sec % BAR_DAY_SECS; /* UTC days are epoch aligned */
        snprintf(text_buf, (size_t)buf_len, "%02ld.%02ld.%02ld", day / 3600,
                 (day / 60) % 60, day % 60);
        return;
    }
    if (io->clock_gettime(DESK_CLOCK_MONOTONIC, &ts) != 0) return;
    if (ts.tv_sec < 0) return; /* no sane uptime to show */
    bar_uptime(text_buf, buf_len, ts.tv_sec);
}
