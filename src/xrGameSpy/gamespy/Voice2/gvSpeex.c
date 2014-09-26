#include "gvSpeex.h"
#include <speex.h>
#include "gvCodec.h"


static GVBool gviSpeexInitialized;
static void * gviSpeexEncoderState;
static SpeexBits gviSpeexBits;
static int gviSpeexEncodedFrameSize;
static int gviSpeexSamplesPerFrame;

//Encode/Decode buffer.
static float * gviSpeexBuffer;

GVBool gviSpeexInitialize(int quality, GVRate sampleRate)
{
	int rate;
	int bitsPerFrame;
	int samplesPerSecond;
	
	// we shouldn't already be initialized
	if(gviSpeexInitialized)
		return GVFalse;

	// create a new encoder state
	if (sampleRate == GVRate_8KHz)
		gviSpeexEncoderState = speex_encoder_init(&speex_nb_mode);
	else if (sampleRate == GVRate_16KHz)
		gviSpeexEncoderState = speex_encoder_init(&speex_wb_mode);
	else
		return GVFalse;

	if(!gviSpeexEncoderState)
		return GVFalse;

	// set the sampling rate
	samplesPerSecond = sampleRate;
	speex_encoder_ctl(gviSpeexEncoderState, SPEEX_SET_SAMPLING_RATE, &samplesPerSecond);

	// Get the samples per frame setting.
	speex_encoder_ctl(gviSpeexEncoderState, SPEEX_GET_FRAME_SIZE, &gviSpeexSamplesPerFrame);

	// set the quality
	speex_encoder_ctl(gviSpeexEncoderState, SPEEX_SET_QUALITY, &quality);

	// initialize the bits struct
	speex_bits_init(&gviSpeexBits);

	// get the bitrate
	speex_encoder_ctl(gviSpeexEncoderState, SPEEX_GET_BITRATE, &rate);

	// convert to bits per frame
	bitsPerFrame = (rate / (sampleRate / gviSpeexSamplesPerFrame));

	// convert to bytes per frame and store, round up to allocate more space than needed.
	gviSpeexEncodedFrameSize = (bitsPerFrame / 8);
	if (bitsPerFrame % 8)
		gviSpeexEncodedFrameSize++;

	// create our encoding and decoding buffer.
	gviSpeexBuffer = (float *)gsimalloc(gviSpeexSamplesPerFrame * sizeof(float));

	// we're now initialized
	gviSpeexInitialized = GVTrue;

	return GVTrue;
}

void gviSpeexCleanup(void)
{
	// make sure there is something to cleanup
	if(!gviSpeexInitialized)
		return;

	// free up encoding and decoding buffer.
	gsifree(gviSpeexBuffer);

	// destroy the encoder state
	speex_encoder_destroy(gviSpeexEncoderState);
	gviSpeexEncoderState = NULL;

	// destroy the bits struct
	speex_bits_destroy(&gviSpeexBits);

	// no longer initialized
	gviSpeexInitialized = GVFalse;
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
}

void gviSpeexFreeDecoder(GVDecoderData data)
{
	// destory the decoder state
	speex_decoder_destroy((void *)data);
}

void gviSpeexEncode(GVByte * out, const GVSample * in)
{
	int bytesWritten;
	int i;

	// convert the input to floats for encoding
	for(i = 0 ; i < gviSpeexSamplesPerFrame ; i++)
		gviSpeexBuffer[i] = in[i];

	// flush the bits
	speex_bits_reset(&gviSpeexBits);

	// encode the frame
	speex_encode(gviSpeexEncoderState, gviSpeexBuffer, &gviSpeexBits);

	// write the bits to the output
	bytesWritten = speex_bits_write(&gviSpeexBits, (char *)out, gviSpeexEncodedFrameSize);
	assert(bytesWritten == gviSpeexEncodedFrameSize);
}

void gviSpeexDecodeAdd(GVSample * out, const GVByte * in, GVDecoderData data)
{
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
}

void gviSpeexDecodeSet(GVSample * out, const GVByte * in, GVDecoderData data)
{
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
}

void gviSpeexResetEncoder(void)
{
	// reset the encoder's state
	speex_encoder_ctl(gviSpeexEncoderState, SPEEX_RESET_STATE, NULL);
}
