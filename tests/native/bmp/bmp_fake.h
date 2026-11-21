/*
 * bmp_fake.h - registered-file fake for the desk_io read_file seam.
 *
 * photo_draw only calls read_file, so this desk_io table fills the
 * rest with quiet stubs: the widget test needs neither the desk
 * loop nor its io.c. Unregistered paths answer -1 (the missing
 * file case); a registered file longer than the cap answers a
 * truncated read, like the target binding.
 */
#ifndef TEST_BMP_FAKE_H
#define TEST_BMP_FAKE_H

#include <stdlib.h>
#include "bmp_gen.h" /* frame fixtures, DESK_PATH_MAX via config.h */
#include "desk/desk.h"
#include "widgets/photo.h"

#define FAKE_FILE_SLOTS 4
#define FAKE_FILE_DATA 256

struct fake_files {
    int calls[FAKE_FILE_SLOTS];
    int fail[FAKE_FILE_SLOTS];
    unsigned long len[FAKE_FILE_SLOTS];
    char path[FAKE_FILE_SLOTS][DESK_PATH_MAX];
    unsigned char data[FAKE_FILE_SLOTS][FAKE_FILE_DATA];
    int n;
};

static struct fake_files P;

static inline void files_reset(void) { memset(&P, 0, sizeof P); }

/* Register one file. data may be NULL when the content does not
 * matter (an oversized file is rejected before it is decoded). */
static inline int file_add(const char *path, const unsigned char *data,
                           unsigned long len) {
    if (P.n >= FAKE_FILE_SLOTS) return -1;
    snprintf(P.path[P.n], DESK_PATH_MAX, "%s", path);
    P.len[P.n] = len;
    if (data != NULL && len <= FAKE_FILE_DATA)
        memcpy(P.data[P.n], data, (size_t)len);
    return P.n++;
}

static inline long files_read(const char *path, unsigned char *buf,
                              unsigned long cap) {
    int k;

    for (k = 0; k < P.n; k++) {
        unsigned long have;
        if (strcmp(P.path[k], path) != 0) continue;
        P.calls[k]++;
        if (P.fail[k]) return -1;
        if (P.len[k] > cap) return (long)cap; /* read stops at the cap */
        /* Only the stored prefix is real; a file longer than the
         * slot reads out as its registered length either way. */
        have = P.len[k] < FAKE_FILE_DATA ? P.len[k] : FAKE_FILE_DATA;
        memcpy(buf, P.data[k], (size_t)have);
        return (long)P.len[k];
    }
    return -1;
}

/* Quiet stubs for the seams photo_draw never calls. */
static inline int stub_open(const char *path, int flags) {
    (void)path;
    (void)flags;
    return -1;
}

static inline int stub_close(int fd) { (void)fd; return 0; }

static inline long stub_ioctl(int fd, unsigned long req, unsigned long arg) {
    (void)fd;
    (void)req;
    (void)arg;
    return -1;
}

static inline long stub_lseek(int fd, long off, int whence) {
    (void)fd;
    (void)whence;
    return off;
}

static inline long stub_write(int fd, const void *buf, unsigned long n) {
    (void)fd;
    (void)buf;
    return (long)n;
}

static inline long stub_spawn(const char *path, char *const argv[],
                              int ring) {
    (void)path;
    (void)argv;
    (void)ring;
    return -1;
}

static inline long stub_waitpid(int pid, int *status, int options) {
    (void)pid;
    (void)options;
    *status = 0;
    return 0;
}

static inline long stub_mouse_read(void *pointer) {
    (void)pointer;
    return -1;
}

static inline int stub_tty_raw(void) { return 0; }

static inline void stub_tty_restore(void) {}

static inline int stub_tty_poll(int timeout_ms) {
    (void)timeout_ms;
    return 0;
}

static inline int stub_kbd_read(void) { return -1; }

static inline void stub_catch_signals(void) {}

static inline void stub_delay(void) {}

static inline void *stub_frame_alloc(unsigned long bytes) {
    return malloc((size_t)bytes);
}

static inline void stub_frame_free(void *frame) { free(frame); }

/* photo_draw never reads a clock: a quiet refusal. */
static inline long stub_clock_gettime(int clk_id, struct desk_timespec *ts) {
    (void)clk_id;
    (void)ts;
    return -1;
}

static const struct desk_io PHOTO_IO = {
    stub_open,       stub_close,     stub_ioctl,     stub_lseek,
    stub_write,      stub_spawn,     stub_waitpid,   stub_mouse_read,
    stub_tty_raw,    stub_tty_restore, stub_tty_poll, stub_kbd_read,
    stub_catch_signals, stub_delay,  stub_frame_alloc, stub_frame_free,
    files_read,      stub_clock_gettime,
};

#endif /* TEST_BMP_FAKE_H */
