////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef __GS_UDP_ENGINE_H__
#define __GS_UDP_ENGINE_H__

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// UDP Communication Engine
//
#include "gsCommon.h"
#include "../gt2/gt2.h"
#include "../darray.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Constants

// The UDP Engine should try the MSS (Maximum Segment Size = IP Packet size - IP Header)
// as a default size to keep the packets from being fragmented.  Currently 1460 is the
// MSS for windows.  Consoles may have a different size.
#define GS_UDP_DEFAULT_OUT_BUFFSIZE  1460
#define GS_UDP_DEFAULT_IN_BUFFSIZE 1500

// a default size for address strings
#define GS_IP_ADDR_AND_PORT          22

// A fixed header len. unless we require a bigger size, it will stay this size.
// These need to be used for calculating the free space available on
// the outgoing buffers for an IP and Port which represent the peer.
#define GS_UDP_MSG_HEADER_LEN        16
#define GS_UDP_RELIABLE_MSG_HEADER   7

// The following error codes will be given back to the higher level app 
// or message handler.
typedef enum _GSUdpErrorCode
{
	GS_UDP_NO_ERROR,
	GS_UDP_NO_MEMORY,
	GS_UDP_REJECTED,
	GS_UDP_NETWORK_ERROR,
	GS_UDP_ADDRESS_ERROR,
	GS_UDP_ADDRESS_ALREADY_IN_USE,
	GS_UDP_TIMED_OUT,
	GS_UDP_REMOTE_ERROR,
	GS_UDP_SEND_FAILED,
	GS_UDP_INVALID_MESSAGE,
	GS_UDP_PARAMETER_ERROR,
	GS_UDP_NOT_INITIALIZED,
	GS_UDP_MSG_TOO_BIG,
	GS_UDP_UNKNOWN_ERROR,
	// Add new errors before here
	GS_UDP_NUM_ERROR_CODES
} GSUdpErrorCode;

// Used so that an app or message handler does 
// not need to request to start talking to a peer twice
// Also lets higher level app or message handlers know about
// if a communication channel to a peer has been broken.
typedef enum _GSUdpPeerState
{
	GS_UDP_PEER_CONNECTING,
	GS_UDP_PEER_CONNECTED,
	GS_UDP_PEER_CLOSING,
	GS_UDP_PEER_CLOSED,
	// Add new connection state before here
	GS_UDP_PEER_STATE_NUM
} GSUdpPeerState;

// When a communication channel to a peer is closed
// the closed callback will let us know why it was closed
typedef enum _GSUdpCloseReason
{
	GS_UDP_CLOSED_LOCALLY,
	GS_UDP_CLOSED_REMOTELY,
	// errors:
	GS_UDP_CLOSED_BY_COMM_ERROR, // An invalid message was received (it was either unexpected or wrongly formatted).
	GS_UDP_CLOSED_BY_LOW_MEM,
	// Add new reasons before here
	GS_UDP_CLOSED_NUM
} GSUdpCloseReason;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Callbacks

// Errors to give higher layers feedback
typedef void (*gsUdpErrorCallback)(GSUdpErrorCode theCode, void *theUserData);


// app Request attempt callback used to tell registered listeners about connection attempts
typedef void (*gsUdpAppConnectAttemptCallback)(unsigned int theIp, unsigned short thePort, 
											   int theLatency, unsigned char *theInitMsg, 
											   unsigned int theInitMsgLen, void *theUserData);


// peer communication channel callback types
typedef void (*gsUdpConnClosedCallback)(unsigned int ip, unsigned short port, GSUdpCloseReason reason, 
										void *theUserData);
typedef void (*gsUdpConnReceivedDataCallback)(unsigned int ip, unsigned short port, 
											  unsigned char *message, unsigned int messageLength, 
											  gsi_bool reliable, void *theUserData);
typedef void (*gsUdpConnConnectedCallback)(unsigned int ip, unsigned short port, 
										   GSUdpErrorCode error, gsi_bool rejected, 
										   void *theUserData);
typedef void (*gsUdpConnPingCallback)(unsigned int ip, unsigned short port, unsigned int latency, 
									  void *theUserData);


// Messages that cannot be interpreted are passed on to the higher level app
typedef gsi_bool (*gsUdpUnknownMsgCallback)(unsigned int ip, unsigned short port, 
										unsigned char *message, unsigned int messageLength, 
										void *theUserData);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Public Functionality
// Initialization and test functions
gsi_bool gsUdpEngineIsInitialized();
GSUdpErrorCode gsUdpEngineInitialize(unsigned short thePort, int theIncomingBufSize, 
									 int theOutgoingBufSize, gsUdpErrorCallback theAppNetworkError, 
									 gsUdpConnConnectedCallback theAppConnected,
									 gsUdpConnClosedCallback theAppClosed,
									 gsUdpConnPingCallback theAppPing,
									 gsUdpConnReceivedDataCallback theAppReceive,
									 gsUdpUnknownMsgCallback theAppUnownMsg,
									 gsUdpAppConnectAttemptCallback theAppConnectAttempt,
									 void *theAppUserData);
// update and shutdown 
GSUdpErrorCode gsUdpEngineThink();
GSUdpErrorCode gsUdpEngineShutdown();

// Connectivity functions
GSUdpErrorCode gsUdpEngineGetPeerState(unsigned int theIp, unsigned short thePort, 
									   GSUdpPeerState *thePeerState);
GSUdpErrorCode gsUdpEngineStartTalkingToPeer(unsigned int theIp, unsigned short thePort,
											 char theInitMsg[GS_UDP_MSG_HEADER_LEN], int timeOut);
GSUdpErrorCode gsUdpEngineAcceptPeer(unsigned int theIp, unsigned short thePort);
GSUdpErrorCode gsUdpEngineRejectPeer(unsigned int theIp, unsigned short thePort);

// Sending functionality
// WARNING: Messages should not be greater than the outgoing buffer size minus the header 
// and the 7 byte header for reliable messages (used for internal gt2 operations). Most
// UDP fragmentation occurs if messages are bigger than 1500 bytes.  Also, some routers are 
// known to drop those packets that are larger than 1500 bytes because of set MTU sizes.  
// The recommended outgoing buffer size is the default (1460).  So take that, and subtract 
// 16 for message handler header and reliable message header (if sending data reliably).
// freeSpace = 1460 - 16 - 7
GSUdpErrorCode gsUdpEngineSendMessage(unsigned int theIp, unsigned short thePort, 
									  char theHeader[GS_UDP_MSG_HEADER_LEN], unsigned char *theMsg, 
									  unsigned int theMsgLen, gsi_bool theReliable);

// This function should be called for those parts of the code that want specific handling of messages
// Any call to send should include the header registered here.  
GSUdpErrorCode gsUdpEngineAddMsgHandler(char theInitMsg[GS_UDP_MSG_HEADER_LEN], 
										char theHeader[GS_UDP_MSG_HEADER_LEN], 
										gsUdpErrorCallback theMsgHandlerError, 
										gsUdpConnConnectedCallback theMsgHandlerConnected, 
										gsUdpConnClosedCallback theMsgHandlerClosed, 
										gsUdpConnPingCallback theMsgHandlerPing,
										gsUdpConnReceivedDataCallback theMsgHandlerRecv,
										void *theUserData);
GSUdpErrorCode gsUdpEngineRemoveMsgHandler(char theHeader[GS_UDP_MSG_HEADER_LEN]);
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Public Utility functionality
SOCKET gsUdpEngineGetSocket();
void gsUdpEngineAddrToString(unsigned int theIp, unsigned short thePort, 
							 char addrstring[GS_IP_ADDR_AND_PORT]);
unsigned int gsUdpEngineGetLocalAddr();
unsigned short gsUdpEngineGetLocalPort();

// lets the app or message handler know if it is able to shutdown the udp layer
gsi_bool gsUdpEngineNoMoreMsgHandlers();
gsi_bool gsUdpEngineNoApp();

// check the remaining free space on the outgoing buffer for the peer based 
// IP and port
int gsUdpEngineGetPeerOutBufferFreeSpace(unsigned int theIp, unsigned short thePort);

#endif
