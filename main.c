/*
 * chip8 - CHIP8 Interpreter
 *
 * Mike Mallin, 2020
 */

#include <stdbool.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 	640
#define WINDOW_HEIGHT 	480

#define ERROR_LOG(...) (fprintf(stderr, __VA_ARGS__))

/* Variables related to SDL window and rendering */
static SDL_Window       *main_window = NULL;

static SDL_Window *
get_window (void)
{
	return main_window;
}

static void
run_main_event_loop (void)
{
    bool loop = true;
    uint32_t frame_start_ticks = 0;
    uint32_t frame_end_ticks = 0;
    uint32_t frame_delta_ticks = 0;

    printf("Entering main loop\n");

    while (loop) {
        SDL_Event event;

        /* Bump the delta in case the framerate is too fast */
        if (frame_delta_ticks == 0) {
            frame_delta_ticks = 1;
        }

        printf("FPS: %3.3f\r", 1/(frame_delta_ticks*0.001f));

        frame_start_ticks = SDL_GetTicks();

        /* Process incoming events.
         * NOTE: This will chew up 100% CPU.
         * Would be nice to have a better way to wait between drawing frames */
        if (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                loop = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_q) {
                    loop = false;
                }   
            }
        }

        /* Update & render go here */

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
}

static void
at_exit (void)
{
    if (get_window()) {
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

    run_main_event_loop();

    return EXIT_SUCCESS;
}
