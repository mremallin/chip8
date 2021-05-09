/*
 * chip8 - CHIP8 Interpreter
 *
 * Mike Mallin, 2020
 */

#include <stdbool.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>

#include "chip8.h"
#include "chip8_utils.h"

#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   320

#define ERROR_LOG(...) (fprintf(stderr, __VA_ARGS__))

/* Variables related to SDL window and rendering */
static SDL_Window       *main_window = NULL;
static SDL_Texture      *screen_texture = NULL;
static SDL_Renderer     *renderer = NULL;

static bool              is_running = true;
static uint32_t         *screen_backing_store = NULL;

static SDL_Window *
get_window (void)
{
    return main_window;
}

/* The chip8 keyboard is layed out as follows:
 *
 * -----------------
 * | 1 | 2 | 3 | C |
 * -----------------
 * | 4 | 5 | 6 | D |
 * -----------------
 * | 7 | 8 | 9 | E |
 * -----------------
 * | A | 0 | B | F |
 * -----------------
 *
 * This is mapped to a QWERTY keyboard via the following
 * layout:
 *
 * -----------------
 * | 1 | 2 | 3 | 4 |
 * -----------------
 * | Q | W | E | R |
 * -----------------
 * | A | S | D | F |
 * -----------------
 * | Z | X | C | V |
 * -----------------
 *
 */

static chip8_key_et
map_sdl_key_to_chip8_key (SDL_Event *event)
{
    chip8_key_et key = CHIP8_KEY_MAX;

    switch (event->key.keysym.sym) {
        default:
            break;
        case SDLK_1:
            key = CHIP8_KEY_1;
            break;
        case SDLK_2:
            key = CHIP8_KEY_2;
            break;
        case SDLK_3:
            key = CHIP8_KEY_3;
            break;
        case SDLK_4:
            key = CHIP8_KEY_C;
            break;
        case SDLK_q:
            key = CHIP8_KEY_4;
            break;
        case SDLK_w:
            key = CHIP8_KEY_5;
            break;
        case SDLK_e:
            key = CHIP8_KEY_6;
            break;
        case SDLK_r:
            key = CHIP8_KEY_D;
            break;
        case SDLK_a:
            key = CHIP8_KEY_7;
            break;
        case SDLK_s:
            key = CHIP8_KEY_8;
            break;
        case SDLK_d:
            key = CHIP8_KEY_9;
            break;
        case SDLK_f:
            key = CHIP8_KEY_E;
            break;
        case SDLK_z:
            key = CHIP8_KEY_A;
            break;
        case SDLK_x:
            key = CHIP8_KEY_0;
            break;
        case SDLK_c:
            key = CHIP8_KEY_B;
            break;
        case SDLK_v:
            key = CHIP8_KEY_F;
            break;
    }

    return (key);
}

static void
handle_key_down_event (SDL_Event *event)
{
    chip8_key_et key;
    assert(event != NULL);

    key = map_sdl_key_to_chip8_key(event);
    if (key != CHIP8_KEY_MAX) {
        key_pressed(key);
    }
}

static void
handle_key_up_event (SDL_Event *event)
{
    chip8_key_et key;
    assert(event != NULL);

    key = map_sdl_key_to_chip8_key(event);
    if (key != CHIP8_KEY_MAX) {
        key_released(key);
    }
}

static void
paint_screen (void)
{
    uint8_t *vram = chip8_get_vram();
    uint32_t *gpu_pixels = NULL;
    int pitch = 0;

    int x;
    int y;

    SDL_LockTexture(screen_texture, NULL, (void **)&gpu_pixels, &pitch);

    for (y = 0; y < DISPLAY_HEIGHT_PIXELS; y++) {
        for (x = 0; x < DISPLAY_WIDTH_PIXELS; x++) {
            if (vram[y * DISPLAY_WIDTH_PIXELS + x] != 0) {
                gpu_pixels[y * DISPLAY_WIDTH_PIXELS + x] = 0xFFFFFFFF;
            } else {
                gpu_pixels[y * DISPLAY_WIDTH_PIXELS + x] = 0x00000000;
            }
        }
    }

    SDL_UnlockTexture(screen_texture);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

typedef struct minmax32_st_ {
    uint32_t min;
    uint32_t max;
} minmax32_st;

typedef struct frame_histogram_st_ {
    minmax32_st event;
    minmax32_st interpreter;
    minmax32_st drawing;
} frame_histogram_st;

static void
histogram_reset (minmax32_st *hist)
{
    hist->min = UINT32_MAX;
    hist->max = 0;
}

static void
update_histogram (minmax32_st *hist, uint32_t time)
{
    if (time <= hist->min) {
        hist->min = time;
    } else if (time >= hist->max) {
        hist->max = time;
    }
}

static void
print_histogram (frame_histogram_st *hist)
{
    printf("Events     : %u - %u\n", hist->event.min, hist->event.max);
    printf("Interpreter: %u - %u\n", hist->interpreter.min, hist->interpreter.max);
    printf("Drawing    : %u - %u\n", hist->drawing.min, hist->drawing.max);
}

static void
run_main_event_loop (void)
{
    uint32_t step_frame_delta_ticks = 0;
    uint32_t timers_frame_delta_ticks = 0;
    uint32_t screen_frame_delta_ticks = 0;
    uint32_t frame_count = 0;
    frame_histogram_st hist = {{0}};
    SDL_Event event;

    histogram_reset(&hist.event);
    histogram_reset(&hist.interpreter);
    histogram_reset(&hist.drawing);

    printf("Entering main loop\n");

    while (is_running) {

        /* Process incoming events.
         * NOTE: This will chew up 100% CPU.
         * Would be nice to have a better way to wait between drawing frames */
        if (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                is_running = false;
            } else if (event.type == SDL_KEYDOWN) {
                handle_key_down_event(&event);
            } else if (event.type == SDL_KEYUP) {
                handle_key_up_event(&event);
            }
        }

        /* Update & render go here */
        step_frame_delta_ticks = SDL_GetTicks();
        chip8_step();
        step_frame_delta_ticks = SDL_GetTicks() - step_frame_delta_ticks;
        update_histogram(&hist.interpreter, step_frame_delta_ticks);

        timers_frame_delta_ticks = SDL_GetTicks();
        update_timers();
        timers_frame_delta_ticks = SDL_GetTicks() - timers_frame_delta_ticks;
        update_histogram(&hist.interpreter, timers_frame_delta_ticks);

        screen_frame_delta_ticks = SDL_GetTicks();
        paint_screen();
        screen_frame_delta_ticks = SDL_GetTicks() - screen_frame_delta_ticks;
        update_histogram(&hist.drawing, screen_frame_delta_ticks);

        //printf("Times: %u, %u, %u\n", step_frame_delta_ticks, timers_frame_delta_ticks, screen_frame_delta_ticks);

        if (frame_count >= 1000) {
            frame_count = 0;
            print_histogram(&hist);
        } else {
            frame_count++;
        }
    }

    printf("\nExiting...\n");
}

static void
init_sdl (void)
{
    int rc = 0;

    rc = SDL_Init(SDL_INIT_VIDEO);
    if (rc != 0) {
        ERROR_LOG("SDL_Init failed (%u): %s\n", rc, SDL_GetError());
        exit(rc);
    }

    main_window = SDL_CreateWindow("CHIP8",
                                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (main_window == NULL) {
        ERROR_LOG("SDL_CreateWindow failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    renderer = SDL_CreateRenderer(get_window(), -1,
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        ERROR_LOG("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    screen_texture = SDL_CreateTexture(renderer,
                                       SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       DISPLAY_WIDTH_PIXELS, DISPLAY_HEIGHT_PIXELS);
    if (screen_texture == NULL) {
        ERROR_LOG("SDL_CreateTexture failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    screen_backing_store = malloc(DISPLAY_WIDTH_PIXELS * DISPLAY_HEIGHT_PIXELS * 4);
    assert(screen_backing_store);
}

static void
at_exit (void)
{
    if (get_window()) {
        SDL_DestroyTexture(screen_texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(get_window());
        main_window = NULL;
        SDL_Quit();
    }
}

static void
print_renderer_info (void)
{
    SDL_RendererInfo render_info;
    int i;

    SDL_GetRendererInfo(renderer, &render_info);

    for (i = 0; i < render_info.num_texture_formats; i++) {
        printf("Format: %s\n", SDL_GetPixelFormatName(render_info.texture_formats[i]));
    }
}

int
main (int argc, char *argv[])
{
    /* Setup program exit cleanup routines */
    atexit(at_exit);

    init_sdl();
    chip8_init();

    if (argc >= 2) {
        chip8_load_program(argv[1]);
    } else {
        printf("Must provide a program to load!\n");
        exit(EXIT_FAILURE);
    }

    print_renderer_info();

    run_main_event_loop();

    return EXIT_SUCCESS;
}
