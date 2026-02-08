cmake_minimum_required(VERSION 3.10)
project(vitals_monitor_simulator C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compile flags
add_compile_options(-Wall -Wextra -O2)

# Define preprocessor macros for LVGL includes
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE -DLV_CONF_INCLUDE_SIMPLE)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl
)

# Collect LVGL source files
file(GLOB_RECURSE LVGL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src/*.c
)

# SDL wrapper source files
set(SDL_WRAPPER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sdl_display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sdl_input.c
)

# Application source files
set(APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)

# Create executable
add_executable(simulator
    ${LVGL_SOURCES}
    ${SDL_WRAPPER_SOURCES}
    ${APP_SOURCES}
)

# Link SDL2
target_link_libraries(simulator ${SDL2_LIBRARIES})

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
