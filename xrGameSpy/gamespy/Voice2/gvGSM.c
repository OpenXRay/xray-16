#include "gvGSM.h"
#include <gsm.h>

// these are standard for GSM
#define GVI_SAMPLES_PER_FRAME  160
#define GVI_ENCODED_FRAME_SIZE  33

static GVBool gviGSMInitialized;
static gsm gviGSMEncoderState;

GVBool gviGSMInitialize(void)
{
	const int gsm_ltp = 1;

	// we shouldn't already be initialized
	if(gviGSMInitialized)
		return GVFalse;

	// create a new encoder state
	gviGSMEncoderState = gsm_create();
	if(!gviGSMEncoderState)
		return GVFalse;

	// set the LTP cut option
	gsm_option(gviGSMEncoderState, GSM_OPT_LTP_CUT, &gsm_ltp);

	// we're now initialized
	gviGSMInitialized = GVTrue;

	return GVTrue;
}

void gviGSMCleanup(void)
{
	// make sure there is something to cleanup
	if(!gviGSMInitialized)
		return;

	// destroy the encoder state
	gsm_destroy(gviGSMEncoderState);
	gviGSMEncoderState = NULL;

	// no longer initialized
	gviGSMInitialized = GVFalse;
}

int gviGSMGetSamplesPerFrame(void)
{
	return GVI_SAMPLES_PER_FRAME;
}

int gviGSMGetEncodedFrameSize(void)
{
	return GVI_ENCODED_FRAME_SIZE;
}

GVBool gviGSMNewDecoder(GVDecoderData * data)
{
	gsm decoder;

	// create a new decoder state
	decoder = gsm_create();
	if(!decoder)
		return GVFalse;

	*data = decoder;
	return GVTrue;
}

void gviGSMFreeDecoder(GVDecoderData data)
{
	// destory the decoder state
	gsm_destroy((gsm)data);
}

void gviGSMEncode(GVByte * out, const GVSample * in)
{
	gsm_encode((gsm)gviGSMEncoderState, (gsm_signal*)in, (gsm_byte*)out);
}

void gviGSMDecodeAdd(GVSample * out, const GVByte * in, GVDecoderData data)
{
	GVSample temp[GVI_SAMPLES_PER_FRAME];
	int i;

	// decode it
	gsm_decode((gsm)data, (gsm_byte*)in, (gsm_signal*)temp);

	// add the output
	for(i = 0 ; i < GVI_SAMPLES_PER_FRAME ; i++)
		out[i] += temp[i];
}

void gviGSMDecodeSet(GVSample * out, const GVByte * in, GVDecoderData data)
{
	gsm_decode((gsm)data, (gsm_byte*)in, (gsm_signal*)out);
}
