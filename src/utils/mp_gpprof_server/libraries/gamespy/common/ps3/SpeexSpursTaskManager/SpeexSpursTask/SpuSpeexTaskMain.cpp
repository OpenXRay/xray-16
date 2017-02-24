// Copyright 2007 GameSpy Industries, Inc
// 
// Speex SPURS Task 
// Encode supported
// Decode supported
//

#define __STDC_CONSTANT_MACROS

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/spu_thread.h>
#include <sys/spu_event.h>
#include <spu_printf.h>

#include <spu_mfcio.h>
#include <cell/dma.h>
#include <cell/atomic.h>
#include "SPUAssert.h"

#include "spursSupportInterface.h"
#include "SpursSpeexTaskManager.h"
#include "SpuFakeDma.h"
#include "SpursSpeexCInterface.h"

#include <speex.h>

#include <sys/return_code.h>
#include "LibSN_SPU.h"

#define GVRate_8KHz   8000
#define GVRate_16KHz 16000

// use this for data read in from each task request
SpursSpeexTaskDesc	gviSpursSpeexTaskDesc;

// internal data used by speex functions
SpeexBits gviSpursSpeexBits;

char	gviSpursSpeexStateBuffer[SPEEX_ENCODER_STATE_BUFFER_SIZE] POST_ALIGN(128);
char	gviSpursSpeexBitsBuffer[MAX_BYTES_PER_FRAME] POST_ALIGN(16);

void spuDebugPrintf(const char *fmt, ...)
{
	#ifdef SPU_VERBOSE_DEBUGGING
	char debugOut[256];
	va_list args;
	va_start(args, fmt);
	vsprintf(debugOut, fmt, args);
	va_end(args);
	spu_printf(debugOut);
	#endif
}

void gviSpursSpeexEncoderInitialize(SpursSpeexTaskOutput *spuOutput)
{
	void *gviSpeexEncoderState;
	int sampleRate = gviSpursSpeexTaskDesc.mSamplesPerSecond;
	int quality = gviSpursSpeexTaskDesc.mQuality;
	int rate;
	int bitsPerFrame;

	//spuDebugPrintf("[Speex][SPU] sample rate: %d, quality: %d\n", sampleRate, quality);

	// create a new encoder state
	if (sampleRate == GVRate_8KHz)
		gviSpeexEncoderState = speex_encoder_init(&speex_nb_mode, gviSpursSpeexStateBuffer);
	else if (sampleRate == GVRate_16KHz)
		gviSpeexEncoderState = speex_encoder_init(&speex_wb_mode, gviSpursSpeexStateBuffer);
	else
	{
		//spuDebugPrintf("[Speex][SPU] Initializing Speex failed\n");
		spuOutput->mSpeexReturnCode = -2;
		return;
	}

	if(!gviSpeexEncoderState)
	{
		//spuDebugPrintf("[Speex][SPU] Initializing Speex failed\n");
		spuOutput->mSpeexReturnCode = -3;
		return;
	}

	//spuDebugPrintf("[Speex][SPU] Done getting speex mode\n");
	//spuDebugPrintf("[Speex][SPU] encoder state addr: 0x%8x\n", gviSpeexEncoderState);
	
	// set the sampling rate
	speex_encoder_ctl(gviSpeexEncoderState, SPEEX_SET_SAMPLING_RATE, &sampleRate);

	// Get the samples per frame setting.
	speex_encoder_ctl(gviSpeexEncoderState, SPEEX_GET_FRAME_SIZE, &spuOutput->mSpeexSamplesPerFrame);

	// set the quality
	speex_encoder_ctl(gviSpeexEncoderState, SPEEX_SET_QUALITY, &quality);

	// (re)initialize the bits struct
	speex_bits_init_buffer(&gviSpursSpeexBits,gviSpursSpeexBitsBuffer,sizeof(gviSpursSpeexBitsBuffer));

	//spuDebugPrintf("[Speex][SPU] speex bits addr: 0x%8x\n", gviSpeexBits);

	// get the bit rate
	speex_encoder_ctl(gviSpeexEncoderState, SPEEX_GET_BITRATE, &rate);

	//spuDebugPrintf("[Speex][SPU] Done with customizing options for speex\n");

	// convert to bits per frame
	bitsPerFrame = (rate / (sampleRate / spuOutput->mSpeexSamplesPerFrame));

	// convert to bytes per frame and store, round up to allocate more space than needed.
	spuOutput->mSpeexEncodedFrameSize = (bitsPerFrame / 8);
	if (bitsPerFrame % 8)
		spuOutput->mSpeexEncodedFrameSize++;

	// we're now initialized
	spuOutput->mSpeexInitialized = 1;

	//spuDebugPrintf("[Speex][SPU] Done with initing speex\n");

	spuOutput->mSpeexReturnCode = 0;
}

void gviSpursSpeexDecoderInitialize(SpursSpeexTaskOutput *spuTaskOut)
{
	void * decoder = gviSpursSpeexStateBuffer;
	int perceptualEnhancement = 1;

	// create a new decoder state
	if (gviSpursSpeexTaskDesc.mSamplesPerSecond == GVRate_8KHz)
		speex_decoder_init(&speex_nb_mode, decoder);
	else if (gviSpursSpeexTaskDesc.mSamplesPerSecond == GVRate_16KHz)
		speex_decoder_init(&speex_wb_mode, decoder);
	else
	{
		//spuDebugPrintf("[Speex][SPU] Error: invalid sample rate!\n");
		spuTaskOut->mSpeexReturnCode = -1;
	}

	if(!decoder)
	{
		//spuDebugPrintf("[Speex][SPU] Error: initializing decoder failed!\n");
		spuTaskOut->mSpeexReturnCode = -2;
	}

	// turn on the perceptual enhancement
	speex_decoder_ctl(decoder, SPEEX_SET_ENH, &perceptualEnhancement);

	spuTaskOut->mSpeexReturnCode = 0;
}

void gviSpursSpeexEncode(SpursSpeexTaskOutput *spuTaskOut)
{	
	short *inBuffer;
	float *speexBuffer;
	char *outBuffer;
	unsigned int i;
	spuTaskOut->mSpeexEncodedFrameSize = 0;
	spuTaskOut->mSpeexInitialized = 1;
	spuTaskOut->mSpeexSamplesPerFrame = 0;
	spuTaskOut->mSpeexReturnCode = 0;
	spuTaskOut->mSpeexOutBufferSize = 0;

	speexBuffer = (float *)memalign(16, gviSpursSpeexTaskDesc.mInputBufferSize * sizeof(float));
	inBuffer = (short *)memalign(16, gviSpursSpeexTaskDesc.mInputBufferSize * sizeof(short));
	outBuffer = (char *)memalign(16, gviSpursSpeexTaskDesc.mOutputBufferSize);
	
	memset(speexBuffer, 0, gviSpursSpeexTaskDesc.mInputBufferSize * sizeof(float));
	memset(inBuffer, 0, gviSpursSpeexTaskDesc.mInputBufferSize * sizeof(short));
	memset(outBuffer, 0, gviSpursSpeexTaskDesc.mOutputBufferSize);

	cellDmaGet(inBuffer, (uint64_t)gviSpursSpeexTaskDesc.mInputBuffer, gviSpursSpeexTaskDesc.mInputBufferSize * sizeof(short), DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));
		
	// convert the input to floats for encoding
	for(i = 0 ; i < gviSpursSpeexTaskDesc.mInputBufferSize ; i++)
		speexBuffer[i] = inBuffer[i];

	// (re)initialize the bits struct
	speex_bits_init_buffer(&gviSpursSpeexBits,gviSpursSpeexBitsBuffer,sizeof(gviSpursSpeexBitsBuffer));

	// flush the bits
	speex_bits_reset(&gviSpursSpeexBits);

	// encode the frame
	speex_encode(gviSpursSpeexStateBuffer, speexBuffer, &gviSpursSpeexBits);
	// write the bits to the output
	spuTaskOut->mSpeexOutBufferSize = speex_bits_write(&gviSpursSpeexBits, (char *)outBuffer, gviSpursSpeexTaskDesc.mEncodedFrameSize);
	//spuDebugPrintf("[Speex][SPU] transferring data back, output size should be: %d\n", gviSpursSpeexTaskDesc.mOutputBufferSize>16?gviSpursSpeexTaskDesc.mOutputBufferSize:16);
	cellDmaPut(outBuffer, (uint64_t)gviSpursSpeexTaskDesc.mOutputBuffer, gviSpursSpeexTaskDesc.mOutputBufferSize, DMA_TAG(1), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));
	//spuDebugPrintf("[Speex][SPU] done transferring data back\n");
	free(speexBuffer);
	free(inBuffer);
	free(outBuffer);
	spuTaskOut->mSpeexReturnCode = 0;
}

void gviSpursSpeexDecodeAdd(SpursSpeexTaskOutput *spuTaskOut)
{
	char *inBuffer;
	float *speexBuffer;
	short *outBuffer;
	int rcode;
	unsigned int i;
	
	//spuDebugPrintf("[Speex][SPU] allocating buffers for decoding\n");
	speexBuffer = (float *)memalign(16, gviSpursSpeexTaskDesc.mOutputBufferSize * sizeof(float));
	outBuffer = (short *)memalign(16, gviSpursSpeexTaskDesc.mOutputBufferSize * sizeof(short));
	inBuffer = (char *)memalign(16, gviSpursSpeexTaskDesc.mInputBufferSize);

	memset(speexBuffer, 0, gviSpursSpeexTaskDesc.mOutputBufferSize * sizeof(float));
	memset(outBuffer, 0, gviSpursSpeexTaskDesc.mOutputBufferSize);
	memset(inBuffer, 0, gviSpursSpeexTaskDesc.mInputBufferSize * sizeof(short));
	
	
	//spuDebugPrintf("[Speex][SPU] done allocating, getting input data, inbuffer size: %d\n", gSpuSampleTaskDesc.mInputBufferSize);
	cellDmaGet(inBuffer, (uint64_t)gviSpursSpeexTaskDesc.mInputBuffer, gviSpursSpeexTaskDesc.mInputBufferSize, DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));
	// spuDebugPrintf("[Speex][SPU] done getting input data, preparing for speex to decode\n");
	// read the data into the bits
	// (re)initialize the bits struct
	speex_bits_init_buffer(&gviSpursSpeexBits,gviSpursSpeexBitsBuffer,sizeof(gviSpursSpeexBitsBuffer));

	speex_bits_read_from(&gviSpursSpeexBits, (char *)inBuffer, gviSpursSpeexTaskDesc.mEncodedFrameSize);

	// decode it
	rcode = speex_decode((void *)gviSpursSpeexStateBuffer, &gviSpursSpeexBits, speexBuffer);
	assert(rcode == 0);
	//spuDebugPrintf("[Speex][SPU] done with speex decode\n");
	// convert the output from floats
	for(i = 0 ; i < gviSpursSpeexTaskDesc.mOutputBufferSize ; i++)
		outBuffer[i] = (short)speexBuffer[i];
	
	//spuDebugPrintf("[Speex][SPU] transferring data back\n");
	cellDmaPut(outBuffer, (uint64_t)gviSpursSpeexTaskDesc.mOutputBuffer, gviSpursSpeexTaskDesc.mOutputBufferSize * sizeof(short), DMA_TAG(1), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));
	//spuDebugPrintf("[Speex][SPU] done transferring data back\n");
	free(speexBuffer);
	free(inBuffer);
	free(outBuffer);
	spuTaskOut->mSpeexReturnCode = 0;
}

void gviSpursSpeexDecodeSet(SpursSpeexTaskOutput *spuTaskOut)
{
	char *inBuffer;
	float *speexBuffer;
	short *outBuffer;
	int rcode;
	unsigned int i;

	speexBuffer = (float *)memalign(16, gviSpursSpeexTaskDesc.mOutputBufferSize * sizeof(float));
	outBuffer = (short *)memalign(16, gviSpursSpeexTaskDesc.mOutputBufferSize * sizeof(short));
	inBuffer = (char *)memalign(16, gviSpursSpeexTaskDesc.mInputBufferSize);

	memset(speexBuffer, 0, gviSpursSpeexTaskDesc.mOutputBufferSize * sizeof(float));
	memset(inBuffer, 0, gviSpursSpeexTaskDesc.mOutputBufferSize * sizeof(short));
	memset(outBuffer, 0, gviSpursSpeexTaskDesc.mInputBufferSize);

	cellDmaGet(inBuffer, (uint64_t)gviSpursSpeexTaskDesc.mInputBuffer, gviSpursSpeexTaskDesc.mInputBufferSize, DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	// read the data into the bits
	speex_bits_read_from(&gviSpursSpeexBits, (char *)inBuffer, gviSpursSpeexTaskDesc.mEncodedFrameSize);

	// decode it
	rcode = speex_decode((void *)gviSpursSpeexStateBuffer, &gviSpursSpeexBits, speexBuffer);
	assert(rcode == 0);

	// convert the output from floats
	for(i = 0 ; i < gviSpursSpeexTaskDesc.mOutputBufferSize ; i++)
		// Expanded to remove warnings in VS2K5
		outBuffer[i] = (short)speexBuffer[i];

	cellDmaPut(outBuffer, (uint64_t)gviSpursSpeexTaskDesc.mOutputBuffer, gviSpursSpeexTaskDesc.mOutputBufferSize * sizeof(short), DMA_TAG(1), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));
	free(speexBuffer);
	free(inBuffer);
	free(outBuffer);
	spuTaskOut->mSpeexReturnCode = 0;
}

void procesEncodeInit(unsigned int uiPtr)
{
	SpursSpeexTaskOutput spuOutput;

	//spuDebugPrintf("[Speex][SPU] CMD_SAMPLE_TASK_ENCODE_INIT_COMMAND\n");
	cellDmaGet(&gviSpursSpeexTaskDesc, uiPtr, sizeof(SpursSpeexTaskDesc), DMA_TAG(1), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));
	
	if (gviSpursSpeexTaskDesc.mDebugPause)
	{
		snPause();
	}

	gviSpursSpeexEncoderInitialize(&spuOutput);
	if (spuOutput.mSpeexReturnCode < 0)
	{
		spuDebugPrintf("[Speex][SPU] failed to initialize encoder, ret = %d\n", spuOutput.mSpeexReturnCode);
	}

	//spuDebugPrintf("[Speex][SPU] done with initializing things for speex, now returning data via DMA put\n");

	//printGlobalTaskDescData();

	cellDmaPut(&spuOutput,	(uint64_t)gviSpursSpeexTaskDesc.mSpeexTaskOutput, sizeof(SpursSpeexTaskOutput), DMA_TAG(1),
		0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	//spuDebugPrintf("[Speex][SPU] task dma done\n");

	cellDmaLargePut(gviSpursSpeexStateBuffer, (uint64_t)gviSpursSpeexTaskDesc.mSpeexStateBuffer, SPEEX_ENCODER_STATE_BUFFER_SIZE, DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	//spuDebugPrintf("[Speex][SPU] buffer dma done\n");
}

void processDecodeInit(unsigned int uiPtr)
{
	SpursSpeexTaskOutput spuOutput;
	cellDmaGet(&gviSpursSpeexTaskDesc, uiPtr, sizeof(SpursSpeexTaskDesc), DMA_TAG(1), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	//spuDebugPrintf("[Speex][SPU] CMD_SAMPLE_TASK_DECODE_INIT_COMMAND\n");
	
	if (gviSpursSpeexTaskDesc.mDebugPause)
	{
		snPause();
	}

	gviSpursSpeexDecoderInitialize(&spuOutput);

	if (spuOutput.mSpeexReturnCode < 0)
	{
		spuDebugPrintf("[Speex][SPU] failed to initialize decoder, ret = %d\n", spuOutput.mSpeexReturnCode);
	}

	cellDmaPut(&spuOutput,	(uint64_t)gviSpursSpeexTaskDesc.mSpeexTaskOutput, sizeof(SpursSpeexTaskOutput), DMA_TAG(1),
		0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	cellDmaLargePut(gviSpursSpeexStateBuffer, (uint64_t)gviSpursSpeexTaskDesc.mSpeexStateBuffer, 
		gviSpursSpeexTaskDesc.mSpeexStateBufferSize, DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	//spuDebugPrintf("[Speex][SPU] buffer dma done\n");
}

void processEncode(unsigned int uiPtr)
{
	SpursSpeexTaskOutput spuOutput;
	cellDmaGet(&gviSpursSpeexTaskDesc, uiPtr, sizeof(SpursSpeexTaskDesc), DMA_TAG(1), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	spuDebugPrintf("[Speex][SPU] CMD_SAMPLE_TASK_ENCODE_COMMAND\n");

	if (gviSpursSpeexTaskDesc.mDebugPause)
	{
		snPause();
	}
	cellDmaLargeGet(gviSpursSpeexStateBuffer, (uint64_t)gviSpursSpeexTaskDesc.mSpeexStateBuffer, SPEEX_ENCODER_STATE_BUFFER_SIZE, DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));
	
	gviSpursSpeexEncode(&spuOutput);

	if (spuOutput.mSpeexReturnCode < 0)
	{
		spuDebugPrintf("SPU: failed to encode, ret = %d\n", spuOutput.mSpeexReturnCode);
	}

	cellDmaPut(&spuOutput,	(uint64_t)gviSpursSpeexTaskDesc.mSpeexTaskOutput, sizeof(SpursSpeexTaskOutput), DMA_TAG(1),
		0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	cellDmaLargePut(gviSpursSpeexStateBuffer, (uint64_t)gviSpursSpeexTaskDesc.mSpeexStateBuffer, SPEEX_ENCODER_STATE_BUFFER_SIZE, DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	spuDebugPrintf("[Speex][SPU] buffer dma done\n");
}


void processDecodeAdd(unsigned int uiPtr)
{
	SpursSpeexTaskOutput spuOutput;
	cellDmaGet(&gviSpursSpeexTaskDesc, uiPtr, sizeof(SpursSpeexTaskDesc), DMA_TAG(1), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	//spuDebugPrintf("[Speex][SPU] CMD_SAMPLE_TASK_DECODEADD_COMMAND\n");

	if (gviSpursSpeexTaskDesc.mDebugPause)
	{
		snPause();
	}
	
	cellDmaLargeGet(gviSpursSpeexStateBuffer, (uint64_t)gviSpursSpeexTaskDesc.mSpeexStateBuffer, gviSpursSpeexTaskDesc.mSpeexStateBufferSize, DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));
	
	gviSpursSpeexDecodeAdd(&spuOutput);

	if (spuOutput.mSpeexReturnCode < 0)
	{
		spuDebugPrintf("SPU: failed to decode, ret = %d\n", spuOutput.mSpeexReturnCode);
	}

	cellDmaPut(&spuOutput, (uint64_t)gviSpursSpeexTaskDesc.mSpeexTaskOutput, sizeof(SpursSpeexTaskOutput), DMA_TAG(1),
		0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));
	
	cellDmaLargePut(gviSpursSpeexStateBuffer, (uint64_t)gviSpursSpeexTaskDesc.mSpeexStateBuffer, SPEEX_DECODER_STATE_BUFFER_SIZE, DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	//spuDebugPrintf("[Speex][SPU] done sending back state buffer\n");
}

void processDecodeSet(unsigned int uiPtr)
{
	SpursSpeexTaskOutput spuOutput;
	cellDmaGet(&gviSpursSpeexTaskDesc, uiPtr, sizeof(SpursSpeexTaskDesc), DMA_TAG(1), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	//spuDebugPrintf("[Speex][SPU] CMD_SAMPLE_TASK_DECODESET_COMMAND\n");

	if (gviSpursSpeexTaskDesc.mDebugPause)
	{
		snPause();
	}
	cellDmaLargeGet(gviSpursSpeexStateBuffer, (uint64_t)gviSpursSpeexTaskDesc.mSpeexStateBuffer, SPEEX_DECODER_STATE_BUFFER_SIZE, DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));
	
	gviSpursSpeexDecodeSet(&spuOutput);

	if (spuOutput.mSpeexReturnCode < 0)
	{
		spuDebugPrintf("SPU: failed to encode, ret = %d\n", spuOutput.mSpeexReturnCode);
	}

	cellDmaPut(&spuOutput, (uint64_t)gviSpursSpeexTaskDesc.mSpeexTaskOutput, sizeof(SpursSpeexTaskOutput), DMA_TAG(1),
		0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	cellDmaLargePut(gviSpursSpeexStateBuffer, (uint64_t)gviSpursSpeexTaskDesc.mSpeexStateBuffer, SPEEX_DECODER_STATE_BUFFER_SIZE, DMA_TAG(1), 0,0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	//spuDebugPrintf("[Speex][SPU] buffer dma done\n");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void cellSpursMain(qword argTask, uint64_t argTaskset) 
{
	// grab the arguments to extract the command
	CellSPURSArgument args={uiQWord : (vec_uint4) argTask};

	unsigned int uiCommand, uiArg1=0, uiArg2=0;
	//int spuId = cellSpursGetCurrentSpuId();
	//int taskId = cellSpursGetTaskId();

	//spuDebugPrintf("[Speex][SPU] taskid: %d, spuId: %d\n", taskId, spuId);
	
	// grab the command and arguments to be processed below
	uiCommand=args.uiCommand;
	uiArg1=args.uiArgument0;
	uiArg2=args.uiArgument1;
	
	uint64_t uiPtr = uiArg1;
	
	switch(uiCommand) 
	{
		
		case SPEEX_TASK_ENCODE_INIT_COMMAND:
		{
			// cleaner this way
			procesEncodeInit(uiPtr);
			
			sendResponseToPPUAndExit(args.ppuResponseQueue, (uint32_t)uiArg2, 0);	
			break;
		}
	
		case SPEEX_TASK_ENCODE_COMMAND:
		{			
			processEncode(uiPtr);
			sendResponseToPPUAndExit(args.ppuResponseQueue, (uint32_t)uiArg2, 0);	
			break;
		}
	
		case SPEEX_TASK_DECODE_INIT_COMMAND:
		{
			processDecodeInit(uiPtr);
			sendResponseToPPUAndExit(args.ppuResponseQueue, (uint32_t)uiArg2, 0);	
			break;
		}
		case SPEEX_TASK_DECODEADD_COMMAND:
		{
			processDecodeAdd(uiPtr);
			sendResponseToPPUAndExit(args.ppuResponseQueue, (uint32_t)uiArg2, 0);	
			break;
		}
		
		case SPEEX_TASK_DECODESET_COMMAND:
		{
			processDecodeSet(uiPtr);
			sendResponseToPPUAndExit(args.ppuResponseQueue, (uint32_t)uiArg2, 0);
			break;
		}
	
		default:
		{
			//spuDebugPrintf("SPURS Sample:unknown case in switch uiCommand: %x uiArg1 %x uiArg2 %x\n",uiCommand,uiArg1,uiArg2);
		}

	}
}


