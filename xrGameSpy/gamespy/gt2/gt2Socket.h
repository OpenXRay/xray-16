/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _GT2_SOCKET_H_
#define _GT2_SOCKET_H_

#include "gt2Main.h"

GT2Result gti2CreateSocket
(
	GT2Socket * socket,
	const char * localAddress,
	int outgoingBufferSize,
	int incomingBufferSize,
	gt2SocketErrorCallback callback,
	GTI2ProtocolType type
);

void gti2CloseSocket(GT2Socket socket);

void gti2Listen(GT2Socket socket, gt2ConnectAttemptCallback callback);

GT2Result gti2NewSocketConnection(GT2Socket socket, GT2Connection * connection, unsigned int ip, unsigned short port);
void gti2FreeSocketConnection(GT2Connection connection);

GT2Connection gti2SocketFindConnection(GT2Socket socket, unsigned int ip, unsigned short port);

// ip is network byte order, port is host byte order
// returns false if there was a fatal error
GT2Bool gti2SocketSend(GT2Socket socket, unsigned int ip, unsigned short port, const GT2Byte * message, int len);

GT2Bool gti2SocketConnectionsThink(GT2Socket socket);

void gti2FreeClosedConnections(GT2Socket socket);

void gti2SocketError(GT2Socket socket);

#endif
