/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEERMANGLE_H_
#define _PEERMANGLE_H_

/*************
** INCLUDES **
*************/
#include "peer.h"
#include "peerMain.h"


#ifdef __cplusplus
extern "C" {
#endif

/************
** DEFINES **
************/
#define PI_USER_MAX_LEN        128

/**************
** FUNCTIONS **
**************/
void piMangleTitleRoom(char buffer[PI_ROOM_MAX_LEN], const char * title);
void piMangleGroupRoom(char buffer[PI_ROOM_MAX_LEN], int groupID);
void piMangleStagingRoom(char buffer[PI_ROOM_MAX_LEN], const char * title,
                         unsigned int publicIP, unsigned int privateIP, unsigned short privatePort);
void piMangleUser(char buffer[PI_USER_MAX_LEN], unsigned int IP, int profileID);
PEERBool piDemangleUser(const char buffer[PI_USER_MAX_LEN], unsigned int * IP, int * profileID);
void piMangleIP(char buffer[11], unsigned int IP);
unsigned int piDemangleIP(const char buffer[11]);

#ifdef __cplusplus
}
#endif

#endif
