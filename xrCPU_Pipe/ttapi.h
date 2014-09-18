#ifndef _TTAPI_H_INCLUDED_
#define _TTAPI_H_INCLUDED_

#include <windows.h>

/*
	Trivial (and dumb) Threads API
*/

typedef VOID (*PTTAPI_WORKER_FUNC)( LPVOID lpWorkerParameters );
typedef PTTAPI_WORKER_FUNC LPPTTAPI_WORKER_FUNC;

#ifdef XRCPU_PIPE_EXPORTS
	#define TTAPI __declspec(dllexport)
#else // XRCPU_PIPE_EXPORTS
	#define TTAPI __declspec(dllimport)
#endif // XRCPU_PIPE_EXPORTS

extern "C"  {

	// Initializes subsystem
	// Returns zero for error, and number of workers on success
	DWORD TTAPI ttapi_Init();

	// Destroys subsystem
	VOID TTAPI ttapi_Done();

	// Return number of workers
	DWORD TTAPI ttapi_GetWorkersCount();

	// Adds new task
	// No more than TTAPI_HARDCODED_THREADS should be added
	VOID TTAPI ttapi_AddWorker( LPPTTAPI_WORKER_FUNC lpWorkerFunc , LPVOID lpvWorkerFuncParams );

	// Runs and wait for all workers to complete job
	VOID TTAPI ttapi_RunAllWorkers();

}

#endif // _TTAPI_H_INCLUDED_