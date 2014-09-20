/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEERQR_H_
#define _PEERQR_H_

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
#define PI_QUERYPORT       6500

/**************
** FUNCTIONS **
**************/
PEERBool piStartReporting(PEER peer, SOCKET socket, unsigned short port);
void piStopReporting(PEER peer);
void piSendStateChanged(PEER peer);
void piQRThink(PEER peer);
PEERBool piStartAutoMatchReporting(PEER peer);
void piStopAutoMatchReporting(PEER peer);

#ifdef __cplusplus
}
#endif

#endif
