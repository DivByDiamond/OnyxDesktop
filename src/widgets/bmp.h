/*
 * bmp.h - decode an uncompressed BMP into a frame blit.
 *
 * Supported input: BITMAPFILEHEADER with the 'BM' magic and a
 * BITMAPINFOHEADER (biSize >= 40) with biPlanes 1, biBitCount 24
 * (BGR rows padded to 4 bytes) or 32 (BGRX), biCompression BI_RGB
 * (0). A positive height means bottom-up rows, a negative one
 * top-down; both decode upright. Linear dimensions above
 * BMP_MAX_DIM are refused before any arithmetic.
 *
 * bmp_blit validates the whole file against len first, so a
 * truncated or hostile file never reads out of bounds; pixels are
 * written through ui_put_px, so a blit straddling the frame edge
 * draws exactly the intersection and an empty intersection is a
 * quiet success.
 */
#ifndef ONYXDESK_WIDGETS_BMP_H
#define ONYXDESK_WIDGETS_BMP_H

#include "ui.h"

#define BMP_MAX_DIM 4096

/* 0: the image is on the frame (possibly nothing was visible);
 * -1: broken file or unusable frame, nothing was drawn. */
int bmp_blit(unsigned char *frame, const struct fb_info *info,
             const unsigned char *data, unsigned long len, int x, int y);

#endif /* ONYXDESK_WIDGETS_BMP_H */
