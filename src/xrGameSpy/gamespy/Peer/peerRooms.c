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
#include "peerRooms.h"
#include "peerPlayers.h"
#include "peerMangle.h"
#include "peerCallbacks.h"
#include "peerKeys.h"
#include "peerQR.h"
#include "peerHost.h"
#include "peerOperations.h"
#include "peerSB.h"

/**************
** FUNCTIONS **
**************/
PEERBool piRoomsInit
(
	PEER peer
)
{
	int roomType;

	PEER_CONNECTION;

	// Init rooms.
	//////////////
	for(roomType = 0 ; roomType < NumRooms ; roomType++)
	{
		if(connection->stayInTitleRoom && ((RoomType)roomType == TitleRoom))
			continue;

		ROOM[0] = '\0';
		NAME[0] = '\0';
		ENTERING_ROOM = PEERFalse;
		IN_ROOM = PEERFalse;
		connection->oldFlags[roomType] = 0;

#ifdef GSI_UNICODE
		ROOM_W[0] = '\0';
#endif
	}
	connection->groupID = 0;
	connection->titleRoomChannel[0] = '\0';

	return PEERTrue;
}

void piRoomsCleanup
(
	PEER peer
)
{
	int roomType;

	PEER_CONNECTION;

	// Check all the rooms.
	///////////////////////
	for(roomType = 0 ; roomType < NumRooms ; roomType++)
	{
		if(connection->stayInTitleRoom && ((RoomType)roomType == TitleRoom))
			continue;

		// Are we in or entering the room?
		//////////////////////////////////
		if(IN_ROOM || ENTERING_ROOM)
		{
			// Leave it.
			////////////
			piLeaveRoom(peer, (RoomType)roomType, NULL);
		}
		ROOM[0] = '\0';
		NAME[0] = '\0';
		ENTERING_ROOM = PEERFalse;
		IN_ROOM = PEERFalse;
	}
	connection->titleRoomChannel[0] = '\0';
}

void piStartedEnteringRoom
(
	PEER peer,
	RoomType roomType,
	const char * room
)
{
	PEER_CONNECTION;

	ASSERT_ROOMTYPE(roomType);
	assert(room);
	assert(room[0]);
	assert(strlen(room) < PI_ROOM_MAX_LEN);
	if(strlen(room) >= PI_ROOM_MAX_LEN)
		return;

	// Check that we're not entering, or in, this room.
	///////////////////////////////////////////////////
	assert(!ROOM[0]);
	assert(!ENTERING_ROOM);
	assert(!IN_ROOM);

	// Start entering.
	//////////////////
	strcpy(ROOM, room);
	ENTERING_ROOM = PEERTrue;
	connection->oldFlags[roomType] = 0;

#ifdef GSI_UNICODE
	UTF8ToUCS2String(ROOM, ROOM_W);
#endif
}


void piFinishedEnteringRoom
(
	PEER peer,
	RoomType roomType,
	const char * name
)
{
	PEER_CONNECTION;

	ASSERT_ROOMTYPE(roomType);

	if(!name)
		name = "";

	// Check that we're entering.
	/////////////////////////////
	assert(ROOM[0]);
	assert(ENTERING_ROOM);
	assert(!IN_ROOM);
	assert(strlen(name) < PI_ROOM_MAX_LEN);

	// We're in.
	////////////
	IN_ROOM = PEERTrue;
	ENTERING_ROOM = PEERFalse;
	strzcpy(NAME, name, PI_NAME_MAX_LEN);

#ifdef GSI_UNICODE
	UTF8ToUCS2String(NAME, NAME_W);
#endif

	// Set the flags.
	/////////////////
	piSetLocalFlags(peer);

	// Refresh the watch keys for this room.
	////////////////////////////////////////
	piKeyCacheRefreshRoom(peer, roomType);
}

void piLeaveRoom
(
	PEER peer,
	RoomType roomType,
	const char * reason
)
{
	PEER_CONNECTION;

	ASSERT_ROOMTYPE(roomType);

	// Check that we're in/entering this room.
	//////////////////////////////////////////
	if(!ENTERING_ROOM && !IN_ROOM)
		return;

	assert(ROOM[0]);

	// Are we entering?
	///////////////////
	if(ENTERING_ROOM)
	{
		// Cancel the operation.
		////////////////////////
		piCancelJoinOperation(peer, roomType);
	}

	// Leave the channel.
	/////////////////////
	if(connection->connected)
		chatLeaveChannelA(connection->chat, ROOM, reason);

	// Clear all the players out of this room.
	//////////////////////////////////////////
	piClearRoomPlayers(peer, roomType);

	// Reset in/entering states.
	////////////////////////////
	if(IN_ROOM)
	{
		assert(!ENTERING_ROOM);
		IN_ROOM = PEERFalse;
	}
	else
	{
		assert(ENTERING_ROOM);
		ENTERING_ROOM = PEERFalse;
	}

	// Clear the room/name.
	///////////////////////
	ROOM[0] = '\0';
	NAME[0] = '\0';

#ifdef GSI_UNICODE
	ROOM_W[0] = '\0';
#endif

	// Clear the flags.
	///////////////////
	connection->oldFlags[roomType] = 0;

	// Do roomtype specific stuff.
	//////////////////////////////
	if(roomType == StagingRoom)
	{
		// Stop hosting.
		////////////////
		piStopHosting(peer, PEERFalse);

		// Stop reporting as long as we're not playing.
		///////////////////////////////////////////////
		if(!connection->playing)
			piStopReporting(peer);

		// No host server.
		//////////////////
		piSBFreeHostServer(peer);

		// Turn ready off.
		//////////////////
		connection->ready = PEERFalse;

		// Clear passworded flag.
		/////////////////////////
		connection->passwordedRoom = PEERFalse;

		// Set the flags.
		/////////////////
		piSetLocalFlags(peer);
	}
	else if(roomType == GroupRoom)
	{
		// Clear the group ID.
		//////////////////////
		connection->groupID = 0;
	}

	// Cleanse the key cache.
	/////////////////////////
	piKeyCacheCleanse(peer);
}

PEERBool piRoomToType
(
	PEER peer,
	const char * room,
	RoomType * roomType
)
{
	int i;

	PEER_CONNECTION;

	for(i = 0 ; i < NumRooms ; i++)
	{
		if(strcasecmp(room, ROOMS[i]) == 0)
		{
			*roomType = (RoomType)i;
			return PEERTrue;
		}
	}

	return PEERFalse;
}

void piSetLocalFlags
(
	PEER peer
)
{
	char buffer[NumRooms][128];
	char * titleRoom;
	char * groupRoom;
	char * stagingRoom;
	const char * key = "b_flags";
	int nFlags;

	PEER_CONNECTION;
	
	if(!connection->connected)
		return;

	if(connection->inRoom[TitleRoom] || connection->enteringRoom[TitleRoom])
		titleRoom = buffer[TitleRoom];
	else
		titleRoom = NULL;
	if(connection->inRoom[GroupRoom] || connection->enteringRoom[GroupRoom])
		groupRoom = buffer[GroupRoom];
	else
		groupRoom = NULL;
	if(connection->inRoom[StagingRoom] || connection->enteringRoom[StagingRoom])
		stagingRoom = buffer[StagingRoom];
	else
		stagingRoom = NULL;

	// Check for staging room.
	//////////////////////////
	if(connection->inRoom[StagingRoom])
	{
		if(titleRoom)
			*titleRoom++ = 's';
		if(groupRoom)
			*groupRoom++ = 's';
		*stagingRoom++ = 's';

		// Check for ready.
		///////////////////
		if(connection->ready)
			*stagingRoom++ = 'r';

		// Check for hosting.
		/////////////////////
		if(connection->hosting)
			*stagingRoom++ = 'h';
	}

	// Check for playing.
	/////////////////////
	if(connection->playing)
	{
		if(titleRoom)
			*titleRoom++ = 'g';
		if(groupRoom)
			*groupRoom++ = 'g';
		if(stagingRoom)
			*stagingRoom++ = 'g';
	}

	// Check for away.
	//////////////////
	if(connection->away)
	{
		if(titleRoom)
			*titleRoom++ = 'a';
		if(groupRoom)
			*groupRoom++ = 'a';
		if(stagingRoom)
			*stagingRoom++ = 'a';
	}

	// Cap it off.
	//////////////
	if(titleRoom)
	{
		*titleRoom = '\0';
		titleRoom = buffer[TitleRoom];
	}
	if(groupRoom)
	{
		*groupRoom = '\0';
		groupRoom = buffer[GroupRoom];
	}
	if(stagingRoom)
	{
		*stagingRoom = '\0';
		stagingRoom = buffer[StagingRoom];
	}

	// Set the keys.
	////////////////
	if(titleRoom)
	{
		nFlags = piParseFlags(titleRoom);
		if(nFlags != connection->oldFlags[TitleRoom])
		{
			chatSetChannelKeysA(connection->chat, connection->rooms[TitleRoom], connection->nick, 1, &key, (const char **)&titleRoom);
			connection->oldFlags[TitleRoom] = nFlags;
		}
	}
	if(groupRoom)
	{
		nFlags = piParseFlags(groupRoom);
		if(nFlags != connection->oldFlags[GroupRoom])
		{
			chatSetChannelKeysA(connection->chat, connection->rooms[GroupRoom], connection->nick, 1, &key, (const char **)&groupRoom);
			connection->oldFlags[GroupRoom] = nFlags;
		}
	}
	if(stagingRoom)
	{
		nFlags = piParseFlags(stagingRoom);
		if(nFlags != connection->oldFlags[StagingRoom])
		{
			chatSetChannelKeysA(connection->chat, connection->rooms[StagingRoom], connection->nick, 1, &key, (const char **)&stagingRoom);
			connection->oldFlags[StagingRoom] = nFlags;
		}
	}
}
