/**
 * @file sdl_display.c
 * @brief SDL2 display driver for LVGL 9
 *
 * Simple SDL2 integration for LVGL v9.x simulator on Mac
 */

#include "sdl_display.h"
#include <stdlib.h>

/* SDL and LVGL globals */
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static lv_display_t *disp;
static uint32_t *fb;

/* Display dimensions */
static uint32_t hor_res = 800;
static uint32_t ver_res = 480;

/**
 * @brief Initialize SDL2 and create window
 */
bool sdl_display_init(uint32_t width, uint32_t height) {
    hor_res = width;
    ver_res = height;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return false;
    }

    /* Create window */
    window = SDL_CreateWindow(
        "Vitals Monitor Simulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        hor_res, ver_res,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    /* Create renderer */
    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    /* Create texture for framebuffer */
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        hor_res, ver_res
    );

    if (!texture) {
        SDL_Log("Texture creation failed: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    /* Allocate framebuffer */
    fb = (uint32_t *)malloc(hor_res * ver_res * sizeof(uint32_t));
    if (!fb) {
        SDL_Log("Framebuffer allocation failed");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    /* Create LVGL display */
    disp = lv_display_create(hor_res, ver_res);
    if (!disp) {
        SDL_Log("LVGL display creation failed");
        free(fb);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    /* Set display buffer */
    lv_display_set_flush_cb(disp, sdl_display_flush);
    lv_display_set_buffers(disp, fb, NULL, hor_res * ver_res * sizeof(uint32_t), LV_DISPLAY_RENDER_MODE_DIRECT);

    SDL_Log("SDL display initialized: %dx%d", hor_res, ver_res);
    return true;
}

/**
 * @brief Flush callback - copy framebuffer to SDL texture and render
 */
void sdl_display_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *color_p) {
    (void)disp_drv;  /* Unused */
    (void)area;      /* Unused in direct mode */

    /* Update texture with framebuffer */
    SDL_UpdateTexture(texture, NULL, fb, hor_res * sizeof(uint32_t));

    /* Render */
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    /* Tell LVGL we're done */
    lv_display_flush_ready(disp_drv);
}

/**
 * @brief Get the display object
 */
lv_display_t *sdl_display_get(void) {
    return disp;
}

/**
 * @brief Clean up SDL resources
 */
void sdl_display_deinit(void) {
    if (fb) {
        free(fb);
        fb = NULL;
    }
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();
}
