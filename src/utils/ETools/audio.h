
#ifndef __AUDIO_H
#define __AUDIO_H

#include "encode.h"
#include <stdio.h>

int setup_resample(oe_enc_opt *opt);
void clear_resample(oe_enc_opt *opt);
void setup_downmix(oe_enc_opt *opt);
void clear_downmix(oe_enc_opt *opt);
void setup_scaler(oe_enc_opt *opt, float scale);
void clear_scaler(oe_enc_opt *opt);

typedef struct
{
	int (*id_func)(unsigned char *buf, int len); /* Returns true if can load file */
	int id_data_len; /* Amount of data needed to id whether this can load the file */
	int (*open_func)(FILE *in, oe_enc_opt *opt, unsigned char *buf, int buflen);
	void (*close_func)(void *);
	char *format;
	char *description;
} input_format;


typedef struct {
	short format;
	short channels;
	int samplerate;
	int bytespersec;
	short align;
	short samplesize;
} wav_fmt;

typedef struct {
	short channels;
	short samplesize;
	long totalsamples;
	long samplesread;
	FILE *f;
	short bigendian;
} wavfile;

typedef struct {
	short channels;
	int totalframes;
	short samplesize;
	int rate;
	int offset;
	int blocksize;
} aiff_fmt;

typedef wavfile aifffile; /* They're the same */

input_format *open_audio_file(FILE *in, oe_enc_opt *opt);

int raw_open(FILE *in, oe_enc_opt *opt);
int wav_open(FILE *in, oe_enc_opt *opt, unsigned char *buf, int buflen);
int aiff_open(FILE *in, oe_enc_opt *opt, unsigned char *buf, int buflen);
int wav_id(unsigned char *buf, int len);
int aiff_id(unsigned char *buf, int len);
void wav_close(void *);
void raw_close(void *);

long wav_read(void *, float **buffer, int samples);
long wav_ieee_read(void *, float **buffer, int samples);
long raw_read_stereo(void *, float **buffer, int samples);

#endif /* __AUDIO_H */

