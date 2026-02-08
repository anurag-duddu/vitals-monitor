/**
 * @file main.c
 * @brief LVGL simulator main application for Vitals Monitor
 *
 * This is the Mac development simulator using SDL2. It allows rapid UI development
 * without needing the target hardware.
 */

#include "lvgl.h"
#include "sdl_display.h"
#include "sdl_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

static bool running = true;

/* Signal handler for clean exit */
static void signal_handler(int signum) {
    (void)signum;
    printf("\nShutting down simulator...\n");
    running = false;
}

/**
 * @brief Create a simple demo screen with "Hello, Vitals Monitor"
 */
static void create_demo_screen(void) {
    /* Get the active screen */
    lv_obj_t *scr = lv_screen_active();

    /* Set background color */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    /* Create a label for the title */
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Hello, Vitals Monitor");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -40);

    /* Create a subtitle label */
    lv_obj_t *subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "LVGL Simulator Running\nPhase 0 Complete!");
    lv_obj_set_style_text_align(subtitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x00FF00), 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 20);

    /* Create a status label at the bottom */
    lv_obj_t *status = lv_label_create(scr);
    lv_label_set_text(status, "Ready for Phase 1: Basic UI Framework");
    lv_obj_set_style_text_font(status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -20);

    /* Add a simple button */
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 80);
    lv_obj_set_size(btn, 200, 60);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);

    printf("Demo screen created successfully\n");
}

/**
 * @brief Main application entry point
 */
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("  Bedside Vitals Monitor - Simulator\n");
    printf("  Phase 0: Development Environment\n");
    printf("========================================\n\n");

    /* Set up signal handlers for clean exit */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize LVGL */
    lv_init();
    printf("LVGL initialized (version %d.%d.%d)\n",
           lv_version_major(), lv_version_minor(), lv_version_patch());

    /* Initialize SDL display */
    if (!sdl_display_init(800, 480)) {
        fprintf(stderr, "Failed to initialize SDL display\n");
        return 1;
    }

    /* Initialize SDL input */
    if (!sdl_input_init()) {
        fprintf(stderr, "Failed to initialize SDL input\n");
        sdl_display_deinit();
        return 1;
    }

    /* Create the demo screen */
    create_demo_screen();

    printf("\nSimulator running. Press Ctrl+C to exit.\n");
    printf("Window size: 800x480 (matching target hardware)\n\n");

    /* Main event loop */
    while (running) {
        /* Process SDL events */
        if (!sdl_input_process_events()) {
            running = false;
            break;
        }

        /* Handle LVGL tasks */
        uint32_t time_till_next = lv_timer_handler();

        /* Sleep for a short time */
        usleep(time_till_next * 1000);
    }

    /* Cleanup */
    printf("Cleaning up...\n");
    sdl_display_deinit();

    printf("Simulator exited cleanly.\n");
    return 0;
}
