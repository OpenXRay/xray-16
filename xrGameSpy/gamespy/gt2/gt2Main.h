/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _GT2_MAIN_H_
#define _GT2_MAIN_H_

#include "gt2.h"
#include "../darray.h"
#include "../hashtable.h"
#include "gt2Auth.h"

/*************************
** CONFIGURABLE DEFINES **
*************************/

// these defines are internal to GT2 and are NOT guaranteed to persist from version to version.


//  If set, this will convert all big endian vars to little endian before sending accross the net
//	And on big endian machines, convert little endian to big endian on recv
//#define	_GT2_ENDIAN_CONVERT_ENABLE	// add this to your compiler pre-processor options

#if defined GSI_BIG_ENDIAN && defined _GT2_ENDIAN_CONVERT_ENABLE
	#define	_GT2_ENDIAN_CONVERT
#endif



// any unreliable application message that starts with this magic string will have extra overhead.
// the string can be changed to something that your application will not use, or not use frequently.
// the only impact of this change will be to make your application incomatible with other application's
// using either the original or another different magic string.
// the string can consist of any number of characters, as long as there's at least one character, and the
// length define matches the string's length.
#define GTI2_MAGIC_STRING         "\xFE\xFE"
#define GTI2_MAGIC_STRING_LEN     2

// the size of the buffer into which GT2 directly receives messages.  this buffer is declared on the stack,
// and so can be fairly large on most systems without having any impact.  however, on some systems with small
// stacks, this size can overflow the stack, in which case it should be lowered.
// note, this buffer size only needs to be slighty larger than the largest message that will be sent ("slighty
// larger" due to overhead with reliable messages, and unreliable messages starting with the magic string).
#if defined(_PS2) && defined(INSOCK)
	#define GTI2_STACK_RECV_BUFFER_SIZE  NETBUFSIZE		// Max for Insock. Otherwise SOCKET_ERROR
#elif defined(_NITRO)
	#define GTI2_STACK_RECV_BUFFER_SIZE  1500
#elif defined (_XBOX)									// Xbox packets are 1304,  
	#define GTI2_STACK_RECV_BUFFER_SIZE  4096			// when using VDP sockets, 2 bytes are used for data length
#else													
	#define GTI2_STACK_RECV_BUFFER_SIZE  65535
#endif

// a server will disconnect a client that doesn't not successfully connect within this time (in milliseconds).
// if the connectAttemptCallback has been called, and GT2 is awaiting an accept/reject, the attempt will
// not be timed-out (although the client may abort the attempt at any time).
#define GTI2_SERVER_TIMEOUT     (1 * 60 * 1000)
// the time (in milliseconds) GT2 waits between resending a message whose delivery has not yet been confirmed.
#define GTI2_RESEND_TIME        1000
// the time (in milliseconds) GT2 waits after receiving a message it must acknowledge before it actually sends
// the ack.  this allows it to combine acks, or include acks as part of other reliable messages it sends.
// if an ack is pending, a new incoming message does not reset this timer.
#define GTI2_PENDING_ACK_TIME   100
// if GT2 does not send a message for this amount of time (in milliseconds), it sends a keep-alive message.
#define GTI2_KEEP_ALIVE_TIME    (30 * 1000)
// if this is defined, it sets the percentage of sent datagrams to drop.  this is good for simulating what will
// happen on a high packet loss connection.
//#define GTI2_DROP_SEND_RATE     30
typedef enum
{
	GTI2UdpProtocol,			// UDP socket type for standard sockets
	GTI2VdpProtocol		= 2,	// VDP socket type only used for Xbox VDP sockets
	GTI2AdHocProtocol	= 3		// socket type only used for PSP Adhoc sockets
} GTI2ProtocolType;

// The Maximum offset of eiter UDP or VDP
// measured in bytes
// used as a buffer offset
#define MAX_PROTOCOL_OFFSET 2

/**********
** TYPES **
**********/

typedef enum
{
	// client-only states
	GTI2AwaitingServerChallenge,  // sent challenge, waiting for server's challenge
	GTI2AwaitingAcceptance,       // sent response, waiting for accept/reject from server

	// server-only states
	GTI2AwaitingClientChallenge,  // receiving challenge from a new client
	GTI2AwaitingClientResponse,   // sent challenge, waiting for client's response
	GTI2AwaitingAcceptReject,     // got client's response, waiting for app to accept/reject

	// post-negotiation states
	GTI2Connected,                // connected
	GTI2Closing,                  // sent a close message (GTI2Close or GTI2Reject), waiting for confirmation
	GTI2Closed                    // connection has been closed, free it as soon as possible
} GTI2ConnectionState;

// message types
typedef enum
{
	// reliable messages
	// all start with <magic-string> <type> <serial-number> <expected-serial-number>
	// type is 1 bytes, SN and ESN are 2 bytes each
	GTI2MsgAppReliable,       // reliable application message
	GTI2MsgClientChallenge,   // client's challenge to the server (initial connection request)
	GTI2MsgServerChallenge,   // server's response to the client's challenge, and his challenge to the client
	GTI2MsgClientResponse,    // client's response to the server's challenge
	GTI2MsgAccept,            // server accepting client's connection attempt
	GTI2MsgReject,            // server rejecting client's connection attempt
	GTI2MsgClose,             // message indicating the connection is closing
	GTI2MsgKeepAlive,         // keep-alive used to help detect dropped connections

	GTI2NumReliableMessages,

	// unreliable messages
	GTI2MsgAck = 100,         // acknowledge receipt of reliable message(s)
	GTI2MsgNack,              // alert sender to missing reliable message(s)
	GTI2MsgPing,              // used to determine latency
	GTI2MsgPong,              // a reply to a ping
	GTI2MsgClosed             // confirmation of connection closure (GTI2MsgClose or GTI2MsgReject) - also sent in response to bad messages from unknown addresses

	// unreliable messages don't really have a message type, just the magic string repeated at the start
} GTI2MessageType;

/***************
** STRUCTURES **
***************/

typedef struct GTI2Buffer
{
	GT2Byte * buffer;         // The buffer's bytes.
	int size;                 // Number of bytes in buffer.
	int len;                  // Length of actual data in buffer.
} GTI2Buffer;

typedef struct GTI2IncomingBufferMessage
{
	int start;  // the start of the message
	int len;  // the length of the message
	GTI2MessageType type;  // the type
	unsigned short serialNumber;  // the serial number
} GTI2IncomingBufferMessage;

typedef struct GTI2OutgoingBufferMessage
{
	int start;  // the start of the message
	int len;  // the length of the message
	unsigned short serialNumber;  // the serial number
	gsi_time lastSend;  // last time this message was sent
} GTI2OutgoingBufferMessage;

typedef struct GTI2Socket
{
	SOCKET socket;  // the network socket used for all network communication

	unsigned int ip;  // the ip this socket is bound to
	unsigned short port;  // the port this socket is bound to

	HashTable connections;  // the connections that are using this socket
	DArray closedConnections;  // connections that are closed no longer get a spot in the hash table

	GT2Bool close;  // if true, a close was attempted inside a callback, and it should be closed as soon as possible
	GT2Bool error;  // if true, there was a socket error using this socket

	int callbackLevel;  // if >0, then we're inside a callback (or recursive callbacks)
	gt2ConnectAttemptCallback connectAttemptCallback;  // if set, callback used to handle incoming connection attempts
	gt2SocketErrorCallback socketErrorCallback;  // if set, call this in case of an error
	gt2DumpCallback sendDumpCallback;  // if set, gets called for every datagram sent
	gt2DumpCallback receiveDumpCallback;  // if set, gets called for every datagram and connection reset received
	gt2UnrecognizedMessageCallback unrecognizedMessageCallback;  // if set, gets called for all unrecognized messages

	void * data;  // user data

	int outgoingBufferSize;  // per-connection buffer sizes
	int incomingBufferSize;

	GTI2ProtocolType protocolType;  // set to UDP or VDP protocol depending on the call to create socket
									// also used as an offset for VDP sockets
	int protocolOffset;
	GT2Bool broadcastEnabled;  // set to true if the socket has already been broadcast enabled
} GTI2Socket;

typedef struct GTI2Connection
{
	// ip and port uniquely identify this connection on this socket
	unsigned int ip;  // the ip on the other side of this connection (network byte order)
	unsigned short port;  // the port on the other side of this connection (host byte order)

	GTI2Socket * socket;  // the parent socket

	GTI2ConnectionState state;  // connection state

	GT2Bool initiated;  // if true, the local side of the connection initiated the connection (client)

	GT2Bool freeAtAcceptReject;  // if true, don't free the connection until accept/reject is called

	GT2Result connectionResult;  // the result of the connect attempt

	gsi_time startTime;  // the time the connection was created
	gsi_time timeout;  // the timeout value passed into gt2Connect

	int callbackLevel;  // if >0, then we're inside a callback (or recursive callbacks)
	GT2ConnectionCallbacks callbacks;  // connection callbacks
	
	char * initialMessage;  // this is the initial message for the client
	int initialMessageLen;  // the initial message length

	void * data;  // user data

	GTI2Buffer incomingBuffer;  // buffer for incoming data
	GTI2Buffer outgoingBuffer;  // buffer for outgoing data
	DArray incomingBufferMessages;  // identifies incoming messages stored in the buffer
	DArray outgoingBufferMessages;  // identifies outgoing messages stored in the buffer

	unsigned short serialNumber;  // serial number of the next message to be sent out
	unsigned short expectedSerialNumber;  // the next serial number we're expecting from the remote side

	char response[GTI2_RESPONSE_LEN];  // after the challenge is sent during negotiation, this is the response we're expecting

	gsi_time lastSend;  // the last time something was sent on this connection
	gsi_time challengeTime;  // the time the challenge was sent

	GT2Bool pendingAck;  // if true, there is an ack waiting to go out, either on its own or as part of a reliable message

	gsi_time pendingAckTime;  // the time at which the pending ack was first set
	
	DArray sendFilters;  // filters that apply to outgoing data
	DArray receiveFilters;  // filters that apply to incoming data

} GTI2Connection;

// store last 32 ip's in a ring buffer
#define MAC_TABLE_SIZE 32	// must be power of 2
typedef struct
{
	gsi_u32 ip;
	char	mac[6];
} GTI2MacEntry;

#ifdef GSI_ADHOC
static int lastmactableentry = 0;
static GTI2MacEntry MacTable[MAC_TABLE_SIZE];
#endif // GSI_ADHOC

#endif
