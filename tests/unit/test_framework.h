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

/* Global counters — defined in test_runner.c */
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

#endif /* TEST_FRAMEWORK_H */
