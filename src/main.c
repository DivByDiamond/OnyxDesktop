/*
 * main.c - onyxdesk entry point.
 *
 * Usage: onyxdesk [--autostart NAME] [--once]
 *
 * /etc/desk.conf is read at startup; missing, empty or malformed text
 * falls back to the built-in defaults, never a crash. The exit code is
 * the desk_run result (0 = clean session, 3 = no framebuffer or the
 * claim was refused).
 */
#include <stdio.h>
#include <string.h>
#include "config/config.h"
#include "desk/desk.h"

#define DESK_CONF_PATH "/etc/desk.conf"
#define DESK_CONF_MAX 4096

/* Leave *out usable no matter what: defaults first, parse on top. */
static void load_config(struct desk_config *out) {
    FILE *fp;
    size_t n = 0;
    char text[DESK_CONF_MAX];

    desk_config_default(out);
    fp = fopen(DESK_CONF_PATH, "r");
    if (fp == NULL) return;
    n = fread(text, 1, sizeof text - 1, fp);
    if (n == sizeof text - 1 && fgetc(fp) != EOF) {
        fprintf(stderr, "desk: %s truncated at %d bytes\n", DESK_CONF_PATH,
                DESK_CONF_MAX - 1);
    }
    fclose(fp);
    text[n] = 0;
    desk_config_parse(text, out); /* -1 leaves the defaults in place */
}

int main(int argc, char **argv) {
    struct desk_config config;
    const char *autostart = NULL;
    int once = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--once") == 0 && !once) {
            once = 1;
        } else if (strcmp(argv[i], "--autostart") == 0 && i + 1 < argc &&
                   autostart == NULL) {
            autostart = argv[++i];
        } else {
            fprintf(stderr, "desk: ignoring argument %s\n", argv[i]);
        }
    }
    load_config(&config);
    return desk_run(&config, autostart, once);
}
