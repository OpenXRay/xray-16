/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#ifndef _GT2ACLIENT_H_
#define _GT2ACLIENT_H_

#include "gt2aMath.h"
#include "gt2aLogic.h"

#define UPDATE_HISTORY_LEN    250

typedef struct Player
{
	GT2Bool used;              // If this slot is in use or not.
	V2f position;             // The current position (0 <= x,y < MAP_MAX).
	float rotation;           // Client's view angle (0 <= rotation < 360).
	int motion;               // The client's current motion (STILL, FORWARD, BACKWARD).
	int turning;              // The client's current tunring direction (STILL, LEFT, RIGHT).
	char nick[MAX_NICK];      // The client's nick.
	int score;                // The client's score.
	GT2Bool dead;              // True if this client is currently dead.
	float roll;               // How much to roll them (-1->0->1).
} Player;

typedef struct CObject
{
	GT2Bool used;              // If this slot is in use or not.
	ObjectType type;          // The type of object.
	V2f position;             // The object's position.
	float rotation;           // The object's rotation.
	unsigned long totalTime;  // The amount of time this object has existed.
} CObject;

typedef struct UpdateInfo
{
	int diff;                 // Time since the last update was received.
	int len;                  // Length of the update, in bytes.
} UpdateInfo;

extern float localRotation;
extern char serverAddress[128];
extern int localMotion;
extern int localTurning;
extern Player players[MAX_PLAYERS];
extern GT2Bool connected;
extern int localIndex;
extern unsigned long lastServerUpdate;
extern char localNick[MAX_NICK];
extern CObject cObjects[MAX_OBJECTS];
extern UpdateInfo updateHistory[UPDATE_HISTORY_LEN];
extern int updateHistoryStart;
extern int ClientNumAsteroids;

// Stats.
/////////
extern int reliableBytesSentClient;
extern int reliableBytesReceivedClient;
extern int unreliableBytesSentClient;
extern int unreliableBytesReceivedClient;
extern int reliableMessagesSentClient;
extern int reliableMessagesReceivedClient;
extern int unreliableMessagesSentClient;
extern int unreliableMessagesReceivedClient;

GT2Bool InitializeClient
(
	void
);

void ClientThink
(
	unsigned long now
);

void SendPress
(
	const char * value
);

void SendChat
(
	const char * message
);

#endif