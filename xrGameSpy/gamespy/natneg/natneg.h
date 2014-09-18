
/******
GameSpy NAT Negotiation SDK
  
Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com

******

 Please see the GameSpy NAT Negotiation SDK documentation for more 
 information

******/


#ifndef _NATNEG_H_
#define _NATNEG_H_
#include "../common/gsCommon.h"
#include "NATify.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 
NAT Negotiation Packet Magic Bytes
These bytes will start each incoming packet that is part of the NAT Negotiation SDK.
If you are sharing a game socket with the SDK, you can use these bytes to determine when to
pass a packet to NNProcessData
*/
#define NATNEG_MAGIC_LEN 6
#define NN_MAGIC_0 0xFD
#define NN_MAGIC_1 0xFC
#define NN_MAGIC_2 0x1E
#define NN_MAGIC_3 0x66
#define NN_MAGIC_4 0x6A
#define NN_MAGIC_5 0xB2

// This external array contains all 6 magic bytes - you can use it with memcmp to quickly check incoming packets for the bytes
extern unsigned char NNMagicData[];

/*
Possible states for the SDK. The two you will be notified for are:
ns_initack - when the NAT Negotiation server acknowledges your connection request
ns_connectping - when direct negotiation with the other client has started
*/
typedef enum {ns_initsent, ns_initack, ns_connectping, ns_finished, ns_canceled, ns_reportsent, ns_reportack } NegotiateState;

/*
Possible reslts of the negotiation.
nr_success: Successful negotiation, other parameters can be used to continue communications with the client
nr_deadbeatpartner: Partner did not register with the NAT Negotiation Server
nr_inittimeout: Unable to communicate with NAT Negotiation Server
nr_unknownerror: NAT Negotiation server indicated an unknown error condition
*/
typedef enum {nr_success, nr_deadbeatpartner, nr_inittimeout, nr_pingtimeout, nr_unknownerror, nr_noresult } NegotiateResult;

/*
Possible errors that can be returned when starting a negotiation
ne_noerror: No error
ne_allocerror: Memory allocation failed
ne_socketerror: Socket allocation failed
ne_dnserror: DNS lookup failed
*/
typedef enum {ne_noerror, ne_allocerror, ne_socketerror, ne_dnserror} NegotiateError;


//Callback prototype for your progress function
typedef void (*NegotiateProgressFunc)(NegotiateState state, void *userdata);

//Callback prototype for your negotiation completed function
typedef void (*NegotiateCompletedFunc)(NegotiateResult result, SOCKET gamesocket, struct sockaddr_in *remoteaddr, void *userdata);

//Callback prototype for your NAT detection results function
typedef void (*NatDetectionResultsFunc)(gsi_bool success, NAT nat);


/*
NNBeginNegotiation
-------------------
Starts the negotiation process.
cookie: Shared cookie value that both players will use so that the NAT Negotiation Server can match them up.
clientindex: One client must use clientindex 0, the other must use clientindex 1. 
progresscallback: Callback function that will be called as the state changes
completedcallback: Callback function that will be called when negotiation is complete.
userdata: Pointer for your own use that will be passed into the callback functions.
*/
NegotiateError NNBeginNegotiation(int cookie, int clientindex, NegotiateProgressFunc progresscallback, NegotiateCompletedFunc completedcallback, void *userdata);


/*
NNBeginNegotiationWithSocket
-------------------
Starts the negotiation process using the socket provided, which will be shared with the game.
Incoming traffic is not processed automatically - you will need to read the data off the socket and pass NN packets to NNProcessData
*/
NegotiateError NNBeginNegotiationWithSocket(SOCKET gamesocket, int cookie, int clientindex, NegotiateProgressFunc progresscallback, NegotiateCompletedFunc completedcallback, void *userdata);


/*
NNThink
-------------------
Processes any negotiation requests that are in progress
*/
void NNThink();


/*
NNProcessData
-------------------
When sharing a socket with the NAT Negotiation SDK, you must read incoming data and pass packets that start the the NN magic bytes
to this function for processing, along with the address they came from.
*/
void NNProcessData(char *data, int len, struct sockaddr_in *fromaddr);


/*
NNCancel
-------------------
Cancels a NAT Negotiation request in progress
*/
void NNCancel(int cookie);


/*
NNFreeNegotiateList
-------------------
De-allocates the memory used by for the negotiate list when you are done with NAT Negotiation.
The list will be re-allocated at a later time if you start additional negotiations.
If any negotiations are outstanding this will cancel them.
*/
void NNFreeNegotiateList();


/*
NNStartNatDetection
-------------------
Detects if there is network address translation going on between the machine and the Internet.
*/
NegotiateError NNStartNatDetection(NatDetectionResultsFunc resultscallback);


//Used for over-riding the default negotiation hostnames. Should not be used normally.
extern char *Matchup2Hostname;
extern char *Matchup1Hostname;

#ifdef __cplusplus
}
#endif

#endif
