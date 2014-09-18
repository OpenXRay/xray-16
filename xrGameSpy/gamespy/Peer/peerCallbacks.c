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
#include "peer.h"
#include "peerMain.h"
#include "peerCallbacks.h"
#include "peerOperations.h"

/************
** DEFINES **
************/
#define ASSERT_DATA(data)      assert(data->type >= 0);\
                               assert(data->callback);\
							   assert(data->params);

/**********
** TYPES **
**********/
typedef struct piCallbackData
{
	piCallbackType type;  // PI_<type>_CALLBACK
	PEERBool success;  // operation success
	PEERCBType callback;  // the function callback -- int type for ANSI compatability
	void * callbackParam;  // user-data for the callback
	void * params;  // extra callback params
	int ID;  // unique ID for this callback
	PEERBool inCall;  // set to true immediately before callback is called, then to false
} piCallbackData;

typedef struct piCallbackFuncs
{
	piCallbackType type;
	PEERBool (* copy)
	(
		void * paramsOut,
		void * paramsIn
	);
	void (* gsifree)
	(
		void * params
	);
	void (* call)
	(
		PEER peer,
		piCallbackData * data
	);
} piCallbackFuncs;

/**************
** CALLBACKS **
**************/
static int piAddCallback
(
	PEER peer,
	PEERBool success,
	PEERCBType callback,
	void * param,
	piCallbackType type,
	void * paramsIn,
	size_t paramsSize,
	int opID
);

/* Connect.
**********/
typedef struct piConnectParams
{
	int failureReason;
} piConnectParams;
static PEERBool piConnectCopy(void * paramsOut_, void * paramsIn_)
{
	piConnectParams * paramsOut = (piConnectParams *)paramsOut_;
	piConnectParams * paramsIn = (piConnectParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->failureReason = paramsIn->failureReason;
	
	return PEERTrue;
}
static void piConnectFree(void * params_)
{
	piConnectParams * params = (piConnectParams *)params_;

	assert(params);
	GSI_UNUSED(params);
}
static void piConnectCall(PEER peer, piCallbackData * data)
{
	piConnectParams * params;

	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_CONNECT_CALLBACK);
	
	params = data->params;
	((peerConnectCallback)data->callback)(peer, data->success, params->failureReason, data->callbackParam);
}
void piAddConnectCallback
(
	PEER peer,
	PEERBool success,
	int failureReason,
	peerConnectCallback callback,
	void * param,
	int opID
)
{
	piConnectParams params;

	params.failureReason = failureReason;

	piAddCallback(peer, success, (PEERCBType)callback, param, PI_CONNECT_CALLBACK, &params, sizeof(piConnectParams), opID);
}

/* JoinRoom.
***********/
typedef struct piJoinRoomParams
{
	PEERJoinResult result;
	RoomType roomType;
} piJoinRoomParams;
static PEERBool piJoinRoomCopy(void * paramsOut_, void * paramsIn_)
{
	piJoinRoomParams * paramsOut = (piJoinRoomParams *)paramsOut_;
	piJoinRoomParams * paramsIn = (piJoinRoomParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->result = paramsIn->result;
	paramsOut->roomType = paramsIn->roomType;
	
	return PEERTrue;
}
static void piJoinRoomFree(void * params_)
{
	piJoinRoomParams * params = (piJoinRoomParams *)params_;

	assert(params);
	GSI_UNUSED(params);
}
static void piJoinRoomCall(PEER peer, piCallbackData * data)
{
	piJoinRoomParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_JOIN_ROOM_CALLBACK);
	
	params = data->params;
	((peerJoinRoomCallback)(PEERCBType)data->callback)(peer, data->success, params->result, params->roomType, data->callbackParam);
}
void piAddJoinRoomCallback
(
	PEER peer,
	PEERBool success,
	PEERJoinResult result,
	RoomType roomType,
	peerJoinRoomCallback callback,
	void * param,
	int opID
)
{
	piJoinRoomParams params;

	// if this was called as part of an AutoMatch attempt, call the callback directly
	// this is safe because we're only calling into Peer code
	if(peerIsAutoMatching(peer) && (roomType == StagingRoom))
	{
		callback(peer, success, result, roomType, param);
		return;
	}

	params.result = result;
	params.roomType = roomType;

	piAddCallback(peer, success, (PEERCBType)callback, param, PI_JOIN_ROOM_CALLBACK, &params, sizeof(piJoinRoomParams), opID);
}

/* ListGroupsRooms.
******************/
typedef struct piListGroupRoomsParams
{
	int groupID;
	SBServer server;
	char * name;
	int numWaiting;
	int maxWaiting;
	int numGames;
	int numPlaying;
} piListGroupRoomsParams;
static PEERBool piListGroupRoomsCopy(void * paramsOut_, void * paramsIn_)
{
	piListGroupRoomsParams * paramsOut = (piListGroupRoomsParams *)paramsOut_;
	piListGroupRoomsParams * paramsIn = (piListGroupRoomsParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->groupID = paramsIn->groupID;
	paramsOut->server = paramsIn->server;
	paramsOut->numWaiting = paramsIn->numWaiting;
	paramsOut->maxWaiting = paramsIn->maxWaiting;
	paramsOut->numGames = paramsIn->numGames;
	paramsOut->numPlaying = paramsIn->numPlaying;
	if(paramsIn->name)
	{
		paramsOut->name = goastrdup(paramsIn->name);
		if(paramsIn->name && !paramsOut->name)
			return PEERFalse;
	}
	else
		paramsOut->name = NULL;

	return PEERTrue;
}
static void piListGroupRoomsFree(void * params_)
{
	piListGroupRoomsParams * params = (piListGroupRoomsParams *)params_;

	assert(params);

	gsifree(params->name);
}
static void piListGroupRoomsCall(PEER peer, piCallbackData * data)
{
	piListGroupRoomsParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_LIST_GROUP_ROOMS_CALLBACK);
	
	params = data->params;
	
#ifndef GSI_UNICODE
	((peerListGroupRoomsCallback)(PEERCBType)data->callback)(peer, data->success, params->groupID, params->server, params->name, params->numWaiting, params->maxWaiting, params->numGames, params->numPlaying, data->callbackParam);
#else
	{
		unsigned short* name_W = NULL;
		if (params->name != NULL)
			name_W = UTF8ToUCS2StringAlloc(params->name);
		((peerListGroupRoomsCallback)(int)data->callback)(peer, data->success, params->groupID, params->server, name_W, params->numWaiting, params->maxWaiting, params->numGames, params->numPlaying, data->callbackParam);
		gsifree(name_W);
	}
#endif
}
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
)
{
	piListGroupRoomsParams params;
	params.groupID = groupID;
	params.server = server;
	params.name = (char *)name;
	params.numWaiting = numWaiting;
	params.maxWaiting = maxWaiting;
	params.numGames = numGames;
	params.numPlaying = numPlaying;

	piAddCallback(peer, success, (PEERCBType)callback, param, PI_LIST_GROUP_ROOMS_CALLBACK, &params, sizeof(piListGroupRoomsParams), opID);
}

/* ListingGames.
***************/
typedef struct piListingGamesParams
{
	char * name;
	SBServer server;
	PEERBool staging;
	int msg;
	int progress;
} piListingGamesParams;
static PEERBool piListingGamesCopy(void * paramsOut_, void * paramsIn_)
{
	piListingGamesParams * paramsOut = (piListingGamesParams *)paramsOut_;
	piListingGamesParams * paramsIn = (piListingGamesParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	if(paramsIn->name)
	{
		paramsOut->name = goastrdup(paramsIn->name);
		if(paramsIn->name && !paramsOut->name)
			return PEERFalse;
	}
	else
		paramsOut->name = NULL;
	paramsOut->staging = paramsIn->staging;
	paramsOut->server = paramsIn->server;
	paramsOut->msg = paramsIn->msg;
	paramsOut->progress = paramsIn->progress;
	
	return PEERTrue;
}
static void piListingGamesFree(void * params_)
{
	piListingGamesParams * params = (piListingGamesParams *)params_;

	assert(params);

	gsifree(params->name);
}
static void piListingGamesCall(PEER peer, piCallbackData * data)
{
	piListingGamesParams * params;
	int len;
	int i;

	PEER_CONNECTION;
		
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_LISTING_GAMES_CALLBACK);

	params = data->params;

	// This is a bit of a hack.  We don't want the server browser progess to show
	//	100% until the last callback is being called.
	if (params->progress == 100)
	{
		len = ArrayLength(connection->callbackList);
		for(i = 0; i < len; i++)
		{
			// look for a listing games callback
			piCallbackData* anotherData = (piCallbackData *)ArrayNth(connection->callbackList, i);
			if(anotherData != data && anotherData->type == PI_LISTING_GAMES_CALLBACK)
			{
				piListingGamesParams* anotherParams = (piListingGamesParams *)anotherData->params;
				if(anotherParams->msg == PEER_UPDATE)
				{
					// Another update callback will follow this one,
					// limit the current callback's progress to 99%
					params->progress = 99;
					break;
				}
			}
		}
	}

#ifndef GSI_UNICODE
	((peerListingGamesCallback)(PEERCBType)data->callback)(peer, data->success, params->name, params->server, params->staging, params->msg, params->progress, data->callbackParam);
#else
	{
		unsigned short* name_W = NULL; 
		if (params->name != NULL)
			name_W = UTF8ToUCS2StringAlloc(params->name);
		((peerListingGamesCallback)(int)data->callback)(peer, data->success, name_W, params->server, params->staging, params->msg, params->progress, data->callbackParam);
		gsifree(name_W);
	}
#endif
}

void piAddListingGamesCallback
(
	PEER peer,
	PEERBool success,
	SBServer server,
	int msg
)
{
	piListingGamesParams params;
	const char * name;
	const char * gamemode;
	PEERBool staging;
	int count;
	int pendingCount;
	int progress;
	PEER_CONNECTION;

	// if this is a remove, remove any queued adds or updates
	if(msg == PEER_REMOVE)
		piClearListingGameServerCallbacks(peer, server);

	// get info from the server object
	if(server)
	{
		name = SBServerGetStringValueA(server, "hostname", "(No Name)");
		gamemode = SBServerGetStringValueA(server, "gamemode", "");
		staging = (PEERBool)(strcasecmp(gamemode, "openstaging") == 0);
	}
	else
	{
		name = NULL;
		staging = PEERFalse;
	}

	// set the progress
	if(connection->initialGameList)
	{
		count = SBServerListCount(&connection->gameList);
		if(count)
		{
			pendingCount = (connection->gameEngine.querylist.count + connection->gameEngine.pendinglist.count);
			progress = (((count - pendingCount) * 100) / count);
		}
		else
		{
			progress = 0;
		}
	}
	else
	{
		progress = 100;
	}

	params.name = (char *)name;
	params.server = server;
	params.staging = staging;
	params.msg = msg;
	params.progress = progress;
	piAddCallback(peer, success, (PEERCBType)connection->gameListCallback, connection->gameListParam, PI_LISTING_GAMES_CALLBACK, &params, sizeof(piListingGamesParams), -1);
}

/* NickError.
*************/
typedef struct piNickErrorParams
{
	int type;
	char * nick;
	int numSuggestedNicks;
	char ** suggestedNicks;
} piNickErrorParams;
static PEERBool piNickErrorCopy(void * paramsOut_, void * paramsIn_)
{
	int i;
	int num;
	PEERBool success = PEERTrue;
	piNickErrorParams * paramsOut = (piNickErrorParams *)paramsOut_;
	piNickErrorParams * paramsIn = (piNickErrorParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	memset(paramsOut, 0, sizeof(piNickErrorParams));
	num = paramsOut->numSuggestedNicks = paramsIn->numSuggestedNicks;

	paramsOut->type = paramsIn->type;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			success = PEERFalse;
	}
	else
		paramsOut->nick = NULL;

	if(success && num && paramsIn->suggestedNicks)
	{
		paramsOut->suggestedNicks = (char **)gsimalloc(sizeof(char *) * num);
		if(!paramsOut->suggestedNicks)
			success = PEERFalse;
		else
			memset(paramsOut->suggestedNicks, 0, sizeof(char *) * num);

		for(i = 0 ; success && (i < num) ; i++)
		{
			paramsOut->suggestedNicks[i] = goastrdup(paramsIn->suggestedNicks[i]);
			if(!paramsOut->suggestedNicks[i])
				success = PEERFalse;
		}
	}

	if(!success)
	{
		gsifree(paramsOut->nick);
		for(i = 0 ; i < num ; i++)
		{
			if(paramsOut->suggestedNicks)
				gsifree(paramsOut->suggestedNicks[i]);
		}
		gsifree(paramsOut->suggestedNicks);
	}

	return success;
}
static void piNickErrorFree(void * params_)
{
	int i;
	piNickErrorParams * params = (piNickErrorParams *)params_;

	assert(params);

	gsifree(params->nick);
	for(i = 0 ; i < params->numSuggestedNicks ; i++)
	{
		if(params->suggestedNicks)
			gsifree(params->suggestedNicks[i]);
	}
	gsifree(params->suggestedNicks);
}
static void piNickErrorCall(PEER peer, piCallbackData * data)
{
	piNickErrorParams * params;

	//PEER_CONNECTION;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_NICK_ERROR_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerNickErrorCallback)(PEERCBType)data->callback)(peer, params->type, params->nick, params->numSuggestedNicks, (const char **)params->suggestedNicks, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short** suggestedNicks_W = UTF8ToUCS2StringArrayAlloc((const UTF8String *)params->suggestedNicks, params->numSuggestedNicks);
		int i;
		((peerNickErrorCallback)data->callback)(peer, params->type, nick_W, params->numSuggestedNicks, (const unsigned short**)suggestedNicks_W, data->callbackParam);
		gsifree(nick_W);
		for (i=0; i < params->numSuggestedNicks; i++)
		{
			gsifree(suggestedNicks_W[i]);
		}
		gsifree(suggestedNicks_W);
	}
#endif
}

void piAddNickErrorCallback
(
	PEER peer,
	int type,
	const char * nick,
	int numSuggestedNicks,
	const char ** suggestedNicks,
	void * param,
	int opID
)
{
	piNickErrorParams params;

	PEER_CONNECTION;

	params.type = type;
	params.nick = (char *)nick;
	params.numSuggestedNicks = numSuggestedNicks;
	params.suggestedNicks = (char **)suggestedNicks;
	piAddCallback(peer, PEERFalse, (PEERCBType)connection->nickErrorCallback, param, PI_NICK_ERROR_CALLBACK, &params, sizeof(piNickErrorParams), opID);
}

/* EnumPlayers.
**************/
typedef struct piEnumPlayersParams
{
	RoomType roomType;
	int index;
	char * nick;
	int flags;
} piEnumPlayersParams;
static PEERBool piEnumPlayersCopy(void * paramsOut_, void * paramsIn_)
{
	piEnumPlayersParams * paramsOut = (piEnumPlayersParams *)paramsOut_;
	piEnumPlayersParams * paramsIn = (piEnumPlayersParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	paramsOut->index = paramsIn->index;
	paramsOut->flags = paramsIn->flags;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(paramsIn->nick && !paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	
	return PEERTrue;
}
static void piEnumPlayersFree(void * params_)
{
	piEnumPlayersParams * params = (piEnumPlayersParams *)params_;

	assert(params);

	gsifree(params->nick);
}
static void piEnumPlayersCall(PEER peer, piCallbackData * data)
{
	piEnumPlayersParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_ENUM_PLAYERS_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerEnumPlayersCallback)data->callback)(peer, data->success, params->roomType, params->index, params->nick, params->flags, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		((peerEnumPlayersCallback)data->callback)(peer, data->success, params->roomType, params->index, nick_W, params->flags, data->callbackParam);
		gsifree(nick_W);
	}
#endif
}
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
)
{
	piEnumPlayersParams params;
	params.roomType = roomType;
	params.index = index;
	params.nick = (char *)nick;
	params.flags = flags;

	piAddCallback(peer, success, (PEERCBType)callback, param, PI_ENUM_PLAYERS_CALLBACK, &params, sizeof(piEnumPlayersParams), opID);
}

/* GetPlayerInfo.
****************/
typedef struct piGetPlayerInfoParams
{
	char * nick;
	unsigned int IP;
	int profileID;
} piGetPlayerInfoParams;
static PEERBool piGetPlayerInfoCopy(void * paramsOut_, void * paramsIn_)
{
	piGetPlayerInfoParams * paramsOut = (piGetPlayerInfoParams *)paramsOut_;
	piGetPlayerInfoParams * paramsIn = (piGetPlayerInfoParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	paramsOut->IP = paramsIn->IP;
	paramsOut->profileID = paramsIn->profileID;
	
	return PEERTrue;
}
static void piGetPlayerInfoFree(void * params_)
{
	piGetPlayerInfoParams * params = (piGetPlayerInfoParams *)params_;

	assert(params);

	gsifree(params->nick);
}
static void piGetPlayerInfoCall(PEER peer, piCallbackData * data)
{
	piGetPlayerInfoParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_GET_PLAYER_INFO_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerGetPlayerInfoCallback)data->callback)(peer, data->success, params->nick, params->IP, params->profileID, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		((peerGetPlayerInfoCallback)data->callback)(peer, data->success, nick_W, params->IP, params->profileID, data->callbackParam);
		gsifree(nick_W);
	}
#endif
}
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
)
{
	piGetPlayerInfoParams params;
	params.nick = (char *)nick;
	params.IP = IP;
	params.profileID = profileID;

	piAddCallback(peer, success, (PEERCBType)callback, param, PI_GET_PLAYER_INFO_CALLBACK, &params, sizeof(piGetPlayerInfoParams), opID);
}

/* GetPlayerProfileID.
*********************/
typedef struct piGetPlayerProfileIDParams
{
	char * nick;
	int profileID;
} piGetPlayerProfileIDParams;
static PEERBool piGetPlayerProfileIDCopy(void * paramsOut_, void * paramsIn_)
{
	piGetPlayerProfileIDParams * paramsOut = (piGetPlayerProfileIDParams *)paramsOut_;
	piGetPlayerProfileIDParams * paramsIn = (piGetPlayerProfileIDParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	paramsOut->profileID = paramsIn->profileID;
	
	return PEERTrue;
}
static void piGetPlayerProfileIDFree(void * params_)
{
	piGetPlayerProfileIDParams * params = (piGetPlayerProfileIDParams *)params_;

	assert(params);

	gsifree(params->nick);
}
static void piGetPlayerProfileIDCall(PEER peer, piCallbackData * data)
{
	piGetPlayerProfileIDParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_GET_PLAYER_PROFILE_ID_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerGetPlayerProfileIDCallback)data->callback)(peer, data->success, params->nick, params->profileID, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		((peerGetPlayerProfileIDCallback)data->callback)(peer, data->success, nick_W, params->profileID, data->callbackParam);
		gsifree(nick_W);
	}
#endif
}
void piAddGetPlayerProfileIDCallback
(
	PEER peer,
	PEERBool success,
	const char * nick,
	int profileID,
	peerGetPlayerProfileIDCallback callback,
	void * param,
	int opID
)
{
	piGetPlayerProfileIDParams params;
	params.nick = (char *)nick;
	params.profileID = profileID;

	piAddCallback(peer, success, (PEERCBType)callback, param, PI_GET_PLAYER_PROFILE_ID_CALLBACK, &params, sizeof(piGetPlayerProfileIDParams), opID);
}

/* GetPlayerIP.
**************/
typedef struct piGetPlayerIPParams
{
	char * nick;
	unsigned int IP;
} piGetPlayerIPParams;
static PEERBool piGetPlayerIPCopy(void * paramsOut_, void * paramsIn_)
{
	piGetPlayerIPParams * paramsOut = (piGetPlayerIPParams *)paramsOut_;
	piGetPlayerIPParams * paramsIn = (piGetPlayerIPParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->IP = paramsIn->IP;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	
	return PEERTrue;
}
static void piGetPlayerIPFree(void * params_)
{
	piGetPlayerIPParams * params = (piGetPlayerIPParams *)params_;

	assert(params);

	gsifree(params->nick);
}
static void piGetPlayerIPCall(PEER peer, piCallbackData * data)
{
	piGetPlayerIPParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_GET_PLAYER_IP_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerGetPlayerIPCallback)data->callback)(peer, data->success, params->nick, params->IP, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		((peerGetPlayerIPCallback)data->callback)(peer, data->success, nick_W, params->IP, data->callbackParam);
		gsifree(nick_W);
	}
#endif
}
void piAddGetPlayerIPCallback
(
	PEER peer,
	PEERBool success,
	const char * nick,
	unsigned int IP,
	peerGetPlayerIPCallback callback,
	void * param,
	int opID
)
{
	piGetPlayerIPParams params;
	params.nick = (char *)nick;
	params.IP = IP;

	piAddCallback(peer, success, (PEERCBType)callback, param, PI_GET_PLAYER_IP_CALLBACK, &params, sizeof(piGetPlayerIPParams), opID);
}

/* Room Message.
***************/
typedef struct piRoomMessageParams
{
	RoomType roomType;
	char * nick;
	char * message;
	MessageType messageType;
} piRoomMessageParams;
static PEERBool piRoomMessageCopy(void * paramsOut_, void * paramsIn_)
{
	piRoomMessageParams * paramsOut = (piRoomMessageParams *)paramsOut_;
	piRoomMessageParams * paramsIn = (piRoomMessageParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	paramsOut->messageType = paramsIn->messageType;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	if(paramsIn->message)
	{
		paramsOut->message = goastrdup(paramsIn->message);
		if(!paramsOut->message)
		{
			gsifree(paramsOut->nick);
			return PEERFalse;
		}
	}
	else
		paramsOut->message = NULL;
	
	return PEERTrue;
}
static void piRoomMessageFree(void * params_)
{
	piRoomMessageParams * params = (piRoomMessageParams *)params_;

	assert(params);

	gsifree(params->nick);
	gsifree(params->message);
}
static void piRoomMessageCall(PEER peer, piCallbackData * data)
{
	piRoomMessageParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_ROOM_MESSAGE_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerRoomMessageCallback)data->callback)(peer, params->roomType, params->nick, params->message, params->messageType, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short* message_W = UTF8ToUCS2StringAlloc(params->message);

		((peerRoomMessageCallback)data->callback)(peer, params->roomType, nick_W, message_W, params->messageType, data->callbackParam);
		gsifree(nick_W);
		gsifree(message_W);
	}
#endif
}
void piAddRoomMessageCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * message,
	MessageType messageType
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->roomMessage)
	{
		piRoomMessageParams params;
		params.roomType = roomType;
		params.nick = (char *)nick;
		params.message = (char *)message;
		params.messageType = messageType;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->roomMessage, callbacks->param, PI_ROOM_MESSAGE_CALLBACK, &params, sizeof(piRoomMessageParams), -1);
	}
}

/* Room UTM.
***********/
typedef struct piRoomUTMParams
{
	RoomType roomType;
	char * nick;
	char * command;
	char * parameters;
	PEERBool authenticated;
} piRoomUTMParams;
static PEERBool piRoomUTMCopy(void * paramsOut_, void * paramsIn_)
{
	piRoomUTMParams * paramsOut = (piRoomUTMParams *)paramsOut_;
	piRoomUTMParams * paramsIn = (piRoomUTMParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->authenticated = paramsIn->authenticated;
	paramsOut->roomType = paramsIn->roomType;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	if(paramsIn->command)
	{
		paramsOut->command = goastrdup(paramsIn->command);
		if(!paramsOut->command)
		{
			gsifree(paramsOut->nick);
			return PEERFalse;
		}
	}
	else
		paramsOut->command = NULL;
	if(paramsIn->parameters)
	{
		paramsOut->parameters = goastrdup(paramsIn->parameters);
		if(!paramsOut->parameters)
		{
			gsifree(paramsOut->nick);
			gsifree(paramsOut->command);
			return PEERFalse;
		}
	}
	else
		paramsOut->parameters = NULL;

	return PEERTrue;
}
static void piRoomUTMFree(void * params_)
{
	piRoomUTMParams * params = (piRoomUTMParams *)params_;

	assert(params);

	gsifree(params->nick);
	gsifree(params->command);
	gsifree(params->parameters);
}
static void piRoomUTMCall(PEER peer, piCallbackData * data)
{
	piRoomUTMParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_ROOM_UTM_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerRoomUTMCallback)data->callback)(peer, params->roomType, params->nick, params->command, params->parameters, params->authenticated, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short* command_W = UTF8ToUCS2StringAlloc(params->command);
		unsigned short* parameters_W = UTF8ToUCS2StringAlloc(params->parameters);
		((peerRoomUTMCallback)data->callback)(peer, params->roomType, nick_W, command_W, parameters_W, params->authenticated, data->callbackParam);
		gsifree(nick_W);
		gsifree(command_W);
		gsifree(parameters_W);
	}
#endif
}
void piAddRoomUTMCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * command,
	const char * parameters,
	PEERBool authenticated
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->roomUTM)
	{
		piRoomUTMParams params;
		params.roomType = roomType;
		params.nick = (char *)nick;
		params.command = (char *)command;
		params.parameters = (char *)parameters;
		params.authenticated = authenticated;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->roomUTM, callbacks->param, PI_ROOM_UTM_CALLBACK, &params, sizeof(piRoomUTMParams), -1);
	}
}

/* Room Name Changed.
********************/
typedef struct piRoomNameChangedParams
{
	RoomType roomType;
} piRoomNameChangedParams;
static PEERBool piRoomNameChangedCopy(void * paramsOut_, void * paramsIn_)
{
	piRoomNameChangedParams * paramsOut = (piRoomNameChangedParams *)paramsOut_;
	piRoomNameChangedParams * paramsIn = (piRoomNameChangedParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	
	return PEERTrue;
}
static void piRoomNameChangedFree(void * params_)
{
	piRoomNameChangedParams * params = (piRoomNameChangedParams *)params_;

	assert(params);
	GSI_UNUSED(params);
}
static void piRoomNameChangedCall(PEER peer, piCallbackData * data)
{
	piRoomNameChangedParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_ROOM_NAME_CHANGED_CALLBACK);
	
	params = data->params;
	((peerRoomNameChangedCallback)data->callback)(peer, params->roomType, data->callbackParam);
}
void piAddRoomNameChangedCallback
(
	PEER peer,
	RoomType roomType
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->roomNameChanged)
	{
		piRoomNameChangedParams params;
		params.roomType = roomType;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->roomNameChanged, callbacks->param, PI_ROOM_NAME_CHANGED_CALLBACK, &params, sizeof(piRoomNameChangedParams), -1);
	}
}

/* Room Mode Changed.
********************/
typedef struct piRoomModeChangedParams
{
	RoomType roomType;
	CHATChannelMode mode;
} piRoomModeChangedParams;
static PEERBool piRoomModeChangedCopy(void * paramsOut_, void * paramsIn_)
{
	piRoomModeChangedParams * paramsOut = (piRoomModeChangedParams *)paramsOut_;
	piRoomModeChangedParams * paramsIn = (piRoomModeChangedParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	paramsOut->mode = paramsIn->mode;
	
	return PEERTrue;
}
static void piRoomModeChangedFree(void * params_)
{
	piRoomModeChangedParams * params = (piRoomModeChangedParams *)params_;

	assert(params);
	GSI_UNUSED(params);
}
static void piRoomModeChangedCall(PEER peer, piCallbackData * data)
{
	piRoomModeChangedParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_ROOM_MODE_CHANGED_CALLBACK);
	
	params = data->params;
	((peerRoomModeChangedCallback)data->callback)(peer, params->roomType, &params->mode, data->callbackParam);
}
void piAddRoomModeChangedCallback
(
	PEER peer,
	RoomType roomType,
	CHATChannelMode * mode
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->roomModeChanged)
	{
		piRoomModeChangedParams params;
		params.roomType = roomType;
		params.mode = *mode;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->roomModeChanged, callbacks->param, PI_ROOM_MODE_CHANGED_CALLBACK, &params, sizeof(piRoomModeChangedParams), -1);
	}
}

/* Player Message.
*****************/
typedef struct piPlayerMessageParams
{
	char * nick;
	char * message;
	MessageType messageType;
} piPlayerMessageParams;
static PEERBool piPlayerMessageCopy(void * paramsOut_, void * paramsIn_)
{
	piPlayerMessageParams * paramsOut = (piPlayerMessageParams *)paramsOut_;
	piPlayerMessageParams * paramsIn = (piPlayerMessageParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->messageType = paramsIn->messageType;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	if(paramsIn->message)
	{
		paramsOut->message = goastrdup(paramsIn->message);
		if(!paramsOut->message)
		{
			gsifree(paramsOut->nick);
			return PEERFalse;
		}
	}
	else
		paramsOut->message = NULL;
	
	return PEERTrue;
}
static void piPlayerMessageFree(void * params_)
{
	piPlayerMessageParams * params = (piPlayerMessageParams *)params_;

	assert(params);

	gsifree(params->nick);
	gsifree(params->message);
}
static void piPlayerMessageCall(PEER peer, piCallbackData * data)
{
	piPlayerMessageParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_PLAYER_MESSAGE_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerPlayerMessageCallback)data->callback)(peer, params->nick, params->message, params->messageType, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short* message_W = UTF8ToUCS2StringAlloc(params->message);
		((peerPlayerMessageCallback)data->callback)(peer, nick_W, message_W, params->messageType, data->callbackParam);
		gsifree(nick_W);
		gsifree(message_W);
	}
#endif
}
void piAddPlayerMessageCallback
(
	PEER peer,
	const char * nick,
	const char * message,
	MessageType messageType
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->playerMessage)
	{
		piPlayerMessageParams params;
		params.nick = (char *)nick;
		params.message = (char *)message;
		params.messageType = messageType;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->playerMessage, callbacks->param, PI_PLAYER_MESSAGE_CALLBACK, &params, sizeof(piPlayerMessageParams), -1);
	}
}

/* Player UTM.
*************/
typedef struct piPlayerUTMParams
{
	char * nick;
	char * command;
	char * parameters;
	PEERBool authenticated;
} piPlayerUTMParams;
static PEERBool piPlayerUTMCopy(void * paramsOut_, void * paramsIn_)
{
	piPlayerUTMParams * paramsOut = (piPlayerUTMParams *)paramsOut_;
	piPlayerUTMParams * paramsIn = (piPlayerUTMParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->authenticated = paramsIn->authenticated;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	if(paramsIn->command)
	{
		paramsOut->command = goastrdup(paramsIn->command);
		if(!paramsOut->command)
		{
			gsifree(paramsOut->nick);
			return PEERFalse;
		}
	}
	else
		paramsOut->command = NULL;
	if(paramsIn->parameters)
	{
		paramsOut->parameters = goastrdup(paramsIn->parameters);
		if(!paramsOut->parameters)
		{
			gsifree(paramsOut->nick);
			gsifree(paramsOut->command);
			return PEERFalse;
		}
	}
	else
		paramsOut->parameters = NULL;

	return PEERTrue;
}
static void piPlayerUTMFree(void * params_)
{
	piPlayerUTMParams * params = (piPlayerUTMParams *)params_;

	assert(params);

	gsifree(params->nick);
	gsifree(params->command);
	gsifree(params->parameters);
}
static void piPlayerUTMCall(PEER peer, piCallbackData * data)
{
	piPlayerUTMParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_PLAYER_UTM_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerPlayerUTMCallback)data->callback)(peer, params->nick, params->command, params->parameters, params->authenticated, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short* command_W = UTF8ToUCS2StringAlloc(params->command);
		unsigned short* parameters_W = UTF8ToUCS2StringAlloc(params->parameters);
		((peerPlayerUTMCallback)data->callback)(peer, nick_W, command_W, parameters_W, params->authenticated, data->callbackParam);
		gsifree(nick_W);
		gsifree(command_W);
		gsifree(parameters_W);
	}
#endif
}
void piAddPlayerUTMCallback
(
	PEER peer,
	const char * nick,
	const char * command,
	const char * parameters,
	PEERBool authenticated
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->playerUTM)
	{
		piPlayerUTMParams params;
		params.nick = (char *)nick;
		params.command = (char *)command;
		params.parameters = (char *)parameters;
		params.authenticated = authenticated;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->playerUTM, callbacks->param, PI_PLAYER_UTM_CALLBACK, &params, sizeof(piPlayerUTMParams), -1);
	}
}

/* Ready Changed.
****************/
typedef struct piReadyChangedParams
{
	char * nick;
	PEERBool ready;
} piReadyChangedParams;
static PEERBool piReadyChangedCopy(void * paramsOut_, void * paramsIn_)
{
	piReadyChangedParams * paramsOut = (piReadyChangedParams *)paramsOut_;
	piReadyChangedParams * paramsIn = (piReadyChangedParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	paramsOut->ready = paramsIn->ready;
	
	return PEERTrue;
}
static void piReadyChangedFree(void * params_)
{
	piReadyChangedParams * params = (piReadyChangedParams *)params_;

	assert(params);

	gsifree(params->nick);
}
static void piReadyChangedCall(PEER peer, piCallbackData * data)
{
	piReadyChangedParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_READY_CHANGED_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerReadyChangedCallback)data->callback)(peer, params->nick, params->ready, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		((peerReadyChangedCallback)data->callback)(peer, nick_W, params->ready, data->callbackParam);
		gsifree(nick_W);
	}
#endif
}
void piAddReadyChangedCallback
(
	PEER peer,
	const char * nick,
	PEERBool ready
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->readyChanged)
	{
		piReadyChangedParams params;
		params.nick = (char *)nick;
		params.ready = ready;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->readyChanged, callbacks->param, PI_READY_CHANGED_CALLBACK, &params, sizeof(piReadyChangedParams), -1);
	}
}

/* GameStarted.
**************/
typedef struct piGameStartedParams
{
	SBServer server;
	char * message;
} piGameStartedParams;
static PEERBool piGameStartedCopy(void * paramsOut_, void * paramsIn_)
{
	piGameStartedParams * paramsOut = (piGameStartedParams *)paramsOut_;
	piGameStartedParams * paramsIn = (piGameStartedParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->server = paramsIn->server;
	if(paramsIn->message)
	{
		paramsOut->message = goastrdup(paramsIn->message);
		if(!paramsOut->message)
			return PEERFalse;
	}
	else
		paramsOut->message = NULL;
	
	return PEERTrue;
}
static void piGameStartedFree(void * params_)
{
	piGameStartedParams * params = (piGameStartedParams *)params_;

	assert(params);

	gsifree(params->message);
}
static void piGameStartedCall(PEER peer, piCallbackData * data)
{
	piGameStartedParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_GAME_STARTED_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerGameStartedCallback)data->callback)(peer, params->server, params->message, data->callbackParam);
#else
	{
		unsigned short* message_W = UTF8ToUCS2StringAlloc(params->message);
		((peerGameStartedCallback)data->callback)(peer, params->server, message_W, data->callbackParam);
		gsifree(message_W);
		}
#endif
}
void piAddGameStartedCallback
(
	PEER peer,
	SBServer server,
	const char * message
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->gameStarted)
	{
		piGameStartedParams params;
		params.server = server;
		params.message = (char *)message;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->gameStarted, callbacks->param, PI_GAME_STARTED_CALLBACK, &params, sizeof(piGameStartedParams), -1);
	}
}

/* Player Joined.
****************/
typedef struct piPlayerJoinedParams
{
	RoomType roomType;
	char * nick;
} piPlayerJoinedParams;
static PEERBool piPlayerJoinedCopy(void * paramsOut_, void * paramsIn_)
{
	piPlayerJoinedParams * paramsOut = (piPlayerJoinedParams *)paramsOut_;
	piPlayerJoinedParams * paramsIn = (piPlayerJoinedParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	
	return PEERTrue;
}
static void piPlayerJoinedFree(void * params_)
{
	piPlayerJoinedParams * params = (piPlayerJoinedParams *)params_;

	assert(params);

	gsifree(params->nick);
}
static void piPlayerJoinedCall(PEER peer, piCallbackData * data)
{
	piPlayerJoinedParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_PLAYER_JOINED_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerPlayerJoinedCallback)data->callback)(peer, params->roomType, params->nick, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		((peerPlayerJoinedCallback)data->callback)(peer, params->roomType, nick_W, data->callbackParam);
		gsifree(nick_W);
	}
#endif
}
void piAddPlayerJoinedCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->playerJoined)
	{
		piPlayerJoinedParams params;
		params.roomType = roomType;
		params.nick = (char *)nick;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->playerJoined, callbacks->param, PI_PLAYER_JOINED_CALLBACK, &params, sizeof(piPlayerJoinedParams), -1);
	}
}

/* Player Left.
**************/
typedef struct piPlayerLeftParams
{
	RoomType roomType;
	char * nick;
	char * reason;
} piPlayerLeftParams;
static PEERBool piPlayerLeftCopy(void * paramsOut_, void * paramsIn_)
{
	piPlayerLeftParams * paramsOut = (piPlayerLeftParams *)paramsOut_;
	piPlayerLeftParams * paramsIn = (piPlayerLeftParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	if(paramsIn->reason)
	{
		paramsOut->reason = goastrdup(paramsIn->reason);
		if(!paramsOut->reason)
		{
			gsifree(paramsOut->nick);
			return PEERFalse;
		}
	}
	else
		paramsOut->reason = NULL;
	
	return PEERTrue;
}
static void piPlayerLeftFree(void * params_)
{
	piPlayerLeftParams * params = (piPlayerLeftParams *)params_;

	assert(params);

	gsifree(params->nick);
	gsifree(params->reason);
}
static void piPlayerLeftCall(PEER peer, piCallbackData * data)
{
	piPlayerLeftParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_PLAYER_LEFT_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerPlayerLeftCallback)data->callback)(peer, params->roomType, params->nick, params->reason, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short* reason_W = UTF8ToUCS2StringAlloc(params->reason);
		((peerPlayerLeftCallback)data->callback)(peer, params->roomType, nick_W, reason_W, data->callbackParam);
		gsifree(nick_W);
		gsifree(reason_W);
	}
#endif
}
void piAddPlayerLeftCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * reason
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->playerLeft)
	{
		piPlayerLeftParams params;
		params.roomType = roomType;
		params.nick = (char *)nick;
		params.reason = (char *)reason;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->playerLeft, callbacks->param, PI_PLAYER_LEFT_CALLBACK, &params, sizeof(piPlayerLeftParams), -1);
	}
}

/* Kicked.
*********/
typedef struct piKickedParams
{
	RoomType roomType;
	char * nick;
	char * reason;
} piKickedParams;
static PEERBool piKickedCopy(void * paramsOut_, void * paramsIn_)
{
	piKickedParams * paramsOut = (piKickedParams *)paramsOut_;
	piKickedParams * paramsIn = (piKickedParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	if(paramsIn->reason)
	{
		paramsOut->reason = goastrdup(paramsIn->reason);
		if(!paramsOut->reason)
		{
			gsifree(paramsOut->nick);
			return PEERFalse;
		}
	}
	else
		paramsOut->reason = NULL;
	
	return PEERTrue;
}
static void piKickedFree(void * params_)
{
	piKickedParams * params = (piKickedParams *)params_;

	assert(params);

	gsifree(params->nick);
	gsifree(params->reason);
}
static void piKickedCall(PEER peer, piCallbackData * data)
{
	piKickedParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_KICKED_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerKickedCallback)data->callback)(peer, params->roomType, params->nick, params->reason, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short* reason_W = UTF8ToUCS2StringAlloc(params->reason);
		((peerKickedCallback)data->callback)(peer, params->roomType, nick_W, reason_W, data->callbackParam);
		gsifree(nick_W);
		gsifree(reason_W);
	}
#endif
}
void piAddKickedCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * reason
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->kicked)
	{
		piKickedParams params;
		params.roomType = roomType;
		params.nick = (char *)nick;
		params.reason = (char *)reason;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->kicked, callbacks->param, PI_KICKED_CALLBACK, &params, sizeof(piKickedParams), -1);
	}
}

/* New Player List.
******************/
typedef struct piNewPlayerListParams
{
	RoomType roomType;
} piNewPlayerListParams;
static PEERBool piNewPlayerListCopy(void * paramsOut_, void * paramsIn_)
{
	piNewPlayerListParams * paramsOut = (piNewPlayerListParams *)paramsOut_;
	piNewPlayerListParams * paramsIn = (piNewPlayerListParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;

	return PEERTrue;
}
static void piNewPlayerListFree(void * params_)
{
	piNewPlayerListParams * params = (piNewPlayerListParams *)params_;

	assert(params);
	GSI_UNUSED(params);
}
static void piNewPlayerListCall(PEER peer, piCallbackData * data)
{
	piNewPlayerListParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_NEW_PLAYER_LIST_CALLBACK);

	params = data->params;
	((peerNewPlayerListCallback)data->callback)(peer, params->roomType, data->callbackParam);
}
void piAddNewPlayerListCallback
(
	PEER peer,
	RoomType roomType
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->newPlayerList)
	{
		piNewPlayerListParams params;
		params.roomType = roomType;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->newPlayerList, callbacks->param, PI_NEW_PLAYER_LIST_CALLBACK, &params, sizeof(piNewPlayerListParams), -1);
	}
}

/* Player Changed Nick.
**********************/
typedef struct piPlayerChangedNickParams
{
	RoomType roomType;
	char * oldNick;
	char * newNick;
} piPlayerChangedNickParams;
static PEERBool piPlayerChangedNickCopy(void * paramsOut_, void * paramsIn_)
{
	piPlayerChangedNickParams * paramsOut = (piPlayerChangedNickParams *)paramsOut_;
	piPlayerChangedNickParams * paramsIn = (piPlayerChangedNickParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	if(paramsIn->oldNick)
	{
		paramsOut->oldNick = goastrdup(paramsIn->oldNick);
		if(!paramsOut->oldNick)
			return PEERFalse;
	}
	else
		paramsOut->oldNick = NULL;
	if(paramsIn->newNick)
	{
		paramsOut->newNick = goastrdup(paramsIn->newNick);
		if(!paramsOut->newNick)
		{
			gsifree(paramsOut->oldNick);
			return PEERFalse;
		}
	}
	else
		paramsOut->newNick = NULL;
	
	return PEERTrue;
}
static void piPlayerChangedNickFree(void * params_)
{
	piPlayerChangedNickParams * params = (piPlayerChangedNickParams *)params_;

	assert(params);

	gsifree(params->oldNick);
	gsifree(params->newNick);
}
static void piPlayerChangedNickCall(PEER peer, piCallbackData * data)
{
	piPlayerChangedNickParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_PLAYER_CHANGED_NICK_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerPlayerChangedNickCallback)data->callback)(peer, params->roomType, params->oldNick, params->newNick, data->callbackParam);
#else
	{
		unsigned short* oldNick_W = UTF8ToUCS2StringAlloc(params->oldNick);
		unsigned short* newNick_W = UTF8ToUCS2StringAlloc(params->newNick);
		((peerPlayerChangedNickCallback)data->callback)(peer, params->roomType, oldNick_W, newNick_W, data->callbackParam);
		gsifree(oldNick_W);
		gsifree(newNick_W);
	}
#endif
}
void piAddPlayerChangedNickCallback
(
	PEER peer,
	RoomType roomType,
	const char * oldNick,
	const char * newNick
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->playerChangedNick)
	{
		piPlayerChangedNickParams params;
		params.roomType = roomType;
		params.oldNick = (char *)oldNick;
		params.newNick = (char *)newNick;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->playerChangedNick, callbacks->param, PI_PLAYER_CHANGED_NICK_CALLBACK, &params, sizeof(piPlayerChangedNickParams), -1);
	}
}

/* Player Info.
**************/
typedef struct piPlayerInfoParams
{
	RoomType roomType;
	char * nick;
	unsigned int IP;
	int profileID;
} piPlayerInfoParams;
static PEERBool piPlayerInfoCopy(void * paramsOut_, void * paramsIn_)
{
	piPlayerInfoParams * paramsOut = (piPlayerInfoParams *)paramsOut_;
	piPlayerInfoParams * paramsIn = (piPlayerInfoParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	paramsOut->IP = paramsIn->IP;
	paramsOut->profileID = paramsIn->profileID;
	
	return PEERTrue;
}
static void piPlayerInfoFree(void * params_)
{
	piPlayerInfoParams * params = (piPlayerInfoParams *)params_;

	assert(params);

	gsifree(params->nick);
}
static void piPlayerInfoCall(PEER peer, piCallbackData * data)
{
	piPlayerInfoParams * params;
	PEER_CONNECTION;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_PLAYER_INFO_CALLBACK);

	params = data->params;

	// Don't call this if we're not in the room anymore.
	////////////////////////////////////////////////////
	if(!connection->inRoom[params->roomType])
		return;

#ifndef GSI_UNICODE
	((peerPlayerInfoCallback)data->callback)(peer, params->roomType, params->nick, params->IP, params->profileID, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		((peerPlayerInfoCallback)data->callback)(peer, params->roomType, nick_W, params->IP, params->profileID, data->callbackParam);
		gsifree(nick_W);
		}
#endif
}
void piAddPlayerInfoCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	unsigned int IP,
	int profileID
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->playerInfo)
	{
		piPlayerInfoParams params;
		params.roomType = roomType;
		params.nick = (char *)nick;
		params.IP = IP;
		params.profileID = profileID;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->playerInfo, callbacks->param, PI_PLAYER_INFO_CALLBACK, &params, sizeof(piPlayerInfoParams), -1);
	}
}

/* Disconnected.
***************/
typedef struct piDisconnectedParams
{
	char * reason;
} piDisconnectedParams;
static PEERBool piDisconnectedCopy(void * paramsOut_, void * paramsIn_)
{
	piDisconnectedParams * paramsOut = (piDisconnectedParams *)paramsOut_;
	piDisconnectedParams * paramsIn = (piDisconnectedParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	if(paramsIn->reason)
	{
		paramsOut->reason = goastrdup(paramsIn->reason);
		if(!paramsOut->reason)
			return PEERFalse;
	}
	else
		paramsOut->reason = NULL;
	
	return PEERTrue;
}
static void piDisconnectedFree(void * params_)
{
	piDisconnectedParams * params = (piDisconnectedParams *)params_;

	assert(params);

	gsifree(params->reason);
}
static void piDisconnectedCall(PEER peer, piCallbackData * data)
{
	piDisconnectedParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_DISCONNECTED_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerDisconnectedCallback)data->callback)(peer, params->reason, data->callbackParam);
#else
	{
		unsigned short* reason_W = UTF8ToUCS2StringAlloc(params->reason);
		((peerDisconnectedCallback)data->callback)(peer, reason_W, data->callbackParam);
		gsifree(reason_W);
		}
#endif
}
void piAddDisconnectedCallback
(
	PEER peer,
	const char * reason
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->disconnected)
	{
		piDisconnectedParams params;
		params.reason = (char *)reason;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->disconnected, callbacks->param, PI_DISCONNECTED_CALLBACK, &params, sizeof(piDisconnectedParams), -1);
	}
}

/* Ping.
*******/
typedef struct piPingParams
{
	char * nick;
	int ping;
} piPingParams;
static PEERBool piPingCopy(void * paramsOut_, void * paramsIn_)
{
	piPingParams * paramsOut = (piPingParams *)paramsOut_;
	piPingParams * paramsIn = (piPingParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->ping = paramsIn->ping;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	
	return PEERTrue;
}
static void piPingFree(void * params_)
{
	piPingParams * params = (piPingParams *)params_;

	assert(params);

	gsifree(params->nick);
}
static void piPingCall(PEER peer, piCallbackData * data)
{
	piPingParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_PING_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerPingCallback)data->callback)(peer, params->nick, params->ping, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		((peerPingCallback)data->callback)(peer, nick_W, params->ping, data->callbackParam);
		gsifree(nick_W);
		}
#endif
}
void piAddPingCallback
(
	PEER peer,
	const char * nick,
	int ping
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->ping)
	{
		piPingParams params;
		params.nick = (char *)nick;
		params.ping = ping;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->ping, callbacks->param, PI_PING_CALLBACK, &params, sizeof(piPingParams), -1);
	}
}

/* CrossPing.
************/
typedef struct piCrossPingParams
{
	char * nick1;
	char * nick2;
	int crossPing;
} piCrossPingParams;
static PEERBool piCrossPingCopy(void * paramsOut_, void * paramsIn_)
{
	piCrossPingParams * paramsOut = (piCrossPingParams *)paramsOut_;
	piCrossPingParams * paramsIn = (piCrossPingParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->crossPing = paramsIn->crossPing;
	if(paramsIn->nick1)
	{
		paramsOut->nick1 = goastrdup(paramsIn->nick1);
		if(!paramsOut->nick1)
			return PEERFalse;
	}
	else
		paramsOut->nick1 = NULL;
	if(paramsIn->nick2)
	{
		paramsOut->nick2 = goastrdup(paramsIn->nick2);
		if(!paramsOut->nick2)
		{
			gsifree(paramsOut->nick1);
			return PEERFalse;
		}
	}
	else
		paramsOut->nick2 = NULL;
	
	return PEERTrue;
}
static void piCrossPingFree(void * params_)
{
	piCrossPingParams * params = (piCrossPingParams *)params_;

	assert(params);

	gsifree(params->nick1);
	gsifree(params->nick2);
}
static void piCrossPingCall(PEER peer, piCallbackData * data)
{
	piCrossPingParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_CROSS_PING_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerCrossPingCallback)data->callback)(peer, params->nick1, params->nick2, params->crossPing, data->callbackParam);
#else
	{
		unsigned short* nick1_W = UTF8ToUCS2StringAlloc(params->nick1);
		unsigned short* nick2_W = UTF8ToUCS2StringAlloc(params->nick2);
		((peerCrossPingCallback)data->callback)(peer, nick1_W, nick2_W, params->crossPing, data->callbackParam);
		gsifree(nick1_W);
		gsifree(nick2_W);
	}
#endif
}
void piAddCrossPingCallback
(
	PEER peer,
	const char * nick1,
	const char * nick2,
	int crossPing
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->crossPing)
	{
		piCrossPingParams params;
		params.nick1 = (char *)nick1;
		params.nick2 = (char *)nick2;
		params.crossPing = crossPing;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->crossPing, callbacks->param, PI_CROSS_PING_CALLBACK, &params, sizeof(piCrossPingParams), -1);
	}
}

/* Change Nick.
**************/
typedef struct piChangeNickParams
{
	char * oldNick;
	char * newNick;
} piChangeNickParams;
static PEERBool piChangeNickCopy(void * paramsOut_, void * paramsIn_)
{
	piChangeNickParams * paramsOut = (piChangeNickParams *)paramsOut_;
	piChangeNickParams * paramsIn = (piChangeNickParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	if(paramsIn->newNick)
	{
		paramsOut->newNick = goastrdup(paramsIn->newNick);
		if(!paramsOut->newNick)
			return PEERFalse;
	}
	else
		paramsOut->newNick = NULL;
	if(paramsIn->oldNick)
	{
		paramsOut->oldNick = goastrdup(paramsIn->oldNick);
		if(!paramsOut->oldNick)
		{
			gsifree(paramsOut->newNick);
			return PEERFalse;
		}
	}
	else
		paramsOut->oldNick = NULL;

	return PEERTrue;
}
static void piChangeNickFree(void * params_)
{
	piChangeNickParams * params = (piChangeNickParams *)params_;

	assert(params);

	gsifree(params->newNick);
	gsifree(params->oldNick);
}
static void piChangeNickCall(PEER peer, piCallbackData * data)
{
	piChangeNickParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_CHANGE_NICK_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerChangeNickCallback)data->callback)(peer, data->success, params->oldNick, params->newNick, data->callbackParam);
#else
	{
		unsigned short* oldNick_W = UTF8ToUCS2StringAlloc(params->oldNick);
		unsigned short* newNick_W = UTF8ToUCS2StringAlloc(params->newNick);
		((peerChangeNickCallback)data->callback)(peer, data->success, oldNick_W, newNick_W, data->callbackParam);
		gsifree(oldNick_W);
		gsifree(newNick_W);
	}
#endif
}
void piAddChangeNickCallback
(
	PEER peer,
	PEERBool success,
	const char * oldNick,
	const char * newNick,
	peerChangeNickCallback callback,
	void * param,
	int opID
)
{
	piChangeNickParams params;
	params.newNick = (char *)newNick;
	params.oldNick = (char *)oldNick;

	piAddCallback(peer, success, (PEERCBType)callback, param, PI_CHANGE_NICK_CALLBACK, &params, sizeof(piChangeNickParams), opID);
}

/* GlobalKeyChanged.
*******************/
typedef struct piGlobalGlobalKeyChangedParams
{
	RoomType roomType;
	char * nick;
	char * key;
	char * value;
} piGlobalKeyChangedParams;
static PEERBool piGlobalKeyChangedCopy(void * paramsOut_, void * paramsIn_)
{
	piGlobalKeyChangedParams * paramsOut = (piGlobalKeyChangedParams *)paramsOut_;
	piGlobalKeyChangedParams * paramsIn = (piGlobalKeyChangedParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	if(paramsIn->key)
	{
		paramsOut->key = goastrdup(paramsIn->key);
		if(!paramsOut->key)
		{
			gsifree(paramsOut->nick);
			return PEERFalse;
		}
	}
	else
		paramsOut->key = NULL;
	if(paramsIn->value)
	{
		paramsOut->value = goastrdup(paramsIn->value);
		if(!paramsOut->value)
		{
			gsifree(paramsOut->nick);
			gsifree(paramsOut->key);
			return PEERFalse;
		}
	}
	else
		paramsOut->value = NULL;
	
	return PEERTrue;
}
static void piGlobalKeyChangedFree(void * params_)
{
	piGlobalKeyChangedParams * params = (piGlobalKeyChangedParams *)params_;

	assert(params);

	gsifree(params->nick);
	gsifree(params->key);
	gsifree(params->value);
}
static void piGlobalKeyChangedCall(PEER peer, piCallbackData * data)
{
	piGlobalKeyChangedParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_GLOBAL_KEY_CHANGED_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerGlobalKeyChangedCallback)data->callback)(peer, params->nick, params->key, params->value, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short* key_W = UTF8ToUCS2StringAlloc(params->key);
		unsigned short* value_W = UTF8ToUCS2StringAlloc(params->value);
		((peerGlobalKeyChangedCallback)data->callback)(peer, nick_W, key_W, value_W, data->callbackParam);
		gsifree(nick_W);
		gsifree(key_W);
		gsifree(value_W);
	}
#endif
}
void piAddGlobalKeyChangedCallback
(
	PEER peer,
	const char * nick,
	const char * key,
	const char * value
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->globalKeyChanged)
	{
		piGlobalKeyChangedParams params;
		params.nick = (char *)nick;
		params.key = (char *)key;
		params.value = (char *)value;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->globalKeyChanged, callbacks->param, PI_GLOBAL_KEY_CHANGED_CALLBACK, &params, sizeof(piGlobalKeyChangedParams), -1);
	}
}

/* RoomKeyChanged.
*****************/
typedef struct piRoomKeyChangedParams
{
	RoomType roomType;
	char * nick;
	char * key;
	char * value;
} piRoomKeyChangedParams;
static PEERBool piRoomKeyChangedCopy(void * paramsOut_, void * paramsIn_)
{
	piRoomKeyChangedParams * paramsOut = (piRoomKeyChangedParams *)paramsOut_;
	piRoomKeyChangedParams * paramsIn = (piRoomKeyChangedParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	if(paramsIn->key)
	{
		paramsOut->key = goastrdup(paramsIn->key);
		if(!paramsOut->key)
		{
			gsifree(paramsOut->nick);
			return PEERFalse;
		}
	}
	else
		paramsOut->key = NULL;
	if(paramsIn->value)
	{
		paramsOut->value = goastrdup(paramsIn->value);
		if(!paramsOut->value)
		{
			gsifree(paramsOut->nick);
			gsifree(paramsOut->key);
			return PEERFalse;
		}
	}
	else
		paramsOut->value = NULL;
	
	return PEERTrue;
}
static void piRoomKeyChangedFree(void * params_)
{
	piRoomKeyChangedParams * params = (piRoomKeyChangedParams *)params_;

	assert(params);

	gsifree(params->nick);
	gsifree(params->key);
	gsifree(params->value);
}
static void piRoomKeyChangedCall(PEER peer, piCallbackData * data)
{
	piRoomKeyChangedParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_ROOM_KEY_CHANGED_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerRoomKeyChangedCallback)data->callback)(peer, params->roomType, params->nick, params->key, params->value, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short* key_W = UTF8ToUCS2StringAlloc(params->key);
		unsigned short* value_W = UTF8ToUCS2StringAlloc(params->value);
		((peerRoomKeyChangedCallback)data->callback)(peer, params->roomType, nick_W, key_W, value_W, data->callbackParam);
		gsifree(nick_W);
		gsifree(key_W);
		gsifree(value_W);
	}
#endif
}
void piAddRoomKeyChangedCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * key,
	const char * value
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->roomKeyChanged)
	{
		piRoomKeyChangedParams params;
		params.roomType = roomType;
		params.nick = (char *)nick;
		params.key = (char *)key;
		params.value = (char *)value;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->roomKeyChanged, callbacks->param, PI_ROOM_KEY_CHANGED_CALLBACK, &params, sizeof(piRoomKeyChangedParams), -1);
	}
}

/* GetGlobalKeys.
****************/
typedef struct piGetGlobalKeysParams
{
	char * nick;
	int num;
	char ** keys;
	char ** values;
} piGetGlobalKeysParams;
static PEERBool piGetGlobalKeysCopy(void * paramsOut_, void * paramsIn_)
{
	int i;
	int num;
	PEERBool success = PEERTrue;
	piGetGlobalKeysParams * paramsOut = (piGetGlobalKeysParams *)paramsOut_;
	piGetGlobalKeysParams * paramsIn = (piGetGlobalKeysParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	memset(paramsOut, 0, sizeof(piGetGlobalKeysParams));
	num = paramsOut->num = paramsIn->num;

	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			success = PEERFalse;
	}
	else
		paramsOut->nick = NULL;

	if(success && num)
	{
		paramsOut->keys = (char **)gsimalloc(sizeof(char *) * num);
		if(!paramsOut->keys)
			success = PEERFalse;
		else
			memset(paramsOut->keys, 0, sizeof(char *) * num);
	}

	if(success && num && paramsIn->values)
	{
		paramsOut->values = (char **)gsimalloc(sizeof(char *) * num);
		if(!paramsOut->values)
			success = PEERFalse;
		else
			memset(paramsOut->values, 0, sizeof(char *) * num);
	}

	if(success && num && paramsIn->values)
	{
		for(i = 0 ; success && (i < num) ; i++)
		{
			paramsOut->keys[i] = goastrdup(paramsIn->keys[i]);
			if(!paramsOut->keys[i])
				success = PEERFalse;
			else
			{
				paramsOut->values[i] = goastrdup(paramsIn->values[i]);
				if(!paramsOut->values[i])
					success = PEERFalse;
			}
		}
	}

	if(!success)
	{
		gsifree(paramsOut->nick);
		for(i = 0 ; i < num ; i++)
		{
			if(paramsOut->keys)
				gsifree(paramsOut->keys[i]);
			if(paramsOut->values)
				gsifree(paramsOut->values[i]);
		}
		gsifree(paramsOut->keys);
		gsifree(paramsOut->values);
	}

	return success;
}
static void piGetGlobalKeysFree(void * params_)
{
	int i;
	piGetGlobalKeysParams * params = (piGetGlobalKeysParams *)params_;

	assert(params);

	gsifree(params->nick);
	for(i = 0 ; i < params->num ; i++)
	{
		gsifree(params->keys[i]);
		if(params->values)
			gsifree(params->values[i]);
	}
	gsifree(params->keys);
	gsifree(params->values);
}
static void piGetGlobalKeysCall(PEER peer, piCallbackData * data)
{
	piGetGlobalKeysParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_GET_GLOBAL_KEYS_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerGetGlobalKeysCallback)data->callback)(peer, data->success, params->nick, params->num, (const char **)params->keys, (const char **)params->values, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short** keys_W = UTF8ToUCS2StringArrayAlloc((const UTF8String *)params->keys, params->num);
		unsigned short** values_W = UTF8ToUCS2StringArrayAlloc((const UTF8String *)params->values, params->num);
		int i;
		((peerGetGlobalKeysCallback)data->callback)(peer, data->success, nick_W, params->num, (const unsigned short**)keys_W, (const unsigned short**)values_W, data->callbackParam);
		gsifree(nick_W);
		for (i=0; i < params->num; i++)
		{
			gsifree(keys_W[i]);
			if (values_W != NULL) // may be a NULL when getting keys for "*"
				gsifree(values_W[i]);
		}
		gsifree(keys_W);
		gsifree(values_W);
	}
#endif
}
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
)
{
	piGetGlobalKeysParams params;
	params.nick = (char *)nick;
	params.num = num;
	params.keys = (char **)keys;
	params.values = (char **)values;

	piAddCallback(peer, success, (PEERCBType)callback, param, PI_GET_GLOBAL_KEYS_CALLBACK, &params, sizeof(piGetGlobalKeysParams), opID);
}

/* GetRoomKeys.
****************/
typedef struct piGetRoomKeysParams
{
	RoomType roomType;
	char * nick;
	int num;
	char ** keys;
	char ** values;
} piGetRoomKeysParams;
static PEERBool piGetRoomKeysCopy(void * paramsOut_, void * paramsIn_)
{
	int i;
	int num;
	PEERBool success = PEERTrue;
	piGetRoomKeysParams * paramsOut = (piGetRoomKeysParams *)paramsOut_;
	piGetRoomKeysParams * paramsIn = (piGetRoomKeysParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	memset(paramsOut, 0, sizeof(piGetRoomKeysParams));
	num = paramsOut->num = paramsIn->num;

	paramsOut->roomType = paramsIn->roomType;

	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			success = PEERFalse;
	}
	else
		paramsOut->nick = NULL;

	if(success && num)
	{
		paramsOut->keys = (char **)gsimalloc(sizeof(char *) * num);
		if(!paramsOut->keys)
			success = PEERFalse;
		else
			memset(paramsOut->keys, 0, sizeof(char *) * num);
	}

	if(success && num && paramsIn->values)
	{
		paramsOut->values = (char **)gsimalloc(sizeof(char *) * num);
		if(!paramsOut->values)
			success = PEERFalse;
		else
			memset(paramsOut->values, 0, sizeof(char *) * num);
	}

	if(success && num && paramsIn->values)
	{
		for(i = 0 ; success && (i < num) ; i++)
		{
			paramsOut->keys[i] = goastrdup(paramsIn->keys[i]);
			if(!paramsOut->keys[i])
				success = PEERFalse;
			else if(paramsOut->values)
			{
				paramsOut->values[i] = goastrdup(paramsIn->values[i]);
				if(!paramsOut->values[i])
					success = PEERFalse;
			}
		}
	}

	if(!success)
	{
		gsifree(paramsOut->nick);
		for(i = 0 ; i < num ; i++)
		{
			if(paramsOut->keys)
				gsifree(paramsOut->keys[i]);
			if(paramsOut->values)
				gsifree(paramsOut->values[i]);
		}
		gsifree(paramsOut->keys);
		gsifree(paramsOut->values);
	}

	return success;
}
static void piGetRoomKeysFree(void * params_)
{
	int i;
	piGetRoomKeysParams * params = (piGetRoomKeysParams *)params_;

	assert(params);

	gsifree(params->nick);
	for(i = 0 ; i < params->num ; i++)
	{
		gsifree(params->keys[i]);
		if(params->values)
			gsifree(params->values[i]);
	}
	gsifree(params->keys);
	gsifree(params->values);
}
static void piGetRoomKeysCall(PEER peer, piCallbackData * data)
{
	piGetRoomKeysParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_GET_ROOM_KEYS_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerGetRoomKeysCallback)data->callback)(peer, data->success, params->roomType, params->nick, params->num, params->keys, params->values, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		unsigned short** keys_W = UTF8ToUCS2StringArrayAlloc((const UTF8String *)params->keys, params->num);
		unsigned short** values_W = UTF8ToUCS2StringArrayAlloc((const UTF8String *)params->values, params->num);
		int i;
		((peerGetRoomKeysCallback)data->callback)(peer, data->success, params->roomType, nick_W, params->num, keys_W, values_W, data->callbackParam);
		gsifree(nick_W);
		for (i=0; i < params->num; i++)
		{
			gsifree(keys_W[i]);
			if (values_W != NULL) // may be a NULL when getting keys for "*"
				gsifree(values_W[i]);
		}
		gsifree(keys_W);
		gsifree(values_W);
	}
#endif
}
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
)
{
	piGetRoomKeysParams params;
	params.roomType = roomType;
	params.nick = (char *)nick;
	params.num = num;
	params.keys = (char **)keys;
	params.values = (char **)values;

	piAddCallback(peer, success, (PEERCBType)callback, param, PI_GET_ROOM_KEYS_CALLBACK, &params, sizeof(piGetRoomKeysParams), opID);
}

/* PlayerFlagsChanged.
*********************/
typedef struct piPlayerFlagsChangedParams
{
	RoomType roomType;
	char * nick;
	int oldFlags;
	int newFlags;
} piPlayerFlagsChangedParams;
static PEERBool piPlayerFlagsChangedCopy(void * paramsOut_, void * paramsIn_)
{
	piPlayerFlagsChangedParams * paramsOut = (piPlayerFlagsChangedParams *)paramsOut_;
	piPlayerFlagsChangedParams * paramsIn = (piPlayerFlagsChangedParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->roomType = paramsIn->roomType;
	paramsOut->newFlags = paramsIn->newFlags;
	paramsOut->oldFlags = paramsIn->oldFlags;
	if(paramsIn->nick)
	{
		paramsOut->nick = goastrdup(paramsIn->nick);
		if(!paramsOut->nick)
			return PEERFalse;
	}
	else
		paramsOut->nick = NULL;
	
	return PEERTrue;
}
static void piPlayerFlagsChangedFree(void * params_)
{
	piPlayerFlagsChangedParams * params = (piPlayerFlagsChangedParams *)params_;

	assert(params);

	gsifree(params->nick);
}
static void piPlayerFlagsChangedCall(PEER peer, piCallbackData * data)
{
	piPlayerFlagsChangedParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_PLAYER_FLAGS_CHANGED_CALLBACK);

	params = data->params;
#ifndef GSI_UNICODE
	((peerPlayerFlagsChangedCallback)data->callback)(peer, params->roomType, params->nick, params->oldFlags, params->newFlags, data->callbackParam);
#else
	{
		unsigned short* nick_W = UTF8ToUCS2StringAlloc(params->nick);
		((peerPlayerFlagsChangedCallback)data->callback)(peer, params->roomType, nick_W, params->oldFlags, params->newFlags, data->callbackParam);
		gsifree(nick_W);
	}
#endif
}
void piAddPlayerFlagsChangedCallback
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	int oldFlags,
	int newFlags
)
{
	PEERCallbacks * callbacks;

	PEER_CONNECTION;

	callbacks = &connection->callbacks;
	if(callbacks->playerFlagsChanged)
	{
		piPlayerFlagsChangedParams params;
		params.roomType = roomType;
		params.nick = (char *)nick;
		params.oldFlags = oldFlags;
		params.newFlags = newFlags;

		piAddCallback(peer, PEERTrue, (PEERCBType)callbacks->playerFlagsChanged, callbacks->param, PI_PLAYER_FLAGS_CHANGED_CALLBACK, &params, sizeof(piPlayerFlagsChangedParams), -1);
	}
}

/* Authenticate CD Key.
**********************/
typedef struct piAuthenticateCDKeyParams
{
	int result;
	char * message;
} piAuthenticateCDKeyParams;
static PEERBool piAuthenticateCDKeyCopy(void * paramsOut_, void * paramsIn_)
{
	piAuthenticateCDKeyParams * paramsOut = (piAuthenticateCDKeyParams *)paramsOut_;
	piAuthenticateCDKeyParams * paramsIn = (piAuthenticateCDKeyParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->result = paramsIn->result;
	if(paramsIn->message)
	{
		paramsOut->message = goastrdup(paramsIn->message);
		if(!paramsOut->message)
			return PEERFalse;
	}
	else
		paramsOut->message = NULL;

	return PEERTrue;
}
static void piAuthenticateCDKeyFree(void * params_)
{
	piAuthenticateCDKeyParams * params = (piAuthenticateCDKeyParams *)params_;

	assert(params);

	gsifree(params->message);
}
static void piAuthenticateCDKeyCall(PEER peer, piCallbackData * data)
{
	piAuthenticateCDKeyParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_AUTHENTICATE_CDKEY_CALLBACK);
	
	params = data->params;
#ifndef GSI_UNICODE
	((peerAuthenticateCDKeyCallback)data->callback)(peer, params->result, params->message, data->callbackParam);
#else
	{
		unsigned short* message_W = UTF8ToUCS2StringAlloc(params->message);
		((peerAuthenticateCDKeyCallback)data->callback)(peer, params->result, message_W, data->callbackParam);
		gsifree(message_W);
	}
#endif
}
void piAddAuthenticateCDKeyCallback
(
	PEER peer,
	int result,
	const char * message,
	peerAuthenticateCDKeyCallback callback,
	void * param,
	int opID
)
{
	piAuthenticateCDKeyParams params;
	params.result = result;
	params.message = (char *)message;

	piAddCallback(peer, PEERTrue, (PEERCBType)callback, param, PI_AUTHENTICATE_CDKEY_CALLBACK, &params, sizeof(piAuthenticateCDKeyParams), opID);
}

/* AutoMatch Status.
*******************/
typedef struct piAutoMatchStatusParams
{
	PEERAutoMatchStatus status;
} piAutoMatchStatusParams;
static PEERBool piAutoMatchStatusCopy(void * paramsOut_, void * paramsIn_)
{
	piAutoMatchStatusParams * paramsOut = (piAutoMatchStatusParams *)paramsOut_;
	piAutoMatchStatusParams * paramsIn = (piAutoMatchStatusParams *)paramsIn_;

	assert(paramsOut);
	assert(paramsIn);

	paramsOut->status = paramsIn->status;

	return PEERTrue;
}
static void piAutoMatchStatusFree(void * params_)
{
	piAutoMatchStatusParams * params = (piAutoMatchStatusParams *)params_;

	assert(params);
	GSI_UNUSED(params);
}
static void piAutoMatchStatusCall(PEER peer, piCallbackData * data)
{
	piAutoMatchStatusParams * params;
	
	assert(data);
	assert(data->callback);
	assert(data->params);
	assert(data->type == PI_AUTO_MATCH_STATUS_CALLBACK);

	params = data->params;
	((peerAutoMatchStatusCallback)data->callback)(peer, params->status, data->callbackParam);
}
void piAddAutoMatchStatusCallback
(
	PEER peer
)
{
	piAutoMatchStatusParams params;
	piOperation * operation;

	PEER_CONNECTION;

	operation = connection->autoMatchOperation;
	if(!operation || !operation->callback)
		return;

	params.status = connection->autoMatchStatus;

	piAddCallback(peer, PEERTrue, operation->callback, operation->callbackParam, PI_AUTO_MATCH_STATUS_CALLBACK, &params, sizeof(piAutoMatchStatusParams), operation->ID);
}

/* AutoMatch Rate.
*****************/
int piCallAutoMatchRateCallback
(
	PEER peer,
	SBServer server
)
{
	piOperation * operation;

	PEER_CONNECTION;

	operation = connection->autoMatchOperation;
	if(!operation || !operation->callback2)
		return 0;

	return ((peerAutoMatchRateCallback)connection->autoMatchOperation->callback2)(peer, server, operation->callbackParam);
}

/************
** GLOBALS **
************/
static const piCallbackFuncs callbackFuncs[] =
{
	{
		PI_CONNECT_CALLBACK,
		piConnectCopy,
		piConnectFree,
		piConnectCall
	},
	{
		PI_JOIN_ROOM_CALLBACK,
		piJoinRoomCopy,
		piJoinRoomFree,
		piJoinRoomCall
	},
	{
		PI_LIST_GROUP_ROOMS_CALLBACK,
		piListGroupRoomsCopy,
		piListGroupRoomsFree,
		piListGroupRoomsCall
	},
	{
		PI_LISTING_GAMES_CALLBACK,
		piListingGamesCopy,
		piListingGamesFree,
		piListingGamesCall
	},
	{
		PI_NICK_ERROR_CALLBACK,
		piNickErrorCopy,
		piNickErrorFree,
		piNickErrorCall
	},
	{
		PI_ENUM_PLAYERS_CALLBACK,
		piEnumPlayersCopy,
		piEnumPlayersFree,
		piEnumPlayersCall
	},
	{
		PI_GET_PLAYER_INFO_CALLBACK,
		piGetPlayerInfoCopy,
		piGetPlayerInfoFree,
		piGetPlayerInfoCall
	},
	{
		PI_GET_PLAYER_PROFILE_ID_CALLBACK,
		piGetPlayerProfileIDCopy,
		piGetPlayerProfileIDFree,
		piGetPlayerProfileIDCall
	},
	{
		PI_GET_PLAYER_IP_CALLBACK,
		piGetPlayerIPCopy,
		piGetPlayerIPFree,
		piGetPlayerIPCall
	},
	{
		PI_ROOM_MESSAGE_CALLBACK,
		piRoomMessageCopy,
		piRoomMessageFree,
		piRoomMessageCall
	},
	{
		PI_ROOM_UTM_CALLBACK,
		piRoomUTMCopy,
		piRoomUTMFree,
		piRoomUTMCall
	},
	{
		PI_ROOM_NAME_CHANGED_CALLBACK,
		piRoomNameChangedCopy,
		piRoomNameChangedFree,
		piRoomNameChangedCall
	},
	{
		PI_ROOM_MODE_CHANGED_CALLBACK,
		piRoomModeChangedCopy,
		piRoomModeChangedFree,
		piRoomModeChangedCall
	},
	{
		PI_PLAYER_MESSAGE_CALLBACK,
		piPlayerMessageCopy,
		piPlayerMessageFree,
		piPlayerMessageCall
	},
	{
		PI_PLAYER_UTM_CALLBACK,
		piPlayerUTMCopy,
		piPlayerUTMFree,
		piPlayerUTMCall
	},
	{
		PI_READY_CHANGED_CALLBACK,
		piReadyChangedCopy,
		piReadyChangedFree,
		piReadyChangedCall
	},
	{
		PI_GAME_STARTED_CALLBACK,
		piGameStartedCopy,
		piGameStartedFree,
		piGameStartedCall
	},
	{
		PI_PLAYER_JOINED_CALLBACK,
		piPlayerJoinedCopy,
		piPlayerJoinedFree,
		piPlayerJoinedCall
	},
	{
		PI_PLAYER_LEFT_CALLBACK,
		piPlayerLeftCopy,
		piPlayerLeftFree,
		piPlayerLeftCall
	},
	{
		PI_KICKED_CALLBACK,
		piKickedCopy,
		piKickedFree,
		piKickedCall
	},
	{
		PI_NEW_PLAYER_LIST_CALLBACK,
		piNewPlayerListCopy,
		piNewPlayerListFree,
		piNewPlayerListCall
	},
	{
		PI_PLAYER_CHANGED_NICK_CALLBACK,
		piPlayerChangedNickCopy,
		piPlayerChangedNickFree,
		piPlayerChangedNickCall
	},
	{
		PI_PLAYER_INFO_CALLBACK,
		piPlayerInfoCopy,
		piPlayerInfoFree,
		piPlayerInfoCall
	},
	{
		PI_DISCONNECTED_CALLBACK,
		piDisconnectedCopy,
		piDisconnectedFree,
		piDisconnectedCall
	},
	{
		PI_PING_CALLBACK,
		piPingCopy,
		piPingFree,
		piPingCall
	},
	{
		PI_CROSS_PING_CALLBACK,
		piCrossPingCopy,
		piCrossPingFree,
		piCrossPingCall
	},
	{
		PI_CHANGE_NICK_CALLBACK,
		piChangeNickCopy,
		piChangeNickFree,
		piChangeNickCall
	},
	{
		PI_GLOBAL_KEY_CHANGED_CALLBACK,
		piGlobalKeyChangedCopy,
		piGlobalKeyChangedFree,
		piGlobalKeyChangedCall
	},
	{
		PI_ROOM_KEY_CHANGED_CALLBACK,
		piRoomKeyChangedCopy,
		piRoomKeyChangedFree,
		piRoomKeyChangedCall
	},
	{
		PI_GET_GLOBAL_KEYS_CALLBACK,
		piGetGlobalKeysCopy,
		piGetGlobalKeysFree,
		piGetGlobalKeysCall
	},
	{
		PI_GET_ROOM_KEYS_CALLBACK,
		piGetRoomKeysCopy,
		piGetRoomKeysFree,
		piGetRoomKeysCall
	},
	{
		PI_PLAYER_FLAGS_CHANGED_CALLBACK,
		piPlayerFlagsChangedCopy,
		piPlayerFlagsChangedFree,
		piPlayerFlagsChangedCall
	},
	{
		PI_AUTHENTICATE_CDKEY_CALLBACK,
		piAuthenticateCDKeyCopy,
		piAuthenticateCDKeyFree,
		piAuthenticateCDKeyCall
	},
	{
		PI_AUTO_MATCH_STATUS_CALLBACK,
		piAutoMatchStatusCopy,
		piAutoMatchStatusFree,
		piAutoMatchStatusCall
	},
	{
		PI_NUM_CALLBACK_TYPES,
		NULL,
		NULL,
		NULL
	}
};

/**************
** FUNCTIONS **
**************/
static void piCallbackListFree(void *elem1)
{
	piCallbackData * data = (piCallbackData *)elem1;
	//ASSERT_DATA(data);

	// Call the gsifree func.
	//////////////////////
	callbackFuncs[data->type].gsifree(data->params);

	// Cleanup the callback data.
	/////////////////////////////
#ifdef GSI_MANIC_DEBUG
	// Set the data to a fill value so we catch the overwrite
	memset(data->params, 0xea, sizeof(data->params));
#endif
	gsifree(data->params);

}

PEERBool piCallbacksInit
(
	PEER peer
)
{
	PEER_CONNECTION;

#ifdef _DEBUG
{
	// Consistency check.
	/////////////////////
	int i;
	for(i = 0 ; i <= PI_NUM_CALLBACK_TYPES ; i++)
		assert(callbackFuncs[i].type == i);
}
#endif

	// No callbacks yet.
	////////////////////
	connection->callbacksQueued = 0;
	connection->callbacksCalled = 0;
	connection->callbackDepth = 0;

	// Init the list.
	/////////////////
	connection->callbackList = ArrayNew(sizeof(piCallbackData), 0, piCallbackListFree);
	if(!connection->callbackList)
		return PEERFalse;

	return PEERTrue;
}

void piCallbacksCleanup
(
	PEER peer
)
{
	PEER_CONNECTION;

	// gsifree the callback list.
	//////////////////////////
	if(connection->callbackList)
		ArrayFree(connection->callbackList);
}

#ifdef GSI_MANIC_DEBUG
#include "stdio.h"
#endif
static void piCallCallback(PEER peer, piCallbackData * data, int index)
{
	PEER_CONNECTION;

	// In the call.
	///////////////
	data->inCall = PEERTrue;
	connection->callbackDepth++;

	// Call it.
	///////////
	callbackFuncs[data->type].call(peer, data);

	// Out of the call.
	///////////////////
	data->inCall = PEERFalse;
	connection->callbackDepth--;

	// One more called.
	///////////////////
	connection->callbacksCalled++;

	// gsifree it.
	///////////
	ArrayDeleteAt(connection->callbackList, index);
}

void piCallbacksThink
(
	PEER peer,
	int blockingID
)
{
	int index;
	int len;
	piCallbackData * data;

	PEER_CONNECTION;

	assert(blockingID >= -1);

	// Blocking call?
	/////////////////
	if(blockingID != -1)
	{
		// How many?
		////////////
		len = ArrayLength(connection->callbackList);
		assert(len >= 0);

		// Check if this callback is finished.
		//////////////////////////////////////
		for(index = 0 ; index < len ; index++)
		{
			// Get the nth element.
			///////////////////////
			data = (piCallbackData *)ArrayNth(connection->callbackList, index);
			assert(data);

			// Check the ID and specifically for disconnect.
			////////////////////////////////////////////////
			if((data->ID == blockingID) || (data->type == PI_DISCONNECTED_CALLBACK))
			{
				// Call it.
				///////////
				piCallCallback(peer, data, index);

				break;
			}
		}
	}
	else
	{
		int numInCalls = 0;
		while(ArrayLength(connection->callbackList) > numInCalls)
		{
			// Get the callback data.
			/////////////////////////
			data = (piCallbackData *)ArrayNth(connection->callbackList, numInCalls);
			assert(data);

			// Are we already in this call? (how philosophical)
			///////////////////////////////////////////////////
			if(data->inCall)
			{
				numInCalls++;
			}
			else
			{
				// Call it.
				///////////
				piCallCallback(peer, data, numInCalls);
			}
		}
	}
}

static int piAddCallback
(
	PEER peer,
	PEERBool success,
	PEERCBType callback,
	void * param,
	piCallbackType type,
	void * paramsIn,
	size_t paramsSize,
	int opID
)
{
	piCallbackData data;
	void * paramsOut;

	PEER_CONNECTION;

	assert(callback);
	//assert(type >= 0);
	assert(type < PI_NUM_CALLBACK_TYPES);
	assert(paramsIn);
	assert(paramsSize > 0);

	// If no callback, nothing to do.
	/////////////////////////////////
	if(!callback)
		return -1;

	// Allocate the output struct.
	//////////////////////////////
	paramsOut = gsimalloc(paramsSize);
	if(!paramsOut)
		return -1;

	// Zero it.
	///////////
	memset(paramsOut, 0, paramsSize);

	// Copy the input to the output.
	////////////////////////////////
	if(!callbackFuncs[type].copy(paramsOut, paramsIn))
	{
		assert(0);
		gsifree(paramsOut);
		return -1;
	}

	// Fill in the data.
	////////////////////
	data.type = type;
	data.success = success;
	data.callback = callback;
	data.callbackParam = param;
	data.params = paramsOut;
	data.ID = opID;
	data.inCall = PEERFalse;

	// Add it to the list.
	//////////////////////
	ArrayAppend(connection->callbackList, &data);
	connection->callbacksQueued++;

	return data.ID;
}

static int GS_STATIC_CALLBACK piIsCallbackFinishedCompareCallback
(
 const void *elem1, 
 const void *elem2
)
{
	piCallbackData * data1 = (piCallbackData *)elem1;
	piCallbackData * data2 = (piCallbackData *)elem2;
	assert(data1);
	assert(data2);

	return (data1->ID - data2->ID);
}

PEERBool piIsCallbackFinished
(
	PEER peer,
	int opID
)
{
	int index;
	piCallbackData data;

	PEER_CONNECTION;

	// Search for it.
	/////////////////
	data.ID = opID;
	index = ArraySearch(connection->callbackList, &data, piIsCallbackFinishedCompareCallback, 0, 0);
	
	return (index == NOT_FOUND)?PEERTrue:PEERFalse;
}

void piClearCallbacks
(
	PEER peer,
	piCallbackType type
)
{
	piCallbackData * data;
	int len;
	int i;

	PEER_CONNECTION;

	len = ArrayLength(connection->callbackList);
	for(i = (len - 1) ; i >= 0 ; i--)
	{
		data = (piCallbackData *)ArrayNth(connection->callbackList, i);
		if(data->type == type)
			ArrayDeleteAt(connection->callbackList, i);
	}
}

void piClearListingGameServerCallbacks(PEER peer, SBServer server)
{
	piCallbackData * data;
	piListingGamesParams * params;
	int len;
	int i;

	PEER_CONNECTION;

	len = ArrayLength(connection->callbackList);
	for(i = (len - 1) ; i >= 0 ; i--)
	{
		data = (piCallbackData *)ArrayNth(connection->callbackList, i);
		if(data->type == PI_LISTING_GAMES_CALLBACK)
		{
			params = (piListingGamesParams *)data->params;
			if(params->server == server)
				ArrayDeleteAt(connection->callbackList, i);
		}
	}
}
