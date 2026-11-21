/*
 * config.h - /etc/desk.conf model and parser.
 *
 * The file is a flat list of key=value lines:
 *
 *   # comment
 *   panel_height=24
 *   app=osnake
 *   app=ochat-gui
 *
 * Unknown keys are ignored. panel_height must be 1..200; each app name
 * is 1..DESK_NAME_MAX-1 characters and must not contain whitespace.
 * Malformed known values fail the whole parse with -1: the list is
 * never silently truncated. Empty or comment-only text is not an error
 * and yields the defaults.
 */
#ifndef ONYXDESK_CONFIG_H
#define ONYXDESK_CONFIG_H

#define DESK_MAX_APPS 8
#define DESK_NAME_MAX 32
#define DESK_DEFAULT_PANEL_HEIGHT 24

struct desk_config {
    int panel_height;
    int n_apps;
    char apps[DESK_MAX_APPS][DESK_NAME_MAX];
};

/* Fill *out with the built-in defaults: a 24 px panel and the
 * built-in app list (osnake, ochat-gui). A config without any app=
 * line keeps this list; the first app= line replaces it. A NULL out
 * is ignored. */
void desk_config_default(struct desk_config *out);

/* Parse config text into *out.
 * Returns 0 on success, -1 on a malformed known value or a NULL out.
 * On -1 *out is reset to the defaults, so the caller always holds a
 * usable config. NULL text is treated as empty. */
int desk_config_parse(const char *text, struct desk_config *out);

#endif /* ONYXDESK_CONFIG_H */
