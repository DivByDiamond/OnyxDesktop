/*
 * config.h - /etc/desk.conf model and parser.
 *
 * The file is a flat list of key=value lines:
 *
 *   # comment
 *   panel_height=24
 *   app=osnake
 *   bind=launcher:esc
 *   photo=/etc/desk-logo.bmp,540,560
 *
 * Unknown keys are ignored. panel_height must be 1..200; each app
 * name is 1..DESK_NAME_MAX-1 characters and must not contain
 * whitespace. bind=<action>:<key> maps launcher or console to esc,
 * tab, enter or one printable ASCII character. photo=<path>,<x>,<y>
 * adds up to DESK_MAX_PHOTOS photos with a comma-free path of at
 * most DESK_PATH_MAX-1 characters and non-negative decimal
 * coordinates. Malformed panel_height/app values fail the whole
 * parse with -1: the list is never silently truncated. Malformed
 * bind/photo lines are only dropped with a stderr note, so a
 * broken line never leaves a half-updated entry behind. Empty or
 * comment-only text is not an error and yields the defaults.
 */
#ifndef ONYXDESK_CONFIG_H
#define ONYXDESK_CONFIG_H

#define DESK_MAX_APPS 8
#define DESK_NAME_MAX 32
#define DESK_DEFAULT_PANEL_HEIGHT 24
#define DESK_MAX_PHOTOS 4
#define DESK_PATH_MAX 64

/* Key codes accepted by bind=; a single printable ASCII character
 * (32..126) is also a valid key and is stored as its code. */
#define DESK_KEY_ESC 27
#define DESK_KEY_TAB 9
#define DESK_KEY_ENTER 13

/* bind_key slots: [0] toggles the launcher, [1] the console. */
#define DESK_BIND_LAUNCHER 0
#define DESK_BIND_CONSOLE 1

struct desk_config {
    int panel_height;
    int n_apps;
    char apps[DESK_MAX_APPS][DESK_NAME_MAX];
    char bind_key[2];
    int n_photos;
    char photos[DESK_MAX_PHOTOS][DESK_PATH_MAX];
    int photo_x[DESK_MAX_PHOTOS];
    int photo_y[DESK_MAX_PHOTOS];
};

/* Fill *out with the built-in defaults: a 24 px panel, the built-in
 * app list (osnake, ochat-gui), launcher on esc, console on tab and
 * no photos. A config without any app= line keeps this list; the
 * first app= line replaces it. A NULL out is ignored. */
void desk_config_default(struct desk_config *out);

/* Parse config text into *out.
 * Returns 0 on success, -1 on a malformed known value or a NULL out.
 * On -1 *out is reset to the defaults, so the caller always holds a
 * usable config. NULL text is treated as empty. */
int desk_config_parse(const char *text, struct desk_config *out);

#endif /* ONYXDESK_CONFIG_H */
