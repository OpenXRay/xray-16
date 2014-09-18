/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEERROOMS_H_
#define _PEERROOMS_H_

/*************
** INCLUDES **
*************/
#include "peerMain.h"


#ifdef __cplusplus
extern "C" {
#endif


/**************
** FUNCTIONS **
**************/
PEERBool piRoomsInit(PEER peer);
void piRoomsCleanup(PEER peer);
void piStartedEnteringRoom(PEER peer, RoomType roomType, const char * room);
void piFinishedEnteringRoom(PEER peer, RoomType roomType, const char * name);
void piLeaveRoom(PEER peer, RoomType roomType, const char * reason);
PEERBool piRoomToType(PEER peer, const char * room, RoomType * roomType);
void piSetLocalFlags(PEER peer);

#ifdef __cplusplus
}
#endif

#endif
