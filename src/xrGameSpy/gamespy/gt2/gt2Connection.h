/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _GT2_CONNECTION_H_
#define _GT2_CONNECTION_H_

#include "gt2Main.h"

GT2Result gti2NewOutgoingConnection(GT2Socket socket, GT2Connection * connection, unsigned int ip, unsigned short port);
GT2Result gti2NewIncomingConnection(GT2Socket socket, GT2Connection * connection, unsigned int ip, unsigned short port);

GT2Result gti2StartConnectionAttempt
(
	GT2Connection connection,
	const GT2Byte * message,
	int len,
	GT2ConnectionCallbacks * callbacks
);

GT2Bool gti2AcceptConnection(GT2Connection connection, GT2ConnectionCallbacks * callbacks);

void gti2RejectConnection(GT2Connection connection, const GT2Byte * message, int len);

GT2Bool gti2ConnectionSendData(GT2Connection connection, const GT2Byte * message, int len);

GT2Bool gti2ConnectionThink(GT2Connection connection, gsi_time now);

void gti2CloseConnection(GT2Connection connection, GT2Bool hard);

void gti2ConnectionClosed(GT2Connection connection);

void gti2ConnectionCleanup(GT2Connection connection);

#endif
