

#ifndef __SPEEX_TASK_OUTPUT_H
#define __SPEEX_TASK_OUTPUT_H

#define POST_ALIGN(x)	__attribute__((aligned (x)))

#ifdef __cplusplus
extern "C" { 
#endif

#define SPEEX_ENCODER_WB_BUFFERSIZE 32256
#define SPEEX_ENCODER_NB_BUFFERSIZE 32288
#define SPEEX_ENCODER_STATE_BUFFER_SIZE (SPEEX_ENCODER_NB_BUFFERSIZE + SPEEX_ENCODER_WB_BUFFERSIZE+128) 
#define SPEEX_DECODER_NB_BUFFERSIZE 16832
#define SPEEX_DEOCDER_WB_BUFFERSIZE 24192
#define SPEEX_DECODER_STATE_BUFFER_SIZE (SPEEX_DECODER_NB_BUFFERSIZE+SPEEX_DEOCDER_WB_BUFFERSIZE+128)
//#define SPEEX_STATE_
	///pure output, any input is in SpuSampleTaskDesc
struct SpursSpeexTaskOutput
{
	int           mSpeexInitialized;
	int			  mSpeexSamplesPerFrame;
	int			  mSpeexEncodedFrameSize;
	int           mSpeexOutBufferSize;
	int           mSpeexReturnCode;
} POST_ALIGN(128);

#ifdef __cplusplus
}
#endif

#endif //__SPEEX_TASK_OUTPUT_H
