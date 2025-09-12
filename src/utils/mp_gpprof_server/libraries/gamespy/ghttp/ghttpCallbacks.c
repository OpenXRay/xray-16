 /*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#include "ghttpCallbacks.h"
#include "ghttpPost.h"

void ghiCallCompletedCallback
(
	GHIConnection * connection
)
{
	GHTTPBool freeBuffer;
	char * buffer;
	GHTTPByteCount bufferLen;

	assert(connection);
	
#ifdef GSI_COMMON_DEBUG
	if(connection->result != GHTTPSuccess)
	{
		gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_WarmError,
			"Socket Error: %d\n", connection->socketError);
	}
#endif

	// Check for no callback.
	/////////////////////////
	if(!connection->completedCallback)
		return;

	// Figure out the buffer/bufferLen parameters.
	//////////////////////////////////////////////
	if(connection->type == GHIGET)
	{
		buffer = connection->getFileBuffer.data;
	}
	else
	{
		buffer = NULL;
	}
	bufferLen = connection->fileBytesReceived;

	// Call the callback.
	/////////////////////
	freeBuffer = connection->completedCallback(
		connection->request,
		connection->result,
		buffer,
		bufferLen,
		connection->callbackParam);

	// Check for gsifree.
	//////////////////
	if(buffer && !freeBuffer)
		connection->getFileBuffer.dontFree = GHTTPTrue;
}

void ghiCallProgressCallback
(
	GHIConnection * connection,
	const char * buffer,
	GHTTPByteCount bufferLen
)
{	
	assert(connection);

	// Check for no callback.
	/////////////////////////
	if(!connection->progressCallback)
		return;

	// Call the callback.
	/////////////////////
	connection->progressCallback(
		connection->request,
		connection->state,
		buffer,
		bufferLen,
		connection->fileBytesReceived,
		connection->totalSize,
		connection->callbackParam
		);
}

void ghiCallPostCallback
(
	GHIConnection * connection
)
{
	assert(connection);

	// Check for no callback.
	/////////////////////////
	if(!connection->postingState.callback)
		return;

	// Call the callback.
	/////////////////////
	connection->postingState.callback(
		connection->request,
		connection->postingState.bytesPosted,
		connection->postingState.totalBytes,
		connection->postingState.index,
		ArrayLength(connection->postingState.states),
		connection->callbackParam
		);
}
