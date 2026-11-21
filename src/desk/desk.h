/*
 * desk.h - the desk daemon main loop and its test seams.
 *
 * desk_run drives one whole desktop session:
 *   open /dev/fb0 -> GET_INFO -> CLAIM -> DESK_READY -> paint ->
 *   (optional autostart launch) -> mouse loop (click = launch a
 *   configured app, button 2 = quit) -> RELEASE + DESK_EXIT <code>.
 *
 * fb/claim failures are a stderr note + DESK_EXIT 3 and never RELEASE
 * a claim that is not ours. Once claimed, the fb is held for the whole
 * session including while a launched application runs (design doc:
 * managed apps draw to the fb without claiming it themselves).
 *
 * All OS contact sits behind struct desk_io: io.c binds the real
 * OnyxOS syscalls on target builds; host tests inject a fake table with
 * desk_set_io(). All smoke markers go through desk_emit(), so tests
 * capture the exact transcript via desk_set_marker_sink().
 */
#ifndef ONYXDESK_DESK_H
#define ONYXDESK_DESK_H

#include "config/config.h"

#define DESK_FB_DEV "/dev/fb0"
#define DESK_FB_FLAGS 2       /* O_RDWR on OnyxOS */
#define DESK_FB_GET_INFO 0x4600 /* five u32: w, h, bpp, pitch, size */
#define DESK_FB_CLAIM 0x46F0
#define DESK_FB_RELEASE 0x46F1
#define DESK_RING 1           /* spawn ring hint; SYS_spawn is gated */
#define DESK_EXIT_FB 3        /* no framebuffer, or claim refused */

#define DESK_BTN_CLICK 1
#define DESK_BTN_EXIT 2

/* OnyxOS errno values (libonyxc errno.h) seen from waitpid; the desk
 * retries them like libonyxc system() does (process/runtime.c). */
#define DESK_EINTR 4
#define DESK_ENOENT 2

/* Kernel repr(C) MouseEvent is six bytes; eight are reserved for ABI
 * compatibility (same layout otablet uses in device/frontend.c). */
struct desk_pointer {
    unsigned short x, y;
    unsigned char buttons, pad;
    unsigned short reserved;
};

/* Clock reading behind the clock_gettime seam: the same two longs as
 * the OnyxOS/glibc struct timespec, spelled here so desk.h needs no
 * libc header. clk_id is one of the DESK_CLOCK_* values below. */
struct desk_timespec {
    long tv_sec;
    long tv_nsec;
};

#define DESK_CLOCK_REALTIME 0   /* libonyxc io/time.h, SYS_clock_gettime */
#define DESK_CLOCK_MONOTONIC 1

struct desk_io {
    /* A complete contract: every entry must be filled by the provider
     * (the target table in io.c and every test fake alike); the loop
     * calls them without NULL checks. */
    int (*open)(const char *path, int flags);
    int (*close)(int fd);
    long (*ioctl)(int fd, unsigned long request, unsigned long arg);
    long (*lseek)(int fd, long offset, int whence);
    long (*write)(int fd, const void *buf, unsigned long count);
    long (*spawn)(const char *path, char *const argv[], int ring);
    long (*waitpid)(int pid, int *status, int options);
    long (*mouse_read)(void *pointer);
    /* The tty the desk owns while no child runs: raw right after a
     * successful CLAIM, restored around every launch and on the way
     * out. A tty_raw refusal only disables the binds (kbd/kbd.h). */
    int (*tty_raw)(void);
    void (*tty_restore)(void);
    int (*tty_poll)(int timeout_ms); /* >0: fd 0 is readable */
    int (*kbd_read)(void);           /* one byte, or -1 on error */
    void (*catch_signals)(void);
    void (*delay)(void);
    void *(*frame_alloc)(unsigned long bytes);
    void (*frame_free)(void *frame);
    /* Whole-file read (photo= files): up to cap bytes into buf,
     * returns the byte count (0..cap), or -1 when the file cannot
     * be opened or read. Callers that must reject oversized files
     * pass a cap with headroom and judge by the returned length. */
    long (*read_file)(const char *path, unsigned char *buf, unsigned long cap);
    /* One clock read: 0 on success, negative on failure. bar_widgets
     * is the only caller and treats a NULL table or seam as a quiet
     * refusal (the widget string stays empty). */
    long (*clock_gettime)(int clk_id, struct desk_timespec *ts);
};

void desk_set_io(const struct desk_io *io); /* NULL = target default */
const struct desk_io *desk_io_default(void); /* NULL on host builds */
void desk_set_marker_sink(void (*sink)(const char *marker));
void desk_request_stop(void); /* the SIGINT/SIGTERM path */

/* One session. Returns the DESK_EXIT code: 0 = clean, 3 = fb/claim
 * failure. autostart names a /bin app launched first; once exits
 * after the first successfully launched app returns. */
int desk_run(const struct desk_config *config, const char *autostart,
             int once);

#endif /* ONYXDESK_DESK_H */
