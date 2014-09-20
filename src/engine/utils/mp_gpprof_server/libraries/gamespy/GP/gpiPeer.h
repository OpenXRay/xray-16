/*
gpiPeer.h
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

#ifndef _GPIPEER_H_
#define _GPIPEER_H_

//INCLUDES
//////////
#include "gpi.h"

//DEFINES
/////////
// Peer states.
///////////////
#define GPI_PEER_NOT_CONNECTED       100
#define GPI_PEER_GETTING_SIG         101
#define GPI_PEER_GOT_SIG             102
#define GPI_PEER_CONNECTING          103
#define GPI_PEER_WAITING             104
#define GPI_PEER_CONNECTED           105
#define GPI_PEER_DISCONNECTED        106

// Timeout for a peer connection, in milliseconds.
/////////////////////////////////////////////
#define GPI_PEER_TIMEOUT               (10 * 1000)

// Timeout for a peer operation, in milliseconds
////////////////////////////////////////////
#define GPI_PEER_OP_TIMEOUT            60000

typedef enum
{
	GPI_PEER_OP_STATE_NONE,
	GPI_PEER_OP_STATE_REQUESTED,
	GPI_PEER_OP_STATE_FINISHED
} GPIPeerOpState;
 
typedef struct GPITransferID_s * GPITransferID_st;

//TYPES
///////
// A peer message.
//////////////////
typedef struct GPIMessage
{
	GPIBuffer buffer;
	int type;
	int start;
} GPIMessage;

typedef struct _GPIPeerOp
{
	GPIPeerOpState state;
	void *userData;
	GPCallback callback;
	struct _GPIPeerOp * next;
	int type;
	gsi_time timeout;
} GPIPeerOp;

typedef struct _GPIPeerOpQueue
{
	GPIPeerOp * opList;
	GPIPeerOp * first;
	GPIPeerOp * last;
} GPIPeerOpQueue;

// A peer connection.
/////////////////////
typedef struct GPIPeer_s
{
	int state;
	GPIBool initiated;
	//SOCKET sock;
	unsigned int ip;
	unsigned short port;
	GPProfile profile;
	time_t timeout;
	int nackCount;
	GPIBuffer inputBuffer;
	GPIBuffer outputBuffer;
	DArray messages;
	GPIPeerOpQueue peerOpQueue;
	struct GPIPeer_s * pnext;
} GPIPeer;

//FUNCTIONS
///////////
GPResult
gpiProcessPeers(
  GPConnection * connection
);

GPResult
gpiPeerGetSig(
  GPConnection * connection,
  GPIPeer * peer
);

GPResult
gpiPeerStartConnect(
  GPConnection * connection,
  GPIPeer * peer
);

// NOTE: use this function when in a gp function
GPIPeer * gpiGetPeerByProfile(const GPConnection * connection,
							  int profileid);

// NOTE: use this function only when in a UDP layer callback
GPIPeer *gpiGetPeerByAddr(const GPConnection *connection,
                          unsigned int ip,
                          unsigned short port);

gsi_bool gpiIsPeerConnected(GPIPeer *peer);

GPIPeer *
gpiAddPeer(
  GPConnection * connection,
  int profileid,
  GPIBool initiate
);

void
gpiDestroyPeer(
  GPConnection * connection,
  GPIPeer * peer
);

void
gpiRemovePeer(
  GPConnection * connection,
  GPIPeer * peer
);

GPResult
gpiPeerAddMessage(
  GPConnection * connection,
  GPIPeer * peer,
  int type,
  const char * message
);

GPResult
gpiPeerStartTransferMessage(
  GPConnection * connection,
  GPIPeer * peer,
  int type,
  const struct GPITransferID_s * transferID
);

GPResult
gpiPeerFinishTransferMessage(
  GPConnection * connection,
  GPIPeer * peer,
  const char * message,
  int len
);

GPResult
gpiPeerSendMessages(
  GPConnection * connection,
  GPIPeer * peer
);

void gpiPeerLeftCallback(unsigned int ip, unsigned short port, GSUdpCloseReason reason, void *userData);
void gpiPeerMessageCallback(unsigned int ip, unsigned short port, unsigned char *message, 
							unsigned int messageLength, gsi_bool reliable, void *userData);
void gpiPeerAcceptedCallback(unsigned int ip, unsigned short port, 
							 GSUdpErrorCode error, gsi_bool rejected, void *userData);
void gpiPeerPingReplyCallback(unsigned int ip, unsigned short port, unsigned int latency, void *userData);

void gpiPeerAddOp(GPIPeer *peer, GPIPeerOp *operation);
void gpiPeerRemoveOp(GPIPeer *peer, GPIPeerOp *operation);
void gpiCheckTimedOutPeerOperations(GPConnection * connection, GPIPeer * peer);
#endif
