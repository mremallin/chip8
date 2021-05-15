/*
 * chip8_utils - CHIP8 Interpreter Utilities
 *
 * Mike Mallin, 2020
 */

#include "chip8_utils.h"

#include <stdlib.h>
#include <assert.h>
#include <SDL2/SDL.h>

#include "chip8.h"
#include "chip8_sound.h"

/* The current state of a given key */
static bool s_keys_pressed[CHIP8_KEY_MAX] = { false };

static uint32_t s_delay_timer = 0;
static uint32_t s_delay_timer_started_at = 0;
static uint32_t s_sound_timer = 0;
static uint32_t s_sound_timer_started_at = 0;

uint8_t
get_random_byte (void)
{
    return (uint8_t)rand();
}

void
key_pressed (chip8_key_et key)
{
    assert(key < CHIP8_KEY_MAX);
    s_keys_pressed[key] = true;
    chip8_notify_key_pressed(key);
}

void
key_released (chip8_key_et key)
{
    assert(key < CHIP8_KEY_MAX);
    s_keys_pressed[key] = false;
}

bool
get_key_pressed (chip8_key_et key)
{
    assert(key < CHIP8_KEY_MAX);
    return s_keys_pressed[key];
}

uint8_t
get_delay_timer_remaining (void)
{
    return s_delay_timer;
}

void
set_delay_timer (uint8_t ticks)
{
    s_delay_timer_started_at = SDL_GetTicks();
    s_delay_timer = ticks;
}

void
set_sound_timer (uint8_t ticks)
{
    s_sound_timer_started_at = SDL_GetTicks();
    s_sound_timer = ticks;
}

void
update_timers (void)
{
    if (s_delay_timer &&
        ((SDL_GetTicks() - s_delay_timer_started_at) >= 16)) {
        s_delay_timer_started_at = SDL_GetTicks();
        s_delay_timer -= 1;
    }

    if (s_sound_timer &&
        ((SDL_GetTicks() - s_sound_timer_started_at) >= 16)) {
        s_sound_timer_started_at = SDL_GetTicks();
        s_sound_timer -= 1;

        if (s_sound_timer == 0) {
            /* Beep */
            chip8_sound_beep();
        }
    }
}
