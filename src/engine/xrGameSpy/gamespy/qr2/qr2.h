#ifndef _QR2_H_
#define _QR2_H_

#include "../common/gsCommon.h"


/**********
qr2regkeys.h contains defines for all of the reserved keys currently available.
***********/
#include "qr2regkeys.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef GSI_UNICODE
#define qr2_init			qr2_initA
#define qr2_init_socket		qr2_init_socketA
#define qr2_parse_query		qr2_parse_queryA
#define qr2_buffer_add		qr2_buffer_addA
#else
#define qr2_init			qr2_initW
#define qr2_init_socket		qr2_init_socketW
#define qr2_parse_query		qr2_parse_queryW
#define qr2_buffer_add		qr2_buffer_addW
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

//no need to escape strings, more flexible querying, less bandwidth, no need to space check buffers


/********
ERROR CONSTANTS
---------------
These constants are returned from qr2_init and the error callback to signal an error condition
***************/
typedef enum 
{e_qrnoerror, //no error occured
e_qrwsockerror, //a standard socket call failed (exhausted resources?)
e_qrbinderror, //the SDK was unable to find an available port to bind on
e_qrdnserror, //a DNS lookup (for the master server) failed
e_qrconnerror,  //the server is behind a nat and does not support negotiation
e_qrnochallengeerror, //no challenge was received from the master - either the master is down, or a firewall is blocking UDP
qr2_error_t_count
} qr2_error_t;


/********
KEY TYPES
---------------
Server information is reported in key/value pairs. There are three key types:
key_server - General information about the game in progress
key_player - Information about a specific player
key_team - Information about a specific team
***************/
typedef enum {key_server, key_player, key_team, key_type_count} qr2_key_type;

/*********
NUM_PORTS_TO_TRY
----------------
This value is the maximum number of ports that will be scanned to
find an open query port, starting from the value passed to qr2_init
as the base port. Generally there is no reason to modify this value.
***********/
#define NUM_PORTS_TO_TRY 100


/*********
MAGIC VALUES
----------------
These values will be used at the start of all QR2 query packets. If you are processing query
data on your game socket, you can use these bytes to determine if a packet should be forwarded
to the QR2 SDK for processing.
***********/
#define QR_MAGIC_1 0xFE
#define QR_MAGIC_2 0xFD

/* The app can resolve the master server hostname for this
game itself and store the IP here before calling qr2_init.
For more information, contact devsupport@gamespy.com. */
extern char qr2_hostname[64];

/***********
qr2_t
----
This abstract type is used to instantiate multiple instances of the
Query & Reporting SDK (for example, if you are running multiple servers
in the same process).
For most games, you can ignore this value and pass NULL in to all functions
that require it. A single global instance will be used in this case.
************/
typedef struct qr2_implementation_s *qr2_t;

/***********
qr2_keybuffer_t
---------------
This structure is used to store a list of keys when enumerating available keys.
Use the qr2_keybuffer_add function to add keys to the list.
************/
typedef struct qr2_keybuffer_s *qr2_keybuffer_t;


/***********
qr2_buffer_t
------------
This structure stores data that will be sent back to a client in response to 
a query. Use the qr2_buffer_add functions to add data to the buffer in your callbacks.
************/
typedef struct qr2_buffer_s *qr2_buffer_t;

typedef struct qr2_ipverify_node_s *qr2_ipverify_node_t;


/********
qr2_serverkeycallback_t
-------------------
This is the prototype for one of the callback functions you will need to provide.
The serverkey callback is called when a client requests information about a specific
server key.
[keyid] is the key being requested.
[outbuf] is the destination buffer for the value information. Use qr2_buffer_add to report the value.
[userdata] is the pointer that was passed into qr2_init. You can use this for an
	object or structure pointer if needed.
If you don't have a value for the provided keyid, you should add a empty ("") string to the buffer.
********/
typedef void (*qr2_serverkeycallback_t)(int keyid, qr2_buffer_t outbuf, void *userdata);
	
/********
qr2_playerteamkeycallback_t
-------------------
This is the prototype for two of the callback functions you will need to provide.
The player key callback is called when a client requests information about a specific key
for a specific player.
The team key callback is called when a client requests the value for a team key.
[keyid] is the key being requested.
[index] is the 0-based index of the player or team being requested.
[outbuf] is the destination buffer for the value information. Use qr2_buffer_add to report the value.
[userdata] is the pointer that was passed into qr2_init. You can use this for an
	object or structure pointer if needed.
If you don't have a value for the provided keyid, you should add a empty ("") string to the buffer.
********/
typedef void (*qr2_playerteamkeycallback_t)(int keyid, int index, qr2_buffer_t outbuf, void *userdata);	

	
/********
qr2_keylistcallback_t
-------------------
This is the prototype for one of the callback functions you will need to provide.
The key list callback is called when the SDK needs to determine all of the keys you game has
values for.
[keytype] is the type of keys being requested (server, player, team). You should only add keys
	of this type to the keybuffer.
[keybuffer] is the structure that holds the list of keys. Use qr2_keybuffer_add to add a key
	to the buffer.
[userdata] is the pointer that was passed into qr2_init. You can use this for an
	object or structure pointer if needed.
********/
typedef void (*qr2_keylistcallback_t)(qr2_key_type keytype, qr2_keybuffer_t keybuffer, void *userdata);	


/********
qr2_countcallback_t
-------------------
This is the prototype for one of the callback functions you will need to provide.
The count callback is used by the SDK to get a count of player or teams on the server.
[keytype] should be used to determine whether the player or team count is being requested (key_player or key_team will be passed)
[userdata] is the pointer that was passed into qr2_init. You can use this for an
	object or structure pointer if needed.
If your game does not support teams, you can return 0 for the count of teams.
********/
typedef int  (*qr2_countcallback_t)(qr2_key_type keytype, void *userdata);	

/********
qr2_adderrorcallback_t
-------------------
This is the prototype for one of the callback functions you will need to provide.
The add error callback is called in response to a message from the master server indicating a problem listing the server.
[error] is a code that can be used to determine the specific listing error.
[errmsg] is a human-readable error string returned from the master server.
[userdata] is the pointer that was passed into qr2_init. You can use this for an
	object or structure pointer if needed.
The most common error that will be returned is if the master is unable to list the server due to a firewall or proxy
that would block incoming game packets.
********/
typedef void (*qr2_adderrorcallback_t)(qr2_error_t error, gsi_char *errmsg, void *userdata);	


//todo - document
typedef void (*qr2_natnegcallback_t)(int cookie, void *userdata);	
typedef void (*qr2_clientmessagecallback_t)(gsi_char *data, int len, void *userdata);	
typedef void (*qr2_publicaddresscallback_t)(unsigned int ip, unsigned short port, void *userdata);
typedef void (*qr2_clientconnectedcallback_t)(SOCKET gamesocket, struct sockaddr_in *remoteaddr, void *userdata);

//#if defined(QR2_IP_FILTER)
typedef void (*qr2_denyqr2responsetoipcallback_t)(void *userdata, unsigned int sender_ip, int * result);
//#endif //#if defined(QR2_IP_FILTER)

void qr2_register_natneg_callback(qr2_t qrec, qr2_natnegcallback_t nncallback);
void qr2_register_clientmessage_callback(qr2_t qrec, qr2_clientmessagecallback_t cmcallback);
void qr2_register_publicaddress_callback(qr2_t qrec, qr2_publicaddresscallback_t pacallback);
void qr2_register_clientconnected_callback(qr2_t qrec, qr2_clientconnectedcallback_t cccallback);

//#if defined(QR2_IP_FILTER)
void qr2_register_denyresponsetoip_callback(qr2_t qrec, qr2_denyqr2responsetoipcallback_t dertoipcallback);
//#endif //#if defined(QR2_IP_FILTER)


/*****************
QR2_REGISTER_KEY
--------------------
Use this function to register custom server, player, and team keys that your server reports.
[keyid] is the ID number you have chosen for this key. The first NUM_RESERVED_KEYS (50) keys are reserved, all other
	keyid values up to MAX_REGISTERED_KEYS (254) are available for your use.
[key] is the string name of the key. Player keys should end in "_" (such as "score_") and team keys should end in "_t".
All custom keys should be registered prior to calling qr2_init. Reserved keys are already registered and should not be
passed to this function.
*******************/
void qr2_register_key(int keyid, const gsi_char *key);


/************
QR2_INIT
--------
This function initializes the Query and Reporting SDK and prepares the SDK to accept incoming
queries and send heartbeats to the master server.
[qrec] if not null, will be filled with the qr2_t instance for this server.
	If you are not using more than one instance of the Query & Reporting SDK you
	can pass in NULL for this value.
[ip] is an optional parameter that determines which dotted IP address to bind to on
	a multi-homed machine. You can pass NULL to bind to all IP addresses.
	If your game networking supports binding to user-specified IPs, you should make sure the same IP
	is bound by the Query and Reporting SDK.
[baseport] is the port to accept queries on. If baseport is not available, the
	Query and Reporting SDK will scan for an available port in the range of 
	baseport -> baseport + NUM_PORTS_TO_TRY
	Optionally, you can pass in 0 to have a port chosen automatically
	(makes it harder for debugging/testing).
[gamename] is the unique gamename that you were given
[secretkey] is your unique secret key
[ispublic] is 1 if the server should send heartbeats to the GameSpy master server and be publicly listed, 
	If 0, the server will only be available for LAN browsing
[natnegotiate] is 1 if the server supports GameSpy's NAT-negotiation technology (or another similar technology)
	 which allows hosting behind a NAT. If you do not support NAT-negotiation (i.e. a 3rd party handshake server), 
	 pass 0 to prevent the server from being listed if it is behind a NAT that cannot be traversed by outside clients.
[qr2_*_callback] are your callback functions, these cannot be NULL
[userdata] is an optional, implementation specific parameter that will be
	passed to all callback functions. Use it to store an object or structure
	pointer if needed.

Returns
e_qrnoerror is successful, otherwise one of the qr2_error_t constants above.
************/
qr2_error_t qr2_init(/*[out]*/qr2_t *qrec, const gsi_char *ip, int baseport, const gsi_char *gamename, const gsi_char *secret_key,
			int ispublic, int natnegotiate,
			qr2_serverkeycallback_t server_key_callback,
			qr2_playerteamkeycallback_t player_key_callback,
			qr2_playerteamkeycallback_t team_key_callback,
			qr2_keylistcallback_t key_list_callback,
			qr2_countcallback_t playerteam_count_callback,
			qr2_adderrorcallback_t adderror_callback,
			void *userdata);

/************
QR2_INIT_SOCKET
--------
This version of qr2_init allows the game to specify the UDP socket to use for
sending heartbeats and query replies. This enables the game and the QR2 SDK to
share a single UDP socket for all networking, which can make hosting games
behind a NAT proxy possible (see the documentation for more information).
You must also use qr2_parse_query to pass in any data received for the QR SDK
on the socket, since the SDK will not try to read any data off the socket directly.
[s] is the UDP socket to use for heartbeats and query replies. It must be a valid
	socket and should be bound to a port before calling qr_init_socket. It can be
	blocking or non-blocking.
[boundport] is the local port that the socket is bound to.
All other parameters are the same as described in qr2_init.
************/
qr2_error_t qr2_init_socket(/*[out]*/qr2_t *qrec, SOCKET s, int boundport, const gsi_char *gamename, const gsi_char *secret_key,
					 int ispublic, int natnegotiate,
					 qr2_serverkeycallback_t server_key_callback,
					 qr2_playerteamkeycallback_t player_key_callback,
					 qr2_playerteamkeycallback_t team_key_callback,
					 qr2_keylistcallback_t key_list_callback,
					 qr2_countcallback_t playerteam_count_callback,
					 qr2_adderrorcallback_t adderror_callback,
					 void *userdata);

/*******************
QR2_THINK
-------------------
This function should be called somewhere in your main program loop to
process any pending server queries and send a heartbeat if needed.

Query replies are very latency sensative, so you should make sure this
function is called at least every 100ms while your game is in progress.
The function has very low overhead and should not cause any performance
problems.
Unless you are using multiple instances of the SDK, you should pass NULL
for qrec.
********************/
void qr2_think(qr2_t qrec);

/*******************
QR2_PARSE_QUERY
-------------------
Use only with qr2_init_socket to pass in data that is destined for the Q&R SDK
from your game socket. 
You still need to call qr2_think in your main loop, and just call this
function whenever data is received. 
Unless you are using multiple instances of the SDK, you should pass NULL
for qrec.
[query] is the packet of query data received from the client. 
[len] is the length of the data
[sender] is the address that the query is received from. The QR SDK will reply
directly to that address using the socket provided in qr2_init_socket.
*******************/
void qr2_parse_query(qr2_t qrec, gsi_char *query, int len, struct sockaddr *sender);

/*****************
QR2_SEND_STATECHANGED
--------------------
This function forces a "statechanged" heartbeat to be sent immediately.
Use it any time you have changed the gamestate of your game to signal the
master to update your status.
Unless you are using multiple instances of the SDK, you should pass NULL
for qrec.
*******************/
void qr2_send_statechanged(qr2_t qrec);

/*****************
QR2_SHUTDOWN
------------
This function closes the sockets created in qr_init and takes care of
any misc. cleanup. You should try to call it when before exiting the server process.
An "exiting" statechanged heartbeat will automatically be sent to assist in quickly
de-listing the server.
If you pass in a qrec that was returned from qr_init, all resources associated
with that qrec will be freed. If you passed NULL into qr_int, you can pass
NULL in here as well.
******************/
void qr2_shutdown(qr2_t qrec);



/*****************
QR2_KEYBUFFER_ADD
------------
Use this function to add a registered key to the key buffer when asked to provide 
a list of supported keys.
******************/
gsi_bool qr2_keybuffer_add(qr2_keybuffer_t keybuffer, int keyid);

/*****************
QR2_BUFFER_ADD / ADD_INT
------------
These functions are used to add a key's value to the outgoing buffer when
requested in a callback function.
******************/
gsi_bool qr2_buffer_add(qr2_buffer_t outbuf, const gsi_char *value);
gsi_bool qr2_buffer_add_int(qr2_buffer_t outbuf, int value);


/* for CDKey SDK integration */
#define REQUEST_KEY_LEN 4
#define RECENT_CLIENT_MESSAGES_TO_TRACK 10
typedef void (*cdkey_process_t)(char *buf, int len, struct sockaddr *fromaddr);

/* ip verification / spoof prevention */
#define QR2_IPVERIFY_TIMEOUT        4000  // timeout after 4 seconds round trip time
#define QR2_IPVERIFY_ARRAY_SIZE     200   // allowed outstanding queryies in those 4 seconds
#define QR2_IPVERIFY_MAXDUPLICATES  5     // allow maximum of 5 requests per IP/PORT
struct qr2_ipverify_info_s
{
	struct sockaddr_in addr;      // addr = 0 when not in use
	gsi_u32            challenge;
	gsi_time           createtime; 
};

struct qr2_implementation_s
{
	SOCKET hbsock;
	char gamename[64];
	char secret_key[64];
	char instance_key[REQUEST_KEY_LEN];
	qr2_serverkeycallback_t server_key_callback;
	qr2_playerteamkeycallback_t player_key_callback;
	qr2_playerteamkeycallback_t team_key_callback;
	qr2_keylistcallback_t key_list_callback;
	qr2_countcallback_t playerteam_count_callback;
	qr2_adderrorcallback_t adderror_callback;					
	qr2_natnegcallback_t nn_callback;
	qr2_clientmessagecallback_t cm_callback;
	qr2_publicaddresscallback_t pa_callback;
	qr2_clientconnectedcallback_t cc_callback;
//#if defined(QR2_IP_FILTER)
	qr2_denyqr2responsetoipcallback_t denyresp2_ip_callback;
//#endif //#if defined(QR2_IP_FILTER)


	gsi_time lastheartbeat;
	gsi_time lastka;
	int userstatechangerequested;
	int listed_state;
	int ispublic;	 
	int qport;
	int read_socket;
	int nat_negotiate;
	struct sockaddr_in hbaddr;
	cdkey_process_t cdkeyprocess;
	int client_message_keys[RECENT_CLIENT_MESSAGES_TO_TRACK];
	int cur_message_key;
	unsigned int publicip;
	unsigned short publicport;
	void *udata;

	gsi_u8 backendoptions; // received from server inside challenge packet 
	struct qr2_ipverify_info_s ipverify[QR2_IPVERIFY_ARRAY_SIZE];
};

// These need to be defined, even in GSI_UNICODE MODE
void qr2_parse_queryA(qr2_t qrec, char *query, int len, struct sockaddr *sender);
gsi_bool qr2_buffer_addA(qr2_buffer_t outbuf, const char *value);
qr2_error_t qr2_initA(/*[out]*/qr2_t *qrec, const char *ip, int baseport, const char *gamename, const char *secret_key,
			int ispublic, int natnegotiate,
			qr2_serverkeycallback_t server_key_callback,
			qr2_playerteamkeycallback_t player_key_callback,
			qr2_playerteamkeycallback_t team_key_callback,
			qr2_keylistcallback_t key_list_callback,
			qr2_countcallback_t playerteam_count_callback,
			qr2_adderrorcallback_t adderror_callback,
			void *userdata);
qr2_error_t qr2_init_socketA(/*[out]*/qr2_t *qrec, SOCKET s, int boundport, const char *gamename, const char *secret_key,
					 int ispublic, int natnegotiate,
					 qr2_serverkeycallback_t server_key_callback,
					 qr2_playerteamkeycallback_t player_key_callback,
					 qr2_playerteamkeycallback_t team_key_callback,
					 qr2_keylistcallback_t key_list_callback,
					 qr2_countcallback_t playerteam_count_callback,
					 qr2_adderrorcallback_t adderror_callback,
					 void *userdata);


#ifdef __cplusplus
}
#endif


#endif
