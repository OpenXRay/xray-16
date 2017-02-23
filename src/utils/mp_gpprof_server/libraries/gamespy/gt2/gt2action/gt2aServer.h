/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#ifndef _GT2ASERVER_H_
#define _GT2ASERVER_H_

#define MAX_CLIENTS            MAX_PLAYERS

typedef struct Client
{
	GT2Bool used;              // If this slot is in use or not.
	int index;                // This client's clientIndex.
	GT2Connection connection;  // The GT2Connection for this client.
	V2f position;             // The current position (0 <= x,y < MAP_MAX).
	float rotation;           // Client's view angle (0 <= rotation < 360).
	int motion;               // Client's current motion (STILL, FORWARD, BACKWARD).
	int turning;              // Client's current turning direction (STILL, LEFT, RIGHT).
	unsigned long lastUpdate; // Last update received from this client.
	char nick[MAX_NICK];      // The client's nick.
	int score;                // The client's score.
	GT2Bool dead;              // True if this client was killed and not respawned.
	unsigned long spawnTime;  // If dead, spawn at this time.
	unsigned long lastPress;  // When the last press event was received.
	int numAsteroids;         // The number of asteroids being held by this player.
} Client;

extern Client clients[MAX_CLIENTS];

GT2Bool InitializeServer
(
	void
);

void ServerThink
(
	unsigned long now
);

void SendSound
(
	int sound,
	int sendClient
);

void SendNumAsteroids
(
	int total,
	int sendClient
);

void BroadcastText
(
	const char * message,
	int exceptIndex,
	GT2Bool reliable
);

#endif