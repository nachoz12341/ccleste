#include <nds.h>
#include <stdio.h>

#include "strm.h"

static short read_short(uint8_t *data) {
	short val = 0;
	val = *data;
	val = val + (*(data + 1) << 8);
	return val;
}

static int read_long(uint8_t *data) {
	int val = 0;
	val = *data;
	val = val + (*(data + 1) << 8);
	val = val + (*(data + 2) << 16);
	val = val + (*(data + 3) << 24);
	return val;
}

static int find_chunk(FILE *f, char *name) {
	unsigned char data[8];
	do {
		if(fread(data, 1, 8, f) < 8) {
			return -1;
		}
		for(int i=0;i<8;i++) {
			//printf("%c %02x \n",data[i],data[i]);
		}
		int chunk_len = read_long(data + 4);
		if (memcmp(data, name, 4) == 0) {
			return chunk_len;
		}
		if (fseek(f, chunk_len, SEEK_CUR) < 0) {
			break;
		}
	} while (1);

	return -1;
}

int parse_wav(FILE* f, Sound_t* sound) {
	unsigned char data[64];
	int chunk_length;

	chunk_length = find_chunk(f, "RIFF");
	if (chunk_length < 0) {
		return -1;
	}
	
	if (fread(data, 1, 4, f) < 4) {
	}

	if (memcmp(data, "WAVE", 4) != 0) {
		return -1;
	}

	chunk_length = find_chunk(f, "fmt ");
	if (chunk_length < 0) {
		return -1;
	}
	if (fread(data, 1, 16, f) < 16) {
		return -1;
	}
	short format = read_short(data);
	short channels = read_short(data + 2);
	short speed = read_long(data + 4);
	short samplebits = read_short(data + 14);

	int data_len = find_chunk(f, "data");

	switch(speed) {
	case 22050:
		sound->sampleRate = 22075;
		break;
	default:
		sound->sampleRate = speed;
		break;
	}
	sound->dataSize = data_len;
	sound->stereo = channels == 2;
	sound->bytesPerSample = samplebits / 8;
	sound->dataStart = ftell(f);

	printf("sound->sampleRate %d\n", sound->sampleRate);
	printf("sound->dataSize %d\n", sound->dataSize);
	printf("sound->stereo %d\n", sound->stereo);
	printf("sound->bytesPerSample %d\n", sound->bytesPerSample);
	printf("sound->dataStart %d\n", sound->dataStart);

	return 0;
}
