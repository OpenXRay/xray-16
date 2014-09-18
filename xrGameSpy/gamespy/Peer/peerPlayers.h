/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEERPLAYERS_H_
#define _PEERPLAYERS_H_

/*************
** INCLUDES **
*************/
#include "peerMain.h"


#ifdef __cplusplus
extern "C" {
#endif


/************
** DEFINES **
************/
#define PI_PING_HISTORY_LEN         4

/**********
** TYPES **
**********/
typedef struct piPlayer
{
	char nick[PI_NICK_MAX_LEN];     // this has to be first
	PEERBool inRoom[NumRooms];      // true if in the room
	PEERBool local;                 // true for the local player

	unsigned int IP;                // the IP in network byte order
	int profileID;                  // gp profile id
	PEERBool gotIPAndProfileID;     // true if we have the IP and profile ID

	int flags[NumRooms];            // player's room flags

	// Ping stuff
	unsigned long lastPingSend;     // last time a ping was sent
	unsigned long lastPingRecv;     // last time a ping was received
	unsigned long lastXping;        // last time a xping was sent
	PEERBool waitingForPing;        // if true we're waiting for a ping to return
	int pingsReturned;              // number of pings returned
	int pingsLostConsecutive;       // number of pings lost in a row
	int pingAverage;                // the average ping time
	int pingHistory[PI_PING_HISTORY_LEN];  // history of pings
	int pingHistoryNum;             // the number of pings in the ping history
	int numPings;                   // number of pings for this player
	PEERBool xpingSent;             // if true, we already sent a xping with the current average
	PEERBool inPingRoom;            // true if the player is in a room getting pinged
	PEERBool inXpingRoom;           // true if the player is in a room getting xpinged
	PEERBool mustPing;              // if true, ping this player as soon as possible
	PEERBool pingOnce;              // a one-time ping has been triggered with peerPingPlayer
} piPlayer;

/**************
** FUNCTIONS **
**************/
PEERBool piPlayersInit(PEER peer);
void piPlayersCleanup(PEER peer);
piPlayer * piPlayerJoinedRoom(PEER peer, const char * nick, RoomType roomType, int mode);
void piPlayerLeftRoom(PEER peer, const char * nick, RoomType roomType);
void piPlayerChangedNick(PEER peer, const char * oldNick, const char * newNick);
void piClearRoomPlayers(PEER peer, RoomType roomType);
piPlayer * piGetPlayer(PEER peer, const char * nick);
typedef void (* piEnumRoomPlayersCallback)(PEER peer, RoomType roomType, piPlayer * player, int index, void * param);
void piEnumRoomPlayers(PEER peer, RoomType roomType, piEnumRoomPlayersCallback callback, void * param);
piPlayer * piFindPlayerByIP(PEER peer, unsigned int IP);
void piSetPlayerIPAndProfileID(PEER peer, const char * nick, unsigned int IP, int profileID);
int piParseFlags(const char * flags);
void piSetPlayerRoomFlags(PEER peer, const char * nick, RoomType roomType, const char * flags);
void piSetPlayerModeFlags(PEER peer, const char * nick, RoomType roomType, int mode);
PEERBool piIsPlayerVIP(piPlayer * player, RoomType roomType);
PEERBool piIsPlayerHost(piPlayer * player);
PEERBool piIsPlayerOp(piPlayer * player);
piPlayer * piFindPlayerByIndex(PEER peer, RoomType roomType, int index);
piPlayer * piFindRoomHost(PEER peer, RoomType roomType);
piPlayer * piFindRoomOp(PEER peer, RoomType roomType);
int piCountRoomOps(PEER peer, RoomType roomType, const char * exclude);

#ifdef __cplusplus
}
#endif

#endif
