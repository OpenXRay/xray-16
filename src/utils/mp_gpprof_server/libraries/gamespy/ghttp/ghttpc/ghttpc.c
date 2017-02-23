 /*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#include "../../common/gsCommon.h"
#include "../ghttp.h"

#ifdef UNDER_CE
	void RetailOutputA(CHAR *tszErr, ...);
	#define printf RetailOutputA
#elif defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

#define MAX_REQUESTS 20

typedef struct Result
{
	GHTTPBool started;
	gsi_time startTime;
	gsi_time stopTime;
	GHTTPResult result;
} Result;

static int pendingRequests;
static Result results[MAX_REQUESTS];

static gsi_char * stateStrings[] =
{
	_T("Socket Init"),
	_T("Host Lookup"),
	_T("Lookup Pending"),
	_T("Connecting"),
	_T("Securing Session"),
	_T("Sending Request"),
	_T("Posting"),
	_T("Waiting"),
	_T("Receiving Status"),
	_T("Receiving Headers"),
	_T("Receiving File")
};

static gsi_char * resultStrings[] =
{
	_T("GHTTPSuccess"),
	_T("GHTTPOutOfMemory"),
	_T("GHTTPBufferOverflow"),
	_T("GHTTPParseURLFailed"),
	_T("GHTTPHostLookupFailed"),
	_T("GHTTPSocketFailed"),
	_T("GHTTPConnectFailed"),
	_T("GHTTPBadResponse"),
	_T("GHTTPRequestRejected"),
	_T("GHTTPUnauthorized"),
	_T("GHTTPForbidden"),
	_T("GHTTPFileNotFound"),
	_T("GHTTPServerError"),
	_T("GHTTPFileWriteFailed"),
	_T("GHTTPFileReadFailed"),
	_T("GHTTPFileIncomplete"),
	_T("GHTTPFileToBig"),
	_T("GHTTPEncryptionError"),
	_T("GHTTPRequestCancelled")
};


#ifdef __MWERKS__  // CodeWarrior will warn if function not prototyped
int test_main(int argc, char **argv);
#endif

#ifdef GSI_COMMON_DEBUG
	static void DebugCallback(GSIDebugCategory theCat, GSIDebugType theType,
	                          GSIDebugLevel theLevel, const char * theTokenStr,
	                          va_list theParamList)
	{
		GSI_UNUSED(theLevel);

		printf("[%s][%s] ", 
				gGSIDebugCatStrings[theCat], 
				gGSIDebugTypeStrings[theType]);
		vprintf(theTokenStr, theParamList);

#if defined(_WIN32) && !defined(_XBOX)
		{
			static char buffer[4098];
			sprintf(buffer, "[%s][%s] ", 
					gGSIDebugCatStrings[theCat], 
					gGSIDebugTypeStrings[theType]);
			OutputDebugString(buffer);
			vsprintf(buffer, theTokenStr, theParamList);
			OutputDebugString(buffer);
		}
#endif
	}
#endif

static GHTTPBool CompletedCallback
(
	GHTTPRequest request,
	GHTTPResult result,
	char * buffer,
	GHTTPByteCount bufferLen,
	void * param
)
{
	int index = (int)param;

	pendingRequests--;

	// Save off the result.
	///////////////////////
	results[index].result = result;

	// Save the time.
	/////////////////
	results[index].stopTime = current_time();

	// Check the result and print out some info.
	////////////////////////////////////////////
	if(result == GHTTPSuccess)
		_tprintf(_T("%d finished\n"), index);
	else
		_tprintf(_T("%d failed: %s\n"), index, resultStrings[result]);

	// Don't free this buffer, its from the stack.
	//////////////////////////////////////////////
	if(index == 1)
		return GHTTPFalse;

	// Free the buffer (if there is one).
	/////////////////////////////////////
	GSI_UNUSED(request);
	GSI_UNUSED(buffer);
	GSI_UNUSED(bufferLen);

	return GHTTPTrue;
}

static void ProgressCallback
(
	GHTTPRequest request,
	GHTTPState state,
	const char * buffer,
	GHTTPByteCount bufferLen,
	GHTTPByteCount bytesReceived,
	GHTTPByteCount totalSize,
	void * param
)
{
	int index = (int)param;

	// Show the current state.
	//////////////////////////
	_tprintf(_T("%d state: %s"), index, stateStrings[state]);

	// If we're receiving the file, show the progress.
	//////////////////////////////////////////////////
	if(state == GHTTPReceivingFile)
	{
		// Display based on if we know the total size.
		//////////////////////////////////////////////
		if(totalSize != -1)
			_tprintf(_T(" (%d / %d bytes)\n"), bytesReceived, totalSize);
		else
			_tprintf(_T(" (%d bytes)\n"), bytesReceived);
	}
	else
		_tprintf(_T("\n"));
		
	GSI_UNUSED(request);
	GSI_UNUSED(buffer);
	GSI_UNUSED(bufferLen);
}

static void CheckRequest(GHTTPRequest request, int index)
{
	assert(index < MAX_REQUESTS);
	results[index].started = (request < 0)?GHTTPFalse:GHTTPTrue;
	if(results[index].started)
		results[index].startTime = current_time();
}

int test_main(int argc, char **argv)
{
	int i;
	static char buffer[10000] = "";
	GHTTPRequest request;
	int numRequests;

#ifdef GSI_COMMON_DEBUG
	// Define GSI_COMMON_DEBUG if you want to view the SDK debug output
	// Set the SDK debug log file, or set your own handler using gsSetDebugCallback
	//gsSetDebugFile(stdout); // output to console
	gsSetDebugCallback(DebugCallback);

	// Set some debug levels
	gsSetDebugLevel(GSIDebugCat_All, GSIDebugType_All, GSIDebugLevel_Hardcore);
	//gsSetDebugLevel(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Verbose);   // Show Detailed data on network traffic
	//gsSetDebugLevel(GSIDebugCat_App, GSIDebugType_All, GSIDebugLevel_Hardcore);  // Show All app comment
#endif

	// get a file
	request = ghttpGet(
		_T("http://www.gamespy.net/images/dev_serv_main.jpg"),
		GHTTPFalse,
		CompletedCallback,
		(void *)pendingRequests);
	CheckRequest(request, pendingRequests);
	pendingRequests++;

	// put a file into our own buffer, with progress updates
	// also gets redirected
	request = ghttpGetEx(
		_T("http://google.com/intl/en_ALL/images/logo.gif"),
		NULL,
		buffer, sizeof(buffer),
		NULL,
		GHTTPFalse,
		GHTTPFalse,
		ProgressCallback,
		CompletedCallback,
		(void *)pendingRequests);
	CheckRequest(request, pendingRequests);
	pendingRequests++;

	// download a file
#if !defined(NOFILE)
	request = ghttpSave(
		_T("http://www.gamespy.net/images/dev_serv_logo.jpg"),
		_T("logo.jpg"),
		GHTTPFalse,
		CompletedCallback,
		(void *)pendingRequests);
	CheckRequest(request, pendingRequests);
	pendingRequests++;
#endif

	// stream a page
	request = ghttpStreamEx(
		_T("http://www.gamespy.net"),
		NULL,
		NULL,
		GHTTPFalse,
		GHTTPFalse,
		ProgressCallback,
		CompletedCallback,
		(void *)pendingRequests);
	CheckRequest(request, pendingRequests);
	pendingRequests++;

	// get a header
	request = ghttpHead(
		_T("http://sdkdev.gamespy.com/games/st_ladder/web/index.html"),
		GHTTPFalse,
		CompletedCallback,
		(void *)pendingRequests);
	CheckRequest(request, pendingRequests);
	pendingRequests++;

	// stream a secure page	
	request = ghttpStreamEx(
#if defined(_REVOLUTION)
		_T("https://mariokartwii.race.gs.nintendowifi.net/RaceService/test.txt"),
#else
		_T("https://www.gamespyid.com/"),
#endif
		NULL,
		NULL,
		GHTTPFalse,
		GHTTPFalse,
		ProgressCallback,
		CompletedCallback,
		(void *)pendingRequests);

	//if(!IS_GHTTP_ERROR(request))
	//	ghttpSetRequestEncryptionEngine(request, GHTTPEncryptionEngine_GameSpy);
	CheckRequest(request, pendingRequests);
	pendingRequests++;

	// store the number of requests
	numRequests = pendingRequests;

	do
	{
		ghttpThink();
		msleep(20);
	}
	while(pendingRequests);

	ghttpCleanup();

	_tprintf(_T("Results:\n"));
	for(i = 0 ; i < numRequests ; i++)
	{
		if(results[i].started == GHTTPFalse)
			_tprintf(_T("%d: Failed to start\n"), i);
		else
			_tprintf(_T("%d: %s [%dms]\n"), i, resultStrings[results[i].result], results[i].stopTime - results[i].startTime);
	}

	GSI_UNUSED(argc);
	GSI_UNUSED(argv);

	return 0;
}
