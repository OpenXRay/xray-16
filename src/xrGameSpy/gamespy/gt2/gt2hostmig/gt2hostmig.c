/******
gt2hostmig.c
GameSpy Transport 2 SDK
GameSpy Query & Reporting 2 SDK
  
Copyright 2000 GameSpy Industries, Inc

******

 This sample demonstrates the use of the Transport 2 SDK to do host migration.
 It also uses the Query & Reporting 2 SDK to report the host to the 
 Master Server.

 Please see the GameSpy Transport 2 SDK documentation for more 
 information

******/


/*************
** INCLUDES **
*************/
#include <stdio.h>
#include "../gt2.h"
// Needed for the QR2 portion of code
#include "../../qr2/qr2.h"

/************
** DEFINES **
************/
#define PORT              12345
#define MAX_CLIENTS       8
#define TIMEOUT           (30 * 1000)
#define SEND_TIME        500// (10 * 1000)

#define REPORT

/************
** GLOBALS **
************/
GT2Bool hosting;
GT2Bool quit;
GT2ConnectionCallbacks connectionCallbacks;
char messageKey[128];
char messageValue[128];
unsigned int backupHost;
int backupHostIndex;
GT2Bool gotBackupHost;

#ifdef REPORT
////////////////////////////////////////////////////
////////////////////////////////////////////////////
// Defines for QR2
#define QR2_GAME_VERSION	_T("2.00")
#define QR2_GAME_NAME		_T("gmtest")
#define QR2_MAX_PLAYERS		MAX_CLIENTS
#define QR2_BASE_PORT		11111
#define QR2_HOSTNAME        _T("My Host")
// end defines
#endif

GT2Connection clients[MAX_CLIENTS];
int numClients;

/**********************
** UTILITY FUNCTIONS **
**********************/
GT2Bool ParseMessage
(
	const char * message,
	int len
)
{
	const char * keyStart;
	const char * valueStart;
	int copyLen;
	int i;

	// Validate the message.
	for(i = 0 ; i < (len - 1) ; i++)
	{
		if(!message[i])
			return GT2False;
	}
	if(message[i])
		return GT2False;
	if(message[0] != '\\')
		return GT2False;

	// Find the key/value starts.
	keyStart = &message[1];
	valueStart = strchr(keyStart, '\\');
	if(!valueStart)
		return GT2False;
	valueStart++;

	// Copy in the key.
	copyLen = (valueStart - keyStart - 1);
	if(copyLen >= sizeof(messageKey))
		return GT2False;
	memcpy(messageKey, keyStart, copyLen);
	messageKey[copyLen] = '\0';

	// Copy in the value.
	if(strlen(valueStart) >= sizeof(messageValue))
		return GT2False;
	strcpy(messageValue, valueStart);

	return GT2True;
}

const char * GetConnectionName
(
	GT2Connection connection
)
{
	return gt2AddressToString(gt2GetRemoteIP(connection), 0, NULL);
}

void TellBackupHost
(
	int index
)
{
	char buffer[32];
	GT2Connection connection = clients[index];

	// Let him know who the backup host is.
	sprintf(buffer, "\\backuphost\\%s", gt2AddressToString(backupHost, 0, NULL));
	gt2Send(connection, (const GT2Byte *)buffer, -1, GT2True);
}

void NewBackupHost
(
	int index
)
{
	int i;
	GT2Connection connection = clients[index];

	// Let him know.
	gt2Send(connection, (const GT2Byte *)("\\backuphost\\"), -1, GT2True);
	backupHost = gt2GetRemoteIP(connection);
	backupHostIndex = index;

	// Let others know.
	for(i = 0 ; i < MAX_CLIENTS ; i++)
	{
		if(clients[i] && (i != index))
			TellBackupHost(i);
	}
}

/******************
** QR2 CALLBACKS **
******************/
#ifdef REPORT

void server_key_callback(int keyid, qr2_buffer_t outbuf, void *userdata)
{
	switch (keyid)
	{
	case HOSTNAME_KEY:
		qr2_buffer_add(outbuf, QR2_HOSTNAME);
		break;
	case GAMEVER_KEY:
		qr2_buffer_add(outbuf, QR2_GAME_VERSION);
		break;
	case HOSTPORT_KEY:
		qr2_buffer_add_int(outbuf, PORT);
		break;
	case NUMPLAYERS_KEY:
		qr2_buffer_add_int(outbuf, numClients);
		break;
	case MAXPLAYERS_KEY:
		qr2_buffer_add_int(outbuf, QR2_MAX_PLAYERS);
		break;

	default:
		qr2_buffer_add(outbuf, _T(""));
	}
	
	GSI_UNUSED(userdata);
}

// Called when a player key needs to be reported
void playerkey_callback(int keyid, int index, qr2_buffer_t outbuf, void *userdata)
{
	//check for valid index
	if (index >= numClients)
	{
		qr2_buffer_add(outbuf, _T(""));
		return;
	}
	switch (keyid)
	{
		case PLAYER__KEY:
			qr2_buffer_add(outbuf, GetConnectionName(clients[index]));
			break;
		default:
			qr2_buffer_add(outbuf, _T(""));
			break;		
	}
	
	GSI_UNUSED(userdata);
}

// Called when a team key needs to be reported
void teamkey_callback(int keyid, int index, qr2_buffer_t outbuf, void *userdata)
{
	qr2_buffer_add(outbuf, _T(""));

	GSI_UNUSED(userdata);
	GSI_UNUSED(index);
	GSI_UNUSED(keyid);
}	

// Called when we need to report the list of keys we report values for
void keylist_callback(qr2_key_type keytype, qr2_keybuffer_t keybuffer, void *userdata)
{
	//need to add all the keys we support
	switch (keytype)
	{
	case key_server:
		qr2_keybuffer_add(keybuffer, HOSTNAME_KEY);
		qr2_keybuffer_add(keybuffer, GAMEVER_KEY);
		qr2_keybuffer_add(keybuffer, HOSTPORT_KEY);
		qr2_keybuffer_add(keybuffer, NUMPLAYERS_KEY);
		qr2_keybuffer_add(keybuffer, MAXPLAYERS_KEY);
		break;
	case key_player:
		qr2_keybuffer_add(keybuffer, PLAYER__KEY);
		break;
	// no team keys are added since there is no team play in this game
	case key_team:
		break;
	}
	
	GSI_UNUSED(userdata);
}

// Called when we need to report the number of players and teams
int count_callback(qr2_key_type keytype, void *userdata)
{
	if (keytype == key_player)
		return numClients;
	else if (keytype == key_team)
		// 0 = Zero teams
		return 0;
	else
		return 0;
		
	GSI_UNUSED(userdata);
}

// Called if our registration with the GameSpy master server failed
void adderror_callback(qr2_error_t error, gsi_char *errmsg, void *userdata)
{
	_tprintf(_T("Error adding server: %d, %s\n"), error, errmsg);
	
	GSI_UNUSED(userdata);
}

#endif

/*********************
** SOCKET CALLBACKS **
*********************/
void ConnectAttemptCallback
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
	int i;

	// Do we have a spot open?
	for(i = 0 ; i < MAX_CLIENTS ; i++)
	{
		if(!clients[i])
			break;
	}
	if(i == MAX_CLIENTS)
	{
		// Nothing open, reject.
		gt2Reject(connection, (const GT2Byte *)("Server full"), -1);
		printf("Rejected client, server full\n");
		return;
	}

	// We have a spot.
	if(!gt2Accept(connection, &connectionCallbacks))
		return;

	// Add him to the list.
	clients[i] = connection;
	numClients++;

	// Store the index in the connection data.
	gt2SetConnectionData(connection, (void *)i);

	printf("Connection accepted from %s\n", GetConnectionName(connection));

	// Is this the only client?
	if(numClients == 1)
	{
		// He's the backup host.
		NewBackupHost(i);
	}
	else
	{
		// Let him know who the backup host is.
		TellBackupHost(i);
	}

	GSI_UNUSED(len);
	GSI_UNUSED(message);
	GSI_UNUSED(latency);
	GSI_UNUSED(port);
	GSI_UNUSED(ip);
	GSI_UNUSED(socket);
}

void SocketErrorCallback
(
	GT2Socket socket
)
{
	printf("socket error!\n");
	quit = GT2True;

	GSI_UNUSED(socket);
}

/*************************
** CONNECTION CALLBACKS **
*************************/
void ConnectedCallback
(
	GT2Connection connection,
	GT2Result result,
	GT2Byte * message,
	int len
)
{
	if(result != GT2Success)
	{
		printf("Connection failed: ");
		if(result == GT2OutOfMemory)
			printf("OutOfMemory\n");
		else if(result == GT2Rejected)
			printf("Rejected: %s\n", message);
		else if(result == GT2NetworkError)
			printf("NetworkError\n");
		else if(result == GT2AddressError)
			printf("AddressError\n");
		else if(result == GT2DuplicateAddress)
			printf("DuplicateAddress\n");
		else if(result == GT2TimedOut)
			printf("TimedOut\n");
		else if(result == GT2NegotiationError)
			printf("NegotiationError\n");
		else
			printf("Unknown error\n");

		quit = GT2True;
		return;
	}

	printf("Connection accepted\n");

	GSI_UNUSED(len);
	GSI_UNUSED(connection);
}

void ReceivedCallback
(
	GT2Connection connection,
	GT2Byte * message,
	int len,
	GT2Bool reliable
)
{
	if(!ParseMessage((const char *)message, len))
	{
		printf("Error parsing incoming message from %s\n", GetConnectionName(connection));
		return;
	}

	// Check for quit.
	if(strcasecmp(messageKey, "quit") == 0)
	{
		printf("Got remote quit: %s\n", messageValue);
		quit = GT2True;
		return;
	}

	// Are we the host?
	if(hosting)
	{
		// Check for echo.
		if(strcasecmp(messageKey, "echo") == 0)
		{
			// Send back the same.
			gt2Send(connection, message, len, reliable);
			return;
		}
	}
	else
	{
		// Check for echo reply.
		if(strcasecmp(messageKey, "echo") == 0)
			return;

		// Check for a backup-host.
		if(strcasecmp(messageKey, "backuphost") == 0)
		{
			printf("New backup host: %s\n", messageValue);

			// Get the backup IP.
			gt2StringToAddress(messageValue, &backupHost, NULL);
			gotBackupHost = GT2True;
			return;
		}
	}

	// This is an unknown message type.
	printf("Unknown message key: %s = %s\n", messageKey, messageValue);
}

void ClosedCallback
(
	GT2Connection connection,
	GT2CloseReason reason
)
{
	// Are we the host?
	if(hosting)
	{
		int i;
		
		// Get the index.
		i = (int)gt2GetConnectionData(connection);

		// Clear it from the list.
		clients[i] = NULL;
		numClients--;

		// Was this the backup host?
		if(i == backupHostIndex)
		{
			// Pick a new backup host.
			if(numClients)
			{
				for(i = 0 ; i < MAX_CLIENTS ; i++)
					if(clients[i])
					{
						NewBackupHost(i);
						break;
					}
			}
		}

		printf("Client connection with %s closed: %d\n", GetConnectionName(connection), reason);
	}
	else
	{
		printf("Connection closed\n");

		// Quit out of here, we'll check for a backup host in main().
		quit = GT2True;
	}
}

/**************
** FUNCTIONS **
**************/
GT2Bool StartHosting
(
	void
)
{
	qr2_error_t rcode;
	GT2Socket socket;
	GT2Result result;

	// Clear.
	quit = GT2False;
	hosting = GT2True;
	numClients = 0;
	memset(clients, 0, sizeof(GT2Connection) * MAX_CLIENTS);


	// create the socket
	result = gt2CreateSocket(&socket, gt2AddressToString(0, PORT, NULL), 0, 0, SocketErrorCallback);
	if(result != GT2Success)
	{
		printf("Failed to create socket (%d)\n", result);
		return GT2False;
	}

	// start listening
	printf("Starting to listen...\n");
	gt2Listen(socket, ConnectAttemptCallback);

#ifdef REPORT
	// Start reporting.
	printf("Starting reporting...\n");
	
	rcode = qr2_init(NULL, NULL, QR2_BASE_PORT, "gmtest", "HA6zkS", 1, 1, server_key_callback, playerkey_callback,
					 teamkey_callback, keylist_callback, count_callback, adderror_callback, NULL);
	
	if(rcode != e_qrnoerror)
		return GT2False;
#endif
	
	// Think.
	while(!quit)
	{
		gt2Think(socket);
		
#ifdef REPORT
		qr2_think(NULL);
#endif
		msleep(10);
	}

#ifdef REPORT
	qr2_shutdown(NULL);
#endif
	
	return GT2True;
}

GT2Bool StartConnecting
(
	const char * server
)
{
	GT2Connection connection;
	GT2Socket socket;
	GT2Result result;
	char address[256];
	unsigned long lastSendTime = 0;
	unsigned long now;

	// Clear.
	quit = GT2False;
	hosting = GT2False;
	gotBackupHost = GT2False;

	// Setup the address with the port.
	sprintf(address, "%s:%d", server, PORT);

	// create the socket
	result = gt2CreateSocket(&socket, NULL, 0, 0, SocketErrorCallback);
	if(result != GT2Success)
	{
		printf("Error creating the socket! (%d)\n", result);
		return GT2False;
	}

	// Connect to the host.
	printf("Connecting to %s (timeout after %d seconds)...\n", address, TIMEOUT / 1000);
	result = gt2Connect(socket, &connection, address, NULL, 0, TIMEOUT, &connectionCallbacks, GT2True);
	if(result != GT2Success)
	{
		printf("Error creating the connection! (%d)\n", result);
		return GT2False;
	}

	// Think.
	while(!quit)
	{
		// Check for sending a new message.
		now = current_time();
		if((now - lastSendTime) > SEND_TIME)
		{
			// Send an echo message.
			gt2Send(connection, (const GT2Byte *)("\\echo\\Hello."), -1, GT2False);

			// New last send time.
			lastSendTime = now;
		}

		// Think.
		gt2Think(socket);

		// Yield.
		msleep(10);
	}

	return GT2True;
}

int main
(
	int argc,
	char ** argv
)
{
	// Set the callbacks.
	memset(&connectionCallbacks, 0, sizeof(GT2ConnectionCallbacks));
	connectionCallbacks.connected = ConnectedCallback;
	connectionCallbacks.received = ReceivedCallback;
	connectionCallbacks.closed = ClosedCallback;

	// Host if no args.
	if(argc < 2)
	{
		if(!StartHosting())
			printf("Failed to start hosting\n");
	}
	else
	{
		// Do the initial connect.
		if(!StartConnecting(argv[1]))
		{
			printf("Failed to connect\n");
		}
		else
		{
			// Do host migration stuff.
			GT2Bool done = GT2False;
			do
			{
				// Do we have a backup host?
				if(gotBackupHost)
				{
					// Are we the backup host?
					if(!backupHost)
					{
						printf("Starting up backup host...\n");

						// Start hosting.
						if(!StartHosting())
						{
							printf("Failed to start backup host.\n");
							done = GT2True;
						}
					}
					else
					{
						const char * address = gt2AddressToString(backupHost, 0, NULL);

						printf("Connecting to backup host %s...\n", address);

						// Start connecting to the new host.
						if(!StartConnecting(address))
						{
							printf("Failed to connect to backup host.\n");
							done = GT2True;
						}
					}
				}
				else
				{
					printf("No backup host, quitting\n");
					done = GT2True;
				}
			}
			while(!done);
		}
	}

	getchar();

	return 0;
}