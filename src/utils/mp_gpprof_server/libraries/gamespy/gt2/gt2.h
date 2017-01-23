/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

/****************************
** GameSpy Transport SDK 2 **
****************************/

/*
** see "configurable defines" in gt2Main.h for certain performance settings that can be changed
*/

#ifndef _GT2_H_
#define _GT2_H_

#include "../common/gsCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********
** TYPES **
**********/

// boolean
typedef int GT2Bool;
#define GT2False 0
#define GT2True 1

// a byte
typedef unsigned char GT2Byte;

// a handle to a socket object (can be used to accept connections and initiate connections)
typedef struct GTI2Socket * GT2Socket;

// a handle to an object representing a connection to a specific IP and port
// the local endpoint is a GT2Socket
typedef struct GTI2Connection * GT2Connection;

// the id of a reliably sent message
// unreliable messages don't have ids
typedef unsigned short GT2MessageID;

// the result of a GT2 operation
// check individual function definitions to see possible results
// TODO: list possible results wherever this is used
typedef enum
{
	GT2Success,             // success
                            // errors:
	GT2OutOfMemory,         // ran out of memory
	GT2Rejected,            // attempt rejected
	GT2NetworkError,        // networking error (could be local or remote)
	GT2AddressError,        // invalid or unreachable address
	GT2DuplicateAddress,    // a connection was attempted to an address that already has a connection on the socket
	GT2TimedOut,			// time out reached
	GT2NegotiationError,	// there was an error negotiating with the remote side
	GT2InvalidConnection,	// the connection didn't exist
	GT2InvalidMessage,		// used for vdp reliable messages containing voice data, no voice data in reliable messages
	GT2SendFailed			// the send failed,
} GT2Result;

// possible states for any GT2Connection
typedef enum
{
	GT2Connecting,   // negotiating the connection
	GT2Connected,    // the connection is active
	GT2Closing,      // the connection is being closed
	GT2Closed        // the connection has been closed and can no longer be used
} GT2ConnectionState;

// The cause of the connection being closed.
typedef enum
{
	GT2LocalClose,         // The connection was closed with gt2CloseConnection.
	GT2RemoteClose,        // The connection was closed remotely.
	                       // errors:
	GT2CommunicationError, // An invalid message was received (it was either unexpected or wrongly formatted).
	GT2SocketError,        // An error with the socket forced the connection to close.
	GT2NotEnoughMemory     // There wasn't enough memory to store an incoming or outgoing message.
} GT2CloseReason;

/************
** GLOBALS **
************/

// The challenge key is a 32 character string
// that is used in the authentication process.
// The key can be set before GT2 is used so
// that the key will be application-specific.
extern char GT2ChallengeKey[33];

/*********************
** SOCKET CALLBACKS **
*********************/

// this callback gets called when there was is an error that forces a socket to close
// all connections that use this socket are terminated, and their gt2CloseCallback callbacks
// will be called before this callback is called (with the reason set to GT2SocketError).
// the socket cannot be used again after this callback returns
typedef void (* gt2SocketErrorCallback)
(
	GT2Socket socket
);

/*********************
** SOCKET FUNCTIONS **
*********************/

// creates a local socket
// if the IP of the local address is 0, then any/all ips will be bound.
// if the port of the local address is 0, then a port will be assigned.
// if either buffer sizes is set to 0, a default value will be used (currently 64K for PC, 4k for Xbox).
// the buffer needs to be able to hold all messages waiting for confirmation of delivery,
// and it needs to hold any messages that arrive out of order. if either buffer runs out
// of space the connection will be dropped.
GT2Result gt2CreateSocket
(
	GT2Socket * socket,  // if the result is GT2Success, the socket object handle will be stored at this address
	const char * localAddress,  // the local address to bind to
	int outgoingBufferSize,  // size of per-connection buffer where sent messages waiting to be confirmed are held, use 0 for default
	int incomingBufferSize,  // size of per-connection buffer where out-of-order received messages are held, use 0 for default
	gt2SocketErrorCallback callback  // a callback that is called if there is an error with the socket
);

// AdHoc Sockets use MAC address instead of IP address.
GT2Result gt2CreateAdHocSocket
(
	GT2Socket * socket,			// if the result is GT2Success, the socket object handle will be stored at this address
	const char * localAddress,  // the local address to bind to
	int outgoingBufferSize,		// size of per-connection buffer where sent messages waiting to be confirmed are held, use 0 for default
	int incomingBufferSize,		// size of per-connection buffer where out-of-order received messages are held, use 0 for default
	gt2SocketErrorCallback callback  // a callback that is called if there is an error with the socket
);

#ifdef _XBOX
// creates a local VDP socket on the Xbox platform
// if the IP of the local address is 0, then any/all ips will be bound.
// if the port of the local address is 0, then a port will be assigned.
// if either buffer sizes is set to 0, a default value will be used (currently 4K).
// the buffer needs to be able to hold all messages waiting for confirmation of delivery,
// and it needs to hold any messages that arrive out of order. if either buffer runs out
// of space the connection will be dropped.
GT2Result gt2CreateVDPSocket
(
	GT2Socket * socket,  // if the result is GT2Success, the socket object handle will be stored at this address
	const char * localAddress,  // the local address to bind to
	int outgoingBufferSize,  // size of per-connection buffer where sent messages waiting to be confirmed are held, use 0 for default
	int incomingBufferSize,  // size of per-connection buffer where out-of-order received messages are held, use 0 for default
	gt2SocketErrorCallback callback  // a callback that is called if there is an error with the socket
);
#endif

// closes a local socket.
// all existing connections will be hard closed, as if gt2CloseAllConnectionsHard was
// called for this socket.  all connections send a close message to the remote side,
// and any closed callbacks will be called from within this function
void gt2CloseSocket(GT2Socket socket);

// processes a socket (and all associated connections)
void gt2Think(GT2Socket socket);

// sends a raw UDP datagram through the socket
// this function bypasses the normal connection logic
// note that all messages sent this way will be unreliable
// to broadcast a datagram, omit the IP from the remoteAddress (e.g., ":12345")
GT2Result gt2SendRawUDP
(
	GT2Socket socket,  // the socket through which to send the raw UDP datagram
	const char * remoteAddress,  // the address to which to send the datagram
	const GT2Byte * message,  // the message to send, or NULL for an empty datagram
	int len  // the len of the message (0 for an empty message, ignored if message==NULL)
);

/*************************
** CONNECTION CALLBACKS **
*************************/

// Called when the connect has completed.
// If the result is GT2Rejected,
// then message is the message that the
// listener passed to gt2Reject.  If the
// result is anything else, then message
// is NULL and len is 0.
typedef void (* gt2ConnectedCallback)
(
	GT2Connection connection,       // The connection object.
	GT2Result result,               // Result from connect attempt.
	GT2Byte * message,              // If result==GT2Rejected, the reason.  Otherwise, NULL.
	int len                         // If result==GT2Rejected, the length of the reason.  Otherwise, 0.
);

// Called when a message is received.
typedef void (* gt2ReceivedCallback)
(
	GT2Connection connection,       // The connection the message was received on.
	GT2Byte * message,              // The message that was received.  Will be NULL if an empty message.
	int len,                        // The length of the message in bytes.  Will be 0 if an empty message.
	GT2Bool reliable                // True if this is was sent reliably.
);

// Called when the connection is closed (remotely or locally).
// The connection can no longer be used after this callback returns.
typedef void (* gt2ClosedCallback)
(
	GT2Connection connection,       // The connection that was closed.
	GT2CloseReason reason           // The reason the connection was closed.
);

// When a reply is received for a ping that was sent, this callback is called.
// The latency reported here is the amount of time between when the ping
// was first sent with gt2Ping and when the pong was received.
typedef void (* gt2PingCallback)
(
	GT2Connection connection,        // the connection the ping was sent on
	int latency                      // the round-trip time for the ping, in milliseconds
);

// Callbacks set for each connection.
// The connected callback is ignored
// when this is passed to gt2Accept.
typedef struct
{
	gt2ConnectedCallback connected; // Called when gt2Connect is complete.
	gt2ReceivedCallback received;   // Called when a message is received.
	gt2ClosedCallback closed;       // Called when the connection is closed (remotely or locally).
	gt2PingCallback ping;           // Called when a ping reply is received.
} GT2ConnectionCallbacks;

/*************************
** CONNECTION FUNCTIONS **
*************************/

// initiates a connection between a local socket and a remote socket
// if blocking is true, the return value signals the connection result:
//   GT2Success means the connect attempt succeeded
//   anything else means it failed
// if blocking is false, the return value signals the current status of the attempt
//   GT2Success means the connection is being attempted
//   anything else means there was an error and the connection attempt has been aborted
GT2Result gt2Connect
(
	GT2Socket socket,  // the local socket to use for the connection
	GT2Connection * connection,  // if the result is GT2Success, and blocking is false, the connection  object handle is stored here
	const char * remoteAddress,  // the address to connect to
	const GT2Byte * message,  // an optional initial message (may be NULL)
	int len,  // length of the initial message (may be 0, or -1 for strlen)
	int timeout,  // timeout in milliseconds (may be 0 for infinite retries)
	GT2ConnectionCallbacks * callbacks,  // callbacks for connection related stuff
	GT2Bool blocking  // if true, don't return until complete (successfuly or unsuccessfuly)
);

// sends data reliably or unreliably
// reliable messages are guaranteed to arrive, arrive in order, and arrive only once.
// unreliable messages are not guaranteed to arrive, arrive in order, or arrive only once.
// because messages may be held in the outgoing buffer (even unreliable messages may need
// to be put in the buffer), the message size cannot exceed
GT2Result gt2Send
(
	GT2Connection connection,  // the connection to send the message on
	const GT2Byte * message,  // the message to send, or NULL for an empty message0
	int len,  // the len of the message (0 for an empty message, ignored if message==NULL)
	GT2Bool reliable  // if true, send the message reliably
);

// sends a ping on a connection in an attempt to determine latency
// the ping is unreliable, and either it or the pong sent in reply
// could be dropped (resulting in the callback never being called),
// or it could even arrive multiple times (resulting in multiple
// calls to the callback).
void gt2Ping(GT2Connection connection);

// starts an attempt to close the connection
// when the close is completed, the connection's closed callback will be called
void gt2CloseConnection(GT2Connection connection);

// same as gt2CloseConnection, but doesn't wait for confirmation from the remote side of the connection
// the closed callback will be called from within this function
void gt2CloseConnectionHard(GT2Connection connection);

// closes all of a socket's connections (essentially calls gt2CloseConnection on each of them).
void gt2CloseAllConnections(GT2Socket socket);

// same as gt2CloseAllConnections, but does a hard close
// any closed callbacks will be called from within this function
void gt2CloseAllConnectionsHard(GT2Socket socket);

/*********************
** LISTEN CALLBACKS **
*********************/

// callback gets called when someone attempts to connect to a socket that is listening for new connections.
// in response to this callback the application should call gt2Accept or gt2Reject.  they do not need
// to be called from inside the callback, however they should be called in a timely manner so that the
// remote side does not need to sit around indefinitely waiting for a response.
// the latency is an estimate of the round trip time between connections.
typedef void (* gt2ConnectAttemptCallback)
(
	GT2Socket socket,  // the socket the attempt came in on
	GT2Connection connection,  // a connection object for the incoming connection attempt
	unsigned int ip,  // the IP being used remotely for the connection attempt
	unsigned short port,  // the port being used remotely for the connection attempt
	int latency,  // the approximate latency on the connection
	GT2Byte * message,  // an optional message sent with the attempt.  Will be NULL if an empty message.
	int len  // the length of the message, in characters.  Will be 0 if an empty message.
);

/*********************
** LISTEN FUNCTIONS **
*********************/

// tells a socket to start listening for incoming connections
// any connections attempts will cause the callback to be called
// if the socket is already listening, this callback will replace the exsiting callback being used
// if the callback is NULL, this will cause the connection to stop listening
void gt2Listen(GT2Socket socket, gt2ConnectAttemptCallback callback);

// after a socket's gt2ConnectAttemptCallback has been called, this function can be used to accept
// the incoming connection attempt.  it can be called from either within the callback or some later time.
// as soon as it is called the connection is active, and messages can be sent and received.  the remote side
// of the connection will have it's connected callback called with the result set to GT2Success.  the callbacks
// that are passed in to this function are the same callbacks that get passed to gt2Connect, with the exception
// that the connected callback can be ignored, as the connection is already established.
// if this function returns GT2True, then the connection has been successfully accepted.  if it returns
// GT2False, then the remote side has already closed the connection attempt.  in that case, the connection
// is considered closed, and it cannot be referenced again.
GT2Bool gt2Accept(GT2Connection connection, GT2ConnectionCallbacks * callbacks);

// after a socket's gt2ConnectAttemptCallback has been called, this function can be used to reject
// the incoming connection attempt.  it can be called from either within the callback or some later time.
// once the function is called the connection is considered closed and cannot be referenced again.  the remote
// side attempting the connection will have its connected callback called with the result set to GT2Rejected.
// if the message is not NULL and the len is not 0, the message will be sent with the rejection, and passed
// into the remote side's connected callback.
void gt2Reject(GT2Connection connection, const GT2Byte * message, int len);

/*************************
** MESSAGE CONFIRMATION **
*************************/
// gets the message id for the last reliably sent message. unreliable messages do not have ids.
// this should be called immediately after gt2Send.  waiting until after a call to gt2Think can result in
// an invalid message id being returned.
// note that the use of filters that can either drop or delay messages can complicate the process, because
// in those cases a call to gt2Send does not guarantee that a message will actually be sent.  in those cases,
// gt2GetLastSentMessageID should be called after gt2FilteredSend, because the actual message will be sent
// from within that function.
GT2MessageID gt2GetLastSentMessageID(GT2Connection connection);

// returns true if confirmation was received locally that the reliable message represented by the message id
// was received by the remote end of the connection.  returns false if confirmation was not yet received.
// this should only be called on message ids that were returned by gt2GetLastSendMessageID, and should be
// used relatively soon after the message was sent, due to message ids wrapping around after a period of time.
GT2Bool gt2WasMessageIDConfirmed(GT2Connection connection, GT2MessageID messageID);

/*********************
** FILTER CALLBACKS **
*********************/

// Callback for filtering outgoing data.
// Call gt2FilteredSend with the filtered data, either from within the callback or later.
// the message points to the same memory location as the message passed to gt2Send (or gt2FilteredSend).
// so if the call to gt2FilteredSend is delayed, it is the filter's responsibility to make sure the
// data is still around when and if it is needed.
typedef void (* gt2SendFilterCallback)
(
	GT2Connection connection,  // The connection on which the message is being sent.
	int filterID,              // Pass this ID to gt2FilteredSend.
	const GT2Byte * message,   // The message being sent.  Will be NULL if an empty message.
	int len,                   // The length of the message being sent, in bytes. Will be 0 if an empty message.
	GT2Bool reliable           // If the message is being sent reliably.
);

// Callback for filtering incoming data.
// Call gt2FilteredRecieve with the filtered data,
// either from within the callback or later.
// the message may point to a memory location supplied to gt2FilteredReceive by a previous filter.
// so if this filter's call to gt2FilteredReceive is delayed, it is the filter's responsibility
// to make sure the data is still around when and if it is needed.
typedef void (* gt2ReceiveFilterCallback)
(
	GT2Connection connection,       // The connection the message was received on.
	int filterID,                   // Pass this ID to gtFilteredReceive.
	GT2Byte * message,              // The message that was received.  Will be NULL if an empty message.
	int len,                        // The length of the message in bytes.  Will be 0 if an empty message.
	GT2Bool reliable                // True if this is a reliable message.
);

/*********************
** FILTER FUNCTIONS **
*********************/

// Adds a filter to the connection's outgoing data.
// Returns GT2False if there was an error adding the filter (due to no free memory)
GT2Bool gt2AddSendFilter
(
	GT2Connection connection,       // The connection on which to add the filter.
	gt2SendFilterCallback callback  // The callback the outgoing data is filtered through.
);

// Removes a filter from the connection's outgoing data.
// if callback is NULL, all send filters are removed
void gt2RemoveSendFilter
(
	GT2Connection connection,       // The connection on which to remove the filter.
	gt2SendFilterCallback callback  // The callback to remove.
);

// Called in response to a gt2SendFilterCallback being called.
// It can be called from within the callback, or at any later time.
void gt2FilteredSend
(
	GT2Connection connection,  // The connection on which the message is being sent.
	int filterID,              // The ID passed to the gt2SendFilterCallback.
	const GT2Byte * message,   // The message being sent. May be NULL.
	int len,                   // The lengt2h of the message being sent, in bytes. May be 0 or -1.
	GT2Bool reliable           // If the message should be sent reliably.
);

// Adds a filter to the connection's incoming data.
// Returns GT2False if there was an error adding the filter (due to no free memory)
GT2Bool gt2AddReceiveFilter
(
	GT2Connection connection,          // The connection on which to add the filter.
	gt2ReceiveFilterCallback callback  // The callback the incoming data is filtered through.
);

// Removes a filter from the connection's incoming data.
// if callback is NULL, all receive filters are removed
void gt2RemoveReceiveFilter
(
	GT2Connection connection,          // The connection on which to remove the filter.
	gt2ReceiveFilterCallback callback  // The callback to remove.
);

// Called in response to a gt2ReceiveFilterCallback being called.
// It can be called from within the callback, or at any later time.
void gt2FilteredReceive
(
	GT2Connection connection,       // The connection the message was received on.
	int filterID,                   // The ID passed to the gt2ReceiveFilterCallback.
	GT2Byte * message,              // The message that was received.  May be NULL.
	int len,                        // The lengt2h of the message in bytes.  May be 0.
	GT2Bool reliable                // True if this is a reliable message.
);

/*****************************
** SOCKET SHARING CALLBACKS **
*****************************/

// this callback gets called when the sock receives a message that it cannot match to an existing
// connection.  if the callback recognizes the message and handles it, it should return GT2True, which
// will tell the socket to ignore the message.  if the callback does not recognize the message, it
// should return GT2False, which tells the socket to let the other side know there is no connection.
typedef GT2Bool (* gt2UnrecognizedMessageCallback)
(
	GT2Socket socket,     // the socket the message was received on
	unsigned int ip,      // the ip of the remote machine the message came from (in network byte order)
	unsigned short port,  // the port on the remote machine (in host byte order)
	GT2Byte * message,    // the message (may be NULL for an empty message)
	int len               // the length of the message (may be 0)
);

/*****************************
** SOCKET SHARING FUNCTIONS **
*****************************/

// this function returns the actual underlying socket for a GT2Socket.
// this can be used for socket sharing purposes, along with the gt2UnrecognizedMessageCallback.
SOCKET gt2GetSocketSOCKET(GT2Socket socket);

// sets a callback that all unrecognized messages are passed to.  an unrecognized message is one
// that can't be matched up to a specific connection. if the callback handles the message, it
// returns true, and the GT2Socket ignores the message.  if the callback does not recognize the message,
// it returns false, and the socket handles the message (by sending a message back indicating the connection
// is closed).  if the callback is NULL, it removes any previously set callback.
void gt2SetUnrecognizedMessageCallback(GT2Socket socket, gt2UnrecognizedMessageCallback callback);

/*******************
** INFO FUNCTIONS **
*******************/

// gets the socket this connection exists on
GT2Socket gt2GetConnectionSocket(GT2Connection connection);

// gets the connection's connection state
// GT2Connecting - the connection is still being negotiated
// GT2Connected - the connection is active (has successfully connected, and not yet closed)
// GT2Closing - the connection is in the process of closing (i.e., sent a close message and waiting for confirmation).
// GT2Closed - the connection has already been closed and will soon be freed
GT2ConnectionState gt2GetConnectionState(GT2Connection connection);

// gets a connection's remote IP (in network byte order)
unsigned int gt2GetRemoteIP(GT2Connection connection);

// gets a connection's remote port (in host byte order)
unsigned short gt2GetRemotePort(GT2Connection connection);

// gets a socket's local IP (in network byte order)
unsigned int gt2GetLocalIP(GT2Socket socket);

// gets a socket's local port (in host byte order)
unsigned short gt2GetLocalPort(GT2Socket socket);

// gets the total size of the connection's incoming buffer.
int gt2GetIncomingBufferSize(GT2Connection connection);

// gets the amount of available space in the connection's incoming buffer.
int gt2GetIncomingBufferFreeSpace(GT2Connection connection);

// gets the total size of the connection's outgoing buffer.
int gt2GetOutgoingBufferSize(GT2Connection connection);

// gets the amount of available space in the connection's outgoing buffer.
int gt2GetOutgoingBufferFreeSpace(GT2Connection connection);

/************************
** USER DATA FUNCTIONS **
************************/

void gt2SetSocketData(GT2Socket socket, void * data);
void * gt2GetSocketData(GT2Socket socket);
void gt2SetConnectionData(GT2Connection connection, void * data);
void * gt2GetConnectionData(GT2Connection connection);

/*************************
** BYTE ORDER FUNCTIONS **
*************************/

unsigned int gt2NetworkToHostInt(unsigned int i);
unsigned int gt2HostToNetworkInt(unsigned int i);
unsigned short gt2HostToNetworkShort(unsigned short s);
unsigned short gt2NetworkToHostShort(unsigned short s);

/**********************
** ADDRESS FUNCTIONS **
**********************/

// Converts an IP and a port into a text string.  The IP must be in network byte order, and the port
// in host byte order.  The string must be able to hold at least 22 characters (including the NUL).
// "XXX.XXX.XXX.XXX:XXXXX"
// If both the IP and port are non-zero, the string will be of the form "1.2.3.4:5" ("<IP>:<port>").
// If the port is zero, and the IP is non-zero, the string will be of the form "1.2.3.4" ("<IP>").
// If the IP is zero, and the port is non-zero, the string will be of the form ":5" (":<port>").
// If both the IP and port are zero, the string will be an empty string ("")
// The string is returned.  If the string paramater is NULL, then an internal static string will be
// used.  There are two internal strings that are alternated between.
const char * gt2AddressToString
(
	unsigned int ip,                // IP in network byte order.  Can be 0.
	unsigned short port,            // Port in host byte order.  Can be 0.
	char string[22]                 // String will be placed in here.  Can be NULL.
);

// Converts a string address into an IP and a port.  The IP is stored in network byte order, and the port
// is stored in host byte order.  Returns false if there was an error parsing the string, or if a supplied
// hostname can't be resolved.
// Possible string forms:
// NULL => all IPs, any port (localAddress only).
// "" => all IPs, any port (localAddress only).
// "1.2.3.4" => 1.2.3.4 IP, any port (localAddress only).
// "host.com" => host.com's IP, any port (localAddress only).
// ":2786" => all IPs, 2786 port (localAddress only).
// "1.2.3.4:0" => 1.2.3.4 IP, any port (localAddress only).
// "host.com:0" => host.com's IP, any port (localAddress only).
// "0.0.0.0:2786" => all IPs, 2786 port (localAddress only).
// "1.2.3.4:2786" => 1.2.3.4 IP, 2786 port (localAddress or remoteAddress).
// "host.com:2786" => host.com's IP, 2786 port (localAddress or remoteAddress).
// If this function needs to resolve a hostname ("host.com") it may need to contact a DNS server, which can
// cause the function to block for an indefinite period of time.  Usually it is < 2 seconds, but on certain
// systems, and under certain circumstances, it can take 30 seconds or longer.
GT2Bool gt2StringToAddress
(
	const char * string,            // The string to convert.
	unsigned int * ip,              // The IP is stored here, in network byte order.  Can be NULL.
	unsigned short * port           // The port is stored here, in host byte order.  Can be NULL.
);

// Gets the host information for a machine on the Internet.  The first version takes an IP in network byte order,
// and the second version takes a string that is either a dotted ip ("1.2.3.4"), or a hostname ("www.gamespy.com").
// If the function can successfully lookup the host's info, the host's main hostname will be returned.  If it
// cannot find the host's info, it returns NULL.
// For the aliases parameter, pass in a pointer to a variable of type (char **).  If this parameter is not NULL,
// and the function succeeds, the variable will point to a NULL-terminated list of alternate names for the host.
// For the ips parameter, pass in a pointer to a variable of type (int **).  If this parameter is not NULL, and
// the function succeeds, the variable will point to a NULL-terminated list of altername IPs for the host.  Each
// element in the list is actually a pointer to an unsigned int, which is an IP address in network byte order.
// The return value, aliases, and IPs all point to an internal data structure, and none of these values should
// be modified directly.  Also, the data is only valid until another call needs to use the same data structure
// (virtually ever internet address function will use this data structure). If the data will be needed in the
// future, it should be copied off.
// If this function needs to resolve a hostname ("host.com") it may need to contact a DNS server, which can
// cause the function to block for an indefinite period of time.  Usually it is < 2 seconds, but on certain
// systems, and under certain circumstances, it can take 30 seconds or longer.
const char * gt2IPToHostInfo(unsigned int ip, char *** aliases, unsigned int *** ips);
const char * gt2StringToHostInfo(const char * string, char *** aliases, unsigned int *** ips);

// The following functions are shortcuts for the above two functions (gt2*ToHostInfo()), and each performs a subset
// of the functionality.  They are provided so that code that only needs certain information can be a little simpler.
// Before using these, read the comments for the gt2*ToHostInfo() functions, as the info also applies to these functions.
const char * gt2IPToHostname(unsigned int ip);
const char * gt2StringToHostname(const char * string);
char ** gt2IPToAliases(unsigned int ip);
char ** gt2StringToAliases(const char * string);
unsigned int ** gt2IPToIPs(unsigned int ip);
unsigned int ** gt2StringToIPs(const char * string);

#ifdef _XBOX
unsigned int gt2XnAddrToIP(XNADDR theAddr, XNKID theKeyId);
GT2Bool gt2IPToXnAddr(int ip, XNADDR *theAddr, XNKID *theKeyId);
#endif

// these are for getting around adhoc which requires a 48 bit address v.s. a 32 bit inet address
void gt2IpToMac(gsi_u32 ip,char *mac);
// change IP address to mac ethernet
gsi_u32 gt2MacToIp(const char *mac);
// change mac ethernet to IP address

/*******************
** DUMP CALLBACKS **
*******************/

// called with either sent or received data
// trying to send a message from within the send dump callback, or letting the socket think from within the receive
// dump callback can cause serious problems, and should not be done.
typedef void (* gt2DumpCallback)
(
	GT2Socket socket,          // the socket the message was on
	GT2Connection connection,  // the connection the message was on, or NULL if there is no connection for this message
	unsigned int ip,           // the remote ip, in network byte order
	unsigned short port,       // the remote port, in host byte order
	GT2Bool reset,             // if true, the connection has been reset (only used by the receive callback)
	const GT2Byte * message,   // the message (should not be modified)
	int len                    // the length of the message
);

/*******************
** DUMP FUNCTIONS **
*******************/

// sets a callback to be called whenever a UDP datagram is sent or received, and when a connection reset is received.
// pass in a callback of NULL to remove the callback.  the dumps sit at a lower level than the filters, and allow an
// app to keep an eye on exactly what datagrams are being sent and received, allowing for close monitoring.  however
// the dumps cannot be used to modify data, only monitor it.  the dumps are useful for debugging purposes, and
// to keep track of data send and receive rates (e.g., the Quake 3 engine's netgraph).
// note that these are the actual UDP datagrams being sent and received - datagrams may be dropped, repeated, or
// out-of-order.  control datagrams (those used internally by the protocol) will be passed to the dump callbacks,
// and certain application messages will have a header at the beginning.
void gt2SetSendDump(GT2Socket socket, gt2DumpCallback callback);
void gt2SetReceiveDump(GT2Socket socket, gt2DumpCallback callback);



#ifdef __cplusplus
}
#endif

#endif
