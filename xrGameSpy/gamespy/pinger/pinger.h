/*
GameSpy Ping SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PINGER_H_
#define _PINGER_H_

#include "../common/gsCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/************
** DEFINES **
************/
// This controls the size of UDP pings.
// The protocol takes up 8 of these bytes.
// The rest can be used by the app.
//////////////////////////////////////////
#ifndef PINGER_UDP_PING_SIZE
#define PINGER_UDP_PING_SIZE     32
#endif

// Value for ping if a ping timed-out.
//////////////////////////////////////
#define PINGER_TIMEOUT           -1

/**********
** TYPES **
**********/
typedef enum { PINGERFalse, PINGERTrue } PINGERBool;

/**************
** CALLBACKS **
**************/
// A function of this type is used as:
// a param to pingerInit, gets called when an uninitiated UDP ping is received
// a param to pingerPing, gets called when the ping is completed, or times out
// if the ping times out, ping is PINGER_TIMEOUT
// IP and port are in network byte order
//////////////////////////////////////////////////////////////////////////////
typedef void (* pingerGotPing)(unsigned int IP,
							   unsigned short port,
							   int ping,
							   const char * data,
							   int len,
							   void * param);

// When a ping is sent, this callback gets called so that
// the application can use the ping bytes not being used
// by the protocol.
// The application can write up to len bytes to data.
// IP and port are in network byte order.
/////////////////////////////////////////////////////////
typedef void (* pingerSetData)(unsigned int IP,
							   unsigned short port,
							   char * data,
							   int len,
							   void * param);

/**************
** FUNCTIONS **
**************/

// Called once to initialize the pinger library.
// localPort is in host byte order
// localAddress=NULL : don't specify a local interface
// localPort=0 : not doing UDP pings
//////////////////////////////////////////////////////
PINGERBool pingerInit(const char * localAddress,
					  unsigned short localPort,
					  pingerGotPing pinged,
					  void * pingedParam,
					  pingerSetData setData,
					  void * setDataParam);

// Called to clean-up when done with pinger.
////////////////////////////////////////////
void pingerShutdown(void);

// Called to do processing.
// The more frequently this function is called,
// the more accurate the pings will be.
//
// Example: if this func is called once every 20ms, and a ping
// is returned as 100ms, then the ping is in the range 90-110.
//////////////////////////////////////////////////////////////
void pingerThink(void);

// Does a ping.  If blocking, does not return until the ping is completed.
// timeout specifes how long (in milliseconds) to wait for the ping reply.
// A value of 0 means wait forever.  Note, this is dangerous because pings
// are not reliable.  A blocking ping with a 0 timeout will never return if
// the ping is lost.
// port=0 : ICMP ping (this is NOT currently supported)
// port!=0 : UDP ping
// IP and port are in network byte order
//////////////////////////////////////////////////////////////////////////
void pingerPing(unsigned int IP,
				unsigned short port,
				pingerGotPing reply,
				void * replyParam,
				PINGERBool blocking,
				gsi_time timeout);
	
#ifdef __cplusplus
}
#endif

#endif
