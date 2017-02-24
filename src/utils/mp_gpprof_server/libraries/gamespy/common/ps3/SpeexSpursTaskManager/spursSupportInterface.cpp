#include "spursConfiguration.h"
#include "spursSupportInterface.h"
#include <spu_printf.h>
#include <sys/ppu_thread.h>
#include <sys/spu_initialize.h>
#include <stdio.h>
#include <stdlib.h>
#include <cell/atomic.h>
#include <assert.h>

#define SCE_EXTERNAL_RELEASE

// for CELL_FS_MAX_FS_PATH_LENGTH
#include <sys/fs.h>

static uint32_t _g_uiNextEventQueueKey=0x92400ABDUL;
uint32_t _SpursSupportGetUniqueEventQueueKey() {
	return _g_uiNextEventQueueKey++;
}

sys_event_queue_t _g_SpuPrintfEventQueue;
sys_ppu_thread_t	_g_SpursPrintfThread;

void _SpursPrintfThreadMain(uint64_t arg);

#define	SPURS_PPU_THREAD_PRIO 1001
#define SPU_PRINTF_EVENT_QUEUE_SIZE 8
#define SPU_PRINTF_EVENT_QUEUE_PORT 0x1
#define SPU_PRINTF_THREAD_PRIO 1001
#define SPU_PRINTF_THREAD_STACK_SIZE (64 * 1024)

/*
typedef struct BulletSpursElf {
	const char *pcElfName;
	void *pvElfImage;
} BulletSpursElf;
*/

#ifdef _DEBUG
extern char _binary_spu_SpeexSpursTaskDebug_elf_start[];
void *g_SpursTaskElfStart = _binary_spu_SpeexSpursTaskDebug_elf_start;
#else
extern char _binary_spu_SpeexSpursTaskRelease_elf_start[];
void *g_SpursTaskElfStart = _binary_spu_SpeexSpursTaskRelease_elf_start;
#endif //#ifdef _DEBUG


/*
BulletSpursElf _g_aSPURSElfs[]={
#ifdef _DEBUG
	{"test.elf", 	_binary_spu_PS3_SPURS_SpeexDebug_elf_start}
#else
	{"test.elf", 	_binary_spu_PS3_SPURS_SpeexRelease_elf_start}
#endif
};
*/

/*
#ifndef SCE_EXTERNAL_RELEASE
// Here's the search path for the SPURS programs:
char *_g_apcSPURSELFSearchPaths[]={
  "/app_home/",
  "/app_home/../../lib/",       // works with samples
  "/app_home/../../../lib/",       // works with tutorials
  "/app_home/../../../../lib/",
  "/app_home/../../../../../lib/",
};
unsigned int _g_uiSPURSELFSearchPathCount=sizeof(_g_apcSPURSELFSearchPaths)/sizeof(char *);
#endif // SCE_EXTERNAL_RELEASE
*/

// E The priority for the SPU thread group used by SPURS
#define SPURS_SPU_THREAD_PRIORITY 200

static CellSpurs *g_spursInstance=0;
static CellSpursTaskset g_spursTaskSet __attribute__ ((aligned(128)));

static int g_iDefaultSPUCount=CELL_SPURS_DEFAULT_SPU_COUNT;
static bool g_bSpursInitialized=false;
static bool g_bUserSpursGiven=false;
static uint32_t g_uiSpursReferenceCounter=0;

/**
 * E This is the main function of the spu_printf service thread.
 * It listens for messages and calls the printf handler when it gets them,
 * then notifies the SPU of completion by using a mailbox.
 */
//UPDATE: this function is not currently being used in the speex task or spurs manager
void _SpursPrintfThreadMain(uint64_t arg) 
{
	sys_event_t event;
	int iReturn;

	// E For unused parameter warnings
	(void) arg;

	while (1) {
		iReturn=sys_event_queue_receive(_g_SpuPrintfEventQueue, &event,
										SYS_NO_TIMEOUT);
	
		if (iReturn!=CELL_OK) {
			fprintf(stderr, "Event queue receive wasn't successful: %i\n",
					iReturn);
			exit(-1);
		}

		iReturn=spu_thread_printf(event.data1, event.data3);
		sys_spu_thread_write_spu_mb(event.data1, iReturn);
	}
}

/*
void *loadBulletImage(const char *pcFilename) {
	void *pvElf;
	FILE *pfInputFile=fopen(pcFilename, "rb");
	uint64_t uiFileSize;

	if (!pfInputFile) {
		return 0;
	}

	fseek(pfInputFile, 0, SEEK_END);
	uiFileSize=ftell(pfInputFile);
	fseek(pfInputFile, 0, SEEK_SET);
	pvElf=memalign(128, uiFileSize);

	if (!pvElf) {
		printf("Cannot allocate memory for file %s\n", pcFilename);
		fclose(pfInputFile);
		return 0;
	}

	fread(pvElf, 1, uiFileSize, pfInputFile);
	fclose(pfInputFile);

	return pvElf;
}

void unloadBulletSpursElfs() {
#ifdef SCE_EXTERNAL_RELEASE
	return;
#endif

	for (int iELF=0; iELF<SPU_ELF_LAST; iELF++) {
		if (_g_aSPURSElfs[iELF].pvElfImage) {
			free(_g_aSPURSElfs[iELF].pvElfImage);
			_g_aSPURSElfs[iELF].pvElfImage=0;
		}
	}
}

// Loads the SPURS .ELF files.  If we're doing an external release, they're linked
// in, so we're ok.
int loadBulletSpursElfs() {
#ifdef SCE_EXTERNAL_RELEASE
	return CELL_SPURS_OK;
#else // SCE_EXTERNAL_RELEASE
	int iReturn=CELL_SPURS_OK;

	for (int iELF=0; iELF<SPU_ELF_LAST; iELF++) {
		if (_g_aSPURSElfs[iELF].pvElfImage) {
			free(_g_aSPURSElfs[iELF].pvElfImage);
			_g_aSPURSElfs[iELF].pvElfImage=0;
		}

		for (int iPath=0; ((unsigned int)iPath<_g_uiSPURSELFSearchPathCount) &&
			(_g_aSPURSElfs[iELF].pvElfImage==0); iPath++) {
			char acFullPath[CELL_FS_MAX_FS_PATH_LENGTH+1];
			snprintf(acFullPath, CELL_FS_MAX_FS_PATH_LENGTH,
				"%s%s", _g_apcSPURSELFSearchPaths[iPath], 
				_g_aSPURSElfs[iELF].pcElfName);
			_g_aSPURSElfs[iELF].pvElfImage=loadBulletImage(acFullPath);
		}

		if (_g_aSPURSElfs[iELF].pvElfImage==0) {
			fprintf(stderr, "Bullet: Cannot load %s\n", _g_aSPURSElfs[iELF].pcElfName);
			iReturn=CELL_SPURS_EINVAL;
			break;
		}
	}

	// If something went wrong, free up any of the .ELF files that loaded successfuly.
	if (iReturn!=CELL_SPURS_OK) {
		unloadBulletSpursElfs();
	}

	return iReturn;
#endif // SCE_EXTERNAL_RELEASE
}
*/

int initializeSpursTaskSet(CellSpurs *pSpurs, int iSPUCount, uint8_t *puiPriorities) 
{
	uint8_t auiLocalPriorities[8]={1,1,1,1,1,1,1,1};
	uint8_t *pPriorities=auiLocalPriorities;
	int iReturn;
	int iNumSPUs=iSPUCount;

	// Not loading from a file anymore
// 	if (loadBulletSpursElfs()!=CELL_SPURS_OK) {
// 		fprintf(stderr, "Bullet: Cannot load SPURS programs.");
// 		return CELL_SPURS_EINVAL;
// 	}

	if (pSpurs) 
	{
		g_bUserSpursGiven=true;

		pPriorities=puiPriorities;
		iNumSPUs=iSPUCount;
		g_spursInstance=pSpurs;
	} 
	else 
	{
		g_bUserSpursGiven=false;

		// E We need to figure out the priority for the SPURS handler thread.
		// E We'll do this by getting the current thread priority, and making
		// E the SPURS handler thread slightly higher priority.
		sys_ppu_thread_t idCurrentThread;
		int iCurrentThreadPriority;

		iReturn=sys_ppu_thread_get_id(&idCurrentThread);
		if (iReturn!=CELL_OK) 
		{
			//fprintf(stderr, "Bullet: Cannot get current thread ID (%d)\n", iReturn);
			return CELL_SPURS_EINVAL;
		}

		iReturn=sys_ppu_thread_get_priority(idCurrentThread,
											&iCurrentThreadPriority);
		if (iReturn!=CELL_OK) 
		{
			//fprintf(stderr, "Bullet: Cannot get current thread priority (%d)\n", iReturn);
			return CELL_SPURS_EINVAL;
		}

		g_spursInstance=(CellSpurs *) memalign(128, sizeof(CellSpurs));
		if (!g_spursInstance)
		{
			//fprintf(stderr,"Cannot allocate room for SPURS\n");		
			return CELL_SPURS_EINVAL;
		}
		
		sys_spu_initialize(iNumSPUs, 0);
		// E Initialize spurs, setting SPU count, priorities, and letting SPURS
		// E know that it should NOT release the SPUs when there is no work to
		// E be done.
		iReturn=cellSpursInitialize(g_spursInstance, iNumSPUs,
									SPURS_SPU_THREAD_PRIORITY,
									iCurrentThreadPriority - 1, false);
		if (iReturn!=CELL_OK) 
		{
			//fprintf(stderr, "Bullet: Cannot initialize SPURS (%d)\n", iReturn);
			free(g_spursInstance);
			return CELL_SPURS_EINVAL;
		}
	}

	// Create SPU printf support.
//	sys_event_queue_attribute_t queue_attr;    /* Event queue attributes */

	// E Create an event queue that the SPU can use to send printf
	// E commands to the PPU for debugging.
// 	sys_event_queue_attribute_initialize(queue_attr);
// 	iReturn=sys_event_queue_create(&_g_SpuPrintfEventQueue, &queue_attr,
// 									_SpursSupportGetUniqueEventQueueKey(),
// 									SPU_PRINTF_EVENT_QUEUE_SIZE);
// 
// 	if (iReturn!=CELL_OK) 
// 	{
// 		return CELL_SPURS_EMISC;
// 	}

	// E We need a thread on the PPU side to handle SPU printf requests.
	// E It will wait indefinitely for messages in the queue we just created.
// 	iReturn = sys_ppu_thread_create (&_g_SpursPrintfThread,
// 										_SpursPrintfThreadMain, 
// 										(uint64_t) 0, SPU_PRINTF_THREAD_PRIO,
// 										SPU_PRINTF_THREAD_STACK_SIZE, 
// 										SYS_PPU_THREAD_CREATE_JOINABLE,
// 										"Bullet_spu_printf_handler");

// 	if (iReturn != CELL_OK) 
// 	{
// 		return CELL_SPURS_EMISC;
// 	}

	// E Now, we attach the PPU-side printf support to SPURS
// 	uint8_t uiPrintfPort=SPU_PRINTF_EVENT_QUEUE_PORT;
// 
// 	iReturn=cellSpursAttachLv2EventQueue(g_spursInstance, _g_SpuPrintfEventQueue,
// 										 &uiPrintfPort, 0);
// 	if (iReturn!=CELL_OK) {
// 		return CELL_SPURS_EMISC;
// 	}

	iReturn=cellSpursCreateTaskset(g_spursInstance, &g_spursTaskSet, 0, pPriorities,
								   iNumSPUs);

	if (iReturn!=CELL_OK) 
	{
		cellSpursFinalize(g_spursInstance);
		free(g_spursInstance);
		return CELL_SPURS_EINVAL;
	}

	g_bSpursInitialized=true;

	return CELL_SPURS_OK;
}

int checkSpursTaskSet() 
{
	if (!g_bSpursInitialized) 
	{
		int returnCode = initializeSpursTaskSet(0, g_iDefaultSPUCount, 0)!=CELL_SPURS_OK;
		return returnCode;
	}
	return CELL_SPURS_OK;
}

/**
 * Initializes Bullet SPUs given a pre-configured SPURS.
 * Pass in a pointer to SPURS as well as the priorities for the code to use.
 * Return is:
 * CELL_SPURS_OK on success
 * CELL_SPURS_EBUSY if SPU usage has already been initialized
 * CELL_SPURS_EINVAL if the priorities or SPURS pointer is invalid.
 */
int spursConfiguration_initWithSpurs(CellSpurs *pSpurs, int iSPUCount, uint8_t auiPriorities[8]) 
{
	if (g_bSpursInitialized) 
	{
		return CELL_SPURS_EBUSY;
	}

	if ((!pSpurs) || (((uintptr_t) pSpurs) & 0x7f)) 
	{
		return CELL_SPURS_EINVAL;
	}

	if (iSPUCount<1 || iSPUCount>6) 
	{
		return CELL_SPURS_EINVAL;
	}

	return initializeSpursTaskSet(pSpurs, iSPUCount, auiPriorities);
}

/**
 * Sets the number of SPUs to be used by a Bullet-initialized SPURS.
 * Valid iSPUCount is in the range 1-6.
 * Return is:
 * CELL_SPURS_OK on success
 * CELL_SPURS_EBUSY if SPU usage has already been initialized
 * CELL_SPURS_EINVAL if iSPUCount is out of range or if SPURS couldn't be
 *   initialized to that many SPUs.
 */
int spursConfiguration_initWithSpuCount(int iSPUCount) 
{
	if (g_bSpursInitialized) 
	{
		return CELL_SPURS_EBUSY;
	}

	if (iSPUCount<1 || iSPUCount>6) 
	{
		return CELL_SPURS_EINVAL;
	}

	return initializeSpursTaskSet(0, iSPUCount, 0);
}

/**
 * Terminates (or disconnects from) SPURS.
 * Return is:
 * CELL_SPURS_OK if SPURS terminates ok, or if it was previously terminated/
 *   never initialized.
 * CELL_SPURS_EBUSY if there are existing Scenes which would need SPURS.
 */
int spursConfiguration_terminate() 
{
	if (!g_bSpursInitialized) 
	{
		return CELL_SPURS_OK;
	}

	if (g_uiSpursReferenceCounter) 
	{
		return CELL_SPURS_EBUSY;
	}
	int iReturn;
	bool bUserSpurs=g_bUserSpursGiven;

	g_bSpursInitialized=false;
	g_bUserSpursGiven=false;

	iReturn=cellSpursShutdownTaskset(&g_spursTaskSet);
	if (iReturn!=CELL_OK) 
	{
		return -1;
		//fprintf(stderr, "Bullet: Error shutting down SPURS task set: %i\n", iReturn);
	}

	iReturn=cellSpursJoinTaskset(&g_spursTaskSet);
	if (iReturn!=CELL_OK) 
	{
		return -2;
		//fprintf(stderr, "Bullet: Error joining SPURS task set: %i\n", iReturn);
	}

	if (!bUserSpurs) 
	{
		int iReturn=cellSpursFinalize(g_spursInstance);

		if (iReturn!=CELL_OK) 
		{
			return -3;
			//fprintf(stderr, "Bullet: Error shutting down SPURS: %d\n", iReturn);
		}

		free(g_spursInstance);
	}

	g_spursInstance=0;

	// Not loading SPURS task from file anymore
	//unloadBulletSpursElfs();
// printf code is commented out for the time being
//#warning TODO: Clean up SPU printf thread.

	return CELL_SPURS_OK;
}

/**
 * Queries whether SPU usage has been initialized.
 * Return is:
 * true if initialized.
 */
bool spursConfiguration_isSpursInitialized() 
{
	return g_bSpursInitialized;
}


SpursSupportInterface::SpursSupportInterface() 
{
	//assert(elfId < SPU_ELF_LAST);
	//m_elfId=elfId;
	m_bQueueInitialized=false;

	cellAtomicIncr32(&g_uiSpursReferenceCounter);
}

SpursSupportInterface::~SpursSupportInterface() 
{
	stopSPU();

	cellAtomicDecr32(&g_uiSpursReferenceCounter);
}

int SpursSupportInterface::startSPU() 
{
	int iReturn;

	if (!m_bQueueInitialized) 
	{
		if (checkSpursTaskSet() != CELL_SPURS_OK)
		{
			return -1;
		}

		iReturn=cellSpursQueueInitialize(&g_spursTaskSet,
			&m_responseQueue, m_aResponseBuffer, sizeof(CellSPURSArgument),
			CELL_SPURS_RESPONSE_QUEUE_SIZE, CELL_SPURS_QUEUE_SPU2PPU);
		if (iReturn!=CELL_OK) 
		{
			return -2;
		}

		iReturn=cellSpursQueueAttachLv2EventQueue(&m_responseQueue);
		if (iReturn!=CELL_OK) 
		{
			return -3;
		}

		m_bQueueInitialized=true;
	}
	return 0;
}

int SpursSupportInterface::stopSPU() 
{
	if (m_bQueueInitialized) 
	{
		int iReturn=cellSpursQueueDetachLv2EventQueue(&m_responseQueue);
		if (iReturn != CELL_OK)
			return -1;
		m_bQueueInitialized=false;
	}
	return 0;
}

int SpursSupportInterface::sendRequest(uint32_t uiCommand, uint32_t uiArgument0, uint32_t uiArgument1) 
{
	int iReturn;
	CellSpursTaskId taskId;
	CellSPURSArgument arguments;

	arguments.ppuResponseQueue=&m_responseQueue;
	arguments.uiCommand=uiCommand;
	arguments.uiArgument0=uiArgument0;
	arguments.uiArgument1=uiArgument1;

	if (checkSpursTaskSet() != CELL_SPURS_OK)
	{
		return -1;
	}

	iReturn=cellSpursCreateTask(&g_spursTaskSet, &taskId, g_SpursTaskElfStart, NULL, 0, 0, &arguments.spursArgument);
	if (iReturn!=CELL_OK) 
	{
		return -2;
	}
	
	return 0;
}


/**
 * Wait for the SPU to send an event back to our event queue.
 */
int SpursSupportInterface::waitForResponse(unsigned int *puiArgument0, unsigned int *puiArgument1) 
{
	CellSPURSArgument response __attribute__((aligned(16)));
	int iReturn;

	iReturn=cellSpursQueuePop(&m_responseQueue, (void *) &response);
	
	if (iReturn!=CELL_OK) 
	{
		return -1;
	}

	if (puiArgument0)
		*puiArgument0=response.uiArgument0;

	if (puiArgument1)
		*puiArgument1=response.uiArgument1;
	return 0;
}

