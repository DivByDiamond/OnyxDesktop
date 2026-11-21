/*
 * ui.c - pixel, text, panel, launcher and hit-test primitives.
 *
 * All writes go through put_px(), which clips to the packed frame and
 * rejects unsupported bpp values, so no caller can draw out of bounds.
 */
#include <stddef.h>
#include "ui.h"
#include "font/font.h"

#define UI_BPP16 16
#define UI_BPP32 32
#define UI_TEXT_MAX 64
#define UI_ADVANCE (UI_GLYPH_W + 1)

/* Reject NULLs, empty frames, unsupported formats and geometries whose
 * byte count would not fit a 32-bit unsigned. */
static int frame_ok(const unsigned char *frame, const struct fb_info *info) {
    unsigned bytes;

    if (frame == NULL || info == NULL || info->width == 0 || info->height == 0) {
        return 0;
    }
    if (info->bpp != UI_BPP16 && info->bpp != UI_BPP32) {
        return 0;
    }
    bytes = info->bpp / 8;
    if (info->width > 0xFFFFFFFFu / bytes) {
        return 0;
    }
    if (info->width * bytes > 0xFFFFFFFFu / info->height) {
        return 0;
    }
    return 1;
}

/* RGB565 -> 0x00RRGGBB, the same expansion otablet uses for 32bpp. */
static unsigned expand(unsigned short c) {
    return (unsigned)(((c >> 11) & 31) * 255 / 31) << 16 |
           (unsigned)(((c >> 5) & 63) * 255 / 63) << 8 |
           (unsigned)((c & 31) * 255 / 31);
}

static void put_px(unsigned char *frame, const struct fb_info *info,
                   int x, int y, unsigned short color) {
    unsigned char *p;
    unsigned c;
    int bytes;

    if (x < 0 || y < 0 || (unsigned)x >= info->width ||
        (unsigned)y >= info->height) {
        return;
    }
    bytes = (int)(info->bpp / 8);
    p = frame + ((size_t)y * info->width + (unsigned)x) * (size_t)bytes;
    if (bytes == 2) {
        p[0] = (unsigned char)(color & 0xFF);
        p[1] = (unsigned char)((color >> 8) & 0xFF);
    } else {
        c = expand(color);
        p[0] = (unsigned char)(c & 0xFF);
        p[1] = (unsigned char)((c >> 8) & 0xFF);
        p[2] = (unsigned char)((c >> 16) & 0xFF);
        p[3] = 0; /* XRGB */
    }
}

static void fill_rect(unsigned char *frame, const struct fb_info *info,
                      int x0, int y0, int w, int h, unsigned short color) {
    int x, y;

    for (y = y0; y < y0 + h; y++) {
        for (x = x0; x < x0 + w; x++) {
            put_px(frame, info, x, y, color);
        }
    }
}

static void draw_text(unsigned char *frame, const struct fb_info *info,
                      int x, int y, const char *text,
                      unsigned short color) {
    const unsigned char *g;
    int i, col, row;

    for (i = 0; i < UI_TEXT_MAX && text[i] != '\0'; i++) {
        g = font_glyph((unsigned char)text[i]);
        for (col = 0; col < UI_GLYPH_W; col++) {
            for (row = 0; row < UI_GLYPH_H; row++) {
                if ((g[col] & (1u << row)) != 0) {
                    put_px(frame, info, x + i * UI_ADVANCE + col, y + row,
                           color);
                }
            }
        }
    }
}

void ui_draw_panel(unsigned char *frame, const struct fb_info *info,
                   int panel_h, const char *active_name) {
    int h, ty;

    if (!frame_ok(frame, info) || panel_h <= 0) {
        return;
    }
    h = (unsigned)panel_h > info->height ? (int)info->height : panel_h;
    fill_rect(frame, info, 0, 0, (int)info->width, h, UI_PANEL_BG);
    if (active_name != NULL) {
        ty = (h - UI_GLYPH_H) / 2; /* negative rows are clipped by put_px */
        draw_text(frame, info, UI_PAD_X, ty, active_name, UI_PANEL_TEXT);
    }
}

void ui_draw_launcher(unsigned char *frame, const struct fb_info *info,
                      const struct desk_config *config, int selected) {
    unsigned y;
    int top, n, row, rh;

    if (!frame_ok(frame, info) || config == NULL) {
        return;
    }
    top = config->panel_height < 0 ? 0 : config->panel_height;
    if ((unsigned)top >= info->height) {
        return;
    }
    n = config->n_apps;
    if (n < 0) {
        n = 0;
    }
    if (n > DESK_MAX_APPS) {
        n = DESK_MAX_APPS;
    }
    fill_rect(frame, info, 0, top, (int)info->width,
              (int)info->height - top, UI_BG);
    for (row = 0; row < n; row++) {
        y = (unsigned)top + (unsigned)row * UI_ROW_H;
        if (y >= info->height) {
            break;
        }
        rh = (int)(info->height - y);
        if (rh > UI_ROW_H) {
            rh = UI_ROW_H;
        }
        if (row == selected) {
            fill_rect(frame, info, 0, (int)y, (int)info->width, rh,
                      UI_SELECTED_BG);
        }
        draw_text(frame, info, UI_PAD_X,
                  (int)y + (UI_ROW_H - UI_GLYPH_H) / 2, config->apps[row],
                  row == selected ? UI_SELECTED_TEXT : UI_TEXT);
    }
}

int ui_hit_test(const struct fb_info *info, int panel_h, int n_apps,
                int x, int y) {
    int row;

    if (info == NULL || info->width == 0 || info->height == 0 ||
        n_apps <= 0 || x < 0 || (unsigned)x >= info->width || y < 0 ||
        (unsigned)y >= info->height) {
        return -1;
    }
    if (n_apps > DESK_MAX_APPS) {
        n_apps = DESK_MAX_APPS;
    }
    if (panel_h < 0) {
        panel_h = 0;
    }
    if (y < panel_h) {
        return -1;
    }
    row = (y - panel_h) / UI_ROW_H;
    return row < n_apps ? row : -1;
}
