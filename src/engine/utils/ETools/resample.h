/* This program is licensed under the GNU Library General Public License,
 * version 2, a copy of which is included with this program (LICENCE.LGPL).
 *   
 * (c) 2002 Simon Hosie <gumboot@clear.net.nz>
 *
 *
 * A resampler
 *
 * reference:
 * 	'Digital Filters', third edition, by R. W. Hamming  ISBN 0-486-65088-X
 *
 * history:
 *	2002-05-31	ready for the world (or some small section thereof)
 *
 *
 * TOOD:
 * 	zero-crossing clipping in coefficient table
 */

#ifndef _RESAMPLE_H_INCLUDED
#define _RESAMPLE_H_INCLUDED

typedef float SAMPLE;

typedef struct
{
	unsigned int channels, infreq, outfreq, taps;
	float *table;
	SAMPLE *pool;

	/* dynamic bits */
	int poolfill;
	int offset;
} res_state;

typedef enum
{
	RES_END,
	RES_GAIN,	/* (double)1.0 */
	RES_CUTOFF,	/* (double)0.80 */ 
	RES_TAPS,	/* (int)45 */
	RES_BETA	/* (double)16.0 */
} res_parameter;

int res_init(res_state *state, int channels, int outfreq, int infreq, res_parameter op1, ...);
/*
 * Configure *state to manage a data stream with the specified parameters.  The
 * string 'params' is currently unspecified, but will configure the parameters
 * of the filter.
 *
 * This function allocates memory, and requires that res_clear() be called when
 * the buffer is no longer needed.
 *
 *
 * All counts/lengths used in the following functions consider only the data in
 * a single channel, and in numbers of samples rather than bytes, even though
 * functionality will be mirrored across as many channels as specified here.
 */


int res_push_max_input(res_state const *state, size_t maxoutput);
/*
 *  Returns the maximum number of input elements that may be provided without
 *  risk of flooding an output buffer of size maxoutput.  maxoutput is
 *  specified in counts of elements, NOT in bytes.
 */


int res_push_check(res_state const *state, size_t srclen);
/*
 * Returns the number of elements that will be returned if the given srclen
 * is used in the next call to res_push().
 */


int res_push(res_state *state, SAMPLE **dstlist, SAMPLE const **srclist, size_t srclen);
int res_push_interleaved(res_state *state, SAMPLE *dest, SAMPLE const *source, size_t srclen);
/*
 * Pushes srclen samples into the front end of the filter, and returns the
 * number of resulting samples.
 *
 * res_push(): srclist and dstlist point to lists of pointers, each of which
 * indicates the beginning of a list of samples.
 *
 * res_push_interleaved(): source and dest point to the beginning of a list of
 * interleaved samples.
 */


int res_drain(res_state *state, SAMPLE **dstlist);
int res_drain_interleaved(res_state *state, SAMPLE *dest);
/*
 * Recover the remaining elements by flushing the internal pool with 0 values,
 * and storing the resulting samples.
 *
 * After either of these functions are called, *state should only re-used in a
 * final call to res_clear().
 */


void res_clear(res_state *state);
/*
 * Free allocated buffers, etc.
 */

#endif

