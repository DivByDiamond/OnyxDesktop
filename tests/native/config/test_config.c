/*
 * test_config.c - host tests for the /etc/desk.conf parser.
 *
 *   make -C tests/native test
 *
 * Covers the format rules from docs/plan-onyxdesktop-impl.md Task 2:
 * '#' comments, panel_height=1..200, app=<name> with 1..DESK_NAME_MAX-1
 * characters and no spaces, unknown keys ignored. Malformed values fail
 * the whole parse (-1, never silent truncation); empty or comment-only
 * text yields the defaults and 0.
 */
#include <stdio.h>
#include <string.h>
#include "config/config.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (cond) {                                                      \
            printf("  ok  %s\n", #cond);                                 \
        } else {                                                         \
            printf("  FAIL %s (%s:%d)\n", #cond, __FILE__, __LINE__);    \
            failures++;                                                  \
        }                                                                \
    } while (0)

/* The built-in defaults: a 24 px panel and the built-in app list. */
#define DEFAULTS(c)                                                      \
    ((c).panel_height == 24 && (c).n_apps == 2 &&                        \
     strcmp((c).apps[0], "osnake") == 0 &&                              \
     strcmp((c).apps[1], "ochat-gui") == 0 && (c).apps[2][0] == '\0')

static void test_defaults(void) {
    struct desk_config c;

    printf("defaults:\n");
    desk_config_default(&c);
    CHECK(c.panel_height == 24);
    CHECK(DEFAULTS(c));
    CHECK(c.apps[2][0] == '\0');
}

static void test_empty_text(void) {
    struct desk_config c;

    printf("empty text:\n");
    CHECK(desk_config_parse("", &c) == 0);
    CHECK(DEFAULTS(c));
    CHECK(desk_config_parse("\n  \n\t\r\n", &c) == 0);
    CHECK(DEFAULTS(c));
    CHECK(desk_config_parse(NULL, &c) == 0);
    CHECK(DEFAULTS(c));
}

static void test_comments_and_unknown_keys(void) {
    struct desk_config c;
    const char *text = "# desk.conf\n"
                       "   # indented comment\n"
                       "theme=dark\n"
                       "colour=#ff0\n"
                       "\n"
                       "panel_height=40\n";

    printf("comments and unknown keys:\n");
    CHECK(desk_config_parse(text, &c) == 0);
    CHECK(c.panel_height == 40);
    /* No app= line anywhere: the built-in list stays. */
    CHECK(c.n_apps == 2);
    CHECK(strcmp(c.apps[0], "osnake") == 0);
    CHECK(strcmp(c.apps[1], "ochat-gui") == 0);
}

static void test_valid_file(void) {
    struct desk_config c;
    const char *text = "panel_height=32\n"
                       "app=osnake\n"
                       "app=ochat-gui\n";

    printf("valid file:\n");
    CHECK(desk_config_parse(text, &c) == 0);
    CHECK(c.panel_height == 32);
    CHECK(c.n_apps == 2);
    CHECK(strcmp(c.apps[0], "osnake") == 0);
    CHECK(strcmp(c.apps[1], "ochat-gui") == 0);
    CHECK(c.apps[2][0] == '\0');
}

static void test_whitespace_tolerance(void) {
    struct desk_config c;
    const char *text = "  panel_height = 30  \r\n\tapp = onyxcalc \r\n";

    printf("whitespace tolerance:\n");
    CHECK(desk_config_parse(text, &c) == 0);
    CHECK(c.panel_height == 30);
    CHECK(c.n_apps == 1);
    CHECK(strcmp(c.apps[0], "onyxcalc") == 0);
}

static void test_app_name_limits(void) {
    struct desk_config c;
    char name[DESK_NAME_MAX + 2];
    char text[DESK_NAME_MAX + 16];

    printf("app name limits:\n");
    memset(name, 'a', sizeof name);
    name[DESK_NAME_MAX] = '\0'; /* exactly DESK_NAME_MAX chars: too long */
    snprintf(text, sizeof text, "app=%s\n", name);
    CHECK(desk_config_parse(text, &c) == -1);
    CHECK(DEFAULTS(c)); /* a failed parse leaves the defaults behind */

    name[DESK_NAME_MAX - 1] = '\0'; /* longest accepted name */
    snprintf(text, sizeof text, "app=%s", name);
    CHECK(desk_config_parse(text, &c) == 0);
    CHECK(c.n_apps == 1);
    CHECK((int)strlen(c.apps[0]) == DESK_NAME_MAX - 1);
}

static void test_bad_app_names(void) {
    struct desk_config c;

    printf("bad app names:\n");
    CHECK(desk_config_parse("app=\n", &c) == -1);
    CHECK(desk_config_parse("app=bad name\n", &c) == -1);
    CHECK(desk_config_parse("app=good\napp=\n", &c) == -1);
    CHECK(DEFAULTS(c));
}

static void test_app_count_overflow(void) {
    struct desk_config c;
    const char *eight = "app=a1\napp=a2\napp=a3\napp=a4\n"
                        "app=a5\napp=a6\napp=a7\napp=a8\n";
    const char *nine = "app=a1\napp=a2\napp=a3\napp=a4\napp=a5\n"
                       "app=a6\napp=a7\napp=a8\napp=a9\n";

    printf("app count:\n");
    CHECK(desk_config_parse(eight, &c) == 0);
    CHECK(c.n_apps == DESK_MAX_APPS);
    CHECK(desk_config_parse(nine, &c) == -1);
    CHECK(DEFAULTS(c));
}

static void test_panel_height(void) {
    struct desk_config c;

    printf("panel height:\n");
    CHECK(desk_config_parse("panel_height=1\n", &c) == 0);
    CHECK(c.panel_height == 1);
    CHECK(desk_config_parse("panel_height=200\n", &c) == 0);
    CHECK(c.panel_height == 200);

    CHECK(desk_config_parse("panel_height=0\n", &c) == -1);
    CHECK(desk_config_parse("panel_height=201\n", &c) == -1);
    CHECK(desk_config_parse("panel_height=abc\n", &c) == -1);
    CHECK(desk_config_parse("panel_height=\n", &c) == -1);
    CHECK(desk_config_parse("panel_height=-1\n", &c) == -1);
    CHECK(desk_config_parse("panel_height=24px\n", &c) == -1);
    CHECK(desk_config_parse("panel_height=99999999999999999999\n", &c) == -1);
    CHECK(DEFAULTS(c));
}

static void test_app_replaces_builtins(void) {
    struct desk_config c;

    printf("app= replaces the built-in list:\n");
    /* The first app= line replaces the built-ins; later lines append. */
    CHECK(desk_config_parse("app=onyxcalc\n", &c) == 0);
    CHECK(c.n_apps == 1);
    CHECK(strcmp(c.apps[0], "onyxcalc") == 0);
    CHECK(c.apps[1][0] == '\0');
    CHECK(desk_config_parse("app=onyxcalc\napp=osnake\n", &c) == 0);
    CHECK(c.n_apps == 2);
    CHECK(strcmp(c.apps[0], "onyxcalc") == 0);
    CHECK(strcmp(c.apps[1], "osnake") == 0);
}

int main(void) {
    test_defaults();
    test_empty_text();
    test_comments_and_unknown_keys();
    test_valid_file();
    test_whitespace_tolerance();
    test_app_replaces_builtins();
    test_app_name_limits();
    test_bad_app_names();
    test_app_count_overflow();
    test_panel_height();
    printf("\ntest_config: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
