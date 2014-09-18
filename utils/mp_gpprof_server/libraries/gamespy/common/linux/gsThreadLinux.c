//
// Linux Threading Support (pthreads)
// 
// NOTE: when implementing this make sure the "-lpthread" compiler option is used
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../gsPlatformUtil.h"
#include "../gsPlatformThread.h"
#include "../gsAssert.h"
#include "../gsDebug.h"
#include <pthread.h>

#define _REENTRANT

#define PTHREAD_NO_ERROR 0


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// These functions are unsupported in the current version of the SDK

gsi_u32 gsiInterlockedIncrement(gsi_u32 * value)
{
	//GS_ASSERT_STR(gsi_false, "gsiInterlockIncrement is unsupported for LINUX in the current version of the SDK\n");
	return __sync_add_and_fetch(value, 1);
}

gsi_u32 gsiInterlockedDecrement(gsi_u32 * value)
{
	//GS_ASSERT_STR(gsi_false, "gsiInterlockIncrement is unsupported for LINUX in the current version of the SDK\n");
	return __sync_add_and_fetch(value, -1);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int gsiStartThread(GSThreadFunc func, gsi_u32 theStackSize, void *arg, GSIThreadID * id)
{
	pthread_attr_init(&id->attr);
	pthread_attr_setstacksize(&id->attr, theStackSize);

	if (pthread_create(&id->thread, &id->attr, (void *)func, arg) != PTHREAD_NO_ERROR)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Misc, GSIDebugLevel_WarmError,
			"Failed to create thread\r\n");
		return -1;
	}

	return 0;
}

void gsiCancelThread(GSIThreadID id)
{
	//should i destroy the attributes here?
	pthread_attr_destroy(&id.attr);

	if (pthread_cancel(id.thread) != PTHREAD_NO_ERROR) {
		//there was an error - how should we handle these? or should we?
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Misc, GSIDebugLevel_WarmError,
			"Failed to cancel thread\r\n");
	}
	//free up memory and set to NULL

	gsifree(&id.thread);
}

// This must be called from INSIDE the thread you wish to exit
void gsiExitThread(GSIThreadID id)
{
	// detach the thread so that it knows to free resources upon exit
	pthread_detach(id.thread);
	
	// exit thread to free up resources
	pthread_exit(NULL);
}

void gsiCleanupThread(GSIThreadID id)
{
	// destroy any leftover attributes associated with the thread
	pthread_attr_destroy(&id.attr);
}

gsi_u32 gsiHasThreadShutdown(GSIThreadID id) 
{ 
	// pthreads lacks detection mechanism for this
	GSI_UNUSED(id);
	return 1;
}

void gsiInitializeCriticalSection(GSICriticalSection *theCrit)
{
	if (pthread_mutex_init(theCrit, NULL) != PTHREAD_NO_ERROR)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Misc, GSIDebugLevel_WarmError,
			"Failed to initialize critical section\r\n");
	}
}

void gsiEnterCriticalSection(GSICriticalSection *theCrit)
{
	if (pthread_mutex_lock(theCrit) != PTHREAD_NO_ERROR)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Misc, GSIDebugLevel_WarmError,
			"Failed to lock mutex for entering critical section\r\n");
	}
}

void gsiLeaveCriticalSection(GSICriticalSection *theCrit)
{
	if (pthread_mutex_unlock(theCrit) != PTHREAD_NO_ERROR)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Misc, GSIDebugLevel_WarmError,
			"Failed to unlock mutex for leaving critical section\r\n");
	}
}

void gsiDeleteCriticalSection(GSICriticalSection *theCrit)
{
	if (pthread_mutex_destroy(theCrit) != PTHREAD_NO_ERROR)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Misc, GSIDebugLevel_WarmError,
			"Failed to destroy mutex\r\n");
	}
	theCrit = NULL;
}

GSISemaphoreID gsiCreateSemaphore(gsi_i32 theInitialCount, gsi_i32 theMaxCount, char* theName)
{
	int result;
	GSISemaphoreID semaphore;

	//we can use the default attributes for the mutex by passing NULL
	result = pthread_mutex_init(&semaphore.mLock, NULL);

	if (result != PTHREAD_NO_ERROR)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Misc, GSIDebugLevel_WarmError,
			"Failed to create semaphore\r\n");
	}

	semaphore.mValue = theInitialCount;
	semaphore.mMax = theMaxCount;

	GSI_UNUSED(theName);
	return semaphore;
}

// Waits for -- and signals -- the semaphore
gsi_u32 gsiWaitForSemaphore(GSISemaphoreID theSemaphore, gsi_u32 theTimeoutMs)
{
	gsi_time startTime = current_time();
	gsi_bool infinite = (theTimeoutMs == GSI_INFINITE)?gsi_true:gsi_false;

	do
	{
		//try to lock, if it doesn't then its busy
		if(pthread_mutex_trylock(&theSemaphore.mLock) == PTHREAD_NO_ERROR)
		{
			if(theSemaphore.mValue > 0)
			{
				theSemaphore.mValue--;
				pthread_mutex_unlock(&theSemaphore.mLock);
				return 1;
			}

			pthread_mutex_unlock(&theSemaphore.mLock);
		}

		if(theTimeoutMs != 0)
			msleep(2);

	} while(gsi_is_true(infinite) || ((current_time() - startTime) < theTimeoutMs));

	return 0;
}

// Allow other objects to access the semaphore
void gsiReleaseSemaphore(GSISemaphoreID theSemaphore, gsi_i32 theReleaseCount)
{
	if(pthread_mutex_trylock(&theSemaphore.mLock) != PTHREAD_NO_ERROR)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Misc, GSIDebugLevel_WarmError,
			"Failed to lock semaphore\r\n");

		GSI_UNUSED(theReleaseCount);
	}
	else
	{
		theSemaphore.mValue += theReleaseCount;
		if(theSemaphore.mValue > theSemaphore.mMax)
			theSemaphore.mValue = theSemaphore.mMax;

		pthread_mutex_unlock(&theSemaphore.mLock);
	}
}

void gsiCloseSemaphore(GSISemaphoreID theSemaphore)
{
	if (pthread_mutex_destroy(&theSemaphore.mLock) != PTHREAD_NO_ERROR)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Misc, GSIDebugLevel_WarmError,
			"Failed to destroy semaphore\r\n");
		GSI_UNUSED(theSemaphore);
	}

	//need to free up memory
	gsifree(&theSemaphore.mValue);
	gsifree(&theSemaphore.mMax);
	gsifree(&theSemaphore);
}


