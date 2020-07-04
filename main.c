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

    int x;
    int y;
    int px;

    for (x = 0; x < DISPLAY_WIDTH_PIXELS; x += 8) {
        for (y = 0; y < DISPLAY_HEIGHT_PIXELS; y += 8) {
            /* Each u8 represents 8 pixels, it is packed 1bpp */
            for (px = 0; px < 8; px++) {
                if (vram[(x/8) * BITS2BYTES(DISPLAY_WIDTH_PIXELS) + (y/8)] & (1 << px)) {
                    screen_backing_store[x * WINDOW_WIDTH + y] = 0xFFFFFFFF;
                } else {
                    screen_backing_store[x * WINDOW_WIDTH + y] = 0;
                }
            }
        }
    }

    SDL_UpdateTexture(screen_texture, NULL, screen_backing_store,
                      WINDOW_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static void
run_main_event_loop (void)
{
    uint32_t frame_start_ticks = 0;
    uint32_t frame_end_ticks = 0;
    uint32_t frame_delta_ticks = 0;

    printf("Entering main loop\n");

    while (is_running) {
        SDL_Event event;

        /* Bump the delta in case the framerate is too fast */
        if (frame_delta_ticks == 0) {
            frame_delta_ticks = 1;
        }

        //printf("FPS: %3.3f\r", 1/(frame_delta_ticks*0.001f));

        frame_start_ticks = SDL_GetTicks();

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
        chip8_step();
        update_timers();
        paint_screen();

        frame_end_ticks = SDL_GetTicks();
        frame_delta_ticks = frame_end_ticks - frame_start_ticks;
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
                                  SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        ERROR_LOG("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    screen_texture = SDL_CreateTexture(renderer,
                                       SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       WINDOW_WIDTH, WINDOW_HEIGHT);
    if (screen_texture == NULL) {
        ERROR_LOG("SDL_CreateTexture failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    screen_backing_store = malloc(WINDOW_WIDTH * WINDOW_HEIGHT * 4);
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
    }

    run_main_event_loop();

    return EXIT_SUCCESS;
}
