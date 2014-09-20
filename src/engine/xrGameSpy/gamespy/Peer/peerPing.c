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
#include <stdio.h>
#include "peerPlayers.h"
#include "peerPing.h"
#include "peerGlobalCallbacks.h"
#include "peerCallbacks.h"
#include "peerMangle.h"

/************
** DEFINES **
************/
#define PI_PING_TIMEOUT            5000
#define PI_PINGS_PER_SEC           25
#define PI_PING_INTERVAL           (1000 / PI_PINGS_PER_SEC)
#define PI_XPING_INTERVAL          2000
#define PI_XPING_PLAYER_INTERVAL   5000
#define PI_PINGER_PORT             13139
#define PI_MAX_PING_PLAYERS        12
#define PI_XPING_NUM_BUCKETS       32
#define PI_DONT_PING_FLAGS         (PEER_FLAG_PLAYING | PEER_FLAG_AWAY)
#define PI_MAX_XPING_NUM_PLAYERS   32

#define PEER_CONNECTION_DATA       piConnection * connection;\
	                               assert(data);\
                                   assert(data->peer);\
                                   connection = (piConnection *)data->peer;\
								   GSI_UNUSED(connection);


/**********
** TYPES **
**********/
typedef struct piXping
{
	char nicks[2][PI_NICK_MAX_LEN];
	int ping;
} piXping;

/**************
** FUNCTIONS **
**************/
static int piXpingTableHashFn
(
	const void *param,
	int numBuckets
)
{
	piXping * xping = (piXping *)param;
	int i;
	int c;
	const char * str;
	unsigned int hash = 0;
	const char * nicks[2];

	assert(xping);
	assert(xping->nicks[0][0]);
	assert(xping->nicks[1][0]);

	nicks[0] = xping->nicks[0];
	nicks[1] = xping->nicks[1];

	// Reverse the order if the second is smaller.
	//////////////////////////////////////////////
	if(strcmp(nicks[1], nicks[0]) < 0)
	{
		const char * temp = nicks[0];
		nicks[0] = nicks[1];
		nicks[1] = temp;
	}

	// Get the hash.
	////////////////
	for(i = 0 ; i < 2 ; i++)
	{
		str = nicks[i];
		while((c = *str++) != '\0')
			hash += (unsigned int)tolower(c);
		hash %= (unsigned int)numBuckets;
	}

	return (int)hash;
}

static int GS_STATIC_CALLBACK piXpingTableCompareFn
(
	const void *param1,
	const void *param2
)
{
	piXping * xping1 = (piXping *)param1;
	piXping * xping2 = (piXping *)param2;

	int i;
	int rcode;
	const char * nicks[2][2];

	assert(xping1);
	assert(xping1->nicks[0][0]);
	assert(xping1->nicks[1][0]);
	assert(xping2);
	assert(xping2->nicks[0][0]);
	assert(xping2->nicks[1][0]);

	nicks[0][0] = xping1->nicks[0];
	nicks[0][1] = xping1->nicks[1];
	nicks[1][0] = xping2->nicks[0];
	nicks[1][1] = xping2->nicks[1];

	// Reverse the order if the second is smaller.
	//////////////////////////////////////////////
	for(i = 0 ; i < 2 ; i++)
	{
		if(strcmp(nicks[i][1], nicks[i][0]) < 0)
		{
			const char * temp = nicks[i][0];
			nicks[i][0] = nicks[i][1];
			nicks[i][1] = temp;
		}
	}

	for(i = 0 ; i < 2 ; i++)
	{
		rcode = strcasecmp(nicks[0][i], nicks[1][i]);
		if(rcode != 0)
			return rcode;
	}

	return 0;
}

static void piXpingTableElementFreeFn
(
	void *param
)
{
	piXping * xping = (piXping *)param;
	assert(xping);
	assert(xping->nicks[0][0]);
	assert(xping->nicks[1][0]);
	GSI_UNUSED(xping);
}

static void piProcessPing
(
	PEER peer,
	piPlayer * player,
	int ping
)
{
	int i;
	int total;

	//PEER_CONNECTION;

	// One more returned.
	/////////////////////
	player->pingsReturned++;
	player->pingsLostConsecutive = 0;

	// We have one more ping for this player.
	/////////////////////////////////////////
	player->numPings++;

	//Update last ping received.
	////////////////////////////
	player->lastPingRecv = current_time();

	// Update the ping history.
	///////////////////////////
	if(player->pingHistoryNum > 0)
		memmove(player->pingHistory + 1, player->pingHistory, min(player->pingHistoryNum, PI_PING_HISTORY_LEN - 1) * sizeof(int));
	player->pingHistory[0] = ping;
	if(player->pingHistoryNum < PI_PING_HISTORY_LEN)
		player->pingHistoryNum++;

	// Recompute the ping average.
	//////////////////////////////
	total = 0;
	for(i = 0 ; i < player->pingHistoryNum ; i++)
		total += player->pingHistory[i];
	player->pingAverage = (total / player->pingHistoryNum);

	// Add a ping callback.
	///////////////////////
	piAddPingCallback(peer, player->nick, ping);

	// New ping, no xping yet.
	//////////////////////////
	player->xpingSent = PEERFalse;

	// If this is a one-timer, stop pinging.
	////////////////////////////////////////
	if(player->pingOnce)
		player->pingOnce = PEERFalse;
}

static void piPinged
(
	unsigned int IP,
	unsigned short port,
	int ping,
	const char * data,
	int len,
	PEER peer
)
{
	piPlayer * player;

	// Find the player.
	///////////////////
	player = piFindPlayerByIP(peer, IP);
	if(!player)
		return;

	// Process the ping.
	////////////////////
	piProcessPing(peer, player, ping);
	
	GSI_UNUSED(port);
	GSI_UNUSED(data);
	GSI_UNUSED(len);
}

PEERBool piPingInit
(
	PEER peer
)
{
	static PEERBool noPings[NumRooms];

	PEER_CONNECTION;

	// If there are no ping rooms, skip this.
	/////////////////////////////////////////
	if(memcmp(connection->pingRoom, noPings, sizeof(noPings)) == 0)
		return PEERTrue;

	// Init the xping table.
	////////////////////////
	connection->xpings = TableNew(sizeof(piXping), PI_XPING_NUM_BUCKETS, piXpingTableHashFn, piXpingTableCompareFn, piXpingTableElementFreeFn);
	if(!connection->xpings)
		return PEERFalse;

	// Init pinger.
	///////////////
	if(!pingerInit(NULL, PI_PINGER_PORT, piPinged, peer, NULL, NULL))
		return PEERFalse;

	// No pings yet.
	////////////////
	connection->lastPingTimeMod = 0;
	connection->lastXpingSend = 0;

	// Init the random number generator.
	////////////////////////////////////
	srand(current_time());

	// We're doing pings.
	/////////////////////
	connection->doPings = PEERTrue;

	return PEERTrue;
}

void piPingCleanup
(
	PEER peer
)
{
	PEER_CONNECTION;

	// Nothing to do if we weren't pinging.
	///////////////////////////////////////
	if(!connection->doPings)
		return;

	// If we're staying in the title room, get out.
	///////////////////////////////////////////////
	if(connection->stayInTitleRoom)
		return;

	// Clear timing stuff.
	//////////////////////
	connection->lastPingTimeMod = 0;
	connection->lastXpingSend = 0;

	// gsifree the xping table.
	////////////////////////
	if(connection->xpings)
		TableFree(connection->xpings);
	connection->xpings = NULL;

	// Cleanup pinger.
	//////////////////
	pingerShutdown();

	// Not doing pings.
	///////////////////
	connection->doPings = PEERFalse;
}

typedef struct piPingerReplyData
{
	PEER peer;
	unsigned int IP;
	int ping;
} piPingerReplyData;

static void piPingerReplyMapFn
(
	void *elem,
	void *clientdata
)
{
	piPlayer * player = (piPlayer *)elem;
	piPingerReplyData * data = (piPingerReplyData *)clientdata;

	PEER_CONNECTION_DATA;

	assert(player);

	// Check if waiting for a ping.
	///////////////////////////////
	if(!player->waitingForPing)
		return;

	// Check the IP.
	////////////////
	if(!player->gotIPAndProfileID || (player->IP != data->IP))
		return;

	// Not waiting anymore.
	///////////////////////
	player->waitingForPing = PEERFalse;

	// Check timeout.
	/////////////////
	if(data->ping == PINGER_TIMEOUT)
	{
		// One more lost.
		/////////////////
		player->pingsLostConsecutive++;

		// If a one-timer drops three in a row, give up.
		////////////////////////////////////////////////
		if(player->pingOnce && (player->pingsLostConsecutive >= 3))
		{
			player->pingsLostConsecutive = 0;
			player->pingOnce = PEERFalse;
		}
	}
	else
	{
		// Process the ping.
		////////////////////
		piProcessPing(data->peer, player, data->ping);
	}
}

static void piPingerReply
(
	unsigned int IP,
	unsigned short port,
	int ping,
	const char * pingData,
	int pingDataLen,
	PEER peer
)
{
	piPingerReplyData data;

	PEER_CONNECTION;

	// Find who sent the ping.
	//////////////////////////
	data.peer = peer;
	data.IP = IP;
	data.ping = ping;
	TableMap(connection->players, piPingerReplyMapFn, &data);
	
	GSI_UNUSED(port);
	GSI_UNUSED(pingData);
	GSI_UNUSED(pingDataLen);
	
}

static void piPingPlayer
(
	PEER peer,
	piPlayer * player
)
{
	assert(player->gotIPAndProfileID);

	// Make sure we're not already waiting.
	///////////////////////////////////////
	assert(!player->waitingForPing);
	if(player->waitingForPing)
		return;

	// Do the ping.
	///////////////
	pingerPing(player->IP, PI_PINGER_PORT, piPingerReply, peer, PINGERFalse, PI_PING_TIMEOUT);

	// Waiting for a ping to return.
	////////////////////////////////
	player->waitingForPing = PEERTrue;

	// Last ping send time.
	///////////////////////
	player->lastPingSend = current_time();

	// Clear must ping flag.
	////////////////////////
	player->mustPing = PEERFalse;
}

typedef struct piPickPingPlayersData
{
	PEER peer;
	piPlayer ** players;
	int max;
	int num;
} piPickPingPlayersData;

static void piPickPingPlayersMap
(
	void * elem,
	void * clientData
)
{
	piPlayer * player = (piPlayer *)elem;
	piPickPingPlayersData * data = (piPickPingPlayersData *)clientData;
	PEER peer = (PEER)data->peer;
	piPlayer * other;
	unsigned long now;
	unsigned long delay;
	int i;
	int j;

	PEER_CONNECTION;

	// Not pinging?
	///////////////
	if(!player->inPingRoom && !player->pingOnce)
		return;

	// Don't have IP?
	/////////////////
	if(!player->gotIPAndProfileID)
		return;

	// Waiting for a ping?
	//////////////////////
	if(player->waitingForPing)
		return;

	// Don't ping the local player.
	///////////////////////////////
	if(player->local)
		return;

	// Don't ping someone who's playing or away.
	////////////////////////////////////////////
	if((player->inRoom[TitleRoom] && (player->flags[TitleRoom] & PI_DONT_PING_FLAGS)) ||
	   (player->inRoom[GroupRoom] && (player->flags[GroupRoom] & PI_DONT_PING_FLAGS)) ||
	   (player->inRoom[StagingRoom] && (player->flags[StagingRoom] & PI_DONT_PING_FLAGS)))
	   return;

	// Check if we have to ping this player.
	////////////////////////////////////////
	if(!player->mustPing)
	{
		// Get the current time.
		////////////////////////
		now = current_time();

		// Slow pings for no response.
		//////////////////////////////
		if((player->pingsLostConsecutive >= 4) && ((now - player->lastPingSend) < 120000))
			return;

		// Pinged recently?
		///////////////////
		if(player->inRoom[StagingRoom])
			delay = 2000;
		else if(player->pingsReturned < 3)
			delay = 5000;
		else
			delay = 30000;
		if((now - player->lastPingSend) < delay)
			return;
		if((now - player->lastPingRecv) < (delay + 1500))
			return;
	}

	// Go through all the spots.
	////////////////////////////
	for(i = (data->max - 1) ; i >= 0 ; i--)
	{
		// Is it empty?
		///////////////
		if(!data->players[i])
			continue;

		// The "other" player.
		//////////////////////
		other = data->players[i];

		// Is this player higher priority than us?
		//////////////////////////////////////////
		if((!other->numPings && player->numPings) ||                        // Not pinged.
			(other->inRoom[StagingRoom] && !player->inRoom[StagingRoom]) || // In staging room.
			(piIsPlayerVIP(other, StagingRoom) && !piIsPlayerVIP(player, StagingRoom)) ||  // Op or voice in the same staging room.
			(strcasecmp(other->nick, player->nick) < 0))                    // Alphabetical.
		{
			break;
		}
	}

	// Bump it up one, so it's set to our spot.
	///////////////////////////////////////////
	i++;

	// Did we not find anything?
	////////////////////////////
	if(i == data->max)
		return;

	// Bump down the other player and all below.
	////////////////////////////////////////////
	for(j = (data->max - 1) ; j > i ; j--)
		data->players[j] = data->players[j - 1];

	// Set this player.
	///////////////////
	data->players[i] = player;

	// One more.
	////////////
	if(data->num < data->max)
		data->num++;
}

// Returns an array of pointers to players to ping,
// or NULL if there's noone to ping.
// The number of players in the array will be no
// larger than min(PI_MAX_PING_PLAYERS, numPings).
///////////////////////////////////////////////////
static piPlayer ** piPickPingPlayers
(
	PEER peer,
	int * numPings
)
{
	static piPlayer * players[PI_MAX_PING_PLAYERS];
	piPickPingPlayersData data;
	PEER_CONNECTION;

	// Check for no players.
	////////////////////////
	if(!connection->players || !(*numPings) || !TableCount(connection->players))
	{
		*numPings = 0;
		return NULL;
	}

	// Setup a list of players to ping.
	///////////////////////////////////
	data.peer = peer;
	data.players = players;
	data.max = min(PI_MAX_PING_PLAYERS, *numPings);
	data.num = 0;
	memset(players, 0, sizeof(piPlayer *) * data.max);
	TableMap(connection->players, piPickPingPlayersMap, &data);

	// Set number of pings.
	///////////////////////
	*numPings = data.num;

	// Check for noone to ping.
	///////////////////////////
	if(!data.players[0])
		return NULL;

	return data.players;
}

static void piXpingPlayer
(
	PEER peer,
	piPlayer * player
)
{
	int roomType;
	char message[(PI_NICK_MAX_LEN * 2) + 32];
	char encodedIP[11];

	PEER_CONNECTION;

	assert(player->inXpingRoom);
	if(!player->inXpingRoom)
		return;

	// Setup the message.
	/////////////////////
	piMangleIP(encodedIP, player->IP);
	sprintf(message, "%s %d", encodedIP, player->pingAverage);

	// Figure out which room to send the xping in.
	//////////////////////////////////////////////
	for(roomType = 0 ; roomType < NumRooms ; roomType++)
	{
		if(player->inRoom[roomType] && connection->xpingRoom[roomType])
		{
			if(connection->numPlayers[roomType] <= PI_MAX_XPING_NUM_PLAYERS)
			{
				assert(IN_ROOM || ENTERING_ROOM);
				if(IN_ROOM || ENTERING_ROOM)
					piSendChannelUTM(peer, ROOM, PI_UTM_XPING, message, PEERFalse);
			}
		}
	}

	// Sent it.
	///////////
	player->xpingSent = PEERTrue;
	player->lastXping = current_time();
	connection->lastXpingSend = (unsigned int)player->lastXping;
}

typedef struct piPickXpingPlayerData
{
	PEER peer;
	piPlayer * player;
} piPickXpingPlayerData;

static void piPickXpingPlayerMap
(
	void * elem,
	void * clientData
)
{
	piPlayer * player = (piPlayer *)elem;
	piPickXpingPlayerData * data = (piPickXpingPlayerData *)clientData;
	PEER peer = (PEER)data->peer;
	unsigned long now;

	PEER_CONNECTION;

	// Not xpinging?
	////////////////
	if(!player->inXpingRoom)
		return;

	// Don't xping the local player.
	////////////////////////////////
	if(player->local)
		return;

	// Don't xping if no pings received.
	///////////////////////////////////
	if(!player->numPings)
		return;

	// Don't xping with the same info.
	//////////////////////////////////
	if(player->xpingSent)
		return;

	// Don't xping too often.
	/////////////////////////
	now = current_time();
	if((now - player->lastXping) < PI_XPING_PLAYER_INTERVAL)
		return;

	// First player?
	////////////////
	if(!data->player)
	{
		data->player = player;
	}
	else if((now - player->lastXping) > (now - data->player->lastXping))
	{
		data->player = player;
	}
}

// Returns a player to ping, or NULL if there's noone to ping.
//////////////////////////////////////////////////////////////
static piPlayer * piPickXpingPlayer
(
	PEER peer
)
{
	piPickXpingPlayerData data;
	PEER_CONNECTION;

	// Check for no players.
	////////////////////////
	if(!connection->players || !TableCount(connection->players))
		return NULL;

	// Find someone near the top of the list.
	/////////////////////////////////////////
	data.peer = peer;
	data.player = NULL;
	TableMap(connection->players, piPickXpingPlayerMap, &data);

	return data.player;
}

void piPingThink
(
	PEER peer
)
{
	unsigned long now;
	int pingTimeMod;
	int numPings;
	piPlayer * player;
	piPlayer ** players;
	int i;

	PEER_CONNECTION;

	// Check if we're not doing pings.
	//////////////////////////////////
	if(!connection->doPings)
		return;
	
	// Don't do pings while playing!
	////////////////////////////////
	if(connection->playing)
		return;
	
	// Don't do pings while away.
	/////////////////////////////
	if(connection->away)
		return;

	// Get the current time.
	////////////////////////
	now = current_time();

	// Get the number of pings to send.
	///////////////////////////////////
	pingTimeMod = (int)(now / PI_PING_INTERVAL);
	if(connection->lastPingTimeMod)
		numPings = (pingTimeMod - connection->lastPingTimeMod);
	else
		numPings = 1;
	if(numPings)
		connection->lastPingTimeMod = pingTimeMod;

	// Send pings.
	//////////////
	players = piPickPingPlayers(peer, &numPings);
	if(players)
	{
		for(i = 0 ; (i < numPings) && players[i] ; i++)
			piPingPlayer(peer, players[i]);
	}

	// Check for sending a crossping.
	/////////////////////////////////
	if((now - connection->lastXpingSend) > PI_XPING_INTERVAL)
	{
		// Try and pick a player to crossping.
		//////////////////////////////////////
		player = piPickXpingPlayer(peer);
		if(player)
			piXpingPlayer(peer, player);
	}

	// Let pinger think.
	////////////////////
	pingerThink();
}

PEERBool piPingInitPlayer
(
	PEER peer,
	piPlayer * player
)
{
	int i;

	PEER_CONNECTION;

	assert(player);

	// Check if we're not doing pings.
	//////////////////////////////////
	if(!connection->doPings)
		return PEERTrue;

	player->lastPingSend = 0;
	player->lastPingRecv = 0;
	player->lastXping = 0;
	player->waitingForPing = PEERFalse;
	player->pingsReturned = 0;
	player->pingsLostConsecutive = 0;
	player->pingAverage = 0;
	for(i = 0 ; i < PI_PING_HISTORY_LEN ; i++)
		player->pingHistory[i] = 0;
	player->pingHistoryNum = 0;
	player->numPings = 0;
	player->xpingSent = PEERFalse;
	player->inPingRoom = PEERFalse;
	player->inXpingRoom = PEERFalse;
	player->mustPing = PEERFalse;
	player->pingOnce = PEERFalse;

	return PEERTrue;
}

void piPingPlayerJoinedRoom
(
	PEER peer,
	piPlayer * player,
	RoomType roomType
)
{
	PEER_CONNECTION;

	assert(player);
	ASSERT_ROOMTYPE(roomType);

	// Check if we're not doing pings.
	//////////////////////////////////
	if(!connection->doPings)
		return;

	// Is this a ping room?
	///////////////////////
	if(connection->pingRoom[roomType])
		player->inPingRoom = PEERTrue;

	// Is this an xping room?
	/////////////////////////
	if(connection->xpingRoom[roomType])
		player->inXpingRoom = PEERTrue;

	// Immediately ping players when they join a staging room.
	//////////////////////////////////////////////////////////
	if(roomType == StagingRoom)
		player->mustPing = PEERTrue;
}

static void piRemoveXping
(
	PEER peer,
	piXping * xping
)
{
	PEER_CONNECTION;
	
	assert(xping);

	TableRemove(connection->xpings, xping);
}

typedef struct piPingPlayerLeftRoomData
{
	PEER peer;
	const char * nick;
} piPingPlayerLeftRoomData;

static void piPingPlayerLeftRoomTableMapFn
(
	void *elem,
	void *clientdata
)
{
	piXping * xping = (piXping *)elem;
	piPingPlayerLeftRoomData * data = (piPingPlayerLeftRoomData *)clientdata;
	assert(xping);
	assert(data);

	// Check if this player is part of this xping.
	//////////////////////////////////////////////
	if((strcmp(xping->nicks[0], data->nick) == 0) || (strcmp(xping->nicks[1], data->nick) == 0))
		piRemoveXping(data->peer, xping);
}

void piPingPlayerLeftRoom
(
	PEER peer,
	piPlayer * player
)
{
	PEER_CONNECTION;

	assert(player);

	// Check if we're not doing pings.
	//////////////////////////////////
	if(!connection->doPings)
		return;

	// Was this player in a ping room?
	//////////////////////////////////
	if(player->inPingRoom)
	{
		int i;
		PEERBool inPingRoom = PEERFalse;

		// Is he still in a ping room?
		//////////////////////////////
		for(i = 0 ; i < NumRooms ; i++)
		{
			if(player->inRoom[i] && connection->pingRoom[i])
				inPingRoom = PEERTrue;
		}
		player->inPingRoom = inPingRoom;
	}

	// Was this player in an xping room?
	////////////////////////////////////
	if(player->inXpingRoom)
	{
		int i;
		PEERBool inXpingRoom = PEERFalse;

		// Is he still in a ping room?
		//////////////////////////////
		for(i = 0 ; i < NumRooms ; i++)
		{
			if(player->inRoom[i] && connection->xpingRoom[i])
				inXpingRoom = PEERTrue;
		}
		player->inXpingRoom = inXpingRoom;

		if(!player->inXpingRoom)
		{
			piPingPlayerLeftRoomData data;
			data.peer = peer;
			data.nick = player->nick;

			// Get rid of all this players xpings.
			//////////////////////////////////////
			TableMapSafe(connection->xpings, piPingPlayerLeftRoomTableMapFn, &data);
		}
	}
}

static piXping * piFindXping
(
	PEER peer,
	const char * nick1,
	const char * nick2
)
{
	piXping xpingMatch;

	PEER_CONNECTION;

	assert(nick1);
	assert(nick1[0]);
	assert(piGetPlayer(peer, nick1));
	assert(nick2);
	assert(nick2[0]);
	assert(piGetPlayer(peer, nick2));

	// Setup the xping match.
	/////////////////////////
	strzcpy(xpingMatch.nicks[0], nick1, PI_NICK_MAX_LEN);
	_strlwr(xpingMatch.nicks[0]);
	strzcpy(xpingMatch.nicks[1], nick2, PI_NICK_MAX_LEN);
	_strlwr(xpingMatch.nicks[0]);

	// Find the xping.
	//////////////////
	return (piXping *)TableLookup(connection->xpings, &xpingMatch);
}

static piXping * piAddXping
(
	PEER peer,
	const char * nick1,
	const char * nick2
)
{
	piXping xpingMatch;
	piXping * xping;

	PEER_CONNECTION;

	assert(nick1);
	assert(nick1[0]);
	assert(piGetPlayer(peer, nick1));
	assert(nick2);
	assert(nick2[0]);
	assert(piGetPlayer(peer, nick2));

	// Setup the one to add.
	////////////////////////
	xping = &xpingMatch;
	strzcpy(xping->nicks[0], nick1, PI_NICK_MAX_LEN);
	_strlwr(xping->nicks[0]);
	strzcpy(xping->nicks[1], nick2, PI_NICK_MAX_LEN);
	_strlwr(xping->nicks[1]);

	// Add it.
	//////////
	TableEnter(connection->xpings, xping);

	// Get it.
	//////////
	xping = (piXping *)TableLookup(connection->xpings, &xpingMatch);

	// Return it.
	/////////////
	return xping;
}

void piUpdateXping
(
	PEER peer,
	const char * nick1,
	const char * nick2,
	int ping
)
{
	piPlayer * player1;
	piPlayer * player2;
	piXping * xping;

	PEER_CONNECTION;

	assert(nick1);
	assert(nick1[0]);
	assert(piGetPlayer(peer, nick1));
	assert(nick2);
	assert(nick2[0]);
	assert(piGetPlayer(peer, nick2));
	assert(ping >= 0);

	// Check if we're not doing pings.
	//////////////////////////////////
	if(!connection->doPings)
		return;

	player1 = piGetPlayer(peer, nick1);
	if(!player1)
		return;
	if(!player1->inXpingRoom)
		return;
	player2 = piGetPlayer(peer, nick2);
	if(!player2)
		return;
	if(!player2->inXpingRoom)
		return;

	// Add it.
	//////////
	xping = piAddXping(peer, nick1, nick2);
	assert(xping);
	if(!xping)
		return;

	// Update.
	//////////
	xping->ping = ping;
}

PEERBool piGetXping
(
	PEER peer,
	const char * nick1,
	const char * nick2,
	int * ping
)
{
	piXping * xping;

	PEER_CONNECTION;

	assert(nick1);
	assert(nick1[0]);
	assert(piGetPlayer(peer, nick1));
	assert(nick2);
	assert(nick2[0]);
	assert(piGetPlayer(peer, nick2));
	assert(ping);

	// Check if we're not doing pings.
	//////////////////////////////////
	if(!connection->doPings)
		return PEERFalse;

	// Get the xping.
	/////////////////
	xping = piFindXping(peer, nick1, nick2);
	assert(xping);
	if(!xping)
		return PEERFalse;

	// Return the ping.
	///////////////////
	*ping = xping->ping;

	return PEERTrue;
}
