/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#include "gt2Main.h"
#include "gt2Auth.h"
#include <stdlib.h>

#define CALCULATEODDMODE(buffer, i, oddmode) ((buffer[i-1] & 1) ^ (i & 1) ^ oddmode ^ (buffer[0] & 1) ^ ((buffer[0] < 79) ? 1 : 0) ^ ((buffer[i-1] < buffer[0]) ? 1 : 0));

char GT2ChallengeKey[33] = "3b8dd8995f7c40a9a5c5b7dd5b481341";

static int gti2VerifyChallenge(const GT2Byte *buffer)
{
	int oddmode = 0;
	int i;
	for (i = 1; i < GTI2_CHALLENGE_LEN ; i++)
	{
		oddmode = CALCULATEODDMODE(buffer,i, oddmode);
		if ((oddmode && (buffer[i] & 1) == 0) || (!oddmode && ((buffer[i] & 1) == 1)))
			return 0; //failed!!
	}
	return 1;
}

GT2Byte * gti2GetChallenge
(
	GT2Byte * buffer
)
{
	int i;
	int oddmode;
	assert(buffer);

	srand((unsigned int)current_time());
	buffer[0] = (GT2Byte)(33 + rand() % 93); //use chars in the range 33 - 125
	oddmode = 0;
	for (i = 1; i < GTI2_CHALLENGE_LEN ; i++)
	{
		oddmode = CALCULATEODDMODE(buffer,i, oddmode);
		buffer[i] = (GT2Byte)(33 + rand() % 93); //use chars in the range 33 - 125
		//if oddmode make sure the char is odd, otherwise make sure it's even
		if ((oddmode && (buffer[i] & 1) == 0) || (!oddmode && ((buffer[i] & 1) == 1)))
			buffer[i]++;

	}
	return buffer;
}

GT2Byte * gti2GetResponse
(
	GT2Byte * buffer,
	const GT2Byte * challenge
)
{
	int i;
	int valid;
	char cchar;
	int keylen = (int)strlen(GT2ChallengeKey);
	int chalrand;
	valid = gti2VerifyChallenge(challenge); //it's an invalid challenge, give them a bogus response
	//assert(GTI2_RESPONSE_LEN <= GTI2_CHALLENGE_LEN);
	for (i = 0 ; i < GTI2_RESPONSE_LEN ; i++)
	{
		//use random vals for spots 0 and 13
		if (!valid || i == 0 || i == 13)
			buffer[i] = (GT2Byte)(33 + rand() % 93); //use chars in the range 33 - 125
		else
		{ //set the character to look back at, never use the random ones!
			if (i == 1 || i == 14)
				cchar = (char)challenge[i];
			else
				cchar = (char)challenge[i-1];
			chalrand = abs((challenge[((i * challenge[i]) + GT2ChallengeKey[(i + challenge[i]) % keylen]) % GTI2_CHALLENGE_LEN] ^ GT2ChallengeKey[(i * 17991 * cchar) % keylen]));
			buffer[i] = (GT2Byte)(33 + chalrand % 93);
		}
	}
	return buffer;
}


GT2Bool gti2CheckResponse
(
	const GT2Byte * response1,
	const GT2Byte * response2
)
{
	int i; //when comparing ignore the ones that are random
	for (i = 0 ; i < GTI2_RESPONSE_LEN ; i++)
	{
		if (i != 0 && i != 13 && response1[i] != response2[i])
			return GT2False;
	}
	return GT2True;
}

