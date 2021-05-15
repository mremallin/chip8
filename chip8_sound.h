/*
 * chip8_sound - CHIP8 Interpreter Sound Management
 *
 * Mike Mallin, 2021
 */

#ifndef __CHIP8_SOUND_H__
#define __CHIP8_SOUND_H__

void
chip8_sound_init(void);

void
chip8_sound_deinit(void);

void
chip8_sound_beep(void);

#endif /* __CHIP8_SOUND_H__ */
