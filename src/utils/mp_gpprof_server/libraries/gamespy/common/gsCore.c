///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Core task/callback manager
#include "gsPlatform.h"
#include "gsPlatformThread.h"

#include "gsCommon.h"
#include "gsCore.h"
#include "gsAssert.h"
#include "../ghttp/ghttp.h"




// This defines how long the core will wait if there is a thread synchronization
// problem when initializing or shutting down the core.  
#define GSI_CORE_INIT_YIELD_MS      100
#define GSI_CORE_SHUTDOWN_YIELD_MS  50


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static GSCoreMgr* gsiGetStaticCore()
{
	static GSCoreMgr gStaticCore;
	return &gStaticCore;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// This is registered with the ANSI atexit() function
//     - don't do anything that might fail
//     - don't do anything that won't complete instantly
//     - don't do anything that requires other objects/resources to exist
static void gsiCoreAtExitShutdown(void)
{
	// delete queue critical section
	GSCoreMgr * aCore = gsiGetStaticCore();
	gsiDeleteCriticalSection(&aCore->mQueueCrit);
	GSI_UNUSED(aCore);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Increment core ref count, initialize the core if necessary
//     - WARNING:  This code is a bit tricky do to multithread issues
void gsCoreInitialize()
{
	GSCoreMgr* aCore = gsiGetStaticCore();

	// Is someone else shutting down the core?
	while(gsi_is_true(aCore->mIsShuttingDown))
		msleep(GSI_CORE_INIT_YIELD_MS); // yield to other thread

	// If we're the first reference, initialize the core
	if (gsiInterlockedIncrement(&aCore->mRefCount) == 1)
	{
		// Are we the first ever?
		if (gsi_is_false(aCore->mIsStaticInitComplete))
		{
			// perform one-time initialization of core critical section
			gsiInitializeCriticalSection(&aCore->mQueueCrit);

			// register function to destroy critical section at program termination
			#ifndef _MANAGED
				atexit(gsiCoreAtExitShutdown);
			#endif

			// one time init completed
			aCore->mIsStaticInitComplete = gsi_true;
		}

		// take the critical section to begin initialization
		// this is necessary in case another thread began shutdown before we incremented ref count
		gsiEnterCriticalSection(&aCore->mQueueCrit);
		gsiLeaveCriticalSection(&aCore->mQueueCrit);

		// wait here if another thread is concurrently shutting down the core
		// we may need to wait a few times if the shutdown does not complete immediately
		while(gsi_is_true(aCore->mIsShuttingDown))
			msleep(GSI_CORE_INIT_YIELD_MS);

		// Setup the task array
		#ifdef GSICORE_DYNAMIC_TASK_LIST
			aCore->mTaskArray = ArrayNew(sizeof(GSTask*), 10, NULL);
			GS_ASSERT(aCore->mTaskArray);
		#else
			memset(aCore->mTaskArray, 0, sizeof(aCore->mTaskArray));
		#endif

		// Init http sdk (ghttp is ref counted)
		ghttpStartup();

		// release other threads that may have blocked during init
		//     - this must be the last thing done at end of init
		aCore->mIsInitialized = gsi_true;
	}
	else
	{
		// Core is already initialized -OR- another thread will initialize the core
		
		// make sure critical section has been initialized
		while(gsi_is_false(aCore->mIsStaticInitComplete))
			msleep(GSI_CORE_INIT_YIELD_MS);

		// take the critical section
		// this is necessary in case another thread began shutdown before we incremented ref count
		gsiEnterCriticalSection(&aCore->mQueueCrit);
		gsiLeaveCriticalSection(&aCore->mQueueCrit);

		// wait for other thread to initial core
		while(gsi_is_false(aCore->mIsInitialized))
			msleep(GSI_CORE_INIT_YIELD_MS); 
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void gsiCoreTaskDispatchCallback(GSTask *theTask, GSTaskResult theResult)
{
	if (theTask->mIsCallbackPending)
	{
		theTask->mIsCallbackPending = 0;
		if (theTask->mCallbackFunc)
			(theTask->mCallbackFunc)(theTask->mTaskData, theResult);
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Return values:
//     GSTaskResult_InProgress - Keep calling gsCoreTaskThink
//     GSTaskResult_Finished   - Task memory freed; task object is now invalid
GSTaskResult gsCoreTaskThink(GSTask* theTask)
{
	GSCoreMgr* aCore = gsiGetStaticCore();
	GSTaskResult aResult = GSTaskResult_None;

	if (theTask == NULL)
		return GSTaskResult_Finished;

	// If the task is running let it think (it may be cancelled and still running)
	if (theTask->mIsRunning && theTask->mThinkFunc)
		aResult = (theTask->mThinkFunc)(theTask->mTaskData);

	// Check for time out
	if ((!theTask->mIsCanceled) && (aResult == GSTaskResult_InProgress))
	{
		if ((theTask->mTimeout != 0) && (current_time() - theTask->mStartTime > theTask->mTimeout))
		{
			// Cancel the task...
			gsiCoreCancelTask(theTask);

			// ...but trigger callback immediately with "Timed Out"
			gsiCoreTaskDispatchCallback(theTask, GSTaskResult_TimedOut);
		}
		//else
		//    continue processing it
	}
	else if (aResult != GSTaskResult_InProgress)
	{
		// Note: This section may be triggered multiple times if the cleanup
		//       function fails.  (possibly due to lack of memory)
		int i=0;
		gsi_bool removeTask = gsi_true;

		// Call the callback if we haven't already
		if (theTask->mIsRunning)
		{
			gsiCoreTaskDispatchCallback(theTask, aResult);
			theTask->mIsRunning = 0;
		}

		// Call Cleanup hook and remove task
		if (theTask->mCleanupFunc)
			removeTask = (theTask->mCleanupFunc)(theTask->mTaskData);

		// Remove the task
		if (gsi_is_true(removeTask))
		{
			gsiEnterCriticalSection(&aCore->mQueueCrit);
			#ifdef GSICORE_DYNAMIC_TASK_LIST
			{
				int len = ArrayLength(aCore->mTaskArray);
				for (i=0; i < len; i++)
				{
					if(*(GSTask**)ArrayNth(aCore->mTaskArray, i) == theTask)
					{
						ArrayRemoveAt(aCore->mTaskArray, i);
						gsifree(theTask);
						break;
					}
				}
			}
			#else
				for (i=0; i < GSICORE_MAXTASKS; i++)
				{
					if (aCore->mTaskArray[i] == theTask)
					{
						aCore->mTaskArray[i] = NULL;
						gsifree(theTask);
						break;
					}
				}
			#endif
			gsiLeaveCriticalSection(&aCore->mQueueCrit);
			return GSTaskResult_Finished;
		}
	}

	// Note: This function should always return InProgress until
	//       the task has been removed from the TaskArray.
	//       The developer may have already received a completed callback
	//       while this continue to return InProgress meaning "still needs to be pumped"
	return GSTaskResult_InProgress;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Optional maximum processing time
//    - Pass in 0 to process each task once
void gsCoreThink(gsi_time theMS)
{
	GSCoreMgr* aCore = gsiGetStaticCore();
	int i=0;
	gsi_time aStartTime = 0;
	gsi_i32 allTasksAreDead = 1;

	if (gsi_is_false(aCore->mIsInitialized))
		return;

	// enter queue critical section
	gsiEnterCriticalSection(&aCore->mQueueCrit);

	// start timing
	aStartTime = current_time();

	// process all tasks in the queue, dispatch callbacks
	// cancelled tasks continue processing until the cancel is acknowledge by the task
	#ifdef GSICORE_DYNAMIC_TASK_LIST
	{
		int len = ArrayLength(aCore->mTaskArray);
		if(len > 0)
			allTasksAreDead = 0;
		for(i=(len-1); i>=0; i--)
		{
			GSTask* task = *(GSTask**)ArrayNth(aCore->mTaskArray, i);
			if(gsi_is_true(task->mAutoThink))
				gsCoreTaskThink(task);
			if (theMS != 0 && (current_time()-aStartTime > theMS))
				break;
		}
	}
	#else
		for (i=0; i<GSICORE_MAXTASKS; i++)
		{
			if (aCore->mTaskArray[i] != NULL)
			{
				allTasksAreDead = 0;

				if (aCore->mTaskArray[i]->mAutoThink == gsi_true)
					gsCoreTaskThink(aCore->mTaskArray[i]);
			}
			// Enough time to process another? (if not, break)
			if (theMS != 0 && (current_time()-aStartTime > theMS))
				break;
		}
	#endif

	// shutting down?
	if (aCore->mIsShuttingDown && allTasksAreDead)
	{
		ghttpCleanup();

#ifdef GSICORE_DYNAMIC_TASK_LIST
        if(aCore->mTaskArray)
        {
            ArrayFree(aCore->mTaskArray);
            aCore->mTaskArray = NULL;
        }
#endif

		aCore->mIsShuttingDown = 0;
	}

	// leave queue critical section
	gsiLeaveCriticalSection(&aCore->mQueueCrit);
	
	GSI_UNUSED(theMS);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void gsCoreShutdown()
{
	GSCoreMgr* aCore = gsiGetStaticCore();
	int i=0;

	// If not initialized, just bail
	if (gsi_is_false(aCore->mIsInitialized))
		return;

	// Take the critical section to prevent anyone from re-initializing while
	// we decide if we need to shutdown
	gsiEnterCriticalSection(&aCore->mQueueCrit);

	// If there are other references, just return
	if (gsiInterlockedDecrement(&aCore->mRefCount)>0)
	{
		gsiLeaveCriticalSection(&aCore->mQueueCrit);
		return;
	}
	else
	{
		// we released the final reference, begin shutdown
		// no other thread will begin using the core until
		// mIsShuttingDown has been set back to false
		aCore->mIsShuttingDown = gsi_true;

		// Cancel all tasks
		#ifdef GSICORE_DYNAMIC_TASK_LIST
		{
			int len = ArrayLength(aCore->mTaskArray);
			for(i=0; i<len; i++)
			{
				gsiCoreCancelTask(*(GSTask**)ArrayNth(aCore->mTaskArray, i));
			}
		}
		#else
			for (i=0; i<GSICORE_MAXTASKS; i++)
			{
				if (aCore->mTaskArray[i] != NULL)
				{
					gsiCoreCancelTask(aCore->mTaskArray[i]);
				}
			}
		#endif
		gsiLeaveCriticalSection(&aCore->mQueueCrit);
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
GSCoreValue gsCoreIsShutdown()
{
	GSCoreMgr* aCore = gsiGetStaticCore();

	if (gsi_is_true(aCore->mIsShuttingDown))
		return GSCore_SHUTDOWN_PENDING;
	if (aCore->mRefCount == 0)
		return GSCore_SHUTDOWN_COMPLETE;

	// The core isn't shutting down, and ref count > 0,
	// therefore the core is in use
	return GSCore_IN_USE;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Adds a GSCoreTask to the execution array
//   - Tasks may come from multiple threads
void gsiCoreExecuteTask(GSTask* theTask, gsi_time theTimeoutMs)
{
	GSCoreMgr* aCore = gsiGetStaticCore();

	// Bail, if the task has already started
	GS_ASSERT(!theTask->mIsRunning);

	// Mark it as started and running
	theTask->mIsCallbackPending = 1;
	theTask->mIsStarted = 1;
	theTask->mIsRunning = 1;
	theTask->mTimeout = theTimeoutMs;
	theTask->mStartTime = current_time();	
	
	// Execute the task
	if (theTask->mExecuteFunc)
		(theTask->mExecuteFunc)(theTask->mTaskData);

	gsiEnterCriticalSection(&aCore->mQueueCrit);
	// add it to the process list
	#ifdef GSICORE_DYNAMIC_TASK_LIST
		ArrayAppend(aCore->mTaskArray, &theTask);
	#else
	{
		int anInsertPos = -1;
		int i=0;
		for (i=0; i<GSICORE_MAXTASKS; i++)
		{
			if (aCore->mTaskArray[i] == NULL)
			{
				anInsertPos = i;
				break;
			}
		}
		GS_ASSERT(anInsertPos != -1); // make sure it got in
		aCore->mTaskArray[anInsertPos] = theTask;
	}
	#endif
	gsiLeaveCriticalSection(&aCore->mQueueCrit);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// cancelling a task is an *async request*
// A task that doesn't support cancelling, such as a blocking socket operation,
// may complete normally even though it was cancelled.
void gsiCoreCancelTask(GSTask* theTask)
{
	GSCoreMgr* aCore = gsiGetStaticCore();

	// Enter critical secction here so the developer 
	// may cancel a task from any thread.  (e.g. The task thread has blocked)
	gsiEnterCriticalSection(&aCore->mQueueCrit);
	if (theTask->mIsRunning && !theTask->mIsCanceled)
	{
		theTask->mIsCanceled = 1;
		if (theTask->mCancelFunc)
			(theTask->mCancelFunc)(theTask->mTaskData);
	}
	gsiLeaveCriticalSection(&aCore->mQueueCrit);
	GSI_UNUSED(aCore);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
GSTask* gsiCoreCreateTask()
{
	GSTask* aTask = (GSTask*)gsimalloc(sizeof(GSTask));
	if (aTask == NULL)
		return NULL;

	memset(aTask, 0, sizeof(GSTask));
	aTask->mAutoThink = gsi_true;
	return aTask;
}
