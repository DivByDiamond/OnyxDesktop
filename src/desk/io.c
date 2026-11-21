/*
 * io.c - real OnyxOS bindings for the desk_io test seam.
 *
 * On target builds every entry maps to a libonyxc wrapper, or to the
 * raw ABI ecall where no wrapper exists (SYS_mouse_read). Host builds
 * have no OnyxOS, so desk_io_default() yields NULL there and the
 * native tests inject their own table instead.
 */
#include "desk.h"

#ifdef __onyx__

#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
/* ABI constants (SYS_mouse_read) that libonyxc does not wrap. The
 * relative path only resolves when a sibling OnyxCompiller checkout
 * lines up; otherwise onyxcc falls back to its own include paths,
 * which are rooted at this compiler's libonyxc, so the header is
 * always found next to the toolchain actually building the desk. */
#include "../../../../OnyxCompiller/include/sys/syscalls.h"

/* SIGINT/SIGTERM never run code here beyond setting the stop flag;
 * the main loop notices it and releases the fb on its way out. */
static void on_signal(int number) {
    (void)number;
    desk_request_stop();
}

static void io_catch_signals(void) {
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
}

static int io_open(const char *path, int flags) {
    return (int)_onyx_open(path, flags, 0);
}

static int io_close(int fd) { return (int)_onyx_close(fd); }

static long io_ioctl(int fd, unsigned long request, unsigned long arg) {
    return _onyx_ioctl(fd, (long)request, (long)arg);
}

static long io_lseek(int fd, long offset, int whence) {
    return _onyx_lseek(fd, offset, whence);
}

static long io_write(int fd, const void *buf, unsigned long count) {
    return _onyx_write(fd, buf, count);
}

static long io_spawn(const char *path, char *const argv[], int ring) {
    return _onyx_spawn(path, argv, ring);
}

static long io_waitpid(int pid, int *status, int options) {
    return _onyx_waitpid(pid, status, options);
}

static long io_mouse_read(void *pointer) {
    return _onyx_syscall1(SYS_mouse_read, (long)pointer);
}

/* Terminal discipline, the osnake/otablet recipe: the saved termios
 * goes back around every launch; VMIN=0/VTIME=0 keeps a read from
 * fd 0 from ever blocking. OnyxOS termios state is per-process, so
 * the saved copy is simply refreshed on every raw arm. */
static struct termios g_saved_tty;
static int g_tty_raw_on;

static int io_tty_raw(void) {
    struct termios raw;

    if (tcgetattr(0, &g_saved_tty) != 0) return 0;
    raw = g_saved_tty;
    cfmakeraw_apply(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(0, 0, &raw) != 0) return 0;
    g_tty_raw_on = 1;
    return 1;
}

static void io_tty_restore(void) {
    if (g_tty_raw_on) {
        tcsetattr(0, 0, &g_saved_tty);
        g_tty_raw_on = 0;
    }
}

/* poll(2) from libonyxc (SYS_poll): tty readiness, never a block. */
static int io_tty_poll(int timeout_ms) {
    struct pollfd pfd;

    pfd.fd = 0;
    pfd.events = POLLIN;
    pfd.revents = 0;
    return poll(&pfd, 1, timeout_ms);
}

static int io_kbd_read(void) {
    unsigned char ch;

    return read(0, &ch, 1) == 1 ? ch : -1;
}

/* Poll pacing, the otablet loop discipline (20 ms per pass). */
static void io_delay(void) { usleep(20000); }

static void *io_frame_alloc(unsigned long bytes) {
    return malloc((size_t)bytes);
}

static void io_frame_free(void *frame) { free(frame); }

/* One open/read/close for a whole regular file (photo= files).
 * 0 is O_RDONLY on OnyxOS; the read stops at cap, so a caller that
 * must reject oversized files passes a cap with headroom. */
static long io_read_file(const char *path, unsigned char *buf,
                         unsigned long cap) {
    long fd, n;

    fd = _onyx_open(path, 0, 0);
    if (fd < 0) return -1;
    n = _onyx_read((int)fd, buf, (size_t)cap);
    if (_onyx_close((int)fd) != 0 || n < 0) return -1;
    return n;
}

static const struct desk_io ONYX_IO = {
    io_open,       io_close,      io_ioctl,      io_lseek,
    io_write,      io_spawn,      io_waitpid,    io_mouse_read,
    io_tty_raw,    io_tty_restore, io_tty_poll,  io_kbd_read,
    io_catch_signals, io_delay,   io_frame_alloc, io_frame_free,
    io_read_file,
};

const struct desk_io *desk_io_default(void) { return &ONYX_IO; }

#else /* host: no OnyxOS, tests bring their own table */

const struct desk_io *desk_io_default(void) { return 0; }

#endif /* __onyx__ */
