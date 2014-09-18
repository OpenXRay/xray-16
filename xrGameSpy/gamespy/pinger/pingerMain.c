/*
GameSpy Ping SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

/****************
** DEFINITIONS **
*****************

UPP = UDP Ping Protocol

*/

/*************
** INCLUDES **
*************/
#include <limits.h>
#include "pinger.h"
#include "../darray.h"

/************
** DEFINES **
************/
#define PI_MAGIC             0x91
#define PI_VERSION           0x01

#define PI_TRIP2_TIMEOUT     10000

#define PI_DATA_MAX_LEN      (PINGER_UDP_PING_SIZE - sizeof(piUDPPing))

/**********
** TYPES **
**********/
// u_char must be 1 byte unsigned, and unsigned short must be 2 bytes unsigned.
////////////////////////////////////////////////////////////////////////
typedef unsigned char  uint8;
typedef unsigned short uint16;

// This is the internal data for a UDP ping.
// This is an exact copy of what gets sent over the network, minus filler bytes.
////////////////////////////////////////////////////////////////////////////////
typedef struct piUDPPing
{
	uint8  magic;    // magic number - helps to ignore bad packets
	uint8  version;  // version - UPP version
	uint16 trip;     // trip number - will always be 1, 2, or 3
	uint16 ID_A;     // this ID is used by A (the source for the first dgram)
	uint16 ID_B;     // this ID is used by B (the destination for the first dgram)
} piUDPPing;

// This is a ping on which we're waiting for a reply.
/////////////////////////////////////////////////////
typedef struct piActivePing
{
	PINGERBool     originator;    // true if we sent the first dgram
	uint16         ID;            // unique ID for this ping
	uint16         expectedTrip;  // expected trip # of the reply
	gsi_time         timestamp;     // time we sent the ping
	gsi_time         timeout;       // time this ping expires
	unsigned int          remoteIP;      // IP of the other end of this ping
	unsigned short        remotePort;    // port of the other end of this ping
	pingerGotPing  reply;         // call this callback if we're the originator (otherwise call pingerPinged)
	void *         replyParam;    // the extra param passed to reply
} piActivePing;

// This is a queued gotPing callback.
/////////////////////////////////////
typedef struct piQueuedCallback
{
	unsigned int IP;
	unsigned short port;
	int ping;
	char * data;
	int len;
	void * param;
	pingerGotPing callback;
} piQueuedCallback;

/************
** GLOBALS **
************/
static PINGERBool       piInitialized = PINGERFalse;
static SOCKET           piSocket = INVALID_SOCKET;
static pingerGotPing    piPingerPinged;
static void *           piPingerPingedParam;
static pingerSetData    piPingerSetData;
static void *           piPingerSetDataParam;
static PINGERBool       piSettingData;
static PINGERBool       piUDPEnabled;
static uint16           piNextID;
static DArray           piActivePingList;
static gsi_time    piLastThinkTime;
static DArray           piCallbacks;

/**************
** FUNCTIONS **
**************/
static uint16 piGetNextID(void)
{
	// Store the next ID.
	/////////////////////
	uint16 ID = piNextID;

	// Increment the ID.
	////////////////////
	if(piNextID == USHRT_MAX)
		piNextID = 1;
	else
		piNextID++;

	return ID;
}

static PINGERBool piBytesToPing(unsigned char * buffer, piUDPPing * udpPing, char * data)
{
	assert(buffer != NULL);
	assert(udpPing != NULL);
	assert(data != NULL);

	// Magic.
	udpPing->magic = *buffer++;

	// Version.
	udpPing->version = *buffer++;

	// Trip.
	udpPing->trip = (unsigned short)(*buffer++ << 8);
	udpPing->trip |= *buffer++;

	// ID_A.
	udpPing->ID_A = (unsigned short)(*buffer++ << 8);
	udpPing->ID_A |= *buffer++;

	// ID_B.
	udpPing->ID_B = (unsigned short)(*buffer++ << 8);
	udpPing->ID_B |= *buffer++;

	// Check for the magic number.
	// This asserted on 11/17/00 at aprox. 1:40PM.
	// on a ping from 205.251.192.203:13139.
	// There were 2 empty bytes at the start of
	// the buffer, then the normal message followed.
	// Same thing on 12/14/00 at approx. 5PM.
	// From 24.156.198.26
	// JED 12/18/00 15:20, again from 24.156.198.26
	// 2001.Jan.15.JED - again from 24.156.198.26
	// 2001.Jan.23.JED - 213.65.117.53 (@11:15am)
	// 2001.Mar.26.JED - 24.159.107.0 (@7:30pm)
	// PANTS|04.20.00 - commented out per JED's request.
	////////////////////////////////////////////////////
	//assert(udpPing->magic == PI_MAGIC);
	if(udpPing->magic != PI_MAGIC)
		return PINGERFalse;

	// Check the version.
	/////////////////////
	assert(udpPing->version == PI_VERSION);
	if(udpPing->version != PI_VERSION)
		return PINGERFalse;

	// Check the trip.
	//////////////////
	assert((udpPing->trip >= 1) && (udpPing->trip <= 3));
	if((udpPing->trip < 1) || (udpPing->trip > 3))
		return PINGERFalse;

	// Fill in the data.
	////////////////////
	memcpy(data, buffer, PI_DATA_MAX_LEN);

	return PINGERTrue;
}

static void piPingToBytes(piUDPPing * udpPing, unsigned char * buffer)
{
	assert(udpPing != NULL);
	assert(buffer != NULL);

	// Magic.
	*buffer++ = udpPing->magic;

	// Version.
	*buffer++ = udpPing->version;

	// Trip.
	*buffer++ = (unsigned char)((udpPing->trip & 0xFF00) >> 8);
	*buffer++ = (unsigned char)(udpPing->trip & 0x00FF);

	// ID_A.
	*buffer++ = (unsigned char)((udpPing->ID_A & 0xFF00) >> 8);
	*buffer++ = (unsigned char)(udpPing->ID_A & 0x00FF);

	// ID_B.
	*buffer++ = (unsigned char)((udpPing->ID_B & 0xFF00) >> 8);
	*buffer++ = (unsigned char)(udpPing->ID_B & 0x00FF);
}

static PINGERBool piSendPing(SOCKADDR_IN * to, uint16 trip, uint16 ID_A, uint16 ID_B, const char * data)
{
	unsigned char buffer[PINGER_UDP_PING_SIZE];
	piUDPPing udpPing;
	int rcode;

	assert(to != NULL);
	assert((trip >= 1) && (trip <= 3));
	assert(!((trip == 2) && (ID_A == 0)));

	// Construct the outgoing ping.
	///////////////////////////////
	udpPing.magic = PI_MAGIC;
	udpPing.version = PI_VERSION;
	udpPing.trip = trip;
	udpPing.ID_A = ID_A;
	udpPing.ID_B = ID_B;
	piPingToBytes(&udpPing, buffer);
	if(data != NULL)
		memcpy(buffer + sizeof(piUDPPing), data, PI_DATA_MAX_LEN);
	else
		memset(buffer + sizeof(piUDPPing), 0, PI_DATA_MAX_LEN);

	// Send the outgoing ping.
	//////////////////////////
	rcode = sendto(piSocket, (char *)buffer, PINGER_UDP_PING_SIZE, 0, (SOCKADDR *)to, sizeof(SOCKADDR_IN));
	// Did it send ok?
	//////////////////
	if(rcode != PINGER_UDP_PING_SIZE)
		return PINGERFalse;

	return PINGERTrue;
}

static PINGERBool piSocketInit(const char * localAddress,
							   unsigned short localPort)
{
	int rcode;
	SOCKADDR_IN sockaddr;
	int bFlag;
	//int iLastError;

	assert(localPort != 0);

	// Setup sockets.
	/////////////////
	SocketStartUp();

	// Setup the address.
	/////////////////////
	memset(&sockaddr, 0, sizeof(SOCKADDR_IN));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(localPort);
	if(localAddress != NULL)
	{
		unsigned int IP;

		// Check for "a.b.c.d".
		///////////////////////
		IP = inet_addr(localAddress);
		if(IP == INADDR_NONE)
		{
			HOSTENT * hostent;

			// Check for "machine.host.domain".
			///////////////////////////////////
			hostent = gethostbyname(localAddress);
			if(hostent == NULL)
			{
//				iLastError = WSAGetLastError();
				assert(0);
				return PINGERFalse;
			}

			// Grab the IP.
			///////////////
			assert(IP != 0);
			IP = *(unsigned int *)hostent->h_addr_list[0];
		}

		// Got the IP.
		//////////////
		sockaddr.sin_addr.s_addr = IP;
	}
	else
	{
		// Any local IP.
		////////////////
		sockaddr.sin_addr.s_addr = INADDR_ANY;
	}

	// Create the socket.
	/////////////////////
	piSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(piSocket == INVALID_SOCKET)
	{
//		iLastError = GOAGetLastError(piSocket);
		assert(0);
		return PINGERFalse;
	}

	// Allow reuse of the socket if not closed properly before.
	///////////////////////////////////////////////////////////
	bFlag = 1;
#ifndef INSOCK   // PS2 INSOCK network layer does not support reuse addr
	rcode = setsockopt(piSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&bFlag, sizeof(bFlag) );
#endif
	/*if(gsiSocketIsError(rcode))
	{
		iLastError = WSAGetLastError();
		assert(0);
		return PINGERFalse;
	}*/

	// Bind the socket.
	///////////////////
	rcode = bind(piSocket, (SOCKADDR *)&sockaddr, sizeof(SOCKADDR_IN));
	if (gsiSocketIsError(rcode))
	{
//		iLastError = GOAGetLastError(piSocket);
		assert(0);
		return PINGERFalse;
	}

	return PINGERTrue;
}

static void piQueueCallback
(
	unsigned int IP,
	unsigned short port,
	int ping,
	const char * data,
	int len,
	void * param,
	pingerGotPing callbackFunc
)
{
	piQueuedCallback callback;

	assert(callbackFunc);
	if(!callbackFunc)
		return;

	// Setup the callback.
	//////////////////////
	callback.IP = IP;
	callback.port = port;
	callback.ping = ping;
	callback.len = len;
	callback.param = param;
	callback.callback = callbackFunc;

	// Copy the data.
	/////////////////
	if(data)
	{
		callback.data = (char *)gsimalloc((unsigned int)len);
		if(!callback.data)
			return;
		memcpy(callback.data, data, (unsigned int)len);
	}
	else
	{
		callback.data = NULL;
	}

	// Add it.
	//////////
	ArrayAppend(piCallbacks, &callback);
}

static void piCallCallbacks(void)
{
	piQueuedCallback * callback;
	piQueuedCallback callbackCopy;

	while(ArrayLength(piCallbacks) > 0)
	{
		// Get the info.
		////////////////
		callback = (piQueuedCallback *)ArrayNth(piCallbacks, 0);
		assert(callback);
		if(!callback)
			return;

		// Copy the info.
		/////////////////
		memcpy(&callbackCopy, callback, sizeof(piQueuedCallback));
		callback = &callbackCopy;

		// Remove it from the list.
		///////////////////////////
		ArrayDeleteAt(piCallbacks, 0);

		// Call it.
		///////////
		callback->callback(callback->IP, callback->port, callback->ping, callback->data, callback->len, callback->param);

		// gsifree it.
		///////////
		gsifree(callback->data);
	}
}

PINGERBool pingerInit(const char * localAddress,
					  unsigned short localPort,
					  pingerGotPing pinged,
					  void * pingedParam,
					  pingerSetData setData,
					  void * setDataParam)
{
	// Check if we're already initialized.
	//////////////////////////////////////
	assert(!piInitialized);

	// Make sure the UDP ping size is at least as large as the protocol struct.
	///////////////////////////////////////////////////////////////////////////
#if !defined(_NITRO)
	assert(PINGER_UDP_PING_SIZE >= sizeof(piUDPPing));
#endif

	// Check if we're already intialized.
	/////////////////////////////////////
	if(piInitialized)
		return PINGERFalse;

	// An empty localAddress is them same as a NULL.
	////////////////////////////////////////////////
	if((localAddress != NULL) && (localAddress[0] == '\0'))
		localAddress = NULL;

	// Initialize data.
	///////////////////
	piPingerPinged = pinged;
	piPingerPingedParam = pingedParam;
	piPingerSetData = setData;
	piPingerSetDataParam = setDataParam;
	piSettingData = PINGERFalse;
	piUDPEnabled = (PINGERBool)(localPort != 0);
	piNextID = 1;
	piLastThinkTime = 0;

	// Initialize the active ping list.
	///////////////////////////////////
	piActivePingList = ArrayNew(sizeof(piActivePing), 0, NULL);
	if(piActivePingList == NULL)
		return PINGERFalse;

	// Initialize the callbacks list.
	/////////////////////////////////
	piCallbacks = ArrayNew(sizeof(piQueuedCallback), 0, NULL);
	if(!piCallbacks)
	{
		ArrayFree(piActivePingList);
		return PINGERFalse;
	}

	// Setup the UDP socket.
	////////////////////////
	if(piUDPEnabled)
	{
		if(!piSocketInit(localAddress, localPort))
		{
			assert(0);

			// Check for partial intialization.
			///////////////////////////////////
			if(piSocket != INVALID_SOCKET)
			{
				closesocket(piSocket);
				piSocket = INVALID_SOCKET;
			}

			// Shut down sockets.
			/////////////////////
			SocketShutDown();

			// gsifree the lists.
			//////////////////
			ArrayFree(piActivePingList);
			ArrayFree(piCallbacks);

			return PINGERFalse;
		}
	}

	// We're initialized!
	/////////////////////
	piInitialized = PINGERTrue;

	return PINGERTrue;
}

void pingerShutdown(void)
{
	if(!piInitialized)
		return;

	// Check for setting data.
	//////////////////////////
	if(piSettingData)
		return;

	// Shut down the socket.
	////////////////////////
	if(piSocket != INVALID_SOCKET)
	{
		closesocket(piSocket);
		piSocket = INVALID_SOCKET;
	}

	// Shut down sockets.
	/////////////////////
	SocketShutDown();

	// gsifree the active ping list.
	/////////////////////////////
	ArrayFree(piActivePingList);

	// gsifree the callbacks list.
	///////////////////////////
	ArrayFree(piCallbacks);

	// Not initialized anymore.
	///////////////////////////
	piInitialized = PINGERFalse;
}

static int GS_STATIC_CALLBACK piFindActivePingCompareFn(const void *elem1, const void *elem2)
{
	piActivePing * activePing1 = (piActivePing *)elem1;
	piActivePing * activePing2 = (piActivePing *)elem2;
	assert(activePing1 != NULL);
	assert(activePing2 != NULL);
	assert(activePing1->ID != 0);
	assert(activePing2->ID != 0);

	return (activePing1->ID - activePing2->ID);
}

static piActivePing * piFindActivePing(uint16 ID, int * index)
{
	piActivePing key;
	int n;

	// Search for a matching ID.
	////////////////////////////
	key.ID = ID;
	n = ArraySearch(piActivePingList, &key, piFindActivePingCompareFn, 0, 0);
	if(index != NULL)
		*index = n;

	// Find it?
	///////////
	if(n == NOT_FOUND)
		return NULL;

	// Found it!
	////////////
	return (piActivePing *)ArrayNth(piActivePingList, n);
}

static int piCalculatePing(gsi_time sendTime, gsi_time recvTime)
{
	int ping;

	// Simple ping calculation.
	///////////////////////////
	ping = (int)(recvTime - sendTime);

	// Take off some time based on the last time we called
	// the think function.
	//////////////////////////////////////////////////////
	if(piLastThinkTime != 0)
	{
#if 0
		ping -= ((int)(recvTime - piLastThinkTime) / 2);
		if(ping < 0)
			ping = 0;
#endif
	}

	return ping;
}

static void piProcessTrip1(piUDPPing * udpPing, const char * data, SOCKADDR_IN * from, gsi_time recvTime)
{
	char dataOut[PI_DATA_MAX_LEN];
	uint16 ID;

	// Validity check.
	// udpPing->ID_A != 0 was triggered by a ping
	// from 213.105.94.117 sometime during 7/3-7/02
	///////////////////////////////////////////////
	assert(udpPing->trip == 1);
	//assert(udpPing->ID_A != 0);
	if(udpPing->ID_A == 0)
		return;
	assert(udpPing->ID_B == 0);
	if(udpPing->ID_B != 0)
		return;

	// Do we want a reply?
	//////////////////////
	if(piPingerPinged != NULL)
	{
		// Get an ID.
		/////////////
		ID = piGetNextID();
	}
	else
	{
		// No reply wanted.
		///////////////////
		ID = 0;
	}

	// Setup the data.
	//////////////////
	memset(dataOut, 0, PI_DATA_MAX_LEN);
	if(piPingerSetData != NULL)
	{
		piSettingData = PINGERTrue;
		piPingerSetData(from->sin_addr.s_addr, from->sin_port, dataOut, PI_DATA_MAX_LEN, piPingerSetDataParam);
		piSettingData = PINGERFalse;
	}

	// Send the return ping (trip 2).
	/////////////////////////////////
	piSendPing(from, 2, udpPing->ID_A, ID, dataOut);

	// Do we need to add an active ping?
	////////////////////////////////////
	if(piPingerPinged != NULL)
	{
		piActivePing activePing;

		// Setup the active ping object.
		////////////////////////////////
		activePing.originator = PINGERFalse;
		activePing.ID = ID;
		activePing.expectedTrip = 3;
		activePing.timestamp = current_time();
		activePing.timeout = (activePing.timestamp + PI_TRIP2_TIMEOUT);
		activePing.remoteIP = from->sin_addr.s_addr;
		activePing.remotePort = from->sin_port;
		activePing.reply = NULL;
		activePing.replyParam = NULL;

		// Add it to the list.
		//////////////////////
		ArrayAppend(piActivePingList, &activePing);
	}
	
	GSI_UNUSED(data);
	GSI_UNUSED(recvTime);
}

static void piProcessTrip2(piUDPPing * udpPing, const char * data, SOCKADDR_IN * from, gsi_time recvTime)
{
	char dataOut[PI_DATA_MAX_LEN];
	int index;
	piActivePing * activePing;

	// Validity check.
	// ID_A != 0 was triggered by a ping from 205.251.192.203.
	// 12/4/00 at approx. 5pm
	// This is the same IP as the shifted ping assert above.
	//////////////////////////////////////////////////////////
	assert(udpPing->trip == 2);
	assert(udpPing->ID_A != 0);
	if(udpPing->ID_A == 0)
		return;
	
	// Were we waiting for this?
	////////////////////////////
	activePing = piFindActivePing(udpPing->ID_A, &index);
	if(activePing == NULL)
		return;

	// Setup the data.
	//////////////////
	memset(dataOut, 0, PI_DATA_MAX_LEN);
	if(piPingerSetData != NULL)
	{
		piSettingData = PINGERTrue;
		piPingerSetData(from->sin_addr.s_addr, from->sin_port, dataOut, PI_DATA_MAX_LEN, piPingerSetDataParam);
		piSettingData = PINGERFalse;
	}

	// Do we send a reply?
	//////////////////////
	if(udpPing->ID_B != 0)
	{
		// Send the return ping.
		////////////////////////
		piSendPing(from, 3, 0, udpPing->ID_B, dataOut);
	}

	// Call the callback?
	/////////////////////
	if(activePing->reply != NULL)
	{
		// Calculate the ping.
		//////////////////////
		int ping = piCalculatePing(activePing->timestamp, recvTime);

		// Add the callback.
		////////////////////
		piQueueCallback(from->sin_addr.s_addr, from->sin_port, ping, data, PI_DATA_MAX_LEN, activePing->replyParam, activePing->reply);
	}
	
	// gsifree the active ping.
	////////////////////////
	ArrayDeleteAt(piActivePingList, index);
}

static void piProcessTrip3(piUDPPing * udpPing, const char * data, SOCKADDR_IN * from, gsi_time recvTime)
{
	int index;
	piActivePing * activePing;
	
	// Validity check.
	//////////////////
	assert(udpPing->trip == 3);
	assert(udpPing->ID_A == 0);
	if(udpPing->ID_A != 0)
		return;
	assert(udpPing->ID_B != 0);
	if(udpPing->ID_B == 0)
		return;

	// Were we waiting for this?
	////////////////////////////
	activePing = piFindActivePing(udpPing->ID_B, &index);
	if(activePing == NULL)
		return;

	// Call the callback.
	/////////////////////
	assert(piPingerPinged != NULL);
	if(piPingerPinged != NULL)
	{
		// Calculate the ping.
		//////////////////////
		int ping = piCalculatePing(activePing->timestamp, recvTime);

		// Call it.
		///////////
		piPingerPinged(from->sin_addr.s_addr, from->sin_port, ping, data, PI_DATA_MAX_LEN, piPingerPingedParam);
	}
	
	// gsifree the active ping.
	////////////////////////
	ArrayDeleteAt(piActivePingList, index);
}

static void piProcessPing(piUDPPing * udpPing, const char * data, SOCKADDR_IN * from, gsi_time recvTime)
{
	assert(udpPing != NULL);
	assert(data != NULL);
	assert(from != NULL);

	// Process based on trip num.
	/////////////////////////////
	if(udpPing->trip == 1)
		piProcessTrip1(udpPing, data, from, recvTime);
	else if(udpPing->trip == 2)
		piProcessTrip2(udpPing, data, from, recvTime);
	else if(udpPing->trip == 3)
		piProcessTrip3(udpPing, data, from, recvTime);
}

static void piProcessIncoming(void)
{
	int rcode;
	unsigned char buffer[PINGER_UDP_PING_SIZE];
	SOCKADDR_IN from;
	int len;
	gsi_time recvTime;
	piUDPPing udpPing;
	char data[PI_DATA_MAX_LEN];
	
	// Check for incoming dgrams.
	/////////////////////////////
	while(piInitialized && CanReceiveOnSocket(piSocket))
	{
		// Read the dgram.
		//////////////////
		len = sizeof(SOCKADDR_IN);

		rcode = recvfrom(piSocket, (char *)buffer, PINGER_UDP_PING_SIZE, 0, (SOCKADDR *)&from, &len);

		// Check for an error.
		//////////////////////
		if (gsiSocketIsError(rcode))
		{
			if(GOAGetLastError(piSocket) == WSAEMSGSIZE)
			{
				// Ignore "too big" errors.
				///////////////////////////
				rcode = PINGER_UDP_PING_SIZE;
			}
			else
			{
				// Something's wrong, get the hell out of here!
				///////////////////////////////////////////////
				return; //ERRCON
			}
		}
		else if(rcode < PINGER_UDP_PING_SIZE)
		{
			// The ping was too small.
			//////////////////////////
			return;  //ERRCON
		}

		// What time did we receive it?
		///////////////////////////////
		recvTime = current_time();

		// Convert the buffer into a ping.
		//////////////////////////////////
		if(piBytesToPing(buffer, &udpPing, data))
		{
			// Process the dgram.
			/////////////////////
			piProcessPing(&udpPing, data, &from, recvTime);
		}
	}

	// Last time we checked for incoming data.
	//////////////////////////////////////////
	piLastThinkTime = current_time();
}

static void piCheckTimeouts(void)
{
	gsi_time now;
	piActivePing * activePing;
	int len;
	int n;

	// Get the number of active pings.
	//////////////////////////////////
	len = ArrayLength(piActivePingList);

	// Get out if no active pings.
	//////////////////////////////
	if(len == 0)
		return;

	// Get the current time.
	////////////////////////
	now = current_time();

	// Go through the list backwards.
	/////////////////////////////////
	for(n = (len - 1) ; n >= 0 ; n--)
	{
		// Get the nth element of the list.
		///////////////////////////////////
		activePing = (piActivePing *)ArrayNth(piActivePingList, n);
		assert(activePing != NULL);
		if(activePing != NULL)
		{
			// Does it have a timeout?
			//////////////////////////
			if(activePing->timeout != 0)
			{
				// Has it timed out?
				////////////////////
				if(activePing->timeout <= now)
				{
					// Do we need to do a failed callback?
					//////////////////////////////////////
					if((activePing->originator) && (activePing->reply != NULL))
					{
						// Queue the callback.
						//////////////////////
						piQueueCallback(activePing->remoteIP, activePing->remotePort, PINGER_TIMEOUT, NULL, 0, activePing->replyParam, activePing->reply);
					}

					// Kill it!
					///////////
					ArrayDeleteAt(piActivePingList, n);
				}
			}
		}
	}
}

void pingerThink(void)
{
	// If not initialized, don't do anything.
	/////////////////////////////////////////
	if(!piInitialized)
		return;

	// Check for setting data.
	//////////////////////////
	if(piSettingData)
		return;

	// Process incoming data.
	/////////////////////////
	piProcessIncoming();

	// Check for timeouts.
	//////////////////////
	piCheckTimeouts();

	// Call queued callbacks.
	/////////////////////////
	piCallCallbacks();
}

void pingerPing(unsigned int IP,
				unsigned short port,
				pingerGotPing reply,
				void * replyParam,
				PINGERBool blocking,
				gsi_time timeout)
{
	SOCKADDR_IN to;
	uint16 ID;

	assert(piInitialized);
	assert(IP != 0);

	// ICMP not supported yet!
	//////////////////////////
	assert(port != 0);
	assert(piUDPEnabled);
	assert(piSocket != INVALID_SOCKET);

	// Check for setting data.
	//////////////////////////
	if(piSettingData)
		return;

	// Construct the outgoing address.
	//////////////////////////////////
	memset(&to, 0, sizeof(SOCKADDR_IN));
	to.sin_family = AF_INET;
	to.sin_port = htons(port);
	to.sin_addr.s_addr = IP;

	// Get an ID for this ping.
	///////////////////////////
	ID = piGetNextID();

	// Send the outgoing ping.
	//////////////////////////
	if(piSendPing(&to, 1, ID, 0, NULL))
	{
		piActivePing activePing;

		// Setup the active ping object.
		////////////////////////////////
		activePing.originator = PINGERTrue;
		activePing.ID = ID;
		activePing.expectedTrip = 2;
		activePing.timestamp = current_time();
		if(timeout == 0)
			activePing.timeout = 0;
		else
			activePing.timeout = (activePing.timestamp + timeout);
		activePing.remoteIP = IP;
		activePing.remotePort = port;
		activePing.reply = reply;
		activePing.replyParam = replyParam;

		// Add it to the list.
		//////////////////////
		ArrayAppend(piActivePingList, &activePing);
	}

	// Is this blocking?
	////////////////////
	if(blocking)
	{
		// Keep going until this ping has been de-listed.
		/////////////////////////////////////////////////
		while(piFindActivePing(ID, NULL) != NULL)
		{
			// Think.
			/////////
			pingerThink();

			// Yield.
			/////////
			msleep(1);
		}

		// Call callbacks.
		//////////////////
		piCallCallbacks();
	}
}
