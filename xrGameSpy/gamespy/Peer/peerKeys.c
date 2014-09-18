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
#include <stdlib.h>
#include "peerKeys.h"
#include "peerCallbacks.h"
#include "peerRooms.h"
#include "peerGlobalCallbacks.h"
#include "peerMangle.h"

/************
** DEFINES **
************/
#define PI_WATCH_KEYS_NUM_BUCKETS      16
#define PI_WATCH_CACHE_NUM_BUCKETS     128

/**********
** TYPES **
**********/
// Stored in the *WatckKey tables.
//////////////////////////////////
typedef struct piWatchKey
{
	char * key;
} piWatchKey;

// Stored in the *WatchCache tables.
////////////////////////////////////
typedef struct piCacheKey
{
	char * nick;
	char * key;
	char * value;
#ifdef GSI_UNICODE
	unsigned short * value_W;
#endif
} piCacheKey;

typedef struct piClearWatchKeyCacheData
{
	const char * key;
	HashTable watchCache;
} piClearWatchKeyCacheData;

typedef struct piRemoveExistingKeysData
{
	int num;
	const char ** keys;
	HashTable watchKeys;
} piRemoveExistingKeysData;

typedef struct piPlayerChangedNickMapData
{
	const char * oldNick;
	const char * newNick;
} piPlayerChangedNickMapData;

typedef struct piSetupKeysMapData
{
	int next;
	char ** keys;
} piSetupKeysMapData;

typedef struct piCleanseRoomCacheMapData
{
	PEER peer;
	RoomType roomType;
} piCleanseRoomCacheMapData;

/**************
** FUNCTIONS **
**************/
#ifndef GSI_UNICODE // NOT GSI_UNICODE, these have to coincide with the chatsdk build
#define piGetRoomKeysCallback		piGetRoomKeysCallbackA
#define piGetGlobalKeysCallback		piGetGlobalKeysCallbackA
#else
#define piGetRoomKeysCallback		piGetRoomKeysCallbackW
#define piGetGlobalKeysCallback		piGetGlobalKeysCallbackW
#endif


static int WatchKeysHash
(
	const void * elem,
	int numBuckets
)
{
	piWatchKey * key = (piWatchKey *)elem;
	int c;
	const char * str;
	unsigned int hash;

	assert(key->key && key->key[0]);

	// Get the hash.
	////////////////
	str = key->key;
	hash = 0;
	while((c = *str++) != '\0')
		hash += (unsigned int)tolower(c);
	hash %= (unsigned int)numBuckets;

	return (int)hash;
}

static int GS_STATIC_CALLBACK WatchKeysCompare
(
	const void * elem1,
	const void * elem2
)
{
	piWatchKey * key1 = (piWatchKey *)elem1;
	piWatchKey * key2 = (piWatchKey *)elem2;

	assert(key1->key && key1->key[0] && key2->key && key2->key[0]);

	return strcasecmp(key1->key, key2->key);
}

static void WatchKeysFree
(
	void * elem
)
{
	piWatchKey * key = (piWatchKey *)elem;

	gsifree(key->key);
}

static int WatchCacheHash
(
	const void * elem,
	int numBuckets
)
{
	piCacheKey * key = (piCacheKey *)elem;
	int c;
	const char * str;
	unsigned int hash;

	assert(key);
	assert(key->nick[0]);

	// Get the hash.
	////////////////
	str = key->key;
	hash = 0;
	while((c = *str++) != '\0')
		hash += (unsigned int)tolower(c);
	hash %= (unsigned int)numBuckets;

	return (int)hash;
}

static int GS_STATIC_CALLBACK WatchCacheCompare
(
	const void * elem1,
	const void * elem2
)
{
	int rcode;

	piCacheKey * key1 = (piCacheKey *)elem1;
	piCacheKey * key2 = (piCacheKey *)elem2;

	rcode = strcasecmp(key1->nick, key2->nick);
	if(rcode)
		return rcode;

	return strcasecmp(key1->key, key2->key);
}

static void WatchCacheFree
(
	void * elem
)
{
	piCacheKey * key = (piCacheKey *)elem;

	gsifree(key->nick);
	gsifree(key->key);
	gsifree(key->value);
#ifdef GSI_UNICODE
	gsifree(key->value_W);
#endif
}

PEERBool piKeysInit
(
	PEER peer
)
{
	int roomType;
	PEER_CONNECTION;

	memset(connection->globalWatchKeys, 0, sizeof(HashTable) * NumRooms);
	memset(connection->roomWatchKeys, 0, sizeof(HashTable) * NumRooms);
	memset(connection->roomWatchCache, 0, sizeof(HashTable) * NumRooms);

	// Create all the tables.
	/////////////////////////
	connection->globalWatchCache = TableNew(sizeof(piCacheKey), PI_WATCH_CACHE_NUM_BUCKETS,
			WatchCacheHash, WatchCacheCompare, WatchCacheFree);
	if(!connection->globalWatchCache)
		return PEERFalse;
	for(roomType = 0 ; roomType < NumRooms ; roomType++)
	{
		connection->globalWatchKeys[roomType] = TableNew(sizeof(piWatchKey), PI_WATCH_KEYS_NUM_BUCKETS,
			WatchKeysHash, WatchKeysCompare, WatchKeysFree);
		connection->roomWatchKeys[roomType] = TableNew(sizeof(piWatchKey), PI_WATCH_KEYS_NUM_BUCKETS,
			WatchKeysHash, WatchKeysCompare, WatchKeysFree);
		connection->roomWatchCache[roomType] = TableNew(sizeof(piCacheKey), PI_WATCH_CACHE_NUM_BUCKETS,
			WatchCacheHash, WatchCacheCompare, WatchCacheFree);

		if(!connection->globalWatchKeys[roomType] || !connection->roomWatchKeys[roomType] ||
			!connection->roomWatchCache[roomType])
		{
			return PEERFalse;
		}
	}

	return PEERTrue;
}

void piKeysCleanup
(
	PEER peer
)
{
	int roomType;

	PEER_CONNECTION;

	if(connection->globalWatchCache)
		TableFree(connection->globalWatchCache);

	for(roomType = 0 ; roomType < NumRooms ; roomType++)
	{
		if(connection->globalWatchKeys[roomType])
			TableFree(connection->globalWatchKeys[roomType]);
		if(connection->roomWatchKeys[roomType])
			TableFree(connection->roomWatchKeys[roomType]);
		if(connection->roomWatchCache[roomType])
			TableFree(connection->roomWatchCache[roomType]);
	}
}

static const char * piGetWatchKeyA
(
	const char * nick,
	const char * key,
	HashTable watchCache
)
{
	piCacheKey keyTemp;
	piCacheKey * cacheKey;

	keyTemp.nick = (char *)nick;
	keyTemp.key = (char *)key;
	cacheKey = (piCacheKey *)TableLookup(watchCache, &keyTemp);

	if(!cacheKey)
		return NULL;

	if(cacheKey->value)
		return cacheKey->value;
	return "";
}
static const unsigned short * piGetWatchKeyW
(
	const unsigned short * nick,
	const unsigned short * key,
	HashTable watchCache
)
{
#ifndef GSI_UNICODE
	GSI_UNUSED(nick);
	GSI_UNUSED(key);
	GSI_UNUSED(watchCache);
	return NULL; // can't use this function unless in GSI_UNICODE mode
#else
	piCacheKey keyTemp;
	piCacheKey * cacheKey;

	char* nick_A = UCS2ToUTF8StringAlloc(nick);
	char* key_A = UCS2ToUTF8StringAlloc(key);

	keyTemp.nick = (char *)nick_A;
	keyTemp.key = (char *)key_A;
	cacheKey = (piCacheKey *)TableLookup(watchCache, &keyTemp);

	gsifree(nick_A);
	gsifree(key_A);

	if (!cacheKey)
		return NULL;

	if(cacheKey->value_W)
		return cacheKey->value_W;
	return L"";
#endif
}

const char * piGetGlobalWatchKeyA
(
	PEER peer,
	const char * nick,
	const char * key
)
{
	PEER_CONNECTION;

	return piGetWatchKeyA(nick, key, connection->globalWatchCache);
}
const unsigned short * piGetGlobalWatchKeyW
(
	PEER peer,
	const unsigned short * nick,
	const unsigned short * key
)
{
	PEER_CONNECTION;

	return piGetWatchKeyW(nick, key, connection->globalWatchCache);
}

const char * piGetRoomWatchKeyA
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * key
)
{
	PEER_CONNECTION;

	return piGetWatchKeyA(nick, key, connection->roomWatchCache[roomType]);
}
const unsigned short * piGetRoomWatchKeyW
(
	PEER peer,
	RoomType roomType,
	const unsigned short * nick,
	const unsigned short * key
)
{
	PEER_CONNECTION;

	return piGetWatchKeyW(nick, key, connection->roomWatchCache[roomType]);
}

static void piCleanseGlobalCacheMap
(
	void * elem,
	void * clientData
)
{
	piPlayer * player;
	int roomType;
	piCacheKey * cacheKey = (piCacheKey *)elem;
	PEER peer = (PEER)clientData;
	piConnection * connection = (piConnection *)peer;
	piWatchKey watchKeyTemp;

	watchKeyTemp.key = cacheKey->key;

	player = piGetPlayer(peer, cacheKey->nick);
	if(player)
	{
		for(roomType = 0 ; roomType < NumRooms ; roomType++)
		{
			if(player->inRoom[roomType])
			{
				if(TableLookup(connection->globalWatchKeys[roomType], &watchKeyTemp))
					return;
			}
		}
	}

	TableRemove(connection->globalWatchCache, cacheKey);
}

static void piCleanseRoomCacheMap
(
	void * elem,
	void * clientData
)
{
	piPlayer * player;
	piCacheKey * cacheKey = (piCacheKey *)elem;
	piCleanseRoomCacheMapData * data = (piCleanseRoomCacheMapData *)clientData;
	piConnection * connection = (piConnection *)data->peer;
	piWatchKey watchKeyTemp;

	watchKeyTemp.key = cacheKey->key;

	player = piGetPlayer(data->peer, cacheKey->nick);
	if(player && player->inRoom[data->roomType])
	{
		if(TableLookup(connection->roomWatchKeys[data->roomType], &watchKeyTemp))
			return;
	}

	TableRemove(connection->globalWatchCache, cacheKey);
}

void piKeyCacheCleanse
(
	PEER peer
)
{
	int roomType;
	piCleanseRoomCacheMapData data;

	PEER_CONNECTION;

	TableMapSafe(connection->globalWatchCache, piCleanseGlobalCacheMap, peer);

	data.peer = peer;
	for(roomType = 0 ; roomType < NumRooms ; roomType++)
	{
		if(IN_ROOM || ENTERING_ROOM)
		{
			data.roomType = (RoomType)roomType;
			TableMapSafe(connection->roomWatchCache[roomType], piCleanseRoomCacheMap, &data);
		}
		else
		{
			TableClear(connection->roomWatchCache[roomType]);
		}
	}
}

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
	PEER peer = (PEER)param;

	if(success && user)
	{
		int i;

		for(i = 0 ; i < num ; i++)
			piGlobalKeyChanged(peer, user, keys[i], values[i]);
	}
	
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
		if (keys_A != NULL)	gsifree(keys_A[i]);
		if (values_A != NULL) gsifree(values_A[i]);
	}
	gsifree(keys_A);
	gsifree(values_A);
}
#endif

static void piGetRoomKeysCallbackA
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
	PEER peer = (PEER)param;

	if(!user && success)
	{
		//
		// If <user> is NULL, then this has been called by the
		// GETCKEY list-end handler (ciRplEndGetCKeyHandler),
		// and the PlayerInfo callback should be called one final
		// time with NULL as the user.
		//
		// Alter the username and proceed so that all of the
		// subsequent handlers and callbacks have a real name to
		// work with. Note "(END)" is illegal as an IRC nickname
		// (09mar01/bgw).
		//
		int i;
		RoomType roomType;
		static const char * szEndName = "(END)";

		if(!piRoomToType(peer, channel, &roomType))
			return;

		for(i = 0 ; i < num ; i++)
			piRoomKeyChanged(peer, roomType, szEndName, keys[i], NULL);
		return;
	}

	if(success /*&& user*/)
	{
		int i;
		RoomType roomType;

		if(!piRoomToType(peer, channel, &roomType))
			return;

		for(i = 0 ; i < num ; i++)
			piRoomKeyChanged(peer, roomType, user, keys[i], values[i]);
	}
	
	GSI_UNUSED(chat);
}
#ifdef GSI_UNICODE
static void piGetRoomKeysCallbackW
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
	piGetRoomKeysCallbackA(chat, success, channel_A, user_A, num, (const char**)keys_A, (const char**)values_A, param);
	gsifree(channel_A);
	gsifree(user_A);
	for(i=0; i<num; i++)
	{
		if (keys_A != NULL) gsifree(keys_A[i]);
		if (values_A != NULL) gsifree(values_A[i]);
	}
	gsifree(keys_A);
	gsifree(values_A);
}
#endif

static void piRemoveExistingKeysMap
(
	void * elem,
	void * clientData
)
{
	int i;

	piWatchKey * key = (piWatchKey *)elem;
	piRemoveExistingKeysData * data = (piRemoveExistingKeysData *)clientData;

	for(i = 0 ; i < data->num ; i++)
		if(strcasecmp(key->key, data->keys[i]) == 0)
			return;

	TableRemove(data->watchKeys, key);
}

void piSetGlobalWatchKeys
(
	PEER peer,
	RoomType roomType,
	int num,
	const char ** keys,
	PEERBool addKeys
)
{
	piWatchKey watchKey;
	int i;

	PEER_CONNECTION;

	ASSERT_ROOMTYPE(roomType);
	assert(num >= 0);

	// If no keys.
	//////////////
	if(num == 0)
	{
		// Nothing to add.
		//////////////////
		if(addKeys)
			return;

		// Clear the list and cache.
		////////////////////////////
		TableClear(connection->globalWatchKeys[roomType]);
		piKeyCacheCleanse(peer);

		return;
	}

	assert(keys);

	if(!addKeys)
	{
		piRemoveExistingKeysData data;
		data.num = num;
		data.keys = keys;
		data.watchKeys = connection->globalWatchKeys[roomType];
		TableMapSafe(connection->globalWatchKeys[roomType], piRemoveExistingKeysMap, &data);
	}

	for(i = 0 ; i < num ; i++)
	{
		watchKey.key = goastrdup(keys[i]);
		TableEnter(connection->globalWatchKeys[roomType], &watchKey);
	}

	// Update them all.
	///////////////////
	if(ENTERING_ROOM || IN_ROOM)
		chatGetGlobalKeysA(connection->chat, ROOM, num, keys, piGetGlobalKeysCallback, peer, CHATFalse);
}

void piSetRoomWatchKeys
(
	PEER peer,
	RoomType roomType,
	int num,
	const char ** keys,
	PEERBool addKeys
)
{
	piWatchKey watchKey;
	int i;

	PEER_CONNECTION;

	ASSERT_ROOMTYPE(roomType);
	assert(num >= 0);

	// If no keys.
	//////////////
	if(num == 0)
	{
		// Nothing to add.
		//////////////////
		if(addKeys)
			return;

		// Clear the list and cache.
		////////////////////////////
		TableClear(connection->roomWatchKeys[roomType]);
		TableClear(connection->roomWatchCache[roomType]);

		return;
	}

	assert(keys);

	if(!addKeys)
	{
		piRemoveExistingKeysData data;
		data.num = num;
		data.keys = keys;
		data.watchKeys = connection->roomWatchKeys[roomType];
		TableMapSafe(connection->roomWatchKeys[roomType], piRemoveExistingKeysMap, &data);
	}

	for(i = 0 ; i < num ; i++)
	{
		watchKey.key = goastrdup(keys[i]);
		TableEnter(connection->roomWatchKeys[roomType], &watchKey);
	}

	// Update them all.
	///////////////////
	if(ENTERING_ROOM || IN_ROOM)
		chatGetChannelKeysA(connection->chat, ROOM, "*", num, keys, piGetRoomKeysCallback, peer, CHATFalse);
}

static PEERBool piKeyChanged
(
	PEER peer,
	const char * nick,
	const char * key,
	const char * value,
	HashTable watchKeys,
	HashTable watchCache,
	PEERBool inRoom,
	RoomType roomType
)
{
	piWatchKey watchKeyTemp;
	piCacheKey cacheKey;

	assert(key && key[0]);

	// We don't track anything for rooms.
	/////////////////////////////////////
	if(!nick || !nick[0])
		return PEERTrue;

	// No NULL values.
	//////////////////
	if(!value)
		value = "";

	// Is this a username (IP and Profile ID)?
	//////////////////////////////////////////
	if(strcasecmp(key, "username") == 0)
	{
		piPlayer * player;

		if(strcmp(nick, "(END)") == 0)
		{
			//
			// If <nick> is "(END)", this is the final message from
			// a GETCKEYS list. Call straight into the PlayerInfo
			// callback with zeroed arguments (09mar01/bgw).
			//
			assert(inRoom);
			piAddPlayerInfoCallback(peer, roomType, NULL, 0, 0);
			return PEERFalse;
		}

		// Get the player.
		//////////////////
		player = piGetPlayer(peer, nick);
		if(player && !player->gotIPAndProfileID)
		{
			int profileID;
			unsigned int IP;
			
			// Get the info.
			////////////////
			if(piDemangleUser(value, &IP, &profileID))
			{
				// Cache the info.
				//////////////////
				piSetPlayerIPAndProfileID(peer, nick, IP, profileID);
			}
		}

		// If this was in a room, call the callback.
		////////////////////////////////////////////
		if(inRoom)
		{
			if(player && player->gotIPAndProfileID)
				piAddPlayerInfoCallback(peer, roomType, nick, player->IP, player->profileID);
			else
				piAddPlayerInfoCallback(peer, roomType, nick, 0, 0);
		}
	}

	// Is this flags?
	/////////////////
	if(inRoom && strcasecmp(key, "b_flags") == 0)
		piSetPlayerRoomFlags(peer, nick, roomType, value);

	// Check if this is a watch key.
	////////////////////////////////
	watchKeyTemp.key = (char *)key;
	if(!TableLookup(watchKeys, &watchKeyTemp))
	{
		// Is it a broadcast key?
		/////////////////////////
		if(inRoom && (strncmp(key, "b_", 2) == 0))
			return PEERTrue;
		return PEERFalse;
	}

	// Cache it.
	////////////
	memset(&cacheKey, 0, sizeof(piCacheKey));
	cacheKey.nick = goastrdup(nick);
	cacheKey.key = goastrdup(key);
	cacheKey.value = goastrdup(value);
#ifdef GSI_UNICODE
	cacheKey.value_W = UTF8ToUCS2StringAlloc(cacheKey.value);
#endif
	TableEnter(watchCache, &cacheKey);

	return PEERTrue;
}

void piGlobalKeyChanged
(
	PEER peer,
	const char * nick,
	const char * key,
	const char * value
)
{
	PEER_CONNECTION;

	if(piKeyChanged(peer, nick, key, value, connection->globalWatchKeys[TitleRoom], connection->globalWatchCache, PEERFalse, (RoomType)0) ||
		piKeyChanged(peer, nick, key, value, connection->globalWatchKeys[GroupRoom], connection->globalWatchCache, PEERFalse, (RoomType)0) ||
		piKeyChanged(peer, nick, key, value, connection->globalWatchKeys[StagingRoom], connection->globalWatchCache, PEERFalse, (RoomType)0))
		piAddGlobalKeyChangedCallback(peer, nick, key, value);
}

void piRoomKeyChanged
(
	PEER peer,
	RoomType roomType,
	const char * nick,
	const char * key,
	const char * value
)
{
	PEER_CONNECTION;

	if(piKeyChanged(peer, nick, key, value, connection->roomWatchKeys[roomType], connection->roomWatchCache[roomType], PEERTrue, roomType))
		piAddRoomKeyChangedCallback(peer, roomType, nick, key, value);
}

static void piPlayerChangedNickMap
(
	void * elem,
	void * clientData
)
{
	piCacheKey * key = (piCacheKey *)elem;
	piPlayerChangedNickMapData * data = (piPlayerChangedNickMapData *)clientData;

	if(strcasecmp(key->nick, data->oldNick) == 0)
	{
		gsifree(key->nick);
		key->nick = goastrdup(data->newNick);
	}
}

void piKeyCachePlayerChangedNick
(
	PEER peer,
	const char * oldNick,
	const char * newNick
)
{
	piPlayerChangedNickMapData data;

	PEER_CONNECTION;

	data.oldNick = oldNick;
	data.newNick = newNick;

	TableMap(connection->globalWatchCache, piPlayerChangedNickMap, &data);
	TableMap(connection->roomWatchCache[0], piPlayerChangedNickMap, &data);
	TableMap(connection->roomWatchCache[1], piPlayerChangedNickMap, &data);
	TableMap(connection->roomWatchCache[2], piPlayerChangedNickMap, &data);
}

static void piSetupKeysMap
(
	void * elem,
	void * clientData
)
{
	piWatchKey * key = (piWatchKey *)elem;
	piSetupKeysMapData * data = (piSetupKeysMapData *)clientData;

	// Add the key to the list.
	///////////////////////////
	data->keys[data->next++] = key->key;
}

static void piKeyCacheRefresh
(
	PEER peer,
	RoomType roomType,
	const char * nick
)
{
	int num;
	piSetupKeysMapData data;
	PEERBool getIP;
	PEERBool getFlags;
	piWatchKey watchKey;

	PEER_CONNECTION;

	assert(IN_ROOM || ENTERING_ROOM);
	if(!IN_ROOM && !ENTERING_ROOM)
		return;

	// Get the IP if we're pinging this room.
	/////////////////////////////////////////
	if(!nick)
		getIP = (connection->pingRoom[roomType] || connection->alwaysRequestPlayerInfo)?PEERTrue:PEERFalse;
	else
		getIP = PEERFalse;

	// Check if we're already getting the IP.
	/////////////////////////////////////////
	if(getIP)
	{
		watchKey.key = "username";
		getIP = (PEERBool)(!TableLookup(connection->roomWatchKeys[roomType], &watchKey));
	}

	// Don't get flags on a player - he just joined, and will set them.
	///////////////////////////////////////////////////////////////////
	getFlags = (PEERBool)(nick == NULL);

	// Check for a b_flags watch key.
	/////////////////////////////////
	if(getFlags)
	{
		watchKey.key = "b_flags";
		getFlags = (PEERBool)(!(TableLookup(connection->globalWatchKeys[roomType], &watchKey) ||
			TableLookup(connection->roomWatchKeys[roomType], &watchKey)));
	}

	data.next = 0;
	num = TableCount(connection->globalWatchKeys[roomType]);
	if(num)
	{
		data.keys = (char **)gsimalloc(sizeof(char *) * num);
		if(!data.keys)
			return;
		TableMap(connection->globalWatchKeys[roomType], piSetupKeysMap, &data);
		assert(data.next == num);
		chatGetGlobalKeysA(connection->chat, nick?nick:ROOM, num, (const char **)data.keys, piGetGlobalKeysCallback, peer, CHATFalse);
		gsifree(data.keys);
	}

	if(!nick)
	{
		data.next = 0;
		num = TableCount(connection->roomWatchKeys[roomType]);
		if(getIP)
			num++;
		if(getFlags)
			num++;
		if(num)
		{
			data.keys = (char **)gsimalloc(sizeof(char *) * num);
			if(!data.keys)
				return;
			TableMap(connection->roomWatchKeys[roomType], piSetupKeysMap, &data);
			if(getIP)
				data.keys[data.next++] = "username";
			if(getFlags)
				data.keys[data.next++] = "b_flags";
			assert(data.next == num);
			chatGetChannelKeysA(connection->chat, ROOM, nick?nick:"*", num, (const char **)data.keys, piGetRoomKeysCallback, peer, CHATFalse);
			gsifree(data.keys);
		}
	}
}

void piKeyCacheRefreshPlayer
(
	PEER peer,
	RoomType roomType,
	const char * nick
)
{
	piKeyCacheRefresh(peer, roomType, nick);
}

void piKeyCacheRefreshRoom
(
	PEER peer,
	RoomType roomType
)
{
	piKeyCacheRefresh(peer, roomType, NULL);
}
