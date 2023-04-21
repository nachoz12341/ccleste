#include <stdio.h>
#include <malloc.h>
#include "strm.h"
#include "wave.h"

static SoundStrm_t *currentStrm = 0;

static void sample_fix(int bytesPerSample, unsigned char *data, int len) {
	//printf("read %p %d\n", data, len);
	DC_FlushRange(data,len);
	if(bytesPerSample  != 1) {
		return;
	}
	for (int i = 0; i < len; ++i) {
		data[i] = data[i] - 128;
	}
}

void MusicTicks(SoundStrm_t *strm, int ticks) {
    if(strm == 0) {
        return;
    }
	int nextTick = strm->ticks + ticks;
	int currentPos = strm->position;
	int bufferSize = strm->bufferSize;
	int endPos = (bufferSize * nextTick)/32;
	int totalBytes = endPos - currentPos;
	int pos = currentPos % bufferSize; 
	//printf("tick: %2d new: %4d tot: %4d\n", nextTick, endPos, totalBytes);
	do {
		int readBytes = totalBytes;
		if(readBytes + pos > bufferSize) {
			readBytes = bufferSize - pos;
		}
		//printf("tick: %2d read: %4d pos: %4d\n", nextTick, readBytes, currentPos);
		int cb = fread(&strm->buffer8[pos], 1, readBytes, strm->streamFile);
		if(cb < readBytes) {
			fseek(strm->streamFile, strm->sound.dataStart, SEEK_SET);
			if(cb <= 0) {
				cb = fread(&strm->buffer8[pos], 1, readBytes, strm->streamFile);
			}
			readBytes = cb;
			if(cb < 0) {
				return;
			}
		}
		sample_fix(strm->sound.bytesPerSample, &strm->buffer8[pos], readBytes);
		totalBytes -= readBytes;
		pos += readBytes;
		if(pos >= bufferSize) {
			pos = 0;
		}
	} while(totalBytes > 0);

	if(endPos > bufferSize) {
		nextTick -= 32;
		endPos -= bufferSize;
	}
	
	strm->position = endPos;
	strm->ticks = nextTick;
}

void MusicTimerCallback() {
    MusicTicks(currentStrm, 1);
}

SoundStrm_t* playSoundStrm(char *name, int use_irq) {
    FILE *f =  fopen(name, "rb");
    if(f == 0) {
        printf("fopen failure\n");
        return 0;
    }
    SoundStrm_t *strm = (SoundStrm_t*)malloc(sizeof(*strm));
    if(strm == 0) {
        fclose(f);
        printf("sound stream allocation failure\n");
        return 0;
    }
    if(parse_wav(f, &strm->sound) < 0) {
        fclose(f);
        free(strm);
        printf("parse failure\n");
        return 0;
    }
    int frequency = strm->sound.sampleRate;
    int format = strm->sound.bytesPerSample == 2 ? SoundFormat_16Bit : SoundFormat_8Bit;
    int bufferSize = strm->sound.sampleRate * strm->sound.bytesPerSample;
    void *buffer = malloc(bufferSize);
    if(buffer == 0) {
        fclose(f);
        free(strm);
        free(buffer);
        printf("buffer allocation failure\n");
        return 0;
    }
    memset(buffer, 0, bufferSize);

    strm->ticks = 0;
    strm->position = 0;
    strm->buffer = buffer;
    strm->bufferSize = bufferSize;
    strm->streamFile = f;

    MusicTicks(strm, 10);


    if(use_irq) {
        int channel = soundPlaySample(buffer, format, bufferSize, frequency, 127, 64, true, 0);
        if(channel < 0) {
            fclose(f);
            free(strm);
            free(buffer);
            printf("Sound play failure\n");
            return 0;
        }
        strm->channel = channel;
        currentStrm = strm;
        timerStart(0, ClockDivider_64, TIMER_FREQ_64(32), MusicTimerCallback);
    }

    return strm;
}

int startSoundStrm(SoundStrm_t *strm, int timer) {
    if(strm == 0) {
        printf("startSoundStrm: invalid stream\n");
        return -1;
    }
    int frequency = strm->sound.sampleRate;
    int format = strm->sound.bytesPerSample == 2 ? SoundFormat_16Bit : SoundFormat_8Bit;
    int bufferSize = strm->bufferSize;
    int channel = soundPlaySample(strm->buffer, format, bufferSize, frequency, 127, 64, true, 0);
    if(channel < 0) {
        printf("startSoundStrm: play sample error\n");
        return -1;
    }
    strm->channel = channel;

    return 0;
}

void processSoundStrm(SoundStrm_t *strm, int ticks) {
    if(ticks > 0) {
        MusicTicks(strm, ticks);
    }
}
