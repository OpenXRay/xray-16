#include "gsCommon.h"
#include "gsAvailable.h"

#define PACKET_TYPE    0x09
#define MASTER_PORT   27900
#define MAX_RETRIES       1
#define TIMEOUT_TIME   2000

// this is the global var that the SDKs check
// to see if they should communicate with the backend
GSIACResult __GSIACResult	= GSIACWaiting;

// this makes the gamename available to all of the SDKs
char __GSIACGamename[64]	= {0};

// this allows devs to do their own hostname resolution
char GSIACHostname[64]		= {0};

// used to keep state during the check
static struct
{
	SOCKET sock;
	SOCKADDR_IN address;
	char packet[64];
	int packetLen;
	gsi_time sendTime;
	int retryCount;
} AC;

static int get_sockaddrin(const char * hostname, int port, SOCKADDR_IN * saddr)
{
	GS_ASSERT(hostname)
	GS_ASSERT(saddr)

	saddr->sin_family = AF_INET;
	saddr->sin_port = htons((unsigned short)port);
	saddr->sin_addr.s_addr = inet_addr(hostname);
	
	if(saddr->sin_addr.s_addr == INADDR_NONE)
	{
		HOSTENT * host = gethostbyname(hostname);
		if(!host)
			return 0;
		saddr->sin_addr.s_addr = *(unsigned int *)host->h_addr_list[0];
	}

	return 1;
}

static void SendPacket(void)
{
	sendto(AC.sock, AC.packet, AC.packetLen, 0, (SOCKADDR *)&AC.address, sizeof(AC.address));
	AC.sendTime = current_time();
}

void GSIStartAvailableCheckA(const char * gamename)
{
	char hostname[64];
	int override;
	int rcode;
	int len;

	GS_ASSERT(gamename)

	// store the gamename
	strcpy(__GSIACGamename, gamename);

	// clear the sock
	AC.sock = INVALID_SOCKET;

	// startup sockets
	SocketStartUp();

	// setup the hostname
	override = GSIACHostname[0];
	if(!override)
		sprintf(hostname, "%s.available." GSI_DOMAIN_NAME, gamename);

	// get the master address
	rcode = get_sockaddrin(override?GSIACHostname:hostname, MASTER_PORT, &AC.address);
	if(!rcode)
		return;

	// create the socket
	AC.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(AC.sock == INVALID_SOCKET)
		return;

	// setup our packet
	AC.packet[0] = PACKET_TYPE;
	len = (int)strlen(gamename);
	memcpy(AC.packet + 5, gamename, (size_t)len + 1);
	AC.packetLen = (len + 6);

	// send it
	SendPacket();

	// no retries yet
	AC.retryCount = 0;
}
#ifdef GSI_UNICODE
void GSIStartAvailableCheckW(const unsigned short * gamename)
{
	char gamename_A[32];
	GS_ASSERT(gamename)

	UCS2ToAsciiString(gamename, gamename_A);

	GSIStartAvailableCheckA(gamename_A);
}
#endif

static int HandlePacket(char * packet, int len, SOCKADDR_IN * address, int * disabledservices)
{
	int bitfield;

	// check the length
	if(len < 7)
		return 1;

	// check the IP
	if(memcmp(&address->sin_addr, &AC.address.sin_addr, sizeof(IN_ADDR)) != 0)
		return 1;

	// check the port
	if(address->sin_port != AC.address.sin_port)
		return 1;

	// check the header
	if(memcmp(packet, "\xFE\xFD\x09", 3) != 0)
		return 1;

	// read out the bitfield
	// read byte-by-byte to avoid alignment issues
	bitfield = (int)((packet[3] << 24) & 0xFF000000);
	bitfield |= ((packet[4] << 16) & 0x00FF0000);
	bitfield |= ((packet[5] << 8) & 0x0000FF00);
	bitfield |= (packet[6] & 0x000000FF);

	// set it
	*disabledservices = bitfield;

	return 0;
}

GSIACResult GSIAvailableCheckThink(void)
{
	char packet[64];
	SOCKADDR_IN address;
	int len = sizeof(address);
	int rcode;
	int disabledservices;

	// if we don't have a sock, possibly because of an initialization error, default to available
	if(AC.sock == INVALID_SOCKET)
	{
		__GSIACResult = GSIACAvailable;
		return __GSIACResult;
	}

	// did we get a response?
	if(CanReceiveOnSocket(AC.sock))
	{
		// read it from the socket
		rcode = (int)recvfrom(AC.sock, packet, (int)sizeof(packet), 0, (SOCKADDR *)&address, &len);

		// verify the packet
		rcode = HandlePacket(packet, rcode, &address, &disabledservices);
		if(rcode == 0)
		{
			// we got a valid response, clean up
			closesocket(AC.sock);

			// set the result based on the bit flags
			if(disabledservices & 1)
				__GSIACResult = GSIACUnavailable;
			else if(disabledservices & 2)
				__GSIACResult = GSIACTemporarilyUnavailable;
			else
				__GSIACResult = GSIACAvailable;

			// return it
			return __GSIACResult;
		}
	}

	// check for a timeout
	if(current_time() > (AC.sendTime + TIMEOUT_TIME))
	{
		// check for too many retries
		if(AC.retryCount == MAX_RETRIES)
		{
			// default to available
			closesocket(AC.sock);
			__GSIACResult = GSIACAvailable;
			return __GSIACResult;
		}

		// send a retry
		SendPacket();
		AC.retryCount++;
	}

	return GSIACWaiting;
}

void GSICancelAvailableCheck(void)
{
	if(AC.sock != INVALID_SOCKET)
	{
		closesocket(AC.sock);
		AC.sock = INVALID_SOCKET;
		__GSIACResult = GSIACWaiting;
	}
}
