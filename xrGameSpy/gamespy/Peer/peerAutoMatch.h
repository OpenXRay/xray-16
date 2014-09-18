/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEERAUTOMATCH_H_
#define _PEERAUTOMATCH_H_

/*************
** INCLUDES **
*************/
#include "peerMain.h"

#ifdef __cplusplus
extern "C" {
#endif

/************
** DEFINES **
************/
#define PI_AUTOMATCH_RATING_KEY "gsi_am_rating"

/**************
** FUNCTIONS **
**************/
void piSetAutoMatchStatus(PEER peer, PEERAutoMatchStatus status);
void piStopAutoMatch(PEER peer);
PEERBool piJoinAutoMatchRoom(PEER peer, SBServer server);

#ifdef __cplusplus
}
#endif

#endif
