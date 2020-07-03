/*
 * chip8.h - CHIP8 Interpreter Core
 *
 * Mike Mallin, 2020
 */

#ifndef __CHIP8_H__
#define __CHIP8_H__

#include "chip8_utils.h"

/**
 * @brief       Informs the interpreter core about a key press
 *
 * @param[in]   The key being pressed
 */
void chip8_notify_key_pressed(chip8_key_et key);

#endif /* __CHIP8_H__ */
