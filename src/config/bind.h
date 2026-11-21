/*
 * bind.h - declarations shared inside src/config.
 *
 * config.c owns the line loop and the v1 keys (panel_height, app);
 * bind.c owns the M2 keys (bind, photo) and the small lexical
 * helpers both files use. cfg_parse_bind and cfg_parse_photo check
 * a whole value before touching the config and always return 0:
 * a bad line is dropped with a stderr note, never a failed parse.
 */
#ifndef ONYXDESK_CONFIG_BIND_H
#define ONYXDESK_CONFIG_BIND_H

#include "config.h"

/* Whitespace classes used by the line-oriented tokenizer. */
int cfg_is_space(char c);
const char *cfg_skip_space_until(const char *p, const char *end);
const char *cfg_trim_end(const char *start, const char *end);

/* Handle the value part of one bind=/photo= line. */
int cfg_parse_bind(const char *value, int len, struct desk_config *c);
int cfg_parse_photo(const char *value, int len, struct desk_config *c);

#endif /* ONYXDESK_CONFIG_BIND_H */
