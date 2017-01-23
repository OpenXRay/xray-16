#include "..\nonport.h"
#include "screen.h"
#include "key.h"
#include "wireless.h"
#include "touch.h"
#include "backup.h"

static void Startup(void)
{
/* System */
	// init the OS system - internally initializes:
	//    arena - OS_InitArenaEx()
	//    communication system between preprocessors - PXI_Init()
	//    lock system - OS_InitLock()
	//    IRQ interrupt tables - OS_InitIrqTable()
	//    exception display system - OS_InitException()
	//    both WRAMs to the ARM7 - MI_SetWramBank()
	//    V count alarm system - OS_InitVAlarm()
	//    thread system - OS_InitThread()
	//    reset system - OS_InitReset()
	//    Game Pak library - CTRDG_Init()
	//    Card library - CARD_Init()
	//    power control system - PM_Init()
	OS_Init();

/* Time */	
	// init the system tick count
	OS_InitTick();

	// init the alarm
	// this is needed for OS_Sleep()
	OS_InitAlarm();

/* RTC */
	RTC_Init();

/* FIFO */
	PXI_InitFifo();

/* Screen */
	ScreenInit();
	Printf("Screen initialized\n");
	
	SetTopScreenLineCentered(SCREEN_HEIGHT / 2, SCWhite, "Starting GameSpy Sample");

/* Keys */
	KeyInit();
	Printf("Input initialized\n");

/* Touch */
	TouchInit();
	Printf("Touch initialized\n");

/* Interrupts */
	OS_EnableIrq();
	OS_EnableInterrupts();
	Printf("Interrupts initialized\n");

/* Heap */
    {
		u32 nHeapAdrs = 0;
		u32 nHeapSize = 1 * 1024 * 1024;
		OSHeapHandle hHeap;

		OS_SetMainArenaLo(OS_InitAlloc(OS_ARENA_MAIN, OS_GetMainArenaLo(), OS_GetMainArenaHi(), 1));
		nHeapAdrs = (u32)OS_AllocFromMainArenaLo(nHeapSize, 32);
		hHeap = OS_CreateHeap(OS_ARENA_MAIN, (void*)(nHeapAdrs), (void*)(nHeapAdrs + nHeapSize));
		OS_SetCurrentHeap(OS_ARENA_MAIN, hHeap);
    }
	Printf("Heap initialized\n");

/* Backup */
	BackupInit();

/* Wireless */
	WirelessInit();
}

static void Shutdown(void)
{
	// close down wireless
	Printf("Wireless cleanup\n");
	WirelessCleanup();
	
//	ClearScreens();
//	SetTopScreenLineCentered(SCREEN_HEIGHT / 2, SCWhite, "GameSpy Sample Shutdown");
//	SVC_WaitVBlankIntr();
	
	// terminate the os system
	Printf("Terminating OS\n");
	OS_Terminate();
}

extern int test_main(int argc, char ** argv);

static void Run(void)
{
	SetPrintMode(PRINT_TO_SCREEN|PRINT_TO_DEBUGGER);

	Printf("\n");
	Printf("GameSpy Test App Starting\n");
	Printf("-------------------------\n");
	
	test_main(0, NULL);

	Printf("------------------------\n");
	Printf("GameSpy Test App Exiting\n");

	SetPrintMode(PRINT_TO_DEBUGGER);
}

void NitroMain(void)
{
	// startup everything we need
	Startup();

	// do stuff
	Run();
	
	// shutdown the system
	Shutdown();
}