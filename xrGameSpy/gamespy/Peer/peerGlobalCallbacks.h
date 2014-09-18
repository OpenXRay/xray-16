/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEERGLOBALCALLBACKS_H_
#define _PEERGLOBALCALLBACKS_H_

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
#define PI_UTM_LAUNCH          "GML"
#define PI_UTM_XPING           "PNG"

/**************
** FUNCTIONS **
**************/

/* Chat.
*******/
#ifndef GSI_UNICODE
#define piChatDisconnected		piChatDisconnectedA
#define piChatPrivateMessage	piChatPrivateMessageA
#else
#define piChatDisconnected		piChatDisconnectedW
#define piChatPrivateMessage	piChatPrivateMessageW
#endif

// Include both forms here so CodeWarrior will have it's happy prototypes
void piChatDisconnectedA(CHAT chat, const char * reason, PEER peer);
void piChatDisconnectedW(CHAT chat, const unsigned short * reason, PEER peer);

void piChatPrivateMessageA(CHAT chat, const char * user, const char * message, int type, PEER peer);
void piChatPrivateMessageW(CHAT chat, const unsigned short * user, const unsigned short * message, int type, PEER peer);

void piSetChannelCallbacks(PEER peer, chatChannelCallbacks * channelCallbacks);

#ifdef __cplusplus
}
#endif

#endif
