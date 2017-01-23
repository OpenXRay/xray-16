 /*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#include "ghttpBuffer.h"
#include "ghttpConnection.h"
#include "ghttpMain.h"
#include "ghttpCommon.h"
#include "../common/gsCrypt.h"
#include "../common/gsSSL.h"


// Resize the buffer.
/////////////////////
GHTTPBool ghiResizeBuffer
(
	GHIBuffer * buffer,
	int sizeIncrement
)
{
	char * tempPtr;
	int newSize;

	assert(buffer);
	assert(sizeIncrement > 0);
	assert(buffer->fixed == GHTTPFalse); // implied by sizeIncrement > 0

	// Check args.
	//////////////
	if(!buffer)
		return GHTTPFalse;
	if(sizeIncrement <= 0)
		return GHTTPFalse;

	// Reallocate with the bigger size.
	///////////////////////////////////
	newSize = (buffer->size + sizeIncrement);
	tempPtr = (char *)gsirealloc(buffer->data, (unsigned int)newSize);
	if(!tempPtr)
		return GHTTPFalse;

	// Set the new info.
	////////////////////
	buffer->data = tempPtr;
	buffer->size = newSize;

	return GHTTPTrue;
}

GHTTPBool ghiInitBuffer
(
	struct GHIConnection * connection,
	GHIBuffer * buffer,
	int initialSize,
	int sizeIncrement
)
{
	GHTTPBool bResult;

	assert(connection);
	assert(buffer);
	assert(initialSize > 0);
	assert(sizeIncrement > 0);

	// Check args.
	//////////////
	if(!connection)
		return GHTTPFalse;
	if(!buffer)
		return GHTTPFalse;
	if(initialSize <= 0)
		return GHTTPFalse;
	if(sizeIncrement <= 0)
		return GHTTPFalse;

	// Init the struct.
	///////////////////
	buffer->connection = connection;
	buffer->data = NULL;
	buffer->size = 0;
	buffer->len = 0;
	buffer->pos = 0;
	buffer->sizeIncrement = sizeIncrement;
	buffer->fixed = GHTTPFalse;
	buffer->dontFree = GHTTPFalse;
	buffer->readOnly = GHTTPFalse;

	// Do the initial resize.
	/////////////////////////
	bResult = ghiResizeBuffer(buffer, initialSize);
	if(!bResult)
		return GHTTPFalse;

	// Start with an empty string.
	//////////////////////////////
	*buffer->data = '\0';

	return GHTTPTrue;
}

GHTTPBool ghiInitFixedBuffer
(
	struct GHIConnection * connection,
	GHIBuffer * buffer,
	char * userBuffer,
	int size
)
{
	assert(connection);
	assert(buffer);
	assert(userBuffer);
	assert(size > 0);

	// Check args.
	//////////////
	if(!connection)
		return GHTTPFalse;
	if(!buffer)
		return GHTTPFalse;
	if(!userBuffer)
		return GHTTPFalse;
	if(size <= 0)
		return GHTTPFalse;

	// Init the struct.
	///////////////////
	buffer->connection = connection;
	buffer->data = userBuffer;
	buffer->size = size;
	buffer->len = 0;
	buffer->pos = 0;
	buffer->sizeIncrement = 0;
	buffer->fixed = GHTTPTrue;
	buffer->dontFree = GHTTPTrue;
	buffer->readOnly = GHTTPFalse;

	// Start with an empty string.
	//////////////////////////////
	*buffer->data = '\0';

	return GHTTPTrue;
}

GHTTPBool ghiInitReadOnlyBuffer
(
	struct GHIConnection * connection,  // The connection.
	GHIBuffer * buffer,  // The buffer to init.
	const char * userBuffer,   // The user-buffer to use.
	int size             // The size of the buffer.
)
{
	assert(connection);
	assert(buffer);
	assert(userBuffer);
	assert(size > 0);

	// Check args.
	//////////////
	if(!connection)
		return GHTTPFalse;
	if(!buffer)
		return GHTTPFalse;
	if(!userBuffer)
		return GHTTPFalse;
	if(size <= 0)
		return GHTTPFalse;

	// Init the struct.
	///////////////////
	buffer->connection = connection;
	buffer->data = (char*)userBuffer; // cast away const
	buffer->size = size;
	buffer->pos = 0;
	buffer->sizeIncrement = 0;
	buffer->fixed = GHTTPTrue;
	buffer->dontFree = GHTTPTrue;
	buffer->readOnly = GHTTPTrue;

	// Start with user supplied data
	//////////////////////////////
	buffer->len = size; 

	return GHTTPTrue;
}

void ghiFreeBuffer
(
	GHIBuffer * buffer
)
{
	assert(buffer);

	// Check args.
	//////////////
	if(!buffer)
		return;
	if(!buffer->data)
		return;

	// Cleanup the struct.
	//////////////////////
	if(!buffer->dontFree)
		gsifree(buffer->data);
	memset(buffer, 0, sizeof(GHIBuffer));
}

GHTTPBool ghiAppendDataToBuffer
(
	GHIBuffer * buffer,
	const char * data,
	int dataLen
)
{
	GHTTPBool bResult;
	int newLen;

	assert(buffer);
	assert(data);
	assert(dataLen >= 0);

	// Check args.
	//////////////
	if(!buffer)
		return GHTTPFalse;
	if(!data)
		return GHTTPFalse;
	if(dataLen < 0)
		return GHTTPFalse;
	if (buffer->readOnly)
		return GHTTPFalse;

	// Get the string length if needed.
	///////////////////////////////////
	if(dataLen == 0)
		dataLen = (int)strlen(data);

	// Get the new length.
	//////////////////////
	newLen = (buffer->len + dataLen);

	// Make sure the array is big enough.
	/////////////////////////////////////
	while(newLen >= buffer->size)
	{
		// Check for a fixed buffer.
		////////////////////////////
		if(buffer->fixed)
		{
			buffer->connection->completed = GHTTPTrue;
			buffer->connection->result = GHTTPBufferOverflow;
			return GHTTPFalse;
		}

		bResult = ghiResizeBuffer(buffer, buffer->sizeIncrement);
		if(!bResult)
		{
			buffer->connection->completed = GHTTPTrue;
			buffer->connection->result = GHTTPOutOfMemory;
			return GHTTPFalse;
		}
	}

	// Add the data.
	////////////////
	memcpy(buffer->data + buffer->len, data, (unsigned int)dataLen);
	buffer->len = newLen;
	buffer->data[buffer->len] = '\0';
	return GHTTPTrue;
}


// Use sparingly. This function wraps the data in an SSL record.
GHTTPBool ghiEncryptDataToBuffer
(
	GHIBuffer * buffer,
	const char * data,
	int dataLen
)
{
	GHIEncryptionResult result;
	int bufSpace = 0;
	int pos = 0;

	assert(buffer);
	assert(data);
	assert(dataLen >= 0);

	// Check args.
	//////////////
	if(!buffer)
		return GHTTPFalse;
	if(!data)
		return GHTTPFalse;
	if(dataLen < 0)
		return GHTTPFalse;
	if (buffer->readOnly)
		return GHTTPFalse;

	// Switch to plain text append when not using SSL
	if (buffer->connection->encryptor.mEngine == GHTTPEncryptionEngine_None ||
		buffer->connection->encryptor.mSessionEstablished == GHTTPFalse)
	{
		return ghiAppendDataToBuffer(buffer, data, dataLen);
	}

	// Get the string length if needed.
	///////////////////////////////////
	if(dataLen == 0)
		dataLen = (int)strlen(data);
	if (dataLen == 0)
		return GHTTPTrue; // no data and strlen == 0
	bufSpace = buffer->size - buffer->len;

	do 
	{	
		int fragmentLen = min(dataLen, GS_SSL_MAX_CONTENTLENGTH);
		
		// Call the encryptor function
		//    bufSize is reduced by the number of bytes written
		result = buffer->connection->encryptor.mEncryptFunc(buffer->connection, &buffer->connection->encryptor, 
													&data[pos], dataLen,
													&buffer->data[buffer->len], &bufSpace);
		if (result == GHIEncryptionResult_BufferTooSmall)
		{
			if (ghiResizeBuffer(buffer, buffer->sizeIncrement) == GHTTPFalse)
				return GHTTPFalse;
			bufSpace = buffer->size - buffer->len;
		}
		else if (result == GHIEncryptionResult_Success)
		{
			// update data and buffer positions
			pos += fragmentLen;
			buffer->len = buffer->size - bufSpace;
		}
		else
		{
			gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_WarmError,
				"ghiEncryptDataToBuffer encountered unhandled return code: %d\r\n", result);
			return GHTTPFalse;
		}
	} while(pos < dataLen);

	return GHTTPTrue;
}

GHTTPBool ghiAppendHeaderToBuffer
(
	GHIBuffer * buffer,
	const char * name,
	const char * value
)
{
	if(!ghiAppendDataToBuffer(buffer, name, 0))
		return GHTTPFalse;
	if(!ghiAppendDataToBuffer(buffer, ": ", 2))
		return GHTTPFalse;
	if(!ghiAppendDataToBuffer(buffer, value, 0))
		return GHTTPFalse;
	if(!ghiAppendDataToBuffer(buffer, CRLF, 2))
		return GHTTPFalse;

	return GHTTPTrue;
}

GHTTPBool ghiAppendCharToBuffer
(
	GHIBuffer * buffer,
	int c
)
{
	GHTTPBool bResult;
	assert(buffer);

	// Check args.
	//////////////
	if(!buffer)
		return GHTTPFalse;
	if (buffer->readOnly)
		return GHTTPFalse;

	// Make sure the array is big enough.
	/////////////////////////////////////
	if((buffer->len + 1) >= buffer->size)
	{
		// Check for a fixed buffer.
		////////////////////////////
		if(buffer->fixed)
		{
			buffer->connection->completed = GHTTPTrue;
			buffer->connection->result = GHTTPBufferOverflow;
			return GHTTPFalse;
		}

		bResult = ghiResizeBuffer(buffer, buffer->sizeIncrement);
		if(!bResult)
		{
			buffer->connection->completed = GHTTPTrue;
			buffer->connection->result = GHTTPOutOfMemory;
			return GHTTPFalse;
		}
	}

	// Add the char.
	////////////////
	buffer->data[buffer->len] = (char)(c & 0xFF);
	buffer->len++;
	buffer->data[buffer->len] = '\0';

	return GHTTPTrue;
}

GHTTPBool ghiAppendIntToBuffer
(
	GHIBuffer * buffer,
	int i
)
{
	char intValue[16];

	sprintf(intValue, "%d", i);

	return ghiAppendDataToBuffer(buffer, intValue, 0);
}

void ghiResetBuffer
(
	GHIBuffer * buffer
)
{
	assert(buffer);

	buffer->len = 0;
	buffer->pos = 0;

	// Start with an empty string.
	//////////////////////////////
	if (!buffer->readOnly)
		*buffer->data = '\0';
}

GHTTPBool ghiSendBufferedData
(
	struct GHIConnection * connection
)
{
	int rcode;
	int writeFlag;
	int exceptFlag;
	char * data;
	int len;

	// Loop while we can send.
	//////////////////////////
	do
	{
		rcode = GSISocketSelect(connection->socket, NULL, &writeFlag, &exceptFlag);
		if((gsiSocketIsError(rcode)) || ((rcode == 1) && exceptFlag))
		{
			connection->completed = GHTTPTrue;
			connection->result = GHTTPSocketFailed;
			if(gsiSocketIsError(rcode))
				connection->socketError = GOAGetLastError(connection->socket);
			else
				connection->socketError = 0;
			return GHTTPFalse;
		}
		if((rcode < 1) || !writeFlag)
		{
			// Can't send anything.
			///////////////////////
			return GHTTPTrue;
		}

		// Figure out what, and how much, to send.
		//////////////////////////////////////////
		data = (connection->sendBuffer.data + connection->sendBuffer.pos);
		len = (connection->sendBuffer.len - connection->sendBuffer.pos);

		// Do the send.
		///////////////
		rcode = ghiDoSend(connection, data, len);
		if(gsiSocketIsError(rcode))
			return GHTTPFalse;

		// Update the position.
		///////////////////////
		connection->sendBuffer.pos += rcode;
	}
	while(connection->sendBuffer.pos < connection->sendBuffer.len);

	return GHTTPTrue;
}


// Read data from a buffer
GHTTPBool ghiReadDataFromBuffer
(
	GHIBuffer * bufferIn,    // the GHIBuffer to read from
	char        bufferOut[], // the raw buffer to write to
	int *       len          // max number of bytes to append, becomes actual length written
)
{
	int bytesAvailable = 0;
	int bytesToCopy    = 0;
	
	
	// Verify parameters
	assert(bufferIn != NULL);
	assert(len != NULL);
	if (*len == 0)
		return GHTTPFalse;

	// Make sure the bufferIn isn't emtpy
	bytesAvailable = (int)bufferIn->len - bufferIn->pos;
	if (bytesAvailable <= 0)
		return GHTTPFalse;

	// Calculate the actual number of bytes to copy
	bytesToCopy = min(*len-1, bytesAvailable);

	// Copy the bytes
	memcpy(bufferOut, bufferIn->data + bufferIn->pos, (size_t)bytesToCopy);
	bufferOut[bytesToCopy] = '\0';
	*len = bytesToCopy;

	// Adjust the bufferIn read position
	bufferIn->pos += bytesToCopy;
	return GHTTPTrue;
}


// Read data from a buffer with a garunteed length
GHTTPBool ghiReadDataFromBufferFixed
(
	GHIBuffer * bufferIn,    // the GHIBuffer to read from
	char        bufferOut[], // the raw buffer to write to
	int         bytesToCopy  // number of bytes to read
)
{
	// Verify parameters
	assert(bufferIn != NULL);
	if (bytesToCopy == 0)
		return GHTTPTrue;

	// Make sure the bufferIn isn't too small
	if (bufferIn->len < bytesToCopy)
		return GHTTPFalse;

	// Copy the bytes
	memcpy(bufferOut, bufferIn->data + bufferIn->pos, (size_t)bytesToCopy);

	// Adjust the bufferIn read position
	bufferIn->pos += bytesToCopy;
	return GHTTPTrue;
}
