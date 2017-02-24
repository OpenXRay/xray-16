
#include "../gt2.h"
#include "../../darray.h"
#include <time.h>


#define STATS 1

GT2Socket Socket;
char RemoteAddress[256];
char LocalAddress[256];
GT2Bool Quit;
DArray ServerSockets;

#if STATS
time_t startTime;
int numConnectAttempts;
int connectResults[8];
int clientReliableMessagesSent;
int serverReliableMessagesSent;
int clientUnreliableMessagesSent;
int serverUnreliableMessagesSent;
int clientReliableBytesSent;
int serverReliableBytesSent;
int clientUnreliableBytesSent;
int serverUnreliableBytesSent;
int clientCloses;
int serverCloses;
#endif

void FreeServerSocket(void * elem)
{
	GT2Socket socket = *(GT2Socket *)elem;

	gt2CloseSocket(socket);
}

int ServerSocketCompare(const void * elem1, const void * elem2)
{
	GT2Socket socket1 = *(GT2Socket *)elem1;
	GT2Socket socket2 = *(GT2Socket *)elem2;

	if(socket1 == socket2)
		return 0;
	return 1;
}

void RemoveServerSocket(GT2Socket socket)
{
	int index;
	
	// find it first
	index = ArraySearch(ServerSockets, &socket, ServerSocketCompare, 0, 0);
	if(index != NOT_FOUND)
		ArrayDeleteAt(ServerSockets, index);
}

/* CLIENT CONNECTIONS */

void ClientReceivedCallback
(
	GT2Connection connection,
	GT2Byte * message,
	int len,
	GT2Bool reliable
)
{
	GT2Connection serverConnection;

	// The server connection for this client connection.
	////////////////////////////////////////////////////
	serverConnection = (GT2Connection)gt2GetConnectionData(connection);

	// Pass the data along.
	///////////////////////
	gt2Send(serverConnection, message, len, reliable);

#if STATS
	// Update stats.
	////////////////
	if(reliable)
	{
		clientReliableMessagesSent++;
		clientReliableBytesSent += len;
	}
	else
	{
		clientUnreliableMessagesSent++;
		clientUnreliableBytesSent += len;
	}
#endif
}

void ClientClosedCallback
(
	GT2Connection connection,
	GT2CloseReason reason
)
{
	GT2Connection serverConnection;

	// The server connection for this client connection.
	////////////////////////////////////////////////////
	serverConnection = (GT2Connection)gt2GetConnectionData(connection);
	if(serverConnection)
	{
		// We don't want them to try closing us....
		///////////////////////////////////////////
		gt2SetConnectionData(serverConnection, NULL);

		// Close the connection to the server.
		//////////////////////////////////////
		gt2CloseConnection(serverConnection);
	}

#if STATS
	// Update stats.
	////////////////
	if(reason == GT2LocalClose)
		clientCloses++;
#endif
}

GT2ConnectionCallbacks ClientConnectionCallbacks =
{
	NULL,
	ClientReceivedCallback,
	ClientClosedCallback
};

/* SERVER CONNECTIONS */

void ServerSocketErrorCallback
(
	GT2Socket socket
)
{
	printf("Server socket error\n");
	Quit = GT2True;

	GSI_UNUSED(socket);
}

void ServerConnectedCallback
(
	GT2Connection connection,
	GT2Result result,
	GT2Byte * message,
	int len
)
{
	GT2Connection clientConnection;

	// The client connection for this server connection.
	////////////////////////////////////////////////////
	clientConnection = (GT2Connection)gt2GetConnectionData(connection);

	// Check the result.
	////////////////////
	if(result == GT2Success)
	{
		// Accept it.
		/////////////
		if(!gt2Accept(clientConnection, &ClientConnectionCallbacks))
			gt2CloseConnection(connection);
	}
	else
	{
		if(result == GT2Rejected)
			gt2Reject(clientConnection, message, len);
		else
			gt2Reject(clientConnection, (const GT2Byte *)("Proxy failed to connect to server."), -1);

		// Close the socket.
		////////////////////
		RemoveServerSocket(gt2GetConnectionSocket(connection));
	}

#if STATS
	// Update stats.
	////////////////
	connectResults[result]++;
#endif
}

void ServerReceivedCallback
(
	GT2Connection connection,
	GT2Byte * message,
	int len,
	GT2Bool reliable
)
{
	GT2Connection clientConnection;

	// The client connection for this server connection.
	////////////////////////////////////////////////////
	clientConnection = (GT2Connection)gt2GetConnectionData(connection);

	// Send it the data.
	////////////////////
	gt2Send(clientConnection, message, len, reliable);

#if STATS
	// Update stats.
	////////////////
	if(reliable)
	{
		serverReliableMessagesSent++;
		serverReliableBytesSent += len;
	}
	else
	{
		serverUnreliableMessagesSent++;
		serverUnreliableBytesSent += len;
	}
#endif
}

void ServerClosedCallback
(
	GT2Connection connection,
	GT2CloseReason reason
)
{
	GT2Connection clientConnection;

	// The client connection for this server connection.
	////////////////////////////////////////////////////
	clientConnection = (GT2Connection)gt2GetConnectionData(connection);
	if(clientConnection)
	{
		// We don't want them to try closing us....
		///////////////////////////////////////////
		gt2SetConnectionData(clientConnection, NULL);

		// Close the connection.
		////////////////////////
		gt2CloseConnection(clientConnection);
	}

	// Close the socket.
	////////////////////
	RemoveServerSocket(gt2GetConnectionSocket(connection));

#if STATS
	// Update stats.
	////////////////
	if(reason == GT2LocalClose)
		serverCloses++;
#endif
}

GT2ConnectionCallbacks ServerConnectionCallbacks =
{
	ServerConnectedCallback,
	ServerReceivedCallback,
	ServerClosedCallback
};

/* LISTENER */

void SocketErrorCallback
(
	GT2Socket socket
)
{
	printf("Socket error (incoming connections socket)\n");
	Quit = GT2True;

	GSI_UNUSED(socket);
}

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
	GT2Socket serverSocket;
	GT2Connection serverConnection;
	GT2Result result;

	// Create a socket for this connection.
	///////////////////////////////////////
	result = gt2CreateSocket(&serverSocket, NULL, 0, 0, NULL);
	if(result != GT2Success)
	{
		gt2Reject(connection, (const GT2Byte *)("Proxy failed to create a new socket."), -1);
		return;
	}

	// Connect to the real server.
	//////////////////////////////
	result = gt2Connect(serverSocket, &serverConnection, RemoteAddress, message, len, 0, &ServerConnectionCallbacks, GT2False);
	if(result != GT2Success)
	{
		gt2Reject(connection, (const GT2Byte *)("Proxy failed to connect to server."), -1);
		gt2CloseSocket(serverSocket);
		return;
	}

	// Add the socket to the array.
	///////////////////////////////
	ArrayAppend(ServerSockets, &serverSocket);

	// The user data for each connection is the other.
	//////////////////////////////////////////////////
	gt2SetConnectionData(serverConnection, connection);
	gt2SetConnectionData(connection, serverConnection);

#if STATS
	// Update stats.
	////////////////
	numConnectAttempts++;
#endif

	GSI_UNUSED(latency);
	GSI_UNUSED(port);
	GSI_UNUSED(ip);
	GSI_UNUSED(socket);
}

#ifdef WIN32
BOOL WINAPI CtrlHandler
(
	DWORD type
)
{
	// Quit.
	////////
	Quit = GT2True;

	// We handled it.
	/////////////////
	return TRUE;

	GSI_UNUSED(type);
}
#endif

#if STATS
void DisplayStats(void)
{
	time_t runTime;
	time_t days;
	time_t hours;
	time_t minutes;
	time_t seconds;
	time_t ctimeTime;
	int numAccepted;
	int numRejected;
	int total;

	// Do the time stuff.
	/////////////////////
	ctimeTime = startTime;
	runTime = (time(NULL) - startTime);
	seconds = (runTime % 60);
	runTime /= 60;
	minutes = (runTime % 60);
	runTime /= 60;
	hours = (runTime % 24);
	runTime /= 24;
	days = runTime;

	// Do some other stuff.
	///////////////////////
	numAccepted = connectResults[GT2Success];
	numRejected = connectResults[GT2Rejected];

	// Print stats.
	///////////////
	printf("Proxying since %s", gsiSecondsToString(&ctimeTime));
	printf("(%d days, %d hours, %d minutes, %d seconds)\n",
		days,
		hours,
		minutes,
		seconds);
	printf("%d connect attempts, %d (%d%%) accepted, %d (%d%%) rejected\n",
		numConnectAttempts,
		numAccepted,
		(numConnectAttempts)?(numAccepted * 100) / numConnectAttempts:0,
		numRejected,
		(numConnectAttempts)?(numRejected * 100) / numConnectAttempts:0);
	total = (clientReliableMessagesSent + clientUnreliableMessagesSent);
	printf("client: %d messages, %d (%d%%) reliable, %d (%d%%) unreliable\n",
		total,
		clientReliableMessagesSent,
		(total)?(clientReliableMessagesSent * 100) / total:0,
		clientUnreliableMessagesSent,
		(total)?(clientUnreliableMessagesSent * 100) / total:0);
	total = (serverReliableMessagesSent + serverUnreliableMessagesSent);
	printf("server: %d messages, %d (%d%%) reliable, %d (%d%%) unreliable\n",
		total,
		serverReliableMessagesSent,
		(total)?(serverReliableMessagesSent * 100) / total:0,
		serverUnreliableMessagesSent,
		(total)?(serverUnreliableMessagesSent * 100) / total:0);
	total = (clientReliableBytesSent + clientUnreliableBytesSent);
	printf("client: %d bytes, %d (%d%%) reliable, %d (%d%%) unreliable\n",
		total,
		clientReliableBytesSent,
		(total)?(clientReliableBytesSent * 100) / total:0,
		clientUnreliableBytesSent,
		(total)?(clientUnreliableBytesSent * 100) / total:0);
	total = (serverReliableBytesSent + serverUnreliableBytesSent);
	printf("server: %d bytes, %d (%d%%) reliable, %d (%d%%) unreliable\n",
		total,
		serverReliableBytesSent,
		(total)?(serverReliableBytesSent * 100) / total:0,
		serverUnreliableBytesSent,
		(total)?(serverUnreliableBytesSent * 100) / total:0);
	total = (clientCloses + serverCloses);
	printf("%d closes, %d (%d%%) client, %d (%d%%) server\n",
		total,
		clientCloses,
		(total)?(clientCloses * 100) / total:0,
		serverCloses,
		(total)?(serverCloses * 100) / total:0);
}
#endif

int main
(
	int argc,
	char ** argv
)
{
	GT2Result result;
	int num;
	int i;
#if STATS
	unsigned int lastStatsTime = 0;
	unsigned int now;
#endif

	// Check args.
	//////////////
	if((argc < 2) || (argc > 3))
	{
		printf("%s <remote host:port> [localhost][:port]\n", argv[0]);
		return 1;
	}

	// First is the remote address.
	///////////////////////////////
	strcpy(RemoteAddress, argv[1]);

	// Second is the local address.
	///////////////////////////////
	if(argc >= 3)
		strcpy(LocalAddress, argv[2]);

	// Create the array of server sockets.
	//////////////////////////////////////
	ServerSockets = ArrayNew(sizeof(GT2Socket), 10, FreeServerSocket);
	if(!ServerSockets)
	{
		printf("Failed to create the array of server sockets\n");
		return 1;
	}

	// Create the socket.
	/////////////////////
	result = gt2CreateSocket(&Socket, LocalAddress, 0, 0, SocketErrorCallback);
	if(result != GT2Success)
	{
		printf("Error creating the socket (%d)\n", result);
		return 1;
	}

	// Start listening.
	///////////////////
	gt2Listen(Socket, ConnectAttemptCallback);

	// Show the port.
	/////////////////
	printf("Listening on port %d\n", gt2GetLocalPort(Socket));

	// For win32, setup a ctrl-c handler.
	/////////////////////////////////////
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	// We're starting.
	//////////////////
	startTime = time(NULL);

	// Loop until we quit.
	//////////////////////
	while(!Quit)
	{
#if STATS
		// Get the current time.
		////////////////////////
		now = current_time();

		// Display stats?
		/////////////////
		if((now - lastStatsTime) >= 30000)
		{
			DisplayStats();
			lastStatsTime = now;
		}
#endif

		// Yield.
		/////////
		msleep(1);

		// Think.
		/////////
		gt2Think(Socket);

		// Let the server sockets think.
		////////////////////////////////
		num = ArrayLength(ServerSockets);
		for(i = (num - 1) ; i >= 0 ; i--)
			gt2Think(*(GT2Socket *)ArrayNth(ServerSockets, i));
	}

#if STATS
	// Display some final stats.
	////////////////////////////
	DisplayStats();
#endif

	return 0;
}
