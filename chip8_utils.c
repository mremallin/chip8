/*
 * chip8_utils - CHIP8 Interpreter Utilities
 *
 * Mike Mallin, 2020
 */

#include "chip8_utils.h"

#include <stdlib.h>

uint8_t
get_random_byte (void)
{
	return (uint8_t)rand();
}
