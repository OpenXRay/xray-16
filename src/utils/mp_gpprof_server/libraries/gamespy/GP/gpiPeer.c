/*
gpiPeer.c
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/
#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4267) //lines: 275
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS

//INCLUDES
//////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpi.h"

//FUNCTIONS
///////////
static GPResult
gpiProcessPeerInitiatingConnection(
  GPConnection * connection,
  GPIPeer * peer
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	//int state;
	char * str = NULL;
	//int len;
	GPIBool connClosed;
	GPIProfile * pProfile;
	GPResult result;
	GSUdpPeerState aPeerState;
	
	GS_ASSERT(peer);
	if (!peer)
		return GP_NETWORK_ERROR;

	GS_ASSERT(peer->state != GPI_PEER_DISCONNECTED && peer->state != GPI_PEER_NOT_CONNECTED);
	if (peer->state == GPI_PEER_DISCONNECTED || peer->state == GPI_PEER_NOT_CONNECTED)
		return GP_NETWORK_ERROR;
	// Check the state.
	///////////////////
	switch(peer->state)
	{
		case GPI_PEER_GETTING_SIG:
			// Do nothing - we're waiting for getinfo to get the sig.
			/////////////////////////////////////////////////////////
			break;

		case GPI_PEER_GOT_SIG:
		{
			// Start the connect.
			/////////////////////
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_State, GSIDebugLevel_Verbose, "Got the peer signature for profileid: %d\n", peer->profile);
			CHECK_RESULT(gpiPeerStartConnect(connection, peer));

			break;
		}
		case GPI_PEER_CONNECTING:
		{	
			// Check if the connect finished.
			/////////////////////////////////
			/*
			CHECK_RESULT(gpiCheckSocketConnect(connection, peer->sock, &state));
			if(state == GPI_DISCONNECTED)
			{
				Error(connection, GP_NETWORK_ERROR, "Error connecting to a peer.");
			}
			*/

			gsUdpEngineGetPeerState(peer->ip, peer->port, &aPeerState);

			if(aPeerState == GS_UDP_PEER_CONNECTED)
			{
				GPIPeer * pcurr;
				GPIBool freePeerSig = GPITrue;

				// Get the profile object.
				//////////////////////////
				if(!gpiGetProfile(connection, peer->profile, &pProfile))
					Error(connection, GP_NETWORK_ERROR, "Error connecting to a peer.");

				// Send the auth.
				/////////////////
				gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\auth\\");
				gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\pid\\");
				gpiAppendIntToBuffer(connection, &peer->outputBuffer, iconnection->profileid);
				gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\nick\\");
				gpiAppendStringToBuffer(connection, &peer->outputBuffer, iconnection->nick);
				gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\sig\\");
				gpiAppendStringToBuffer(connection, &peer->outputBuffer, pProfile->peerSig);
				gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\final\\");

				// Are there any other peers still connecting?
				//////////////////////////////////////////////
				for(pcurr = iconnection->peerList ; pcurr != NULL ; pcurr = pcurr->pnext)
					if((pcurr->profile == peer->profile) && (pcurr != peer))
						if(pcurr->state <= GPI_PEER_CONNECTING)
							freePeerSig = GPIFalse;

				// freeclear it?
				///////////
				if(freePeerSig)
				{
					freeclear(pProfile->peerSig);
					if(gpiCanFreeProfile(pProfile))
						gpiRemoveProfile(connection, pProfile);
				}

				// Update the state.
				////////////////////
				peer->state = GPI_PEER_WAITING;
			}
			
			break;
		}
		case GPI_PEER_WAITING:
		{
			// Check for a response.
			////////////////////////
			//CHECK_RESULT(gpiRecvToBuffer(connection, peer->sock, &peer->inputBuffer, &len, &connClosed, "PR"));

			// Check for a final.
			/////////////////////
			if (peer->inputBuffer.buffer)
				str = strstr(peer->inputBuffer.buffer, "\\final\\");
			if(str != NULL)
			{
				str[0] = '\0';
				str += 7;

				// Was it rejected?
				///////////////////
				if(strncmp(peer->inputBuffer.buffer, "\\anack\\", 7) == 0)
				{
					// Rejected.
					////////////
					peer->nackCount++;

					// Is this more than once?
					//////////////////////////
					if(peer->nackCount > 1)
					{
						// we shouldn't reach this case unless there is a problem with 
						// the server when getting a buddy's signature

						// Give up already.
						///////////////////
						Error(connection, GP_NETWORK_ERROR, "Error getting buddy authorization.");
					}

					// Try getting the latest sig.
					//////////////////////////////
					CHECK_RESULT(gpiPeerGetSig(connection, peer));
				}
				else if(strncmp(peer->inputBuffer.buffer, "\\aack\\", 6) != 0)
				{
					// Unknown message.
					///////////////////
					Error(connection, GP_NETWORK_ERROR, "Error parsing buddy message.");
				}

				// The connection has been established.
				///////////////////////////////////////
				peer->state = GPI_PEER_CONNECTED;
				peer->inputBuffer.len = 0;
			}

			break;
		}
		// code should not reach here.
		default: break;
	}

	// Send stuff that needs to be sent.
	////////////////////////////////////
	if(peer->outputBuffer.len > 0)
	{
		//result = gpiSendFromBuffer(connection, peer->sock, &peer->outputBuffer, &connClosed, GPITrue, "PR");
		result = gpiSendBufferToPeer(connection, peer->ip, peer->port, &peer->outputBuffer, &connClosed, GPITrue);
		if(connClosed || (result != GP_NO_ERROR))
			peer->state = GPI_PEER_DISCONNECTED;
	}
	
	return GP_NO_ERROR;
}

static GPResult
gpiProcessPeerAcceptingConnection(
  GPConnection * connection,
  GPIPeer * peer
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GSUdpPeerState aPeerState;
	char * str;
	//int len;
	GPIBool connClosed;
	char intValue[16];
	int pid;
	char nick[GP_NICK_LEN];
	char sig[33];
	char sigCheck[33];
	char buffer[256];

	// Check the state.
	///////////////////
	GS_ASSERT(peer->state == GPI_PEER_WAITING);
	if (peer->state != GPI_PEER_WAITING)
		return GP_NETWORK_ERROR;

	// Read any pending info.
	/////////////////////////
	//CHECK_RESULT(gpiRecvToBuffer(connection, peer->sock, &peer->inputBuffer, &len, &connClosed, "PR"));
	gsUdpEngineGetPeerState(peer->ip, peer->port, &aPeerState);

	// Check for a closed connection.
	/////////////////////////////////
	if(aPeerState == GS_UDP_PEER_CLOSED)
	{
		peer->state = GPI_PEER_DISCONNECTED;
		return GP_NO_ERROR;
	}

	// Check for a final.
	/////////////////////
	str = strstr(peer->inputBuffer.buffer, "\\final\\");
	if(str != NULL)
	{
		str[0] = '\0';
		str += 7;

		// Is it an auth?
		/////////////////
		if(strncmp(peer->inputBuffer.buffer, "\\auth\\", 6) == 0)
		{
			// Get the pid.
			///////////////
			if(!gpiValueForKey(peer->inputBuffer.buffer, "\\pid\\", intValue, sizeof(intValue)))
			{
				peer->state = GPI_PEER_DISCONNECTED;
				return GP_NO_ERROR;
			}
			pid = atoi(intValue);

			// Get the nick.
			////////////////
			if(!gpiValueForKey(peer->inputBuffer.buffer, "\\nick\\", nick, sizeof(nick)))
			{
				peer->state = GPI_PEER_DISCONNECTED;
				return GP_NO_ERROR;
			}

			// Get the sig.
			///////////////
			if(!gpiValueForKey(peer->inputBuffer.buffer, "\\sig\\", sig, sizeof(sig)))
			{
				peer->state = GPI_PEER_DISCONNECTED;
				return GP_NO_ERROR;
			}

			// Compute what the sig should be.
			//////////////////////////////////
			sprintf(buffer, "%s%d%d",
				iconnection->password,
				iconnection->profileid,
				pid);
			MD5Digest((unsigned char *)buffer, strlen(buffer), sigCheck);

			// Check the sig.
			/////////////////
			if(strcmp(sig, sigCheck) != 0)
			{
				// Bad sig.
				///////////
				gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\anack\\");
				gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\final\\");

				gpiSendBufferToPeer(connection, peer->ip, peer->port, &peer->outputBuffer, &connClosed, GPITrue);
				peer->state = GPI_PEER_DISCONNECTED;
				return GP_NO_ERROR;
			}

			// Send an ack.
			///////////////
			gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\aack\\");
			gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\final\\");
			
			peer->state = GPI_PEER_CONNECTED;
			peer->profile = (GPProfile)pid;
		}
		else
		{
			// Unrecognized command.
			////////////////////////
			peer->state = GPI_PEER_DISCONNECTED;
			return GP_NO_ERROR;
		}
		
		// Update the buffer length.
		////////////////////////////
		peer->inputBuffer.len = 0;
	}

	return GP_NO_ERROR;
}

GPResult
gpiPeerSendMessages(
  GPConnection * connection,
  GPIPeer * peer
)
{
	GPIBool connClosed;
	GPIMessage * message;
	GPResult result;

	GS_ASSERT(peer);
	if (!peer)
		return GP_NETWORK_ERROR;
	// Only send messages if there's nothing waiting in the output buffer.
	//////////////////////////////////////////////////////////////////////
	if(peer->outputBuffer.len)
		return GP_NO_ERROR;

	// Send outgoing messages.
	//////////////////////////
	while(ArrayLength(peer->messages))
	{
		// Get the first message.
		/////////////////////////
		message = (GPIMessage *)ArrayNth(peer->messages, 0);

		// Send as much as possible.
		////////////////////////////
		//result = gpiSendFromBuffer(connection, peer->sock, &message->buffer, &connClosed, GPIFalse, "PR");
		result = gpiSendBufferToPeer(connection, peer->ip, peer->port, &message->buffer, &connClosed, GPIFalse);
		if(connClosed || (result != GP_NO_ERROR))
		{
			peer->state = GPI_PEER_DISCONNECTED;
			return GP_NO_ERROR;
		}

		// Did we not send it all?
		//////////////////////////
		if(message->buffer.pos != message->buffer.len)
			break;

		// Remove the message.
		//////////////////////
		ArrayDeleteAt(peer->messages, 0);
	}

	return GP_NO_ERROR;
}

static GPResult
gpiProcessPeerConnected(
  GPConnection * connection,
  GPIPeer * peer
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	//int len;
	GSUdpPeerState aPeerState;
	GPIBool connClosed;
	GPICallback callback;
	char * buffer;
	int type;
	int messageLen;
	GPResult result;

	GS_ASSERT(peer);
	if (!peer)
		return GP_NETWORK_ERROR;
	// Send stuff.
	//////////////
	if(peer->outputBuffer.len)
	{
		//result = gpiSendFromBuffer(connection, peer->sock, &peer->outputBuffer, &connClosed, GPITrue, "PR");
		result = gpiSendBufferToPeer(connection, peer->ip, peer->port, &peer->outputBuffer, &connClosed, GPITrue);
		if(connClosed || (result != GP_NO_ERROR))
		{
			peer->state = GPI_PEER_DISCONNECTED;
			return GP_NO_ERROR;
		}
	}

	// Send outgoing messages.
	//////////////////////////
	if(!peer->outputBuffer.len)
	{
		CHECK_RESULT(gpiPeerSendMessages(connection, peer));
		if(peer->state == GPI_PEER_DISCONNECTED)
			return GP_NO_ERROR;
	}

	// Read messages.
	/////////////////
	/*
	result = gpiRecvToBuffer(connection, peer->sock, &peer->inputBuffer, &len, &connClosed, "PR");
	if(result != GP_NO_ERROR)
	{
		peer->state = GPI_PEER_DISCONNECTED;
		return GP_NO_ERROR;
	}
	*/
	if(peer->inputBuffer.len > 0)
	{
		peer->timeout = (time(NULL) + GPI_PEER_TIMEOUT);
	}

	// Grab the message header.
	///////////////////////////
	do
	{
		// Read a message.
		//////////////////
		CHECK_RESULT(gpiReadMessageFromBuffer(connection, &peer->inputBuffer, &buffer, &type, &messageLen));
		if(buffer != NULL)
		{
			// Got a message!
			/////////////////
			switch(type)
			{
			case GPI_BM_MESSAGE:
				callback = iconnection->callbacks[GPI_RECV_BUDDY_MESSAGE];
				if(callback.callback != NULL)
				{
					GPRecvBuddyMessageArg * arg;

					arg = (GPRecvBuddyMessageArg *)gsimalloc(sizeof(GPRecvBuddyMessageArg));
					if(arg == NULL)
						Error(connection, GP_MEMORY_ERROR, "Out of memory.");
					arg->profile = peer->profile;
#ifndef GSI_UNICODE
					arg->message = goastrdup(buffer);
#else
					arg->message = UTF8ToUCS2StringAlloc(buffer);
#endif
					arg->date = (unsigned int)time(NULL);
					CHECK_RESULT(gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_MESSAGE));
				}
				break;
				
			case GPI_BM_UTM:
				callback = iconnection->callbacks[GPI_RECV_BUDDY_UTM];
				if (callback.callback != NULL)
				{
					GPRecvBuddyUTMArg * arg;

					arg = (GPRecvBuddyUTMArg *)gsimalloc(sizeof(GPRecvBuddyUTMArg));
					if(arg == NULL)
						Error(connection, GP_MEMORY_ERROR, "Out of memory.");
					arg->profile = peer->profile;
#ifndef GSI_UNICODE
					arg->message = goastrdup(buffer);
#else
					arg->message = UTF8ToUCS2StringAlloc(buffer);
#endif
					arg->date = (unsigned int)time(NULL);
					CHECK_RESULT(gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_MESSAGE));
				}
				break;

			case GPI_BM_PING:
				// Send back a pong.
				////////////////////
				gpiSendBuddyMessage(connection, peer->profile, GPI_BM_PONG, "1", 0, NULL);

				break;

#ifndef NOFILE
			case GPI_BM_PONG:
				// Lets the transfers handle this.
				//////////////////////////////////
				gpiTransfersHandlePong(connection, peer->profile, peer);
				break;
#endif
			case GPI_BM_KEYS_REQUEST:
				CHECK_RESULT(gpiBuddyHandleKeyRequest(connection, peer));
				break;
			case GPI_BM_KEYS_REPLY:
				CHECK_RESULT(gpiBuddyHandleKeyReply(connection, peer, buffer));
				// Let the keys request reply handler take care of this.
				////////////////////////////////////////////////////////
				break;
			case GPI_BM_FILE_SEND_REQUEST:
			case GPI_BM_FILE_SEND_REPLY:
			case GPI_BM_FILE_BEGIN:
			case GPI_BM_FILE_END:
			case GPI_BM_FILE_DATA:
			case GPI_BM_FILE_SKIP:
			case GPI_BM_FILE_TRANSFER_THROTTLE:
			case GPI_BM_FILE_TRANSFER_CANCEL:
			case GPI_BM_FILE_TRANSFER_KEEPALIVE:
				// Handle a transfer protocol message.
				//////////////////////////////////////
				gpiHandleTransferMessage(connection, peer, type, peer->inputBuffer.buffer, buffer, messageLen);


				break;

			default:
				break;
			}

			// Remove it from the buffer.
			/////////////////////////////
			gpiClipBufferToPosition(connection, &peer->inputBuffer);
		}
	}
	while(buffer);

	gsUdpEngineGetPeerState(peer->ip, peer->port, &aPeerState);
	//if(connClosed)
	if (aPeerState == GS_UDP_PEER_CLOSED)
		peer->state = GPI_PEER_DISCONNECTED;
	
	return GP_NO_ERROR;
}


// Used to check for any timed out peer operations 
// assumes peer is not NULL
// makes no assumption of the operation queue
void gpiCheckTimedOutPeerOperations(GPConnection * connection, GPIPeer *peer)
{
	GPIPeerOp *anIterator = peer->peerOpQueue.first;
	GS_ASSERT(peer);
	if (!peer)
		return;
	
	while (anIterator && anIterator != peer->peerOpQueue.last)
	{
		if (anIterator->state != GPI_PEER_OP_STATE_FINISHED &&  current_time() > anIterator->timeout && anIterator->callback)
		{
			// currently only one type of peer operation exists
			// when it's found, we need to provide the application with
			// a result of no data
			if (anIterator->type == GPI_BM_KEYS_REQUEST)
			{	
				GPICallback callback;
				GPGetBuddyStatusInfoKeysArg *arg = (GPGetBuddyStatusInfoKeysArg *)gsimalloc(sizeof(GPGetBuddyStatusInfoKeysArg));
				callback.callback = anIterator->callback;
				callback.param = anIterator->userData;
				arg->keys = NULL;
				arg->numKeys = 0;
				arg->values = NULL;
				arg->profile = peer->profile;
				gpiAddCallback(connection, callback, arg, NULL, 0);
				
			}
			// The peer operation is removed regardless of type
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Notice, "Peer operation timed out");
			gpiPeerRemoveOp(peer, anIterator);
		}
		anIterator = anIterator->next;
	}
}


static GPResult
gpiProcessPeer(
  GPConnection * connection,
  GPIPeer * peer
)
{
	GPResult result = GP_NO_ERROR;
	
	// This state should never get out of initialization.
	/////////////////////////////////////////////////////
	GS_ASSERT(peer->state != GPI_PEER_NOT_CONNECTED);
	if (peer->state == GPI_PEER_NOT_CONNECTED)
		return GP_NETWORK_ERROR;

	// If we're not connected yet.
	//////////////////////////////
	if(peer->state != GPI_PEER_CONNECTED)
	{
		if(peer->initiated)
			result = gpiProcessPeerInitiatingConnection(connection, peer);
		else
			result = gpiProcessPeerAcceptingConnection(connection, peer);
	}

	// If we're connected.
	//////////////////////
	if((result == GP_NO_ERROR) && (peer->state == GPI_PEER_CONNECTED))
	{
		result = gpiProcessPeerConnected(connection, peer);
		gpiCheckTimedOutPeerOperations(connection, peer);
	}

	return result;
}

void
gpiDestroyPeer(
  GPConnection * connection,
  GPIPeer * peer
)
{
#ifndef NOFILE
	// Cleanup any transfers that use this peer.
	////////////////////////////////////////////
	gpiTransferPeerDestroyed(connection, peer);
#endif

	//shutdown(peer->sock, 2);
	//closesocket(peer->sock);
	freeclear(peer->inputBuffer.buffer);
	freeclear(peer->outputBuffer.buffer);
	if(peer->messages)
	{
		ArrayFree(peer->messages);
		peer->messages = NULL;
	}
	freeclear(peer);
	
	GSI_UNUSED(connection);
}

void
gpiRemovePeer(
  GPConnection * connection,
  GPIPeer * peer
)
{
	GPIPeer * pprev;
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIMessage * message;

	GS_ASSERT(peer != NULL);
	if (peer == NULL)
		return;

	GS_ASSERT(iconnection->peerList);
	if (iconnection->peerList == NULL)
		return;
	// Check if this is the first peer.
	///////////////////////////////////
	if(iconnection->peerList == peer)
	{
		iconnection->peerList = peer->pnext;
	}
	else
	{
		// Find the previous peer.
		//////////////////////////
		for(pprev = iconnection->peerList ; pprev->pnext != peer ; pprev = pprev->pnext)
		{
			if(pprev->pnext == NULL)
			{
				// Can't find this peer in the list!
				////////////////////////////////////
				assert(0);
				gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
					"Tried to remove peer not in list.");
				return;
			}
		}
		pprev->pnext = peer->pnext;
	}

	// Check for pending messages.
	//////////////////////////////
	while(ArrayLength(peer->messages))
	{
		// Get the next message.
		////////////////////////
		message = (GPIMessage *)ArrayNth(peer->messages, 0);

		// Don't forward protocol messages.
		///////////////////////////////////
		if(message->type < 100)
			gpiSendServerBuddyMessage(connection, peer->profile, message->type, message->buffer.buffer + message->start);

		// Remove the message.
		//////////////////////
		ArrayDeleteAt(peer->messages, 0);
	}

	gpiDestroyPeer(connection, peer);
}

GPResult gpiProcessPeers(GPConnection *connection)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIPeer * nextPeer;
	GPIPeer * peer;
	//SOCKET incoming;
	GPResult result;

	/*
	// Check for incoming peer connections.
	///////////////////////////////////////
	if(iconnection->peerSocket != INVALID_SOCKET)
	{
		// Have to manually check if accept is possible since
		// PS2 Insock only supports blocking sockets.
		if (CanReceiveOnSocket(iconnection->peerSocket))
		{
			incoming = accept(iconnection->peerSocket, NULL, NULL);
			if(incoming != INVALID_SOCKET)
			{
				// This is a new peer.
				//////////////////////
				peer = gpiAddPeer(connection, -1, GPIFalse);
				if(peer)
				{
					peer->state = GPI_PEER_WAITING;
					peer->sock = incoming;
					SetSockBlocking(incoming, 0);
					gpiSetPeerSocketSizes(peer->sock);
				}
				else
				{
					closesocket(incoming);
				}
			}
		}
	}
	*/
	gsUdpEngineThink();
	// Got through the list of peers.
	/////////////////////////////////
	for(peer = iconnection->peerList ; peer != NULL ; peer = nextPeer)
	{
		// Store the next peer.
		///////////////////////
		nextPeer = peer->pnext;
		if(peer->state == GPI_PEER_DISCONNECTED)
		{
			// Remove it.
			/////////////
			//gsDebug
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Notice, "Peer disconnected, pid: %d", peer->profile);
			gpiRemovePeer(connection, peer);
		}
		else
		{
			// Process the peer.
			////////////////////
			result = gpiProcessPeer(connection, peer);

			// Check for a disconnection or a timeout.
			//////////////////////////////////////////
			if((peer->state == GPI_PEER_DISCONNECTED) || (result != GP_NO_ERROR) || (time(NULL) > peer->timeout))
			{
				// Remove it.
				/////////////
				gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Notice, "Peer disconnected, pid: %d", peer->profile);
				gpiRemovePeer(connection, peer);
			}
		}		
	}
	
	return GP_NO_ERROR;
}

// NOTE: use this function when in a gp function
GPIPeer * gpiGetPeerByProfile(const GPConnection * connection,
							  int profileid)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIPeer * pcurr;

	// Go through the list of peers.
	////////////////////////////////
	for(pcurr = iconnection->peerList ; pcurr != NULL ; pcurr  = pcurr->pnext)
	{
		// Check for a match.
		/////////////////////
		if(pcurr->profile == profileid)
		{
			// Got it.
			//////////
			return pcurr;
		}
	}

	return NULL;
}

// NOTE: use this function only when in a UDP layer callback
GPIPeer * gpiGetPeerByAddr(const GPConnection *connection,
                           unsigned int ip,
                           unsigned short port)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIPeer * pcurr;

	GS_ASSERT(ip);
	GS_ASSERT(port);
	if (!ip && !port)
		return NULL;
	// Go through the list of peers.
	////////////////////////////////
	for(pcurr = iconnection->peerList ; pcurr != NULL ; pcurr  = pcurr->pnext)
	{
		// Check for a match.
		/////////////////////
		if(pcurr->ip == ip && pcurr->port == port)
		{
			// Got it.
			//////////
			return pcurr;
		}
	}

	return NULL;
}

gsi_bool gpiIsPeerConnected(GPIPeer *peer)
{	
	GS_ASSERT(peer);
	if (!peer)
		return gsi_false;

	if (peer && peer->state != GPI_PEER_CONNECTED)
		return gsi_false;
	
	return gsi_true;
}

static void gpiFreeMessage(void * elem)
{
	GPIMessage * message = (GPIMessage *)elem;

	freeclear(message->buffer.buffer);
}

GPIPeer *
gpiAddPeer(
  GPConnection * connection,
  int profileid,
  GPIBool initiate
)
{
	GPIPeer * peer;
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Create a new peer.
	/////////////////////
	peer = (GPIPeer *)gsimalloc(sizeof(GPIPeer));
	if(peer == NULL)
		return NULL;
	memset(peer, 0, sizeof(GPIPeer));
	peer->state = GPI_PEER_NOT_CONNECTED;
	peer->initiated = initiate;
	//peer->sock = INVALID_SOCKET;
	peer->profile = profileid;
	peer->timeout = (time(NULL) + GPI_PEER_TIMEOUT);
	peer->pnext = iconnection->peerList;
	peer->messages = ArrayNew(sizeof(GPIMessage), 0, gpiFreeMessage);
	iconnection->peerList = peer;
	peer->peerOpQueue.first = NULL;
	peer->peerOpQueue.last = NULL;
	peer->peerOpQueue.opList = NULL;
	return peer;
}

GPResult
gpiPeerGetSig(
  GPConnection * connection,
  GPIPeer * peer
)
{
	GPIOperation * operation;

	// Start a get info operation to get the sig.
	/////////////////////////////////////////////
	CHECK_RESULT(gpiAddOperation(connection, GPI_GET_INFO, NULL, &operation, GP_NON_BLOCKING, NULL, NULL));

	// Send the get info.
	/////////////////////
	CHECK_RESULT(gpiSendGetInfo(connection, peer->profile, operation->id));

	// Set the state.
	/////////////////
	peer->state = GPI_PEER_GETTING_SIG;

	return GP_NO_ERROR;
}

GPResult
gpiPeerStartConnect(
  GPConnection * connection,
  GPIPeer * peer
)
{
	//int rcode;
	//struct sockaddr_in address;
	GPIProfile * profile;
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GSUdpErrorCode anError;

	// Get the profile object.
	//////////////////////////
	if(!gpiGetProfile(connection, peer->profile, &profile))
		Error(connection, GP_NETWORK_ERROR, "Error connecting to a peer.");

	/*
	// Create the socket.
	/////////////////////
	peer->sock = socket(AF_INET, SOCK_STREAM, 0);
	if(peer->sock == INVALID_SOCKET)
		CallbackError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error creating a socket.");

	// Make it non-blocking.
	////////////////////////
	rcode = SetSockBlocking(peer->sock, 0);
	if(rcode == 0)
		CallbackError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error making a socket non-blocking.");

	// Bind the socket.
	///////////////////

// BD: PS2 Insock has bug with binding to port 0
// No sockets after the first will be able to bind

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	rcode = bind(peer->sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
	if (gsiSocketIsError(rcode))
		CallbackError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error binding a socket.");

	// Set the socket sizes.
	////////////////////////
	gpiSetPeerSocketSizes(peer->sock);
	
	// Connect the socket.
	//////////////////////
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = profile->buddyStatus->ip;
	address.sin_port = (gsi_u16)profile->buddyStatus->port;
	rcode = connect(peer->sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
	if (gsiSocketIsError(rcode))
	{
		int error = GOAGetLastError(peer->sock);
		if((error != WSAEWOULDBLOCK) && (error != WSAEINPROGRESS) && (error != WSAETIMEDOUT) )
		{
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error connecting a socket.");
		}
	}
	*/

	if (profile->buddyStatusInfo)
	{
		GSUdpPeerState aPeerState;
		gsUdpEngineGetPeerState(profile->buddyStatusInfo->buddyIp , profile->buddyStatusInfo->buddyPort, &aPeerState);
		if (aPeerState != GS_UDP_PEER_CONNECTED || aPeerState != GS_UDP_PEER_CONNECTING)
		{
			anError = gsUdpEngineStartTalkingToPeer(profile->buddyStatusInfo->buddyIp , profile->buddyStatusInfo->buddyPort, 
			iconnection->mHeader, GPI_PEER_TIMEOUT);
			if (anError != GS_UDP_ADDRESS_ALREADY_IN_USE)
				CallbackError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error starting communication with a peer.");
		}
		peer->ip = profile->buddyStatusInfo->buddyIp;
		peer->port = profile->buddyStatusInfo->buddyPort;
	}
	// We're waiting for the connect to complete.
	/////////////////////////////////////////////
	peer->state = GPI_PEER_CONNECTING;

	return GP_NO_ERROR;
}

GPResult
gpiPeerAddMessage(
  GPConnection * connection,
  GPIPeer * peer,
  int type,
  const char * message
)
{
	GPIMessage gpiMessage;
	int len;

	GS_ASSERT(peer != NULL);
	GS_ASSERT(message != NULL);
	
	if (peer == NULL)
		return GP_NETWORK_ERROR;
	if (message == NULL)
		return GP_NETWORK_ERROR;

	// Get the length.
	//////////////////
	len = (int)strlen(message);

	// Clear the message.
	/////////////////////
	memset(&gpiMessage, 0, sizeof(GPIMessage));

	// Copy the type.
	/////////////////
	gpiMessage.type = type;

	// Copy the header to the buffer.
	/////////////////////////////////
	CHECK_RESULT(gpiAppendStringToBuffer(connection, &gpiMessage.buffer, "\\m\\"));
	CHECK_RESULT(gpiAppendIntToBuffer(connection, &gpiMessage.buffer, type));
	CHECK_RESULT(gpiAppendStringToBuffer(connection, &gpiMessage.buffer, "\\len\\"));
	CHECK_RESULT(gpiAppendIntToBuffer(connection, &gpiMessage.buffer, len));
	CHECK_RESULT(gpiAppendStringToBuffer(connection, &gpiMessage.buffer, "\\msg\\\n"));

	// Copy the message to the buffer.
	//////////////////////////////////
	gpiMessage.start = gpiMessage.buffer.len;
	CHECK_RESULT(gpiAppendStringToBufferLen(connection, &gpiMessage.buffer, message, len));
	CHECK_RESULT(gpiAppendCharToBuffer(connection, &gpiMessage.buffer, '\0'));

	// Add it to the list.
	//////////////////////
	ArrayAppend(peer->messages, &gpiMessage);

	// Reset the timeout.
	/////////////////////
	peer->timeout = (time(NULL) + GPI_PEER_TIMEOUT);

	return GP_NO_ERROR;
}

GPResult
gpiPeerStartTransferMessage(
  GPConnection * connection,
  GPIPeer * peer,
  int type,
  const struct GPITransferID_s * transferID
)
{
	char buffer[64];
	GPITransferID tid;
	tid.count = transferID->count;
	tid.profileid = transferID->profileid;
	tid.time = transferID->time;

	GS_ASSERT(transferID);
	if (!transferID)
		return GP_NETWORK_ERROR;
	// Start the message.
	/////////////////////
	sprintf(buffer, "\\m\\%d\\xfer\\%d %u %u", type, tid.profileid, tid.count, tid.time);

	return gpiSendOrBufferString(connection, peer, buffer);
}

GPResult
gpiPeerFinishTransferMessage(
  GPConnection * connection,
  GPIPeer * peer,
  const char * message,
  int len
)
{
	char buffer[32];
	GS_ASSERT(peer != NULL);
	if (!peer)
		return GP_NETWORK_ERROR;

	// Check the message.
	/////////////////////
	if(!message)
		message = "";
	
	if(len == -1)
		len = (int)strlen(message);

	// Set the len and the message.
	///////////////////////////////
	sprintf(buffer, "\\len\\%d\\msg\\\n", len);
	CHECK_RESULT(gpiSendOrBufferString(connection, peer, buffer));

	// Copy the message to the buffer.
	//////////////////////////////////
	CHECK_RESULT(gpiSendOrBufferStringLenToPeer(connection, peer, message, len));
	CHECK_RESULT(gpiSendOrBufferChar(connection, peer, '\0'));

	// Reset the timeout.
	/////////////////////
	peer->timeout = (time(NULL) + GPI_PEER_TIMEOUT);
		
	return GP_NO_ERROR;
}

void gpiPeerLeftCallback(unsigned int ip, unsigned short port, GSUdpCloseReason reason, void *userData)
{

	GPConnection *connection = (GPConnection *)userData;
	GPIPeer *aPeer;
	IN_ADDR anAddr;
	anAddr.s_addr = ip;
	aPeer = gpiGetPeerByAddr(connection, ip, port);
	//gpiRemovePeer(connection, aPeer);
	if (aPeer)
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Notice, 
			"Peer left: addr: %s:%d, profile: %d\n", inet_ntoa(anAddr), port, aPeer->profile);
		aPeer->state = GPI_PEER_DISCONNECTED;
	}
	
	GSI_UNUSED(anAddr);
	GSI_UNUSED(reason);
}

void gpiPeerMessageCallback(unsigned int ip, unsigned short port, unsigned char *message, 
							unsigned int messageLength, gsi_bool reliable, void *userData)
{
	GPConnection *connection = (GPConnection *)userData;
	GPIPeer *aPeer;
	unsigned char * buff;
	int writePos;
	int size;
	IN_ADDR anAddr;
	anAddr.s_addr = ip;
	aPeer = gpiGetPeerByAddr(connection, ip, port);
	if (!aPeer)
	{
		aPeer = gpiAddPeer(connection, -1, GPIFalse);
		if (aPeer)
		{
			aPeer->state = GPI_PEER_WAITING;
			aPeer->ip = ip;
			aPeer->port = port;
		}
		else 
		{
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Memory, GSIDebugLevel_HotError, 
				"gpiPeerMessageCallback: out of memory when allocating peer, addr: %s:%d", inet_ntoa(anAddr), port);
			return;
		}
	}

	buff = (unsigned char *)aPeer->inputBuffer.buffer;
	writePos = aPeer->inputBuffer.len;
	size = aPeer->inputBuffer.size;

	// Check if the buffer needs to be resized.
	///////////////////////////////////////////
	if((int)messageLength > (size - writePos))
	{
		unsigned char *reallocedBuff;
		size = (writePos + max(GPI_READ_SIZE,(int)messageLength));
		reallocedBuff = (unsigned char *)gsirealloc(buff, (unsigned int)size + 1);
		if(reallocedBuff == NULL)
		{
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Memory, GSIDebugLevel_HotError, 
				"gpiPeerMessageCallback: out of memory when reallocating buffer, addr: %s:%d", inet_ntoa(anAddr), port);
			gsifree(buff);
			gpiSetErrorString(connection, "Out of memory.");
			gpiCallErrorCallback(connection, GP_MEMORY_ERROR, GP_NON_FATAL);
			return;
		}
		else
			buff = reallocedBuff;
	}

	memcpy(&buff[writePos], message, messageLength);

	aPeer->inputBuffer.buffer = (char *)buff;
	aPeer->inputBuffer.len += messageLength;
	aPeer->inputBuffer.size = size;
	buff[aPeer->inputBuffer.len] = '\0';
	GSI_UNUSED(reliable);
	GSI_UNUSED(anAddr);
}

void gpiPeerAcceptedCallback(unsigned int ip, unsigned short port, 
							 GSUdpErrorCode error, gsi_bool rejected, void *userData)
{
	GPConnection *connection = (GPConnection *)userData;
	GPIPeer *aPeer;
	IN_ADDR anAddr;
	anAddr.s_addr = ip;
	
	aPeer = gpiGetPeerByAddr(connection, ip, port);
	if (!aPeer)
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_HotError,
			"Peer does not exist: ip-port: %s:%d\n", inet_ntoa(anAddr), port);
	}
	else
	{
		if (rejected)
		{
			aPeer->state = GPI_PEER_DISCONNECTED;
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Notice,
				"Peer Connection rejected: ip-port: %s:%d\n", inet_ntoa(anAddr), port);
			return;
		}
	}
	
	gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Notice,
		"Peer Connection accepted: ip-port: %s:%d\n", inet_ntoa(anAddr), port);
	
	GSI_UNUSED(userData);
	GSI_UNUSED(rejected);
	GSI_UNUSED(error);
	GSI_UNUSED(anAddr);	
}
void gpiPeerPingReplyCallback(unsigned int ip, unsigned short port, unsigned int latency, void *userData)
{
	GSI_UNUSED(userData);
	GSI_UNUSED(latency);
	GSI_UNUSED(port);
	GSI_UNUSED(ip);
}

// gpiPeerAddOp notes:
// Assumes non-null inputs!
// The queue should be empty when the first element is added.
// Any new element added will be added to the end of the queue.
void gpiPeerAddOp(GPIPeer *peer, GPIPeerOp *operation)
{
	GS_ASSERT(peer);
	GS_ASSERT(operation);

	if (!peer || !operation)
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_WarmError, "Peer operation not added");
		return;
	}
	// Three cases can occur:
	// The list is empty - set all pointers to the new node
	// The list has only one element - set the first element's next to the new 
	//     and set the last element to the new
	// The list has more than one element - add the new element to the end of 
	//     the queue
	if (peer->peerOpQueue.opList == NULL)
	{
		peer->peerOpQueue.first = operation;
		peer->peerOpQueue.last = operation;
		peer->peerOpQueue.opList = operation;		
	}
	else if (peer->peerOpQueue.first == peer->peerOpQueue.last)
	{
		peer->peerOpQueue.first->next = operation;
		peer->peerOpQueue.last = operation;
	}
	else
	{
		peer->peerOpQueue.last->next = operation;
		peer->peerOpQueue.last = operation;		
	}

	gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Notice, "Peer Operation Added");
}

// gpiPeerRemoveOp:
// Assumes the list is NOT NULL otherwise it returns.
// Assumes the operation being passed in is on the queue.
// Assumes non-null inputs!
// Completed or Timed out Operations are deleted from queue by finding 
// the operation passed in.  Removal of operations don't necessarily 
// happen in order.
void gpiPeerRemoveOp(GPIPeer *peer, GPIPeerOp *operation)
{
	GS_ASSERT(peer);
	GS_ASSERT(operation);
	if (!peer || !operation)
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_WarmError, "Peer operation not removed");
		return;
	}

	GS_ASSERT(peer->peerOpQueue.opList != NULL);
	if (peer->peerOpQueue.opList == NULL)
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_WarmError, "Peer operation not removed");
		return;
	}

	if (peer->peerOpQueue.first == peer->peerOpQueue.last && peer->peerOpQueue.first == operation)
	{
		peer->peerOpQueue.opList = peer->peerOpQueue.first = peer->peerOpQueue.last = operation->next;
	}
	else if (peer->peerOpQueue.first == operation)
	{
		peer->peerOpQueue.first = peer->peerOpQueue.first->next;
		peer->peerOpQueue.opList = peer->peerOpQueue.first;
	}
	else
	{
		GPIPeerOp *aPrevOp = NULL;
		for(aPrevOp = peer->peerOpQueue.first ; aPrevOp->next != operation ; aPrevOp = aPrevOp->next)
		{
			if(aPrevOp->next == NULL)
			{
				// Can't find this peer in the list!
				////////////////////////////////////
				gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
					"Tried to remove peer operation not in list.");
				return;
			}
		}
		aPrevOp->next = operation->next;
	}

	gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Notice, "Peer operation removed");
	freeclear(operation);
}
