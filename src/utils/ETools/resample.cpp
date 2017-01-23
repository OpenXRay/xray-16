/* resample.c: see resample.h for interesting stuff */

#include "stdafx.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "resample.h"

#pragma warning(disable:4995)
#pragma warning(disable:4244)
#pragma warning(disable:4130)
#pragma warning(disable:4018)
#pragma warning(disable:4267)

/* Some systems don't define this */
#ifndef M_PI
#define M_PI       3.14159265358979323846 
#endif

static int hcf(int arg1, int arg2)
{
	int mult = 1;

	while (~(arg1 | arg2) & 1)
		arg1 >>= 1, arg2 >>= 1, mult <<= 1;

	while (arg1 > 0)
	{
		if (~(arg1 & arg2) & 1)
		{
			arg1 >>= (~arg1 & 1);
			arg2 >>= (~arg2 & 1);
		}
		else if (arg1 < arg2)
			arg2 = (arg2 - arg1) >> 1;
		else
			arg1 = (arg1 - arg2) >> 1;
	}

	return arg2 * mult;
}


static void filt_sinc(float *dest, int N, int step, double fc, double gain, int width)
{
	double s = fc / step;
	int mid, x;
	float *endpoint = dest + N,
		*base = dest,
		*origdest = dest;
	
	assert(width <= N);

	if ((N & 1) == 0)
	{
		*dest = 0.0;
		dest += width;
		if (dest >= endpoint)
			dest = ++base;
		N--;
	}

	mid = N / 2;
	x = -mid;

	while (N--)
	{
		*dest = (x ? sin(x * M_PI * s) / (x * M_PI) * step : fc) * gain;
		x++;
		dest += width;
		if (dest >= endpoint)
			dest = ++base;
	}
	assert(dest == origdest + width);
}


static double I_zero(double x)
{
	int n = 0;
	double u = 1.0,
		s = 1.0,
		t;

	do
	{
		n += 2;
		t = x / n;
		u *= t * t;
		s += u;
	} while (u > 1e-21 * s);

	return s;
}


static void win_kaiser(float *dest, int N, double alpha, int width)
{
	double I_alpha, midsq;
	int x;
	float *endpoint = dest + N,
		*base = dest,
		*origdest = dest;

	assert(width <= N);

	if ((N & 1) == 0)
	{
		*dest = 0.0;
		dest += width;
		if (dest >= endpoint)
			dest = ++base;
		N--;
	}

	x = -(N / 2);
	midsq = (double)(x - 1) * (double)(x - 1);
	I_alpha = I_zero(alpha);

	while (N--)
	{
		*dest *= I_zero(alpha * sqrt(1.0 - ((double)x * (double)x) / midsq)) / I_alpha;
		x++;
		dest += width;
		if (dest >= endpoint)
			dest = ++base;
	}
	assert(dest == origdest + width);
}


int res_init(res_state *state, int channels, int outfreq, int infreq, res_parameter op1, ...)
{
	double beta = 16.0,
		cutoff = 0.80,
		gain = 1.0;
	int taps = 45;

	int factor;

	assert(state);
	assert(channels > 0);
	assert(outfreq > 0);
	assert(infreq > 0);
	assert(taps > 0);

	if (state == NULL || channels <= 0 || outfreq <= 0 || infreq <= 0 || taps <= 0)
		return -1;

	if (op1 != RES_END)
	{
		va_list argp;
		va_start(argp, op1);
		do
		{
			switch (op1)
			{
			case RES_GAIN:
				gain = va_arg(argp, double);
				break;

			case RES_CUTOFF:
				cutoff = va_arg(argp, double);
				assert(cutoff > 0.01 && cutoff <= 1.0);
				break;

			case RES_TAPS:
				taps = va_arg(argp, int);
				assert(taps > 2 && taps < 1000);
				break;
				
			case RES_BETA:
				beta = va_arg(argp, double);
				assert(beta > 2.0);
				break;
			default:
				assert("arglist" == "valid");
				return -1;
			}
			op1 = va_arg(argp, res_parameter);
		} while (op1 != RES_END);
		va_end(argp);
	}

	factor = hcf(infreq, outfreq);
	outfreq /= factor;
	infreq /= factor;

	/* adjust to rational values for downsampling */
	if (outfreq < infreq)
	{
		/* push the cutoff frequency down to the output frequency */
		cutoff = cutoff * outfreq / infreq; 

        /* compensate for the sharper roll-off requirement
         * by using a bigger hammer */
        taps = taps * infreq/outfreq;
	}

	assert(taps >= (infreq + outfreq - 1) / outfreq);

	if ((state->table = (float*)calloc(outfreq * taps, sizeof(float))) == NULL)
		return -1;
	if ((state->pool = (SAMPLE*)calloc(channels * taps, sizeof(SAMPLE))) == NULL)
	{
		free(state->table);
		state->table = NULL;
		return -1;
	}

	state->poolfill = taps / 2 + 1;
	state->channels = channels;
	state->outfreq = outfreq;
	state->infreq = infreq;
	state->taps = taps;
	state->offset = 0;

	filt_sinc(state->table, outfreq * taps, outfreq, cutoff, gain, taps);
	win_kaiser(state->table, outfreq * taps, beta, taps);

	return 0;
}


static SAMPLE sum(float const *scale, int count, SAMPLE const *source, SAMPLE const *trigger, SAMPLE const *reset, int srcstep)
{
	float total = 0.0;

	while (count--)
	{
		total += *source * *scale;

		if (source == trigger)
			source = reset, srcstep = 1;
		source -= srcstep;
		scale++;
	}

	return total;
}


static int push(res_state const * const state, SAMPLE *pool, int * const poolfill, int * const offset, SAMPLE *dest, int dststep, SAMPLE const *source, int srcstep, size_t srclen)
{
	SAMPLE	* const destbase = dest,
		*poolhead = pool + *poolfill,
		*poolend = pool + state->taps,
		*newpool = pool;
	SAMPLE const *refill, *base, *endpoint;
	int	lencheck;


	assert(state);
	assert(pool);
	assert(poolfill);
	assert(dest);
	assert(source);

	assert(state->poolfill != -1);
	
	lencheck = res_push_check(state, srclen);

	/* fill the pool before diving in */
	while (poolhead < poolend && srclen > 0)
	{
		*poolhead++ = *source;
		source += srcstep;
		srclen--;
	}

	if (srclen <= 0)
		return 0;

	base = source;
	endpoint = source + srclen * srcstep;

	while (source < endpoint)
	{
		*dest = sum(state->table + *offset * state->taps, state->taps, source, base, poolend, srcstep);
		dest += dststep;
		*offset += state->infreq;
		while (*offset >= state->outfreq)
		{
			*offset -= state->outfreq;
			source += srcstep;
		}
	}

	assert(dest == destbase + lencheck * dststep);

	/* pretend that source has that underrun data we're not going to get */
	srclen += (source - endpoint) / srcstep;

	/* if we didn't get enough to completely replace the pool, then shift things about a bit */
	if (srclen < state->taps)
	{
		refill = pool + srclen;
		while (refill < poolend)
			*newpool++ = *refill++;

		refill = source - srclen * srcstep;
	}
	else
		refill = source - state->taps * srcstep;

	/* pull in fresh pool data */
	while (refill < endpoint)
	{
		*newpool++ = *refill;
		refill += srcstep;
	}

	assert(newpool > pool);
	assert(newpool <= poolend);

	*poolfill = newpool - pool;

	return (dest - destbase) / dststep;
}


int res_push_max_input(res_state const * const state, size_t maxoutput)
{
	return maxoutput * state->infreq / state->outfreq;
}


int res_push_check(res_state const * const state, size_t srclen)
{
	if (state->poolfill < state->taps)
		srclen -= state->taps - state->poolfill;

	return (srclen * state->outfreq - state->offset + state->infreq - 1) / state->infreq;
}


int res_push(res_state *state, SAMPLE **dstlist, SAMPLE const **srclist, size_t srclen)
{
	int result = -1, poolfill = -1, offset = -1, i;

	assert(state);
	assert(dstlist);
	assert(srclist);
	assert(state->poolfill >= 0);

	for (i = 0; i < state->channels; i++)
	{
		poolfill = state->poolfill;
		offset = state->offset;
		result = push(state, state->pool + i * state->taps, &poolfill, &offset, dstlist[i], 1, srclist[i], 1, srclen);
	}
	state->poolfill = poolfill;
	state->offset = offset;

	return result;
}


int res_push_interleaved(res_state *state, SAMPLE *dest, SAMPLE const *source, size_t srclen)
{
	int result = -1, poolfill = -1, offset = -1, i;
	
	assert(state);
	assert(dest);
	assert(source);
	assert(state->poolfill >= 0);

	for (i = 0; i < state->channels; i++)
	{
		poolfill = state->poolfill;
		offset = state->offset;
		result = push(state, state->pool + i * state->taps, &poolfill, &offset, dest + i, state->channels, source + i, state->channels, srclen);
	}
	state->poolfill = poolfill;
	state->offset = offset;

	return result;
}


int res_drain(res_state *state, SAMPLE **dstlist)
{
	SAMPLE *tail;
	int result = -1, poolfill = -1, offset = -1, i;

	assert(state);
	assert(dstlist);
	assert(state->poolfill >= 0);

	if ((tail = (SAMPLE*)calloc(state->taps, sizeof(SAMPLE))) == NULL)
		return -1;

	for (i = 0; i < state->channels; i++)
	{
		poolfill = state->poolfill;
		offset = state->offset;
		result = push(state, state->pool + i * state->taps, &poolfill, &offset, dstlist[i], 1, tail, 1, state->taps / 2 - 1);
	}
		
	free(tail);

	state->poolfill = -1;

	return result;
}


int res_drain_interleaved(res_state *state, SAMPLE *dest)
{
	SAMPLE *tail;
	int result = -1, poolfill = -1, offset = -1, i;

	assert(state);
	assert(dest);
	assert(state->poolfill >= 0);

	if ((tail = (SAMPLE*)calloc(state->taps, sizeof(SAMPLE))) == NULL)
		return -1;

	for (i = 0; i < state->channels; i++)
	{
		poolfill = state->poolfill;
		offset = state->offset;
		result = push(state, state->pool + i * state->taps, &poolfill, &offset, dest + i, state->channels, tail, 1, state->taps / 2 - 1);
	}
		
	free(tail);

	state->poolfill = -1;

	return result;
}


void res_clear(res_state *state)
{
	assert(state);
	assert(state->table);
	assert(state->pool);

	free(state->table);
	free(state->pool);
	memset(state, 0, sizeof(*state));
}

