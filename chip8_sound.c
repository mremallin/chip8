/*
 * chip8_sound - CHIP8 Interpreter Sound Management
 *
 * Mike Mallin, 2021
 */

#include <stdio.h>
#include <SDL2/SDL_mixer.h>

Mix_Chunk *s_beep_sample = NULL;

void
chip8_sound_init (void)
{
	int result;

	result = Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 512);
	if (result < 0) {
		fprintf(stderr, "Failed to open audio mixer: %u\n", result);
		exit(EXIT_FAILURE);
	}

	result = Mix_AllocateChannels(4);
	if (result < 0) {
		fprintf(stderr, "Failed to allocate audio channels: %u\n", result);
		exit(EXIT_FAILURE);
	}

	s_beep_sample = Mix_LoadWAV("./beep.wav");
	if (s_beep_sample == NULL) {
		fprintf(stderr, "Failed to load ./beep.wav\n");
	}
}

void
chip8_sound_beep (void)
{
	if (s_beep_sample) {
		Mix_PlayChannel(-1, s_beep_sample, 0);
	}
}

void
chip8_sound_deinit (void)
{
	Mix_FreeChunk(s_beep_sample);
	Mix_CloseAudio();
}
