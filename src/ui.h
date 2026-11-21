/*
 * ui.h - panel and launcher drawing primitives for onyxdesk.
 *
 * Pure code: the caller owns the packed frame buffer and its geometry,
 * so the exact same functions run in the host tests and on the target.
 *
 * Pixel formats follow FB_IOCTL_GET_INFO (0x4600), which reports five
 * u32 in this order: width, height, bpp, pitch, size. Only bpp 16
 * (RGB565) and 32 (XRGB) are drawn. The frame is packed
 * width*height*(bpp/8) bytes; pitch is presenter metadata (Task 4
 * writes each row with lseek/ write), never a drawing stride.
 *
 * Layout: the panel is the top strip 0..panel_h-1, the launcher list
 * starts right below it and every entry is UI_ROW_H pixels tall.
 */
#ifndef ONYXDESK_UI_H
#define ONYXDESK_UI_H

#include "config/config.h"

struct fb_info {
    unsigned width;
    unsigned height;
    unsigned bpp;
    unsigned pitch;
    unsigned size;
};

#define UI_GLYPH_W 5
#define UI_GLYPH_H 7
#define UI_ROW_H 12
#define UI_PAD_X 4

/* RGB565 colours; 32bpp output expands them like otablet does. */
#define UI_PANEL_BG 0x2124
#define UI_PANEL_TEXT 0xFFFF
#define UI_BG 0x0841
#define UI_TEXT 0xC618
#define UI_SELECTED_BG 0x001F
#define UI_SELECTED_TEXT 0xFFFF

/* Frame guard shared by every drawing entry point (and the bmp
 * blit): NULLs, empty frames, unsupported depths and geometries
 * whose byte count would not fit a 32-bit unsigned are refused. */
int ui_frame_ok(const unsigned char *frame, const struct fb_info *info);

/* One clipped pixel write: coordinates outside the frame are
 * dropped, RGB565 goes to 16bpp as-is and expands to 32bpp XRGB
 * like otablet does. All ui drawing funnels through this. */
void ui_put_px(unsigned char *frame, const struct fb_info *info,
              int x, int y, unsigned short color);

/* Pack 8-bit channels into RGB565, the colour space of the ui
 * constants; one place knows the layout (widgets feed it). */
unsigned short ui_rgb(unsigned char r, unsigned char g, unsigned char b);

/* Fill the top panel_h rows and draw active_name (NULL = no text).
 * panel_h <= 0 or taller than the frame is clamped safely. */
void ui_draw_panel(unsigned char *frame, const struct fb_info *info,
                   int panel_h, const char *active_name);

/* Map a click to a launcher index, or -1 for the panel strip, the
 * empty area, an out-of-frame coordinate or n_apps <= 0. */
int ui_hit_test(const struct fb_info *info, int panel_h, int n_apps,
                int x, int y);

/* Clear the area below the panel and draw one text row per configured
 * app. selected is highlighted when it is a valid index. */
void ui_draw_launcher(unsigned char *frame, const struct fb_info *info,
                      const struct desk_config *config, int selected);

#endif /* ONYXDESK_UI_H */
