/*
 * chip8_utils - CHIP8 Interpreter Utilities
 *
 * Mike Mallin, 2020
 */

#ifndef __CHIP8_UTILS_H__
#define __CHIP8_UTILS_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief      Gets a randomly generated byte.
 *
 * @return     The random byte.
 */
uint8_t get_random_byte(void);

/**
 * @brief      Enumeration of all Chip8 keys
 */
typedef enum {
    CHIP8_KEY_0,
    CHIP8_KEY_1,
    CHIP8_KEY_2,
    CHIP8_KEY_3,
    CHIP8_KEY_4,
    CHIP8_KEY_5,
    CHIP8_KEY_6,
    CHIP8_KEY_7,
    CHIP8_KEY_8,
    CHIP8_KEY_9,
    CHIP8_KEY_A,
    CHIP8_KEY_B,
    CHIP8_KEY_C,
    CHIP8_KEY_D,
    CHIP8_KEY_E,
    CHIP8_KEY_F,
    CHIP8_KEY_MAX
} chip8_key_et;

/**
 * @brief      Gets the key pressed.
 *
 * @param[in]  key   The key
 *
 * @return     true if the key is currently pressed, false otherwise
 */
bool get_key_pressed(chip8_key_et key);

/**
 * @brief      Tells the Chip8 interpreter about a pressed key
 *
 * @param[in]  key   The key
 */
void key_pressed(chip8_key_et key);

/**
 * @brief      Tells the Chip8 interpreter about a released key
 *
 * @param[in]  key   The key
 */
void key_released(chip8_key_et key);

/**
 * @brief      Gets the number of 1/60 ticks left in the delay timer
 *
 * @return     Number of ticks remaining
 */
uint8_t get_delay_timer_remaining(void);

/**
 * @brief      Sets the number of 1/60s ticks for the delay timer
 *
 * @param[in]  ticks to set
 */
void set_delay_timer(uint8_t ticks);

/**
 * @brief      Sets the number of 1/60s ticks for the sound timer
 *
 * @param[in]  ticks to set
 */
void set_sound_timer(uint8_t ticks);

/**
 * @brief      Triggers the timers to update
 */
void update_timers(void);

#endif /* __CHIP8_UTILS_H__ */
