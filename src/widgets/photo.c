/*
 * photo.c - the photo widget: desk.conf photo= files onto the frame.
 *
 * One scratch buffer, one decoder: read the file whole through the
 * read_file seam, blit it, move on. The buffer asks for one byte
 * more than the cap, so a file larger than PHOTO_FILE_CAP is told
 * apart from an exactly-cap one by its read length, without a
 * second read or a partial decode.
 */
#include <stdio.h>
#include "photo.h"
#include "desk/desk.h"

/* The desk loop is single-threaded: one static buffer, no seam for
 * the allocation, nothing to free on the error paths. */
static unsigned char photo_buf[PHOTO_FILE_CAP + 1];

void photo_draw(unsigned char *frame, const struct fb_info *info,
                const struct desk_config *config, const struct desk_io *io) {
    int n, k;

    if (frame == NULL || info == NULL || config == NULL || io == NULL)
        return;
    n = config->n_photos;
    if (n < 0)
        n = 0;
    if (n > DESK_MAX_PHOTOS)
        n = DESK_MAX_PHOTOS;
    for (k = 0; k < n; k++) {
        const char *path = config->photos[k];
        long len = io->read_file(path, photo_buf, sizeof photo_buf);

        if (len < 0) {
            fprintf(stderr, "desk: photo %s unreadable\n", path);
            continue;
        }
        if (len > PHOTO_FILE_CAP) {
            fprintf(stderr, "desk: photo %s too large\n", path);
            continue;
        }
        if (bmp_blit(frame, info, photo_buf, (unsigned long)len,
                     config->photo_x[k], config->photo_y[k]) != 0)
            fprintf(stderr, "desk: photo %s invalid\n", path);
    }
}
