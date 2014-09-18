#include "gvLogitechPS2Codecs.h"
#include <liblgcodec.h>

#if !defined(_PS2)
#error This file should only be used with the PlayStation2
#endif

static GVBool GVILGCodecInitialized;
static int GVILGCodecHandle;
static int GVILGCodecBytesPerFrame;
static int GVILGCodecSamplesPerFrame;
static int GVILGCodecEncodedFrameSize;
static GVSample * GVILGCodecDecodeBuffer;

GVBool gviLGCodecInitialize(const char * name)
{
	lgCodecDesc * codecDesc;
	int i;
	int rcode;

	// check if the lib hasn't been initialized
	if(!GVILGCodecInitialized)
	{
		// initialize it
		rcode = lgCodecInit();
		if(LGCODEC_FAILED(rcode))
			return GVFalse;

		// we're now initialized
		GVILGCodecInitialized = GVTrue;
	}

	// find the codec
	for(i = 0 ; (codecDesc = lgCodecEnumerate(i)) ; i++)
	{
		// check if the name matches
		if(strcmp(codecDesc->name, name) == 0)
			break;
	}

	// check if we didn't find it
	if(!codecDesc)
		return GVFalse;

	// allocate memory for the decode buffer
	GVILGCodecDecodeBuffer = (GVSample *)gsimalloc((unsigned int)codecDesc->bytes_per_pcm_frame);
	if(!GVILGCodecDecodeBuffer)
		return GVFalse;

	// open a handle to the codec
	rcode = lgCodecOpen(codecDesc->id, &GVILGCodecHandle);
	if(LGCODEC_FAILED(rcode))
	{
		gsifree(GVILGCodecDecodeBuffer);
		return GVFalse;
	}

	// store the codec info
	GVILGCodecBytesPerFrame = codecDesc->bytes_per_pcm_frame;
	GVILGCodecSamplesPerFrame = (int)(GVILGCodecBytesPerFrame / GV_BYTES_PER_SAMPLE);
	GVILGCodecEncodedFrameSize = codecDesc->bytes_per_enc_frame;

	return GVTrue;
}

void gviLGCodecCleanup(void)
{
	gsifree(GVILGCodecDecodeBuffer);
	GVILGCodecDecodeBuffer = NULL;
	lgCodecClose(GVILGCodecHandle);
}

int gviLGCodecGetSamplesPerFrame(void)
{
	return GVILGCodecSamplesPerFrame;
}

int gviLGCodecGetEncodedFrameSize(void)
{
	return GVILGCodecEncodedFrameSize;
}

void gviLGCodecEncode(GVByte * out, const GVSample * in)
{
	int destSize = GVILGCodecEncodedFrameSize;
	lgCodecEncode(GVILGCodecHandle, in, GVILGCodecBytesPerFrame, out, &destSize);
	assert(destSize == GVILGCodecEncodedFrameSize);
}

void gviLGCodecDecodeAdd(GVSample * out, const GVByte * in, GVDecoderData data)
{
	int i;
	int destSize;

	destSize = GVILGCodecBytesPerFrame;
	lgCodecDecode(GVILGCodecHandle, in, GVILGCodecEncodedFrameSize, GVILGCodecDecodeBuffer, &destSize);
	assert(destSize == GVILGCodecBytesPerFrame);

	for(i = 0 ; i < GVILGCodecSamplesPerFrame ; i++)
		out[i] += GVILGCodecDecodeBuffer[i];
		
	GSI_UNUSED(data);
}

void gviLGCodecDecodeSet(GVSample * out, const GVByte * in, GVDecoderData data)
{
	int i;
	int destSize;

	destSize = GVILGCodecBytesPerFrame;
	lgCodecDecode(GVILGCodecHandle, in, GVILGCodecEncodedFrameSize, GVILGCodecDecodeBuffer, &destSize);
	assert(destSize == GVILGCodecBytesPerFrame);

	for(i = 0 ; i < GVILGCodecSamplesPerFrame ; i++)
		out[i] = GVILGCodecDecodeBuffer[i];
		
	GSI_UNUSED(data);
}
