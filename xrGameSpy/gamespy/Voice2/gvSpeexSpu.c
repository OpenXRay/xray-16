#include "gvSpeexSpu.h"
#include <speex.h>
#include "gvCodec.h"

#define GVI_SPEEX_ENCODED_BUFFER 128 //dma has trouble when smaller than 
struct SpursSpeexTaskOutput gSpeexTaskOutput;
char *gviSpeexEncoderStateBuffer;
static GVBool gviSpeexEncoderInitialized;
static int gviSpeexEncodedFrameSize;
static int gviSpeexSamplesPerFrame;
char *gviSpeexEncodedBuffer;
short *gviSpeexDecodedBuffer;
// used for decoding if SPU decoding isn't used
#if defined(GVI_NOT_USING_SPURS_DECODE_TASK)
static float * gviSpeexBuffer;
static SpeexBits gviSpeexBits;
#endif


GVBool gviSpeexInitialize(int quality, GVRate sampleRate)
{
	// we shouldn't already be initialized
	if(gviSpeexEncoderInitialized)
		return GVFalse;

	// align on a 128 byte boundary to make DMA in spurs task easier
	gviSpeexEncoderStateBuffer = (char *)gsimemalign(128,SPEEX_ENCODER_STATE_BUFFER_SIZE);
	gSpeexTaskOutput.mSpeexReturnCode = -1;

	// initialize the bits struct
#if defined(GVI_NOT_USING_SPURS_DECODE_TASK)
	speex_bits_init(&gviSpeexBits);
#endif 

	if (initializeSpursSampleTask() != 0)
		return GVFalse;
	
	// initialize the encoder given the the buffer used to keep track of state
	if (issueSampleTaskEncodeInit(quality, sampleRate, &gSpeexTaskOutput,gviSpeexEncoderStateBuffer,SPEEX_ENCODER_STATE_BUFFER_SIZE) != 0)
		return GVFalse;
	
	assert(gSpeexTaskOutput.mSpeexReturnCode == 0);
	if (gSpeexTaskOutput.mSpeexInitialized != GVTrue)
	{
		return GVFalse;
	}

	gviSpeexSamplesPerFrame = gSpeexTaskOutput.mSpeexSamplesPerFrame;
	gviSpeexEncodedFrameSize = gSpeexTaskOutput.mSpeexEncodedFrameSize;

#if defined(GVI_NOT_USING_SPURS_DECODE_TASK)
	gviSpeexBuffer = (float *)gsimalloc(gviSpeexSamplesPerFrame * sizeof(float));
#endif
	gviSpeexEncodedBuffer = (char *)gsimemalign(128, GVI_SPEEX_ENCODED_BUFFER);
	gviSpeexDecodedBuffer = (short *)gsimemalign(128, gviSpeexSamplesPerFrame*sizeof(short));
	// we're now initialized
	gviSpeexEncoderInitialized = GVTrue;
	
	return GVTrue;
}

void gviSpeexCleanup(void)
{
	// make sure there is something to cleanup
	if(!gviSpeexEncoderInitialized)
		return;
	
#ifdef GVI_NOT_USING_SPURS_DECODE_TASK
	// free up encoding and decoding buffer.
	gsifree(gviSpeexBuffer);

	// destroy the bits struct
	speex_bits_destroy(&gviSpeexBits);
#endif
		
	// destroy speex state buffer
	gsifree(gviSpeexEncoderStateBuffer);
	gsifree(gviSpeexEncodedBuffer);
	gsifree(gviSpeexDecodedBuffer);
	// cleanup spu 
	shutdownSpursTask();

	// no longer initialized
	gviSpeexEncoderInitialized = GVFalse;
}

int gviSpeexGetSamplesPerFrame(void)
{
	return gviSpeexSamplesPerFrame;
}

int gviSpeexGetEncodedFrameSize(void)
{
	return gviSpeexEncodedFrameSize;
}

GVBool gviSpeexNewDecoder(GVDecoderData * data)
{
#ifdef GVI_NOT_USING_SPURS_DECODE_TASK
	void * decoder;
	int perceptualEnhancement = 1;
	
	// create a new decoder state
	if (gviGetSampleRate() == GVRate_8KHz)
		decoder = speex_decoder_init(&speex_nb_mode);
	else if (gviGetSampleRate() == GVRate_16KHz)
		decoder = speex_decoder_init(&speex_wb_mode);
	else
		return GVFalse;

	if(!decoder)
		return GVFalse;

	// turn on the perceptual enhancement
	speex_decoder_ctl(decoder, SPEEX_SET_ENH, &perceptualEnhancement);

	*data = decoder;

	return GVTrue;

#else 
	char *decoder = (char *)gsimemalign(128, SPEEX_DECODER_STATE_BUFFER_SIZE);
	gSpeexTaskOutput.mSpeexReturnCode = -1;
	
	if (issueSampleTaskDecodeInit(decoder, SPEEX_DECODER_STATE_BUFFER_SIZE, gviGetSampleRate(), &gSpeexTaskOutput) != 0)
		return GVFalse;
	
	if (gSpeexTaskOutput.mSpeexReturnCode != 0)
		return GVFalse;

	*data = decoder;
	return GVTrue;
#endif // USE SPU ENCODING
}

void gviSpeexFreeDecoder(GVDecoderData data)
{
#ifdef GVI_NOT_USING_SPURS_DECODE_TASK
	// destroy the decoder state
	speex_decoder_destroy((void *)data);
#else
	gsifree(data);
#endif
}


void gviSpeexEncode(GVByte * out, const GVSample * in)
{	
	int immediateReturn = 0;
	gSpeexTaskOutput.mSpeexInitialized = 1;
	gSpeexTaskOutput.mSpeexReturnCode = -1;
	memset(gviSpeexEncodedBuffer, 0, GVI_SPEEX_ENCODED_BUFFER);
	immediateReturn = issueSampleTaskEncode((short *)in, gviSpeexSamplesPerFrame, gviSpeexEncodedFrameSize, (char *)gviSpeexEncodedBuffer, 
		GVI_SPEEX_ENCODED_BUFFER, &gSpeexTaskOutput,gviSpeexEncoderStateBuffer,SPEEX_ENCODER_STATE_BUFFER_SIZE);
	assert(immediateReturn == 0);
	assert(gSpeexTaskOutput.mSpeexReturnCode == 0);
	memcpy(out, gviSpeexEncodedBuffer, gviSpeexEncodedFrameSize);
	assert(gSpeexTaskOutput.mSpeexOutBufferSize == gviSpeexEncodedFrameSize);	
}

void gviSpeexDecodeAdd(GVSample * out, const GVByte * in, GVDecoderData data)
{
#ifdef GVI_NOT_USING_SPURS_DECODE_TASK
	int rcode;
	int i;

	// read the data into the bits
	speex_bits_read_from(&gviSpeexBits, (char *)in, gviSpeexEncodedFrameSize);

	// decode it
	rcode = speex_decode((void *)data, &gviSpeexBits, gviSpeexBuffer);
	assert(rcode == 0);

	// convert the output from floats
	for(i = 0 ; i < gviSpeexSamplesPerFrame ; i++)
		// Expanded to remove warnings in VS2K5
		out[i] = out[i] + (GVSample)gviSpeexBuffer[i];
#else
	int immediateReturn = 0, i;
	gSpeexTaskOutput.mSpeexInitialized = 1;
	gSpeexTaskOutput.mSpeexReturnCode = -1;
	memset(gviSpeexEncodedBuffer, 0, GVI_SPEEX_ENCODED_BUFFER);
	memcpy(gviSpeexEncodedBuffer, in, gviSpeexEncodedFrameSize);
	immediateReturn = issueSampleTaskDecodeAdd(data, SPEEX_DECODER_STATE_BUFFER_SIZE, gviSpeexEncodedBuffer, GVI_SPEEX_ENCODED_BUFFER, 
		gviSpeexEncodedFrameSize, gviSpeexDecodedBuffer, gviSpeexSamplesPerFrame, &gSpeexTaskOutput);
	for (i = 0; i < gviSpeexSamplesPerFrame; i++)
		out[i] = out[i] + (GVSample)gviSpeexDecodedBuffer[i];
	assert(immediateReturn == 0);
	assert(gSpeexTaskOutput.mSpeexReturnCode == 0);
#endif
}

void gviSpeexDecodeSet(GVSample * out, const GVByte * in, GVDecoderData data)
{
#ifdef GVI_USE_SPURS_DECODE_TASK
	int rcode;
	int i;

	// read the data into the bits
	speex_bits_read_from(&gviSpeexBits, (char *)in, gviSpeexEncodedFrameSize);

	// decode it
	rcode = speex_decode((void *)data, &gviSpeexBits, gviSpeexBuffer);

	assert(rcode == 0);

	// convert the output from floats
	for(i = 0 ; i < gviSpeexSamplesPerFrame ; i++)
		out[i] = (GVSample)gviSpeexBuffer[i];
#else
	int immediateReturn = 0;
	gSpeexTaskOutput.mSpeexInitialized = 1;
	gSpeexTaskOutput.mSpeexReturnCode = -1;
	memset(gviSpeexEncodedBuffer, 0, GVI_SPEEX_ENCODED_BUFFER);
	memcpy(gviSpeexEncodedBuffer, in, gviSpeexEncodedFrameSize);
	immediateReturn = issueSampleTaskDecodeSet(data, SPEEX_DECODER_STATE_BUFFER_SIZE, gviSpeexEncodedBuffer, GVI_SPEEX_ENCODED_BUFFER, 
		gviSpeexEncodedFrameSize, out, gviSpeexSamplesPerFrame, &gSpeexTaskOutput);
	assert(immediateReturn == 0);
	assert(gSpeexTaskOutput.mSpeexReturnCode == 0);
#endif
}

void gviSpeexResetEncoder(void)
{
	speex_encoder_ctl((void *)gviSpeexEncoderStateBuffer, SPEEX_RESET_STATE, NULL);
}
