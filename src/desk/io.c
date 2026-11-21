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

#include <signal.h>
#include <stdlib.h>
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

/* Poll pacing, the otablet loop discipline (20 ms per pass). */
static void io_delay(void) { usleep(20000); }

static void *io_frame_alloc(unsigned long bytes) {
    return malloc((size_t)bytes);
}

static void io_frame_free(void *frame) { free(frame); }

static const struct desk_io ONYX_IO = {
    io_open,       io_close,    io_ioctl,      io_lseek,
    io_write,      io_spawn,    io_waitpid,    io_mouse_read,
    io_catch_signals, io_delay, io_frame_alloc, io_frame_free,
};

const struct desk_io *desk_io_default(void) { return &ONYX_IO; }

#else /* host: no OnyxOS, tests bring their own table */

const struct desk_io *desk_io_default(void) { return 0; }

#endif /* __onyx__ */
