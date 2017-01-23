/*
gpiBuddy.h
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

#ifndef _GPIBUDDY_H_
#define _GPIBUDDY_H_

//INCLUDES
//////////
#include "gpi.h"

//DEFINES
/////////
// Types of bm's.
/////////////////
#define GPI_BM_MESSAGE                    1
#define GPI_BM_REQUEST                    2
#define GPI_BM_REPLY                      3  // only used on the backend
#define GPI_BM_AUTH                       4
#define GPI_BM_UTM                        5
#define GPI_BM_REVOKE                     6  // remote buddy removed from local list
#define GPI_BM_STATUS                   100						
#define GPI_BM_INVITE                   101
#define GPI_BM_PING                     102
#define GPI_BM_PONG                     103
#define GPI_BM_KEYS_REQUEST             104
#define GPI_BM_KEYS_REPLY               105
#define GPI_BM_FILE_SEND_REQUEST        200
#define GPI_BM_FILE_SEND_REPLY          201
#define GPI_BM_FILE_BEGIN               202
#define GPI_BM_FILE_END                 203
#define GPI_BM_FILE_DATA                204
#define GPI_BM_FILE_SKIP                205
#define GPI_BM_FILE_TRANSFER_THROTTLE   206
#define GPI_BM_FILE_TRANSFER_CANCEL     207
#define GPI_BM_FILE_TRANSFER_KEEPALIVE  208

//FUNCTIONS
///////////
GPResult
gpiProcessRecvBuddyMessage(
  GPConnection * connection,
  const char * input
);

GPResult gpiProcessRecvBuddyStatusInfo(GPConnection *connection, const char *input);

GPResult 
gpiProcessRecvBuddyList(
  GPConnection * connection, 
  const char * input
);

GPResult
gpiSendServerBuddyMessage(
  GPConnection * connection,
  int profileid,
  int type,
  const char * message
);

GPResult
gpiSendBuddyMessage(
  GPConnection * connection,
  int profileid,
  int type,
  const char * message,
  int sendOptions,
  GPIPeerOp *peerOp
);

GPResult gpiBuddyHandleKeyRequest(GPConnection *connection, GPIPeer *peer);
GPResult gpiBuddyHandleKeyReply(GPConnection *connection, GPIPeer *peer, char *buffer);

GPResult
gpiAuthBuddyRequest(
  GPConnection * connection,
  GPProfile profile
);

GPIBool
gpiFixBuddyIndices(
  GPConnection * connection,
  GPIProfile * profile,
  void * data
);

GPResult
gpiDeleteBuddy(
  GPConnection * connection,
  GPProfile profile,
  GPIBool sendServerRequest
);

#endif
