/**
 * @file test_integration_runner.c
 * @brief Main test runner for vitals monitor integration tests
 *
 * Runs all integration test suites and prints a summary. Returns
 * non-zero exit code if any test failed.
 *
 * Integration tests verify cross-module interactions:
 *   - alarm_engine + trend_db (alarm persistence)
 *   - auth_manager + audit_log (authentication audit trail)
 *   - patient_data + trend_db (patient vitals association)
 */

#include "test_framework.h"

/* Definition of global test counters (declared extern in test_framework.h) */
int tf_total = 0;
int tf_pass  = 0;
int tf_fail  = 0;

/* Forward declarations of integration test suite functions */
extern void test_alarm_db_integration(void);
extern void test_auth_audit_integration(void);
extern void test_patient_trends_integration(void);

int main(void) {
    printf("========================================\n");
    printf("  Vitals Monitor Integration Tests\n");
    printf("========================================\n");

    RUN_SUITE(test_alarm_db_integration);
    RUN_SUITE(test_auth_audit_integration);
    RUN_SUITE(test_patient_trends_integration);

    TEST_SUMMARY();

    return tf_fail > 0 ? 1 : 0;
}
