/* [SCE CONFIDENTIAL DOCUMENT]
 * PLAYSTATION(R)3 SPU Optimized Bullet Physics Library (http://bulletphysics.com)
 *                Copyright (C) 2007 Sony Computer Entertainment Inc.
 *                                                All Rights Reserved.
 */


#ifndef __SPURS_SUPPORT_INTERFACE_H
#define __SPURS_SUPPORT_INTERFACE_H

#include <cell/spurs/queue.h>
#include "spursUtilityMacros.h"
#include "spursThreadSupportInterface.h"

#ifdef __SPU__
#include <simd>

#include <sdk_version.h>

#if CELL_SDK_VERSION < 0x081000
#define CELL_SPURS_TASK_ERROR_AGAIN CELL_SPURS_EAGAIN
#define CELL_SPURS_TASK_ERROR_BUSY CELL_SPURS_EBUSY
#endif // CELL_SDK_VERSION < 0x081000

#else // __SPU__
#include <cell/spurs/task.h>

#endif // __SPU__

#define CELL_SPURS_RESPONSE_QUEUE_SIZE 128

/**
 * Note:
 * The order of elements in this enum are important, that's why each one is explicitly
 * given a value.  They will correspond to the .elf names/addresses that will be
 * loaded into SPURS.
 * Mixing up these values will cause the wrong code to execute, for instance, the
 * solver may be asked to do a collision detection job.
 */
//////////////////////////////////////////////////////////////////////////
// only one type of SPURS Task ELF 
// typedef enum {
// //	SPU_ELF_MID_PHASE=0,
// //	SPU_ELF_SOLVER,
// 	SPU_ELF_SPEEX,
// 	SPU_ELF_LAST,
// } CellSpursElfId_t;

typedef union CellSPURSArgument 
{
	struct 
	{
		CELL_PPU_POINTER(CellSpursQueue) ppuResponseQueue;
		uint32_t uiCommand;
		uint32_t uiArgument0;
		uint32_t uiArgument1;
	};

#if __PPU__
	CellSpursTaskArgument spursArgument;
#elif __SPU__
	vec_uint4 uiQWord;
#endif
} CellSPURSArgument __attribute__((aligned(16)));

#if __SPU__
#include "SPUAssert.h"
#include <cell/spurs/task.h>

static inline void sendResponseToPPU(uint32_t ppuQueueEA, uint32_t uiArgument0,
											uint32_t uiArgument1, int iTag=1) {
	CellSPURSArgument response 
		__attribute__ ((aligned(16)));

	response.uiArgument0=uiArgument0;
	response.uiArgument1=uiArgument1;

	int iReturn;
	do {
		iReturn=cellSpursQueueTryPushBegin(ppuQueueEA, &response, iTag);
	} while (iReturn == CELL_SPURS_TASK_ERROR_AGAIN ||
			 iReturn == CELL_SPURS_TASK_ERROR_BUSY);

	SPU_ASSERT((iReturn == CELL_OK) && "Error writing to SPURS queue.");

	cellSpursQueuePushEnd(ppuQueueEA, iTag);

}

static inline void sendResponseToPPUAndExit(uint32_t ppuQueueEA, uint32_t uiArgument0,
											uint32_t uiArgument1, int iTag=1) {
	CellSPURSArgument response 
		__attribute__ ((aligned(16)));

	response.uiArgument0=uiArgument0;
	response.uiArgument1=uiArgument1;

	int iReturn;
	do {
		iReturn=cellSpursQueueTryPushBegin(ppuQueueEA, &response, iTag);
	} while (iReturn == CELL_SPURS_TASK_ERROR_AGAIN ||
			 iReturn == CELL_SPURS_TASK_ERROR_BUSY);

	SPU_ASSERT((iReturn == CELL_OK) && "Error writing to SPURS queue.");

	cellSpursQueuePushEnd(ppuQueueEA, iTag);

	cellSpursExit();
}
#elif __PPU__ // not __SPU__

class SpursSupportInterface : public spursThreadSupportInterface
{
public:
	SpursSupportInterface();
	~SpursSupportInterface();
	int sendRequest(uint32_t uiCommand, uint32_t uiArgument0, uint32_t uiArgument1=0);
	int waitForResponse(unsigned int *puiArgument0, unsigned int *puiArgument1);
	int startSPU();
	int stopSPU();

protected:
	//CellSpursElfId_t m_elfId;
	void *m_spursTaskAddress;
	CellSpursQueue m_responseQueue __attribute__((aligned(128)));
	CellSPURSArgument m_aResponseBuffer[CELL_SPURS_RESPONSE_QUEUE_SIZE] __attribute__((aligned(16)));

	bool m_bQueueInitialized;
};
#endif // __SPU__ / __PPU__


#endif // CELL_SPURS_SUPPORT_H
