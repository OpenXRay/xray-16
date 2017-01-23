#pragma once
#include "xrCore/xrCore.h"
// Trivial (and dumb) Threads API

using TTAPIWorkerFunc = void(*)(void *lpWorkerParameters);

// Initializes subsystem
// Returns zero for error, and number of workers on success
int XRCORE_API ttapi_Init(const _processor_info &pi);

// Destroys subsystem
void XRCORE_API ttapi_Done();

// Return number of workers
int XRCORE_API ttapi_GetWorkerCount();

// Adds new task
// No more than TTAPI_HARDCODED_THREADS should be added
void XRCORE_API ttapi_AddWorker(TTAPIWorkerFunc workerFunc, void *workerFuncParams);

// Runs and wait for all workers to complete job
void XRCORE_API ttapi_Run();
