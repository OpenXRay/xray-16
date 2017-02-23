/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#include "gt2Main.h"
#include "gt2Socket.h"
#include "gt2Connection.h"
#include "gt2Message.h"
#include "gt2Callback.h"
#include "gt2Filter.h"
#include "gt2Utility.h"

#define GTI2_INVALID_IP_MASK     0xE0000000

/*********************
** SOCKET FUNCTIONS **
*********************/

// Xbox VDP socket function(s).
#ifdef _XBOX
GT2Result gt2CreateVDPSocket
(
	GT2Socket * socket,
	const char * localAddress,
	int outgoingBufferSize,
	int incomingBufferSize,
	gt2SocketErrorCallback callback
)
{
	return gti2CreateSocket(socket, localAddress, outgoingBufferSize, incomingBufferSize, callback, GTI2VdpProtocol);
}
#endif


GT2Result gt2CreateSocket
(
	GT2Socket * socket,
	const char * localAddress,
	int outgoingBufferSize,
	int incomingBufferSize,
	gt2SocketErrorCallback callback
)
{
	return gti2CreateSocket(socket, localAddress, outgoingBufferSize, incomingBufferSize, callback, GTI2UdpProtocol);
}

GT2Result gt2CreateAdHocSocket
(
	GT2Socket * socket,
	const char * localAddress,
	int outgoingBufferSize,
	int incomingBufferSize,
	gt2SocketErrorCallback callback
)
{
	return gti2CreateSocket(socket, localAddress, outgoingBufferSize, incomingBufferSize, callback, GTI2AdHocProtocol);
}


void gt2CloseSocket(GT2Socket socket)
{
	// hard close the connections
	gt2CloseAllConnectionsHard(socket);

	// close the socket
	gti2CloseSocket(socket);
}

void gt2Think(GT2Socket socket)
{
	// check for incoming messages
	if(!gti2ReceiveMessages(socket))
		return;

	// let the connections think
	if(!gti2SocketConnectionsThink(socket))
		return;
	
	// free closed connections
	gti2FreeClosedConnections(socket);
}

GT2Result gt2SendRawUDP
(
	GT2Socket socket,
	const char * remoteAddress,
	const GT2Byte * message,
	int len
)
{
	unsigned int ip;
	unsigned short port;

	// get the ip and port
	if(!gt2StringToAddress(remoteAddress, &ip, &port) || !port)
		return GT2AddressError;

	// check for invalid IP ranges
	// class D (224-239.*, multicast) and class E (240-255.*, experimental)
	if((ntohl(ip) & GTI2_INVALID_IP_MASK) == GTI2_INVALID_IP_MASK)
		return GT2AddressError;

	// check if this is for broadcast
	if(!ip)
	{
		// check if broadcast is enable
		if(!socket->broadcastEnabled)
		{
			if(!SetSockBroadcast(socket->socket))
				return GT2NetworkError;
			socket->broadcastEnabled = GT2True;
		}

		// set the broadcast ip
		ip = gsiGetBroadcastIP();
	}

	// send the datagram
	gti2SocketSend(socket, ip, port, message, len);

	return GT2Success;
}

/*********************
** LISTEN FUNCTIONS **
*********************/

void gt2Listen(GT2Socket socket, gt2ConnectAttemptCallback callback)
{
	gti2Listen(socket, callback);
}

GT2Bool gt2Accept(GT2Connection connection, GT2ConnectionCallbacks * callbacks)
{
	return gti2AcceptConnection(connection, callbacks);
}

void gt2Reject(GT2Connection connection, const GT2Byte * message, int len)
{
	gti2RejectConnection(connection, message, len);
}

/*************************
** CONNECTION FUNCTIONS **
*************************/

GT2Result gt2Connect
(
	GT2Socket socket,
	GT2Connection * connection,
	const char * remoteAddress,
	const GT2Byte * message,
	int len,
	int timeout,
	GT2ConnectionCallbacks * callbacks,
	GT2Bool blocking
)
{
	GT2Connection connectionTemp;
	GT2Result result;
	GT2Bool done;
	unsigned int ip;
	unsigned short port;

	{
		// get the ip and port
		if(!gt2StringToAddress(remoteAddress, &ip, &port) || !ip || !port)
			return GT2AddressError;
	}

	// check for invalid IP ranges
	// class D (224-239.*, multicast) and class E (240-255.*, experimental)
	if((ntohl(ip) & GTI2_INVALID_IP_MASK) == GTI2_INVALID_IP_MASK)
		return GT2AddressError;

	// create the connection object
	result = gti2NewOutgoingConnection(socket, &connectionTemp, ip, port);
	if(result)
		return result;

	// save the timeout value
	connectionTemp->timeout = (unsigned int)timeout;

	// initiate the connection attempt
	result = gti2StartConnectionAttempt(connectionTemp, message, len, callbacks);
	if(result)
	{
		gti2FreeSocketConnection(connectionTemp);
		return result;
	}

	// if not blocking, return now
	if(!blocking)
	{
		if(connection)
			*connection = connectionTemp;
		return GT2Success;
	}

	// we're not really in a callback, but this will prevent the connection
	// from being freed before the loop finishes.
	connectionTemp->callbackLevel++;

	// if blocking, loop until the connect attempt is done
	do
	{
		// think
		gt2Think(socket);

		// check if we're done
		done = (connectionTemp->state >= GTI2Connected);

		// if we're not done, take a rest
		if(!done)
			msleep(1);
	} while(!done);

	// bring the callback level back down
	connectionTemp->callbackLevel--;

	// is it success?
	if(connectionTemp->state == GTI2Connected)
		*connection = connectionTemp;

	return connectionTemp->connectionResult;
}

GT2Result gt2Send
(
	GT2Connection connection,
	const GT2Byte * message,
	int len,
	GT2Bool reliable
)
{
	// used to check for voice data in reliable messages
	unsigned short vdpDataLength;
	
	// can't send a message if not connected
	if(connection->state != GTI2Connected)
		return GT2InvalidConnection;

	// check the message and len
	gti2MessageCheck(&message, &len);
	
	if (reliable && connection->socket->protocolType == GTI2VdpProtocol)
	{
		memcpy(&vdpDataLength, message, sizeof(unsigned short));
		assert(vdpDataLength + connection->socket->protocolOffset == len);
		if (vdpDataLength + connection->socket->protocolOffset != len)
			return GT2InvalidMessage;
	}
		
	// do we need to filter it?
	if(ArrayLength(connection->sendFilters))
	{
		gti2SendFilterCallback(connection, 0, message, len, reliable);
		return GT2Success;
	}

	if (gti2Send(connection, message, len, reliable))
		return GT2Success;
	
	return GT2SendFailed;
}

void gt2Ping(GT2Connection connection)
{
	gti2SendPing(connection);
}

void gt2CloseConnection(GT2Connection connection)
{
	gti2CloseConnection(connection, GT2False);
}

void gt2CloseConnectionHard(GT2Connection connection)
{
	gti2CloseConnection(connection, GT2True);
}

static void gti2CloseAllConnectionsMap(void * elem, void * clientData)
{
	gt2CloseConnection(*(GT2Connection *)elem);
	
	GSI_UNUSED(clientData);
}

void gt2CloseAllConnections(GT2Socket socket)
{
	TableMapSafe(socket->connections, gti2CloseAllConnectionsMap, NULL);
}

static void gti2CloseAllConnectionsHardMap(void * elem, void * clientData)
{
	gt2CloseConnectionHard(*(GT2Connection *)elem);
	
	GSI_UNUSED(clientData);
}

void gt2CloseAllConnectionsHard(GT2Socket socket)
{
	TableMapSafe(socket->connections, gti2CloseAllConnectionsHardMap, NULL);
}

/*************************
** MESSAGE CONFIRMATION **
*************************/
GT2MessageID gt2GetLastSentMessageID(GT2Connection connection)
{
	return (GT2MessageID)(connection->serialNumber - 1);
}

GT2Bool gt2WasMessageIDConfirmed(GT2Connection connection, GT2MessageID messageID)
{
	return gti2WasMessageIDConfirmed(connection, messageID);
}

/*********************
** FILTER FUNCTIONS **
*********************/

GT2Bool gt2AddSendFilter(GT2Connection connection, gt2SendFilterCallback callback)
{
	return gti2AddSendFilter(connection, callback);
}

void gt2RemoveSendFilter(GT2Connection connection, gt2SendFilterCallback callback)
{
	gti2RemoveSendFilter(connection, callback);
}

void gt2FilteredSend(GT2Connection connection, int filterID, const GT2Byte * message, int len, GT2Bool reliable)
{
	gti2FilteredSend(connection, filterID, message, len, reliable);
}

GT2Bool gt2AddReceiveFilter(GT2Connection connection, gt2ReceiveFilterCallback callback)
{
	return gti2AddReceiveFilter(connection, callback);
}

void gt2RemoveReceiveFilter(GT2Connection connection, gt2ReceiveFilterCallback callback)
{
	gti2RemoveReceiveFilter(connection, callback);
}

void gt2FilteredReceive(GT2Connection connection, int filterID, GT2Byte * message, int len, GT2Bool reliable)
{
	gti2FilteredReceive(connection, filterID, message, len, reliable);
}

/*******************
** INFO FUNCTIONS **
*******************/

GT2Socket gt2GetConnectionSocket(GT2Connection connection)
{
	return connection->socket;
}

GT2ConnectionState gt2GetConnectionState(GT2Connection connection)
{
	if(connection->state < GTI2Connected)
		return GT2Connecting;
	if(connection->state == GTI2Connected)
		return GT2Connected;
	if(connection->state == GTI2Closing)
		return GT2Closing;
	return GT2Closed;
}

unsigned int gt2GetRemoteIP(GT2Connection connection)
{
	return connection->ip;
}

unsigned short gt2GetRemotePort(GT2Connection connection)
{
	return connection->port;
}

unsigned int gt2GetLocalIP(GT2Socket socket)
{
	return socket->ip;
}

unsigned short gt2GetLocalPort(GT2Socket socket)
{
	return socket->port;
}

int gt2GetIncomingBufferSize(GT2Connection connection)
{
	return connection->incomingBuffer.size;
}

int gt2GetIncomingBufferFreeSpace(GT2Connection connection)
{
	return (connection->incomingBuffer.size - connection->incomingBuffer.len);
}

int gt2GetOutgoingBufferSize(GT2Connection connection)
{
	return connection->outgoingBuffer.size;
}

int gt2GetOutgoingBufferFreeSpace(GT2Connection connection)
{
	return (connection->outgoingBuffer.size - connection->outgoingBuffer.len);
}

/*****************************
** SOCKET SHARING FUNCTIONS **
*****************************/

SOCKET gt2GetSocketSOCKET(GT2Socket socket)
{
	return socket->socket;
}

void gt2SetUnrecognizedMessageCallback(GT2Socket socket, gt2UnrecognizedMessageCallback callback)
{
	socket->unrecognizedMessageCallback = callback;
}

/************************
** USER DATA FUNCTIONS **
************************/

void gt2SetSocketData(GT2Socket socket, void * data)
{
	assert(socket);

	socket->data = data;
}

void * gt2GetSocketData(GT2Socket socket)
{
	assert(socket);

	return socket->data;
}

void gt2SetConnectionData(GT2Connection connection, void * data)
{
	assert(connection);

	connection->data = data;
}

void * gt2GetConnectionData(GT2Connection connection)
{
	assert(connection);

	return connection->data;
}

/*******************
** DUMP FUNCTIONS **
*******************/

void gt2SetSendDump(GT2Socket socket, gt2DumpCallback callback)
{
	socket->sendDumpCallback = callback;
}

void gt2SetReceiveDump(GT2Socket socket, gt2DumpCallback callback)
{
	socket->receiveDumpCallback = callback;
}
