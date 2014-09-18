///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../gsPlatformThread.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u32 gsiInterlockedIncrement(gsi_u32 * value)
{
	OSIntrMode state = OS_DisableInterrupts_IrqAndFiq();
	gsi_u32 ret = ++(*value);
	OS_RestoreInterrupts_IrqAndFiq(state);

	// return "ret" rather than "value" here b/c
	// value may be modified by another thread 
	// before we can return it
	return ret;
}

gsi_u32 gsiInterlockedDecrement(gsi_u32 * value)
{
	OSIntrMode state = OS_DisableInterrupts_IrqAndFiq();
	gsi_u32 ret = --(*value);
	OS_RestoreInterrupts_IrqAndFiq(state);

	// return "ret" rather than "value" here b/c
	// value may be modified by another thread 
	// before we can return it
	return ret;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int gsiStartThread(GSThreadFunc aThreadFunc, gsi_u32 theStackSize, void *arg, GSIThreadID* theThreadIdOut)
{
	if(theStackSize & 0x3)
	{
		theStackSize += 0x4;
		theStackSize &= ~0x3;
	}

	theThreadIdOut->mStack = gsimemalign(4, theStackSize);

	OS_CreateThread(&theThreadIdOut->mThread, aThreadFunc, arg, theThreadIdOut->mStack, theStackSize, 15);
	OS_WakeupThreadDirect(&theThreadIdOut->mThread);

	return 0;
}

void gsiCancelThread(GSIThreadID theThreadID)
{
	OS_DestroyThread(&theThreadID.mThread);
	if(theThreadID.mStack)
	{
		gsifree(theThreadID.mStack);
		theThreadID.mStack = NULL;
	}
}

void gsiCleanupThread(GSIThreadID theThreadID)
{
	OS_DestroyThread(&theThreadID.mThread);
	if(theThreadID.mStack)
	{
		gsifree(theThreadID.mStack);
		theThreadID.mStack = NULL;
	}
}

gsi_u32 gsiHasThreadShutdown(GSIThreadID theThreadID)
{
	BOOL shutdown = OS_IsThreadTerminated(&theThreadID.mThread);

	if(shutdown == TRUE)
		return 1;
	return 0;
}

void gsiInitializeCriticalSection(GSICriticalSection *theCrit)
{
	OS_InitMutex(theCrit);
}

void gsiEnterCriticalSection(GSICriticalSection *theCrit)
{
	OS_LockMutex(theCrit);
}

void gsiLeaveCriticalSection(GSICriticalSection *theCrit)
{
	OS_UnlockMutex(theCrit);
}

void gsiDeleteCriticalSection(GSICriticalSection *theCrit)
{
	GSI_UNUSED(theCrit);
}

GSISemaphoreID gsiCreateSemaphore(gsi_i32 theInitialCount, gsi_i32 theMaxCount, char* theName)
{
	GSISemaphoreID semaphore;

	OS_InitMutex(&semaphore.mLock);

	semaphore.mValue = theInitialCount;
	semaphore.mMax = theMaxCount;

	GSI_UNUSED(theName);

	return semaphore;
}

gsi_u32 gsiWaitForSemaphore(GSISemaphoreID theSemaphore, gsi_u32 theTimeoutMs)
{
	gsi_time startTime = current_time();
	gsi_bool infinite = (theTimeoutMs == GSI_INFINITE)?gsi_true:gsi_false;

	do
	{
		if(OS_TryLockMutex(&theSemaphore.mLock) == TRUE)
		{
			if(theSemaphore.mValue > 0)
			{
				theSemaphore.mValue--;
				OS_UnlockMutex(&theSemaphore.mLock);
				return 1;
			}

			OS_UnlockMutex(&theSemaphore.mLock);
		}

		if(theTimeoutMs != 0)
			msleep(2);

	} while(gsi_is_true(infinite) || ((current_time() - startTime) < theTimeoutMs));

	return 0;
}

void gsiReleaseSemaphore(GSISemaphoreID theSemaphore, gsi_i32 theReleaseCount)
{
	OS_LockMutex(&theSemaphore.mLock);

	theSemaphore.mValue += theReleaseCount;
	if(theSemaphore.mValue > theSemaphore.mMax)
		theSemaphore.mValue = theSemaphore.mMax;

	OS_UnlockMutex(&theSemaphore.mLock);
}

void gsiCloseSemaphore(GSISemaphoreID theSemaphore)
{
	GSI_UNUSED(theSemaphore);
}
