/******
gcdkeyc.c
GameSpy CDKey SDK Client Code
  
Copyright 1999-2001 GameSpy Industries, Inc

18002 Skypark Circle
Irvine, California 92614
949.798.4200 (Tel)
949.798.4299 (Fax)
devsupport@gamespy.com

******

 Please see the GameSpy CDKey SDK documentation for more 
 information

******/
#include "../md5.h"
#include "gcdkeyc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RAWSIZE 512

#ifdef __cplusplus
extern "C" {
#endif

	// method = 0 for normal auth response from game server
	// method = 1 for reauth response originating from keymaster
void gcd_compute_response(char *cdkey, char *challenge, char response[RESPONSE_SIZE], CDResponseMethod method)
{
	char rawout[RAWSIZE];
	unsigned int anyrandom;
	char randstr[9];


	/* check to make sure we weren't passed a huge cd key/challenge */
	if (strlen(cdkey) * 2 + strlen(challenge) + 8 >= RAWSIZE)
	{
		strcpy_s(response, RESPONSE_SIZE, "CD Key or challenge too long");
		return;
	}

	/* make sure we are randomized */
	srand(time(NULL) ^ 0x33333333);
	/* Since RAND_MAX is 16 bit on many systems, make sure we get a 32 bit number */
	anyrandom = (rand() << 16 | rand()); 
	sprintf(randstr,"%.8x",anyrandom);

	/* auth response   = MD5(cdkey + random mod 0xffff + challenge) */
	/* reauth response = MD5(challenge + random mode 0xffff + cdkey) */ 
	if (method == 0)
		sprintf(rawout, "%s%d%s",cdkey, anyrandom % 0xFFFF , challenge );
	else
		sprintf(rawout, "%s%d%s",challenge, anyrandom % 0xFFFF, cdkey);

	/* do the cd key md5 */
	MD5Digest((unsigned char *)cdkey, strlen(cdkey), response);
	/* add the random value */
	strcpy_s(&response[32], sizeof(response)-32, randstr);
	/* do the response md5 */
	MD5Digest((unsigned char *)rawout, strlen(rawout), &response[40]);	
}


#ifdef __cplusplus
}
#endif

