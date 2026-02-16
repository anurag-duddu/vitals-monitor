cmake_minimum_required(VERSION 3.10)
project(vitals_monitor_simulator C)

# ============================================================
# Build type support (default to Release for optimized builds)
# ============================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
    message(STATUS "No build type specified, defaulting to Release")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============================================================
# Compiler warnings
# ============================================================
# Base warnings for all code (including LVGL); LVGL sources get -w override below
add_compile_options(-Wall -Wextra)
# Extra warnings (applied globally but LVGL/SQLite sources override with -w)
# NOTE: -Wconversion may trigger on LVGL header macros in project code;
# suppress per-file with set_source_files_properties if needed.
add_compile_options(-Wshadow -Wconversion -Wdouble-promotion -Wformat=2 -Wundef -Wformat-security)

# ============================================================
# Security hardening flags
# ============================================================
add_compile_options(-fstack-protector-strong -fPIE)

# _FORTIFY_SOURCE requires optimization (-O1 or higher); only enable for
# Release and RelWithDebInfo builds to avoid compiler warnings/errors in Debug.
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_definitions(-D_FORTIFY_SOURCE=2)
endif()

# Linker hardening (Linux-only; macOS linker does not support -z flags)
if(NOT APPLE)
    add_link_options(-pie -Wl,-z,relro,-z,now)
else()
    # On macOS, PIE is the default for executables; -pie is implicit.
endif()

# ============================================================
# AddressSanitizer option (for development/debugging)
# ============================================================
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer ENABLED")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# ============================================================
# Build Configuration
# ============================================================
# VITALS_PROVIDER: mock (default for simulator), ipc, direct
# Set via: cmake -DVITALS_PROVIDER=mock ..
# ============================================================

set(VITALS_PROVIDER "mock" CACHE STRING "Vitals data provider: mock, ipc, direct")
set_property(CACHE VITALS_PROVIDER PROPERTY STRINGS mock ipc direct)

message(STATUS "Vitals Provider: ${VITALS_PROVIDER}")

# Define preprocessor macros based on provider
if(VITALS_PROVIDER STREQUAL "mock")
    add_definitions(-DVITALS_PROVIDER_MOCK)
elseif(VITALS_PROVIDER STREQUAL "ipc")
    add_definitions(-DVITALS_PROVIDER_IPC -DUSE_NANOMSG)
elseif(VITALS_PROVIDER STREQUAL "direct")
    add_definitions(-DVITALS_PROVIDER_DIRECT)
endif()

# Define preprocessor macros for LVGL includes
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE -DLV_CONF_INCLUDE_SIMPLE)

# Simulator-specific define
add_definitions(-DSIMULATOR_BUILD=1)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/screens
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/themes
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/icons
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/ipc
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3
)

# Collect LVGL source files
# WARNING: GLOB_RECURSE does not automatically re-run CMake when files are
# added or removed. Re-run cmake manually after updating the LVGL submodule.
# See CMake policy CMP0009 and cmake.org/cmake/help/latest/command/file.html
file(GLOB_RECURSE LVGL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src/*.c
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src)

# Suppress all warnings on LVGL sources (third-party code)
set_source_files_properties(${LVGL_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")

# SDL wrapper source files
set(SDL_WRAPPER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sdl_display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sdl_input.c
)

# Application source files
set(APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)

# UI source files (from src/)
set(UI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/screens/screen_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/screens/screen_main_vitals.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/screens/screen_trends.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/screens/screen_alarms.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/screens/screen_patient.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/screens/screen_settings.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/screens/screen_login.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/screens/screen_audit_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/widgets/widget_numeric_display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/widgets/widget_alarm_banner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/widgets/widget_nav_bar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/widgets/widget_waveform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/themes/theme_vitals.c
)

# SQLite amalgamation (single-file build, no external dependency)
set(SQLITE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.c
)

# Core source files (from src/)
set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/waveform_gen.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/trend_db.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/network_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fhir_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/sync_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/alarm_engine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/patient_data.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/settings_store.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/auth_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/audit_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/abdm_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/ipc/ipc_transport.c
)

# Service source files (from src/services/)
set(SERVICE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/services/service_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/services/sensor_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/services/alarm_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/services/network_service.c
)

# Select vitals provider implementation based on build configuration
if(VITALS_PROVIDER STREQUAL "mock")
    list(APPEND CORE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/mock_data.c
    )
    message(STATUS "Using MOCK vitals provider (simulator mode)")
elseif(VITALS_PROVIDER STREQUAL "ipc")
    list(APPEND CORE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/vitals_provider_ipc.c
    )
    # Find nanomsg library
    find_library(NANOMSG_LIB nanomsg)
    if(NOT NANOMSG_LIB)
        message(WARNING "nanomsg library not found - IPC provider will be stub only")
    endif()
    message(STATUS "Using IPC vitals provider (target mode)")
elseif(VITALS_PROVIDER STREQUAL "direct")
    # Note: direct mode would use HAL-based provider in future
    # For now, fallback to mock implementation
    list(APPEND CORE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/mock_data.c
    )
    message(STATUS "Using DIRECT vitals provider (single-process target test - currently uses mock)")
endif()

# Create executable
add_executable(simulator
    ${LVGL_SOURCES}
    ${SDL_WRAPPER_SOURCES}
    ${APP_SOURCES}
    ${UI_SOURCES}
    ${CORE_SOURCES}
    ${SERVICE_SOURCES}
    ${SQLITE_SOURCES}
)

# SQLite compile-time configuration (single-threaded, minimal footprint)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.c PROPERTIES
    COMPILE_DEFINITIONS "SQLITE_THREADSAFE=0;SQLITE_OMIT_LOAD_EXTENSION;SQLITE_DEFAULT_MEMSTATUS=0;SQLITE_OMIT_PROGRESS_CALLBACK;SQLITE_OMIT_SHARED_CACHE"
    COMPILE_OPTIONS "-w"
)

# Link SDL2
target_link_libraries(simulator ${SDL2_LIBRARIES} m)

# macOS specific: Link against AppKit framework for SDL2
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(simulator ${COCOA_LIBRARY})
endif()

# Set output directory
set_target_properties(simulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Print configuration
message(STATUS "LVGL simulator configuration:")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  SDL2 Libraries: ${SDL2_LIBRARIES}")
message(STATUS "  SDL2 Include Dirs: ${SDL2_INCLUDE_DIRS}")
