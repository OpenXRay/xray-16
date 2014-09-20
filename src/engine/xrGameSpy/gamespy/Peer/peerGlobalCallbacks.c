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
#include <stdlib.h>
#include <stdio.h>
#include "peer.h"
#include "peerAscii.h"
#include "peerMain.h"
#include "peerGlobalCallbacks.h"
#include "peerRooms.h"
#include "peerPlayers.h"
#include "peerCallbacks.h"
#include "peerOperations.h"
#include "peerPing.h"
#include "peerMangle.h"
#include "peerKeys.h"
#include "peerAutoMatch.h"

/************
** DEFINES **
************/
#define PI_UTM_MATCH(utm)       (strncmp(piUTMCommand, utm, strlen(utm) - 1) == 0)

#define PI_UTM_COMMAND_LEN      8
#define PI_UTM_PARAMATERS_LEN   512

/************
** GLOBALS **
************/
static char piUTMCommand[PI_UTM_COMMAND_LEN];
static char piUTMParameters[PI_UTM_PARAMATERS_LEN];

/**************
** FUNCTIONS **
**************/
// Prototype the ASCII versions that are needed even in unicode mode
void peerMessagePlayerA(PEER peer, const char * nick, const char * message,	MessageType messageType);


/* Chat.
*******/
static PEERBool piIsOldUTM
(
	const char * message
)
{
	// Check for no message.
	////////////////////////
	assert(message);
	if(!message)
		return PEERFalse;

	// Check for too short for prefix + 1 char.
	///////////////////////////////////////////
	if(strlen(message) < 4)
		return PEERFalse;

	// Check for no prefix.
	///////////////////////
	if((message[0] != '@') ||
		(message[1] != '@') ||
		(message[2] != '@') ||
		(message[3] == ' '))
	{
		return PEERFalse;
	}

	return PEERTrue;
}

// Returns PEERTrue if a UTM.
/////////////////////////////
static PEERBool piParseUTM
(
	const char * message
)
{
	int len;

	// Check for no message.
	////////////////////////
	assert(message);
	if(!message)
		return PEERFalse;

	// Find the end of the command.
	///////////////////////////////
	len = (int)strcspn(message, "/ ");
	if(len >= PI_UTM_COMMAND_LEN)
		return PEERFalse;
	memcpy(piUTMCommand, message, (unsigned int)len);
	piUTMCommand[len] = '\0';

	// Copy off the parameters.
	///////////////////////////
	message += len;
	if(message[0])
	{
		message++;
		if(strlen(message) >= PI_UTM_PARAMATERS_LEN)
			return PEERFalse;
		strcpy(piUTMParameters, message);
	}
	else
	{
		piUTMParameters[0] = '\0';
	}

	return PEERTrue;
}

static void piProcessUTM
(
	PEER peer,
	piPlayer * player,
	PEERBool inRoom,
	RoomType roomType
)
{
	char * params = piUTMParameters;

	PEER_CONNECTION;

	assert(piUTMCommand[0]);
	assert(player);

	if(PI_UTM_MATCH(PI_UTM_LAUNCH))
	{
#ifdef _DEBUG
		assert(connection->inRoom[StagingRoom]);
		if(inRoom)
			assert(roomType == StagingRoom);
		else
			assert(player->inRoom[StagingRoom]);
#endif
		if(!connection->inRoom[StagingRoom])
			return;
		if(inRoom && (roomType != StagingRoom))
			return;
		if(!inRoom && !player->inRoom[StagingRoom])
			return;

		// Ignore if we're hosting.
		///////////////////////////
		if(connection->hosting)
			return;

		// Only accept launch from ops.
		///////////////////////////////
		if(!piIsPlayerOp(player))
			return;

		// We're playing.
		/////////////////
		connection->playing = PEERTrue;

		// Set the flags.
		/////////////////
		piSetLocalFlags(peer);

		// Add the callback.
		////////////////////
		piAddGameStartedCallback(peer, connection->hostServer, params);

		// If we're AutoMatching, we're now done.
		/////////////////////////////////////////
		if(peerIsAutoMatching(peer))
			piSetAutoMatchStatus(peer, PEERComplete);
	}
	else if(PI_UTM_MATCH(PI_UTM_XPING))
	{
		piPlayer * other;
		int ping;
		unsigned int IP;

#ifdef _DEBUG
//		if(inRoom)
//			assert(connection->xpingRoom[roomType]);
#endif
		if(inRoom && !connection->xpingRoom[roomType])
			return;

		// Check for no params.
		///////////////////////
		if(!params[0])
			return;

		// Get the IP.
		//////////////
		IP = piDemangleIP(params);

		// Get the ping.
		////////////////
		params += 11;
		ping = atoi(params);

		// Figure out who this ping is to.
		//////////////////////////////////
		other = piFindPlayerByIP(peer, IP);
		if(!other)
			return;
		if(strcasecmp(player->nick, other->nick) == 0)
			return;
		if(inRoom && !player->inRoom[roomType])
			return;
		if(!inRoom)
		{
			int i;
			PEERBool success = PEERFalse;

			// Check that the three of us are in a room with xping enabled.
			///////////////////////////////////////////////////////////////
			for(i = 0 ; i < NumRooms ; i++)
			{
				if(connection->xpingRoom[i] && connection->inRoom[i] && player->inRoom[i] && other->inRoom[i])
					success = PEERTrue;
			}
			if(!success)
				return;
		}

		// Update.
		//////////
		piUpdateXping(peer, player->nick, other->nick, ping);

		// Add a xping callback.
		////////////////////////
		piAddCrossPingCallback(peer, player->nick, other->nick, ping);
	}
}

void piChatDisconnectedA
(
	CHAT chat,
	const char * reason,
	PEER peer
)
{
	PEER_CONNECTION;

	// We're disconnected.
	//////////////////////
	connection->disconnect = PEERTrue;

	// Add the disconnected callback.
	/////////////////////////////////
	piAddDisconnectedCallback(peer, reason);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
void piChatDisconnectedW
(
	CHAT chat,
	const unsigned short * reason,
	PEER peer
)
{
	char* reason_A = UCS2ToUTF8StringAlloc(reason);
	piChatDisconnectedA(chat, reason_A, peer);
	gsifree(reason_A);
}
#endif

static void piHandleOldNFO
(
	PEER peer,
	piPlayer * player,
	const char * message
)
{
	// Ignore old NFOs from new clients.
	////////////////////////////////////
	if(strncmp(message + strlen(message) - 2, "X\\", 2) != 0)
	{
		const char * str;

		if(!player->inRoom[StagingRoom])
			return;

		str = strstr(message, "\\$flags$\\");
		if(str)
		{
			PEERBool ready = PEERFalse;
			str += 9;
			while(*str && (*str != '\\'))
			{
				if(*str++ == 'r')
				{
					ready = PEERTrue;
					break;
				}
			}

			if(ready)
				player->flags[StagingRoom] |= PEER_FLAG_READY;
			else
				player->flags[StagingRoom] &= ~PEER_FLAG_READY;
		}
	}
	
	GSI_UNUSED(peer);
}

void piChatPrivateMessageA
(
	CHAT chat,
	const char * user,
	const char * message,
	int type,
	PEER peer
)
{
	assert(message);

	if(!user || !user[0])
		return;

	// Check for old-style UTMs.
	////////////////////////////
	if(piIsOldUTM(message))
	{
		// Check for ready.
		///////////////////
		if(strncasecmp(message, "@@@NFO", 6) == 0)
		{
			piPlayer * player;

			player = piGetPlayer(peer, user);
			if(player)
				piHandleOldNFO(peer, player, message);
		}

		return;
	}

	// Check if it's a UTM.
	///////////////////////
	if((type == CHAT_UTM) || (type == CHAT_ATM))
	{
		if(piParseUTM(message))
		{
			piPlayer * player;

			// Get the player it's from.
			////////////////////////////
			player = piGetPlayer(peer, user);
			if(player)
			{
				// Process it.
				//////////////
				piProcessUTM(peer, player, PEERFalse, (RoomType)0);
			}

			// Pass it along.
			/////////////////
			piAddPlayerUTMCallback(peer, user, piUTMCommand, piUTMParameters, (PEERBool)(type == CHAT_ATM));
		}

		return;
	}

	// It's a regular message, deliver it.
	//////////////////////////////////////
	piAddPlayerMessageCallback(peer, user, message, (MessageType)type);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
void piChatPrivateMessageW
(
	CHAT chat,
	const unsigned short * user,
	const unsigned short * message,
	int type,
	PEER peer
)
{
	char* user_A = UCS2ToUTF8StringAlloc(user);
	char* message_A = UCS2ToUTF8StringAlloc(message);
	piChatPrivateMessageA(chat, user_A, message_A, type, peer);
	gsifree(user_A);
	gsifree(message_A);
}
#endif

static void piChannelMessageA
(
	CHAT chat,
	const char * channel,
	const char * user,
	const char * message,
	int type,
	PEER peer
)
{
	piPlayer * player;
	RoomType roomType;

	//PEER_CONNECTION;

	assert(message);

	// Check the room type.
	///////////////////////
	if(!piRoomToType(peer, channel, &roomType))
		return;

	// Get the player.
	//////////////////
	player = piGetPlayer(peer, user);

	// Check for old-style UTMs.
	////////////////////////////
	if(player && piIsOldUTM(message))
	{
		// Only take stuff in staging rooms.
		////////////////////////////////////
		if(roomType != StagingRoom)
			return;

		// Check for a launch.
		//////////////////////
		if(strncasecmp(message, "@@@GML", 6) == 0)
		{
			// Ignore old launches from new clients.
			////////////////////////////////////////
			if(strncmp(message + strlen(message) - 4, "/OLD", 4) == 0)
				return;

			// Convert this into its modern equivalent.
			///////////////////////////////////////////
			type = CHAT_UTM;
			message = "GML";
		}
		// Check for ready.
		///////////////////
		else if(strncasecmp(message, "@@@NFO", 6) == 0)
		{
			piHandleOldNFO(peer, player, message);

			return;
		}
		else
		{
			return;
		}
	}

	// Check if it's a UTM.
	///////////////////////
	if((type == CHAT_UTM) || (type == CHAT_ATM))
	{
		if(piParseUTM(message))
		{
			// Process it.
			//////////////
			if(player)
				piProcessUTM(peer, player, PEERTrue, roomType);

			// Pass it along.
			/////////////////
			piAddRoomUTMCallback(peer, roomType, user, piUTMCommand, piUTMParameters, (type == CHAT_ATM)?PEERTrue:PEERFalse);
		}

		return;
	}

	// Add the callback.
	////////////////////
	piAddRoomMessageCallback(peer, roomType, user, message, (MessageType)type);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piChannelMessageW
(
	CHAT chat,
	const unsigned short * channel,
	const unsigned short * user,
	const unsigned short * message,
	int type,
	PEER peer
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char* user_A = UCS2ToUTF8StringAlloc(user);
	char* message_A = UCS2ToUTF8StringAlloc(message);
	piChannelMessageA(chat, channel_A, user_A, message_A, type, peer);
	gsifree(channel_A);
	gsifree(user_A);
	gsifree(message_A);
}
#endif

static void piChannelKickedA
(
	CHAT chat,
	const char * channel,
	const char * user,
	const char * reason,
	PEER peer
)
{
	RoomType roomType;

	//PEER_CONNECTION;

	// Figure out the room type.
	////////////////////////////
	if(!piRoomToType(peer, channel, &roomType))
		return;

	// Leave the room.
	//////////////////
	piLeaveRoom(peer, roomType, NULL);

	// Add the callback.
	////////////////////
	piAddKickedCallback(peer, roomType, user, reason);
	
	// If we were kicked from an AutoMatch room, start searching.
	/////////////////////////////////////////////////////////////
	if((roomType == StagingRoom) && peerIsAutoMatching(peer))
		piSetAutoMatchStatus(peer, PEERSearching);
		
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piChannelKickedW
(
	CHAT chat,
	const unsigned short * channel,
	const unsigned short * user,
	const unsigned short * reason,
	PEER peer
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char* user_A = UCS2ToUTF8StringAlloc(user);
	char* reason_A = UCS2ToUTF8StringAlloc(reason);
	piChannelKickedA(chat, channel_A, user_A, reason_A, peer);
	gsifree(channel_A);
	gsifree(user_A);
	gsifree(reason_A);
}
#endif

static void piChannelUserJoinedA
(
	CHAT chat,
	const char * channel,
	const char * user,
	int mode,
	PEER peer
)
{
	RoomType roomType;
	piPlayer * player;

	PEER_CONNECTION;

	// Figure out the room type.
	////////////////////////////
	if(!piRoomToType(peer, channel, &roomType))
		return;

	// Add this player to the room.
	///////////////////////////////
	player = piPlayerJoinedRoom(peer, user, roomType, mode);
	if(!player)
		return;

	// Get IP and profile ID if we don't already have it.
	/////////////////////////////////////////////////////
	if(!player->gotIPAndProfileID)
	{
		const char * info;
		unsigned int IP;
		int profileID;

		if(chatGetBasicUserInfoNoWaitA(connection->chat, user, &info, NULL) && piDemangleUser(info, &IP, &profileID))
		{
			piSetPlayerIPAndProfileID(peer, user, IP, profileID);
		}
	}

	// Refresh this player's watch keys.
	////////////////////////////////////
	piKeyCacheRefreshPlayer(peer, roomType, user);

	// Add the callback.
	////////////////////
	piAddPlayerJoinedCallback(peer, roomType, user);

#if 1
	// If this is the staging room, send our ready state.
	/////////////////////////////////////////////////////
	if((roomType == StagingRoom) && connection->ready)
		peerMessagePlayerA(peer, user, "@@@NFO \\$flags$\\rX\\", NormalMessage);
#endif

	// Check if this is an AutoMatch room.
	//////////////////////////////////////
	if((roomType == StagingRoom) && peerIsAutoMatching(peer))
	{
		// If we are Waiting, we're now Staging.
		////////////////////////////////////////
		if(connection->autoMatchStatus == PEERWaiting)
			piSetAutoMatchStatus(peer, PEERStaging);

		// If we've got maxplayers, we're now Ready.
		////////////////////////////////////////////
		if((connection->autoMatchStatus == PEERStaging) && (connection->numPlayers[StagingRoom] >= connection->maxPlayers))
			piSetAutoMatchStatus(peer, PEERReady);
	}
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piChannelUserJoinedW
(
	CHAT chat,
	const unsigned short * channel,
	const unsigned short * user,
	int mode,
	PEER peer
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char* user_A = UCS2ToUTF8StringAlloc(user);
	piChannelUserJoinedA(chat, channel_A, user_A, mode, peer);
	gsifree(channel_A);
	gsifree(user_A);
}
#endif

static void piChannelUserPartedA
(
	CHAT chat,
	const char * channel,
	const char * user,
	int why,
	const char * reason,
	const char * kicker,
	PEER peer
)
{
	RoomType roomType;
	PEERAutoMatchStatus status = PEERFailed;
	PEERBool newStatus = PEERFalse;
	PEERBool autoMatchRoom;

	PEER_CONNECTION;

	// Figure out the room type.
	////////////////////////////
	if(!piRoomToType(peer, channel, &roomType))
		return;

	// Check if this is an AutoMatch room.
	//////////////////////////////////////
	autoMatchRoom = ((roomType == StagingRoom) && peerIsAutoMatching(peer))?PEERTrue:PEERFalse;
	if(autoMatchRoom)
	{
		piPlayer * player;

		// Get the players.
		///////////////////
		player = piGetPlayer(peer, user);

		// Check if he was the last op.
		///////////////////////////////
		if(!connection->hosting && piIsPlayerHost(player))
		{
			status = PEERSearching;
			newStatus = PEERTrue;
		}
		// Check for host and everyone left.
		////////////////////////////////////
		else if(connection->hosting && connection->numPlayers[StagingRoom] == 2)
		{
			status = PEERWaiting;
			newStatus = PEERTrue;
		}
		// Check for no longer at maxplayers.
		/////////////////////////////////////
		else if(connection->numPlayers[StagingRoom] == connection->maxPlayers)
		{
			status = PEERStaging;
			newStatus = PEERTrue;
		}
	}

	// Remove this player from the room.
	////////////////////////////////////
	piPlayerLeftRoom(peer, user, roomType);

	// Figure out the reason.
	/////////////////////////
	if((why == CHAT_KICKED) || (why == CHAT_KILLED))
		reason = "Kicked";
	else if(!reason)
		reason = "";

	// Add the callback.
	////////////////////
	piAddPlayerLeftCallback(peer, roomType, user, reason);

	// Set status if needed.
	////////////////////////
	if(newStatus)
		piSetAutoMatchStatus(peer, status);

	GSI_UNUSED(chat);
	GSI_UNUSED(kicker);
}
#ifdef GSI_UNICODE
static void piChannelUserPartedW
(
	CHAT chat,
	const unsigned short * channel,
	const unsigned short * user,
	int why,
	const unsigned short * reason,
	const unsigned short * kicker,
	PEER peer
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char* user_A = UCS2ToUTF8StringAlloc(user);
	char* reason_A = UCS2ToUTF8StringAlloc(reason);
	char* kicker_A = UCS2ToUTF8StringAlloc(kicker);
	piChannelUserPartedA(chat, channel_A, user_A, why, reason_A, kicker_A, peer);
	gsifree(channel_A);
	gsifree(user_A);
	gsifree(reason_A);
	gsifree(kicker_A);
}
#endif

static void piChannelUserChangedNickA
(
	CHAT chat,
	const char * channel,
	const char * oldNick,
	const char * newNick,
	PEER peer
)
{
	RoomType roomType;

	// Figure out the room type.
	////////////////////////////
	if(!piRoomToType(peer, channel, &roomType))
		return;

	// Change the nick locally.
	///////////////////////////
	piPlayerChangedNick(peer, oldNick, newNick);

	// Add the callback.
	////////////////////
	piAddPlayerChangedNickCallback(peer, roomType, oldNick, newNick);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piChannelUserChangedNickW
(
	CHAT chat,
	const unsigned short * channel,
	const unsigned short * oldNick,
	const unsigned short * newNick,
	PEER peer
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char* oldNick_A = UCS2ToUTF8StringAlloc(oldNick);
	char* newNick_A = UCS2ToUTF8StringAlloc(newNick);
	piChannelUserChangedNickA(chat, channel_A, oldNick_A, newNick_A, peer);
	gsifree(channel_A);
	gsifree(oldNick_A);
	gsifree(newNick_A);
}
#endif

static void piChannelTopicChangedA
(
	CHAT chat,
	const char * channel,
	const char * topic,
	PEER peer
)
{
	RoomType roomType;

	PEER_CONNECTION;

	// Figure out the room type.
	////////////////////////////
	if(!piRoomToType(peer, channel, &roomType))
		return;
	
	// Don't allow blank names.
	///////////////////////////
	if(!topic[0])
		return;

	// Is it the same as the old name?
	//////////////////////////////////
	//if(strcmp(NAME, topic) == 0)
	//	return;

	// Copy the new name.
	/////////////////////
	strzcpy(NAME, topic, PI_NAME_MAX_LEN);

	// Add a callback.
	//////////////////
	if(IN_ROOM)
		piAddRoomNameChangedCallback(peer, roomType);
		
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piChannelTopicChangedW
(
	CHAT chat,
	const unsigned short * channel,
	const unsigned short * topic,
	PEER peer
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char* topic_A = UCS2ToUTF8StringAlloc(topic);
	piChannelTopicChangedA(chat, channel_A, topic_A, peer);
	gsifree(channel_A);
	gsifree(topic_A);
}
#endif

static void piChannelNewUserListA
(
	CHAT chat,
	const char * channel,
	int num,
	const char ** users,
	int * modes,
	PEER peer
)
{
	int i;
	RoomType roomType;

	//PEER_CONNECTION;

	// Figure out the room type.
	////////////////////////////
	if(!piRoomToType(peer, channel, &roomType))
		return;

	// Clear all the players out of this room.
	//////////////////////////////////////////
	piClearRoomPlayers(peer, roomType);

	// Add all the new ones.
	////////////////////////
	for(i = 0 ; i < num ; i++)
		piPlayerJoinedRoom(peer, users[i], roomType, modes[i]);

	// Refresh keys.
	////////////////
	piKeyCacheRefreshRoom(peer, roomType);

	// Call the callback.
	/////////////////////
	piAddNewPlayerListCallback(peer, roomType);	
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piChannelNewUserListW
(
	CHAT chat,
	const unsigned short * channel,
	int num,
	const unsigned short ** users,
	int * modes,
	PEER peer
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char** users_A = UCS2ToUTF8StringArrayAlloc(users, num);
	int i;
	piChannelNewUserListA(chat, channel_A, num, (const char**)users_A, modes, peer);
	gsifree(channel_A);
	for (i=0; i<num; i++)
		gsifree(users_A[i]);
	gsifree(users_A);
}
#endif

static void piBroadcastKeyChangedA
(
	CHAT chat,
	const char * channel,
	const char * user,
	const char * key,
	const char * value,
	PEER peer
)
{
	RoomType roomType;

	//PEER_CONNECTION;

	// Figure out the room type.
	////////////////////////////
	if(!piRoomToType(peer, channel, &roomType))
		return;

	// Update the watch keys.
	/////////////////////////
	piRoomKeyChanged(peer, roomType, user, key, value);

	// Add the callback.
	////////////////////
		// Removed by Bill 11/20/2003
		// piRoomKeyChanged does this automatically for broadcast keys
//	piAddRoomKeyChangedCallback(peer, roomType, user, key, value);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piBroadcastKeyChangedW
(
	CHAT chat,
	const unsigned short * channel,
	const unsigned short * user,
	const unsigned short * key,
	const unsigned short * value,
	PEER peer
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char* user_A	= UCS2ToUTF8StringAlloc(user);
	char* key_A		= UCS2ToUTF8StringAlloc(key);
	char* value_A	= UCS2ToUTF8StringAlloc(value);
	piBroadcastKeyChangedA(chat, channel_A, user_A, key_A, value_A, peer);
	gsifree(channel_A);
	gsifree(user_A);
	gsifree(key_A);
	gsifree(value_A);
}
#endif

static void piUserModeChangedA
(
	CHAT chat,
	const char * channel,
	const char * user,
	int mode,
	PEER peer
)
{
	RoomType roomType;

	//PEER_CONNECTION;

	// Figure out the room type.
	////////////////////////////
	if(!piRoomToType(peer, channel, &roomType))
		return;

	// Set the user mode.
	/////////////////////
	piSetPlayerModeFlags(peer, user, roomType, mode);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piUserModeChangedW
(
	CHAT chat,
	const unsigned short * channel,
	const unsigned short * user,
	int mode,
	PEER peer
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	char* user_A = UCS2ToUTF8StringAlloc(user);
	piUserModeChangedA(chat, channel_A, user_A, mode, peer);
	gsifree(channel_A);
	gsifree(user_A);
}
#endif

static void piChannelModeChangedA
(
	CHAT chat,
	const char * channel,
	CHATChannelMode * mode,
	PEER peer
)
{
	RoomType roomType;

	//PEER_CONNECTION;

	// Figure out the room type.
	////////////////////////////
	if(!piRoomToType(peer, channel, &roomType))
		return;

	// Add the callback.
	////////////////////
	piAddRoomModeChangedCallback(peer, roomType, mode);
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piChannelModeChangedW
(
	CHAT chat,
	const unsigned short * channel,
	CHATChannelMode * mode,
	PEER peer
)
{
	char* channel_A = UCS2ToUTF8StringAlloc(channel);
	piChannelModeChangedA(chat, channel_A, mode, peer);
	gsifree(channel_A);
}
#endif

void piSetChannelCallbacks
(
	PEER peer,
	chatChannelCallbacks * channelCallbacks
)
{
	memset(channelCallbacks, 0, sizeof(chatChannelCallbacks));
	channelCallbacks->param = peer;
#ifndef GSI_UNICODE // NOT GSI_PEER_UNICODE
	channelCallbacks->channelMessage = piChannelMessageA;
	channelCallbacks->kicked = piChannelKickedA;
	channelCallbacks->userJoined = piChannelUserJoinedA;
	channelCallbacks->userParted = piChannelUserPartedA;
	channelCallbacks->userChangedNick = piChannelUserChangedNickA;
	channelCallbacks->topicChanged = piChannelTopicChangedA;
	channelCallbacks->newUserList = piChannelNewUserListA;
	channelCallbacks->broadcastKeyChanged = piBroadcastKeyChangedA;
	channelCallbacks->userModeChanged = piUserModeChangedA;
	channelCallbacks->channelModeChanged = piChannelModeChangedA;
#else
	channelCallbacks->channelMessage = piChannelMessageW;
	channelCallbacks->kicked = piChannelKickedW;
	channelCallbacks->userJoined = piChannelUserJoinedW;
	channelCallbacks->userParted = piChannelUserPartedW;
	channelCallbacks->userChangedNick = piChannelUserChangedNickW;
	channelCallbacks->topicChanged = piChannelTopicChangedW;
	channelCallbacks->newUserList = piChannelNewUserListW;
	channelCallbacks->broadcastKeyChanged = piBroadcastKeyChangedW;
	channelCallbacks->userModeChanged = piUserModeChangedW;
	channelCallbacks->channelModeChanged = piChannelModeChangedW;
#endif
}

