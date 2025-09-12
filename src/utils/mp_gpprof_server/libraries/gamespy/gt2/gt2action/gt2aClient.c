/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#include <stdlib.h>
#include <string.h>
#include "gt2aMain.h"
#include "gt2aClient.h"
#include "gt2aParse.h"
#include "gt2aDisplay.h"
#include "gt2aSound.h"

#define CLIENT_THINK_TIME        30

static GT2Socket Socket;                   // The socket used to connect to the server
static GT2Connection Connection;         // The connection to the server.
float localRotation;             // The local rotation, >=0, <360.
char serverAddress[128];         // Address of the server.
int localMotion;                 // STILL, FORWARD, BACKWARD.
int localTurning;                // STILL, LEFT, RIGHT.
Player players[MAX_PLAYERS];     // The list of players.
GT2Bool connected;                // True once we received the start message.
int localIndex = -1;             // The local player's index into the players table.
unsigned long lastServerUpdate;  // The last time we received an update from the server.
char localNick[MAX_NICK] = "Player";   // The local player's nick.
CObject cObjects[MAX_OBJECTS];   // The list of objects.
UpdateInfo updateHistory[UPDATE_HISTORY_LEN]; // Time diff for past updates
int updateHistoryStart;          // The starting index of the history.
int ClientNumAsteroids;          // The number of asteroids we're holding.
static unsigned short nextServerUpdateID;  // The expected ID of the next update.
static unsigned short nextClientUpdateID;  // The ID of our next update.

// Stats.
/////////
int reliableBytesSentClient;
int reliableBytesReceivedClient;
int unreliableBytesSentClient;
int unreliableBytesReceivedClient;
int reliableMessagesSentClient;
int reliableMessagesReceivedClient;
int unreliableMessagesSentClient;
int unreliableMessagesReceivedClient;

static void ClientSocketErrorCallback
(
	GT2Socket socket
)
{
	printf("Server socket error\n");
	GSI_UNUSED(socket);
}

static void ClientConnectedCallback
(
	GT2Connection connection,
	GT2Result result,
	GT2Byte * message,
	int len
)
{
	if(result != GT2Success)
	{
		printf("Connection failed (%d", result);
		if(result == GT2Rejected)
			printf(": %s)\n", message);
		else
			printf(")\n");
		exit(1);
	}
	GSI_UNUSED(len);
	GSI_UNUSED(connection);
}

static void ClientReceivedCallback
(
	GT2Connection connection,
	GT2Byte * message,
	int len,
	GT2Bool reliable
)
{
	char buffer[MAX_NICK + 16];
	GTMessageType type;
	int rcode;

	// Check for no message.
	////////////////////////
	if(!message)
		return;

	// Get the message type.
	////////////////////////
	type = gtEncodedMessageType((char *)message);

	// New client?
	//////////////
	if(type == MSG_S_ADDCLIENT)
	{
		byte newPlayerIndex;
		Player * newPlayer;
		char nick[MAX_NICK];

		// Decode it.
		/////////////
		rcode = gtDecode(MSG_S_ADDCLIENT_STR, (char *)message, len,
			&newPlayerIndex,
			nick);
		if(rcode == -1)
			return;

		// Check the index.
		///////////////////
		assert((newPlayerIndex >= 0) && (newPlayerIndex < MAX_PLAYERS));
		if((newPlayerIndex < 0) || (newPlayerIndex >= MAX_PLAYERS))
			return;
		assert(!players[newPlayerIndex].used);

		// Add the client.
		//////////////////
		newPlayer = &players[newPlayerIndex];
		memset(newPlayer, 0, sizeof(Player));
		newPlayer->used = GT2True;

		// Set the nick.
		////////////////
		strncpy(newPlayer->nick, nick, MAX_NICK);
		newPlayer->nick[MAX_NICK - 1] = '\0';

		// Display a join message.
		//////////////////////////
		sprintf(buffer, "%s joined", newPlayer->nick);
		DisplayChat(buffer);
	}
	// Delete client?
	/////////////////
	else if(type == MSG_S_DELCLIENT)
	{
		byte delPlayerIndex;

		// Decode it.
		/////////////
		rcode = gtDecode(MSG_S_DELCLIENT_STR, (char *)message, len,
			&delPlayerIndex);
		if(rcode == -1)
			return;

		// Check the index.
		///////////////////
		assert((delPlayerIndex >= 0) && (delPlayerIndex < MAX_PLAYERS));
		if((delPlayerIndex < 0) || (delPlayerIndex >= MAX_PLAYERS))
			return;
		assert(players[delPlayerIndex].used);

		// Display a part message.
		//////////////////////////
		sprintf(buffer, "%s left", players[delPlayerIndex].nick);
		DisplayChat(buffer);

		// Delete the client.
		/////////////////////
		players[delPlayerIndex].used = GT2False;
	}
	// Connection attempt finished?
	///////////////////////////////
	else if(type == MSG_S_START)
	{
		byte index;

		// Decode it.
		/////////////
		rcode = gtDecode(MSG_S_START_STR, (char *)message, len,
			&index);
		if(rcode == -1)
			return;

		// Check the index.
		///////////////////
		assert((index >= 0) && (index < MAX_PLAYERS));
		if((index < 0) || (index >= MAX_PLAYERS))
			return;

		// Set the local index.
		///////////////////////
		localIndex = index;

		// We finished connecting.
		//////////////////////////
		connected = GT2True;
	}
	// Server update?
	/////////////////
	else if(type == MSG_S_UPDATE)
	{
		Player * player;
		CObject * object;
		byte updatedClients;
		byte updatedObjects;
		byte index;
		unsigned short packedPosition[2];
		int score;
		byte forward;
		byte backward;
		byte right;
		byte left;
		byte dead;
		byte type;
		int time;
		int i;
		unsigned long now;
		unsigned long diff;
		unsigned short updateID;
		unsigned short packedRotation;

		// We got an update.
		////////////////////
		now = current_time();
		diff = (now - lastServerUpdate);
		lastServerUpdate = now;

		// Decode the header.
		/////////////////////
		rcode = gtDecode(MSG_S_UPDATE_STR, (char *)message, len,
			&updateID,
			&updatedClients,
			&updatedObjects);
		if(rcode == -1)
			return;
		message += rcode;
		len -= rcode;

		// Check for first update.
		//////////////////////////
		if(updateHistoryStart == -1)
		{
			nextServerUpdateID = (updateID + 1);
			updateHistoryStart = 0;
		}
		else
		{
			int numDropped;
			UpdateInfo * info;

			// Check for out of order.
			//////////////////////////
			if(updateID < nextServerUpdateID)
				return;

			// Fill in the dropped updates.
			///////////////////////////////
			numDropped = (updateID - nextServerUpdateID);
			nextServerUpdateID = (updateID + 1);

			// Fill in the dropped updates.
			///////////////////////////////
			while(numDropped--)
			{
				info = &updateHistory[updateHistoryStart++];
				updateHistoryStart %= UPDATE_HISTORY_LEN;

				info->diff = -1;
				info->len = 0;
			}

			// Fill in this update.
			///////////////////////
			info = &updateHistory[updateHistoryStart++];
			updateHistoryStart %= UPDATE_HISTORY_LEN;

			info->diff = diff;
			info->len = (len + rcode);
		}

		// Go through each updated client.
		//////////////////////////////////
		for(i = 0 ; i < updatedClients ; i++)
		{
			// Decode the update.
			/////////////////////
			rcode = gtDecodeNoType(MSG_S_UPDATE_CLIENT_STR, (char *)message, len,
				&index,
				&packedPosition[0],
				&packedPosition[1],
				&packedRotation,
				&score,
				&forward,
				&backward,
				&right,
				&left,
				&dead);
			if(rcode == -1)
				return;
			message += rcode;
			len -= rcode;

			// Check the index.
			///////////////////
			assert(index >= 0);
			assert(index < MAX_PLAYERS);
			if((index < 0) || (index >= MAX_PLAYERS))
				return;

			// Cache the player.
			////////////////////
			player = &players[index];

			// Copy the values in.
			//////////////////////
			SetV2f(
				player->position,
				UnsignedShortToPosition(packedPosition[0]),
				UnsignedShortToPosition(packedPosition[1]));
			player->rotation = UnsignedShortToRotation(packedRotation);
			player->score = score;
			if(forward)
				player->motion = FORWARD;
			else if(backward)
				player->motion = BACKWARD;
			else
				player->motion = STILL;
			if(right)
				player->turning = RIGHT;
			else if(left)
				player->turning = LEFT;
			else
				player->turning = STILL;
			player->dead = dead;

			// Update the roll.
			///////////////////
			if(player->dead)
			{
				player->roll = 0;
			}
			else if(left)
			{
				if(player->roll > -1)
				{
					player->roll -= (diff / 1000.0);
					if(player->roll < -1)
						player->roll = -1;
				}
			}
			else if(right)
			{
				if(player->roll < 1)
				{
					player->roll += (diff / 1000.0);
					if(player->roll > 1)
						player->roll = 1;
				}
			}
			else if(player->roll > 0)
			{
				player->roll -= (diff / 1000.0);
				if(player->roll < 0)
					player->roll = 0;
			}
			else if(player->roll < 0)
			{
				player->roll += (diff / 1000.0);
				if(player->roll > 0)
					player->roll = 0;
			}
		}

		// Go through the objects.
		//////////////////////////
		for(i = 0 ; i < updatedObjects ; i++)
		{
			// Decode the object.
			/////////////////////
			rcode = gtDecodeNoType(MSG_S_UPDATE_OBJECT_STR, (char *)message, len,
				&type,
				&packedPosition[0],
				&packedPosition[1],
				&packedRotation,
				&time);
			if(rcode == -1)
				return;
			message += rcode;
			len -= rcode;

			// Cache the object.
			////////////////////
			object = &cObjects[i];
			object->used = GT2True;

			// Copy in the values.
			//////////////////////
			object->type = type;
			SetV2f(
				object->position,
				UnsignedShortToPosition(packedPosition[0]),
				UnsignedShortToPosition(packedPosition[1]));
			object->rotation = UnsignedShortToRotation(packedRotation);
			object->totalTime = time;
		}

		// Mark the rest of the objects as unused.
		//////////////////////////////////////////
		for( ; i < MAX_OBJECTS ; i++)
			cObjects[i].used = GT2False;
	}
	// Chat?
	////////
	else if(type == MSG_S_CHAT)
	{
		char buffer[256];

		// Decode it.
		/////////////
		rcode = gtDecode(MSG_S_CHAT_STR, (char *)message, len,
			&buffer);
		if(rcode == -1)
			return;

		// We got a chat message.
		/////////////////////////
		DisplayChat(buffer);
	}
	// Sound?
	/////////
	else if(type == MSG_S_SOUND)
	{
		byte sound;

		// Decode it.
		/////////////
		rcode = gtDecode(MSG_S_SOUND_STR, (char *)message, len,
			&sound);
		if(rcode == -1)
			return;

		// We got a sound event.
		////////////////////////
		PlaySoundEffect(sound);
	}
	// NumAsteroids?
	////////////////
	else if(type == MSG_S_NUMASTEROIDS)
	{
		byte total;

		// Decode it.
		/////////////
		rcode = gtDecode(MSG_S_NUMASTEROIDS_STR, (char *)message, len,
			&total);
		if(rcode == -1)
			return;

		// We got a new total.
		//////////////////////
		ClientNumAsteroids = total;
	}
	GSI_UNUSED(reliable);
	GSI_UNUSED(connection);
}

static void ClientClosedCallback
(
	GT2Connection connection,
	GT2CloseReason reason
)
{
	printf("Connection closed (%d)\n", reason);
	GSI_UNUSED(connection);
}

void SendUpdate
(
	void
)
{
	if(connected)
	{
		char buffer[64];
		int rcode;

		// Encode the messsage.
		///////////////////////
		rcode = gtEncode(MSG_C_UPDATE, MSG_C_UPDATE_STR, buffer, sizeof(buffer),
			nextClientUpdateID++,
			RotationToUnsignedShort(localRotation),
			localMotion == FORWARD ? 1 : 0,
			localMotion == BACKWARD ? 1 : 0,
			localTurning == RIGHT ? 1 : 0,
			localTurning == LEFT ? 1 : 0);
		if(rcode != -1)
		{
			// Send the message.
			////////////////////
			gt2Send(Connection, (GT2Byte *)buffer, rcode, GT2False);
		}
	}
}

void SendPress
(
	const char * value
)
{
	if(connected)
	{
		char buffer[32];
		int rcode;

		// Encode the message.
		//////////////////////
		rcode = gtEncode(MSG_C_PRESS, MSG_C_PRESS_STR, buffer, sizeof(buffer),
			value);
		if(rcode != -1)
		{
			// Send the message.
			////////////////////
			gt2Send(Connection, (GT2Byte *)buffer, rcode, GT2True);
		}
	}
}

void SendChat
(
	const char * message
)
{
	if(connected)
	{
		char buffer[CHAT_MAX + 8];
		int rcode;

		// Encode the message.
		//////////////////////
		rcode = gtEncode(MSG_C_CHAT, MSG_C_CHAT_STR, buffer, sizeof(buffer),
			message);

		if(rcode != -1)
		{
			// Send the message.
			////////////////////
			gt2Send(Connection, (GT2Byte *)buffer, rcode, GT2True);
		}
	}
}

void ClientThink
(
	unsigned long now
)
{
	static unsigned long lastUpdate;
	static unsigned long lastTurn;
	unsigned long diff;

	// Think.
	/////////
	gt2Think(Socket);

	// Are we connected?
	////////////////////
	if(!connected)
		return;

	// For the first update, just set the time.
	///////////////////////////////////////////
	if(!lastUpdate)
	{
		lastUpdate = now;
		return;
	}

	// How long since the last update?
	//////////////////////////////////
	diff = (now - lastUpdate);

	// Check for an update.
	///////////////////////
	if(diff >= CLIENT_THINK_TIME)
	{
		// Send an update.
		//////////////////
		SendUpdate();

		// Update the last send time.
		/////////////////////////////
		lastUpdate = now;
	}

	// How long since the last turn?
	////////////////////////////////
	diff = (now - lastTurn);

	// Update our rotation.
	///////////////////////
	if((localIndex != -1) && !players[localIndex].dead)
		localRotation = ComputeNewRotation(localRotation, localTurning, diff, PLAYER_TURN_SPEED);

	// Update the last turn time.
	/////////////////////////////
	lastTurn = now;
}

static void ClientSendMonitor
(
	GT2Connection connection,
	int filterID,
	const GT2Byte * message,
	int len,
	GT2Bool reliable
)
{
	// Update stats.
	////////////////
	if(reliable)
	{
		reliableBytesSentClient += len;
		reliableMessagesSentClient++;
	}
	else
	{
		unreliableBytesSentClient += len;
		unreliableMessagesSentClient++;
	}

	// We're done with the message.
	///////////////////////////////
	gt2FilteredSend(connection, filterID, message, len, reliable);
}

static void ClientReceiveMonitor
(
	GT2Connection connection,
	int filterID,
	GT2Byte * message,
	int len,
	GT2Bool reliable
)
{
	// Update stats.
	////////////////
	if(reliable)
	{
		reliableBytesReceivedClient += len;
		reliableMessagesReceivedClient++;
	}
	else
	{
		unreliableBytesReceivedClient += len;
		unreliableMessagesReceivedClient++;
	}

	// We're done with the message.
	///////////////////////////////
	gt2FilteredReceive(connection, filterID, message, len, reliable);
}

GT2Bool InitializeClient
(
	void
)
{
	GT2ConnectionCallbacks connectionCallbacks;
	GT2Result result;
	char buffer[256];
	int rcode;

	// Setup callback structs.
	//////////////////////////
	memset(&connectionCallbacks, 0, sizeof(GT2ConnectionCallbacks));
	connectionCallbacks.connected = ClientConnectedCallback;
	connectionCallbacks.received = ClientReceivedCallback;
	connectionCallbacks.closed = ClientClosedCallback;

	// Init stuff.
	//////////////
	lastServerUpdate = 0;
	updateHistoryStart = -1;
	nextServerUpdateID = 0;
	nextClientUpdateID = 0;

	// If there was no server, we're connecting to ourselves.
	/////////////////////////////////////////////////////////
	if(!serverAddress[0])
		strcpy(serverAddress, "127.0.0.1" PORT_STRING);

	// Create the socket.
	/////////////////////
	result = gt2CreateSocket(&Socket, NULL, 0, 0, ClientSocketErrorCallback);
	if(result != GT2Success)
		return GT2False;

	// Setup the initial data.
	//////////////////////////
	rcode = gtEncode(MSG_C_INITIAL, MSG_C_INITIAL_STR, buffer, sizeof(buffer),
		localNick);
	if(rcode == -1)
		return GT2False;

	// Connect.
	///////////
	result = gt2Connect(Socket, &Connection, serverAddress, (GT2Byte *)buffer, rcode, 0, &connectionCallbacks, GT2False);
	if(result != GT2Success)
		return GT2False;

	// Add our traffic monitors.
	////////////////////////////
	gt2AddSendFilter(Connection, ClientSendMonitor);
	gt2AddReceiveFilter(Connection, ClientReceiveMonitor);

	return (Connection != NULL);
}