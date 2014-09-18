/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEERHOST_H_
#define _PEERHOST_H_

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
PEERBool piStartHosting(PEER peer, SOCKET socket, unsigned short port);
void piStopHosting(PEER peer, PEERBool stopReporting);

#ifdef __cplusplus
}
#endif

#endif
