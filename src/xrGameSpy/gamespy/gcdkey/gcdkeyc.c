/******
gcdkeyc.c
GameSpy CDKey SDK Client Code
  
Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

******

 Please see the GameSpy CDKey SDK documentation for more 
 information

******/

#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4267) //lines: 64, 68
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS


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
		strcpy(response,"CD Key or challenge too long");
		return;
	}

	/* make sure we are randomized */
	srand((unsigned int)time(NULL) ^ 0x33333333);
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
	strcpy(&response[32], randstr);
	/* do the response md5 */
	MD5Digest((unsigned char *)rawout, strlen(rawout), &response[40]);	
}


#ifdef __cplusplus
}
#endif

