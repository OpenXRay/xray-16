/*
GameSpy GT2 SDK
GT2Action - sample app
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

*/

#include "gt2aMain.h"
#include "gt2aServer.h"
#include "gt2aParse.h"
#include "gt2aMath.h"
#include "gt2aSound.h"
#include "gt2aLogic.h"

#define SERVER_THINK_TIME      30

static GT2Socket Socket;
Client clients[MAX_CLIENTS];
static int numClients;
static GT2ConnectionCallbacks connectionCallbacks;
static unsigned short nextServerUpdateID;

static void Broadcast
(
	const char * message,
	int len,
	int exceptIndex,
	GT2Bool reliable
)
{
	int i;

	for(i = 0 ; i < MAX_CLIENTS ; i++)
		if(clients[i].used && (i != exceptIndex))
			gt2Send(clients[i].connection, (const GT2Byte *)message, len, reliable);
}

void BroadcastText
(
	const char * message,
	int exceptIndex,
	GT2Bool reliable
)
{
	static char messageCopy[CHAT_MAX];
	static char buffer[256];
	char * slash;
	int rcode;

	// Copy off the chat message.
	/////////////////////////////
	strncpy(messageCopy, message, CHAT_MAX);
	messageCopy[CHAT_MAX - 1] = '\0';

	// Because of the way gt2a is sending messages
	// we need to change \ to /.
	//////////////////////////////////////////////
	while(slash = strchr(messageCopy, '\\'))
		*slash = '/';

	// Encode the message.
	//////////////////////
	rcode = gtEncode(MSG_S_CHAT, MSG_S_CHAT_STR, buffer, sizeof(buffer),
		messageCopy);
	if(rcode != -1)
	{
		// Send this chat message out to everyone.
		//////////////////////////////////////////
		Broadcast(buffer, rcode, -1, reliable);
	}
}

static int AddClient
(
	void
)
{
	int i;

	// Find an open slot.
	/////////////////////
	for(i = 0 ; i < MAX_CLIENTS ; i++)
		if(!clients[i].used)
			break;

	// Nothing open?
	////////////////
	if(i == MAX_CLIENTS)
		return -1;

	// Clear it.
	////////////
	memset(&clients[i], 0, sizeof(Client));
	clients[i].index = i;
	clients[i].used = GT2True;

	// One more client.
	///////////////////
	numClients++;
	assert(numClients <= MAX_CLIENTS);

	return i;
}

static void RemoveClient
(
	int clientIndex 
)
{
	// Not in use anymore.
	//////////////////////
	clients[clientIndex].used = GT2False;

	// One less client.
	///////////////////
	numClients--;
	assert(numClients >= 0);
}

static void SendAddClient
(
	int sendClientIndex,
	int newClientIndex
)
{
	char buffer[256];
	Client * sendClient;
	Client * newClient;
	int rcode;

	assert((sendClientIndex >= 0) && (sendClientIndex < MAX_CLIENTS));
	sendClient = &clients[sendClientIndex];
	assert(sendClient->used);
	assert(sendClient->connection);

	assert((newClientIndex >= 0) && (newClientIndex < MAX_CLIENTS));
	newClient = &clients[newClientIndex];
	assert(newClient->used);
	assert(newClient->connection);

	// Encode the message.
	/////////////////////
	rcode = gtEncode(MSG_S_ADDCLIENT, MSG_S_ADDCLIENT_STR, buffer, sizeof(buffer),
		newClientIndex,
		newClient->nick);
	if(rcode != -1)
	{
		// Send the message.
		////////////////////
		gt2Send(sendClient->connection, (const GT2Byte *)buffer, rcode, GT2True);
	}
}

static void SendDelClient
(
	int sendClient,
	int delClient
)
{
	char buffer[32];
	int rcode;

	assert((sendClient >= 0) && (sendClient < MAX_CLIENTS));
	assert(clients[sendClient].used);
	assert(clients[sendClient].connection);
	assert((delClient >= 0) && (delClient < MAX_CLIENTS));

	// Encode the message.
	/////////////////////
	rcode = gtEncode(MSG_S_DELCLIENT, MSG_S_DELCLIENT_STR, buffer, sizeof(buffer),
		delClient);
	if(rcode != -1)
	{
		// Send the message.
		////////////////////
		gt2Send(clients[sendClient].connection, (const GT2Byte *)buffer, rcode, GT2True);
	}
}

static void SendStart
(
	int sendClient
)
{
	char buffer[32];
	int rcode;

	assert((sendClient >= 0) && (sendClient < MAX_CLIENTS));
	assert(clients[sendClient].used);
	assert(clients[sendClient].connection);

	// Encode the message.
	/////////////////////
	rcode = gtEncode(MSG_S_START, MSG_S_START_STR, buffer, sizeof(buffer),
		sendClient);
	if(rcode != -1)
	{
		// Send the message.
		////////////////////
		gt2Send(clients[sendClient].connection, (const GT2Byte *)buffer, rcode, GT2True);
	}
}

void SendSound
(
	int sound,
	int sendClient
)
{
	char buffer[32];
	int rcode;

	assert(sendClient >= -1);
	assert(sendClient < MAX_CLIENTS);
#ifdef _DEBUG
	if(sendClient != -1)
	{
		assert(clients[sendClient].used);
		assert(clients[sendClient].connection);
	}
#endif

	// Encode the message.
	/////////////////////
	rcode = gtEncode(MSG_S_SOUND, MSG_S_SOUND_STR, buffer, sizeof(buffer),
		sound);
	if(rcode != -1)
	{
		// Send the message.
		////////////////////
		if(sendClient == -1)
			Broadcast(buffer, rcode, -1, GT2False);
		else
			gt2Send(clients[sendClient].connection, (const GT2Byte *)buffer, rcode, GT2False);
	}
}

void SendNumAsteroids
(
	int total,
	int sendClient
)
{
	char buffer[32];
	int rcode;

	assert(sendClient >= -1);
	assert(sendClient < MAX_CLIENTS);
	assert(clients[sendClient].used);
	assert(clients[sendClient].connection);
	assert(total >= 0);
	assert(total <= PLAYER_MAX_ASTEROIDS);

	// Encode the message.
	/////////////////////
	rcode = gtEncode(MSG_S_NUMASTEROIDS, MSG_S_NUMASTEROIDS_STR, buffer, sizeof(buffer),
		(byte)total);;
	if(rcode != -1)
	{
		// Send the message.
		////////////////////
		gt2Send(clients[sendClient].connection, (const GT2Byte *)buffer, rcode, GT2True);
	}
}

static void SendUpdates
(
	void
)
{
	int i;
	char * buffer;
	int len;
	static char update[1024 * 8];
	Client * client;
	SObject * object;
	unsigned long now;
	int rcode;

	// Get the current time.
	////////////////////////
	now = current_time();

	// Setup the encoding parameters.
	/////////////////////////////////
	buffer = update;
	len = sizeof(update);

	// First encode the header.
	///////////////////////////
	rcode = gtEncode(MSG_S_UPDATE, MSG_S_UPDATE_STR, buffer, len,
		nextServerUpdateID++,
		numClients,
		numObjects);
	if(rcode == -1)
		return;
	buffer += rcode;
	len -= rcode;

	// Encode each client.
	//////////////////////
	for(i = 0 ; i < MAX_CLIENTS ; i++)
	{
		if(clients[i].used)
		{
			client = &clients[i];

			rcode = gtEncodeNoType(MSG_S_UPDATE_CLIENT_STR, buffer, len,
				i,
				PositionToUnsignedShort(client->position[0]),
				PositionToUnsignedShort(client->position[1]),
				RotationToUnsignedShort(client->rotation),
				client->score,
				client->motion == FORWARD ? 1 : 0,
				client->motion == BACKWARD ? 1 : 0,
				client->turning == RIGHT ? 1 : 0,
				client->turning == LEFT ? 1 : 0,
				client->dead);
			if(rcode == -1)
				return;
			buffer += rcode;
			len -= rcode;
		}
	}

	// Encode each object.
	//////////////////////
	for(i = 0 ; i < MAX_OBJECTS ; i++)
	{
		if(sObjects[i].used)
		{
			object = &sObjects[i];

			rcode = gtEncodeNoType(MSG_S_UPDATE_OBJECT_STR, buffer, len,
				object->type,
				PositionToUnsignedShort(object->position[0]),
				PositionToUnsignedShort(object->position[1]),
				RotationToUnsignedShort(object->rotation),
				(now - object->startTime));
			if(rcode == -1)
				return;
			buffer += rcode;
			len -= rcode;
		}
	}

	// Send the update.
	///////////////////
	Broadcast(update, sizeof(update) - len, -1, GT2False);
}

static void ServerReceivedCallback
(
	GT2Connection connection,
	GT2Byte * message,
	int len,
	GT2Bool reliable
)
{
	int clientIndex;
	Client * client;
	GTMessageType type;
	int rcode;

	// Check for no message.
	////////////////////////
	if(!message)
		return;

	// Which client was this?
	/////////////////////////
	clientIndex = (int)gt2GetConnectionData(connection);
	assert((clientIndex >= 0) && (clientIndex < MAX_CLIENTS));
	if((clientIndex < 0) || (clientIndex >= MAX_CLIENTS))
		return;
	assert(clients[clientIndex].used);
	if(!clients[clientIndex].used)
		return;
	client = &clients[clientIndex];

	// Get the message type.
	////////////////////////
	type = gtEncodedMessageType((char *)message);

	// Update?
	//////////
	if(type == MSG_C_UPDATE)
	{
		unsigned short updateID;
		unsigned long now;
		byte forward;
		byte backward;
		byte right;
		byte left;
		unsigned short packedRotation;

		// Get the time we received it.
		///////////////////////////////
		now = current_time();

		// Decode it.
		/////////////
		rcode = gtDecode(MSG_C_UPDATE_STR, (char *)message, len,
			&updateID,
			&packedRotation,
			&forward,
			&backward,
			&right,
			&left);
		if(rcode == -1)
			return;

		// Set the rotation.
		////////////////////
		client->rotation = UnsignedShortToRotation(packedRotation);

		// Interpret the motion & turning bits.
		///////////////////////////////////////
		if(forward)
			client->motion = FORWARD;
		else if(backward)
			client->motion = BACKWARD;
		else
			client->motion = STILL;
		if(right)
			client->turning = RIGHT;
		else if(left)
			client->turning = LEFT;
		else
			client->turning = STILL;

		// Check if we should move the client.
		//////////////////////////////////////
		if(client->lastUpdate && !client->dead && client->motion)
		{
			unsigned long diff;

			// Get how long since the last update.
			//////////////////////////////////////
			diff = (now - client->lastUpdate);

			// Compute the new position.
			////////////////////////////
			ComputeNewPosition(
				client->position,
				client->position,
				client->motion,
				client->rotation,
				diff,
				PLAYER_SPEED,
				GT2True);
		}

		// Set the last update time.
		////////////////////////////
		client->lastUpdate = now;
	}
	// Press?
	/////////
	else if(type == MSG_C_PRESS)
	{
		unsigned long now;
		char press[64];

		// A dead client can't fire.
		////////////////////////////
		if(client->dead)
			return;

		// Get the current time.
		////////////////////////
		now = current_time();

		// Is this too soon?
		////////////////////
		if((now - client->lastPress) < PRESS_TIME)
			return;

		// Decode it.
		/////////////
		rcode = gtDecode(MSG_C_PRESS_STR, (char *)message, len,
			press);
		if(rcode == -1)
			return;

		// The last press.
		//////////////////
		client->lastPress = now;

		// Do the press.
		////////////////
		ClientPress(clientIndex, press);
	}
	// Chat?
	////////
	else if(type == MSG_C_CHAT)
	{
		char buffer[MAX_NICK + CHAT_MAX + 16];

		// Start forming the outgoing chat message.
		///////////////////////////////////////////
		sprintf(buffer, "%s: ", client->nick);

		// Decode the incoming message.
		///////////////////////////////
		rcode = gtDecode(MSG_C_CHAT_STR, (char *)message, len,
			buffer + strlen(buffer));
		if(rcode == -1)
			return;

		// Send this chat message out to everyone.
		//////////////////////////////////////////
		BroadcastText(buffer, -1, GT2True);
	}
}
 
static void ServerClosedCallback
(
	GT2Connection connection,
	GT2CloseReason reason
)
{
	int clientIndex;
	int i;
	SObject * object;

	// Which client was this?
	/////////////////////////
	clientIndex = (int)gt2GetConnectionData(connection);
	assert((clientIndex >= 0) && (clientIndex < MAX_CLIENTS));
	if((clientIndex < 0) || (clientIndex >= MAX_CLIENTS))
		return;
	assert(clients[clientIndex].used);
	if(!clients[clientIndex].used)
		return;

	// Remove this client.
	//////////////////////
	RemoveClient(clientIndex);

	// Send the remove client message.
	//////////////////////////////////
	for(i = 0 ; i < MAX_CLIENTS ; i++)
		if(clients[i].used)
			SendDelClient(i, clientIndex);

	// Mark all owned objects as not owned.
	///////////////////////////////////////
	for(i = 0 ; i < MAX_OBJECTS ; i++)
	{
		object = &sObjects[i];
		if(object->used && (object->owner == clientIndex))
			object->owner = -1;
	}
}

static void ServerConnectAttemptCallback
(
	GT2Socket socket,
	GT2Connection connection,
	unsigned int ip,
	unsigned short port,
	int latency,
	GT2Byte * message,
	int len
)
{
	int newClientIndex;
	Client * newClient;
	int i;
	char nick[MAX_NICK];
	int rcode;

	// Make sure we got a message.
	//////////////////////////////
	if(!message)
	{
		gt2Reject(connection, (const GT2Byte *)"No initial data received", -1);
		return;
	}

	// Check the message type.
	//////////////////////////
	if(gtEncodedMessageType((char *)message) != MSG_C_INITIAL)
	{
		gt2Reject(connection, (const GT2Byte *)"Bad message type", -1);
		return;
	}

	// Decode the data.
	///////////////////
	rcode = gtDecode(MSG_C_INITIAL_STR, (char *)message, len,
		nick);
	if(rcode == -1)
	{
		gt2Reject(connection, (const GT2Byte *)"Bad message data", -1);
		return;
	}

	// Add the client to the list.
	//////////////////////////////
	newClientIndex = AddClient();
	if(newClientIndex == -1)
	{
		gt2Reject(connection, (const GT2Byte *)"Server full", -1);
		return;
	}
	newClient = &clients[newClientIndex];

	// Accept the connection.
	/////////////////////////
	if(!gt2Accept(connection, &connectionCallbacks))
	{
		// Remove the client.
		/////////////////////
		RemoveClient(newClientIndex);
		return;
	}

	// Set the connection's data to its index.
	//////////////////////////////////////////
	gt2SetConnectionData(connection, (void *)newClientIndex);

	// Set some properties for the new client.
	//////////////////////////////////////////
	newClient->connection = connection;
	strncpy(newClient->nick, nick, MAX_NICK);
	newClient->nick[MAX_NICK - 1] = '\0';

	// Spawn the client.
	////////////////////
	ClientSpawn(newClient);

	// Send new client message for the clients already connected.
	// Also send new client messages for new client to other clients.
	/////////////////////////////////////////////////////////////////
	for(i = 0 ; i < MAX_CLIENTS ; i++)
	{
		if(clients[i].used)
		{
			SendAddClient(i, newClientIndex);
			if(i != newClientIndex)
				SendAddClient(newClientIndex, i);
		}
	}

	// It has no asteroids.
	///////////////////////
	SendNumAsteroids(0, newClientIndex);

	// Tell the new client it can start.
	////////////////////////////////////
	SendStart(newClientIndex);
}

static void ServerSocketErrorCallback
(
	GT2Socket socket
)
{
	printf("Server socket error\n");
}

GT2Bool InitializeServer
(
	void
)
{
	GT2Result result;
	int i;

	// Setup callback structs.
	//////////////////////////
	memset(&connectionCallbacks, 0, sizeof(GT2ConnectionCallbacks));
	connectionCallbacks.received = ServerReceivedCallback;
	connectionCallbacks.closed = ServerClosedCallback;

	// Set client indices.
	//////////////////////
	for(i = 0 ; i < MAX_CLIENTS ; i++)
		clients[i].index = i;

	// Create the socket.
	/////////////////////
	result = gt2CreateSocket(&Socket, PORT_STRING, 0, 0, ServerSocketErrorCallback);
	if(result != GT2Success)
		return GT2False;

	// Listen.
	//////////
	gt2Listen(Socket, ServerConnectAttemptCallback);

	// Initialize the logic.
	////////////////////////
	InitializeLogic();

	return GT2True;
}

void ServerThink
(
	unsigned long now
)
{
	static GT2Bool firstTime = GT2True;
	static unsigned long last;
	unsigned long diff;

	// Think.
	/////////
	gt2Think(Socket);

	// The first time through just store the time.
	// This is so diff is accurate the second time through.
	///////////////////////////////////////////////////////
	if(firstTime)
	{
		firstTime = GT2False;
		last = now;
		return;
	}

	// How long since the last update?
	//////////////////////////////////
	diff = (now - last);

	// Check for an update.
	///////////////////////
	if(diff < SERVER_THINK_TIME)
		return;

	// New last update.
	///////////////////
	last = now;

	// Process objects.
	///////////////////
	ObjectsThink(now, diff);

	// Send updates.
	////////////////
	SendUpdates();
}