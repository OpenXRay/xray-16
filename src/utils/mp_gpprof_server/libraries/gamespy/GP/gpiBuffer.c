/*
gpiBuffer.c
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4244) //lines: 720
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS


//INCLUDES
//////////
#include <stdlib.h>
#include <string.h>
#include "gpi.h"

//DEFINES
/////////
#define GPI_DUMP_NET_TRAFFIC

//FUNCTIONS
///////////
GPResult
gpiAppendCharToBuffer(
  GPConnection * connection,
  GPIBuffer * outputBuffer,
  char c
)
{
	int len;
	int size;
	char * output;

	assert(outputBuffer != NULL);

	// Init locals.
	///////////////
	len = outputBuffer->len;
	size = outputBuffer->size;
	output = outputBuffer->buffer;

	// Check if it needs to be resized.
	///////////////////////////////////
	if(size == len)
	{
		size += GPI_READ_SIZE;
		output = (char*)gsirealloc(output, (unsigned int)size + 1);
		if(output == NULL)
			Error(connection, GP_MEMORY_ERROR, "Out of memory.");
	}

	// Do the copy.
	///////////////
	output[len] = c;
	output[len + 1] = '\0';

	// Update the buffer info.
	//////////////////////////
	outputBuffer->len++;
	outputBuffer->size = size;
	outputBuffer->buffer = output;

	return GP_NO_ERROR;
}

GPResult
gpiAppendStringToBufferLen(
  GPConnection * connection,
  GPIBuffer * outputBuffer,
  const char * string,
  int stringLen
)
{
	int len;
	int size;
	char * output;

	assert(string != NULL);
	assert(stringLen >= 0);
	assert(outputBuffer != NULL);

	if(!string)
		return GP_NO_ERROR;

	// Init locals.
	///////////////
	len = outputBuffer->len;
	size = outputBuffer->size;
	output = outputBuffer->buffer;

	// Check if it needs to be resized.
	///////////////////////////////////
	if((size - len) < stringLen)
	{
		size += max(GPI_READ_SIZE, stringLen);
		output = (char*)gsirealloc(output, (unsigned int)size + 1);
		if(output == NULL)
			Error(connection, GP_MEMORY_ERROR, "Out of memory.");
	}

	// Do the copy.
	///////////////
	memcpy(&output[len], string, (unsigned int)stringLen);
	output[len + stringLen] = '\0';

	// Update the buffer info.
	//////////////////////////
	outputBuffer->len += stringLen;
	outputBuffer->size = size;
	outputBuffer->buffer = output;

	return GP_NO_ERROR;
}

GPResult
gpiAppendStringToBuffer(
  GPConnection * connection,
  GPIBuffer * outputBuffer,
  const char * buffer
)
{
	return gpiAppendStringToBufferLen(connection, outputBuffer, buffer, (int)strlen(buffer));
}

GPResult gpiAppendShortToBuffer(
								GPConnection * connection,
								GPIBuffer * outputBuffer, 
								short num)
{
	char shortVal[8];
	sprintf(shortVal, "%d", num);
	return gpiAppendStringToBuffer(connection, outputBuffer, shortVal);
}

GPResult gpiAppendUShortToBuffer(
								 GPConnection * connection,
								 GPIBuffer * outputBuffer, 
								 unsigned short num)
{
	char shortVal[8];
	sprintf(shortVal, "%u", num);
	return gpiAppendStringToBuffer(connection, outputBuffer, shortVal);
}

GPResult
gpiAppendIntToBuffer(
  GPConnection * connection,
  GPIBuffer * outputBuffer,
  int num
)
{
	char intValue[16];
	sprintf(intValue,"%d",num);
	return gpiAppendStringToBuffer(connection, outputBuffer, intValue);
}

GPResult
gpiAppendUIntToBuffer(
  GPConnection * connection,
  GPIBuffer * outputBuffer,
  unsigned int num
)
{
	char intValue[16];
	sprintf(intValue,"%u",num);
	return gpiAppendStringToBuffer(connection, outputBuffer, intValue);
}

static GPResult
gpiSendData(
  GPConnection * connection,
  SOCKET sock,
  const char * buffer,
  int bufferLen,
  GPIBool * closed,
  int * sent,
  char id[3]
)
{
	int rcode;

	rcode = send(sock, buffer, bufferLen, 0);
	if(gsiSocketIsError(rcode))
	{
		rcode = GOAGetLastError(sock);
		if((rcode != WSAEWOULDBLOCK) && (rcode != WSAEINPROGRESS) && (rcode != WSAETIMEDOUT) )
		{
			// handle peer connections specially
			if((id[0] == 'P') && (id[1] == 'R'))
				return GP_NETWORK_ERROR;
			CallbackError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error sending on a socket.");
		}

		*sent = 0;
		*closed = GPIFalse;
	}
	else if(rcode == 0)
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Comment,
			"SENDXXXX(%s): Connection closed\n", id);

		*sent = 0;
		*closed = GPITrue;
	}
	else
	{
		#if defined(GPI_DUMP_NET_TRAFFIC) && defined(GSI_COMMON_DEBUG)
		{
			static int sendCount;
			char *buf = (char *)gsimalloc((size_t)(rcode + 1));
			memcpy(buf, buffer, (size_t)rcode);
			buf[rcode] = '\0';
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_RawDump, "SENT%04d(%s): %s\n", sendCount++, id, buf);
			freeclear(buf);
		}
		#elif defined(GSI_COMMON_DEBUG)
		{
			static int sendCount;
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_RawDump,
				"SENT%04d(%s): %d\n", sendCount++, id, rcode);
		}
		#endif

		*sent = rcode;
		*closed = GPIFalse;
	}

	return GP_NO_ERROR;
}

GPResult
gpiSendOrBufferChar(
  GPConnection * connection,
  GPIPeer_st peer,
  char c
)
{
	//GPIBool closed;
	//int sent;
/*
	assert(peer->outputBuffer.buffer != NULL);

	// Only try to send if the buffer is empty and there are no messages.
	/////////////////////////////////////////////////////////////////////
	if(!(peer->outputBuffer.len - peer->outputBuffer.pos) && !ArrayLength(peer->messages))
	{
		CHECK_RESULT(gpiSendData(connection, peer->sock, &c, 1, &closed, &sent, "PT"));
		if(sent)
			return GP_NO_ERROR;
	}

	// Buffer if not sent.
	//////////////////////
	return gpiAppendCharToBuffer(connection, &peer->outputBuffer, c);
	*/
	GSI_UNUSED(c);
	GSI_UNUSED(peer);
	GSI_UNUSED(connection);
	return GP_NO_ERROR;
}

GPResult
gpiSendOrBufferStringLenToPeer(
	GPConnection * connection,
	GPIPeer_st peer,
	const char * string,
	int stringLen
)
{
	GPIConnection *iconnection;
	
	unsigned int sent;
	unsigned int total;
	unsigned int remaining;

	assert(peer->outputBuffer.buffer != NULL);
	
	sent = 0;
	iconnection = (GPIConnection *)*connection;
	remaining = (unsigned int)stringLen;
	total = 0;

	// Check for nothing to send.
	/////////////////////////////
	if(stringLen == 0)
		return GP_NO_ERROR;

	// Only try to send if the buffer is empty and there are no messages.
	/////////////////////////////////////////////////////////////////////
	if(!(peer->outputBuffer.len - peer->outputBuffer.pos) && !ArrayLength(peer->messages))
	{
		if ((int)remaining <= (gsUdpEngineGetPeerOutBufferFreeSpace(peer->ip, peer->port) - GS_UDP_RELIABLE_MSG_HEADER - GS_UDP_MSG_HEADER_LEN))
		{
			gsUdpEngineSendMessage(peer->ip, peer->port, iconnection->mHeader, (unsigned char *)string, remaining, gsi_true);
			total = remaining;
			remaining = 0;
		}
		else
		{
			unsigned int freeSpace = (unsigned int)gsUdpEngineGetPeerOutBufferFreeSpace(peer->ip, peer->port);
			if (freeSpace > (GS_UDP_MSG_HEADER_LEN + GS_UDP_RELIABLE_MSG_HEADER))
			{	
				sent = freeSpace - (GS_UDP_MSG_HEADER_LEN + GS_UDP_RELIABLE_MSG_HEADER);
				gsUdpEngineSendMessage(peer->ip, peer->port, iconnection->mHeader, (unsigned char *)string, 
					sent, gsi_true);
				total = sent;
				remaining -= sent;
			}
		}

	}

	// Buffer what wasn't sent.
	///////////////////////////
	if(remaining)
		CHECK_RESULT(gpiAppendStringToBufferLen(connection, &peer->outputBuffer, &string[total], (int)remaining));

	return GP_NO_ERROR;
}

/*
GPResult
gpiSendOrBufferStringLen(
						 GPConnection * connection,
						 GPIPeer_st peer,
						 const char * string,
						 int stringLen
						 )
{
	
	GPIBool closed;
	int sent;
	int total;
	int remaining;
	
	
	assert(peer->outputBuffer.buffer != NULL);

	remaining = stringLen;
	total = 0;

	// Check for nothing to send.
	/////////////////////////////
	if(stringLen == 0)
		return GP_NO_ERROR;

	// Only try to send if the buffer is empty and there are no messages.
	/////////////////////////////////////////////////////////////////////
	if(!(peer->outputBuffer.len - peer->outputBuffer.pos) && !ArrayLength(peer->messages))
	{
		do
		{
			CHECK_RESULT(gpiSendData(connection, peer->sock, &string[total], remaining, &closed, &sent, "PT"));
			if(sent)
			{
				total += sent;
				remaining -= sent;
			}
		}
		while(sent && remaining);
	}

	// Buffer what wasn't sent.
	///////////////////////////
	if(remaining)
		CHECK_RESULT(gpiAppendStringToBufferLen(connection, &peer->outputBuffer, &string[total], remaining));

	
	GSI_UNUSED(stringLen);
	GSI_UNUSED(string);
	GSI_UNUSED(peer);
	GSI_UNUSED(connection);
	return GP_NO_ERROR;
}
*/

GPResult
gpiSendOrBufferString(
  GPConnection * connection,
  GPIPeer_st peer,
  char * string
)
{
	return gpiSendOrBufferStringLenToPeer(connection, peer, string, (int)strlen(string));
}

GPResult
gpiSendOrBufferInt(
  GPConnection * connection,
  GPIPeer_st peer,
  int num
)
{
	char intValue[16];
	sprintf(intValue,"%d",num);
	return gpiSendOrBufferString(connection, peer, intValue);
}

GPResult
gpiSendOrBufferUInt(
  GPConnection * connection,
  GPIPeer_st peer,
  unsigned int num
)
{
	char intValue[16];
	sprintf(intValue,"%u",num);
	return gpiSendOrBufferString(connection, peer, intValue);
}

GPResult
gpiRecvToBuffer(
  GPConnection * connection,
  SOCKET sock,
  GPIBuffer * inputBuffer,
  int * bytesRead,
  GPIBool * connClosed,
  char id[3]
)
{
	char * buffer;
	int len;
	int size;
	int rcode;
	int total;
	GPIBool closed;

	assert(sock != INVALID_SOCKET);
	assert(inputBuffer != NULL);
	assert(bytesRead != NULL);
	assert(connClosed != NULL);

	// Init locals.
	///////////////
	buffer = inputBuffer->buffer;
	len = inputBuffer->len;
	size = inputBuffer->size;
	total = 0;
	closed = GPIFalse;

	do
	{
		// Check if the buffer needs to be resized.
		///////////////////////////////////////////
		if((len + GPI_READ_SIZE) > size)
		{
			size = (len + GPI_READ_SIZE);
			buffer = (char *)gsirealloc(buffer, (unsigned int)size + 1);
			if(buffer == NULL)
				Error(connection, GP_MEMORY_ERROR, "Out of memory.");
		}

		// Read from the network.
		rcode = recv(sock, &buffer[len], size - len, 0);


		if(gsiSocketIsError(rcode))
		{
			int error = GOAGetLastError(sock);
			if((error != WSAEWOULDBLOCK) && (error != WSAEINPROGRESS) && (error != WSAETIMEDOUT) )
			{
				Error(connection, GP_NETWORK_ERROR, "There was an error reading from a socket.");
			}
		}
		else if(rcode == 0)
		{
			// Check for a closed connection.
			/////////////////////////////////
			closed = GPITrue;
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Comment,
				"RECVXXXX(%s): Connection closed\n", id);
		}
		else
		{
			#if defined(GPI_DUMP_NET_TRAFFIC) && defined(GSI_COMMON_DEBUG)
			{
				static int recvCount;
				char *buf = (char *)gsimalloc((size_t)(rcode + 1));
				memcpy(buf, &buffer[len], (size_t)rcode);
				buf[rcode] = '\0';
				gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_RawDump,
					"RECV%04d(%s): %s\n", recvCount++, id, buf);
				freeclear(buf);
			}
			#elif defined(GSI_COMMON_DEBUG)
			{
				static int recvCount;
				gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_RawDump,
					"RECV%04d(%s): %d\n", recvCount++, id, rcode);
			}
			#endif
			// Update the buffer len.
			/////////////////////////
			len += rcode;

			// Update the total.
			////////////////////
			total += rcode;
		}

		buffer[len] = '\0';
	}
	while((rcode >= 0) && !closed && (total < (128 * 1024)));

	if(total)
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_RawDump,
			"RECVTOTL(%s): %d\n", id, total);
	}
	
	// Set output stuff.
	////////////////////
	inputBuffer->buffer = buffer;
	inputBuffer->len = len;
	inputBuffer->size = size;
	*bytesRead = total;
	*connClosed = closed;

	GSI_UNUSED(id); //to get rid of codewarrior warnings

	return GP_NO_ERROR;
}

GPResult
gpiSendFromBuffer(
  GPConnection * connection,
  SOCKET sock,
  GPIBuffer * outputBuffer,
  GPIBool * connClosed,
  GPIBool clipSentData,
  char id[3]
)
{
	GPIBool closed;
	int sent;
	int total;
	int remaining;
	char * buffer;
	int pos;
	int len;

	assert(outputBuffer != NULL);

	buffer = outputBuffer->buffer;
	len = outputBuffer->len;
	pos = outputBuffer->pos;
	remaining = (len - pos);
	total = 0;

	// Check for nothing to send.
	/////////////////////////////
	if(remaining == 0)
		return GP_NO_ERROR;

	do
	{
		CHECK_RESULT(gpiSendData(connection, sock, &buffer[pos + total], remaining, &closed, &sent, id));
		if(sent)
		{
			total += sent;
			remaining -= sent;
		}
	}
	while(sent && remaining);

	if(clipSentData)
	{
		if(total > 0)
		{
			memmove(buffer, &buffer[total], (unsigned int)remaining + 1);
			len -= total;
		}
	}
	else
	{
		pos += total;
	}

	assert(len >= 0);
	assert(pos >= 0);
	assert(pos <= len);

	// Set outputs.
	///////////////
	outputBuffer->len = len;
	outputBuffer->pos = pos;
	if(connClosed)
		*connClosed = closed;

	return GP_NO_ERROR;
}

GPResult gpiSendBufferToPeer(GPConnection * connection, unsigned int ip, unsigned short port, 
							 GPIBuffer * outputBuffer, GPIBool *closed, GPIBool clipSentData)
{
	GPIConnection *iconnection = (GPIConnection *)*connection;
	//GPIBool closed;
	unsigned int remaining;
	unsigned char * buffer;
	unsigned int pos;
	unsigned int len;
	unsigned int total = 0;
	GSUdpPeerState aPeerState;
	assert(outputBuffer != NULL);

	buffer = (unsigned char *)outputBuffer->buffer;
	len = (unsigned int)outputBuffer->len;
	pos = (unsigned int)outputBuffer->pos;
	remaining = (len - pos);

	// Check for nothing to send.
	/////////////////////////////
	if(remaining == 0)
		return GP_NO_ERROR;

	// length of message remaining must be smaller than total buffer size minus gt2 reliable msg header size minus
	// in order to send the message in one shot.
	if ((int)remaining <= (gsUdpEngineGetPeerOutBufferFreeSpace(ip, port) - GS_UDP_RELIABLE_MSG_HEADER - GS_UDP_MSG_HEADER_LEN))
	{
		gsUdpEngineSendMessage(ip, port, iconnection->mHeader, &buffer[pos], remaining, gsi_true);
		total = remaining;
		remaining = 0;
	}
	else
	{
		unsigned int freeSpace =0;
		unsigned int sendAmount = 0;
		do 
		{
			freeSpace = (unsigned int)gsUdpEngineGetPeerOutBufferFreeSpace(ip, port);
			sendAmount = freeSpace - (GS_UDP_MSG_HEADER_LEN + GS_UDP_RELIABLE_MSG_HEADER);
			if (sendAmount <= (GS_UDP_MSG_HEADER_LEN + GS_UDP_RELIABLE_MSG_HEADER))
				break;
			if (gsUdpEngineSendMessage(ip, port, iconnection->mHeader, &buffer[pos+total], sendAmount, gsi_true) == GS_UDP_SEND_FAILED)
				break;
			total += sendAmount;
			remaining -= sendAmount;
		}while (remaining);
	}

	if(clipSentData)
	{
		if (total > 0)
		{
			memmove(buffer, &buffer[total], remaining + 1);
			len -= total;
		}
	}
	else
	{
		pos += total;
	}
	// Set outputs.
	///////////////
	outputBuffer->len = (int)len;
	outputBuffer->pos = (int)pos;

	gsUdpEngineGetPeerState(ip, port, &aPeerState);
	if (aPeerState == GS_UDP_PEER_CLOSED)
		*closed = GPITrue;
	else 
		*closed = GPIFalse;

	return GP_NO_ERROR;
}

GPResult
gpiReadMessageFromBuffer(
  GPConnection * connection,
  GPIBuffer * inputBuffer,
  char ** message,
  int * type,
  int * plen
)
{
	char * str;
	int len;
	char intValue[16];

	// Default.
	///////////
	*message = NULL;

	// Check for not enough data.
	/////////////////////////////
	if(inputBuffer->len < 5)
		return GP_NO_ERROR;

	// Find the end of the header.
	//////////////////////////////
	str = strchr(inputBuffer->buffer, '\n');
	if(str != NULL)
	{
		// Check that this is the msg.
		//////////////////////////////
		if(strncmp(str - 5, "\\msg\\", 5) != 0)
			return GP_NETWORK_ERROR;

		// Cap the header.
		//////////////////
		*str = '\0';

		// Read the header.
		///////////////////
		if(!gpiValueForKey(inputBuffer->buffer, "\\m\\", intValue, sizeof(intValue)))
			return GP_NETWORK_ERROR;
		*type = atoi(intValue);

		// Get the length.
		//////////////////
		if(!gpiValueForKey(inputBuffer->buffer, "\\len\\", intValue, sizeof(intValue)))
			return GP_NETWORK_ERROR;
		len = atoi(intValue);
		len++;

		// Is the whole message available?
		//////////////////////////////////
		if(inputBuffer->len > ((str - inputBuffer->buffer) + len))
		{
			// Does it not end with a NUL?
			//////////////////////////////
			if(str[len] != '\0')
				return GP_NETWORK_ERROR;

			// Set the message stuff.
			/////////////////////////
			*message = &str[1];
			*plen = (len - 1);

			// Set the position to the end of the message.
			//////////////////////////////////////////////
			inputBuffer->pos = ((str - inputBuffer->buffer) + len + 1);
		}
		else
		{
			// Put the LF back.
			///////////////////
			*str = '\n';
		}
	}

	GSI_UNUSED(connection);
	return GP_NO_ERROR;
}

GPResult
gpiClipBufferToPosition(
  GPConnection * connection,
  GPIBuffer * buffer
)
{
	if(!buffer || !buffer->buffer || !buffer->pos)
		return GP_NO_ERROR;

	buffer->len -= buffer->pos;
	if(buffer->len)
		memmove(buffer->buffer, buffer->buffer + buffer->pos, (unsigned int)buffer->len);
	buffer->buffer[buffer->len] = '\0';
	buffer->pos = 0;

	GSI_UNUSED(connection);
	return GP_NO_ERROR;
}
