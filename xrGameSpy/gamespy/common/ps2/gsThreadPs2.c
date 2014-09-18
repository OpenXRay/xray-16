///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../gscommon.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u32 gsiInterlockedIncrement(gsi_u32 * value)
{
	int interrupt = DI();
	int ret = ++(*value);
	if (interrupt)
		EI();

	// return "ret" rather than "value" here b/c
	// value may be modified by another thread 
	// before we can return it
	return ret;
}

gsi_u32 gsiInterlockedDecrement(gsi_u32 * value)
{
	int interrupt = DI();
	int ret = --(*value);
	if (interrupt)
		EI();

	// return "ret" rather than "value" here b/c
	// value may be modified by another thread 
	// before we can return it
	return ret;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void gsiInitializeCriticalSection(GSICriticalSection *theCrit) 
{
	theCrit->mSemaphore = gsiCreateSemaphore(1, 1, NULL); 
	theCrit->mOwnerThread = 0;
	theCrit->mEntryCount = 0;
}
void gsiEnterCriticalSection(GSICriticalSection *theCrit) 
{ 
	// If we're not already in it, wait for it
	if (GetThreadId() != theCrit->mOwnerThread)
	{
		gsiWaitForSemaphore(theCrit->mSemaphore, 0);
		theCrit->mOwnerThread = GetThreadId();
	}

	// Increment entry count
	theCrit->mEntryCount++;
}
void gsiLeaveCriticalSection(GSICriticalSection *theCrit)
{ 
	// We must be the owner? (assert?)
	if (GetThreadId() != theCrit->mOwnerThread)
	{
		assert(GetThreadId() == theCrit->mOwnerThread);
		return;
	}

	// Release semaphore
	theCrit->mEntryCount--;
	if (theCrit->mEntryCount == 0)
	{
		theCrit->mOwnerThread = 0;
		gsiReleaseSemaphore(theCrit->mSemaphore, 1);     
	}
}

void gsiDeleteCriticalSection(GSICriticalSection *theCrit) 
{ 
	gsiCloseSemaphore(theCrit->mSemaphore);       
}

gsi_u32 gsiHasThreadShutdown(GSIThreadID theThreadID) 
{ 
	struct ThreadParam aStatus;
	ReferThreadStatus(theThreadID, &aStatus);
	if (aStatus.status == THS_DORMANT)
		return 1; // dead
	else
		return 0; // still kicking;
}

GSISemaphoreID gsiCreateSemaphore(gsi_i32 theInitialCount, gsi_i32 theMaxCount, char* theName)
{
	struct SemaParam aParam;
	int aSemaphore = 0;

	aParam.initCount = theInitialCount;
	aParam.maxCount = theMaxCount;
	
	aSemaphore = CreateSema(&aParam);
	if (aSemaphore < 0)
	{
		gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Misc, GSIDebugLevel_WarmError,
			"Failed to create semaphore\r\n");
	}
	
	GSI_UNUSED(theName);
	
	return aSemaphore;
}

gsi_u32 gsiWaitForSemaphore(GSISemaphoreID theSemaphore, gsi_u32 theTimeoutMs)
{
	int result = WaitSema(theSemaphore);
	return (gsi_u32)result;

	GSI_UNUSED(theTimeoutMs);
}

void gsiReleaseSemaphore(GSISemaphoreID theSemaphore, gsi_i32 theReleaseCount)
{
	while (theReleaseCount-- > 0)
		SignalSema(theSemaphore);
	//ReleaseSemaphore(theSemaphore, theReleaseCount, NULL);
}

void gsiCloseSemaphore(GSISemaphoreID theSemaphore)
{
	DeleteSema(theSemaphore);
}

int gsiStartThread(GSThreadFunc func,  gsi_u32 theStackSize, void *arg, GSIThreadID *id)
{
	const unsigned int stackSize = theStackSize;
	const int threadPriority = 3;
	struct ThreadParam param;
	void * stack;
	int threadID;

	// allocate a stack
	stack = gsimemalign(16, stackSize);
	if(!stack)
		return -1;

	// setup the thread parameters
	param.entry = func;
	param.stack = stack;
	param.stackSize = (int)stackSize;
	param.gpReg = &_gp;
	param.initPriority = threadPriority;

	// create the thread
	threadID = CreateThread(&param);
	if(threadID == -1)
	{
		gsifree(stack);
		return -1;
	}

	// start the thread
	if(StartThread(threadID, arg) == -1)
	{
		DeleteThread(threadID);
		gsifree(stack);
		return -1;
	}

	// store the id
	*id = threadID;

	// Note:  This was added to prevent PS2 lockups when starting multiple threads
	//        The PS2 would block for approx 5 seconds
	msleep(1);

	return 0;
}

void gsiCancelThread(GSIThreadID id)
{
	void* aStack = NULL;

	// get the stack ptr
	struct ThreadParam aThreadParam;
	ReferThreadStatus(id, &aThreadParam);
	aStack = (void*)aThreadParam.stack;

	// terminate the thread
	TerminateThread(id);

	// delete the thread
	DeleteThread(id);

	//free the stack
	gsifree(aStack);
}

// This must be called from INSIDE the thread you wish to exit
void gsiExitThread(GSIThreadID id)
{
    // TODO: does PS2 need to explicitly EXIT a thread like win32/linux?
	GSI_UNUSED(id);
}

void gsiCleanupThread(GSIThreadID id)
{
	// same as cancel (terminates just to be sure)
	gsiCancelThread(id);
}