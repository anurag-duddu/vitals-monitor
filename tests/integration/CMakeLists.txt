cmake_minimum_required(VERSION 3.10)
project(vitals_monitor_integration_tests C)
set(CMAKE_C_STANDARD 99)

# ── Compiler warnings ────────────────────────────────────────
# Match simulator warning flags; LVGL and SQLite sources override with -w below.
add_compile_options(-Wall -Wextra -Wshadow -Wconversion -Wdouble-promotion -Wformat=2 -Wundef -g)

# ── Preprocessor defines (required for LVGL headers) ──────
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE -DLV_CONF_INCLUDE_SIMPLE -DSIMULATOR_BUILD=1)

# ── Code coverage option ────────────────────────────────────
option(ENABLE_COVERAGE "Enable code coverage with gcov/lcov" OFF)
if(ENABLE_COVERAGE)
    message(STATUS "Code coverage ENABLED")
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../simulator/sqlite3
    ${CMAKE_CURRENT_SOURCE_DIR}/../../simulator/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/../unit
)

# ── SQLite amalgamation (compile with warnings suppressed) ──
set(SQLITE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../simulator/sqlite3/sqlite3.c)
set_source_files_properties(${SQLITE_SRC} PROPERTIES
    COMPILE_DEFINITIONS "SQLITE_THREADSAFE=0;SQLITE_OMIT_LOAD_EXTENSION"
    COMPILE_OPTIONS "-w"
)

# ── LVGL sources (needed for theme_vitals.h -> lvgl.h chain) ──
# WARNING: GLOB_RECURSE does not automatically re-run CMake when files are
# added or removed. Re-run cmake manually after updating the LVGL submodule.
file(GLOB_RECURSE LVGL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../simulator/lvgl/src/*.c
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../simulator/lvgl/src)
# Suppress warnings on LVGL sources
set_source_files_properties(${LVGL_SOURCES} PROPERTIES
    COMPILE_OPTIONS "-w"
)

# ── Core modules under test ────────────────────────────────
set(MODULES_UNDER_TEST
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/alarm_engine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/patient_data.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/settings_store.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/auth_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/audit_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/trend_db.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/ui/themes/theme_vitals.c
)

# ── Test executable ────────────────────────────────────────
add_executable(integration_test_runner
    test_integration_runner.c
    test_alarm_db_integration.c
    test_auth_audit_integration.c
    test_patient_trends_integration.c
    test_failure_scenarios.c
    ${MODULES_UNDER_TEST}
    ${SQLITE_SRC}
    ${LVGL_SOURCES}
)

target_link_libraries(integration_test_runner m)

# ── Enable CTest integration ──────────────────────────────
enable_testing()
add_test(NAME integration_tests COMMAND integration_test_runner)
