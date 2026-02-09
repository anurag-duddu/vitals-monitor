/**
 * @file test_runner.c
 * @brief Main test runner for vitals monitor unit tests
 *
 * Runs all test suites and prints a summary. Returns non-zero
 * exit code if any test failed.
 */

#include "test_framework.h"

/* Definition of global test counters (declared extern in test_framework.h) */
int tf_total = 0;
int tf_pass  = 0;
int tf_fail  = 0;

/* Forward declarations of test suite functions */
extern void test_alarm_engine(void);
extern void test_patient_data(void);
extern void test_settings_store(void);
extern void test_auth_manager(void);
extern void test_audit_log(void);

int main(void) {
    printf("========================================\n");
    printf("  Vitals Monitor Unit Tests\n");
    printf("========================================\n");

    RUN_SUITE(test_alarm_engine);
    RUN_SUITE(test_patient_data);
    RUN_SUITE(test_settings_store);
    RUN_SUITE(test_auth_manager);
    RUN_SUITE(test_audit_log);

    TEST_SUMMARY();

    return tf_fail > 0 ? 1 : 0;
}
