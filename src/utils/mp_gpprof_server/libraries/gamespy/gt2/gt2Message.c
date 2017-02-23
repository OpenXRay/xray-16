/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#include "gt2Message.h"
#include "gt2Buffer.h"
#include "gt2Connection.h"
#include "gt2Socket.h"
#include "gt2Callback.h"
#include "gt2Utility.h"
#include <stdlib.h>

static unsigned short gti2UShortFromBuffer(const GT2Byte * buffer, int pos)
{
	unsigned short s;
	s = (unsigned short)((buffer[pos] << 8) & 0xFF00);
	pos++;
	s |= buffer[pos];

	return s;
}


static void gti2UShortToBuffer(GT2Byte * buffer, int pos, unsigned short s)
{
	buffer[pos++] = (GT2Byte)((s >> 8) & 0xFF);
	buffer[pos] = (GT2Byte)(s & 0xFF);
}

static int gti2SNDiff(unsigned short SN1, unsigned short SN2)
{
	return (short)(SN1 - SN2);
}

static GT2Bool gti2ConnectionError(GT2Connection connection, GT2Result result, GT2CloseReason reason)
{
	// first check if we're still connecting
	if(connection->state < GTI2Connected)
	{
		// check if the local side is the initiator
		if(connection->initiated)
		{
			// mark it as closed
			gti2ConnectionClosed(connection);

			// call the callback
			if(!gti2ConnectedCallback(connection, result, NULL, 0))
				return GT2False;
		}
		else
		{
			// are we waiting for accept/reject?
			if(connection->state == GTI2AwaitingAcceptReject)
				connection->freeAtAcceptReject = GT2True;

			// mark it as closed
			gti2ConnectionClosed(connection);
		}
	}
	// report the close, as long as we're not already closed
	else if(connection->state != GTI2Closed)
	{
		// mark it as closed
		gti2ConnectionClosed(connection);

		// call the callback
		if(!gti2ClosedCallback(connection, reason))
			return GT2False;
	}

	return GT2True;
}

static GT2Bool gti2ConnectionCommunicationError(GT2Connection connection)
{
	return gti2ConnectionError(connection, GT2NegotiationError, GT2CommunicationError);
}

static GT2Bool gti2ConnectionMemoryError(GT2Connection connection)
{
	// let the other side know
	if(!gti2SendClosed(connection))
		return GT2False;

	return gti2ConnectionError(connection, GT2OutOfMemory, GT2NotEnoughMemory);
}





static GT2Bool gti2HandleESN(GT2Connection connection, unsigned short ESN)
{
	int len;
	int i;
	GTI2OutgoingBufferMessage * message;
	int shortenBy;

	// get the number of messages in the outgoing queue
	len = ArrayLength(connection->outgoingBufferMessages);
	if(!len)
		return GT2True;

	// loop through until we hit one we can't remove
	for(i = 0 ; i < len ; i++)
	{
		// get the message
		message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i);

		// don't stop until we get to the ESN
		if(gti2SNDiff(message->serialNumber, ESN) >= 0)
			break;
	}

	// check for not removing any
	if(i == 0)
		return GT2True;

	// remove the message info structs
	while(i--)
		ArrayDeleteAt(connection->outgoingBufferMessages, i);

	// check how many messages are left
	len = ArrayLength(connection->outgoingBufferMessages);
	if(!len)
	{
		// buffer is empty
		connection->outgoingBuffer.len = 0;
		return GT2True;
	}

	// figure out how much to move everything forward
	message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, 0);
	shortenBy = message->start;

	// do the move on the info structs
	for(i = 0 ; i < len ; i++)
	{
		message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i);
		message->start -= shortenBy;
	}

	// move the actual data
	gti2BufferShorten(&connection->outgoingBuffer, 0, shortenBy);

	return GT2True;
}

static GT2Bool gti2HandleAppUnreliable(GT2Connection connection, GT2Byte * message, int len)
{
	// check the state
	if((connection->state != GTI2Connected) && (connection->state != GTI2Closing))
		return GT2True;

	// do we need to filter it?
	if(ArrayLength(connection->receiveFilters))
	{
		if(!gti2ReceiveFilterCallback(connection, 0, message, len, GT2False))
			return GT2False;
		return GT2True;
	}

	// we received data, call the callback
	if(!gti2ReceivedCallback(connection, message, len, GT2False))
		return GT2False;

	return GT2True;
}

static GT2Bool gti2HandleAppReliable(GT2Connection connection, GT2Byte * message, int len)
{
	// check the state
	if((connection->state != GTI2Connected) && (connection->state != GTI2Closing))
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;
	}
	else
	{
		// do we need to filter it?
		if(ArrayLength(connection->receiveFilters))
		{
			if(!gti2ReceiveFilterCallback(connection, 0, message, len, GT2True))
				return GT2False;
			return GT2True;
		}

		// we received data, call the callback
		if(!gti2ReceivedCallback(connection, message, len, GT2True))
			return GT2False;
	}

	return GT2True;
}

static GT2Bool gti2HandleClientChallenge(GT2Connection connection, GT2Byte * message, int len)
{
	char response[GTI2_RESPONSE_LEN];
	char challenge[GTI2_CHALLENGE_LEN];

	// check the state
	if(connection->state != GTI2AwaitingClientChallenge)
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// make sure the message is long enough
	if(len < GTI2_CHALLENGE_LEN)
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// generate a response to the challenge
	gti2GetResponse((GT2Byte *)response, message);

	// generate our own challenge
	gti2GetChallenge((GT2Byte *)challenge);

	// store what our response will be
	gti2GetResponse((GT2Byte *)connection->response, (GT2Byte *)challenge);

	// send our own challenge
	if(!gti2SendServerChallenge(connection, response, challenge))
		return GT2False;

	// new state
	connection->state = GTI2AwaitingClientResponse;

	return GT2True;
}



static GT2Bool gti2HandleServerChallenge(GT2Connection connection, GT2Byte * message, int len)
{
	char response[GTI2_RESPONSE_LEN];

	// check the state
	if(connection->state != GTI2AwaitingServerChallenge)
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// make sure the message is long enough
	if(len < (GTI2_RESPONSE_LEN + GTI2_CHALLENGE_LEN))
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// check the response
	if(!gti2CheckResponse(message, (GT2Byte *)connection->response))
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// generate our response to the server's challenge
	gti2GetResponse((GT2Byte *)response, message + GTI2_RESPONSE_LEN);

	// send the response, including our intial message
	if(!gti2SendClientResponse(connection, response, connection->initialMessage, connection->initialMessageLen))
		return GT2False;

	// free the initial message
	if(connection->initialMessage)
	{
		gsifree(connection->initialMessage);
		connection->initialMessage = NULL;
	}

	// new state
	connection->state = GTI2AwaitingAcceptance;

	return GT2True;
}

static GT2Bool gti2HandleClientResponse(GT2Connection connection, GT2Byte * message, int len)
{
	int latency;

	// check the state
	if(connection->state != GTI2AwaitingClientResponse)
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// make sure the message is long enough
	if(len < (GTI2_RESPONSE_LEN))
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// check the response
	if(!gti2CheckResponse(message, (GT2Byte *)connection->response))
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// need to make sure the connection didn't just stop listening
	if(!connection->socket->connectAttemptCallback)
	{
		// send them a closed
		if(!gti2SendClosed(connection))
			return GT2False;

		// mark it as closed
		gti2ConnectionClosed(connection);

		return GT2True;
	}

	// new state
	connection->state = GTI2AwaitingAcceptReject;

	// calculate the approx. latency
	latency = (int)(current_time() - connection->challengeTime);

	// it's up to the app now
	if(!gti2ConnectAttemptCallback(connection->socket, connection, connection->ip, connection->port, latency, message + GTI2_RESPONSE_LEN, len - GTI2_RESPONSE_LEN))
		return GT2False;

	return GT2True;
}

static GT2Bool gti2HandleAccept(GT2Connection connection)
{
	// check the state
	if(connection->state != GTI2AwaitingAcceptance)
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// new state
	connection->state = GTI2Connected;

	// call the callback
	if(!gti2ConnectedCallback(connection, GT2Success, NULL, 0))
		return GT2False;

	return GT2True;
}

static GT2Bool gti2HandleReject(GT2Connection connection, GT2Byte * message, int len)
{
	// check the state
	if(connection->state != GTI2AwaitingAcceptance)
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// mark it as closed
	gti2ConnectionClosed(connection);

	// send a closed reply
	if(!gti2SendClosed(connection))
		return GT2False;

	// call the callback
	if(!gti2ConnectedCallback(connection, GT2Rejected, message, len))
		return GT2False;

	return GT2True;
}

static GT2Bool gti2HandleClose(GT2Connection connection)
{
	GT2Bool localClose;

	// send a closed reply
	if(!gti2SendClosed(connection))
		return GT2False;

	// were we attempting to close this connection?
	localClose = (connection->state == GTI2Closing);

	// handle it as an error (so the right callbacks are called)
	if(!gti2ConnectionError(connection, GT2Rejected, localClose?GT2LocalClose:GT2RemoteClose))
		return GT2False;

	return GT2True;
}

static GT2Bool gti2DeliverReliableMessage(GT2Connection connection, GTI2MessageType type, GT2Byte * message, int len)
{
	// bump our ESN
	connection->expectedSerialNumber++;

	if(type == GTI2MsgAppReliable)
	{
		if(!gti2HandleAppReliable(connection, message, len))
			return GT2False;
	}
	else if(type == GTI2MsgClientChallenge)
	{
		if(!gti2HandleClientChallenge(connection, message, len))
			return GT2False;
	}
	else if(type == GTI2MsgServerChallenge)
	{
		if(!gti2HandleServerChallenge(connection, message, len))
			return GT2False;
	}
	else if(type == GTI2MsgClientResponse)
	{
		if(!gti2HandleClientResponse(connection, message, len))
			return GT2False;
	}
	else if(type == GTI2MsgAccept)
	{
		if(!gti2HandleAccept(connection))
			return GT2False;
	}
	else if(type == GTI2MsgReject)
	{
		if(!gti2HandleReject(connection, message, len))
			return GT2False;
	}
	else if(type == GTI2MsgClose)
	{
		if(!gti2HandleClose(connection))
			return GT2False;
	}
	else if(type == GTI2MsgKeepAlive)
	{
		// ignore
	}

	return GT2True;
}
#ifdef WIN32
static int __cdecl gti2IncomingBufferMessageCompare(const void * elem1, const void * elem2)
#else
static int gti2IncomingBufferMessageCompare(const void * elem1, const void * elem2)
#endif
{
	const GTI2IncomingBufferMessage * message1 = (GTI2IncomingBufferMessage *)elem1;
	const GTI2IncomingBufferMessage * message2 = (GTI2IncomingBufferMessage *)elem2;

	return gti2SNDiff(message1->serialNumber, message2->serialNumber);
}

static GT2Bool gti2BufferIncomingMessage(GT2Connection connection, GTI2MessageType type, unsigned short SN, GT2Byte * message, int len, GT2Bool * overflow)
{
	GTI2IncomingBufferMessage messageInfo;
	GTI2IncomingBufferMessage * bufferedMessage;
	int num;
	int i;

	// check the number of messages being held
	num = ArrayLength(connection->incomingBufferMessages);

	// check if this message is already buffered
	for(i = 0 ; i < num ; i++)
	{
		// get the message
		bufferedMessage = (GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, i);

		// check if this is the same message
		if(bufferedMessage->serialNumber == SN)
		{
			*overflow = GT2False;
			return GT2True;
		}

		// check if we've already past the target SN
		if(gti2SNDiff(bufferedMessage->serialNumber, SN) > 0)
			break;
	}

	// check that there's enough space to store the message
	if(gti2GetBufferFreeSpace(&connection->incomingBuffer) < len)
	{
		*overflow = GT2True;
		return GT2True;
	}

	// setup the message info
	messageInfo.start = connection->incomingBuffer.len;
	messageInfo.len = len;
	messageInfo.type = type;
	messageInfo.serialNumber = SN;

	// add it to the list
	ArrayInsertSorted(connection->incomingBufferMessages, &messageInfo, gti2IncomingBufferMessageCompare);

	// make sure the length is one more
	if(ArrayLength(connection->incomingBufferMessages) != (num + 1))
	{
		*overflow = GT2True;
		return GT2True;
	}

	// copy the message into the buffer
	gti2BufferWriteData(&connection->incomingBuffer, message, len);

	// check for sending a nack
	// we want to send one when we think a message or messages were probably dropped
	if(num == 0)
	{
		// if we're the only message in the hold, send a nack
		if(!gti2SendNack(connection, connection->expectedSerialNumber, (unsigned short)(SN - 1)))
			return GT2False;
	}
	else
	{
		GTI2IncomingBufferMessage * msg;

		// are we the highest message?
		msg = (GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, num);
		if(msg->serialNumber == SN)
		{
			GTI2IncomingBufferMessage * prev;
			unsigned short diff;

			// if we're not right after the second-highest SN, the ones in between were probably dropped
			prev = (GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, num - 1);
			diff = (unsigned short)gti2SNDiff(SN, prev->serialNumber);
			if(diff > 1)
			{
				if(!gti2SendNack(connection, (unsigned short)(prev->serialNumber + 1), (unsigned short)(SN - 1)))
					return GT2False;
			}
		}
	}

	*overflow = GT2False;
	return GT2True;
}

static void gti2RemoveHoldMessage(GT2Connection connection, GTI2IncomingBufferMessage * message, int index)
{
	int moveAfter;
	int shortenBy;
	int moveEnd = 0;
	int num;
	int i;

	// save off info about it
	moveAfter = message->start;
	shortenBy = message->len;

	// delete it
	ArrayDeleteAt(connection->incomingBufferMessages, index);

	// loop through and fix up message's stored after this one in the buffer
	// also figure out exactly how much data we'll need to move
	num = ArrayLength(connection->incomingBufferMessages);
	for(i = 0 ; i < num ; i++)
	{
		// check if this message needs to be moved forward
		message = (GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, i);
		if(message->start > moveAfter)
		{
			message->start -= shortenBy;
			moveEnd = max(moveEnd, message->start + message->len);
		}
	}

	// fix up the buffer itself
	gti2BufferShorten(&connection->incomingBuffer, moveAfter, shortenBy);
}

static GT2Bool gti2DeliverHoldMessages(GT2Connection connection)
{
	GTI2IncomingBufferMessage * message;
	int num;
	int i;

restart:

	// loop through the buffered messages, checking if there are any that can now be delivered
	// loop through backwards to ease removal
	num = ArrayLength(connection->incomingBufferMessages);
	for(i = (num - 1) ; i >= 0 ; i--)
	{
		message = (GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, i);

		// we should deliver this if it's what we're expecting
		if(message->serialNumber == connection->expectedSerialNumber)
		{
			// deliver it
			if(!gti2DeliverReliableMessage(connection, message->type, connection->incomingBuffer.buffer + message->start, message->len))
				return GT2False;

			// remove it
			gti2RemoveHoldMessage(connection, message, i);

			// we need to go through this loop again.
			// goto's are evil, but a little evil is good here
			goto restart;
		}
	}

	return GT2True;
}

static void gti2SetPendingAck(GT2Connection connection)
{
	// if there's not a pending ack, set one
	if(!connection->pendingAck)
	{
		connection->pendingAck = GT2True;
		connection->pendingAckTime = current_time();
	}
}

static GT2Bool gti2HandleReliableMessage(GT2Connection connection, GTI2MessageType type, GT2Byte * message, int len)
{
	unsigned short SN;
	unsigned short ESN;
	const int headerLength = connection->socket->protocolOffset + GTI2_MAGIC_STRING_LEN + 1 + 2 + 2;
	GT2Bool overflow;

	// check if it's long enough
	if(len < (connection->socket->protocolOffset + GTI2_MAGIC_STRING_LEN + 1 + 2 + 2))  // magic string + type + SN + ESN
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// get the SN
	SN = gti2UShortFromBuffer(message + connection->socket->protocolOffset, GTI2_MAGIC_STRING_LEN + 1);

	// get the ESN
	ESN = gti2UShortFromBuffer(message + connection->socket->protocolOffset, GTI2_MAGIC_STRING_LEN + 3);

	// update the message and length to point to the actual message
	if (connection->socket->protocolType == GTI2VdpProtocol && type == GTI2MsgAppReliable)
	{
		message[connection->socket->protocolOffset + GTI2_MAGIC_STRING_LEN + 3] = message[0];
		message[connection->socket->protocolOffset + GTI2_MAGIC_STRING_LEN + 4] = message[1];
		message += headerLength - connection->socket->protocolOffset;
		len -= headerLength - connection->socket->protocolOffset;
	}
	else
	{
		message += headerLength;
		len -= headerLength;
	}
	
	// handle the ESN
	if(!gti2HandleESN(connection, ESN))
		return GT2False;

	// check if it's the SN we expected
	if(SN == connection->expectedSerialNumber)
	{
		// make sure we ack this message
		// do this before delivering, because we might send an ack as part of a reliable reply
		gti2SetPendingAck(connection);

		// deliver the message
		if(!gti2DeliverReliableMessage(connection, type, message, len))
			return GT2False;

		// check if there are any messages in the hold that can now be delivered
		if(!gti2DeliverHoldMessages(connection))
			return GT2False;

		return GT2True;
	}

	// check if the message is a duplicate
	if(gti2SNDiff(SN, connection->expectedSerialNumber) < 0)
	{
		// it's a duplicate, ack it again
		gti2SetPendingAck(connection);

		// ignore it
		return GT2True;
	}

	// we can't deliver this yet, so put it in the hold
	if(!gti2BufferIncomingMessage(connection, type, SN, message, len, &overflow))
		return GT2False;

	// check for a buffer overflow
	if(overflow)
	{
		if(!gti2ConnectionMemoryError(connection))
			return GT2False;
	}

	return GT2True;
}

static GT2Bool gti2HandleAck(GT2Connection connection, const GT2Byte * message, int len)
{
	unsigned short ESN;

	// make sure it has enough space for the ESN
	if(len != 2)
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// get the ESN
	ESN = gti2UShortFromBuffer(message, 0);

	// handle it
	if(!gti2HandleESN(connection, ESN))
		return GT2False;

	return GT2True;
}

static GT2Bool gti2HandleNack(GT2Connection connection, const GT2Byte * message, int len)
{
	unsigned short SNMin;
	unsigned short SNMax;
	int num;
	int i;
	GTI2OutgoingBufferMessage * messageInfo;

	// read based on length.
	SNMin = gti2UShortFromBuffer(message, 0);
	if(len == 2)
	{
		SNMax = SNMin;
	}
	else if(len == 4)
	{
		SNMax = gti2UShortFromBuffer(message, 2);
	}
	else
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}

	// loop through the messages, resending any specified ones
	num = ArrayLength(connection->outgoingBufferMessages);
	for(i = 0 ; i < num ; i++)
	{
		messageInfo = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i);
		if((gti2SNDiff(messageInfo->serialNumber, SNMin) >= 0) && (gti2SNDiff(messageInfo->serialNumber, SNMax) <= 0))
		{
			if(!gti2ResendMessage(connection, messageInfo))
				return GT2False;
		}
	}

	return GT2True;
}

static GT2Bool gti2HandlePing(GT2Connection connection, GT2Byte * message, int len)
{
	// send it right back
	return gti2SendPong(connection, message, len);
}

static GT2Bool gti2HandlePong(GT2Connection connection, const GT2Byte * message, int len)
{
	gsi_time startTime;

	// do we care about this?
	if(!connection->callbacks.ping)
		return GT2True;

	// is this a pong we're interested in?
	// "time" + ping-sent-time
	if(len != (4 + sizeof(gsi_time)))
		return GT2True;
	if(memcmp(message, "time", 4) != 0)
		return GT2True;

	// get the start time
	memcpy(&startTime, message + 4, sizeof(gsi_time));

	// call the callback
	if(!gti2PingCallback(connection, (int)(current_time() - startTime)))
		return GT2False;

	return GT2True;
}

static GT2Bool gti2HandleClosed(GT2Connection connection)
{
	GT2Bool localClose;

	// are we already closed?
	if(connection->state == GTI2Closed)
		return GT2True;

	// were we attempting to close this connection?
	localClose = (connection->state == GTI2Closing);

	// handle it as an error (so the right callbacks are called)
	if(!gti2ConnectionError(connection, GT2Rejected, localClose?GT2LocalClose:GT2RemoteClose))
		return GT2False;

	return GT2True;
}

static GT2Bool gti2HandleUnreliableMessage(GT2Connection connection, GTI2MessageType type, GT2Byte * message, int len)
{
	int headerLength;
	GT2Byte * dataStart;
	int dataLen;

	// most unreliable messages don't need the header
	headerLength = (connection->socket->protocolOffset + GTI2_MAGIC_STRING_LEN + 1);
	dataStart = (message + headerLength);
	dataLen = (len - headerLength);
	
	// handle unreliable messages based on type
	if(type == GTI2MsgAck)
	{
		if(!gti2HandleAck(connection, dataStart, dataLen))
			return GT2False;
	}
	else if(type == GTI2MsgNack)
	{
		if(!gti2HandleNack(connection, dataStart, dataLen))
			return GT2False;
	}
	else if(type == GTI2MsgPing)
	{
		if(!gti2HandlePing(connection, message, len))
			return GT2False;
	}
	else if(type == GTI2MsgPong)
	{
		if(!gti2HandlePong(connection, dataStart, dataLen))
			return GT2False;
	}
	else if(type == GTI2MsgClosed)
	{
		if(!gti2HandleClosed(connection))
			return GT2False;
	}

	return GT2True;
}

// VDP sockets have data length which needs to be stripped off
static GT2Bool gti2HandleMessage(GT2Socket socket, GT2Byte * message, int len, unsigned int ip, unsigned short port)
{
	GT2Connection connection;
	GT2Bool magicString;
	GT2Result result;
	GTI2MessageType type;
	GT2Bool handled;
	int actualLength = len - socket->protocolOffset;

	// VDP messages have 2 byte header which is removed based on protocol
	GT2Byte *actualMessage = message + socket->protocolOffset;
	
	// find out if we have an existing connection for this address
	connection = gti2SocketFindConnection(socket, ip, port);

	// let the dump handle this
	if(socket->receiveDumpCallback)
	{
		if(!gti2DumpCallback(socket, connection, ip, port, GT2False, message, len, GT2False))
			return GT2False;
	}
	
	// check if the message starts with the magic string
	// use greater than for the len compare because it also must have a type
	magicString = ((actualLength > GTI2_MAGIC_STRING_LEN) && (memcmp(actualMessage, GTI2_MAGIC_STRING, GTI2_MAGIC_STRING_LEN) == 0));
	
	// check if we don't have a connection
	if(!connection)
	{
		// if we don't know who this is from, let the unrecognized message callback have first crack at it
		if(!gti2UnrecognizedMessageCallback(socket, ip, port, message, len, &handled))
			return GT2False;

		// if they handled it, we don't care about it.
		if(handled)
			return GT2True;

		// if this isn't a connection request, tell them the connection is closed
		if(!magicString || (actualMessage[GTI2_MAGIC_STRING_LEN] != GTI2MsgClientChallenge))
		{
			// if this is a closed message, don't send one back (to avoid recursion)
			if(!magicString || (actualMessage[GTI2_MAGIC_STRING_LEN] != GTI2MsgClosed))
			{
				if(!gti2SendClosedOnSocket(socket, ip, port))
					return GT2False;
			}
			return GT2True;
		}	
		
		// if we're not listening, we just ignore this
		if(!socket->connectAttemptCallback)
			return GT2True;

		// create a connection
		result = gti2NewIncomingConnection(socket, &connection, ip, port);
		if(result != GT2Success)
		{
			// as long as this wasn't a duplicate address error, tell them we're closed
			// in the case of duplicates, we don't want to close the existing connection
			if(result != GT2DuplicateAddress)
			{
				if(!gti2SendClosedOnSocket(socket, ip, port))
					return GT2False;
			}
			return GT2True;
		}
	}

	// is the connection already closed?
	if(connection->state == GTI2Closed)
	{
		// if this is a closed message, don't send one back (to avoid recursion)
		if(!magicString || (actualMessage[GTI2_MAGIC_STRING_LEN] != GTI2MsgClosed))
		{
			if(!gti2SendClosed(connection))
				return GT2False;
		}

		return GT2True;
	}
	
	// check if this is an unreliable app message with a magic string header
	if(magicString && ((actualLength >= (GTI2_MAGIC_STRING_LEN * 2)) && (memcmp(actualMessage + GTI2_MAGIC_STRING_LEN, GTI2_MAGIC_STRING, GTI2_MAGIC_STRING_LEN) == 0)))
	{
		message[3] = message[1];
		message[2] = message[0];
		message = actualMessage;
		actualMessage += socket->protocolOffset;
		actualLength -= socket->protocolOffset;
		len -= GTI2_MAGIC_STRING_LEN;
		magicString = GT2False;
	}
	
	// if it doesn't have a magic string it's an app unreliable
	if(!magicString)
	{
		// First determine if the connection found has gone throught the internal challenge response
		if (connection->state < GTI2Connected)
		{
			// pass any message that doesn't have a magic string to 
			// the app so that the SDK doesn't drop them
			if(!gti2UnrecognizedMessageCallback(socket, ip, port, message, len, &handled))
				return GT2False;
		}
		else
		{
			if(!gti2HandleAppUnreliable(connection, message, len))
				return GT2False;
		}

		return GT2True;
	}

	// get the message type
	type = (GTI2MessageType)actualMessage[GTI2_MAGIC_STRING_LEN];
	// check for a bad type
	/*if(type < 0)
	{
		if(!gti2ConnectionCommunicationError(connection))
			return GT2False;

		return GT2True;
	}*/


	// check if it's reliable
	if(type < GTI2NumReliableMessages)
	{
		// handle it
		if(!gti2HandleReliableMessage(connection, type, message, len))
			return GT2False;
		return GT2True;
	}

	// handle unreliable messages
	if(!gti2HandleUnreliableMessage(connection, type, message, len))
		return GT2False;

	return GT2True;
}


GT2Bool gti2HandleConnectionReset(GT2Socket socket, unsigned int ip, unsigned short port)
{
	GT2Connection connection;

	// find the connection for the reset
	connection = gti2SocketFindConnection(socket, ip, port);

	// let the dump know about this
	if(socket->receiveDumpCallback)
	{
		if(!gti2DumpCallback(socket, connection, ip, port, GT2True, NULL, 0, GT2False))
			return GT2False;
	}

	// there's no connection, so ignore it
	if(!connection)
		return GT2True;

	// are we waiting for a response from the server?
	if(connection->state == GTI2AwaitingServerChallenge)
	{
		// are we still within the timeout time?
		if(!connection->timeout || ((current_time() - connection->startTime) < connection->timeout))
			return GT2True;

		// report this as a timeout
		if(!gti2ConnectionError(connection, GT2TimedOut, GT2RemoteClose))
			return GT2False;
	}
	else
	{
		// report the error
		if(!gti2ConnectionError(connection, GT2Rejected, GT2RemoteClose))
			return GT2False;
	}

	return GT2True;
}

GT2Bool gti2HandleHostUnreachable(GT2Socket socket, unsigned int ip, unsigned short port, GT2Bool send)
{
	GT2Connection connection;

	// find the connection for the reset
	connection = gti2SocketFindConnection(socket, ip, port);

	// let the dump know about this
	if(socket->receiveDumpCallback)
	{
		if(!gti2DumpCallback(socket, connection, ip, port, GT2True, NULL, 0, send))
			return GT2False;
	}

	// there's no connection, so ignore it
	if(!connection)
		return GT2True;


	// report the error
	if(!gti2ConnectionError(connection, GT2TimedOut, GT2RemoteClose))
		return GT2False;

	return GT2True;
}


#ifdef GSI_ADHOC



// return length if successful
// <=0 on error
gsi_bool _NetworkAdHocSocketRecv(int socket_id,
							char	*buf,
							int		bufferlen,
							int		flags,
							char	*saddr,		//struct SceNetEtherAddr  = char[6];
							gsi_u16 *sport);








// return 0 if no data, -1 if error,  >0 if data to read
int _NetworkAdHocCanReceiveOnSocket(int socket_id);

GT2Bool gti2ReceiveAdHocMessages(GT2Socket socket,char *buffer, int buffersize)
{
	int rcode;
	SOCKADDR_IN address;
	int addressLen;//, datasize;

	// check for messages
	while	(1)
	{
		int datasize =  _NetworkAdHocCanReceiveOnSocket(socket->socket);
		if (datasize < 0)	// error
		{
			gti2SocketError(socket);
			return GT2False;
		}

		if (datasize == 0)
			break;		// no data
		{
			// We have data to recv
			// receive the message
			char		mac[6];
			gsi_u16		port;
			//gsi_u32		ip;

			addressLen = sizeof(address);

			rcode = _NetworkAdHocSocketRecv(socket->socket, buffer,buffersize , 0, mac,&port);
			if(rcode < 0)	// fatal socket error
			{
				#if(0)	// notes
					if(0)//rcode == WSAECONNRESET)
					{
							// handle the reset
							if(!gti2HandleConnectionReset(socket, address.sin_addr.s_addr, ntohs(address.sin_port)))
								return GT2False;
					}
					else 
					if (rcode == WSAEHOSTUNREACH)
					{
						if (!gti2HandleHostUnreachable(socket, address.sin_addr.s_addr, ntohs(address.sin_port), GT2False))
							return GT2False;
					}			
					else
				#endif
					{
						gti2SocketError(socket);
						return GT2False;
					}
			}
			if(rcode == 0)	// no data
			{
				return GT2False;
			}

			// at this point we have valid data

			// change ethernet to IP address
			address.sin_addr.s_addr = gt2MacToIp(mac);
			address.sin_port = port;

			#ifdef RECV_LOG
				// log it
				gti2LogMessage(address.sin_addr.s_addr, ntohs(address.sin_port),
					socket->ip, socket->port,
					buffer, rcode);
			#endif
			// handle the message
			if(!gti2HandleMessage(socket, (GT2Byte *)buffer, rcode, address.sin_addr.s_addr, address.sin_port))
				return GT2False;
		}
	}

	return GT2True;
}
#endif

GT2Bool gti2ReceiveMessages(GT2Socket socket)
{
	int rcode;
	SOCKADDR_IN address;
	int addressLen;


	// avoid overflowing stack
	#if (GTI2_STACK_RECV_BUFFER_SIZE > 1600)
		static char buffer[GTI2_STACK_RECV_BUFFER_SIZE];
	#else
		char buffer[GTI2_STACK_RECV_BUFFER_SIZE];
	#endif


	#ifdef GSI_ADHOC
	if(socket->protocolType == GTI2AdHocProtocol)
	{
		return gti2ReceiveAdHocMessages(socket,buffer,GTI2_STACK_RECV_BUFFER_SIZE);
	}
	#endif

	// check for messages
	while	(CanReceiveOnSocket(socket->socket))
	{
		// mj todo: get this plat specific stuff out of here.  Belongs in play specific layer.
		// abstract recvfrom

		// receive the message
		addressLen = sizeof(address);
		
		rcode = recvfrom(socket->socket, buffer, sizeof(buffer), 0, (SOCKADDR *)&address, &addressLen);
		
		if (gsiSocketIsError(rcode))
		{
			rcode = GOAGetLastError(socket->socket);
			if(rcode == WSAECONNRESET)
			{
				// handle the reset
				if(!gti2HandleConnectionReset(socket, address.sin_addr.s_addr, ntohs(address.sin_port)))
					return GT2False;
			}
			#ifndef SN_SYSTEMS
				else if (rcode == WSAEHOSTUNREACH)
				{
					if (!gti2HandleHostUnreachable(socket, address.sin_addr.s_addr, ntohs(address.sin_port), GT2False))
						return GT2False;
				}
			#endif
			else if(rcode != WSAEMSGSIZE)
			{
				// fatal socket error
				gti2SocketError(socket);
				return GT2False;
			}
		}
		else
		{
			#ifdef RECV_LOG
				// log it
				gti2LogMessage(address.sin_addr.s_addr, ntohs(address.sin_port),
					socket->ip, socket->port,
					buffer, rcode);
			#endif
			// handle the message
			if(!gti2HandleMessage(socket, (GT2Byte *)buffer, rcode, address.sin_addr.s_addr, ntohs(address.sin_port)))
				return GT2False;
		}
	}

	return GT2True;
}

static GT2Bool gti2StoreOutgoingReliableMessageInfo(GT2Connection connection, unsigned short SN, int len)
{
	GTI2OutgoingBufferMessage messageInfo;
	int num;

	// setup the message info
	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.start = connection->outgoingBuffer.len;
	messageInfo.len = len;
	messageInfo.serialNumber = SN;
	messageInfo.lastSend = current_time();

	// check the number of messages before we do the add
	num = ArrayLength(connection->outgoingBufferMessages);

	// add it to the list
	ArrayAppend(connection->outgoingBufferMessages, &messageInfo);

	// make sure the length is one more
	if(ArrayLength(connection->outgoingBufferMessages) != (num + 1))
		return GT2False;

	return GT2True;
}

static GT2Bool gti2BeginReliableMessage(GT2Connection connection, GTI2MessageType type, int len, GT2Bool * overflow)
{
	int freeSpace;

	// VDP data length needed in the front of every packet
	unsigned short vdpDataLength = (unsigned short)(len - connection->socket->protocolOffset);
	
	// check how much free space is in the outgoing buffer
	freeSpace = gti2GetBufferFreeSpace(&connection->outgoingBuffer);

	// do we have the space to hold it?
	if(freeSpace < len)
	{
		if(!gti2ConnectionMemoryError(connection))
			return GT2False;

		*overflow = GT2True;
		return GT2True;
	}

	// store the message's info
	if(!gti2StoreOutgoingReliableMessageInfo(connection, connection->serialNumber, len))
	{
		if(!gti2ConnectionMemoryError(connection))
			return GT2False;

		*overflow = GT2True;
		return GT2True;
	}

	// setup the header
	if (connection->socket->protocolType == GTI2VdpProtocol)
		gti2BufferWriteData(&connection->outgoingBuffer, (const GT2Byte *)&vdpDataLength, connection->socket->protocolOffset);
	gti2BufferWriteData(&connection->outgoingBuffer, (const GT2Byte *)GTI2_MAGIC_STRING, GTI2_MAGIC_STRING_LEN);
	gti2BufferWriteByte(&connection->outgoingBuffer, (GT2Byte)type);
	gti2BufferWriteUShort(&connection->outgoingBuffer, connection->serialNumber++);
	gti2BufferWriteUShort(&connection->outgoingBuffer, connection->expectedSerialNumber);

	*overflow = GT2False;
	return GT2True;
}

static GT2Bool gti2EndReliableMessage(GT2Connection connection)
{
	GTI2OutgoingBufferMessage * message;
	int len;

	// the message we're sending is the last one
	len = ArrayLength(connection->outgoingBufferMessages);
	assert(len > 0);
	message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, len - 1);

	// send it
	if(!gti2ConnectionSendData(connection, connection->outgoingBuffer.buffer + message->start, message->len))
		return GT2False;

	// we just did an ack (as part of the message)
	connection->pendingAck = GT2False;

	return GT2True;
}

GT2Bool gti2SendAppReliable(GT2Connection connection, const GT2Byte * message, int len)
{
	int totalLen;
	GT2Bool overflow;

	// magic string + type + SN + ESN + message
	totalLen = (GTI2_MAGIC_STRING_LEN + 1 + 2 + 2 + len);

	// begin the message
	if(!gti2BeginReliableMessage(connection, GTI2MsgAppReliable, totalLen, &overflow))
		return GT2False;
	if(overflow)
		return GT2True;

	// write the message
	gti2BufferWriteData(&connection->outgoingBuffer, message, len);

	// end the message
	if(!gti2EndReliableMessage(connection))
		return GT2False;

	return GT2True;
}

GT2Bool gti2SendClientChallenge(GT2Connection connection, const char challenge[GTI2_CHALLENGE_LEN])
{
	// magic string + type + SN + ESN + challenge
	int totalLen = (GTI2_MAGIC_STRING_LEN + 1 + 2 + 2 + GTI2_CHALLENGE_LEN);
	GT2Bool overflow;

	// begin the message
	if(!gti2BeginReliableMessage(connection, GTI2MsgClientChallenge, totalLen + connection->socket->protocolOffset, &overflow))
		return GT2False;

	if(overflow)
		return GT2True;

	// write the challenge
	gti2BufferWriteData(&connection->outgoingBuffer, (const GT2Byte *)challenge, GTI2_CHALLENGE_LEN);

	// end the message
	if(!gti2EndReliableMessage(connection))
		return GT2False;

	return GT2True;
}

GT2Bool gti2SendServerChallenge(GT2Connection connection, const char response[GTI2_RESPONSE_LEN], const char challenge[GTI2_CHALLENGE_LEN])
{
	// magic string + type + SN + ESN + response + challenge
	int totalLen = (GTI2_MAGIC_STRING_LEN + 1 + 2 + 2 + GTI2_RESPONSE_LEN + GTI2_CHALLENGE_LEN);
	GT2Bool overflow;

	// begin the message
	if(!gti2BeginReliableMessage(connection, GTI2MsgServerChallenge, totalLen + connection->socket->protocolOffset, &overflow))
		return GT2False;

	if(overflow)
		return GT2True;

	// write the response
	gti2BufferWriteData(&connection->outgoingBuffer, (const GT2Byte *)response, GTI2_RESPONSE_LEN);

	// write the challenge
	gti2BufferWriteData(&connection->outgoingBuffer, (const GT2Byte *)challenge, GTI2_CHALLENGE_LEN);

	// end the message
	if(!gti2EndReliableMessage(connection))
		return GT2False;

	// save the time
	connection->challengeTime = connection->lastSend;

	return GT2True;
}

GT2Bool gti2SendClientResponse(GT2Connection connection, const char response[GTI2_RESPONSE_LEN], const char * message, int len)
{
	// magic string + type + SN + ESN + response + message
	int totalLen = (GTI2_MAGIC_STRING_LEN + 1 + 2 + 2 + GTI2_RESPONSE_LEN + len);
	GT2Bool overflow;

	// begin the message
	if(!gti2BeginReliableMessage(connection, GTI2MsgClientResponse, totalLen + connection->socket->protocolOffset, &overflow))
		return GT2False;

	if(overflow)
		return GT2True;

	// write the response
	gti2BufferWriteData(&connection->outgoingBuffer, (const GT2Byte *)response, GTI2_RESPONSE_LEN);

	// write the message
	gti2BufferWriteData(&connection->outgoingBuffer, (const GT2Byte *)message, len);

	// end the message
	if(!gti2EndReliableMessage(connection))
		return GT2False;

	return GT2True;
}

GT2Bool gti2SendAccept(GT2Connection connection)
{
	// magic string + type + SN + ESN
	int totalLen = (GTI2_MAGIC_STRING_LEN + 1 + 2 + 2);
	GT2Bool overflow;

	// begin the message
	if(!gti2BeginReliableMessage(connection, GTI2MsgAccept, totalLen  + connection->socket->protocolOffset, &overflow))
		return GT2False;

	if(overflow)
		return GT2True;

	// end the message
	if(!gti2EndReliableMessage(connection))
		return GT2False;

	return GT2True;
}

GT2Bool gti2SendReject(GT2Connection connection, const GT2Byte * message, int len)
{
	// magic string + type + SN + ESN + message
	int totalLen = (GTI2_MAGIC_STRING_LEN + 1 + 2 + 2 + len);
	GT2Bool overflow;

	// begin the message
	if(!gti2BeginReliableMessage(connection, GTI2MsgReject, totalLen + connection->socket->protocolOffset, &overflow))
		return GT2False;

	if(overflow)
		return GT2True;

	// write the message
	gti2BufferWriteData(&connection->outgoingBuffer, message, len);

	// end the message
	if(!gti2EndReliableMessage(connection))
		return GT2False;

	return GT2True;
}

GT2Bool gti2SendClose(GT2Connection connection)
{
	// magic string + type + SN + ESN
	int totalLen = (GTI2_MAGIC_STRING_LEN + 1 + 2 + 2);
	GT2Bool overflow;

	// begin the message
	if(!gti2BeginReliableMessage(connection, GTI2MsgClose, totalLen + connection->socket->protocolOffset, &overflow))
		return GT2False;

	if(overflow)
		return GT2True;

	// end the message
	if(!gti2EndReliableMessage(connection))
		return GT2False;

	return GT2True;
}

GT2Bool gti2SendKeepAlive(GT2Connection connection)
{
	// magic string + type + SN + ESN
	int totalLen = (GTI2_MAGIC_STRING_LEN + 1 + 2 + 2);
	GT2Bool overflow;

	// begin the message
	if(!gti2BeginReliableMessage(connection, GTI2MsgKeepAlive, totalLen  + connection->socket->protocolOffset, &overflow))
		return GT2False;

	if(overflow)
		return GT2True;

	// end the message
	if(!gti2EndReliableMessage(connection))
		return GT2False;

	return GT2True;
}

GT2Bool gti2SendAppUnreliable(GT2Connection connection, const GT2Byte * message, int len)
{
	int freeSpace;
	int totalLen;
	GT2Byte * start;

	// check if we can send it right away (unreliable that doesn't start with the magic string)
	if((len < GTI2_MAGIC_STRING_LEN) || 
		(memcmp(message + connection->socket->protocolOffset, GTI2_MAGIC_STRING, GTI2_MAGIC_STRING_LEN) != 0))
	{
		if(!gti2ConnectionSendData(connection, message, len))
			return GT2False;

		return GT2True;
	}
	
	// magic string + message
	totalLen = (GTI2_MAGIC_STRING_LEN + len);

	// check how much free space is in the outgoing buffer
	freeSpace = gti2GetBufferFreeSpace(&connection->outgoingBuffer);

	// do we have the space to hold it?
	if(freeSpace < totalLen)
	{
		// just drop it
		return GT2True;
	}

	// store the start of the actual message in the buffer
	start = (connection->outgoingBuffer.buffer + connection->outgoingBuffer.len);

	// Copy the VDP data length if necessary	
	if (connection->socket->protocolType == GTI2VdpProtocol)
		gti2BufferWriteData(&connection->outgoingBuffer, message, 2);
	
	// copy it in, repeating the magic string at the beginning
	gti2BufferWriteData(&connection->outgoingBuffer, (const GT2Byte *)GTI2_MAGIC_STRING, GTI2_MAGIC_STRING_LEN);
	
	// copy the data at the starting position + offset based on the protocol
	gti2BufferWriteData(&connection->outgoingBuffer, message + connection->socket->protocolOffset, 
		(int)(len - connection->socket->protocolOffset));
	
	// do the send
	if(!gti2ConnectionSendData(connection, start, totalLen))
		return GT2False;

	// we don't need to save the message
	gti2BufferShorten(&connection->outgoingBuffer, -1, totalLen);
	
	return GT2True;
}

GT2Bool gti2SendAck(GT2Connection connection)
{
	// always allocate data length + magic string + type + ESN
	// part of the buffer may not be used but more efficience to be on stack
	char buffer[MAX_PROTOCOL_OFFSET + GTI2_MAGIC_STRING_LEN + 1 + 2];
	int pos = 0;
	
	// write the VDP data length
	if (connection->socket->protocolType == GTI2VdpProtocol)
	{
		short dataLength = (GTI2_MAGIC_STRING_LEN + 1 + 2);
		memcpy(buffer, &dataLength, 2);
		pos += 2;
	}

	// write the magic string
	memcpy(buffer + pos, GTI2_MAGIC_STRING, GTI2_MAGIC_STRING_LEN);
	pos += GTI2_MAGIC_STRING_LEN;

	// write the type
	buffer[pos++] = GTI2MsgAck;

	// write the ESN
	gti2UShortToBuffer((GT2Byte *)buffer, pos, connection->expectedSerialNumber);
	pos += 2;
	
	// send it
	if(!gti2ConnectionSendData(connection, (const GT2Byte *)buffer, pos))
		return GT2False;

	// we just did an ack
	connection->pendingAck = GT2False;

	return GT2True;
}


GT2Bool gti2SendNack(GT2Connection connection, unsigned short SNMin, unsigned short SNMax)
{
	// data length + magic string + type + SNMin [+ SNMax]
	// part of the buffer may not be used but more efficience to be on stack
	char buffer[MAX_PROTOCOL_OFFSET + GTI2_MAGIC_STRING_LEN + 1 + 2 + 2];
	int pos = 0;

	// write the VDP data length
	if (connection->socket->protocolType == GTI2VdpProtocol)
	{
		short dataLength = (GTI2_MAGIC_STRING_LEN + 1 + 2 + 2);
		memcpy(buffer, &dataLength, 2);
		pos += 2;
	}

	// write the magic string
	memcpy(buffer + pos, GTI2_MAGIC_STRING, GTI2_MAGIC_STRING_LEN);
	pos += GTI2_MAGIC_STRING_LEN;

	// write the type
	buffer[pos++] = GTI2MsgNack;

	// write the SNMin
	gti2UShortToBuffer((GT2Byte *)buffer, pos, SNMin);
	pos += 2;

	// write the SNMax if it's different
	if(SNMin != SNMax)
	{
		gti2UShortToBuffer((GT2Byte *)buffer, pos, SNMax);
		pos += 2;
	}

	// send it
	if(!gti2ConnectionSendData(connection, (const GT2Byte *)buffer, pos))
		return GT2False;

	return GT2True;
}


GT2Bool gti2SendPing(GT2Connection connection)
{
	// data length + magic string + type + "time" + current time
	// part of the buffer may not be used but more efficience to be on stack
	char buffer[MAX_PROTOCOL_OFFSET + GTI2_MAGIC_STRING_LEN + 1 + 4 + sizeof(gsi_time)];
	int pos = 0;
	gsi_time now;

	// write the VDP data length
	if (connection->socket->protocolType == GTI2VdpProtocol)
	{
		short dataLength = (GTI2_MAGIC_STRING_LEN + 1 + 4 + sizeof(gsi_time));
		memcpy(buffer, &dataLength, 2);
		pos += 2;
	}
	// write the magic string
	memcpy(buffer + pos, GTI2_MAGIC_STRING, GTI2_MAGIC_STRING_LEN);
	pos += GTI2_MAGIC_STRING_LEN;

	// write the type
	buffer[pos++] = GTI2MsgPing;

	// copy the time id
	memcpy(buffer + pos, "time", 4);
	pos += 4;

	// write the current time
	now = current_time();
	memcpy(buffer + pos, &now, sizeof(gsi_time));

	pos += (int)sizeof(gsi_time);
	// send it
	if(!gti2ConnectionSendData(connection, (const GT2Byte *)buffer, pos))
		return GT2False;

	return GT2True;
}


GT2Bool gti2SendPong(GT2Connection connection, GT2Byte * message, int len)
{
	// change the ping to a pong
	message[GTI2_MAGIC_STRING_LEN] = GTI2MsgPong;

	// send it
	return gti2ConnectionSendData(connection, message, len);
}

GT2Bool gti2SendClosed(GT2Connection connection)
{
	// normal close
	return gti2SendClosedOnSocket(connection->socket, connection->ip, connection->port);
}


GT2Bool gti2SendClosedOnSocket(GT2Socket socket, unsigned int ip, unsigned short port)
{
	// Vdp data length (not including voice) + magic string + type
	// part of the buffer may not be used but more efficience to be on stack
	char buffer[MAX_PROTOCOL_OFFSET + GTI2_MAGIC_STRING_LEN + 1]; 
	int pos = 0;

	// write the data length
	if (socket->protocolType == GTI2VdpProtocol)
	{
		short dataLength = GTI2_MAGIC_STRING_LEN + 1;
		memcpy(buffer, &dataLength, 2);
		pos += 2;
	}

	// write the magic string
	memcpy(buffer + pos, GTI2_MAGIC_STRING, GTI2_MAGIC_STRING_LEN);
	pos += GTI2_MAGIC_STRING_LEN;

	// write the type
	buffer[pos++] = GTI2MsgClosed;

	// send it
	if(!gti2SocketSend(socket, ip, port, (const GT2Byte *)buffer, pos))
		return GT2False;

	return GT2True;
}


GT2Bool gti2ResendMessage(GT2Connection connection, GTI2OutgoingBufferMessage * message)
{
	GTI2MessageType type;
	int pos;

	// replace the ESN (it's after the magic string, the type, and the SN)
	pos = (message->start + connection->socket->protocolOffset + GTI2_MAGIC_STRING_LEN + 1 + 2);
	
	gti2UShortToBuffer(connection->outgoingBuffer.buffer, pos, connection->expectedSerialNumber);

	// send the message
	if(!gti2ConnectionSendData(connection, connection->outgoingBuffer.buffer + message->start, message->len))
		return GT2False;

	// update the last time sent
	message->lastSend = connection->lastSend;

	// if it was a server challenge, update that time too
	type = (GTI2MessageType)connection->outgoingBuffer.buffer[message->start + connection->socket->protocolOffset + GTI2_MAGIC_STRING_LEN];
	
	if(type == GTI2MsgServerChallenge)
		connection->challengeTime = connection->lastSend;

	return GT2True;
}

GT2Bool gti2Send(GT2Connection connection, const GT2Byte * message, int len, GT2Bool reliable)
{
	if (reliable)
		return gti2SendAppReliable(connection, message, len);
	//send unreliable messages
	return gti2SendAppUnreliable(connection, message, len);
}

GT2Bool gti2WasMessageIDConfirmed(const GT2Connection connection, GT2MessageID messageID)
{
	GTI2OutgoingBufferMessage * message;
	int len;

	// if there are no reliable messages waiting confirmation, then this has already been confirmed
	len = ArrayLength(connection->outgoingBufferMessages);
	if(!len)
		return GT2True;

	// get the oldest message waiting confirmation
	message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, 0);

	// if the message id we are looking for is older than the first one waiting confirmation,
	// then it has already been confirmed
	if(gti2SNDiff(messageID, message->serialNumber) < 0)
		return GT2True;

	// the message hasn't been confirmed yet
	return GT2False;
}
