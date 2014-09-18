#include "../gv.h"
#include "voicesample.h"
#include <stdio.h>

GVByte voice_sample[80000];

#if defined(_PS2)

#include "../gvLogitechPS2Codecs.h"

static GVBool Init(GVCodec codec)
{
	const char * name;

	// figure out the quality
	// the mapping of codec to quality is copied from gvCodec.c
	if(codec == GVCodecSuperHighQuality)
		name = "uLaw";
	else if(codec == GVCodecHighQuality)
		name = "G723.24";
	else if(codec == GVCodecAverage)
		name = "GSM";
	else if(codec == GVCodecLowBandwidth)
		name = "SPEEX";
	else if(codec == GVCodecSuperLowBandwidth)
		name = "LPC10";
	else
		return GVFalse;

	return gviLGCodecInitialize(name);
}

static void Cleanup(void)
{
	gviLGCodecCleanup();
}

static int GetSamplesPerFrame(void)
{
	return gviLGCodecGetSamplesPerFrame();
}

static GVBool NewDecoder(GVDecoderData * data)
{
	data = NULL;
	return GVTrue;
}

static void FreeDecoder(GVDecoderData data)
{
}

static void Encode(GVByte * out, const GVSample * in)
{
	gviLGCodecEncode(out, in);
}

static void Decode(GVSample * out, const GVByte * in, GVDecoderData data)
{
	gviLGCodecDecodeAdd(out, in, data);
}

#elif defined(_PSP) || defined(_WIN32)

#include "../gsm-1.0-pl12/inc/gsm.h"

gsm EncoderState;

static GVBool Init(GVCodec codec)
{
	int gsm_ltp = 1;
	EncoderState = gsm_create();
	gsm_option(EncoderState, GSM_OPT_LTP_CUT, &gsm_ltp);
	
	GSI_UNUSED(codec);
	return (EncoderState != NULL)?GVTrue:GVFalse;	
}

static void Cleanup(void)
{
	gsm_destroy(EncoderState);
}

static int GetSamplesPerFrame(void)
{
	return 160;
}

static GVBool NewDecoder(GVDecoderData * data)
{
	*data = (GVDecoderData)gsm_create();
	return (*data != NULL)?GVTrue:GVFalse;
}

static void FreeDecoder(GVDecoderData data)
{
	gsm_destroy((gsm)data);
}

static void Encode(GVByte * out, const GVSample * in)
{
	gsm_encode((gsm)EncoderState, (gsm_signal*)in, (gsm_byte*)out);
}

static void Decode(GVSample * out, const GVByte * in, GVDecoderData data)
{
	gsm_decode((gsm)data, (gsm_byte*)in, (gsm_signal*)out);
}

#else

#include "../gvSpeex.h"

static GVBool Init(GVCodec codec, GVRate sampleRate)
{
	int quality;

	// figure out the quality
	// the mapping of codec to quality is copied from gvCodec.c
	if(codec == GVCodecSuperHighQuality)
		quality = 10;
	else if(codec == GVCodecHighQuality)
		quality = 7;
	else if(codec == GVCodecAverage)
		quality = 4;
	else if(codec == GVCodecLowBandwidth)
		quality = 2;
	else if(codec == GVCodecSuperLowBandwidth)
		quality = 1;
	else
		return GVFalse;
	
	gvSetSampleRate(sampleRate);
	return gviSpeexInitialize(quality, sampleRate);
}

static void Cleanup(void)
{
	gviSpeexCleanup();
}

static int GetSamplesPerFrame(void)
{
	return gviSpeexGetSamplesPerFrame();
}

static GVBool NewDecoder(GVDecoderData * data)
{
	return gviSpeexNewDecoder(data);
}

static void FreeDecoder(GVDecoderData data)
{
	gviSpeexFreeDecoder(data);
}

static void Encode(GVByte * out, const GVSample * in)
{
	gviSpeexEncode(out, in);
}

static void Decode(GVSample * out, const GVByte * in, GVDecoderData data)
{
	gviSpeexDecodeAdd(out, in, data);
}

#endif

static void TestCodec(GVCodec codec, GVRate sampleRate, const char * name)
{
	int samplesPerFrame;
	GVSample * samples;
	int numFrames;
	GVByte encodeOut[1000];
	GVSample decodeOut[1000];
	int i;
	unsigned long startTime;
	unsigned long endTime;
	GVDecoderData decoderData;
	unsigned long sampleTime = 0;
	unsigned long totalEncodeTime = 0;
	unsigned long totalDecodeTime = 0;
	double encodeFraction;
	double decodeFraction;

	printf("Testing %s\n", name);


	if(!Init(codec, sampleRate))
	{
		printf("Failed to init\n");
		return;
	}

	sampleTime = (unsigned long)((double)sizeof(voice_sample) / ((int)(sampleRate) * GV_BYTES_PER_SAMPLE) * 1000000);

	if(!NewDecoder(&decoderData))
	{
		printf("Failed to allocate decoder data\n");
		return;
	}

	samplesPerFrame = GetSamplesPerFrame();
	numFrames = (sizeof(voice_sample) / GV_BYTES_PER_SAMPLE / samplesPerFrame);

	samples = (GVSample *)voice_sample;

	totalEncodeTime = 0;
	for(i = 0 ; i < numFrames ; i++)
	{
		startTime = current_time_hires();
		Encode(encodeOut, samples);
		endTime = current_time_hires();
		totalEncodeTime += (endTime - startTime);
		samples += samplesPerFrame;
		msleep(0);

		startTime = current_time_hires();
		Decode(decodeOut, encodeOut, decoderData);
		endTime = current_time_hires();
		totalDecodeTime += (endTime - startTime);
		msleep(0);
	}

	encodeFraction = ((double)totalEncodeTime / sampleTime);
	decodeFraction = ((double)totalDecodeTime / sampleTime);

	printf("Encode: %0.1f%%\n", encodeFraction * 100);
	printf("Decode: %0.1f%%\n", decodeFraction * 100);

	FreeDecoder(decoderData);

	Cleanup();
}

#if defined(_PS2) || defined(_PSP)
	#ifdef __MWERKS__ // CodeWarrior will warn if not prototyped
		int test_main(int argc, char **argp);
	#endif	
int test_main(int argc, char **argp)
#else
int main(int argc, char **argp)
#endif // _PS2
{
	printf("Testing codecs\n");

	TestCodec(GVCodecSuperLowBandwidth, GVRate_8KHz, "GVCodecSuperLowBandwidth");
#if !defined(_PS2)
	TestCodec(GVCodecLowBandwidth, GVRate_8KHz, "GVCodecLowBandwidth");
#endif
	TestCodec(GVCodecAverage, GVRate_8KHz, "GVCodecAverage");
	TestCodec(GVCodecHighQuality, GVRate_8KHz, "GVCodecHighQuality");
	TestCodec(GVCodecSuperHighQuality,  GVRate_8KHz, "GVCodecSuperHighQuality");

	printf("Testing complete\n");

	GSI_UNUSED(argc);
	GSI_UNUSED(argp);
	
	return 0;
}
