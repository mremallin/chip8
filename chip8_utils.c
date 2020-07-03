/*
 * chip8_utils - CHIP8 Interpreter Utilities
 *
 * Mike Mallin, 2020
 */

#include "chip8_utils.h"

#include <stdlib.h>
#include <assert.h>

/* The current state of a given key */
static bool s_keys_pressed[CHIP8_KEY_MAX] = { false };

void
key_pressed (chip8_key_et key)
{
	assert(key < CHIP8_KEY_MAX);
	s_keys_pressed[key] = true;
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
get_random_byte (void)
{
	return (uint8_t)rand();
}
