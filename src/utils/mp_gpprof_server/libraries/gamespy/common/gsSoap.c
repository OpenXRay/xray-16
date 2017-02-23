///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// gSOAP Glue
#include "gsCore.h"
#include "gsSoap.h"
#include "gsPlatformThread.h"
#include "gsXML.h"
#include "../ghttp/ghttpASCII.h"


// GAMESPY DEVELOPERS ->  Use gsiExecuteSoap


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Soap task delegates
static void gsiSoapTaskExecute(void* theTask);
static void gsiSoapTaskCallback(void* theTask, GSTaskResult theResult);
static void gsiSoapTaskCancel(void* theTask);
static gsi_bool gsiSoapTaskCleanup(void* theTask);
static GSTaskResult gsiSoapTaskThink(void* theTask);

// Http triggered callbacks (don't take action now, wait for task callbacks)
static GHTTPBool gsiSoapTaskHttpCompletedCallback(GHTTPRequest request, GHTTPResult result, 
											 char * buffer, GHTTPByteCount bufferLen, 
											 void * param);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Execute a soap function (this should be the only call made from other SDKs)
GSSoapTask* gsiExecuteSoap(const char* theURL, const char* theService,
					 GSXmlStreamWriter theRequestSoap, GSSoapCallbackFunc theCallbackFunc, 
					 void* theUserData)
{
	GSSoapTask* aSoapTask = NULL;
	GSTask* aCoreTask = NULL;

	aSoapTask = (GSSoapTask*)gsimalloc(sizeof(GSSoapTask));
	if (aSoapTask == NULL)
		return NULL; // out of memory
	
	aSoapTask->mCallbackFunc = theCallbackFunc;
	aSoapTask->mCustomFunc   = NULL;
	aSoapTask->mURL          = theURL;
	aSoapTask->mService      = theService;
	aSoapTask->mRequestSoap  = theRequestSoap;
	aSoapTask->mPostData     = NULL;
	aSoapTask->mResponseSoap = NULL;
	aSoapTask->mResponseBuffer = NULL;
	aSoapTask->mUserData     = theUserData;
	aSoapTask->mRequestResult= (GHTTPResult)0;
	aSoapTask->mCompleted    = gsi_false;

	aCoreTask = gsiCoreCreateTask();
	if (aCoreTask == NULL)
	{
		gsifree(aSoapTask);
		return NULL; // out of memory
	}

	aCoreTask->mCallbackFunc = gsiSoapTaskCallback;
	aCoreTask->mExecuteFunc  = gsiSoapTaskExecute;
	aCoreTask->mThinkFunc    = gsiSoapTaskThink;
	aCoreTask->mCleanupFunc  = gsiSoapTaskCleanup;
	aCoreTask->mCancelFunc   = gsiSoapTaskCancel;
	aCoreTask->mTaskData     = (void*)aSoapTask;

	aSoapTask->mCoreTask = aCoreTask;

	gsiCoreExecuteTask(aCoreTask, 0);

	return aSoapTask;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Execute a soap function with a GSSoapCustomFunc that can access the soap
// structure prior to execution. This allows the client to set DIME
// attachments. (The GSSoapCustomFunc parameter could be added to
// gsiExecuteSoap itself as long as existing client code is updated)
GSSoapTask* gsiExecuteSoapCustom(const char* theURL, const char* theService,
					 GSXmlStreamWriter theRequestSoap, GSSoapCallbackFunc theCallbackFunc, 
					 GSSoapCustomFunc theCustomFunc, void* theUserData)
{
	GSSoapTask* aSoapTask = NULL;
	GSTask* aCoreTask = NULL;

	aSoapTask = (GSSoapTask*)gsimalloc(sizeof(GSSoapTask));
	aSoapTask->mCallbackFunc = theCallbackFunc;
	aSoapTask->mCustomFunc   = theCustomFunc;
	aSoapTask->mURL          = theURL;
	aSoapTask->mService      = theService;
	aSoapTask->mRequestSoap  = theRequestSoap;
	aSoapTask->mPostData     = NULL;
	aSoapTask->mResponseSoap = NULL;
	aSoapTask->mResponseBuffer = NULL;
	aSoapTask->mUserData     = theUserData;
	aSoapTask->mRequestResult= (GHTTPResult)0;
	aSoapTask->mCompleted    = gsi_false;
	
	aCoreTask = gsiCoreCreateTask();
	aCoreTask->mCallbackFunc = gsiSoapTaskCallback;
	aCoreTask->mExecuteFunc  = gsiSoapTaskExecute;
	aCoreTask->mThinkFunc    = gsiSoapTaskThink;
	aCoreTask->mCleanupFunc  = gsiSoapTaskCleanup;
	aCoreTask->mCancelFunc   = gsiSoapTaskCancel;
	aCoreTask->mTaskData     = (void*)aSoapTask;

	aSoapTask->mCoreTask = aCoreTask;

	gsiCoreExecuteTask(aCoreTask, 0);

	return aSoapTask;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Cancels a soap task.  
//    - Because of network race conditions, the task may complete before it 
//      can be cancelled. If this happens, the task callback will be triggered
//      with status GHTTPRequestCancelled and the result data will be discarded.
void gsiCancelSoap(GSSoapTask * theTask)
{
	GS_ASSERT(theTask != NULL);

	// Still in progress? cancel it!
	if (gsi_is_false(theTask->mCompleted))
		gsiCoreCancelTask(theTask->mCoreTask);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
				    //////////  HTTP CALLBACKS  //////////

static GHTTPBool gsiSoapTaskHttpCompletedCallback(GHTTPRequest request, GHTTPResult result, 
											 char * buffer, GHTTPByteCount bufferLen, 
											 void * param)
{
	gsi_bool parseResult = gsi_false;

	GSSoapTask* aSoapTask = (GSSoapTask*)param;
	aSoapTask->mRequestResult = result;
	aSoapTask->mCompleted = gsi_true;
	aSoapTask->mResponseBuffer = buffer;

	if (result == GHTTPSuccess)
	{
		aSoapTask->mResponseSoap = gsXmlCreateStreamReader();
		if (aSoapTask->mResponseSoap == NULL)
		{
			// OOM!
			aSoapTask->mRequestResult = GHTTPOutOfMemory;
		}
		else
		{
			parseResult = gsXmlParseBuffer(aSoapTask->mResponseSoap, buffer, (int)bufferLen);
			if (gsi_is_false(parseResult))
			{
				// Todo: handle multiple error conditions
				aSoapTask->mRequestResult = GHTTPBadResponse;
			}
		}
	}

	GSI_UNUSED(request);

	return GHTTPFalse; // don't let http free the buffer
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
				//////////  SOAP EXECUTE TASK  //////////


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Checks to see if the soap task has completed
//   - return GSTaskResult_InProgress for "keep checking"
//   - return anything else           for "finished - trigger callback and delete"
static GSTaskResult gsiSoapTaskThink(void* theTask)
{
	// is the request still processing?
	GSSoapTask* aSoapTask = (GSSoapTask*)theTask;
	if (gsi_is_true(aSoapTask->mCompleted))
		return GSTaskResult_Finished;
	else
	{
		ghttpRequestThink(aSoapTask->mRequestId);
		return GSTaskResult_InProgress;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Spawns the soap thread and begins execution
static void gsiSoapTaskExecute(void* theTask)
{
	GSSoapTask* aSoapTask = (GSSoapTask*)theTask;
	//int threadID = 0;

	// make sure we aren't reusing a task without first resetting it
	GS_ASSERT(gsi_is_false(aSoapTask->mCompleted));

	aSoapTask->mPostData = ghttpNewPost();
	if (aSoapTask->mPostData == NULL)
	{
		// OOM: abort task
		aSoapTask->mCompleted = gsi_true;
		aSoapTask->mRequestResult = GHTTPOutOfMemory;
		return;
	}

	ghttpPostSetAutoFree(aSoapTask->mPostData, GHTTPFalse);
	ghttpPostAddXml(aSoapTask->mPostData, aSoapTask->mRequestSoap);

	// Allow client to further configure soap object if desired
	if (aSoapTask->mCustomFunc != NULL)
		(aSoapTask->mCustomFunc)(aSoapTask->mPostData, aSoapTask->mUserData);


	aSoapTask->mRequestId = ghttpGetExA(aSoapTask->mURL, aSoapTask->mService, 
		NULL, 0, aSoapTask->mPostData, GHTTPFalse, GHTTPFalse, NULL,
		gsiSoapTaskHttpCompletedCallback, (void*)aSoapTask);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Called when the soap task needs to be cancelled
static void gsiSoapTaskCancel(void* theTask)
{
	GSSoapTask * soapTask = (GSSoapTask*)theTask;
	if (gsi_is_false(soapTask->mCompleted))
	{
		if (soapTask->mRequestId >= 0)
			ghttpCancelRequest(soapTask->mRequestId);
		soapTask->mRequestResult = GHTTPRequestCancelled;
		soapTask->mCompleted = gsi_true;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Called when the soap task completes or is cancelled/timed out
static void gsiSoapTaskCallback(void* theTask, GSTaskResult theResult)
{
	// Call the developer callback
	GSSoapTask* aSoapTask = (GSSoapTask*)theTask;

	(aSoapTask->mCallbackFunc)(aSoapTask->mRequestResult, aSoapTask->mRequestSoap, 
		aSoapTask->mResponseSoap, aSoapTask->mUserData);

	GSI_UNUSED(theResult);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// After the soap call has completed, launch a separate cleanup event (see comments)
static gsi_bool gsiSoapTaskCleanup(void *theTask)
{
	GSSoapTask* aSoapTask = (GSSoapTask*)theTask;

	if (aSoapTask->mResponseSoap != NULL)
		gsXmlFreeReader(aSoapTask->mResponseSoap);
	if (aSoapTask->mResponseBuffer != NULL)
		gsifree(aSoapTask->mResponseBuffer);
	if (aSoapTask->mPostData != NULL)
		ghttpFreePost(aSoapTask->mPostData); // this also frees the request soap xml
	gsifree(aSoapTask);
	
	return gsi_true;
}
