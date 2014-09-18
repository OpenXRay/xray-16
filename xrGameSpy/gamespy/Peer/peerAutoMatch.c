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
#include "peerAutoMatch.h"
#include "peerMangle.h"
#include "peerOperations.h"
#include "peerSB.h"
#include "peerQR.h"
#include "peerCallbacks.h"
#include "peerPlayers.h"
#include "peerRooms.h"

/**************
** FUNCTIONS **
**************/
static PEERBool piCreateAutoMatchRoom(PEER peer);

static void piCleanAutoMatch(PEER peer)
{
	PEER_CONNECTION;

	// free memory
	gsifree(connection->autoMatchFilter);

	// remove the operation
	piRemoveOperation(peer, connection->autoMatchOperation);
	
	connection->autoMatchOperation = NULL;
}

void piSetAutoMatchStatus(PEER peer, PEERAutoMatchStatus status)
{
	piOperation * operation;
	PEER_CONNECTION;

	// make sure there is an operation
	operation = connection->autoMatchOperation;
	assert(operation);
	if(!operation)
		return;

	// we need to create a room before we can really switch to PEERWaiting
	if((status == PEERWaiting) && !connection->inRoom[StagingRoom])
	{
		if(!piCreateAutoMatchRoom(peer))
			piSetAutoMatchStatus(peer, connection->autoMatchSBFailed?PEERFailed:PEERSearching);
		return;
	}

	// check if this is already the current status
	if(connection->autoMatchStatus != status)
	{
		// set the new status
		connection->autoMatchStatus = status;
		
		// add the callback
		piAddAutoMatchStatusCallback(peer);
	}

	// handle the status
	switch(status)
	{		
	case PEERFailed:
		// stop
		piSBStopListingAutoMatches(peer);
		piStopAutoMatchReporting(peer);
		piLeaveRoom(peer, StagingRoom, "");

		// clean
		piCleanAutoMatch(peer);
		break;

	case PEERSearching:
		// stop
		piStopAutoMatchReporting(peer);
		piLeaveRoom(peer, StagingRoom, "");

		// start
		if(!connection->autoMatchBrowsing)
		{
			if(!piSBStartListingAutoMatches(peer))
			{
				piSetAutoMatchStatus(peer, connection->autoMatchQRFailed?PEERFailed:PEERWaiting);
				return;
			}
		}
		break;

	case PEERWaiting:
		// start
		assert(connection->inRoom[StagingRoom]);
		if(!connection->autoMatchBrowsing && !connection->autoMatchSBFailed)			
			piSBStartListingAutoMatches(peer);

		if(!connection->autoMatchReporting)
		{
			if(!piStartAutoMatchReporting(peer))
			{
				piSetAutoMatchStatus(peer, connection->autoMatchSBFailed?PEERFailed:PEERSearching);
				return;
			}
		}
		break;

	case PEERStaging:
		// stop
		if(!connection->hosting)
		{
			piStopAutoMatchReporting(peer);
		}
		piSBStopListingAutoMatches(peer);

		// start
		if(connection->hosting && !connection->autoMatchReporting)
		{
			if(!piStartAutoMatchReporting(peer))
			{
				piSetAutoMatchStatus(peer, PEERSearching);
				return;
			}
		}
		break;

	case PEERReady:
		// start
		if (connection->hosting && !connection->autoMatchReporting)
			piStartAutoMatchReporting(peer);
		
		break;
	case PEERComplete:
		// stop
		piSBStopListingAutoMatches(peer);
		piStopAutoMatchReporting(peer);

		// clean
		piCleanAutoMatch(peer);
		break;
	}
}

void piStopAutoMatch(PEER peer)
{
	//PEERBool inRoom;

	PEER_CONNECTION;

	// make sure we're AutoMatching
	if(peerIsAutoMatching(peer))
	{
		// we don't want the status callback called
		if(connection->autoMatchOperation)
			connection->autoMatchOperation->callback = (PEERCBType)NULL;

		// trick it into thinking it's not in a staging room (if it is)
		//inRoom = connection->inRoom[StagingRoom];
		//connection->inRoom[StagingRoom] = PEERFalse;
		
		// cleanup
		piSetAutoMatchStatus(peer, PEERFailed);

		// reset the inRoom flag
		//connection->inRoom[StagingRoom] = inRoom;
	}
}

static void piJoinAutoMatchRoomCallback
(
	PEER peer,
	PEERBool success,
	PEERJoinResult result,
	RoomType roomType,
	void * param
)
{
	PEER_CONNECTION;

	// if we're the only one, or if there's no host, leave
	if(success && ((connection->numPlayers[StagingRoom] <= 1) || !piCountRoomOps(peer, StagingRoom, connection->nick)))
	{
		piLeaveRoom(peer, StagingRoom, "");
		success = PEERFalse;
	}

	// check if it succeeded
	if(success)
	{
		// we're now staging
		piSetAutoMatchStatus(peer, PEERStaging);

		// if we've got maxplayers, we're now Ready
		if((connection->autoMatchStatus == PEERStaging) && (connection->numPlayers[StagingRoom] >= connection->maxPlayers))
			piSetAutoMatchStatus(peer, PEERReady);
	}

	// if we were waiting, and this failed, restart waiting
	if(!success && (connection->autoMatchStatus == PEERWaiting))
		piSetAutoMatchStatus(peer, PEERWaiting);
		
	GSI_UNUSED(result);
	GSI_UNUSED(roomType);
	GSI_UNUSED(param);
}

PEERBool piJoinAutoMatchRoom(PEER peer, SBServer server)
{
	unsigned int publicIP;
	unsigned int privateIP;
	unsigned short privatePort;
	char room[PI_ROOM_MAX_LEN];

	PEER_CONNECTION;

	// Get the public and private IPs and ports.
	publicIP = SBServerGetPublicInetAddress(server);
	privateIP = SBServerGetPrivateInetAddress(server);
	if(SBServerHasPrivateAddress(server))
		privatePort = SBServerGetPrivateQueryPort(server);
	else
		privatePort = SBServerGetPublicQueryPort(server);

	if(!publicIP)
		return PEERFalse;

	// get the staging room.
	piMangleStagingRoom(room, connection->title, publicIP, privateIP, privatePort);

	// start the operation.
	if(!piNewJoinRoomOperation(peer, StagingRoom, room, NULL, piJoinAutoMatchRoomCallback, NULL, piGetNextID(peer)))
		return PEERFalse;

	// clone the server
	connection->hostServer = piSBCloneServer(server);

	return PEERTrue;
}

// GetChannelModeCallback used to get the channel modes 
// and fix the modes: Limit and OpsObeyChannelLimit
static void piAutoMatchGetChannelCallback(CHAT chat, CHATBool success, const gsi_char *channel, CHATChannelMode *mode, void *param)
{
	piConnection *connection = (piConnection *)param;
	if (success)
	{

		mode->Limit = connection->maxPlayers;
		mode->OpsObeyChannelLimit = CHATTrue;
		
		// Don't let ops bypass channel limit on the number of players in room
		chatSetChannelMode(chat, channel, mode);
	}
	else
	{
		// handle the failure
		connection->autoMatchQRFailed = PEERTrue;
		piSetAutoMatchStatus((PEER *)&connection, connection->autoMatchSBFailed?PEERFailed:PEERSearching);
	}
}

static void piCreateAutoMatchRoomCallback
(
	PEER peer,
	PEERBool success,
	PEERJoinResult result,
	RoomType roomType,
	void * param
)
{
	PEERAutoMatchStatus status;

	PEER_CONNECTION;

	if(success)
	{
		// set the status based on the number of people in the room
		if(connection->numPlayers[StagingRoom] <= 1)
		{
			status = PEERWaiting;

			// get the channel modes so we can set the limits on our staging channel
			chatGetChannelMode(connection->chat, peerGetRoomChannel(peer, StagingRoom), piAutoMatchGetChannelCallback, 
				(void *)connection, CHATFalse);
		}
		else if(connection->numPlayers[StagingRoom] >= connection->maxPlayers)
		{
			status = PEERReady;
		}
		else
		{
			status = PEERStaging;
		}

		// set the Waiting status
		piSetAutoMatchStatus(peer, status);
	}
	else
	{
		// handle the failure
		connection->autoMatchQRFailed = PEERTrue;
		piSetAutoMatchStatus(peer, connection->autoMatchSBFailed?PEERFailed:PEERSearching);
	}
	
	GSI_UNUSED(result);
	GSI_UNUSED(roomType);
	GSI_UNUSED(param);
}

static PEERBool piCreateAutoMatchRoom(PEER peer)
{
	piOperation * operation;

	PEER_CONNECTION;

	// get the AutoMatch operation
	operation = connection->autoMatchOperation;

	// start the operations
	if(!piNewCreateStagingRoomOperation(peer, connection->nick, "", connection->maxPlayers, operation->socket, operation->port, piCreateAutoMatchRoomCallback, NULL, piGetNextID(peer)))
	{
		connection->autoMatchQRFailed = PEERTrue;
		return PEERFalse;
	}

	return PEERTrue;
}
