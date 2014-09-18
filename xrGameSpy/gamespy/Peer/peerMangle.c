/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

/*
**
** Title Room
**   #GSP!<gamename>
**
** Group Room
**   #GPG!<groupid>
**
** Staging Room
**   #GSP!<gamename>!X<encoded public IP><encoded public port><encoded private IP><encoded private port>X
**
** User
**   X<encoded public IP>X|<profile ID>
**
*/

/*************
** INCLUDES **
*************/
#include <limits.h>
#include "peerMain.h"
#include "peerMangle.h"

/************
** DEFINES **
************/
#define PI_SEPERATOR            "!"

/************
** GLOBALS **
************/
PEERBool piOldMangleStagingRooms;
static const char digits_hex[] = "0123456789abcdef";
static const char digits_crypt[] = "aFl4uOD9sfWq1vGp";
static const char new_digits_crypt[] = "qJ1h4N9cP3lzD0Ka";
static const unsigned int ip_xormask = 0xc3801dc7;
static char cryptbuffer[32];

/**************
** FUNCTIONS **
**************/
//originally ripped from Aphex's ipencode.h
static const char * EncodeIP(unsigned int ip, char * buffer, PEERBool newCrypt)
{
	const char * crypt = newCrypt?new_digits_crypt:digits_crypt;
	int i;
	char * str;
	int digit_idx;

	// XOR the IP address.
	ip ^= ip_xormask;

	// Print out the ip addr in hex form.
	sprintf(cryptbuffer, "%08x", ip);

	// Translate chars in positions 0 through 7 from hex digits to "crypt" digits.
	for(i = 0 ; i < 8 ; i++)
	{
		str = strchr(digits_hex, cryptbuffer[i]);
		digit_idx = (str - digits_hex);

		if((digit_idx < 0) || (digit_idx > 15)) // sanity check
		{
			strcpy(cryptbuffer, "14saFv19"); // equivalent to 0.0.0.0
			break;
		}

		cryptbuffer[i] = crypt[digit_idx];
	}

	if(buffer)
	{
		strcpy(buffer, cryptbuffer);
		return buffer;
	}

	return cryptbuffer;
}

//originally ripped from Aphex's ipencode.h
static unsigned int DecodeIP(const char * buffer, PEERBool newCrypt)
{
	const char * crypt = newCrypt?new_digits_crypt:digits_crypt;
	unsigned int ip;
	char * str;
	int digit_idx;
	int i;

	if(!buffer)
		return 0;
	
	// Translate chars from hex digits to "crypt" digits.
	for(i = 0 ; i < 8 ; i++)
	{
		str = strchr(crypt, buffer[i]);
		digit_idx = (str - crypt);

		if((digit_idx < 0) || (digit_idx > 15))
			return 0;

		cryptbuffer[i] = digits_hex[digit_idx];
	}

	// Cap the buffer.
	cryptbuffer[i] = '\0';

	// Convert the string to an unsigned long (the XORd ip addr).
	sscanf(cryptbuffer, "%x", &ip);

	// re-XOR the IP address.
	ip ^= ip_xormask;

	return ip;
}

static const char * piStagingRoomHash(unsigned int publicIP, unsigned int privateIP, unsigned short port, char * buffer)
{
	unsigned int result;

	publicIP = ntohl(publicIP);
	privateIP = ntohl(privateIP);

	result = (((privateIP >> 24) & 0xFF) | ((privateIP >> 8) & 0xFF00) | ((privateIP << 8) & 0xFF0000) | ((privateIP << 24) & 0xFF000000));
	result ^= publicIP;
	result ^= (port | (port << 16));

	return EncodeIP(result, buffer, PEERTrue);
}

void piMangleTitleRoom
(
	char buffer[PI_ROOM_MAX_LEN],
	const char * title
)
{
	assert(buffer);
	assert(title);
	assert(title[0]);

	sprintf(buffer, "#GSP" PI_SEPERATOR "%s",
		title);
}

void piMangleGroupRoom
(
	char buffer[PI_ROOM_MAX_LEN],
	int groupID
)
{
	assert(buffer);
	assert(groupID);

	sprintf(buffer, "#GPG" PI_SEPERATOR "%d", groupID);
}

void piMangleStagingRoom
(
	char buffer[PI_ROOM_MAX_LEN],
	const char * title,
	unsigned int publicIP,
	unsigned int privateIP,
	unsigned short privatePort
)
{
	char encodeBuffer[9];
	int borderChar;

	assert(buffer);
	assert(title);
	assert(title[0]);

	if(piOldMangleStagingRooms)
	{
		EncodeIP(publicIP, encodeBuffer, PEERFalse);
		borderChar = 'X';
	}
	else
	{
		piStagingRoomHash(publicIP, privateIP, privatePort, encodeBuffer);
		borderChar = 'M';
	}

	sprintf(buffer, "#GSP" PI_SEPERATOR "%s" PI_SEPERATOR "%c%s%c", title, borderChar, encodeBuffer, borderChar);
}

void piMangleUser
(
	char buffer[PI_USER_MAX_LEN],
	unsigned int IP,
	int profileID
)
{
	assert(buffer);
	assert(IP != 0);
	assert(profileID >= 0);

	sprintf(buffer, "X%sX|%d",
		EncodeIP(IP, NULL, PEERFalse),
		profileID);
}

PEERBool piDemangleUser
(
	const char buffer[PI_USER_MAX_LEN],
	unsigned int * IP,
	int * profileID
)
{
	unsigned int decodedIP;
	int scannedProfileID;

	assert(buffer);
	if(buffer == NULL)
		return PEERFalse;

	// Check the length.
	////////////////////
	if(strlen(buffer) < 12)
		return PEERFalse;

	// Check for the Xs.
	////////////////////
	if((buffer[0] != 'X') && (buffer[9] != 'X'))
		return PEERFalse;

	// Get the IP.
	//////////////
	decodedIP = DecodeIP(buffer + 1, PEERFalse);
	if(!decodedIP)
		return PEERFalse;

	// Check the profile ID.
	////////////////////////
	if(!isdigit(buffer[11]))
		return PEERFalse;

	// Get the pid.
	///////////////
	scannedProfileID = atoi(buffer + 11);

	// Check what is wanted.
	////////////////////////
	if(IP)
		*IP = decodedIP;
	if(profileID)
		*profileID = scannedProfileID;


	return PEERTrue;
}

void piMangleIP
(
	char buffer[11],
	unsigned int IP
)
{
	assert(buffer);
	assert(IP != 0);

	EncodeIP(IP, buffer + 1, PEERFalse);
	buffer[0] = 'X';
	buffer[9] = 'X';
	buffer[10] = '\0';
}

unsigned int piDemangleIP
(
	const char buffer[11]
)
{
	assert(buffer);
	if(!buffer)
		return 0;

	// Check for the Xs.
	////////////////////
	if((buffer[0] != 'X') && (buffer[9] != 'X'))
		return 0;

	return DecodeIP(buffer + 1, PEERFalse);
}
