/*
 * bmp.c - uncompressed 24/32-bit BMP decoder.
 *
 * One path: read and fully validate the headers against len, then
 * blit row by row through ui_put_px. The destination rectangle is
 * clipped to the frame before the loops run, so every byte read
 * stays inside len and every write inside the frame, whatever the
 * file and the coordinates say.
 */
#include <stddef.h>
#include "bmp.h"

#define BMP_FILE_HDR 14
#define BMP_INFO_HDR 40
#define BMP_HDR_LEN (BMP_FILE_HDR + BMP_INFO_HDR)
#define BMP_PLANE_COUNT 1
#define BMP_TOP_DOWN_BIT 0x80000000UL
#define BMP_U32_SPACE 0x100000000UL

/* Little-endian readers, the on-disk field order of the headers. */
static unsigned get16(const unsigned char *p) {
    return (unsigned)p[0] | (unsigned)p[1] << 8;
}

static unsigned long get32(const unsigned char *p) {
    return (unsigned long)p[0] | (unsigned long)p[1] << 8 |
           (unsigned long)p[2] << 16 | (unsigned long)p[3] << 24;
}

int bmp_blit(unsigned char *frame, const struct fb_info *info,
             const unsigned char *data, unsigned long len, int x, int y) {
    unsigned long width, height, row_bytes, pixels, off_bits, file_size;
    unsigned bpp, info_size;
    int top_down;
    long sx0, sx1, sy0, sy1, sx, sy;

    if (data == NULL || len < BMP_HDR_LEN || !ui_frame_ok(frame, info))
        return -1;
    if (data[0] != 'B' || data[1] != 'M')
        return -1;
    file_size = get32(data + 2);
    off_bits = get32(data + 10); /* bfOffBits */
    info_size = (unsigned)get32(data + 14);
    width = get32(data + 18);
    height = get32(data + 22);
    if (get16(data + 26) != BMP_PLANE_COUNT)
        return -1;
    bpp = get16(data + 28);
    if (bpp != 24 && bpp != 32)
        return -1;
    if (get32(data + 30) != 0) /* BI_RGB only */
        return -1;
    if (info_size < BMP_INFO_HDR || width == 0 || width > BMP_MAX_DIM)
        return -1;
    /* A negative i32 height (u32 view above 0x7FFFFFFF) is top-down;
     * its magnitude is the two's complement of the stored value. */
    top_down = height > BMP_TOP_DOWN_BIT - 1;
    if (top_down)
        height = BMP_U32_SPACE - height;
    if (height == 0 || height > BMP_MAX_DIM)
        return -1;
    row_bytes = bpp == 32 ? width * 4 : (width * 3 + 3) & ~3UL;
    pixels = row_bytes * height;
    if (off_bits < (unsigned long)BMP_FILE_HDR + info_size)
        return -1; /* pixels must start after the headers */
    if (off_bits + pixels > len || file_size < off_bits + pixels)
        return -1; /* pixel data truncated */
    if (file_size > len)
        return -1; /* claims more file than the buffer holds */

    /* Clip [x, x+width) x [y, y+height) to the frame; the loops
     * then only touch source pixels that land inside. The right
     * edge clamps for any sign of x, so an image wider than the
     * frame never walks discarded columns. */
    sx0 = x < 0 ? -(long)x : 0;
    sx1 = (long)width;
    if ((long)x + (long)width > (long)info->width)
        sx1 = (long)info->width - x;
    sy0 = y < 0 ? -(long)y : 0;
    sy1 = (long)height;
    if ((long)y + (long)height > (long)info->height)
        sy1 = (long)info->height - y;
    if (sx0 >= sx1 || sy0 >= sy1)
        return 0; /* nothing visible: a valid empty blit */

    for (sy = sy0; sy < sy1; sy++) {
        unsigned long src_row = top_down ? (unsigned long)sy
                                         : height - 1 - (unsigned long)sy;
        const unsigned char *row = data + off_bits + src_row * row_bytes;
        for (sx = sx0; sx < sx1; sx++) {
            const unsigned char *px = row + (unsigned long)sx * (bpp / 8);
            ui_put_px(frame, info, (int)((long)x + sx), (int)((long)y + sy),
                      ui_rgb(px[2], px[1], px[0]));
        }
    }
    return 0;
}
