 /*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2001 GameSpy Industries, Inc

18002 Skypark Circle
Irvine, California 92614
949.798.4200 (Tel)
949.798.4299 (Fax)
devsupport@gamespy.com
*/

#include "ghttpMain.h"
#include "ghttpASCII.h"
#include "ghttpConnection.h"
#include "ghttpCallbacks.h"
#include "ghttpProcess.h"
#include "ghttpPost.h"
#include "ghttpCommon.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Ascii versions which must be available even in the unicode build
GHTTPRequest ghttpGetExA(const char * URL, const char * headers, char * buffer, int bufferSize, GHTTPPost post, GHTTPBool throttle, GHTTPBool blocking, ghttpProgressCallback progressCallback, ghttpCompletedCallback completedCallback, void * param);
GHTTPRequest ghttpSaveExA(const char * URL, const char * filename, const char * headers, GHTTPPost post, GHTTPBool throttle, GHTTPBool blocking, ghttpProgressCallback progressCallback, ghttpCompletedCallback completedCallback, void * param);
GHTTPRequest ghttpStreamExA(const char * URL, const char * headers, GHTTPPost post, GHTTPBool throttle, GHTTPBool blocking, ghttpProgressCallback progressCallback, ghttpCompletedCallback completedCallback, void * param);
GHTTPRequest ghttpHeadExA(const char * URL, const char * headers, GHTTPBool throttle, GHTTPBool blocking, ghttpProgressCallback progressCallback, ghttpCompletedCallback completedCallback, void * param);
GHTTPRequest ghttpPostExA(const char * URL, const char * headers, GHTTPPost post, GHTTPBool throttle, GHTTPBool blocking, ghttpProgressCallback progressCallback, ghttpCompletedCallback completedCallback, void * param);



// Reference count.
///////////////////
static int ghiReferenceCount;

// Called right before callback is called.
// Sets result based on response status code.
/////////////////////////////////////////////
static void ghiHandleStatus
(
	GHIConnection * connection
)
{
	// Check the status code.
	/////////////////////////
	switch(connection->statusCode / 100)
	{
	case 1:  // Informational.
		return;
	case 2:  // Successful.
		return;
	case 3:  // Redirection.
		return;
	case 4:  // Client Error.
		switch(connection->statusCode)
		{
		case 401:
			connection->result = GHTTPUnauthorized;
			break;
		case 403:
			connection->result = GHTTPForbidden;
			break;
		case 404:
		case 410:
			connection->result = GHTTPFileNotFound;
			break;
		default:
			connection->result = GHTTPRequestRejected;
			break;
		}
		return;
	case 5:  // Internal Server Error.
		connection->result = GHTTPServerError;
		return;
	}
}

// Processes a single connection based on its state.
// Returns true if the connection is finished.
////////////////////////////////////////////////////
static GHTTPBool ghiProcessConnection
(
	GHIConnection * connection
)
{
	GHTTPBool completed;

	assert(connection);
	assert(ghiRequestToConnection(connection->request) == connection);

	// Don't process if already processing this connection.
	// Happens if, for example, ghttpThink is called from a callback.
	/////////////////////////////////////////////////////////////////
	if(connection->processing)
		return GHTTPFalse;

	// We're now processing.
	////////////////////////
	connection->processing = GHTTPTrue;

	// Process based on state.
	// else-if is not used so that if one ghiDo*()
	// finishes the one after it can start.
	//////////////////////////////////////////////

	if(connection->state == GHTTPSocketInit)
		ghiDoSocketInit(connection);
	if(connection->state == GHTTPHostLookup)
		ghiDoHostLookup(connection);
	if(connection->state == GHTTPLookupPending)
		ghiDoLookupPending(connection);
	if(connection->state == GHTTPConnecting)
		ghiDoConnecting(connection);
	if(connection->state == GHTTPSecuringSession)
		ghiDoSecuringSession(connection);
	if(connection->state == GHTTPSendingRequest)
		ghiDoSendingRequest(connection);
	if(connection->state == GHTTPPosting)
		ghiDoPosting(connection);
	if(connection->state == GHTTPWaiting)
		ghiDoWaiting(connection);
	if(connection->state == GHTTPReceivingStatus)
		ghiDoReceivingStatus(connection);
	if(connection->state == GHTTPReceivingHeaders)
		ghiDoReceivingHeaders(connection);
	if(connection->state == GHTTPReceivingFile)
		ghiDoReceivingFile(connection);

	// Check for a redirect.
	////////////////////////
	if(connection->redirectURL)
		ghiRedirectConnection(connection);

	// Grab completed before we possibly free it.
	/////////////////////////////////////////////
	completed = connection->completed;
	
	// Graceful shutdown support.  
	// Close connection when there is no more data
	if (connection->result == GHTTPRequestCancelled && !connection->completed && !CanReceiveOnSocket(connection->socket))
	{
		connection->completed = GHTTPTrue;
	}

	// Is it finished?
	//////////////////
	if(connection->completed)
	{
		// Set result based on status code.
		///////////////////////////////////
		ghiHandleStatus(connection);

		// If we're saving to file, close it before the callback.
		/////////////////////////////////////////////////////////
#ifndef NOFILE
		if(connection->saveFile)
		{
			fclose(connection->saveFile);
			connection->saveFile = NULL;
		}
#endif
		// Log buffer data
		ghiLogResponse(connection->getFileBuffer.data, connection->getFileBuffer.len);

		// Call the callback.
		/////////////////////
		ghiCallCompletedCallback(connection);

		// Free it.
		///////////
		ghiFreeConnection(connection);
	}
	else
	{
		// Done processing. This is in the else,
		// because we don't want to set it if the
		// connection has already been freed.
		/////////////////////////////////////////
		connection->processing = GHTTPFalse;
	}

	return completed;
}

void ghttpStartup
(
	void
)
{
	// This will just return if we haven't created the lock yet.
	////////////////////////////////////////////////////////////
	ghiLock();

	// One more startup.
	////////////////////
	ghiReferenceCount++;

	// Check if we are the first.
	/////////////////////////////
	if(ghiReferenceCount == 1)
	{
		// Create the lock.
		///////////////////
		ghiCreateLock();

		// Set some defaults.
		/////////////////////
		ghiThrottleBufferSize = GHI_DEFAULT_THROTTLE_BUFFER_SIZE;
		ghiThrottleTimeDelay = GHI_DEFAULT_THROTTLE_TIME_DELAY;
	}
	else
	{
		// Unlock the lock.
		///////////////////
		ghiUnlock();
	}
}

void ghttpCleanup
(
	void
)
{
	// Lockdown for cleanup.
	////////////////////////
	ghiLock();

	// One less.
	////////////
	ghiReferenceCount--;

	// Should we cleanup?
	/////////////////////
	if(!ghiReferenceCount)
	{
		// Cleanup the connections.
		///////////////////////////
		ghiCleanupConnections();

		// Cleanup proxy.
		/////////////////
		if(ghiProxyAddress)
		{
			gsifree(ghiProxyAddress);
			ghiProxyAddress = NULL;
		}

		// Unlock the lock before freeing it.
		/////////////////////////////////////
		ghiUnlock();

		// Free the lock.
		/////////////////
		ghiFreeLock();
	}
	else
	{
		// Unlock our lock.
		///////////////////
		ghiUnlock();
	}
}

GHTTPRequest ghttpGetA
(
	const char * URL,
	GHTTPBool blocking,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	return ghttpGetExA(URL, NULL, NULL, 0, NULL, GHTTPFalse, blocking, NULL, completedCallback, param);
}
#ifdef GSI_UNICODE
GHTTPRequest ghttpGetW
(
	const unsigned short * URL,
	GHTTPBool blocking,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	char URL_A[1024];

	assert(URL != NULL);
	UCS2ToAsciiString(URL, (char*)URL_A);
	return ghttpGetA(URL_A, blocking, completedCallback, param);
}
#endif

GHTTPRequest ghttpGetExA
(
	const char * URL,
	const char * headers,
	char * buffer,
	int bufferSize,
	GHTTPPost post,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	GHTTPBool bResult;
	GHIConnection * connection;

	assert(URL && URL[0]);
	assert(bufferSize >= 0);
	assert(!buffer || bufferSize);

	// Check args.
	//////////////
	if(!URL || !URL[0])
		return GHTTPInvalidURL;
	if(bufferSize < 0)
		return GHTTPInvalidBufferSize;
	if(buffer && !bufferSize)
		return GHTTPInvalidBufferSize;

	// Startup if it hasn't been done.
	//////////////////////////////////
	if(!ghiReferenceCount)
		ghttpStartup();

	// Get a new connection object.
	///////////////////////////////
	connection = ghiNewConnection();
	if(!connection)
		return GHTTPInsufficientMemory;

	// Fill in the necessary info.
	//////////////////////////////
	connection->type = GHIGET;
	connection->URL = goastrdup(URL);
	if(!connection->URL)
	{
		ghiFreeConnection(connection);
		return GHTTPInsufficientMemory;
	}
	if(headers && *headers)
	{
		connection->sendHeaders = goastrdup(headers);
		if(!connection->sendHeaders)
		{
			ghiFreeConnection(connection);
			return GHTTPInsufficientMemory;
		}
	}
	connection->post = post;
	connection->blocking = blocking;
	connection->progressCallback = progressCallback;
	connection->completedCallback = completedCallback;
	connection->callbackParam = param;
	connection->throttle = throttle;
	connection->userBufferSupplied = (buffer != NULL)?GHTTPTrue:GHTTPFalse;
	if(connection->userBufferSupplied)
		bResult = ghiInitFixedBuffer(connection, &connection->getFileBuffer, buffer, bufferSize);
	else
		bResult = ghiInitBuffer(connection, &connection->getFileBuffer, GET_FILE_BUFFER_INITIAL_SIZE, GET_FILE_BUFFER_INCREMENT_SIZE);
	if(!bResult)
	{
		ghiFreeConnection(connection);
		return GHTTPUnspecifiedError;
	}

	// Setup the post state if needed.
	//////////////////////////////////
	if(post && !ghiPostInitState(connection))
	{
		ghiFreeConnection(connection);
		return GHTTPInvalidPost;
	}

	// Check blocking.
	//////////////////
	if(blocking)
	{
		// Loop until completed.
		////////////////////////
		while(!ghiProcessConnection(connection))
			msleep(10);

		// Done.
		////////
		return 0;
	}

	return connection->request;
}
#ifdef GSI_UNICODE
GHTTPRequest ghttpGetExW
(
	const unsigned short * URL,
	const unsigned short * headers,
	char * buffer,
	int bufferSize,
	GHTTPPost post,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	char URL_A[1024];
	char headers_A[1024] = { '\0' };

	assert(URL != NULL);
	UCS2ToAsciiString(URL, (char*)URL_A);
	if (headers != NULL)
		UCS2ToAsciiString(headers, headers_A);
	return ghttpGetExA((char*)URL_A, (char*)headers_A, buffer, bufferSize, post, throttle, blocking, progressCallback, completedCallback, param);
}
#endif

GHTTPRequest ghttpSaveA
(
	const char * URL,
	const char * filename,
	GHTTPBool blocking,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	return ghttpSaveExA(URL, filename, NULL, NULL, GHTTPFalse, blocking, NULL, completedCallback, param);
}
#ifdef GSI_UNICODE
GHTTPRequest ghttpSaveW
(
	const unsigned short * URL,
	const unsigned short * filename,
	GHTTPBool blocking,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	char URL_A[1024] = { '\0' };
	char filename_A[1024] = { '\0' };

	assert(URL != NULL);
	UCS2ToAsciiString(URL, URL_A);
	UCS2ToAsciiString(filename, filename_A);
	return ghttpSaveA(URL_A, filename_A, blocking, completedCallback, param);
}
#endif

static GHTTPRequest _ghttpSaveEx
(
	const char * URL,
	const gsi_char * filename,
	const char * headers,
	GHTTPPost post,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	GHIConnection * connection;

	assert(URL && URL[0]);
	assert(filename && filename[0]);

	// Check args.
	//////////////
	if(!URL || !URL[0])
		return GHTTPInvalidURL;
	if(!filename || !filename[0])
		return GHTTPInvalidFileName;

	// Startup if it hasn't been done.
	//////////////////////////////////
	if(!ghiReferenceCount)
		ghttpStartup();

	// Get a new connection object.
	///////////////////////////////
	connection = ghiNewConnection();
	if(!connection)
		return GHTTPInsufficientMemory;

	// Fill in the necessary info.
	//////////////////////////////
	connection->type = GHISAVE;
	connection->URL = goastrdup(URL);
	if(!connection->URL)
	{
		ghiFreeConnection(connection);
		return GHTTPInsufficientMemory;
	}
	if(headers && *headers)
	{
		connection->sendHeaders = goastrdup(headers);
		if(!connection->sendHeaders)
		{
			ghiFreeConnection(connection);
			return GHTTPInsufficientMemory;
		}
	}
	connection->post = post;
	connection->blocking = blocking;
	connection->progressCallback = progressCallback;
	connection->completedCallback = completedCallback;
	connection->callbackParam = param;
	connection->throttle = throttle;

	// Setup the post state if needed.
	//////////////////////////////////
	if(post && !ghiPostInitState(connection))
	{
		ghiFreeConnection(connection);
		return GHTTPInvalidPost;
	}

	// Open the file we're saving to.
	/////////////////////////////////
#ifdef NOFILE
	connection->saveFile = NULL;
#else
	connection->saveFile = _tfopen(filename, _T("wb"));
#endif
	if(!connection->saveFile)
	{
		ghiFreeConnection(connection);
		return GHTTPFailedToOpenFile;
	}

	// Check blocking.
	//////////////////
	if(blocking)
	{
		// Loop until completed.
		////////////////////////
		while(!ghiProcessConnection(connection))
			msleep(10);

		// Done.
		////////
		return 0;
	}

	return connection->request;
}

GHTTPRequest ghttpSaveExA
(
	const char * URL,
	const char * filename,
	const char * headers,
	GHTTPPost post,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	#ifdef GSI_UNICODE
		unsigned short filename_W[1024];
		AsciiToUCS2String(filename, filename_W);
		return _ghttpSaveEx(URL, filename_W, headers, post, throttle, blocking, progressCallback, completedCallback, param);
	#else
		return _ghttpSaveEx(URL, filename, headers, post, throttle, blocking, progressCallback, completedCallback, param);
	#endif
}

#ifdef GSI_UNICODE
GHTTPRequest ghttpSaveExW
(
	const unsigned short * URL,
	const unsigned short * filename,
	const unsigned short * headers,
	GHTTPPost post,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	char URL_A[1024];
	//char filename_A[1024] = { '\0' };
	char headers_A[1024] = { '\0' };

	assert(URL_A != NULL);
	UCS2ToAsciiString(URL, URL_A);
	//if (filename != NULL)
	//	UCS2ToAsciiString(filename, filename_A);
	if (headers != NULL)
		UCS2ToAsciiString(headers, headers_A);

	return _ghttpSaveEx(URL_A, filename, headers_A, post, throttle, blocking, progressCallback, completedCallback, param);
}
#endif

GHTTPRequest ghttpStreamA
(
	const char * URL,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	return ghttpStreamExA(URL, NULL, NULL, GHTTPFalse, blocking, progressCallback, completedCallback, param);
}
#ifdef GSI_UNICODE
GHTTPRequest ghttpStreamW
(
	const unsigned short * URL,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	char* URL_A = { '\0' };
	UCS2ToAsciiString(URL, URL_A);
	return ghttpStreamA(URL_A, blocking, progressCallback, completedCallback, param);
}
#endif

GHTTPRequest ghttpStreamExA
(
	const char * URL,
	const char * headers,
	GHTTPPost post,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	GHIConnection * connection;

	assert(URL && URL[0]);

	// Check args.
	//////////////
	if(!URL || !URL[0])
		return GHTTPInvalidURL;

	// Startup if it hasn't been done.
	//////////////////////////////////
	if(!ghiReferenceCount)
		ghttpStartup();

	// Get a new connection object.
	///////////////////////////////
	connection = ghiNewConnection();
	if(!connection)
		return GHTTPInsufficientMemory;

	// Fill in the necessary info.
	//////////////////////////////
	connection->type = GHISTREAM;
	connection->URL = goastrdup(URL);
	if(!connection->URL)
	{
		ghiFreeConnection(connection);
		return GHTTPInsufficientMemory;
	}
	if(headers && *headers)
	{
		connection->sendHeaders = goastrdup(headers);
		if(!connection->sendHeaders)
		{
			ghiFreeConnection(connection);
			return GHTTPInsufficientMemory;
		}
	}
	connection->post = post;
	connection->blocking = blocking;
	connection->progressCallback = progressCallback;
	connection->completedCallback = completedCallback;
	connection->callbackParam = param;
	connection->throttle = throttle;

	// Setup the post state if needed.
	//////////////////////////////////
	if(post && !ghiPostInitState(connection))
	{
		ghiFreeConnection(connection);
		return GHTTPInvalidPost;
	}

	// Check blocking.
	//////////////////
	if(blocking)
	{
		// Loop until completed.
		////////////////////////
		while(!ghiProcessConnection(connection))
			msleep(10);

		// Done.
		////////
		return 0;
	}

	return connection->request;
}
#ifdef GSI_UNICODE
GHTTPRequest ghttpStreamExW
(
	const unsigned short * URL,
	const unsigned short * headers,
	GHTTPPost post,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	char URL_A[1024] = {'\0'};
	char headers_A[1024] = {'\0'};
	UCS2ToAsciiString(URL, URL_A);
	if(headers != NULL)
		UCS2ToAsciiString(headers, headers_A);
	return ghttpStreamExA(URL_A, headers_A, post, throttle, blocking, progressCallback, completedCallback, param);
}
#endif

GHTTPRequest ghttpHeadA
(
	const char * URL,
	GHTTPBool blocking,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	return ghttpHeadExA(URL, NULL, GHTTPFalse, blocking, NULL, completedCallback, param);
}
#ifdef GSI_UNICODE
GHTTPRequest ghttpHeadW
(
	const unsigned short * URL,
	GHTTPBool blocking,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	char URL_A[1024] = {'\0'};
	UCS2ToAsciiString(URL, URL_A);
	return ghttpHeadA(URL_A, blocking, completedCallback, param);
}
#endif

GHTTPRequest ghttpHeadExA
(
	const char * URL,
	const char * headers,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	GHIConnection * connection;

	assert(URL && URL[0]);

	// Check args.
	//////////////
	if(!URL || !URL[0])
		return GHTTPInvalidURL;

	// Startup if it hasn't been done.
	//////////////////////////////////
	if(!ghiReferenceCount)
		ghttpStartup();

	// Get a new connection object.
	///////////////////////////////
	connection = ghiNewConnection();
	if(!connection)
		return GHTTPInsufficientMemory;

	// Fill in the necessary info.
	//////////////////////////////
	connection->type = GHIHEAD;
	connection->URL = goastrdup(URL);
	if(!connection->URL)
	{
		ghiFreeConnection(connection);
		return GHTTPInsufficientMemory;
	}
	if(headers && *headers)
	{
		connection->sendHeaders = goastrdup(headers);
		if(!connection->sendHeaders)
		{
			ghiFreeConnection(connection);
			return GHTTPInsufficientMemory;
		}
	}
	connection->blocking = blocking;
	connection->progressCallback = progressCallback;
	connection->completedCallback = completedCallback;
	connection->callbackParam = param;
	connection->throttle = throttle;

	// Check blocking.
	//////////////////
	if(blocking)
	{
		// Loop until completed.
		////////////////////////
		while(!ghiProcessConnection(connection))
			msleep(10);

		// Done.
		////////
		return 0;
	}

	return connection->request;
}
#ifdef GSI_UNICODE
GHTTPRequest ghttpHeadExW
(
	const unsigned short * URL,
	const unsigned short * headers,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	char URL_A[1024] = {'\0'};
	char headers_A[1024] = {'\0'};
	if (URL != NULL)
		UCS2ToAsciiString(URL, URL_A);
	if (headers != NULL)
		UCS2ToAsciiString(headers, headers_A);
	return ghttpHeadExA(URL_A, headers_A, throttle, blocking, progressCallback, completedCallback, param);
}
#endif

GHTTPRequest ghttpPostA
(
	const char * URL,
	GHTTPPost post,
	GHTTPBool blocking,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	return ghttpPostExA(URL, NULL, post, GHTTPFalse, blocking, NULL, completedCallback, param);
}
#ifdef GSI_UNICODE
GHTTPRequest ghttpPostW
(
	const unsigned short * URL,
	GHTTPPost post,
	GHTTPBool blocking,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	char URL_A[1024] = {'\0'};
	UCS2ToAsciiString(URL, URL_A);
	return ghttpPostA(URL_A, post, blocking, completedCallback, param);
}
#endif

GHTTPRequest ghttpPostExA
(
	const char * URL,
	const char * headers,
	GHTTPPost post,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	GHIConnection * connection;

	assert(URL && URL[0]);
	assert(post);

	// Check args.
	//////////////
	if(!URL || !URL[0])
		return GHTTPInvalidURL;
	if(!post)
		return GHTTPInvalidPost;

	// Startup if it hasn't been done.
	//////////////////////////////////
	if(!ghiReferenceCount)
		ghttpStartup();

	// Get a new connection object.
	///////////////////////////////
	connection = ghiNewConnection();
	if(!connection)
		return GHTTPInsufficientMemory;

	// Fill in the necessary info.
	//////////////////////////////
	connection->type = GHIPOST;
	connection->URL = goastrdup(URL);
	if(!connection->URL)
	{
		ghiFreeConnection(connection);
		return GHTTPInsufficientMemory;
	}
	if(headers && *headers)
	{
		connection->sendHeaders = goastrdup(headers);
		if(!connection->sendHeaders)
		{
			ghiFreeConnection(connection);
			return GHTTPInsufficientMemory;
		}
	}
	connection->post = post;
	connection->blocking = blocking;
	connection->progressCallback = progressCallback;
	connection->completedCallback = completedCallback;
	connection->callbackParam = param;
	connection->throttle = throttle;

	// Setup the post state if needed.
	//////////////////////////////////
	if(post && !ghiPostInitState(connection))
	{
		ghiFreeConnection(connection);
		return GHTTPInvalidPost;
	}

	// Check blocking.
	//////////////////
	if(blocking)
	{
		// Loop until completed.
		////////////////////////
		while(!ghiProcessConnection(connection))
			msleep(10);

		// Done.
		////////
		return 0;
	}

	return connection->request;
}
#ifdef GSI_UNICODE
GHTTPRequest ghttpPostExW
(
	const unsigned short * URL,
	const unsigned short * headers,
	GHTTPPost post,
	GHTTPBool throttle,
	GHTTPBool blocking,
	ghttpProgressCallback progressCallback,
	ghttpCompletedCallback completedCallback,
	void * param
)
{
	char URL_A[1024] = {'\0'};
	char headers_A[1024] = {'\0'};
	UCS2ToAsciiString(URL, URL_A);
	if (headers != NULL)
		UCS2ToAsciiString(headers, headers_A);
	return ghttpPostExA(URL_A, headers_A, post, throttle, blocking, progressCallback, completedCallback, param);
}
#endif

void ghttpThink
(
	void
)
{
	// Process all the connections.
	///////////////////////////////
	ghiEnumConnections(ghiProcessConnection);
}

GHTTPBool ghttpRequestThink
(
	GHTTPRequest request
)
{
	GHIConnection * connection;

	// Get the connection object for this request.
	//////////////////////////////////////////////
	connection = ghiRequestToConnection(request);
	if(!connection)
		return GHTTPFalse;

	// Think.
	/////////
	ghiProcessConnection(connection);
	return GHTTPTrue;
}

void ghttpCancelRequest
(
	GHTTPRequest request
)
{
	GHIConnection * connection;

	// Get the connection object for this request.
	//////////////////////////////////////////////
	connection = ghiRequestToConnection(request);
	if(!connection)
		return;

	// Free it.
	///////////
	ghiFreeConnection(connection);
}

#if !defined(INSOCK)
// INSOCK does not support partial shutdown 
void ghttpCloseRequest
(
	GHTTPRequest request
)
{
	GHIConnection * connection;

	connection = ghiRequestToConnection(request);
	if (!connection)
		return;

	if (connection->socket)
	{
		// Gracefully close the connection
		// SDK will dispatch a "request cancelled" callback when all data
		// has been received
		shutdown(connection->socket, 1);
		connection->result = GHTTPRequestCancelled;
	}
}
#endif

GHTTPState ghttpGetState
(
	GHTTPRequest request
)
{
	GHIConnection * connection;

	// Get the connection object for this request.
	//////////////////////////////////////////////
	connection = ghiRequestToConnection(request);
	if(!connection)
		return (GHTTPState)0;

	return connection->state;
}

const char * ghttpGetResponseStatus
(
	GHTTPRequest request,
	int * statusCode
)
{
	GHIConnection * connection;

	// Get the connection object for this request.
	//////////////////////////////////////////////
	connection = ghiRequestToConnection(request);
	if(!connection)
		return NULL;

	// Check if we don't have the status yet.
	/////////////////////////////////////////
	if(connection->state <= GHTTPReceivingStatus)
		return NULL;

	// Set the status code.
	///////////////////////
	if(statusCode)
		*statusCode = connection->statusCode;

	return (connection->recvBuffer.data + connection->statusStringIndex);
}

const char * ghttpGetHeaders
(
	GHTTPRequest request
)
{
	GHIConnection * connection;

	// Get the connection object for this request.
	//////////////////////////////////////////////
	connection = ghiRequestToConnection(request);
	if(!connection)
		return NULL;

	// Check if we don't have the headers yet.
	//////////////////////////////////////////
	if(connection->state < GHTTPReceivingHeaders)
		return NULL;

	// Verify we have headers.
	//////////////////////////
	if(connection->headerStringIndex >= connection->recvBuffer.len)
		return NULL;

	return (connection->recvBuffer.data + connection->headerStringIndex);
}

const char * ghttpGetURL
(
	GHTTPRequest request
)
{
	GHIConnection * connection;

	// Get the connection object for this request.
	//////////////////////////////////////////////
	connection = ghiRequestToConnection(request);
	if(!connection)
		return NULL;

	return connection->URL;
}

GHTTPBool ghttpSetProxy
(
	const char * server
)
{
	return ghiSetProxy(server);
}

GHTTPBool ghttpSetRequestProxy
(
	GHTTPRequest request, 
	const char * server 
)
{
	return ghiSetRequestProxy(request, server);
}


void ghttpSetThrottle
(
	GHTTPRequest request,
	GHTTPBool throttle
)
{
	GHIConnection * connection;

	// Get the connection object for this request.
	//////////////////////////////////////////////
	connection = ghiRequestToConnection(request);
	if(!connection)
		return;

	connection->throttle = throttle;

	// Set the buffer size based on the throttle setting.
	/////////////////////////////////////////////////////
	if(connection->socket != INVALID_SOCKET)
		SetReceiveBufferSize(connection->socket, throttle?ghiThrottleBufferSize:(8 * 1024));
}

void ghttpThrottleSettings
(
	int bufferSize,
	gsi_time timeDelay
)
{
	ghiThrottleSettings(bufferSize, timeDelay);
}

void ghttpSetMaxRecvTime
(
	GHTTPRequest request,
	gsi_time maxRecvTime
)
{
	GHIConnection* connection = ghiRequestToConnection(request);
	if (connection == NULL)
		return;

	connection->maxRecvTime = maxRecvTime;
}

// Internal prototypes for persistent HTTP connections
// Prevents warnings from strict compilers
//////////////////////////////////////////////////////
SOCKET ghttpGetSocket(GHTTPRequest request);
GHTTPBool ghttpReuseSocket(GHTTPRequest request, SOCKET socket);

// For use in persistent HTTP connections
// Call this in the completed callback to obtain the socket, which can be used with 
// ghttpReuseSocket to make a second request to the same host
///////////////////////////////////////////////////////////////////
SOCKET ghttpGetSocket
(
	GHTTPRequest request
)
{
	GHIConnection * connection;
	SOCKET ret;

	// Get the connection object for this request.
	//////////////////////////////////////////////
	connection = ghiRequestToConnection(request);
	if(!connection)
		return INVALID_SOCKET;

	// Only allow them to grab the socket during the competion callback (not while a request is in process)
	/////////////////////////////////////////////////////////
	if (!connection->completed)
		return INVALID_SOCKET;

	ret = connection->socket;
	// Mark the connection as invalid so that it doesn't get closed
	connection->socket = INVALID_SOCKET;
	
	return ret;
}

// For use in persistent HTTP connections
// Call this after creating a request, but before calling think the first time in order to reuse
// an existing connection to the same server
// If the socket passed is INVALID_SOCKET, a new connection will be created and marked as persistent
///////////////////////////////////////////////////////////////////
GHTTPBool ghttpReuseSocket
(
	GHTTPRequest request,
	SOCKET socket
)
{
	GHIConnection * connection;

	// Get the connection object for this request.
	//////////////////////////////////////////////
	connection = ghiRequestToConnection(request);
	if(!connection)
		return GHTTPFalse;
	if (connection->state != GHTTPSocketInit)
		return GHTTPFalse;
	if (connection->socket != INVALID_SOCKET)
		return GHTTPFalse;

	connection->persistConnection = GHTTPTrue;
	connection->socket = socket;


	/*
	if (socket == INVALID_SOCKET)
	{
		// The connection is marked as persistent, but we still need to lookup and connect, so don't advance the state
		/////////////////////////////////////////////////////////////////////
		return GHTTPTrue;
	}

	// Skip the host lookup & connect - send data once the socket is writable
	//////////////////////////////////////////////
	connection->state = GHTTPConnecting;	
	*/
	return GHTTPTrue;
}


GHTTPPost ghttpNewPost
(
	void
)
{
	return ghiNewPost();
}

void ghttpPostSetAutoFree
(
	GHTTPPost post,
	GHTTPBool autoFree
)
{
	assert(post);
	if(!post)
		return;

	ghiPostSetAutoFree(post, autoFree);
}

void ghttpFreePost
(
	GHTTPPost post
)
{
	assert(post);
	if(!post)
		return;

	ghiFreePost(post);
}

GHTTPBool ghttpPostAddStringA
(
	GHTTPPost post,
	const char * name,
	const char * string
)
{
	assert(post);
	assert(name && name[0]);

	if(!post)
		return GHTTPFalse;
	if(!name || !name[0])
		return GHTTPFalse;
	if(!string)
		string = "";

	return ghiPostAddString(post, name, string);
}
#ifdef GSI_UNICODE
GHTTPBool ghttpPostAddStringW
(
	GHTTPPost post,
	const unsigned short * name,
	const unsigned short * string
)
{
	char name_A[1024] = {'\0'};
	char string_A[1024] = {'\0'};
	if (name != NULL)
		UCS2ToAsciiString(name, name_A);
	if (string != NULL)
		UCS2ToAsciiString(string, string_A);
	return ghttpPostAddStringA(post, name_A, string_A);
}
#endif

GHTTPBool ghttpPostAddFileFromDiskA
(
	GHTTPPost post,
	const char * name,
	const char * filename,
	const char * reportFilename,
	const char * contentType
)
{
	assert(post);
	assert(name && name[0]);
	assert(filename && filename[0]);

	if(!post)
		return GHTTPFalse;
	if(!name || !name[0])
		return GHTTPFalse;
	if(!filename || !filename[0])
		return GHTTPFalse;
	if(!reportFilename || !reportFilename[0])
		reportFilename = filename;
	if(!contentType)
		contentType = "application/octet-stream";

	return ghiPostAddFileFromDisk(post, name, filename, reportFilename, contentType);
}
#ifdef GSI_UNICODE
GHTTPBool ghttpPostAddFileFromDiskW
(
	GHTTPPost post,
	const unsigned short * name,
	const unsigned short * filename,
	const unsigned short * reportFilename,
	const unsigned short * contentType
)
{
	char name_A[1024] = {'\0'};
	char filename_A[1024] = {'\0'};
	char reportFilename_A[1024] = {'\0'};
	char contentType_A[1024] = {'\0'};
	if (name != NULL)			UCS2ToAsciiString(name, name_A);
	if (filename != NULL)		UCS2ToAsciiString(filename, filename_A);
	if (reportFilename != NULL)	UCS2ToAsciiString(reportFilename, reportFilename_A);
	if (contentType != NULL)	UCS2ToAsciiString(contentType, contentType_A);
	return ghttpPostAddFileFromDiskA(post, name_A, filename_A, reportFilename_A, contentType_A);
}
#endif

GHTTPBool ghttpPostAddFileFromMemoryA
(
	GHTTPPost post,
	const char * name,
	const char * buffer,
	int bufferLen,
	const char * reportFilename,
	const char * contentType
)
{
	assert(post);
	assert(name && name[0]);
	assert(bufferLen >= 0);
#ifdef _DEBUG
	if(bufferLen > 0)
		assert(buffer);
#endif
	assert(reportFilename && reportFilename[0]);

	if(!post)
		return GHTTPFalse;
	if(!name || !name[0])
		return GHTTPFalse;
	if(bufferLen < 0)
		return GHTTPFalse;
	if(!bufferLen && !buffer)
		return GHTTPFalse;
	if(!contentType)
		contentType = "application/octet-stream";

	return ghiPostAddFileFromMemory(post, name, buffer, bufferLen, reportFilename, contentType);
}
#ifdef GSI_UNICODE
GHTTPBool ghttpPostAddFileFromMemoryW
(
	GHTTPPost post,
	const unsigned short * name,
	const char * buffer,
	int bufferLen,
	const unsigned short * reportFilename,
	const unsigned short * contentType
)
{
	char name_A[1024] = { '\0' };
	char reportFilename_A[1024] = { '\0' };
	char contentType_A[1024] = { '\0' };
	if (name != NULL)
		UCS2ToAsciiString(name, name_A);
	if (reportFilename != NULL)
		UCS2ToAsciiString(reportFilename, reportFilename_A);
	if (contentType != NULL)
		UCS2ToAsciiString(contentType, contentType_A);

	
	return ghttpPostAddFileFromMemoryA(post, name_A, buffer, bufferLen, reportFilename_A, contentType_A);
	
	GSI_UNUSED(reportFilename);
	GSI_UNUSED(contentType);
}
#endif

GHTTPBool ghttpPostAddXml
(
	GHTTPPost post,
	GSXmlStreamWriter soap
)
{
	GS_ASSERT(post != NULL);
	GS_ASSERT(soap != NULL);

	return ghiPostAddXml(post, soap);
}

void ghttpPostSetCallback
(
	GHTTPPost post,
	ghttpPostCallback callback,
	void * param
)
{
	assert(post);

	if(!post)
		return;

	ghiPostSetCallback(post, callback, param);
}


