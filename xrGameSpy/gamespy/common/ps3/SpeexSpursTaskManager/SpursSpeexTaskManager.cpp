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

//#define __CELLOS_LV2__ 1

#define USE_SAMPLE_PROCESS 1
#ifdef USE_SAMPLE_PROCESS


#include "spursThreadSupportInterface.h"

//#include "SPUAssert.h"
#include <string.h>


#include "SpursSpeexTaskManager.h"


#include <stdio.h>


void	SampleThreadFunc(void* userPtr,void* lsMemory)
{
	//do nothing
	printf("hello world\n");
}

void*	SamplelsMemoryFunc()
{
	//don't create local store memory, just return 0
	return 0;
}



extern "C" 
{
	extern char SPU_SAMPLE_ELF_SYMBOL[];
};





SpursSpeexTaskManager::SpursSpeexTaskManager(spursThreadSupportInterface*	threadInterface, unsigned int maxNumOutstandingTasks)
:m_threadInterface(threadInterface),
m_maxNumOutstandingTasks(maxNumOutstandingTasks)
{

	m_taskBusy.resize(m_maxNumOutstandingTasks);
	mSpursSpeexTaskDesc.resize(m_maxNumOutstandingTasks);

	for (int i = 0; (unsigned int)i < m_maxNumOutstandingTasks; i++)
	{
		m_taskBusy[i] = false;
	}
	m_numBusyTasks = 0;
	m_currentTask = 0;

	m_initialized = false;
}

SpursSpeexTaskManager::~SpursSpeexTaskManager()
{
	m_threadInterface->stopSPU();
}



int SpursSpeexTaskManager::initialize()
{
#ifdef DEBUG_SPU_TASK_SCHEDULING
	printf("SpuSampleTaskProcess::initialize()\n");
#endif //DEBUG_SPU_TASK_SCHEDULING
	
	for (int i = 0; (unsigned int)i < m_maxNumOutstandingTasks; i++)
	{
		m_taskBusy[i] = false;
	}
	m_numBusyTasks = 0;
	m_currentTask = 0;
	m_initialized = true;

	if (m_threadInterface->startSPU() != 0)
	{
		return -1;
	}
	return 0;
}



int SpursSpeexTaskManager::issueEncodeInitTask( int theQuality,int theGviSpeexSamplesPerSecond,  SpursSpeexTaskOutput *taskOutput, char *userAllocatedSpeexBuffer,int userAllocatedSpeexBufferSize )
{
	m_taskBusy[m_currentTask] = true;
	m_numBusyTasks++;

	SpursSpeexTaskDesc& taskDesc = mSpursSpeexTaskDesc[m_currentTask];
	taskDesc.mSpeexStateBuffer =  userAllocatedSpeexBuffer;
	taskDesc.mSpeexStateBufferSize =  userAllocatedSpeexBufferSize;

	taskDesc.mQuality = theQuality;
	taskDesc.mSamplesPerSecond = theGviSpeexSamplesPerSecond;
	taskDesc.mSpeexTaskOutput = taskOutput;
	if (issueTask(taskDesc,SPEEX_TASK_ENCODE_INIT_COMMAND) != 0)
		return -1;
	return 0;
}

int SpursSpeexTaskManager::issueEncodeTask(int16_t * inBuffer, int inBufferSize, int encodedFrameSize, char *outBuffer, 
										   int outBufferSize, SpursSpeexTaskOutput *taskOuput,char *userAllocatedSpeexBuffer,
										   int userAllocatedSpeexBufferSize )
{
	m_taskBusy[m_currentTask] = true;
	m_numBusyTasks++;

	SpursSpeexTaskDesc& taskDesc = mSpursSpeexTaskDesc[m_currentTask];
	taskDesc.mSpeexStateBuffer =  userAllocatedSpeexBuffer;
	taskDesc.mSpeexStateBufferSize =  userAllocatedSpeexBufferSize;
	taskDesc.mEncodedFrameSize = encodedFrameSize;
	taskDesc.mInputBuffer = inBuffer;
	taskDesc.mInputBufferSize = inBufferSize;
	taskDesc.mOutputBuffer = outBuffer;
	taskDesc.mOutputBufferSize = outBufferSize;
	taskDesc.mSpeexTaskOutput = 	taskOuput;
	if (issueTask(taskDesc,SPEEX_TASK_ENCODE_COMMAND) != 0)
		return -1;
	return 0;
}

int SpursSpeexTaskManager::issueDecodeAddTask(char *decoderStateBuffer, int decoderStateBufferSize, char *inBuffer, int inBufferSize, int encodedFrameSize,  
											  short* outBuffer, int outBufferSize, struct SpursSpeexTaskOutput *taskOutput)
{
	m_taskBusy[m_currentTask] = true;
	m_numBusyTasks++;

	SpursSpeexTaskDesc& taskDesc = mSpursSpeexTaskDesc[m_currentTask];
	taskDesc.mSpeexStateBuffer = decoderStateBuffer;
	taskDesc.mSpeexStateBufferSize = decoderStateBufferSize;
	taskDesc.mEncodedFrameSize = encodedFrameSize;
	taskDesc.mInputBuffer = inBuffer;
	taskDesc.mInputBufferSize = inBufferSize;
	taskDesc.mOutputBuffer = outBuffer;
	taskDesc.mOutputBufferSize = outBufferSize;
	taskDesc.mSpeexTaskOutput = taskOutput;
	if (issueTask(taskDesc,SPEEX_TASK_DECODEADD_COMMAND) != 0)
		return -1;
	return 0;
}

int SpursSpeexTaskManager::issueDecodeSetTask(char *decoderStateBuffer, int decoderStateBufferSize, char *inBuffer, int inBufferSize,  int encodedFrameSize,
											  short* outBuffer, int outBufferSize, struct SpursSpeexTaskOutput *taskOutput )
{
	m_taskBusy[m_currentTask] = true;
	m_numBusyTasks++;

	SpursSpeexTaskDesc& taskDesc = mSpursSpeexTaskDesc[m_currentTask];
	taskDesc.mSpeexStateBuffer = decoderStateBuffer;
	taskDesc.mSpeexStateBufferSize = decoderStateBufferSize;
	taskDesc.mEncodedFrameSize = encodedFrameSize;
	taskDesc.mInputBuffer = inBuffer;
	taskDesc.mInputBufferSize = inBufferSize;
	taskDesc.mOutputBuffer = outBuffer;
	taskDesc.mOutputBufferSize = outBufferSize;
	taskDesc.mSpeexTaskOutput = taskOutput;
	if (issueTask(taskDesc,SPEEX_TASK_DECODESET_COMMAND) !=  0)
		return -1;
	return 0;
}

int SpursSpeexTaskManager::issueDecodeInitTask( char *decoderStateBuffer, int decoderStateBufferSize, int sampleRate, struct SpursSpeexTaskOutput *taskOutput )
{
	m_taskBusy[m_currentTask] = true;
	m_numBusyTasks++;

	SpursSpeexTaskDesc& taskDesc = mSpursSpeexTaskDesc[m_currentTask];
	taskDesc.mSpeexStateBuffer = decoderStateBuffer;
	taskDesc.mSpeexStateBufferSize = decoderStateBufferSize;
	taskDesc.mSamplesPerSecond = sampleRate;
	taskDesc.mSpeexTaskOutput = taskOutput;
	if (issueTask(taskDesc,SPEEX_TASK_DECODE_INIT_COMMAND) != 0)
		return -1;
	return 0;
}


int SpursSpeexTaskManager::issueTask( SpursSpeexTaskDesc& taskDesc,uint32_t uiCommand )
{
#ifdef DEBUG_SPU_TASK_SCHEDULING
	printf("SpuSampleTaskProcess::issueTask (m_currentTask= %d\)n", m_currentTask);
#endif //DEBUG_SPU_TASK_SCHEDULING

	//some bookkeeping to recognize finished tasks
	taskDesc.mTaskId = m_currentTask;

	if (m_threadInterface->sendRequest(uiCommand, (uint32_t) &taskDesc, m_currentTask) != 0)
	{
		m_taskBusy[m_currentTask] = false;
		m_numBusyTasks--;
		return -1;
	}

	// if all tasks busy, wait for spu event to clear the task.
	if (m_numBusyTasks >= m_maxNumOutstandingTasks)
	{
		unsigned int taskId;
		unsigned int outputSize;

		if (m_threadInterface->waitForResponse(&taskId, &outputSize) != 0)
		{
			return -2;
		}

		//printf("PPU: after issue, received event: %u %d\n", taskId, outputSize);
		//postProcess(taskId, outputSize);
		m_taskBusy[taskId] = false;
		m_numBusyTasks--;
	}

	// find new task buffer
	for (unsigned int i = 0; i < m_maxNumOutstandingTasks; i++)
	{
		if (!m_taskBusy[i])
		{
			m_currentTask = i;
			break;
		}
	}
	return 0;
}

///Optional PPU-size post processing for each task
// void SpuSampleTaskProcess::postProcess(int taskId, int outputSize)
// {
// 
// }


int SpursSpeexTaskManager::flush()
{
#ifdef DEBUG_SPU_TASK_SCHEDULING
	printf("\nSpuCollisionTaskProcess::flush()\n");
#endif //DEBUG_SPU_TASK_SCHEDULING

	// all tasks are issued, wait for all tasks to be complete
	while(m_numBusyTasks > 0)
	{
		// Consolidating SPU code
		unsigned int taskId;
		unsigned int outputSize;

		if (m_threadInterface->waitForResponse(&taskId, &outputSize) != 0)
			return -1;
		//printf("PPU: flushing, received event: %u %d\n", taskId, outputSize);
		//postProcess(taskId, outputSize);
		m_taskBusy[taskId] = false;
		m_numBusyTasks--;
	}
	return 0;
}
#endif //USE_SAMPLE_PROCESS
