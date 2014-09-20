/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEERCALLBACKS_H_
#define _PEERCALLBACKS_H_

/*************
** INCLUDES **
*************/
#include "peerMain.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********
** TYPES **
**********/
typedef enum piCallbackType
{
	PI_CONNECT_CALLBACK,
	PI_JOIN_ROOM_CALLBACK,
	PI_LIST_GROUP_ROOMS_CALLBACK,
	PI_LISTING_GAMES_CALLBACK,
	PI_NICK_ERROR_CALLBACK,
	PI_ENUM_PLAYERS_CALLBACK,
	PI_GET_PLAYER_INFO_CALLBACK,
	PI_GET_PLAYER_PROFILE_ID_CALLBACK,
	PI_GET_PLAYER_IP_CALLBACK,
	PI_ROOM_MESSAGE_CALLBACK,
	PI_ROOM_UTM_CALLBACK,
	PI_ROOM_NAME_CHANGED_CALLBACK,
	PI_ROOM_MODE_CHANGED_CALLBACK,
	PI_PLAYER_MESSAGE_CALLBACK,
	PI_PLAYER_UTM_CALLBACK,
	PI_READY_CHANGED_CALLBACK,
	PI_GAME_STARTED_CALLBACK,
	PI_PLAYER_JOINED_CALLBACK,
	PI_PLAYER_LEFT_CALLBACK,
	PI_KICKED_CALLBACK,
	PI_NEW_PLAYER_LIST_CALLBACK,
	PI_PLAYER_CHANGED_NICK_CALLBACK,
	PI_PLAYER_INFO_CALLBACK,
	PI_DISCONNECTED_CALLBACK,
	PI_PING_CALLBACK,
	PI_CROSS_PING_CALLBACK,
	PI_CHANGE_NICK_CALLBACK,
	PI_GLOBAL_KEY_CHANGED_CALLBACK,
	PI_ROOM_KEY_CHANGED_CALLBACK,
	PI_GET_GLOBAL_KEYS_CALLBACK,
	PI_GET_ROOM_KEYS_CALLBACK,
	PI_PLAYER_FLAGS_CHANGED_CALLBACK,
	PI_AUTHENTICATE_CDKEY_CALLBACK,
	PI_AUTO_MATCH_STATUS_CALLBACK,
	PI_NUM_CALLBACK_TYPES
} piCallbackType;

/**************
** FUNCTIONS **
**************/
PEERBool piCallbacksInit(PEER peer);
void piCallbacksCleanup(PEER peer);
void piCallbacksThink(PEER peer, int blockingID);
PEERBool piIsCallbackFinished(PEER peer, int opID);
void piClearCallbacks(PEER peer, piCallbackType type);
void piClearListingGameServerCallbacks(PEER peer, SBServer server);

/**************
** CALLBACKS **
**************/

void piAddConnectCallback
(
	PEER peer,
	PEERBool success,
	int failureReason,
	peerConnectCallback callback,
	void * param,
	int opID
);

void piAddJoinRoomCallback
(
	PEER peer,
	PEERBool success,
	PEERJoinResult result,
	RoomType roomType,
	peerJoinRoomCallback callback,
	void * param,
	int opID
);

void piAddListGroupRoomsCallback
(
	PEER peer,
	PEERBool success,
	int groupID,
	SBServer server,
	const char * name,
	int numWaiting,
	int maxWaiting,
	int numGames,
	int numPlaying,
	peerListGroupRoomsCallback callback,
	void * param,
	int opID
);

void piAddListingGamesCallback
(
	PEER peer,
	PEERBool success,
	SBServer server,
	int msg
);

void piAddEnumPlayersCallback
(
	PEER peer,
	PEERBool success,
	RoomType roomType,
	int index,
	const char * nick,
	int flags,
	peerEnumPlayersCallback callback,
	void * param,
	int opID
);

void piAddGetPlayerInfoCallback
(
	PEER peer,
	PEERBool success,
	const char * nick,
	unsigned int IP,
	int profileID,
	peerGetPlayerInfoCallback callback,
	void * param,
	int opID
);

void piAddGetPlayerProfileIDCallback
(
	PEER peer,
	PEERBool success,
	const char * nick,
	int profileID,
	peerGetPlayerProfileIDCallback callback,
	void * param,
	int opID
);

void piAddGetPlayerIPCallback
(
	PEER peer,
	PEERBool success,
	const char * nick,
	unsigned int IP,
	peerGetPlayerIPCallback callback,
	void * param,
	int opID
);

void piAddRoomMessageCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * message,
	MessageType messageType
);

void piAddRoomUTMCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * command,
	const char * parameters,
	PEERBool authenticated
);

void piAddRoomNameChangedCallback
(
	PEER peer,
	RoomType roomType
);

void piAddRoomModeChangedCallback
(
	PEER peer,
	RoomType roomType,
	CHATChannelMode * mode
);

void piAddPlayerMessageCallback
(
	PEER peer,
	const char * nick,
	const char * message,
	MessageType messageType
);

void piAddPlayerUTMCallback
(
	PEER peer,
	const char * nick,
	const char * command,
	const char * parameters,
	PEERBool authenticated
);

void piAddReadyChangedCallback
(
	PEER peer,
	const char * nick,
	PEERBool ready
);

void piAddGameStartedCallback
(
	PEER peer,
	SBServer server,
	const char * message
);

void piAddPlayerJoinedCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick
);

void piAddPlayerLeftCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * reason
);

void piAddKickedCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * reason
);

void piAddNewPlayerListCallback
(
	PEER peer,
	RoomType roomType
);

void piAddPlayerChangedNickCallback
(
	PEER peer,
	RoomType roomType,
	const char * oldNick,
	const char * newNick
);

void piAddPlayerInfoCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	unsigned int IP,
	int profileID
);

void piAddDisconnectedCallback
(
	PEER peer,
	const char * reason
);

void piAddPingCallback
(
	PEER peer,
	const char * nick,
	int ping
);

void piAddCrossPingCallback
(
	PEER peer,
	const char * nick1,
	const char * nick2,
	int crossPing
);

void piAddNickErrorCallback
(
	PEER peer,
	int type,
	const char * nick,
	int numSuggestedNicks,
	const char ** suggestedNicks,
	void * param,
	int opID
);

void piAddChangeNickCallback
(
	PEER peer,
	PEERBool success,
	const char * oldNick,
	const char * newNick,
	peerChangeNickCallback callback,
	void * param,
	int opID
);

void piAddGlobalKeyChangedCallback
(
	PEER peer,
	const char * nick,
	const char * key,
	const char * value
);

void piAddRoomKeyChangedCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * key,
	const char * value
);

void piAddGetGlobalKeysCallback
(
	PEER peer,
	PEERBool success,
	const char * nick,
	int num,
	const char ** keys,
	const char ** values,
	peerGetGlobalKeysCallback callback,
	void * param,
	int opID
);

void piAddGetRoomKeysCallback
(
	PEER peer,
	PEERBool success,
	RoomType roomType,
	const char * nick,
	int num,
	const char ** keys,
	const char ** values,
	peerGetRoomKeysCallback callback,
	void * param,
	int opID
);

void piAddPlayerFlagsChangedCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	int oldFlags,
	int newFlags
);

void piAddAuthenticateCDKeyCallback
(
	PEER peer,
	int result,
	const char * message,
	peerAuthenticateCDKeyCallback callback,
	void * param,
	int opID
);

void piAddAutoMatchStatusCallback
(
	PEER peer
);

int piCallAutoMatchRateCallback
(
	PEER peer,
	SBServer server
);

#ifdef __cplusplus
}
#endif

#endif
