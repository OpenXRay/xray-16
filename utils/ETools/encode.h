#ifndef __ENCODE_H
#define __ENCODE_H

#include <stdio.h>
#include <vorbis/codec.h>

typedef void TIMER;
typedef long (*audio_read_func)(void *src, float **buffer, int samples);
typedef void (*progress_func)(char *fn, long totalsamples, 
		long samples, double time);
typedef void (*enc_end_func)(char *fn, double time, int rate, 
		long samples, long bytes);
typedef void (*enc_start_func)(char *fn, char *outfn, int bitrate, 
        float quality, int qset, int managed, int min_br, int max_br);
typedef void (*error_func)(char *errormessage);


void *timer_start(void);
double timer_time(void *);
void timer_clear(void *);
int create_directories(char *);

void update_statistics_full(char *fn, long total, long done, double time);
void update_statistics_notime(char *fn, long total, long done, double time);
void update_statistics_null(char *fn, long total, long done, double time);
void start_encode_full(char *fn, char *outfn, int bitrate, float quality, int qset,
        int managed, int min, int max);
void start_encode_null(char *fn, char *outfn, int bitrate, float quality, int qset,
        int managed, int min, int max);
void final_statistics(char *fn, double time, int rate, long total_samples,
		long bytes);
void final_statistics_null(char *fn, double time, int rate, long total_samples,
		long bytes);
void encode_error(char *errmsg);

typedef struct {
    char *arg;
    char *val;
} adv_opt;

typedef struct
{
	char **title;
	int title_count;
	char **artist;
	int artist_count;
	char **album;
	int album_count;
	char **comments;
	int comment_count;
	char **tracknum;
	int track_count;
	char **dates;
	int date_count;
	char **genre;
	int genre_count;
    adv_opt *advopt;
    int advopt_count;
	int copy_comments;

	int quiet;

	int rawmode;
	int raw_samplesize;
	int raw_samplerate;
	int raw_channels;
    int raw_endianness;

	char *namefmt;
    char *namefmt_remove;
    char *namefmt_replace;
	char *outfile;

	/* All 3 in kbps */
    int managed;
	int min_bitrate;
	int nominal_bitrate;
	int max_bitrate;

	/* Float from 0 to 1 (low->high) */
	float quality;
    int quality_set;

    int resamplefreq;
    int downmix;
    float scale;

	unsigned int serial;
} oe_options;

typedef struct
{
	vorbis_comment *comments;
	unsigned int serialno;

	audio_read_func read_samples;
	progress_func progress_update;
	enc_end_func end_encode;
	enc_start_func start_encode;
	error_func error;
	
	void *readdata;

	long total_samples_per_channel;
	int channels;
	long rate;
	int samplesize;
    int endianness;
    int resamplefreq;
	int copy_comments;

	/* Various bitrate/quality options */
    int managed;
	int bitrate;
	int min_bitrate;
	int max_bitrate;
	float quality;
    int quality_set;
    adv_opt *advopt;
    int advopt_count;

	FILE *out;
	char *filename;
	char *infilename;
} oe_enc_opt;


int oe_encode(oe_enc_opt *opt);

#endif /* __ENCODE_H */
