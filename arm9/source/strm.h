#pragma once

#include <nds.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
	int32_t stereo;
	int32_t sampleRate;
	int32_t bytesPerSample;
	int32_t dataSize;
	int32_t dataStart;
	union {
		uint8_t *samples8;
		int16_t *samples16;
	};
} Sound_t;

typedef struct {
    Sound_t sound;
    int32_t channel;
    int32_t ticks;
    int32_t position;
    int32_t bufferSize;
	union {
        void *buffer;
		uint8_t *buffer8;
		int16_t *buffer16;
	};
	union {
		FILE* streamFile;
	};
} SoundStrm_t;

SoundStrm_t* playSoundStrm(char *name, int use_irq);
int startSoundStrm(SoundStrm_t *strm, int timer);
void processSoundStrm(SoundStrm_t *strm, int ticks);
