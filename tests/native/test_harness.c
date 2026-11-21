/*
 * test_harness.c - host test driver scaffolding and self-check.
 *
 * Runs on any Linux with gcc, no OnyxOS or onyxcc needed:
 *
 *   make -C tests/native test
 *
 * The CHECK macro and the "N checks, M failures" summary are the shape
 * every OnyxDesktop test driver reuses (test_config, test_ui, test_desk
 * arrive in later tasks). A failed check increments the counter and the
 * driver returns non-zero, so CI cannot pass on a broken suite.
 */
#include <stdio.h>

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

static void test_harness_plumbing(void) {
    printf("harness plumbing:\n");
    CHECK(1 + 1 == 2);
    CHECK(sizeof(char) == 1);
    CHECK(1 != 0);
}

int main(void) {
    test_harness_plumbing();
    printf("\ntest_harness: %d checks, %d failures\n", checks, failures);
    return failures > 0;
}
