/*
 * desk.c - portable desk daemon main loop.
 * Every OS contact goes through struct desk_io (io.c binds the real
 * OnyxOS syscalls on target; host tests inject a fake) and every smoke
 * marker through desk_emit(); see desk.h for the session contract.
 */
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "desk.h"
#include "kbd/kbd.h"
#include "ui.h"

static const struct desk_io *g_io;
static void (*g_sink)(const char *);
static volatile sig_atomic_t g_stop;

void desk_set_io(const struct desk_io *io) { g_io = io; }

void desk_set_marker_sink(void (*sink)(const char *marker)) { g_sink = sink; }

void desk_request_stop(void) { g_stop = 1; }

/* One smoke marker per call, flushed; a test sink replaces stdout. */
static void desk_emit(const char *fmt, ...) {
    char marker[64];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(marker, sizeof marker, fmt, ap);
    va_end(ap);
    if (g_sink) g_sink(marker);
    else { printf("%s\n", marker); fflush(stdout); }
}

/* GET_INFO sanity: nonzero geometry, a supported depth, and a device
 * that holds the packed frame plus the last present row, written at
 * (height-1)*pitch; u32 terms cannot overflow unsigned long. */
static int fb_sane(const struct fb_info *i) {
    unsigned long bytes = i->bpp / 8;

    if (i->width == 0 || i->height == 0) return 0;
    if (i->bpp != 16 && i->bpp != 32) return 0;
    return (unsigned long)i->width * i->height * bytes <= i->size &&
           (unsigned long)(i->height - 1) * i->pitch +
               (unsigned long)i->width * bytes <= i->size;
}

/* Row-by-row blit, the otablet present discipline: every row is
 * seeked to its pitch offset and written as one width*bytes chunk. */
static int present(const struct desk_io *io, int fd,
                   const unsigned char *frame, const struct fb_info *i) {
    unsigned long bytes = i->bpp / 8;
    unsigned long len = i->width * bytes;
    unsigned long y;

    for (y = 0; y < i->height; y++) {
        if (io->lseek(fd, (long)(y * i->pitch), SEEK_SET) < 0) return 0;
        if (io->write(fd, frame + y * i->width * bytes, len) != (long)len)
            return 0;
    }
    return 1;
}

/* Fresh launcher frame plus one present. Returns 0, or DESK_EXIT_FB
 * when the fb write failed (the note is logged here, once, not at
 * every caller). */
static int repaint(const struct desk_config *c, const struct fb_info *i,
                   unsigned char *frame, const struct desk_io *io, int fd) {
    int ok;

    ui_draw_panel(frame, i, c->panel_height, "onyxdesk");
    ui_draw_launcher(frame, i, c, -1);
    ok = present(io, fd, frame, i);
    if (!ok) fprintf(stderr, "desk: present failed\n");
    return ok ? 0 : DESK_EXIT_FB;
}

/* Managed launch of /bin/<name>: the tty goes to the child for the
 * run and back afterwards (a spawn that never started re-arms raw
 * immediately). APP_STARTED/APP_RETURN only for a child that ran. */
static int launch(const struct desk_io *io, const char *name, int kbd) {
    char path[DESK_NAME_MAX + 5];
    char *argv[2];
    long pid, waited;
    int status = 0;

    if (strlen(name) >= DESK_NAME_MAX) {
        fprintf(stderr, "desk: app name too long: %s\n", name);
        return -1;
    }
    memcpy(path, "/bin/", 5);
    strcpy(path + 5, name);
    argv[0] = path;
    argv[1] = NULL;
    kbd_yield(io, kbd); /* the child owns the tty while it runs */
    pid = io->spawn(path, argv, DESK_RING);
    if (pid < 0) {
        fprintf(stderr, "desk: spawn %s failed\n", name);
        kbd_reclaim(io, kbd);
        return -1;
    }
    desk_emit("APP_STARTED %s %d", name, (int)pid);
    /* A parked waiter answers -EINTR/-ENOENT (runtime.c): retry like
     * libonyxc system() does, never treat those as the exit code. */
    do {
        waited = io->waitpid((int)pid, &status, 0);
    } while (waited == -DESK_EINTR || waited == -DESK_ENOENT);
    kbd_reclaim(io, kbd); /* the child returned: the tty is ours */
    if (waited < 0) status = 1;
    desk_emit("APP_RETURN %d", status);
    return 0;
}

int desk_run(const struct desk_config *config, const char *autostart,
             int once) {
    struct desk_config fallback;
    const struct desk_io *io = g_io ? g_io : desk_io_default();
    struct fb_info info;
    struct desk_pointer p;
    unsigned char *frame;
    unsigned long frame_bytes;
    int fd, down = 0, rc = 0, kbd = 0, hit;

    if (config == NULL) {
        desk_config_default(&fallback);
        config = &fallback;
    }
    g_stop = 0; /* a stop request belongs to this session only */
    if (io == NULL) {
        fprintf(stderr, "desk: no io backend\n");
        desk_emit("DESK_EXIT %d", DESK_EXIT_FB);
        return DESK_EXIT_FB;
    }
    fd = io->open(DESK_FB_DEV, DESK_FB_FLAGS);
    if (fd < 0) {
        fprintf(stderr, "desk: cannot open %s\n", DESK_FB_DEV);
        goto bad;
    }
    if (io->ioctl(fd, DESK_FB_GET_INFO, (unsigned long)&info) < 0 ||
        !fb_sane(&info)) {
        fprintf(stderr, "desk: unusable framebuffer info\n");
        goto bad;
    }
    if (io->ioctl(fd, DESK_FB_CLAIM, 0) < 0) {
        fprintf(stderr, "desk: framebuffer claim refused\n");
        goto bad; /* no RELEASE: the claim is not ours */
    }
    /* The claim is ours: arm the signal path before anything else so a
     * SIGTERM cannot bypass the RELEASE. Every exit goes through out:. */
    io->catch_signals();
    kbd = kbd_arm(io); /* raw for the session, or the binds stay off */
    frame_bytes = (unsigned long)info.width * info.height * (info.bpp / 8);
    frame = io->frame_alloc(frame_bytes);
    if (frame == NULL) {
        fprintf(stderr, "desk: out of memory\n");
        rc = DESK_EXIT_FB;
        goto out;
    }
    desk_emit("DESK_READY");
    if ((rc = repaint(config, &info, frame, io, fd)) != 0) goto out;
    if (autostart != NULL) {
        if (launch(io, autostart, kbd) == 0 && once) goto out;
        if (g_stop) goto out;
        if ((rc = repaint(config, &info, frame, io, fd)) != 0) goto out;
    }
    for (;;) {
        if (g_stop) break;
        if (io->mouse_read(&p) < 0) break;
        if (kbd) kbd_pass(config, io);
        io->delay(); /* every table fills every entry: see desk.h */
        if (p.buttons & DESK_BTN_EXIT) break;
        if (!(p.buttons & DESK_BTN_CLICK)) {
            down = 0;
            continue;
        }
        if (down) continue; /* held: one front edge, one launch */
        down = 1;
        hit = ui_hit_test(&info, config->panel_height, config->n_apps,
                          p.x, p.y);
        if (hit < 0) continue;
        if (launch(io, config->apps[hit], kbd) == 0 && once) break;
        if (g_stop) break;
        if ((rc = repaint(config, &info, frame, io, fd)) != 0) break;
    }
out:
    if (frame != NULL) io->frame_free(frame);
    kbd_yield(io, kbd); /* hand the tty back on the way out */
    io->ioctl(fd, DESK_FB_RELEASE, 0); /* exactly one RELEASE per claim */
    io->close(fd);
    desk_emit("DESK_EXIT %d", rc);
    return rc;
bad: /* fb open/info/claim failures: no claim to release */
    if (fd >= 0) io->close(fd);
    desk_emit("DESK_EXIT %d", DESK_EXIT_FB);
    return DESK_EXIT_FB;
}
