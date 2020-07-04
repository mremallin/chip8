/*
 * chip8.h - CHIP8 Interpreter Core
 *
 * Mike Mallin, 2020
 */

#ifndef __CHIP8_H__
#define __CHIP8_H__

#include "chip8_utils.h"

/**
 * @brief       Initializes the interpreter core
 */
void chip8_init(void);

/**
 * @brief       Loads a program into the interpreter
 *
 * @param[in]   The path to the file
 */
void chip8_load_program(char *file_path);


#define DISPLAY_WIDTH_PIXELS    64
#define DISPLAY_HEIGHT_PIXELS   32
#define BITS2BYTES(_bits) (_bits / 8)

/**
 * @brief       Steps the interpreter one instruction
 */
void chip8_step(void);

/**
 * @brief       Gets the contents of VRAM
 *
 * @returns     The VRAM buffer, packed as bytes.
 */
uint8_t *chip8_get_vram(void);

/**
 * @brief       Informs the interpreter core about a key press
 *
 * @param[in]   The key being pressed
 */
void chip8_notify_key_pressed(chip8_key_et key);

#endif /* __CHIP8_H__ */
