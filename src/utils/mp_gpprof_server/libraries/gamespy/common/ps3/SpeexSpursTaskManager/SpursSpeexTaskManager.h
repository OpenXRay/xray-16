// Gamespy Technology
// NOTE:  this code has been provided by Sony for usage in Speex SPURS Manager

/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2007 Erwin Coumans  http://bulletphysics.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef SPU_SAMPLE_TASK_PROCESS_H
#define SPU_SAMPLE_TASK_PROCESS_H

#include <assert.h>


#include "spursPlatformDefinitions.h"

#include <stdlib.h>

#include "spursAlignedObjectArray.h"

#include "SpuSpeexTaskOutput.h"

///SpuSampleTaskDesc
struct SpursSpeexTaskDesc
{
	SpursSpeexTaskDesc()
		:mDebugPause(false)
	{
		
	}
	int           mQuality; 
	int           mSamplesPerSecond;
	int           mEncodedFrameSize;
	void          *mInputBuffer;     // make it 
	unsigned int   mInputBufferSize; 

	SpursSpeexTaskOutput	*mSpeexTaskOutput;
	void            *mOutputBuffer; 
	unsigned int     mOutputBufferSize;

	char            *mSpeexStateBuffer; 
	unsigned int     mSpeexStateBufferSize;
	bool	mDebugPause;

	uint16_t 	  mTaskId;
	//uint16_t	_padding_[3]; //padding to make this multiple of 16 bytes
} POST_ALIGN(128);

//just add your commands here, try to keep them globally unique for debugging purposes
#define SPEEX_TASK_ENCODE_COMMAND      10
#define SPEEX_TASK_ENCODE_INIT_COMMAND 11
#define SPEEX_TASK_DECODEADD_COMMAND   12
#define SPEEX_TASK_DECODESET_COMMAND   13
#define SPEEX_TASK_DECODE_INIT_COMMAND 14



/// SpuSampleTaskProcess handles SPU processing of collision pairs.
/// When PPU issues a task, it will look for completed task buffers
/// PPU will do postprocessing, dependent on workunit output (not likely)
class SpursSpeexTaskManager
{
	// track task buffers that are being used, and total busy tasks
	spursAlignedObjectArray<bool>	m_taskBusy;
	spursAlignedObjectArray<SpursSpeexTaskDesc>mSpursSpeexTaskDesc;
	
	unsigned int   m_numBusyTasks;

	// the current task and the current entry to insert a new work unit
	unsigned int   m_currentTask;

	bool m_initialized;

	//void postProcess(int taskId, int outputSize);
	
	class	spursThreadSupportInterface*	m_threadInterface;

	unsigned int	m_maxNumOutstandingTasks;


	int issueTask(SpursSpeexTaskDesc& taskDesc,uint32_t uiCommand);


public:
	SpursSpeexTaskManager(spursThreadSupportInterface*	threadInterface, unsigned int maxNumOutstandingTasks);
	
	~SpursSpeexTaskManager();
	
	///call initialize in the beginning of the frame, before addCollisionPairToTask
	int initialize();

	int issueEncodeTask(int16_t * inBuffer, int inBufferSize, int encodedFrameSize, char *outBuffer, int outBufferSize, 
		                SpursSpeexTaskOutput *taskOuput,char *m_userAllocatedSpeexBuffer,int userAllocatedSpeexBufferSize);

	int issueEncodeInitTask(int theQuality,int theGviSpeexSamplesPerSecond,  SpursSpeexTaskOutput *taskOutput, char *m_userAllocatedSpeexBuffer,
	                        int userAllocatedSpeexBufferSize);

	int issueDecodeAddTask(char *decoderStateBuffer, int decoderStateBufferSize, char *inBuffer, int inBufferSize, int encodedFrameSize,
		                   short* outBuffer, int outBufferSize, struct SpursSpeexTaskOutput *taskOutput);

	int issueDecodeSetTask(char *decoderStateBuffer, int decoderStateBufferSize, char *inBuffer, int inBufferSize, int encodedFrameSize,
		                   short* outBuffer, int outBufferSize, struct SpursSpeexTaskOutput *taskOutput);
	
	int issueDecodeInitTask(char *decoderStateBuffer, int decoderStateBufferSize, int sampleRate, struct SpursSpeexTaskOutput *taskOutput);

	///call flush to submit potential outstanding work to SPUs and wait for all involved SPUs to be finished
	int flush();
};


#endif // SPU_SAMPLE_TASK_PROCESS_H

