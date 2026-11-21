/*
 * test_bar.c - panel time widget string tests.
 *
 * bar_widgets reads the clock seams and fills one string: a realtime
 * reading from 2021 on is the clock, anything earlier is the monotonic
 * uptime, and a missing or refused clock leaves the buffer empty. Every
 * emitted character must map to its own 5x7 glyph, which is why the
 * separator is a dot: the font has no colon and falls back to '?'.
 */
#include <stdio.h>
#include <string.h>
#include "fake_bar.h"
#include "font/font.h"

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

static char out[64];

/* One bar_widgets run over freshly scripted clocks. */
static const char *run(long real_sec, long mono_sec, int fail_real,
                       int fail_mono) {
    memset(&T, 0, sizeof T);
    T.real_sec = real_sec;
    T.mono_sec = mono_sec;
    T.fail_real = fail_real;
    T.fail_mono = fail_mono;
    memset(out, 'X', sizeof out - 1);
    out[sizeof out - 1] = '\0';
    bar_widgets(out, (int)sizeof out, &BAR_IO);
    return out;
}

/* Every character maps to its own glyph, never to the '?' fallback. */
static int renders(const char *s) {
    int i;

    for (i = 0; s[i] != '\0'; i++)
        if (font_glyph((unsigned char)s[i]) == font_glyph('?')) return 0;
    return 1;
}

static void test_clock_format(void) {
    CHECK(strcmp(run(1767225600, 0, 0, 0), "00.00.00") == 0);
    CHECK(strcmp(run(1767225665, 0, 0, 0), "00.01.05") == 0);
    CHECK(strcmp(run(1767270896, 0, 0, 0), "12.34.56") == 0);
    CHECK(strcmp(run(1767258307, 0, 0, 0), "09.05.07") == 0);
    CHECK(strcmp(run(1767311999, 0, 0, 0), "23.59.59") == 0);
    CHECK(strcmp(run(1767312000, 0, 0, 0), "00.00.00") == 0);
    CHECK(T.real_calls == 1 && T.mono_calls == 0); /* no uptime read */
}

static void test_year_threshold(void) {
    CHECK(strcmp(run(1609459200, 0, 0, 0), "00.00.00") == 0);  /* 2021-01-01 */
    CHECK(strcmp(run(1609459199, 0, 0, 0), "up 0.00") == 0);   /* 2020-12-31 */
    CHECK(strcmp(run(1577836800, 65, 0, 0), "up 1.05") == 0);  /* 2020-01-01 */
    CHECK(strcmp(run(0, 65, 0, 0), "up 1.05") == 0);           /* 1970-01-01 */
}

static void test_uptime_format(void) {
    CHECK(strcmp(run(0, 0, 0, 0), "up 0.00") == 0);
    CHECK(strcmp(run(0, 59, 0, 0), "up 0.59") == 0);
    CHECK(strcmp(run(0, 65, 0, 0), "up 1.05") == 0);
    CHECK(strcmp(run(0, 3599, 0, 0), "up 59.59") == 0);
    CHECK(strcmp(run(0, 3600, 0, 0), "up 1.00.00") == 0);
    CHECK(strcmp(run(0, 3700, 0, 0), "up 1.01.40") == 0);
    CHECK(strcmp(run(0, 36000, 0, 0), "up 10.00.00") == 0);
    CHECK(T.real_calls == 1 && T.mono_calls == 1); /* both clocks read */
}

static void test_empty_on_refusal(void) {
    memset(out, 'X', sizeof out - 1);
    out[sizeof out - 1] = '\0';
    bar_widgets(out, (int)sizeof out, NULL);
    CHECK(out[0] == '\0'); /* no provider table at all */
    bar_widgets(out, (int)sizeof out, &NO_CLOCK_IO);
    CHECK(out[0] == '\0'); /* NULL seam */
    CHECK(strcmp(run(1767225600, 0, 1, 0), "") == 0); /* realtime refused */
    CHECK(T.real_calls == 1 && T.mono_calls == 0);
    CHECK(strcmp(run(0, 65, 0, 1), "") == 0);         /* monotonic refused */
    CHECK(T.real_calls == 1 && T.mono_calls == 1);
    CHECK(strcmp(run(0, -5, 0, 0), "") == 0);         /* nonsense uptime */
    CHECK(strcmp(run(-1, 7, 0, 0), "up 0.07") == 0);  /* pre-1970 realtime */
}

static void test_buffer_limits(void) {
    char small[4];
    char one[1];

    memset(&T, 0, sizeof T);
    T.real_sec = 1767270896;
    memset(small, 'X', sizeof small);
    bar_widgets(small, (int)sizeof small, &BAR_IO);
    CHECK(strcmp(small, "12.") == 0); /* truncated, still terminated */
    memset(one, 'X', sizeof one);
    bar_widgets(one, (int)sizeof one, &BAR_IO);
    CHECK(one[0] == '\0'); /* only room for the terminator */
    memset(out, 'X', sizeof out - 1);
    out[sizeof out - 1] = '\0';
    bar_widgets(out, 0, &BAR_IO);
    CHECK(out[0] == 'X'); /* no room: buffer untouched */
    bar_widgets(NULL, (int)sizeof out, &BAR_IO);
    CHECK(T.real_calls == 2); /* no buffer: no clock read */
}

static void test_glyphs(void) {
    CHECK(renders(run(1767270896, 0, 0, 0))); /* 12.34.56 */
    CHECK(renders(run(0, 3700, 0, 0)));       /* up 1.01.40 */
    CHECK(strchr(run(1767270896, 0, 0, 0), ':') == NULL);
}

int main(void) {
    test_clock_format();
    test_year_threshold();
    test_uptime_format();
    test_empty_on_refusal();
    test_buffer_limits();
    test_glyphs();
    printf("\ntest_bar: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
