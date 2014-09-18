/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEERSB_H_
#define _PEERSB_H_

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
PEERBool piSBInit(PEER peer);
void piSBCleanup(PEER peer);
PEERBool piSBStartListingGames(PEER peer, const unsigned char * fields, int numFields, const char * filter);
void piSBStopListingGames(PEER peer);
void piSBUpdateGame(PEER peer, SBServer server, PEERBool fullUpdate, PEERBool forceUpdateByMaster, PEERBool icmpEcho);
PEERBool piSBStartListingGroups(PEER peer, const char * fields);
void piSBStopListingGroups(PEER peer);
void piSBThink(PEER peer);
void piSendNatNegotiateCookie(PEER peer, unsigned int ip, unsigned short port, int cookie);
void piSendMessageToServer(PEER peer, unsigned int ip, unsigned short port, const char * data, int len);
PEERBool piSBStartListingAutoMatches(PEER peer);
void piSBStopListingAutoMatches(PEER peer);
SBServer piSBCloneServer(SBServer server);
void piSBFreeHostServer(PEER peer);

#ifdef __cplusplus
}
#endif

#endif
