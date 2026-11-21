/*
 * bar.h - the panel time widget: one string, two clocks.
 *
 * bar_widgets() fills text_buf with the time widget of the panel: the
 * clock when the realtime reading belongs to a year past 2020, the
 * monotonic uptime otherwise. Decisions fixed here:
 *
 *   - Separator: the spec writes HH:MM:SS, but the 5x7 font has no
 *     colon glyph (font_glyph() would fall back to '?'), so the
 *     separator is the dot, the only clock-like separator in the
 *     table. Uptime is "up M.SS" below an hour, "up H.MM.SS" from an
 *     hour on (minutes without hours until 60 minutes).
 *   - Year past 2020: tv_sec >= 1609459200 is exactly 2021-01-01T00:00:00Z,
 *     and the widget prints no date, so no calendar walk is needed.
 *   - The active app name is not built here: ui_draw_panel already
 *     draws it (v1), and positioning it to the right is a drawing
 *     change for the desk loop, not a string builder job.
 *   - NULL table, NULL clock seam or a refused reading yield an empty
 *     string and no output: the caller rebuilds the string every
 *     second, so a note per frame would flood the log.
 *   - The refresh rate belongs to the caller (the desk loop updates
 *     once a second); bar only formats the current reading.
 *
 * With buf_len > 0 the buffer is always NUL-terminated, truncated to
 * fit when needed; 32 bytes cover every string this can produce.
 */
#ifndef ONYXDESK_WIDGETS_BAR_H
#define ONYXDESK_WIDGETS_BAR_H

struct desk_io;

/* Fill text_buf with the panel time widget (clock or uptime) read
 * through the clock_gettime seam of *io. A NULL buffer or buf_len <= 0
 * is a no-op; a missing or refused reading leaves an empty string. */
void bar_widgets(char *text_buf, int buf_len, const struct desk_io *io);

#endif /* ONYXDESK_WIDGETS_BAR_H */
