/*
 * photo.h - render the photo= entries of desk.conf onto the frame.
 *
 * photo_draw reads each configured file through the desk_io
 * read_file seam and blits it at its configured coordinate. A
 * missing, oversized or malformed photo is skipped with one stderr
 * note; the desk never dies on a bad photo. The read cap keeps a
 * runaway file from eating memory: bigger files are refused.
 */
#ifndef ONYXDESK_WIDGETS_PHOTO_H
#define ONYXDESK_WIDGETS_PHOTO_H

#include "bmp.h"

struct desk_io;

/* Largest accepted photo file, 64 KiB. */
#define PHOTO_FILE_CAP 65536

/* Draw every photo of *config (at most DESK_MAX_PHOTOS entries,
 * anything else in n_photos is ignored) onto the packed frame.
 * NULL arguments are a quiet no-op. */
void photo_draw(unsigned char *frame, const struct fb_info *info,
                const struct desk_config *config, const struct desk_io *io);

#endif /* ONYXDESK_WIDGETS_PHOTO_H */
