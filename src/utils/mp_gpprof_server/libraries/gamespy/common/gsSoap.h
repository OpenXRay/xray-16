///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __SOAP_H__
#define __SOAP_H__


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gsCommon.h"
#include "gsCore.h"

#include "../ghttp/ghttp.h"

#if defined(__cplusplus)
extern "C"
{
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
typedef void(*GSSoapCallbackFunc)(GHTTPResult theHTTPResult, GSXmlStreamWriter theRequest, GSXmlStreamReader theResponse, void *theUserData);
typedef void(*GSSoapCustomFunc)(GHTTPPost theSoap, void* theUserData);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
typedef struct 
{
	GSSoapCallbackFunc mCallbackFunc;
	GSSoapCustomFunc mCustomFunc;
	const char *mURL;
	const char *mService;

	GSXmlStreamWriter mRequestSoap;
	GSXmlStreamReader mResponseSoap;

	char *    mResponseBuffer; // so we can free it later
	GHTTPPost mPostData; // so we can free it later

	void *   mUserData;
	GSTask * mCoreTask;

	GHTTPRequest mRequestId;
	GHTTPResult  mRequestResult;
	gsi_bool     mCompleted;
} GSSoapTask;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Execute a soap call (Uses GameSpy core object)
GSSoapTask* gsiExecuteSoap(const char *theURL, const char *theService,
					 GSXmlStreamWriter theSoapData, GSSoapCallbackFunc theCallbackFunc,
					 void *theUserData);

// Alternate version with GSSoapCustomFunc parameter allows client access
// to soap object to set DIME attachments
GSSoapTask* gsiExecuteSoapCustom(const char* theURL, const char* theService, 
					 GSXmlStreamWriter theSoapData, GSSoapCallbackFunc theCallbackFunc, 
					 GSSoapCustomFunc theCustomFunc, void* theUserData);


void gsiCancelSoap(GSSoapTask * theTask);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
} // extern "C"
#endif

#endif // __SOAP_H__
