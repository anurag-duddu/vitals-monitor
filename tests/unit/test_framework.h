/**
 * @file test_framework.h
 * @brief Minimal unit test framework for the vitals monitor project
 *
 * Provides assertion macros with automatic pass/fail counting and
 * a summary printer. No external dependencies beyond stdio/string/math.
 *
 * Counters are declared extern here and defined in test_runner.c
 * so they are shared across all translation units.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <math.h>

/* Global counters — defined in test_runner.c (or test_integration_runner.c)
 *
 * IMPORTANT: One-Definition Rule (ODR) for test counters.
 *
 * tf_total, tf_pass, and tf_fail are declared `extern` here so every
 * translation unit that #includes this header sees the same counters.
 * They MUST be defined exactly ONCE in the main test runner .c file:
 *
 *     int tf_total = 0;
 *     int tf_pass  = 0;
 *     int tf_fail  = 0;
 *
 * Do NOT declare these as `static` in any translation unit, and do NOT
 * add a second definition in any test .c file.  If you add a new test
 * runner binary, define them once in that runner's main .c file.
 *
 * For unit tests  -> defined in tests/unit/test_runner.c
 * For integration -> defined in tests/integration/test_integration_runner.c
 */
extern int tf_total;
extern int tf_pass;
extern int tf_fail;

#define ASSERT_TRUE(cond) do { \
    tf_total++; \
    if (!(cond)) { \
        printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ_INT(a, b) do { \
    tf_total++; \
    if ((a) != (b)) { \
        printf("  FAIL %s:%d: %d != %d\n", __FILE__, __LINE__, (int)(a), (int)(b)); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define ASSERT_NEQ_INT(a, b) do { \
    tf_total++; \
    if ((a) == (b)) { \
        printf("  FAIL %s:%d: %d == %d (expected not equal)\n", __FILE__, __LINE__, (int)(a), (int)(b)); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define ASSERT_STR_EQ(a, b) do { \
    tf_total++; \
    if (strcmp((a), (b)) != 0) { \
        printf("  FAIL %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define ASSERT_FLOAT_NEAR(a, b, eps) do { \
    tf_total++; \
    if (fabs((double)(a) - (double)(b)) >= (eps)) { \
        printf("  FAIL %s:%d: %.4f != %.4f (eps=%.4f)\n", __FILE__, __LINE__, \
               (double)(a), (double)(b), (double)(eps)); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define ASSERT_NULL(ptr) do { \
    tf_total++; \
    if ((ptr) != NULL) { \
        printf("  FAIL %s:%d: expected NULL\n", __FILE__, __LINE__); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define ASSERT_NOT_NULL(ptr) do { \
    tf_total++; \
    if ((ptr) == NULL) { \
        printf("  FAIL %s:%d: unexpected NULL\n", __FILE__, __LINE__); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define ASSERT_GT_INT(a, b) do { \
    tf_total++; \
    if (!((a) > (b))) { \
        printf("  FAIL %s:%d: %d not > %d\n", __FILE__, __LINE__, (int)(a), (int)(b)); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define ASSERT_GE_INT(a, b) do { \
    tf_total++; \
    if (!((a) >= (b))) { \
        printf("  FAIL %s:%d: %d not >= %d\n", __FILE__, __LINE__, (int)(a), (int)(b)); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define RUN_SUITE(fn) do { \
    printf("\n--- %s ---\n", #fn); \
    fn(); \
} while (0)

#define TEST_SUMMARY() do { \
    printf("\n========================================\n"); \
    printf("RESULTS: %d total, %d passed, %d failed\n", tf_total, tf_pass, tf_fail); \
    printf("========================================\n"); \
} while (0)

/* ── Setup / Teardown ─────────────────────────────────────── */

/**
 * TEST_SETUP(fn) / TEST_TEARDOWN(fn): Register per-suite setup/teardown.
 * Call at the start of each test suite function.
 * The registered function is called by RUN_TEST() before/after each test.
 */
static void (*_tf_setup_fn)(void)    = NULL;
static void (*_tf_teardown_fn)(void) = NULL;

#define TEST_SETUP(fn)    (_tf_setup_fn = (fn))
#define TEST_TEARDOWN(fn) (_tf_teardown_fn = (fn))

#define RUN_TEST(fn) do { \
    if (_tf_setup_fn) _tf_setup_fn(); \
    printf("  [TEST] %s\n", #fn); \
    fn(); \
    if (_tf_teardown_fn) _tf_teardown_fn(); \
} while (0)

/* ── Comparison assertions (less-than / less-or-equal) ──── */

#define ASSERT_LT_INT(a, b) do { \
    tf_total++; \
    if (!((a) < (b))) { \
        printf("  FAIL %s:%d: %d not < %d\n", __FILE__, __LINE__, (int)(a), (int)(b)); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define ASSERT_LE_INT(a, b) do { \
    tf_total++; \
    if (!((a) <= (b))) { \
        printf("  FAIL %s:%d: %d not <= %d\n", __FILE__, __LINE__, (int)(a), (int)(b)); \
        tf_fail++; \
    } else { \
        tf_pass++; \
    } \
} while (0)

/* ── Early-return assertions (for tests where continuing is unsafe) ── */

#define ASSERT_NOT_NULL_OR_RETURN(ptr) do { \
    tf_total++; \
    if ((ptr) == NULL) { \
        printf("  FAIL %s:%d: unexpected NULL (aborting test)\n", __FILE__, __LINE__); \
        tf_fail++; \
        return; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#define ASSERT_TRUE_OR_RETURN(cond) do { \
    tf_total++; \
    if (!(cond)) { \
        printf("  FAIL %s:%d: %s (aborting test)\n", __FILE__, __LINE__, #cond); \
        tf_fail++; \
        return; \
    } else { \
        tf_pass++; \
    } \
} while (0)

#endif /* TEST_FRAMEWORK_H */
