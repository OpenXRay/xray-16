/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#include "gt2Connection.h"
#include "gt2Socket.h"
#include "gt2Message.h"
#include "gt2Callback.h"
#include "gt2Utility.h"
#include <stdlib.h>

GT2Result gti2NewOutgoingConnection(GT2Socket socket, GT2Connection * connection, unsigned int ip, unsigned short port)
{
	GT2Result result;

	// create the object
	result = gti2NewSocketConnection(socket, connection, ip, port);
	if(result != GT2Success)
		return result;

	// set initial states
	(*connection)->state = GTI2AwaitingServerChallenge;
	(*connection)->initiated = GT2True;

	return GT2Success;
}

GT2Result gti2NewIncomingConnection(GT2Socket socket, GT2Connection * connection, unsigned int ip, unsigned short port)
{
	GT2Result result;

	// create the object
	result = gti2NewSocketConnection(socket, connection, ip, port);
	if(result != GT2Success)
		return result;

	// set initial states
	(*connection)->state = GTI2AwaitingClientChallenge;
	(*connection)->initiated = GT2False;

	return GT2Success;
}

GT2Result gti2StartConnectionAttempt
(
	GT2Connection connection,
	const GT2Byte * message,
	int len,
	GT2ConnectionCallbacks * callbacks
)
{
	char challenge[GTI2_CHALLENGE_LEN];

	// check the message and len
	gti2MessageCheck(&message, &len);

	// copy off the message
	if(len > 0)
	{
		connection->initialMessage = (char *)gsimalloc((unsigned int)len);
		if(!connection->initialMessage)
			return GT2OutOfMemory;

		memcpy(connection->initialMessage, message, (unsigned int)len);
		connection->initialMessageLen = len;
	}

	// copy the callbacks
	if(callbacks)
		connection->callbacks = *callbacks;

	// generate a challenge
	gti2GetChallenge((GT2Byte *)challenge);

	// generate and store the expected response
	gti2GetResponse((GT2Byte *)connection->response, (GT2Byte *)challenge);

	// send the client challenge
	gti2SendClientChallenge(connection, challenge);

	// update our state
	connection->state = GTI2AwaitingServerChallenge;

	return GT2Success;
}

GT2Bool gti2AcceptConnection(GT2Connection connection, GT2ConnectionCallbacks * callbacks)
{
	// was the connection already closed?
	if(connection->freeAtAcceptReject)
	{
		// clear the flag
		connection->freeAtAcceptReject = GT2False;

		// let the app know if was already closed
		return GT2False;
	}

	// make sure this flag gets cleared
	connection->freeAtAcceptReject = GT2False;

	// check that we're still awaiting this
	if(connection->state != GTI2AwaitingAcceptReject)
		return GT2False;

	// let the other side know
	gti2SendAccept(connection);

	// update our state
	connection->state = GTI2Connected;

	// store the callbacks
	if(callbacks)
		connection->callbacks = *callbacks;

	return GT2True;
}

void gti2RejectConnection(GT2Connection connection, const GT2Byte * message, int len)
{
	// make sure this flag gets cleared
	connection->freeAtAcceptReject = GT2False;

	// check that we're still awaiting this
	if(connection->state != GTI2AwaitingAcceptReject)
		return;

	// check the message and len
	gti2MessageCheck(&message, &len);

	// let the other side know
	gti2SendReject(connection, message, len);

	// update our state
	connection->state = GTI2Closing;
}

GT2Bool gti2ConnectionSendData(GT2Connection connection, const GT2Byte * message, int len)
{
	// send the data on the socket
	if(!gti2SocketSend(connection->socket, connection->ip, connection->port, message, len))
		return GT2False;

	// mark the time (used for keep-alives)
	connection->lastSend = current_time();

	return GT2True;
}

static GT2Bool gti2CheckTimeout(GT2Connection connection, gsi_time now)
{
	// are we still trying to connect?
	if(connection->state < GTI2Connected)
	{
		GT2Bool timedOut = GT2False;

		// is this the initiator
		if(connection->initiated)
		{
			// do we have a timeout?
			if(connection->timeout)
			{
				// check the time taken against the timeout
				if((now - connection->startTime) > connection->timeout)
					timedOut = GT2True;
			}
		}
		else
		{
			// don't time them out if they're waiting for us
			if(connection->state < GTI2AwaitingAcceptReject)
			{
				// check the time taken against the timeout
				if((now - connection->startTime) > GTI2_SERVER_TIMEOUT)
					timedOut = GT2True;
			}
		}

		// check if we timed out
		if(timedOut)
		{
			// let them know
			gti2SendClosed(connection);

			// mark it as closed
			gti2ConnectionClosed(connection);

			// call the callback
			if(!gti2ConnectedCallback(connection, GT2TimedOut, NULL, 0))
				return GT2False;
		}
	}

	return GT2True;
}

static GT2Bool gti2SendRetries(GT2Connection connection, gsi_time now)
{
	int i;
	int len;
	GTI2OutgoingBufferMessage * message;

	// go through the list of outgoing messages awaiting confirmation
	len = ArrayLength(connection->outgoingBufferMessages);
	for(i = 0 ; i < len ; i++)
	{
		// get the message
		message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i);

		// check if it's time to resend it
		if((now - message->lastSend) > GTI2_RESEND_TIME)
		{
			if(!gti2ResendMessage(connection, message))
				return GT2False;
		}
	}

	return GT2True;
}

static GT2Bool gti2CheckPendingAck(GT2Connection connection, gsi_time now)
{
	// check for nothing pending
	if(!connection->pendingAck)
		return GT2True;

	// check how long it has been pending
	if((now - connection->pendingAckTime) > GTI2_PENDING_ACK_TIME)
	{
		if(!gti2SendAck(connection))
			return GT2False;
	}

	return GT2True;
}

static GT2Bool gti2CheckKeepAlive(GT2Connection connection, gsi_time now)
{
	if((now - connection->lastSend) > GTI2_KEEP_ALIVE_TIME)
	{
		if(!gti2SendKeepAlive(connection))
			return GT2False;
	}

	return GT2True;
}

GT2Bool gti2ConnectionThink(GT2Connection connection, gsi_time now)
{
	// check timeout
	if(!gti2CheckTimeout(connection, now))
		return GT2False;

	// check keep alives
	if(!gti2CheckKeepAlive(connection, now))
		return GT2False;

	// send retries
	if(!gti2SendRetries(connection, now))
		return GT2False;

	// check the pending ack
	if(!gti2CheckPendingAck(connection, now))
		return GT2False;

	return GT2True;
}

void gti2CloseConnection(GT2Connection connection, GT2Bool hard)
{
	// check if it should be hard or soft closed
	if(hard)
	{
		// check if it's already closed
		if(connection->state >= GTI2Closed)
			return;

		// mark it as closed
		gti2ConnectionClosed(connection);

		// send a closed message
		gti2SendClosed(connection);

		// call the callback
		gti2ClosedCallback(connection, GT2LocalClose);

		// try and free it
		gti2FreeSocketConnection(connection);
	}
	else
	{
		// mark it as closing
		connection->state = GTI2Closing;

		// send the close
		gti2SendClose(connection);
	}
}

void gti2ConnectionClosed(GT2Connection connection)
{
	// check for already closed
	if(connection->state == GTI2Closed)
		return;

	// mark the connection as closed
	connection->state = GTI2Closed;

	// remove it from the connected list
	TableRemove(connection->socket->connections, &connection);

	// add it to the closed list
	ArrayAppend(connection->socket->closedConnections, &connection);
}

void gti2ConnectionCleanup(GT2Connection connection)
{
	if(connection->initialMessage)
		gsifree(connection->initialMessage);

	if(connection->incomingBuffer.buffer)
		gsifree(connection->incomingBuffer.buffer);
	if(connection->outgoingBuffer.buffer)
		gsifree(connection->outgoingBuffer.buffer);

	if(connection->incomingBufferMessages)
		ArrayFree(connection->incomingBufferMessages);
	if(connection->outgoingBufferMessages)
		ArrayFree(connection->outgoingBufferMessages);
	
	if(connection->sendFilters)
		ArrayFree(connection->sendFilters);
	if(connection->receiveFilters)
		ArrayFree(connection->receiveFilters);

	gsifree(connection);
}
