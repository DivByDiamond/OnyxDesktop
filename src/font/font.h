/*
 * font.h - the tiny 5x7 glyph table used by the panel and launcher.
 *
 * Internal to the ui module: drawing code asks for one glyph and gets
 * five column bytes back. Column bit 0 is the top row.
 */
#ifndef ONYXDESK_FONT_H
#define ONYXDESK_FONT_H

#include "ui.h"

/* Glyph for c, or the '?' glyph for anything outside the supported
 * ASCII subset. Lowercase letters map onto their uppercase glyph. */
const unsigned char *font_glyph(unsigned char c);

#endif /* ONYXDESK_FONT_H */
