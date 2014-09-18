#include "gvUtil.h"
#include <math.h>
#include <limits.h>

GVScalar gviGetSamplesVolume(const GVSample * samplesPtr, int numSamples)
{
	GVSample value;
	GVSample max = 0;
	int i;

	for(i = 0 ; i < numSamples ; i++)
	{
		value = samplesPtr[i];
		if(value < 0)
			value = (GVSample)(-value);
		if(value > max)
			max = value;
	}

	return ((GVScalar)max / -SHRT_MIN);
}

GVBool gviIsOverThreshold(const GVSample * samplesPtr, int numSamples, GVScalar threshold)
{
	int i;

	if(threshold == (GVScalar)0.0)
		return GVTrue;

	for(i = 0 ; i < numSamples ; i++)
	{
		if(labs(samplesPtr[i]) > (threshold * SHRT_MAX))
			return GVTrue;
	}

	return GVFalse;
}

int gviRoundUpToNearestMultiple(int value, int base)
{
	int remainder;

	remainder = (value % base);
	if(remainder)
		value += (base - remainder);

	return value;
}

int gviRoundDownToNearestMultiple(int value, int base)
{
	value -= (value % base);

	return value;
}

int gviRoundToNearestMultiple(int value, int base)
{
	int remainder;

	remainder = (value % base);
	if(remainder < (base / 2))
		value -= remainder;
	else
		value += (base - remainder);

	return value;
}

int gviMultiplyByBytesPerMillisecond(int value)
{
	return (int)(value * GVISampleRate * GV_BYTES_PER_SAMPLE / 1000);
}

int gviDivideByBytesPerMillisecond(int value)
{
	return (int)(value * 1000 / (GVISampleRate * GV_BYTES_PER_SAMPLE));
}
