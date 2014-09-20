/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#include "gt2Socket.h"
#include "gt2Buffer.h"
#include "gt2Message.h"
#include "gt2Connection.h"
#include "gt2Utility.h"
#include "gt2Callback.h"
#include <stdlib.h>

#ifdef GSI_ADHOC
// External functions defined at the platform specific level
// to be moved to nonport or socket.h
extern int	_NetworkAdHocSocketCreate(gsi_u16 port);
extern void _NetworkAdHocSocketDestroy	(	int adhoc_socket);
extern int  _NetworkAdHocSocketSendTo	(	int adhoc_socket,
											const void *data,
											int			len,
											int			flags,
											const char *dest_addr,
											gsi_u16		dest_port
										);
extern void	NetAdhocMacGet(char *mac);
#endif

#define GTI2_DEFAULT_INCOMING_BUFFER_SIZE    (64 * 1024)
#define GTI2_DEFAULT_OUTGOING_BUFFER_SIZE    (64 * 1024)

#define STARTING_SERIAL_NUMBER   0

static int gti2ConnectionHash(const void * elem, int numBuckets)
{
	GT2Connection connection = *(GT2Connection *)elem;

	return (int)(((connection->ip * connection->port)) % numBuckets);
}

static int GS_STATIC_CALLBACK gti2ConnectionCompare(const void * elem1, const void * elem2)
{
	GT2Connection connection1 = *(GT2Connection *)elem1;
	GT2Connection connection2 = *(GT2Connection *)elem2;

	if(connection1->ip != connection2->ip)
		return (int)(connection1->ip - connection2->ip);

	return (int)(short)(connection1->port - connection2->port);
}

static void gti2ConnectionFree(void * elem)
{
	gti2ConnectionCleanup(*(GT2Connection *)elem);
}

GT2Connection gti2SocketFindConnection(GT2Socket socket, unsigned int ip, unsigned short port)
{
	GTI2Connection connection;
	GT2Connection connectionPtr;
	GT2Connection * connectionPtrPtr;

	connection.ip = ip;
	connection.port = port;

	connectionPtr = &connection;
	connectionPtrPtr = (GT2Connection *)TableLookup(socket->connections, &connectionPtr);
	if(connectionPtrPtr)
		return *connectionPtrPtr;

	return NULL;
}

GT2Result gti2CreateSocket
(
	GT2Socket * sock,
	const char * localAddress,
	int outgoingBufferSize,
	int incomingBufferSize,
	gt2SocketErrorCallback callback,
	GTI2ProtocolType type
)
{
	SOCKADDR_IN address;
	GT2Socket socketTemp;
	int rcode;
	unsigned int ip;
	unsigned short port;
	int len;

	// startup the sockets engine if needed
	SocketStartUp();

	// check for using defaults
	if(!incomingBufferSize)
		incomingBufferSize = GTI2_DEFAULT_INCOMING_BUFFER_SIZE;
	if(!outgoingBufferSize)
		outgoingBufferSize = GTI2_DEFAULT_OUTGOING_BUFFER_SIZE;

	// convert the address to an IP and port
	if(!gt2StringToAddress(localAddress, &ip, &port))
		return GT2AddressError;

	// allocate the object
	socketTemp = (GT2Socket)gsimalloc(sizeof(GTI2Socket));
	if(!socketTemp)
		return GT2OutOfMemory;

	// set initial values
	memset(socketTemp, 0, sizeof(GTI2Socket));
	socketTemp->socket = INVALID_SOCKET;
	socketTemp->incomingBufferSize = incomingBufferSize;
	socketTemp->outgoingBufferSize = outgoingBufferSize;
	socketTemp->socketErrorCallback = callback;

	// create the connections table
	socketTemp->connections = TableNew2(sizeof(GT2Connection), 32, 2, gti2ConnectionHash, gti2ConnectionCompare, NULL);
	if(!socketTemp->connections)
	{
		gsifree(socketTemp);
		return GT2OutOfMemory;
	}

	// create the closed connections list
	socketTemp->closedConnections = ArrayNew(sizeof(GT2Connection), 4, gti2ConnectionFree);
	if(!socketTemp->closedConnections)
	{
		TableFree(socketTemp->connections);
		gsifree(socketTemp);
		return GT2OutOfMemory;
	}

	// create the socket

#ifdef _XBOX
	if (type == GTI2VdpProtocol)

		socketTemp->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_VDP);
	else 
#endif
#ifdef GSI_ADHOC
	if (type == GTI2AdHocProtocol)
	{
		socketTemp->socket = _NetworkAdHocSocketCreate( port);
	}
	else 
#endif 
	socketTemp->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	socketTemp->protocolType	= type;

	if (type == GTI2AdHocProtocol)
	{
		socketTemp->protocolOffset	= 0;
	}
	else
		socketTemp->protocolOffset	= type;

#ifdef _XBOX
	if (type == GTI2UdpProtocol)
	{
		SetSockBroadcast(socketTemp->socket);
		socketTemp->protocolOffset	= type;
	}
#endif 

	if(socketTemp->socket == INVALID_SOCKET)
	{
		TableFree(socketTemp->connections);
		ArrayFree(socketTemp->closedConnections);
		gsifree(socketTemp);
		return GT2NetworkError;
	}

	// bind it
	memset(&address, 0, sizeof(SOCKADDR_IN));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = ip;
	address.sin_port = htons(port);
	if (type != GTI2AdHocProtocol)
	{
		rcode = bind(socketTemp->socket, (SOCKADDR *)&address, sizeof(SOCKADDR_IN));
		if (gsiSocketIsError(rcode))
		{
			closesocket(socketTemp->socket);
			TableFree(socketTemp->connections);
			ArrayFree(socketTemp->closedConnections);
			gsifree(socketTemp);
			return GT2NetworkError;
		}
	}

	// get the ip and port we're bound to
	#ifdef GSI_ADHOC
	if (type == GTI2AdHocProtocol)
	{
		char mac[6];
		NetAdhocMacGet(mac);

		socketTemp->ip = gt2MacToIp(mac);
		socketTemp->port = ntohs(address.sin_port);
	}
	else
	#endif
	{
		len = sizeof(SOCKADDR_IN);
		getsockname(socketTemp->socket, (SOCKADDR *)&address, &len);
		socketTemp->ip = address.sin_addr.s_addr;
		socketTemp->port = ntohs(address.sin_port);
	}

	*sock = socketTemp;

	return GT2Success;
}

void gti2CloseSocket(GT2Socket socket)
{

	if(socket->callbackLevel)
	{
		socket->close = GT2True;
		return;
	}

#ifdef GSI_ADHOC
	if (socket->protocolType == GTI2AdHocProtocol)
	{
		_NetworkAdHocSocketDestroy(socket->socket);
	}
	else
#endif 
	{
		closesocket(socket->socket);
	}
	
	TableFree(socket->connections);
	ArrayFree(socket->closedConnections);
	gsifree(socket);

	SocketShutDown();
}

void gti2Listen(GT2Socket socket, gt2ConnectAttemptCallback callback)
{
	socket->connectAttemptCallback = callback;
}

static GT2Connection gti2CreateConnectionObject(void)
{
	// mj todo: give options of allocating this from a static pool for games with known number of connections.
	return (GT2Connection)gsimalloc(sizeof(GTI2Connection));
}

GT2Result gti2NewSocketConnection(GT2Socket socket, GT2Connection * connection, unsigned int ip, unsigned short port)
{
	GT2Connection connectionPtr = NULL;

	// check if this ip and port already exists
	if(gti2SocketFindConnection(socket, ip, port))
		return GT2DuplicateAddress;

	// allocate a connection object
	connectionPtr = gti2CreateConnectionObject();
	if(!connectionPtr)
		goto out_of_memory;

	// set some basics
	memset(connectionPtr, 0, sizeof(GTI2Connection));
	connectionPtr->ip = ip;
	connectionPtr->port = port;
	connectionPtr->socket = socket;
	connectionPtr->startTime = current_time();
	connectionPtr->lastSend = connectionPtr->startTime;
	connectionPtr->serialNumber = STARTING_SERIAL_NUMBER;
	connectionPtr->expectedSerialNumber = STARTING_SERIAL_NUMBER;

	// allocate the buffers
	if(!gti2AllocateBuffer(&connectionPtr->incomingBuffer, socket->incomingBufferSize))
		goto out_of_memory;
	if(!gti2AllocateBuffer(&connectionPtr->outgoingBuffer, socket->outgoingBufferSize))
		goto out_of_memory;

	// allocate the message arrays
	connectionPtr->incomingBufferMessages = ArrayNew(sizeof(GTI2IncomingBufferMessage), 64, NULL);
	if(!connectionPtr->incomingBufferMessages)
		goto out_of_memory;
	connectionPtr->outgoingBufferMessages = ArrayNew(sizeof(GTI2OutgoingBufferMessage), 64, NULL);
	if(!connectionPtr->outgoingBufferMessages)
		goto out_of_memory;
	
	// allocate the filter arrays
	connectionPtr->sendFilters = ArrayNew(sizeof(gt2SendFilterCallback), 2, NULL);
	if(!connectionPtr->sendFilters)
		goto out_of_memory;
	connectionPtr->receiveFilters = ArrayNew(sizeof(gt2ReceiveFilterCallback), 2, NULL);
	if(!connectionPtr->receiveFilters)
		goto out_of_memory;

	// add it to the table
	TableEnter(socket->connections, &connectionPtr);

	// check that it's in the table (and get the address)
	*connection = gti2SocketFindConnection(socket, ip, port);
	if(!*connection)
		goto out_of_memory;

	return GT2Success;

out_of_memory:

	// there wasn't enough memory, free everything and return the error
	if(connectionPtr)
	{
		gsifree(connectionPtr->incomingBuffer.buffer);
		gsifree(connectionPtr->outgoingBuffer.buffer);
		if(connectionPtr->incomingBufferMessages)
			ArrayFree(connectionPtr->incomingBufferMessages);
		if(connectionPtr->outgoingBufferMessages)
			ArrayFree(connectionPtr->outgoingBufferMessages);
		if(connectionPtr->sendFilters)
			ArrayFree(connectionPtr->sendFilters);
		if(connectionPtr->receiveFilters)
			ArrayFree(connectionPtr->receiveFilters);
		gsifree(connectionPtr);
	}

	return GT2OutOfMemory;
}

void gti2FreeSocketConnection(GT2Connection connection)
{
	// check if we can actually free it
	if(connection->freeAtAcceptReject || connection->callbackLevel)
		return;

	// remove it from the correct list depending on the connect state
	if(connection->state == GTI2Closed)
	{
		int len;
		int i;

		len = ArrayLength(connection->socket->closedConnections);
		for(i = 0 ; i < len ; i++)
		{
			if(connection == *(GT2Connection *)ArrayNth(connection->socket->closedConnections, i))
			{
				ArrayDeleteAt(connection->socket->closedConnections, i);
				return;
			}
		}
	}
	else
	{
		TableRemove(connection->socket->connections, &connection);
	}
}

GT2Bool gti2SocketSend(GT2Socket socket, unsigned int ip, unsigned short port, const GT2Byte * message, int len)
{
	SOCKADDR_IN address;
	int rcode;

#ifdef GTI2_DROP_SEND_RATE
	// drop some percentage of packets and see what happens
	if((rand() % 100) < GTI2_DROP_SEND_RATE)
		return GT2True;
#endif

	// check the message and len
	gti2MessageCheck(&message, &len);

	if (socket->protocolType != GTI2AdHocProtocol)
	{
		#ifndef INSOCK // insock never sets write flag for UDP sockets
			// check if we can't send
			if(!CanSendOnSocket(socket->socket))
				return GT2True;
		#endif
	}

	// do the send
	#ifdef GSI_ADHOC
		if (socket->protocolType == GTI2AdHocProtocol)
		{
			char	mac[6];
			// convert to mac
			gt2IpToMac(ip,mac);
			// change IP address to mac ethernet
			rcode = _NetworkAdHocSocketSendTo(socket->socket, (const char *)message, len, 0, mac, port);
			if(rcode <0)
			{
				gti2SocketError(socket);
				return gsi_false;
			}

			// let the dump handle this
			if(socket->sendDumpCallback)
			{
				if(!gti2DumpCallback(socket, gti2SocketFindConnection(socket, ip, port), ip, port, GT2False, message, len, GT2True))
					return GT2False;
			}
			return gsi_true;
		}

	#endif

	// fill the address structure
	memset(&address, 0, sizeof(SOCKADDR_IN));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = ip;
	address.sin_port = htons(port);
	rcode = sendto(socket->socket, (const char *)message, len, 0, (SOCKADDR *)&address, sizeof(SOCKADDR_IN));
	if (gsiSocketIsError(rcode))
	{
		rcode = GOAGetLastError(socket->socket);
		if(rcode == WSAECONNRESET)
		{
			// handle the reset
			if(!gti2HandleConnectionReset(socket, ip, port))
				return GT2False;
		}
#ifndef SN_SYSTEMS
		else if (rcode == WSAEHOSTUNREACH)
		{
			if (!gti2HandleHostUnreachable(socket, ip, port, GT2True))
				return GT2False;
		}
#endif
		// some systems might return these errors
		else if((rcode == WSAENOBUFS) || (rcode == WSAEWOULDBLOCK))
		{
			return GT2True;
		}
#if defined(SN_SYSTEMS) || defined(_NITRO) || defined(_REVOLUTION)
		// for systems that don't support WSAEHOSTDOWN (EHOSTDOWN)
		else if((rcode != WSAEMSGSIZE))
#else
		else if((rcode != WSAEMSGSIZE) && (rcode != WSAEHOSTDOWN))
#endif
		{
			// fatal socket error
			gti2SocketError(socket);
			return GT2False;
		}
	}
	else
	{
		// let the dump handle this
		if(socket->sendDumpCallback)
		{
			if(!gti2DumpCallback(socket, gti2SocketFindConnection(socket, ip, port), ip, port, GT2False, message, len, GT2True))
				return GT2False;
		}
	}

	return GT2True;
}

static int gti2SocketConnectionsThinkMap(void * elem, void * clientData)
{
	GT2Connection connection = *(GT2Connection *)elem;
	gsi_time now = *(gsi_time *)clientData;

	// only think if we're not closed
	if(connection->state != GTI2Closed)
	{
		// think
		if(!gti2ConnectionThink(connection, now))
			return 0;
	}

	// check for a closed connection
	if((connection->state == GTI2Closed) && !connection->freeAtAcceptReject && !connection->callbackLevel)
		gti2FreeSocketConnection(connection);

	return 1;
}

GT2Bool gti2SocketConnectionsThink(GT2Socket socket)
{
	gsi_time now;

	// get the current time
	now = current_time();

	// go through the list of connections and let them think
	if(TableMapSafe2(socket->connections, gti2SocketConnectionsThinkMap, &now))
		return GT2False;

	return GT2True;
}

void gti2FreeClosedConnections(GT2Socket socket)
{
	int i;
	int len;

	// loop through the closed connections, attempting to free them all
	len = ArrayLength(socket->closedConnections);
	for(i = (len - 1) ; i >= 0 ; i--)
		gti2FreeSocketConnection(*(GT2Connection *)ArrayNth(socket->closedConnections, i));
}

void gti2SocketError(GT2Socket socket)
{
	// if there was already an error, don't go through this again
	if(socket->error)
		return;

	// flag the error
	socket->error = GT2True;

	// first close all the socket's connections
	gt2CloseAllConnectionsHard(socket);

	// call the error callback
	if(!gti2SocketErrorCallback(socket))
		return;

	// close the socket
	gti2CloseSocket(socket);
}
