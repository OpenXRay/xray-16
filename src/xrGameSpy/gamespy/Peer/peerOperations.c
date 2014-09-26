/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

/*************
** INCLUDES **
*************/
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include "peerOperations.h"
#include "peerCallbacks.h"
#include "peerGlobalCallbacks.h"
#include "peerMangle.h"
#include "peerRooms.h"
#include "peerPlayers.h"
#include "peerKeys.h"
#include "peerSB.h"
#include "peerHost.h"
#include "peerAutoMatch.h"
#include "peerQR.h"
#include "../md5.h"

/************
** DEFINES **
************/
#define PI_CHAT_SERVER_ADDRESS    "peerchat." GSI_DOMAIN_NAME
#define PI_CHAT_SERVER_PORT       6667

#define PEER_CONNECTION_OP        piOperation * operation = (piOperation *)param;\
                                  piConnection * connection = NULL;\
                                  PEER peer = NULL;\
                                  assert(operation && operation->peer);\
                                  if(operation && operation->peer)\
                                      peer = operation->peer;\
                                  connection = (piConnection *)peer;\
								  GSI_UNUSED(connection);
#if 0
// for Visual Assist
PEER peer;
piOperation * operation;
#endif

/**********
** TYPES **
**********/
typedef struct piOperationContainer
{
	piOperation * operation;
} piOperationContainer;

/**************
** FUNCTIONS **
**************/
#ifndef GSI_UNICODE
#define piGetGlobalKeysCallback		piGetGlobalKeysCallbackA
#define piGetChannelKeysCallback	piGetChannelKeysCallbackA
#define piGetPlayerInfoCallback		piGetPlayerInfoCallbackA
#define piConnectNickErrorCallback	piConnectNickErrorCallbackA
#define piConnectFillInUserCallback	piConnectFillInUserCallbackA
#define piCreateStagingRoomEnumUsersCallback piCreateStagingRoomEnumUsersCallbackA
#define piCreateStagingRoomEnterChannelCallback piCreateStagingRoomEnterChannelCallbackA
#define piJoinRoomEnumUsersCallback piJoinRoomEnumUsersCallbackA
#define piJoinRoomEnterChannelCallback piJoinRoomEnterChannelCallbackA
#define piChangeNickCallback		piChangeNickCallbackA
#define piAuthenticateCDKeyCallback piAuthenticateCDKeyCallbackA
#else
#define piGetGlobalKeysCallback		piGetGlobalKeysCallbackW
#define piGetChannelKeysCallback	piGetChannelKeysCallbackW
#define piGetPlayerInfoCallback		piGetPlayerInfoCallbackW
#define piConnectNickErrorCallback	piConnectNickErrorCallbackW
#define piConnectFillInUserCallback	piConnectFillInUserCallbackW
#define piCreateStagingRoomEnumUsersCallback piCreateStagingRoomEnumUsersCallbackW
#define piCreateStagingRoomEnterChannelCallback piCreateStagingRoomEnterChannelCallbackW
#define piJoinRoomEnumUsersCallback piJoinRoomEnumUsersCallbackW
#define piJoinRoomEnterChannelCallback piJoinRoomEnterChannelCallbackW
#define piChangeNickCallback		piChangeNickCallbackW
#define piAuthenticateCDKeyCallback piAuthenticateCDKeyCallbackW
#endif

static void piOperationsListFree(void *elem1)
{
	piOperationContainer * container = (piOperationContainer *)elem1;
	piOperation * operation;

	assert(container);
	assert(container->operation);

	operation = container->operation;

	// Free the name.
	/////////////////
	gsifree(operation->name);

	// Free the password.
	/////////////////////
	gsifree(operation->password);

	// Free the user data.
	//////////////////////
	gsifree(operation->data);

	// Close the socket if needed.
	//////////////////////////////
	if(operation->socketClose)
	{
		closesocket(operation->socket);
		SocketShutDown();
	}

	// Free the operation.
	//////////////////////
	gsifree(operation);
}

PEERBool piOperationsInit(PEER peer)
{
	PEER_CONNECTION;

	assert(!connection->operationList);

	// Init data.
	/////////////
	connection->operationsStarted = 0;
	connection->operationsFinished = 0;

	// Init the list.
	/////////////////
	connection->operationList = ArrayNew(sizeof(piOperationContainer), 0, piOperationsListFree);
	if(!connection->operationList)
		return PEERFalse;

	return PEERTrue;
}

void piOperationsReset(PEER peer)
{
	PEER_CONNECTION;

	// Clear the list.
	//////////////////
	if(connection->operationList)
		ArrayClear(connection->operationList);
	
	// Clear the listingGroupsOperation 
	if (connection->listingGroupsOperation)
		connection->listingGroupsOperation = NULL;
}

void piOperationsCleanup(PEER peer)
{
	PEER_CONNECTION;

	// Cleanup the list.
	////////////////////
	if(connection->operationList)
		ArrayFree(connection->operationList);
	connection->operationList = NULL;
}

void piRemoveOperation(PEER peer, piOperation * operation)
{
	piOperationContainer * container;
	piOperation * op;
	int i;
	int count;

	PEER_CONNECTION;

	// Make sure there is a list.
	/////////////////////////////
	if(!connection->operationList)
		return;

	// Loop through the operations.
	///////////////////////////////
	count = ArrayLength(connection->operationList);
	for(i = 0 ; i < count ; i++)
	{
		// Get the operation.
		/////////////////////
		container = (piOperationContainer *)ArrayNth(connection->operationList, i);
		op = container->operation;

		// Check the op.
		////////////////
		if(op == operation)
		{
			// Remove it.
			/////////////
			ArrayDeleteAt(connection->operationList, i);

			// One more finished.
			/////////////////////
			connection->operationsFinished++;

			return;
		}
	}
}

PEERBool piIsOperationFinished(PEER peer, int opID)
{
	piOperationContainer * container;
	piOperation * op;
	int i;
	int count;

	PEER_CONNECTION;

	// Make sure there is a list.
	/////////////////////////////
	if(!connection->operationList)
		return PEERTrue;

	// Loop through the operations.
	///////////////////////////////
	count = ArrayLength(connection->operationList);
	for(i = 0 ; i < count ; i++)
	{
		// Get the operation.
		/////////////////////
		container = (piOperationContainer *)ArrayNth(connection->operationList, i);
		op = container->operation;

		// Check the ID.
		////////////////
		if(op->ID == opID)
			return PEERFalse;
	}

	return PEERTrue;
}

void piCancelJoinOperation(PEER peer, RoomType roomType)
{
	piOperationContainer * container;
	piOperation * operation;
	int i;
	int count;

	PEER_CONNECTION;

	// Make sure there is a list.
	/////////////////////////////
	if(!connection->operationList)
		return;

	// Loop through the operations.
	///////////////////////////////
	count = ArrayLength(connection->operationList);
	for(i = 0 ; i < count ; i++)
	{
		// Get the operation.
		/////////////////////
		container = (piOperationContainer *)ArrayNth(connection->operationList, i);
		operation = container->operation;

		// Is it a join or create?
		//////////////////////////
		if((operation->type == PI_JOIN_ROOM_OPERATION) || (operation->type == PI_CREATE_ROOM_OPERATION))
		{
			// Is it the same room?
			///////////////////////
			if(operation->roomType == roomType)
			{
				// Cancel the operation.
				////////////////////////
				operation->cancel = PEERTrue;

				return;
			}
		}
	}
}

int piGetNextID(PEER peer)
{
	int ID;

	PEER_CONNECTION;

	// Get the ID.
	//////////////
	ID = connection->nextID;

	// Increment.
	/////////////
	connection->nextID++;
	if(connection->nextID < 0)
		connection->nextID = 0;

	return ID;
}

static piOperation * piAddOperation
(
	PEER peer,
	piOperationType type,
	void * data,
	PEERCBType callback,
	void * callbackParam,
	int opID
)
{
	piOperation * operation;
	piOperationContainer container;

	PEER_CONNECTION;

	//assert(type >= 0);
	assert(type < PI_NUM_OPERATION_TYPES);

	// Make sure there is a list.
	/////////////////////////////
	if(!connection->operationList)
		return NULL;

	// Alloc the operaiton.
	///////////////////////
	operation = (piOperation *)gsimalloc(sizeof(piOperation));
	if(!operation)
		return NULL;

	// Fill in the operation.
	/////////////////////////
	memset(operation, 0, sizeof(piOperation));
	operation->peer = peer;
	operation->type = type;
	operation->data = data;
	operation->ID = opID;
	operation->callback = callback;
	operation->callbackParam = callbackParam;
	operation->name = NULL;
	operation->cancel = PEERFalse;

	// Add the operation to the list.
	/////////////////////////////////
	container.operation = operation;
	ArrayAppend(connection->operationList, &container);

	// One more op.
	///////////////
	connection->operationsStarted++;

	return operation;
}

/***************
** OPERATIONS **
***************/

/* Connect.
**********/
static void piConnectConnectCallback
(
	CHAT chat,
	CHATBool success,
	int failureReason,
	void *param
)
{
	PEER_CONNECTION_OP;
	
	// If successful, try and do the connect title stuff.
	/////////////////////////////////////////////////////
	if(success)
	{
		if(!piConnectTitle(peer))
		{
			piDisconnectTitle(peer);
			success = CHATFalse;
		}
	}

	// Connection attempt finished.
	///////////////////////////////
	connection->connecting = PEERFalse;
	connection->connected = (PEERBool)success;

	if(success)
	{
		const char * nick;

		// Setup server pinging.
		////////////////////////
		connection->lastChatPing = current_time();
		
		// Check the nick.
		//////////////////
		nick = chatGetNickA(chat);
		if(strcasecmp(connection->nick, nick) != 0)
		{
			strcpy(connection->nick, nick);

#ifdef GSI_UNICODE
			UTF8ToUCS2String(connection->nick, connection->nick_W);
#endif
		}
	}
	else
	{
		// Set the disconnect flag.
		///////////////////////////
		connection->disconnect = PEERTrue;
	}

	// Add the callback.
	////////////////////
	piAddConnectCallback(peer, (PEERBool)success, failureReason, (peerConnectCallback)operation->callback, operation->callbackParam, operation->ID);

	// Remove the operation.
	////////////////////////
	piRemoveOperation(peer, operation);
}

static void piConnectNickErrorCallbackA
(
	CHAT chat,
	int type,
	const char * nick,
	int numSuggestedNicks,
	const char ** suggestedNicks,
	void *param
)
{
	PEER_CONNECTION_OP;

	// Add the peer callback.
	/////////////////////////
	piAddNickErrorCallback(peer, type, nick, numSuggestedNicks, suggestedNicks, operation->callbackParam, operation->ID);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piConnectNickErrorCallbackW
(
	CHAT chat,
	int type,
	const unsigned short * nick,
	int numSuggestedNicks,
	const unsigned short ** suggestedNicks,
	void *param
)
{
	char* nick_A = UCS2ToUTF8StringAlloc(nick);
	char** suggestedNicks_A = UCS2ToUTF8StringArrayAlloc(suggestedNicks, numSuggestedNicks);
	int i;
	piConnectNickErrorCallbackA(chat, type, nick_A, numSuggestedNicks, (const char**)suggestedNicks_A, param);
	gsifree(nick_A);
	if(suggestedNicks_A)
	{
		for (i=0; i<numSuggestedNicks; i++)
			gsifree(suggestedNicks_A[i]);
	}
}
#endif

static void piConnectFillInUserCallbackA
(
	CHAT chat,
	unsigned int IP,
	char user[128],
	void * param
)
{
	int pid;

	PEER_CONNECTION_OP;

	// 2004.Sep.17.JED - For reasons unknown, in the span of one month Arcade's automated
	//   crash handling system reported +15 null pointer crashes when publicIP was stored.
	//////////////////////////////////////////////////////////////////////////////////////
	if( !connection )
		return;

	// Store the public IP.
	///////////////////////
	connection->publicIP = IP;

	// If chat has a pid, override ours.
	////////////////////////////////////
	pid = chatGetProfileID(connection->chat);
	if(pid)
		connection->profileID = pid;

	// Mangle the IP and pid into the user.
	///////////////////////////////////////
	piMangleUser(user, connection->publicIP, connection->profileID);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piConnectFillInUserCallbackW
(
	CHAT chat,
	unsigned int IP,
	unsigned short user[128], // OUT parameter!!
	void * param
)
{
	// This one is reversed! Call the callback, THEN convert
	char user_A[256];
	piConnectFillInUserCallbackA(chat, IP, user_A, param);
	UTF8ToUCS2String(user_A, user);
}
#endif

PEERBool piNewConnectOperation
(
	PEER peer,
	piConnectType connectType,
	const char * nick,
	int namespaceID,
	const char * email,
	const char * profilenick,
	const char * uniquenick,
	const char * password,
	const char * authtoken,
	const char * partnerchallenge,
	peerConnectCallback callback,
	void * callbackParam,
	int opID
)
{
	piOperation * operation;
	chatGlobalCallbacks globalCallbacks;
	const char * uniqueID;
	char encodedUniqueID[33];
	peerNickErrorCallback nickErrorCallback;

	PEER_CONNECTION;

	assert(callback);

#ifdef _DEBUG
	if(connectType == PI_CONNECT)
	{
		assert(nick && nick[0]);
	}
	else if(connectType == PI_CONNECT_UNIQUENICK_LOGIN)
	{
		assert(namespaceID > 0);
		assert(uniquenick && uniquenick[0]);
		assert(password && password[0]);
	}
	else if(connectType == PI_CONNECT_PROFILENICK_LOGIN)
	{
		assert(namespaceID >= 0);
		assert(email && email[0]);
		assert(profilenick && profilenick[0]);
		assert(password && password[0]);
	}
	else if(connectType == PI_CONNECT_PREAUTH)
	{
		assert(authtoken && authtoken[0]);
		assert(partnerchallenge && partnerchallenge[0]);
	}
#endif

	// Add an operation.
	////////////////////
	operation = piAddOperation(peer, PI_CONNECT_OPERATION, NULL, (PEERCBType)callback, callbackParam, opID);
	if(!operation)
		return PEERFalse;

	// Setup the global callbacks.
	//////////////////////////////
	memset(&globalCallbacks, 0, sizeof(chatGlobalCallbacks));
	globalCallbacks.disconnected = piChatDisconnected;
	globalCallbacks.privateMessage = piChatPrivateMessage;
	globalCallbacks.param = peer;

	// Encode the unique ID.
	////////////////////////
	uniqueID = GOAGetUniqueID();
	MD5Digest((unsigned char *)uniqueID, strlen(uniqueID), encodedUniqueID);

	// Connect to chat.
	///////////////////
	nickErrorCallback = (connection->nickErrorCallback ? piConnectNickErrorCallback : NULL);
	if(connectType == PI_CONNECT)
	{
		connection->chat = chatConnectSecureA(
			PI_CHAT_SERVER_ADDRESS,
			PI_CHAT_SERVER_PORT,
			nick,
			encodedUniqueID,
			connection->sbName,
			connection->sbSecretKey,
			&globalCallbacks,
			nickErrorCallback,
			piConnectFillInUserCallback,
			piConnectConnectCallback,
			operation,
			CHATFalse);
	}
	else if((connectType == PI_CONNECT_UNIQUENICK_LOGIN) || (connectType == PI_CONNECT_PROFILENICK_LOGIN))
	{
		connection->chat = chatConnectLoginA(
			PI_CHAT_SERVER_ADDRESS,
			PI_CHAT_SERVER_PORT,
			namespaceID,
			email,
			profilenick,
			uniquenick,
			password,
			encodedUniqueID,
			connection->sbName,
			connection->sbSecretKey,
			&globalCallbacks,
			nickErrorCallback,
			piConnectFillInUserCallback,
			piConnectConnectCallback,
			operation,
			CHATFalse);
	}
	else if(connectType == PI_CONNECT_PREAUTH)
	{
		connection->chat = chatConnectPreAuthA(
			PI_CHAT_SERVER_ADDRESS,
			PI_CHAT_SERVER_PORT,
			authtoken,
			partnerchallenge,
			encodedUniqueID,
			connection->sbName,
			connection->sbSecretKey,
			&globalCallbacks,
			nickErrorCallback,
			piConnectFillInUserCallback,
			piConnectConnectCallback,
			operation,
			CHATFalse);
	}
	if(!connection->chat)
	{
		piRemoveOperation(peer, operation);
		return PEERFalse;
	}

	return PEERTrue;
}

/* Create Staging Room.
**********************/
static PEERJoinResult piEnterResultToJoinResult
(
	CHATEnterResult result
)
{
	switch(result)
	{
	case CHATEnterSuccess:
		return PEERJoinSuccess;
	case CHATChannelIsFull:
		return PEERFullRoom;
	case CHATInviteOnlyChannel:
		return PEERInviteOnlyRoom;
	case CHATBannedFromChannel:
		return PEERBannedFromRoom;
	case CHATBadChannelPassword:
		return PEERBadPassword;
	default:
		break;
	}

	return PEERJoinFailed;
}

static void piCreateStagingRoomEnumUsersCallbackA
(
	CHAT chat,
	CHATBool success,
	const char * channel,
	int numUsers,
	const char ** users,
	int * modes,
	void *param
)
{
	PEER_CONNECTION_OP;

	// Check if this was cancelled.
	///////////////////////////////
	if(operation->cancel)
	{
		piRemoveOperation(peer, operation);
		return;
	}

	// Start hosting/reporting.
	///////////////////////////
	if(success)
	{
		if(!peerIsAutoMatching(peer))
		{
			if(!piStartHosting(peer, operation->socket, operation->port))
				success = CHATFalse;
			else
			{
				// If we created the socket, hand over responsibility for it to qr2.
				////////////////////////////////////////////////////////////////////
				if(operation->socketClose)
				{
					operation->socketClose = PEERFalse;
					connection->queryReporting->read_socket = 1;
				}
			}
		}
		else
		{
			connection->hosting = PEERTrue;
		}
	}

	// Do stuff based on success.
	/////////////////////////////
	if(success)
	{
		int i;

		// Done entering.
		/////////////////
		piFinishedEnteringRoom(peer, StagingRoom, operation->name);

		// Add everyone to the room.
		////////////////////////////
		for(i = 0 ; i < numUsers ; i++)
			piPlayerJoinedRoom(peer, users[i], StagingRoom, modes[i]);

		// Set the name.
		////////////////
		chatSetChannelTopicA(connection->chat, channel, operation->name);

		// Set a limit on the room.
		///////////////////////////
#ifndef PI_NO_STAGING_ROOM_LIMIT
		if(connection->maxPlayers)
			chatSetChannelLimitA(connection->chat, channel, connection->maxPlayers);
#endif

		// If this is AutoMatch, and we created a socket, hand it over.
		///////////////////////////////////////////////////////////////
		if(operation->socketClose && peerIsAutoMatching(peer))
		{
			connection->autoMatchOperation->socket = operation->socket;
			connection->autoMatchOperation->port = operation->port;
			connection->autoMatchOperation->socketClose = PEERTrue;
			operation->socketClose = PEERFalse;
		}
	}
	else
	{
		// Leave the room.
		//////////////////
		piLeaveRoom(peer, StagingRoom, NULL);
	}

	// Add the callback.
	////////////////////
	piAddJoinRoomCallback(peer, (PEERBool)success, success?PEERJoinSuccess:PEERJoinFailed, StagingRoom, (peerJoinRoomCallback)operation->callback, operation->callbackParam, operation->ID);

	// Remove the operation.
	////////////////////////
	piRemoveOperation(peer, operation);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piCreateStagingRoomEnumUsersCallbackW
(
	CHAT chat,
	CHATBool success,
	const unsigned short * channel,
	int numUsers,
	const unsigned short ** users,
	int * modes,
	void *param
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char** users_A = UCS2ToUTF8StringArrayAlloc(users, numUsers);
	int i;
	piCreateStagingRoomEnumUsersCallbackA(chat, success, channel_A, numUsers, (const char**)users_A, modes, param);
	gsifree(channel_A);
	for (i=0; i<numUsers; i++)
		gsifree(users_A[i]);
	gsifree(users_A);
}
#endif


static void piCreateStagingRoomGetChannelModeCallback(CHAT chat, CHATBool success, const gsi_char * channel, 
	CHATChannelMode * mode, void * param)
{
	piConnection *connection = (piConnection *)param;
	if (success)
	{

		mode->Limit = connection->maxPlayers;
		mode->OpsObeyChannelLimit = CHATTrue;
		
		// Don't let ops bypass channel limit on the number of players in room
		chatSetChannelMode(chat, channel, mode);
	}
	
}


static void piCreateStagingRoomEnterChannelCallbackA
(
	CHAT chat,
	CHATBool success,
	CHATEnterResult result,
	const char * channel,
	void *param
)
{	
	PEER_CONNECTION_OP;

	assert(channel);
	assert(channel[0]);

	// Check if this was cancelled.
	///////////////////////////////
	if(operation->cancel)
	{
		piRemoveOperation(peer, operation);
		return;
	}

	if(success)
	{
		// If passworded, set it.
		/////////////////////////
		if(operation->password)
			chatSetChannelPasswordA(connection->chat, channel, CHATTrue, operation->password);

		
		// get the channel modes so we can set the limits on our staging channel
		chatGetChannelMode(connection->chat, peerGetRoomChannel(peer, StagingRoom), piCreateStagingRoomGetChannelModeCallback, 
				(void *)connection, CHATFalse);

		
		// How many users?
		//////////////////
		chatEnumUsersA(chat, channel, piCreateStagingRoomEnumUsersCallback, operation, CHATFalse);
	}
	else
	{
		PEERJoinResult joinResult;

		// Not entering.
		////////////////
		piLeaveRoom(peer, StagingRoom, NULL);

		// Get the peer result.
		///////////////////////
		if(result == CHATEnterSuccess)
			joinResult = PEERJoinFailed;
		else
			joinResult = piEnterResultToJoinResult(result);

		// Add the callback.
		////////////////////
		piAddJoinRoomCallback(peer, PEERFalse, joinResult, StagingRoom, (peerJoinRoomCallback)operation->callback, operation->callbackParam, operation->ID);

		// Remove the operation.
		////////////////////////
		piRemoveOperation(peer, operation);
	}
}
#ifdef GSI_UNICODE
static void piCreateStagingRoomEnterChannelCallbackW
(
	CHAT chat,
	CHATBool success,
	CHATEnterResult result,
	const unsigned short * channel,
	void *param
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	piCreateStagingRoomEnterChannelCallbackA(chat, success, result, channel_A, param);
	gsifree(channel_A);
}
#endif

// internal QR2 function
qr2_error_t qr2_create_socket(/*[out]*/SOCKET *sock, const char *ip, /*[in/out]*/int * port);

PEERBool piNewCreateStagingRoomOperation
(
	PEER peer,
	const char * name,
	const char * password,
	int maxPlayers,
	SOCKET socket,
	unsigned short port,
	peerJoinRoomCallback callback,
	void * callbackParam,
	int opID
)
{
	piOperation * operation;
	chatChannelCallbacks channelCallbacks;
	char room[PI_ROOM_MAX_LEN];
	PEERBool createdSocket = PEERFalse;

	PEER_CONNECTION;

	assert(name);
	if(!name)
		name = "";

	assert(callback);
	if(!callback)
		return PEERFalse;

	// Save off the maxplayers.
	///////////////////////////
	connection->maxPlayers = maxPlayers;

	// If we don't have a socket, create one to use.
	////////////////////////////////////////////////
	if(socket == INVALID_SOCKET)
	{
		IN_ADDR addr;
		qr2_error_t rcode;
		int privatePort;

		addr.s_addr =  0;//crt -- don't bind to privateIP (for clients that have both public and private)  connection->privateIP;
		if(port)
			privatePort = port;
		else
			privatePort = PI_QUERYPORT;

		rcode = qr2_create_socket(&socket, inet_ntoa(addr), &privatePort);
		if(rcode != e_qrnoerror)
			return PEERFalse;

		port = (unsigned short)privatePort;
		createdSocket = PEERTrue;
	}

	// Get the room name.
	/////////////////////
	piMangleStagingRoom(room, connection->title, connection->publicIP, connection->privateIP, port);
	assert(room[0]);

	// Add the operation.
	/////////////////////
	operation = piAddOperation(peer, PI_CREATE_ROOM_OPERATION, NULL, (PEERCBType)callback, callbackParam, opID);
	if(!operation)
		return PEERFalse;
	operation->socketClose = createdSocket;
	operation->name = goastrdup(name);
	if(!operation->name)
	{
		piRemoveOperation(peer, operation);
		return PEERFalse;
	}
	operation->socket = socket;
	operation->port = port;
	operation->roomType = StagingRoom;
	if(password[0])
		operation->password = goastrdup(password);

	// Set the callbacks.
	/////////////////////
	piSetChannelCallbacks(peer, &channelCallbacks);

	// Create the room.
	///////////////////
	piStartedEnteringRoom(peer, StagingRoom, room);
	chatEnterChannelA(connection->chat, room, NULL, &channelCallbacks, piCreateStagingRoomEnterChannelCallback, operation, CHATFalse);

	// Store passworded flag if needed.
	///////////////////////////////////
	if(password[0])
		connection->passwordedRoom = PEERTrue;

	return PEERTrue;
}

/* Join Room.
************/
static void piJoinRoomEnumUsersCallbackA
(
	CHAT chat,
	CHATBool success,
	const char * channel,
	int numUsers,
	const char ** users,
	int * modes,
	void * param
)
{	
	PEER_CONNECTION_OP;

	// Check if this was cancelled.
	///////////////////////////////
	if(operation->cancel)
	{
		piRemoveOperation(peer, operation);
		return;
	}

	// Check for success.
	/////////////////////
	if(success)
	{
		int i;

		// Finished entering the room.
		//////////////////////////////
		piFinishedEnteringRoom(peer, operation->roomType, "");

		// Add all these people to the room.
		////////////////////////////////////
		for(i = 0 ; i < numUsers ; i++)
			piPlayerJoinedRoom(peer, users[i], operation->roomType, modes[i]);
	}
	else
	{
		// Leave the room.
		//////////////////
		piLeaveRoom(peer, operation->roomType, NULL);
	}

	// Add the callback.
	////////////////////
	piAddJoinRoomCallback(peer, (PEERBool)success, success?PEERJoinSuccess:PEERJoinFailed, operation->roomType, (peerJoinRoomCallback)operation->callback, operation->callbackParam, operation->ID);

	// Remove the operation.
	////////////////////////
	piRemoveOperation(peer, operation);
	
	GSI_UNUSED(chat);
	GSI_UNUSED(channel);
}
#ifdef GSI_UNICODE
static void piJoinRoomEnumUsersCallbackW
(
	CHAT chat,
	CHATBool success,
	const unsigned short * channel,
	int numUsers,
	const unsigned short ** users,
	int * modes,
	void * param
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char** users_A = UCS2ToUTF8StringArrayAlloc(users, numUsers);
	int i;
	piJoinRoomEnumUsersCallbackA(chat, success, channel_A, numUsers, (const char**)users_A, modes, param);
	gsifree(channel_A);
	for (i=0; i<numUsers; i++)
		gsifree(users_A[i]);
	gsifree(users_A);
}
#endif

static void piJoinRoomEnterChannelCallbackA
(
	CHAT chat,
	CHATBool success,
	CHATEnterResult result,
	const char * channel,
	void *param
)
{	
	PEER_CONNECTION_OP;

	assert(channel);
	assert(channel[0]);

	// Check if this was cancelled.
	///////////////////////////////
	if(operation->cancel)
	{
		piSBFreeHostServer(peer);
		piRemoveOperation(peer, operation);
		return;
	}

	if(success)
	{
		// How many users?
		//////////////////
		chatEnumUsersA(chat, channel, piJoinRoomEnumUsersCallback, operation, CHATFalse);
	}
	else
	{
		PEERJoinResult joinResult;

		// Not entering.
		////////////////
		piLeaveRoom(peer, operation->roomType, NULL);

		// Get the peer result.
		///////////////////////
		if(result == CHATEnterSuccess)
			joinResult = PEERJoinFailed;
		else
			joinResult = piEnterResultToJoinResult(result);

		// Add the callback.
		////////////////////
		piAddJoinRoomCallback(peer, PEERFalse, joinResult, operation->roomType, (peerJoinRoomCallback)operation->callback, operation->callbackParam, operation->ID);

		// Remove the operation.
		////////////////////////
		piRemoveOperation(peer, operation);
	}
}
#ifdef GSI_UNICODE
static void piJoinRoomEnterChannelCallbackW
(
	CHAT chat,
	CHATBool success,
	CHATEnterResult result,
	const unsigned short * channel,
	void *param
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	piJoinRoomEnterChannelCallbackA(chat, success, result, channel_A, param);
	gsifree(channel_A);
}
#endif

PEERBool piNewJoinRoomOperation
(
	PEER peer,
	RoomType roomType,
	const char * channel,
	const char * password,
	peerJoinRoomCallback callback,
	void * callbackParam,
	int opID
)
{
	piOperation * operation;
	chatChannelCallbacks channelCallbacks;

	PEER_CONNECTION;

	ASSERT_ROOMTYPE(roomType);
	assert(callback);

	// Check the name.
	//////////////////
	assert(channel);
	assert(channel[0]);
	if(!channel || !channel[0])
		return PEERFalse;

	// Check password.
	//////////////////
	if(!password)
		password = "";

	// Check that the name isn't too long.
	//////////////////////////////////////
	assert(strlen(channel) < PI_ROOM_MAX_LEN);
	if(strlen(channel) >= PI_ROOM_MAX_LEN)
		return PEERFalse;

	// Add the operation.
	/////////////////////
	operation = piAddOperation(peer, PI_JOIN_ROOM_OPERATION, NULL, (PEERCBType)callback, callbackParam, opID);
	if(!operation)
		return PEERFalse;
	operation->roomType = roomType;

	// Set the callbacks.
	/////////////////////
	piSetChannelCallbacks(peer, &channelCallbacks);

	// Join the room.
	/////////////////
	piStartedEnteringRoom(peer, roomType, channel);
	chatEnterChannelA(connection->chat, channel, password, &channelCallbacks, piJoinRoomEnterChannelCallback, operation, CHATFalse);

	return PEERTrue;
}

/* List Group Rooms.
*******************/
PEERBool piNewListGroupRoomsOperation
(
	PEER peer,
	const char * fields,
	peerListGroupRoomsCallback callback,
	void * param,
	int opID
)
{
	piOperation * operation;

	PEER_CONNECTION;

	assert(callback);

	// Add the operation.
	/////////////////////
	operation = connection->listingGroupsOperation = piAddOperation(peer, PI_LIST_GROUP_ROOMS_OPERATION, NULL, (PEERCBType)callback, param, opID);
	if(!operation)
		return PEERFalse;

	// Start the listing.
	/////////////////////
	return piSBStartListingGroups(peer, fields);
}

/* Get Player Info.
*******************/
static void piGetPlayerInfoCallbackA
(
	CHAT chat,
	CHATBool success,
	const char * nick,
	const char * user,
	const char * address,
	void * param
)
{
	int profileID = 0;
	unsigned int IP = 0;

	PEER_CONNECTION_OP;

	assert(nick);
	assert(nick[0]);

	// Check for success.
	/////////////////////
	if(success)
	{
		assert(user);
		assert(user[0]);

		// Get the info.
		////////////////
		if(!piDemangleUser(user, &IP, &profileID))
			success = CHATFalse;

		// Cache the info.
		//////////////////
		if(success)
			piSetPlayerIPAndProfileID(peer, nick, IP, profileID);
	}
	if(!success)
	{
		profileID = 0;
		IP = 0;
	}

	// Add the callback.
	////////////////////
	if(operation->callback)
	{
		if(operation->type == PI_GET_PLAYER_INFO_OPERATION)
			piAddGetPlayerInfoCallback(peer, (PEERBool)success, nick, IP, profileID, (peerGetPlayerInfoCallback)operation->callback, operation->callbackParam, operation->ID);
		else if(operation->type == PI_GET_PROFILE_ID_OPERATION)
			piAddGetPlayerProfileIDCallback(peer, (PEERBool)success, nick, profileID, (peerGetPlayerProfileIDCallback)operation->callback, operation->callbackParam, operation->ID);
		else if(operation->type == PI_GET_IP_OPERATION)
			piAddGetPlayerIPCallback(peer, (PEERBool)success, nick, IP, (peerGetPlayerIPCallback)operation->callback, operation->callbackParam, operation->ID);
		else
			assert(0);
	}

	// Remove the operation.
	////////////////////////
	piRemoveOperation(peer, operation);
	
	GSI_UNUSED(chat);
	GSI_UNUSED(address);
}
#ifdef GSI_UNICODE
static void piGetPlayerInfoCallbackW
(
	CHAT chat,
	CHATBool success,
	const unsigned short * nick,
	const unsigned short * user,
	const unsigned short * address,
	void * param
)
{
	char* nick_A = UCS2ToUTF8StringAlloc(nick);
	char* user_A = UCS2ToUTF8StringAlloc(user);
	char* address_A = UCS2ToUTF8StringAlloc(address);
	piGetPlayerInfoCallbackA(chat, success, nick_A, user_A, address_A, param);
	gsifree(nick_A);
	gsifree(user_A);
	gsifree(address_A);
}
#endif

PEERBool piNewGetPlayerInfoOperation
(
	PEER peer,
	const char * nick,
	peerGetPlayerInfoCallback callback,
	void * param,
	int opID
)
{
	piOperation * operation;

	PEER_CONNECTION;

	assert(nick);
	assert(nick[0]);

	// Add the operation.
	/////////////////////
	operation = piAddOperation(peer, PI_GET_PLAYER_INFO_OPERATION, NULL, (PEERCBType)callback, param, opID);
	if(!operation)
		return PEERFalse;

	// Get the user's info.
	///////////////////////
	chatGetBasicUserInfoA(connection->chat, nick, piGetPlayerInfoCallback, operation, CHATFalse);

	return PEERTrue;
}

PEERBool piNewGetProfileIDOperation
(
	PEER peer,
	const char * nick,
	peerGetPlayerProfileIDCallback callback,
	void * param,
	int opID
)
{
	piOperation * operation;

	PEER_CONNECTION;

	assert(nick);
	assert(nick[0]);

	// Add the operation.
	/////////////////////
	operation = piAddOperation(peer, PI_GET_PROFILE_ID_OPERATION, NULL, (PEERCBType)callback, param, opID);
	if(!operation)
		return PEERFalse;

	// Get the user's info.
	///////////////////////
	chatGetBasicUserInfoA(connection->chat, nick, piGetPlayerInfoCallback, operation, CHATFalse);

	return PEERTrue;
}

PEERBool piNewGetIPOperation
(
	PEER peer,
	const char * nick,
	peerGetPlayerIPCallback callback,
	void * param,
	int opID
)
{
	piOperation * operation;

	PEER_CONNECTION;

	assert(nick);
	assert(nick[0]);

	// Add the operation.
	/////////////////////
	operation = piAddOperation(peer, PI_GET_IP_OPERATION, NULL, (PEERCBType)callback, param, opID);
	if(!operation)
		return PEERFalse;

	// Get the user's info.
	///////////////////////
	chatGetBasicUserInfoA(connection->chat, nick, piGetPlayerInfoCallback, operation, CHATFalse);

	return PEERTrue;
}

/* Change Nick.
**************/
static void piChangeNickCallbackA
(
	CHAT chat,
	CHATBool success,
	const char * oldNick,
	const char * newNick,
	void * param
)
{
	PEER_CONNECTION_OP;

	// Check for success.
	/////////////////////
	if(success)
	{
		// Update the nick locally.
		///////////////////////////
		strcpy(connection->nick, newNick);

#ifdef GSI_UNICODE
		UTF8ToUCS2String(connection->nick, connection->nick_W);
#endif
	}

	// Add the callback.
	////////////////////
	if(operation->callback)
		piAddChangeNickCallback(peer, (PEERBool)success, oldNick, newNick, (peerChangeNickCallback)operation->callback, operation->callbackParam, operation->ID);

	// Remove the operation.
	////////////////////////
	piRemoveOperation(peer, operation);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piChangeNickCallbackW
(
	CHAT chat,
	CHATBool success,
	const unsigned short * oldNick,
	const unsigned short * newNick,
	void * param
)
{
	char* oldNick_A = UCS2ToUTF8StringAlloc(oldNick);
	char* newNick_A = UCS2ToUTF8StringAlloc(newNick);
	piChangeNickCallbackA(chat, success, oldNick_A, newNick_A, param);
	gsifree(oldNick_A);
	gsifree(newNick_A);
}
#endif

PEERBool piNewChangeNickOperation
(
	PEER peer,
	const char * newNick,
	peerChangeNickCallback callback,
	void * param,
	int opID
)
{
	piOperation * operation;

	PEER_CONNECTION;

	assert(newNick);
	assert(newNick[0]);

	// Add the operation.
	/////////////////////
	operation = piAddOperation(peer, PI_CHANGE_NICK_OPERATION, NULL, (PEERCBType)callback, param, opID);
	if(!operation)
		return PEERFalse;

	// Change the nick.
	///////////////////
	chatChangeNickA(connection->chat, newNick, piChangeNickCallback, operation, CHATFalse);

	return PEERTrue;
}

/* Get Global Keys.
******************/
static void piGetGlobalKeysCallbackA
(
	CHAT chat,
	CHATBool success,
	const char * user,
	int num,
	const char ** keys,
	const char ** values,
	void * param
)
{
	int i;

	PEER_CONNECTION_OP;

	// Update the watch keys.
	/////////////////////////
	if(success && user)
	{
		for(i = 0 ; i < num ; i++)
			piGlobalKeyChanged(peer, user, keys[i], values[i]);
	}

	// Add the callback.
	////////////////////
	if(operation->callback)
		piAddGetGlobalKeysCallback(peer, (PEERBool)success, user, num, keys, values, (peerGetGlobalKeysCallback)operation->callback, operation->callbackParam, operation->ID);

	// Remove the operation.
	////////////////////////
	if(!success || !user || !operation->num)
		piRemoveOperation(peer, operation);

	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piGetGlobalKeysCallbackW
(
	CHAT chat,
	CHATBool success,
	const unsigned short * user,
	int num,
	const unsigned short ** keys,
	const unsigned short ** values,
	void * param
)
{
	char* user_A = UCS2ToUTF8StringAlloc(user);
	char** keys_A = UCS2ToUTF8StringArrayAlloc(keys, num);
	char** values_A = UCS2ToUTF8StringArrayAlloc(values, num);
	int i;
	piGetGlobalKeysCallbackA(chat, success, user_A, num, (const char**)keys_A, (const char**)values_A, param);
	gsifree(user_A);
	for (i=0; i<num; i++)
	{
		gsifree(keys_A[i]);
		if (values_A != NULL) // may be a NULL when getting keys for "*"
			gsifree(values_A[i]);
	}
	gsifree(keys_A);
	gsifree(values_A);
}
#endif

PEERBool piNewGetGlobalKeysOperation
(
	PEER peer,
	const char * target,
	int num,
	const char ** keys,
	peerGetGlobalKeysCallback callback,
	void * param,
	int opID
)
{
	piOperation * operation;

	PEER_CONNECTION;

	assert(target);
	assert(num > 0);
	assert(keys);

	if(!target || !target[0])
		return PEERFalse;
	if(num <= 0)
		return PEERFalse;

	// Add the operation.
	/////////////////////
	operation = piAddOperation(peer, PI_GET_GLOBAL_KEYS_OPERATION, NULL, (PEERCBType)callback, param, opID);
	if(!operation)
		return PEERFalse;

	// Use num as a flag for getting a whole channel.
	/////////////////////////////////////////////////
	operation->num = (target[0] == '#');

	// Get the keys.
	////////////////
	chatGetGlobalKeysA(connection->chat, target, num, keys, piGetGlobalKeysCallback, operation, CHATFalse);

	return PEERTrue;
}

/* Get Room Keys.
****************/
static void piGetChannelKeysCallbackA
(
	CHAT chat,
	CHATBool success,
	const char * channel,
	const char * user,
	int num,
	const char ** keys,
	const char ** values,
	void * param
)
{
	int i;

	PEER_CONNECTION_OP;

	// Update the watch keys.
	/////////////////////////
	if(success && user)
	{
		for(i = 0 ; i < num ; i++)
			piRoomKeyChanged(peer, operation->roomType, user, keys[i], values[i]);
	}

	// Add the callback.
	////////////////////
	if(operation->callback)
		piAddGetRoomKeysCallback(peer, (PEERBool)success, operation->roomType, user, num, keys, values, (peerGetRoomKeysCallback)operation->callback, operation->callbackParam, operation->ID);

	// Remove the operation.
	////////////////////////
	if(!success || !user || !operation->num)
		piRemoveOperation(peer, operation);

	GSI_UNUSED(chat);
	GSI_UNUSED(channel);
}
#ifdef GSI_UNICODE
static void piGetChannelKeysCallbackW
(
	CHAT chat,
	CHATBool success,
	const unsigned short * channel,
	const unsigned short * user,
	int num,
	const unsigned short ** keys,
	const unsigned short ** values,
	void * param
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char* user_A = UCS2ToUTF8StringAlloc(user);
	char** keys_A = UCS2ToUTF8StringArrayAlloc(keys, num);
	char** values_A = UCS2ToUTF8StringArrayAlloc(values, num);
	int i;
	piGetChannelKeysCallbackA(chat, success, channel_A, user_A, num, (const char**)keys_A, (const char**)values_A, param);
	gsifree(channel_A);
	gsifree(user_A);
	for (i=0; i<num; i++)
	{
		gsifree(keys_A[i]);
		if (values_A != NULL) // may be a NULL when getting keys for "*"
			gsifree(values_A[i]);
	}
	gsifree(keys_A);
	gsifree(values_A);

}
#endif

PEERBool piNewGetRoomKeysOperation
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	int num,
	const char ** keys,
	peerGetRoomKeysCallback callback,
	void * param,
	int opID
)
{
	piOperation * operation;

	PEER_CONNECTION;

	ASSERT_ROOMTYPE(roomType);
	assert(num >= 0);
	assert(!num || keys);

	if(num < 0)
		return PEERFalse;
	if((num > 0) && !keys)
		return PEERFalse;

	if(!ENTERING_ROOM && !IN_ROOM)
		return PEERFalse;

	// Add the operation.
	/////////////////////
	operation = piAddOperation(peer, PI_GET_ROOM_KEYS_OPERATION, NULL, (PEERCBType)callback, param, opID);
	if(!operation)
		return PEERFalse;
	operation->roomType = roomType;

	// Use num as a flag for getting a whole channel.
	/////////////////////////////////////////////////
	if(nick)
		operation->num = (strcmp(nick, "*") == 0);
	else
		operation->num = 0;

	// Get the keys.
	////////////////
	chatGetChannelKeysA(connection->chat, ROOM, nick, num, keys, piGetChannelKeysCallback, operation, CHATFalse);

	return PEERTrue;
}

/* Authenticate CD Key
*********************/
static void piAuthenticateCDKeyCallbackA
(
	CHAT chat,
	int result,
	const char * message,
	void * param
)
{

	PEER_CONNECTION_OP;

	// Add the callback.
	////////////////////
	if(operation->callback)
		piAddAuthenticateCDKeyCallback(peer, result, message, (peerAuthenticateCDKeyCallback)operation->callback, operation->callbackParam, operation->ID);

	// Remove the operation.
	////////////////////////
	piRemoveOperation(peer, operation);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piAuthenticateCDKeyCallbackW
(
	CHAT chat,
	int result,
	const unsigned short * message,
	void * param
)
{
	char* message_A = UCS2ToUTF8StringAlloc(message);
	piAuthenticateCDKeyCallbackA(chat, result, message_A, param);
	gsifree(message_A);
}
#endif

PEERBool piNewAuthenticateCDKeyOperation
(
	PEER peer,
	const char * cdkey,
	peerAuthenticateCDKeyCallback callback,
	void * param,
	int opID
)
{
	piOperation * operation;

	PEER_CONNECTION;

	assert(cdkey);
	assert(cdkey[0]);

	// Add the operation.
	/////////////////////
	operation = piAddOperation(peer, PI_AUTHENTICATE_CDKEY_OPERATION, NULL, (PEERCBType)callback, param, opID);
	if(!operation)
		return PEERFalse;

	// Check the CD key.
	////////////////////
	chatAuthenticateCDKeyA(connection->chat, cdkey, piAuthenticateCDKeyCallback, operation, CHATFalse);

	return PEERTrue;
}

/* AutoMatch
***********/
PEERBool piNewAutoMatchOperation
(
	PEER peer,
	SOCKET socket,
	unsigned short port,
	peerAutoMatchStatusCallback statusCallback,
	peerAutoMatchRateCallback rateCallback,
	void * param,
	int opID
)
{
	piOperation * operation;
	PEERAutoMatchStatus status;

	PEER_CONNECTION;

	// Add the operation.
	/////////////////////
	operation = piAddOperation(peer, PI_AUTO_MATCH_OPERATION, NULL, (PEERCBType)statusCallback, param, opID);
	if(!operation)
		return PEERFalse;

	// Store the second callback.
	/////////////////////////////
	operation->callback2 = (PEERCBType)rateCallback;

	// Store the socket and port.
	/////////////////////////////
	operation->socket = socket;
	operation->port = port;

	// Track this op.
	/////////////////
	connection->autoMatchOperation = operation;

	// Figure out which status to use.
	//////////////////////////////////
/*	if(connection->inRoom[StagingRoom])
	{
		if(connection->numPlayers[StagingRoom] <= 1)
		{
			if(connection->hosting)
				status = PEERWaiting;
			else
				status = PEERStaging;
		}
		else if(connection->numPlayers[StagingRoom] >= connection->maxPlayers)
		{
			status = PEERReady;
		}
		else
		{
			status = PEERStaging;
		}
	}
	else*/
	{
		status = PEERSearching;
	}

	// Set the status.
	//////////////////
	piSetAutoMatchStatus(peer, status);

	return PEERTrue;
}
