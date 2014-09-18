/* OggEnc
 **
 ** This program is distributed under the GNU General Public License, version 2.
 ** A copy of this license is included with this source.
 **
 ** Copyright 2000-2002, Michael Smith <msmith@xiph.org>
 **
 ** AIFF/AIFC support from OggSquish, (c) 1994-1996 Monty <xiphmont@xiph.org>
 **/
#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "audio.h"
#include "platform.h"
//#include "i18n.h"
#include "resample.h"

#define _(a) a
#define N_(a) a

#ifdef HAVE_LIBFLAC
#include "flac.h"
#endif

#pragma warning(disable:4995)
#pragma warning(disable:4267)
#pragma warning(disable:4244)

#define WAV_HEADER_SIZE 44

/* Macros to read header data */
#define READ_U32_LE(buf) \
	(((buf)[3]<<24)|((buf)[2]<<16)|((buf)[1]<<8)|((buf)[0]&0xff))

#define READ_U16_LE(buf) \
	(((buf)[1]<<8)|((buf)[0]&0xff))

#define READ_U32_BE(buf) \
	(((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|((buf)[3]&0xff))

#define READ_U16_BE(buf) \
	(((buf)[0]<<8)|((buf)[1]&0xff))

/* Define the supported formats here */
input_format formats[] = {
	{wav_id, 12, wav_open, wav_close, "wav", N_("WAV file reader")},
	{aiff_id, 12, aiff_open, wav_close, "aiff", N_("AIFF/AIFC file reader")},
#ifdef HAVE_LIBFLAC
	{flac_id,     4, flac_open, flac_close, "flac", N_("FLAC file reader")},
	{oggflac_id, 32, flac_open, flac_close, "ogg", N_("Ogg FLAC file reader")},
#endif
	{NULL, 0, NULL, NULL, NULL, NULL}
};

input_format *open_audio_file(FILE *in, oe_enc_opt *opt)
{
	int j=0;
	unsigned char *buf=NULL;
	int buf_size=0, buf_filled=0;
	int size,ret;

	while(formats[j].id_func)
	{
		size = formats[j].id_data_len;
		if(size >= buf_size)
		{
			buf = (unsigned char *)realloc(buf, size);
			buf_size = size;
		}

		if(size > buf_filled)
		{
			ret = fread(buf+buf_filled, 1, buf_size-buf_filled, in);
			buf_filled += ret;

			if(buf_filled < size)
			{ /* File truncated */
				j++;
				continue;
			}
		}

		if(formats[j].id_func(buf, buf_filled))
		{
			/* ok, we now have something that can handle the file */
			if(formats[j].open_func(in, opt, buf, buf_filled)) {
                free(buf);
				return &formats[j];
            }
		}
		j++;
	}

    free(buf);

	return NULL;
}

static int seek_forward(FILE *in, int length)
{
	if(fseek(in, length, SEEK_CUR))
	{
		/* Failed. Do it the hard way. */
		unsigned char buf[1024];
		int seek_needed = length, seeked;
		while(seek_needed > 0)
		{
			seeked = fread(buf, 1, seek_needed>1024?1024:seek_needed, in);
			if(!seeked)
				return 0; /* Couldn't read more, can't read file */
			else
				seek_needed -= seeked;
		}
	}
	return 1;
}


static int find_wav_chunk(FILE *in, char *type, unsigned int *len)
{
	unsigned char buf[8];

	while(1)
	{
		if(fread(buf,1,8,in) < 8) /* Suck down a chunk specifier */
		{
			fprintf(stderr, _("Warning: Unexpected EOF in reading WAV header\n"));
			return 0; /* EOF before reaching the appropriate chunk */
		}

		if(memcmp(buf, type, 4))
		{
			*len = READ_U32_LE(buf+4);
			if(!seek_forward(in, *len))
				return 0;

			buf[4] = 0;
			fprintf(stderr, _("Skipping chunk of type \"%s\", length %d\n"), buf, *len);
		}
		else
		{
			*len = READ_U32_LE(buf+4);
			return 1;
		}
	}
}

static int find_aiff_chunk(FILE *in, char *type, unsigned int *len)
{
	unsigned char buf[8];

	while(1)
	{
		if(fread(buf,1,8,in) <8)
		{
			fprintf(stderr, _("Warning: Unexpected EOF in AIFF chunk\n"));
			return 0;
		}

		*len = READ_U32_BE(buf+4);

		if(memcmp(buf,type,4))
		{
			if((*len) & 0x1)
				(*len)++;

			if(!seek_forward(in, *len))
				return 0;
		}
		else
			return 1;
	}
}



double read_IEEE80(unsigned char *buf)
{
	int s=buf[0]&0xff;
	int e=((buf[0]&0x7f)<<8)|(buf[1]&0xff);
	double f=((unsigned long)(buf[2]&0xff)<<24)|
		((buf[3]&0xff)<<16)|
		((buf[4]&0xff)<<8) |
		 (buf[5]&0xff);

	if(e==32767)
	{
		if(buf[2]&0x80)
			return HUGE_VAL; /* Really NaN, but this won't happen in reality */
		else
		{
			if(s)
				return -HUGE_VAL;
			else
				return HUGE_VAL;
		}
	}

	f=ldexp(f,32);
	f+= ((buf[6]&0xff)<<24)|
		((buf[7]&0xff)<<16)|
		((buf[8]&0xff)<<8) |
		 (buf[9]&0xff);

	return ldexp(f, e-16446);
}

/* AIFF/AIFC support adapted from the old OggSQUISH application */
int aiff_id(unsigned char *buf, int len)
{
	if(len<12) return 0; /* Truncated file, probably */

	if(memcmp(buf, "FORM", 4))
		return 0;

	if(memcmp(buf+8, "AIF",3))
		return 0;

	if(buf[11]!='C' && buf[11]!='F')
		return 0;

	return 1;
}

int aiff_open(FILE *in, oe_enc_opt *opt, unsigned char *buf, int buflen)
{
	int aifc; /* AIFC or AIFF? */
	unsigned int len;
	unsigned char *buffer;
	unsigned char buf2[8];
	aiff_fmt format;
	aifffile *aiff = (aifffile *)malloc(sizeof(aifffile));

	if(buf[11]=='C')
		aifc=1;
	else
		aifc=0;

	if(!find_aiff_chunk(in, "COMM", &len))
	{
		fprintf(stderr, _("Warning: No common chunk found in AIFF file\n"));
		return 0; /* EOF before COMM chunk */
	}

	if(len < 18) 
	{
		fprintf(stderr, _("Warning: Truncated common chunk in AIFF header\n"));
		return 0; /* Weird common chunk */
	}

	buffer = (unsigned char *)alloca(len);

	if(fread(buffer,1,len,in) < len)
	{
		fprintf(stderr, _("Warning: Unexpected EOF in reading AIFF header\n"));
		return 0;
	}

	format.channels = READ_U16_BE(buffer);
	format.totalframes = READ_U32_BE(buffer+2);
	format.samplesize = READ_U16_BE(buffer+6);
	format.rate = (int)read_IEEE80(buffer+8);

    aiff->bigendian = 1;

	if(aifc)
	{
		if(len < 22)
		{
			fprintf(stderr, _("Warning: AIFF-C header truncated.\n"));
			return 0;
		}

		if(!memcmp(buffer+18, "NONE", 4)) 
		{
			aiff->bigendian = 1;
		}
		else if(!memcmp(buffer+18, "sowt", 4)) 
		{
			aiff->bigendian = 0;
		}
		else
		{
			fprintf(stderr, _("Warning: Can't handle compressed AIFF-C (%c%c%c%c)\n"), *(buffer+18), *(buffer+19), *(buffer+20), *(buffer+21));
			return 0; /* Compressed. Can't handle */
		}
	}

	if(!find_aiff_chunk(in, "SSND", &len))
	{
		fprintf(stderr, _("Warning: No SSND chunk found in AIFF file\n"));
		return 0; /* No SSND chunk -> no actual audio */
	}

	if(len < 8) 
	{
		fprintf(stderr, _("Warning: Corrupted SSND chunk in AIFF header\n"));
		return 0; 
	}

	if(fread(buf2,1,8, in) < 8)
	{
		fprintf(stderr, _("Warning: Unexpected EOF reading AIFF header\n"));
		return 0;
	}

	format.offset = READ_U32_BE(buf2);
	format.blocksize = READ_U32_BE(buf2+4);

	if( format.blocksize == 0 &&
		(format.samplesize == 16 || format.samplesize == 8))
	{
		/* From here on, this is very similar to the wav code. Oh well. */
		
		opt->rate = format.rate;
		opt->channels = format.channels;
		opt->read_samples = wav_read; /* Similar enough, so we use the same */
		opt->total_samples_per_channel = format.totalframes;

		aiff->f = in;
		aiff->samplesread = 0;
		aiff->channels = format.channels;
		aiff->samplesize = format.samplesize;
		aiff->totalsamples = format.totalframes;

		opt->readdata = (void *)aiff;

		seek_forward(in, format.offset); /* Swallow some data */
		return 1;
	}
	else
	{
		fprintf(stderr, 
				_("Warning: OggEnc does not support this type of AIFF/AIFC file\n"
				" Must be 8 or 16 bit PCM.\n"));
		return 0;
	}
}


int wav_id(unsigned char *buf, int len)
{
	unsigned int flen;
	
	if(len<12) return 0; /* Something screwed up */

	if(memcmp(buf, "RIFF", 4))
		return 0; /* Not wave */

	flen = READ_U32_LE(buf+4); /* We don't use this */

	if(memcmp(buf+8, "WAVE",4))
		return 0; /* RIFF, but not wave */

	return 1;
}

int wav_open(FILE *in, oe_enc_opt *opt, unsigned char *oldbuf, int buflen)
{
	unsigned char buf[16];
	unsigned int len;
	int samplesize;
	wav_fmt format;
	wavfile *wav = (wavfile *)malloc(sizeof(wavfile));

	/* Ok. At this point, we know we have a WAV file. Now we have to detect
	 * whether we support the subtype, and we have to find the actual data
	 * We don't (for the wav reader) need to use the buffer we used to id this
	 * as a wav file (oldbuf)
	 */

	if(!find_wav_chunk(in, "fmt ", &len))
		return 0; /* EOF */

	if(len < 16) 
	{
		fprintf(stderr, _("Warning: Unrecognised format chunk in WAV header\n"));
		return 0; /* Weird format chunk */
	}

	/* A common error is to have a format chunk that is not 16 or 18 bytes
	 * in size.  This is incorrect, but not fatal, so we only warn about 
	 * it instead of refusing to work with the file.  Please, if you
	 * have a program that's creating format chunks of sizes other than
	 * 16 or 18 bytes in size, report a bug to the author.
	 */
	if(len!=16 && len!=18)
		fprintf(stderr, 
				_("Warning: INVALID format chunk in wav header.\n"
				" Trying to read anyway (may not work)...\n"));

	if(fread(buf,1,16,in) < 16)
	{
		fprintf(stderr, _("Warning: Unexpected EOF in reading WAV header\n"));
		return 0;
	}

	/* Deal with stupid broken apps. Don't use these programs.
	 */
	if(len - 16 > 0 && !seek_forward(in, len-16))
	    return 0;

	format.format =      READ_U16_LE(buf); 
	format.channels =    READ_U16_LE(buf+2); 
	format.samplerate =  READ_U32_LE(buf+4);
	format.bytespersec = READ_U32_LE(buf+8);
	format.align =       READ_U16_LE(buf+12);
	format.samplesize =  READ_U16_LE(buf+14);

	if(!find_wav_chunk(in, "data", &len))
		return 0; /* EOF */

	if(format.format == 1)
	{
		samplesize = format.samplesize/8;
		opt->read_samples = wav_read;
	}
	else if(format.format == 3)
	{
		samplesize = 4;
		opt->read_samples = wav_ieee_read;
	}
	else
	{
		fprintf(stderr, 
				_("ERROR: Wav file is unsupported type (must be standard PCM\n"
				" or type 3 floating point PCM\n"));
		return 0;
	}



	if( format.align == format.channels*samplesize &&
			format.samplesize == samplesize*8 && 
    		(format.samplesize == 24 || format.samplesize == 16 || 
             format.samplesize == 8 ||
	     (format.samplesize == 32 && format.format == 3)))
	{
		/* OK, good - we have the one supported format,
		   now we want to find the size of the file */
		opt->rate = format.samplerate;
		opt->channels = format.channels;

		wav->f = in;
		wav->samplesread = 0;
		wav->bigendian = 0;
		wav->channels = format.channels; /* This is in several places. The price
											of trying to abstract stuff. */
		wav->samplesize = format.samplesize;

		if(len)
        {
			opt->total_samples_per_channel = len/(format.channels*samplesize);
		}
		else
		{
			long pos;
			pos = ftell(in);
			if(fseek(in, 0, SEEK_END) == -1)
			{
				opt->total_samples_per_channel = 0; /* Give up */
			}
			else
			{
				opt->total_samples_per_channel = (ftell(in) - pos)/
                    (format.channels*samplesize);
				fseek(in,pos, SEEK_SET);
			}
		}
		wav->totalsamples = opt->total_samples_per_channel;

		opt->readdata = (void *)wav;
		return 1;
	}
	else
	{
		fprintf(stderr, 
				_("ERROR: Wav file is unsupported subformat (must be 8,16, or 24 bit PCM\n"
				"or floating point PCM\n"));
		return 0;
	}
}

long wav_read(void *in, float **buffer, int samples)
{
	wavfile *f = (wavfile *)in;
	int sampbyte = f->samplesize / 8;
	signed char *buf = (signed char*)alloca(samples*sampbyte*f->channels);
	long bytes_read = fread(buf, 1, samples*sampbyte*f->channels, f->f);
	int i,j;
	long realsamples;

	if(f->totalsamples && f->samplesread + 
			bytes_read/(sampbyte*f->channels) > f->totalsamples) {
		bytes_read = sampbyte*f->channels*(f->totalsamples - f->samplesread);
    }

	realsamples = bytes_read/(sampbyte*f->channels);
	f->samplesread += realsamples;
		
	if(f->samplesize==8)
	{
		unsigned char *bufu = (unsigned char *)buf;
		for(i = 0; i < realsamples; i++)
		{
			for(j=0; j < f->channels; j++)
			{
				buffer[j][i]=((int)(bufu[i*f->channels + j])-128)/128.0f;
			}
		}
	}
	else if(f->samplesize==16)
	{
		if(!f->bigendian)
		{
			for(i = 0; i < realsamples; i++)
			{
				for(j=0; j < f->channels; j++)
				{
					buffer[j][i] = ((buf[i*2*f->channels + 2*j + 1]<<8) |
							        (buf[i*2*f->channels + 2*j] & 0xff))/32768.0f;
				}
			}
		}
		else
		{
			for(i = 0; i < realsamples; i++)
			{
				for(j=0; j < f->channels; j++)
				{
					buffer[j][i]=((buf[i*2*f->channels + 2*j]<<8) |
							      (buf[i*2*f->channels + 2*j + 1] & 0xff))/32768.0f;
				}
			}
		}
	}
    else if(f->samplesize==24) 
    {
        if(!f->bigendian) {
            for(i = 0; i < realsamples; i++)
            {
                for(j=0; j < f->channels; j++) 
                {
                    buffer[j][i] = ((buf[i*3*f->channels + 3*j + 2] << 16) |
                      (((unsigned char *)buf)[i*3*f->channels + 3*j + 1] << 8) |
                      (((unsigned char *)buf)[i*3*f->channels + 3*j] & 0xff)) 
                        / 8388608.0f;

                }
            }
        }
        else {
            fprintf(stderr, _("Big endian 24 bit PCM data is not currently "
                              "supported, aborting.\n"));
            return 0;
        }
    }
    else {
        fprintf(stderr, _("Internal error: attempt to read unsupported "
                          "bitdepth %d\n"), f->samplesize);
        return 0;
    }

	return realsamples;
}

long wav_ieee_read(void *in, float **buffer, int samples)
{
	wavfile *f = (wavfile *)in;
	float *buf = (float*) alloca(samples*4*f->channels); /* de-interleave buffer */
	long bytes_read = fread(buf,1,samples*4*f->channels, f->f);
	int i,j;
	long realsamples;


	if(f->totalsamples && f->samplesread +
			bytes_read/(4*f->channels) > f->totalsamples)
		bytes_read = 4*f->channels*(f->totalsamples - f->samplesread);
	realsamples = bytes_read/(4*f->channels);
	f->samplesread += realsamples;

	for(i=0; i < realsamples; i++)
		for(j=0; j < f->channels; j++)
			buffer[j][i] = buf[i*f->channels + j];

	return realsamples;
}


void wav_close(void *info)
{
	wavfile *f = (wavfile *)info;

	free(f);
}

int raw_open(FILE *in, oe_enc_opt *opt)
{
	wav_fmt format; /* fake wave header ;) */
	wavfile *wav = (wavfile *)malloc(sizeof(wavfile));

	/* construct fake wav header ;) */
	format.format =      2; 
	format.channels =    opt->channels;
	format.samplerate =  opt->rate;
	format.samplesize =  opt->samplesize;
	format.bytespersec = opt->channels * opt->rate * opt->samplesize / 8;
	format.align =       format.bytespersec;
	wav->f =             in;
	wav->samplesread =   0;
	wav->bigendian =     opt->endianness;
	wav->channels =      format.channels;
	wav->samplesize =    opt->samplesize;
    wav->totalsamples =  0;

	opt->read_samples = wav_read;
	opt->readdata = (void *)wav;
	opt->total_samples_per_channel = 0; /* raw mode, don't bother */
	return 1;
}

typedef struct {
    res_state resampler;
    audio_read_func real_reader;
    void *real_readdata;
    float **bufs;
    int channels;
    int bufsize;
    int done;
} resampler;

static long read_resampled(void *d, float **buffer, int samples)
{
    resampler *rs = (resampler *)d;
    long in_samples;
    int out_samples;

    in_samples = res_push_max_input(&rs->resampler, samples);
    if(in_samples > rs->bufsize)
        in_samples = rs->bufsize;

    in_samples = rs->real_reader(rs->real_readdata, rs->bufs, in_samples);

    if(in_samples <= 0) {
        if(!rs->done) {
            rs->done = 1;
            out_samples = res_drain(&rs->resampler, buffer);
            return out_samples;
        }
        return 0;
    }

    out_samples = res_push(&rs->resampler, buffer, (float const **)rs->bufs, in_samples);

    if(out_samples <= 0) {
        fprintf(stderr, _("BUG: Got zero samples from resampler: your file will be truncated. Please report this.\n"));
    }

    return out_samples;
}

int setup_resample(oe_enc_opt *opt) {
    resampler *rs = (resampler *)calloc(1, sizeof(resampler));
    int c;

    rs->bufsize = 4096; /* Shrug */
    rs->real_reader = opt->read_samples;
    rs->real_readdata = opt->readdata;
    rs->bufs = (float**)malloc(sizeof(float *) * opt->channels);
    rs->channels = opt->channels;
    rs->done = 0;
    if(res_init(&rs->resampler, rs->channels, opt->resamplefreq, opt->rate, RES_END))
    {
        fprintf(stderr, _("Couldn't initialise resampler\n"));
        return -1;
    }

    for(c=0; c < opt->channels; c++)
        rs->bufs[c] = (float*)malloc(sizeof(float) * rs->bufsize);

    opt->read_samples = read_resampled;
    opt->readdata = rs;
    if(opt->total_samples_per_channel)
        opt->total_samples_per_channel = (int)((float)opt->total_samples_per_channel * 
            ((float)opt->resamplefreq/(float)opt->rate));
    opt->rate = opt->resamplefreq;

    return 0;
}

void clear_resample(oe_enc_opt *opt) {
    resampler *rs = (resampler *)opt->readdata;
    int i;

    opt->read_samples = rs->real_reader;
    opt->readdata = rs->real_readdata;
    res_clear(&rs->resampler);

    for(i = 0; i < rs->channels; i++)
        free(rs->bufs[i]);

    free(rs->bufs);

    free(rs);
}

typedef struct {
    audio_read_func real_reader;
    void *real_readdata;
    int channels;
    float scale_factor;
} scaler;

static long read_scaler(void *data, float **buffer, int samples) {
    scaler *d = (scaler *)data;
    long in_samples = d->real_reader(d->real_readdata, buffer, samples);
    int i,j;

    for(i=0; i < d->channels; i++) {
        for(j=0; j < in_samples; j++) {
            buffer[i][j] *= d->scale_factor;
        }
    }

    return in_samples;
}


void setup_scaler(oe_enc_opt *opt, float scale) {
    scaler *d = (scaler *)calloc(1, sizeof(scaler));

    d->real_reader = opt->read_samples;
    d->real_readdata = opt->readdata;

    opt->read_samples = read_scaler;
    opt->readdata = d;
    d->channels = opt->channels;
    d->scale_factor = scale;
}

void clear_scaler(oe_enc_opt *opt) {
    scaler *d = (scaler *)opt->readdata;

    opt->read_samples = d->real_reader;
    opt->readdata = d->real_readdata;

    free(d);
}

typedef struct {
    audio_read_func real_reader;
    void *real_readdata;
    float **bufs;
} downmix;

static long read_downmix(void *data, float **buffer, int samples)
{
    downmix *d = (downmix *)data;
    long in_samples = d->real_reader(d->real_readdata, d->bufs, samples);
    int i;

    for(i=0; i < in_samples; i++) {
        buffer[0][i] = (d->bufs[0][i] + d->bufs[1][i])*0.5f;
    }

    return in_samples;
}

void setup_downmix(oe_enc_opt *opt) {
    downmix *d = (downmix *)calloc(1, sizeof(downmix));

    if(opt->channels != 2) {
        fprintf(stderr, "Internal error! Please report this bug.\n");
        return;
    }
    
    d->bufs = (float**)malloc(2 * sizeof(float *));
    d->bufs[0] = (float*)malloc(4096 * sizeof(float));
    d->bufs[1] = (float*)malloc(4096 * sizeof(float));
    d->real_reader = opt->read_samples;

    d->real_readdata = opt->readdata;

    opt->read_samples = read_downmix;
    opt->readdata = d;

    opt->channels = 1;
}
void clear_downmix(oe_enc_opt *opt) {
    downmix *d = (downmix *)opt->readdata;

    opt->read_samples = d->real_reader;
    opt->readdata = d->real_readdata;
    opt->channels = 2; /* other things in cleanup rely on this */

    free(d->bufs[0]);
    free(d->bufs[1]);
    free(d->bufs);
    free(d);
}

