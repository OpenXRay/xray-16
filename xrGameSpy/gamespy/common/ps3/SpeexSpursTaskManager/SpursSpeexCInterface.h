#ifndef SPU_TASK_C_INTERFACE_H
#define SPU_TASK_C_INTERFACE_H

//#include <speex.h>
#include "SpuSpeexTaskOutput.h"

#define PL_DECLARE_HANDLE(name) typedef struct name##__ { int unused; } *name

typedef float	plReal;
typedef plReal	plVector3[3];
typedef plReal	plQuaternion[4];

#ifdef __cplusplus
extern "C" { 
#endif

#define GVI_REMAINING_BYTES 3


///initialize SPURS
int initializeSpursSampleTask();

///submit some work to SPURS
int issueSampleTaskEncode(short* inBuffer, int inBufferSize, int encodedFrameSize,  char *outBuffer, int outBufferSize, 
                          struct SpursSpeexTaskOutput *taskOuput, char *userAllocatedSpeexBuffer, 
                          int userAllocatedSpeexBufferSize);

///not finished, need to pass proper data
int issueSampleTaskEncodeInit(int quality, int samplesPerFrame, struct SpursSpeexTaskOutput *taskOutput,
                              char *userAllocatedSpeexBuffer, int userAllocatedSpeexBufferSize);
///not finished, need to pass proper data
int issueSampleTaskDecodeAdd(char *decoderStateBuffer, int decoderStateBufferSize, char *inBuffer, int inBufferSize, int encodedFrameSize,
							 short* outBuffer, int outBufferSize, struct SpursSpeexTaskOutput *taskOutput);
int issueSampleTaskDecodeSet(char *decoderStateBuffer, int decoderStateBufferSize, char *inBuffer, int inBufferSize, int encodedFrameSize,
							 short* outBuffer, int outBufferSize, struct SpursSpeexTaskOutput *taskOutput);
///not finished, need to pass proper data
int issueSampleTaskDecodeInit(char *decoderStateBuffer, int decoderStateBufferSize, int sampleRate, struct SpursSpeexTaskOutput *taskOutput);




///wait for the work to be finished
//int flushSampleTask();

///shutdown SPURS
int shutdownSpursTask();

///used to pass into SPURS

#ifdef __cplusplus
}
#endif

#endif //SPU_TASK_C_INTERFACE_H
