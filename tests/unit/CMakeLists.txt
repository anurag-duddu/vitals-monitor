cmake_minimum_required(VERSION 3.10)
project(vitals_monitor_tests C)
set(CMAKE_C_STANDARD 99)
add_definitions(-DSIMULATOR_BUILD=1 -DLV_CONF_INCLUDE_SIMPLE)

# ── Compiler warnings ────────────────────────────────────────
# Match simulator warning flags; SQLite amalgamation overrides with -w below.
add_compile_options(-Wall -Wextra -Wshadow -Wconversion -Wdouble-promotion -Wformat=2 -Wundef -g)

# ── Code coverage option ─────────────────────────────────────
option(ENABLE_COVERAGE "Enable code coverage with gcov/lcov" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(-fprofile-arcs -ftest-coverage)
    link_libraries(gcov)
endif()

# ── AddressSanitizer option (for development/debugging) ──────
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer ENABLED")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# ── Include paths ──────────────────────────────────────────
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/ui/themes
    ${CMAKE_CURRENT_SOURCE_DIR}/../../simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/../../simulator/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/../../simulator/sqlite3
)

# ── SQLite amalgamation (compile with warnings suppressed) ──
set(SQLITE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../simulator/sqlite3/sqlite3.c)
set_source_files_properties(${SQLITE_SRC} PROPERTIES
    COMPILE_DEFINITIONS "SQLITE_THREADSAFE=0;SQLITE_OMIT_LOAD_EXTENSION"
    COMPILE_OPTIONS "-w"
)

# ── Core modules under test ────────────────────────────────
set(MODULES_UNDER_TEST
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/alarm_engine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/patient_data.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/settings_store.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/auth_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/audit_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/fhir_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/sync_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/trend_db.c
)

# ── Test executable ────────────────────────────────────────
add_executable(test_runner
    test_runner.c
    test_alarm_engine.c
    test_patient_data.c
    test_settings_store.c
    test_auth_manager.c
    test_audit_log.c
    test_fhir_client.c
    test_sync_queue.c
    test_trend_db.c
    ${MODULES_UNDER_TEST}
    ${SQLITE_SRC}
)

target_link_libraries(test_runner m)

# ── Enable CTest integration ──────────────────────────────
enable_testing()
add_test(NAME unit_tests COMMAND test_runner)
