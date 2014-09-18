/******
GameSpy Server Browsing SDK
  
Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

******

 Please see the GameSpy Server Browsing SDK documentation for more 
 information

******/


#ifndef _SB_SERVERBROWSING_H
#define _SB_SERVERBROWSING_H

#include "../common/gsCommon.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	

/*******************
ServerBrowser Typedefs
********************/
//ServerBrowser is an abstract data type used to represent the server list and query engine objects.
typedef struct _ServerBrowser *ServerBrowser;

//SBServer is an abstract data type representing a single server.
#ifndef SBServer 
typedef struct _SBServer *SBServer;
#endif	
//Simple boolean type used for some functions
typedef enum {SBFalse, SBTrue} SBBool;

//Error codes that can be returned from functions
typedef enum 
{
	sbe_noerror,  //no error has occured
	sbe_socketerror, //a socket function has returned an unexpected error
	sbe_dnserror,  //DNS lookup of master address failed
	sbe_connecterror,  //connection to master server failed
	sbe_dataerror, //invalid data was returned from master server
	sbe_allocerror, //memory allocation failed
	sbe_paramerror,	//an invalid parameter was passed to a function
	sbe_duplicateupdateerror //server update requested on a server that was already being updated
} SBError;	

//States the ServerBrowser object can be in
typedef enum 
{
	sb_disconnected, //idle and not connected to the master server
	sb_listxfer,	//downloading list of servers from the master server
	sb_querying,	//querying servers
	sb_connected	//idle but still connected to the master server
} SBState;

//Callbacks that can occur during server browsing operations
typedef enum 
{
	sbc_serveradded, //a server was added to the list, may just have an IP & port at this point
	sbc_serverupdated, //server information has been updated - either basic or full information is now available about this server
	sbc_serverupdatefailed, //an attempt to retrieve information about this server, either directly or from the master, failed
	sbc_serverdeleted, //a server was removed from the list
	sbc_updatecomplete, //the server query engine is now idle
	sbc_queryerror,		//the master returned an error string for the provided query
	sbc_serverchallengereceived // received ip verification challenge from server
} SBCallbackReason;	

//Passed to callback to indicate state of attempt to connect to server
typedef enum
{
	sbcs_succeeded,
	sbcs_failed
} SBConnectToServerState;

//Prototype for the callback function you need to provide
typedef void (*ServerBrowserCallback)(ServerBrowser sb, SBCallbackReason reason, SBServer server, void *instance);

//Prototype for the callback function used when connecting to a server
typedef void (*SBConnectToServerCallback)(ServerBrowser sb, SBConnectToServerState state, SOCKET gamesocket, struct sockaddr_in *remoteaddr, void *instance);

//Maximum length for the SQL filter string
#define MAX_FILTER_LEN 256

//Version defines for query protocol
#define QVERSION_GOA 0
#define QVERSION_QR2 1

/*******************
ServerBrowser Object Functions
********************/

#ifndef GSI_UNICODE
#define ServerBrowserNew			ServerBrowserNewA
#define ServerBrowserUpdate			ServerBrowserUpdateA
#define ServerBrowserLimitUpdate	ServerBrowserLimitUpdateA
#define ServerBrowserAuxUpdateIP	ServerBrowserAuxUpdateIPA
#define ServerBrowserRemoveIP		ServerBrowserRemoveIPA
#define ServerBrowserSendNatNegotiateCookieToServer	ServerBrowserSendNatNegotiateCookieToServerA
#define ServerBrowserSendMessageToServer	ServerBrowserSendMessageToServerA
#define ServerBrowserSort			ServerBrowserSortA
#define SBServerGetStringValue		SBServerGetStringValueA
#define SBServerGetIntValue			SBServerGetIntValueA
#define SBServerGetFloatValue		SBServerGetFloatValueA
#define SBServerGetBoolValue		SBServerGetBoolValueA
#define SBServerGetPlayerStringValue	SBServerGetPlayerStringValueA
#define SBServerGetPlayerIntValue	SBServerGetPlayerIntValueA
#define SBServerGetPlayerFloatValue	SBServerGetPlayerFloatValueA
#define SBServerGetTeamStringValue	SBServerGetTeamStringValueA
#define SBServerGetTeamIntValue		SBServerGetTeamIntValueA
#define SBServerGetTeamFloatValue	SBServerGetTeamFloatValueA
#define ServerBrowserListQueryError	ServerBrowserListQueryErrorA
#define ServerBrowserErrorDesc		ServerBrowserErrorDescA
#define ServerBrowserGetServerByIP	ServerBrowserGetServerByIPA
#else
#define ServerBrowserNew			ServerBrowserNewW
#define ServerBrowserUpdate			ServerBrowserUpdateW
#define ServerBrowserLimitUpdate	ServerBrowserLimitUpdateW
#define ServerBrowserAuxUpdateIP	ServerBrowserAuxUpdateIPW
#define ServerBrowserRemoveIP		ServerBrowserRemoveIPW
#define ServerBrowserSendNatNegotiateCookieToServer	ServerBrowserSendNatNegotiateCookieToServerW
#define ServerBrowserSendMessageToServer	ServerBrowserSendMessageToServerW
#define ServerBrowserSort			ServerBrowserSortW
#define SBServerGetStringValue		SBServerGetStringValueW
#define SBServerGetIntValue			SBServerGetIntValueW
#define SBServerGetFloatValue		SBServerGetFloatValueW
#define SBServerGetBoolValue		SBServerGetBoolValueW
#define SBServerGetPlayerStringValue	SBServerGetPlayerStringValueW
#define SBServerGetPlayerIntValue	SBServerGetPlayerIntValueW
#define SBServerGetPlayerFloatValue	SBServerGetPlayerFloatValueW
#define SBServerGetTeamStringValue	SBServerGetTeamStringValueW
#define SBServerGetTeamIntValue		SBServerGetTeamIntValueW
#define SBServerGetTeamFloatValue	SBServerGetTeamFloatValueW
#define ServerBrowserListQueryError	ServerBrowserListQueryErrorW
#define ServerBrowserErrorDesc		ServerBrowserErrorDescW
#define ServerBrowserGetServerByIP	ServerBrowserGetServerByIPW
#endif
/* 
ServerBrowserNew
----------------
Creates and returns a new (empty) ServerBrowser object.
Returns NULL if an allocation error occurs.

queryForGamename - The gamename you are querying for
queryFromGamename - The gamename you are querying from - generally the same as queryForGamename
queryFromKey - Secret key that corresponds to the queryFromGamename
queryFromVersion - A game-specific version identifier (pass 0 unless told otherwise)
maxConcUpdates - Max number of concurent updates (10-15 for modem users, 20-30 for high-bandwidth)
queryVersion - Query protocol to use. Use QVERSION_GOA for DeveloperSpec/Query&Reporting1 games, and QVERSION_QR2 for games that use Query & Reporting 2
callback - The function that will be called with list updates
instance - User-defined instance data (e.g. structure or object pointer)  */
ServerBrowser ServerBrowserNew(const gsi_char *queryForGamename, const gsi_char *queryFromGamename, const gsi_char *queryFromKey, int queryFromVersion, int maxConcUpdates, int queryVersion, SBBool lanBrowse, ServerBrowserCallback callback, void *instance);

/* 
ServerBrowserFree
-----------------
Free a ServerBrowser and all internal sturctures and servers */
void ServerBrowserFree(ServerBrowser sb);


/* ServerBrowserUpdate
-------------------
Starts an update by downloading a list of servers from the master server, then querying them.

sb - The server browser object to update
async - If SBTrue, the update will be initiated, and ServerListThink must be called for processing and querying to occur
		If SBFalse, the function will not return until the initial list of servers has been completely updated
disconnectOnComplete - If SBTrue, the connection to the master server will be disconnected immediately after the list is downloaded.
					   If SBFalse, the connection will be left open for additional data queries, and can be closed via ServerBrowserDisconnect
basicFields - This array of registered QR2 keys is used to determine the fields requested from servers during the initial "basic" update.
				Only server keys listed in this array will be returned for servers.
numBasicFields - The number of fields in the basicFields array
serverFilter - SQL Filter string that will be applied on the master server to limit the list of servers returned.
				All server keys are available for filtering on the master server, as well as the master-defined "country" and "region" keys. 
				Standard SQL syntax should be used. 
				
ServerBrowserLimitUpdate
------------------------
Identical to ServerBrowserUpdate, except that the number of servers returned can be limited
maxServers - Maximum number of servers to be returned
*/ 
SBError ServerBrowserUpdate(ServerBrowser sb, SBBool async, SBBool disconnectOnComplete, const unsigned char *basicFields, int numBasicFields, const gsi_char *serverFilter);
SBError ServerBrowserLimitUpdate(ServerBrowser sb, SBBool async, SBBool disconnectOnComplete, const unsigned char *basicFields, int numBasicFields, const gsi_char *serverFilter, int maxServers);


/* ServerBrowserThink
-------------------
Processes incoming data from the master server and game servers that are being queried. Should be called
as often as possible while a server list update is in progress (~10ms is ideal). */
SBError ServerBrowserThink(ServerBrowser sb);

/* ServerBrowserLANUpdate
-------------------
Starts an update by searching for servers on the LAN, then querying them. You can specifiy a range of ports to search
for servers. Generally this should start with your standard query port, and range above it, since the QR and QR2 SDKs will
automatically allocate higher port numbers when running multiple servers on the same machine. You should limit your search
to 100 ports or less in most cases to limit flooding of the LAN with broadcast packets.

sb - The server browser object to update
async - If SBTrue, the update will be initiated, and ServerListThink must be called for processing and querying to occur
		If SBFalse, the function will not return until the initial list of servers has been completely updated
startSearchPort - The initial port to begin searching for servers on
endSearchPort - The final port to search. All ports between start and end will be queried. */
SBError ServerBrowserLANUpdate(ServerBrowser sb, SBBool async, unsigned short startSearchPort, unsigned short endSearchPort);

/* ServerBrowserAuxUpdateIP
-------------------
Manually updates a server given an IP address and query port. Use to manually add servers to the list when you just
have an IP and port for them. 

sb - The server browser object to add the server to
ip - The dotted IP address of the server e.g. "1.2.3.4"
port - The query port of the server
viaMaster - If SBTrue, information about the server will be retrieved from the master server instead of attempting to query the server directly.
				If a connection to the master server does not exist, it will be made to kept open afterwards.
			If SBFalse, the server will be contacted directly for information.
async - If SBTrue, the update will be initiated, and ServerListThink must be called for processing and querying to occur
		If SBFalse, the function will not return until the server has been successfully or unsuccessfully updated
fullUpdate - If SBTrue, all server keys/rules/player/team information will be retrieved
			 If SBFalse, only the keys specified in the basicFields array of the ServerBrowserUpdate function will be retrieved */
SBError ServerBrowserAuxUpdateIP(ServerBrowser sb, const gsi_char *ip, unsigned short port, SBBool viaMaster, SBBool async, SBBool fullUpdate);

/* ServerBrowserAuxUpdateServer
-------------------
Manually updates a server object. Generally used to get additional information about a server (for example, to get full rules and
player information from a server that only has basic information so far), but can also be used to "refresh" the information 
about a given server. Data will automatically be retrieved from the master server directly or from the game server as appropriate.
When called asynchronously, multiple server update requests can be queued and will be executed by the query engine in turn.

sb - The server browser object to add the server to
server - Server object to update
async - If SBTrue, the update will be initiated, and ServerListThink must be called for processing and querying to occur
		If SBFalse, the function will not return until the server has been successfully or unsuccessfully updated
fullUpdate - If SBTrue, all server keys/rules/player/team information will be retrieved
			 If SBFalse, only the keys specified in the basicFields array of the ServerBrowserUpdate function will be retrieved */
SBError ServerBrowserAuxUpdateServer(ServerBrowser sb, SBServer server, SBBool async, SBBool fullUpdate);


/* ServerBrowserDisconnects
-------------------
Disconnects an idle connection to the master server when it is no longer needed. Note that if you disconnect
and then request an operation that requires a connection to the master, such as an AuxServerUpdate via the master,
the connection will be automatically re-established. */
void ServerBrowserDisconnect(ServerBrowser sb);


/* ServerBrowserState
-------------------
Returns the current state of the Server Browser object */
SBState ServerBrowserState(ServerBrowser sb);

/* ServerBrowserRemoveIP
-------------------
Removes a server from the list given an IP and query port */
void ServerBrowserRemoveIP(ServerBrowser sb, const gsi_char *ip, unsigned short port);

/* ServerBrowserRemoveServer
-------------------
Removes a server from the list and releases all resources associated with it */
void ServerBrowserRemoveServer(ServerBrowser sb, SBServer server);

/* ServerBrowserHalt
-------------------
Stops a server list update in progress, clears any servers queued to be queried, and disconneects
from the master server. */
void ServerBrowserHalt(ServerBrowser sb);

/* ServerBrowserClear
-------------------
Removes all the servers from the list and frees all resources associated with them. */
void ServerBrowserClear(ServerBrowser sb);

/* ServerBrowserErrorDesc
-------------------
Returns a human-readable error string for the given error code. */
const gsi_char *ServerBrowserErrorDesc(ServerBrowser sb, SBError error);

/* ServerBrowserListQueryError
-------------------
When a list query error occurs, as indicated by the sbc_queryerror callback, this function allows you to
obtain the human-readable error string for the error (generally these errors are caused by errors in the 
filter string) */
const gsi_char *ServerBrowserListQueryError(ServerBrowser sb);

/* ServerBrowserGetServer
----------------------
Returns the server at the specified index, or NULL if the index is out of bounds */
SBServer ServerBrowserGetServer(ServerBrowser sb, int index);

/* ServerBrowserGetServerByIP
----------------------
Returns the SBServer with the specified IP, or NULL if the server is not in the list */
SBServer ServerBrowserGetServerByIP(ServerBrowser sb, const gsi_char* ip, unsigned short port);

/* ServerBrowserCount
------------------
Returns the number of servers on the specified list. Indexing is 0 based, so
the actual server indexes are 0 <= valid index < Count */
int ServerBrowserCount(ServerBrowser sb);

/* ServerBrowserPendingQueryCount
------------------
Returns the number of servers currently being queried or queued to be queried. When this number is 0, the query
engine is idle */
int ServerBrowserPendingQueryCount(ServerBrowser sb);

/* ServerBrowserGetMyPublicIP
------------------
Returns the public IP address for this computer, as seen by an outside machine (the master server). Use to determine
if a server you want to connect to is on the same private network or not. Only valid after a call to ServerListUpdate has
connected to the master server */
char *ServerBrowserGetMyPublicIP(ServerBrowser sb);

/* ServerBrowserGetMyPublicIPAddr
------------------
Same as ServerBrowserGetMyPublicIP except that the address is returned in standard network-byte-order form */
unsigned int ServerBrowserGetMyPublicIPAddr(ServerBrowser sb);


/* ServerBrowserSendNatNegotiateCookieToServer
------------------
Sends a cookie value to the server for use with NAT Negotiation */
SBError ServerBrowserSendNatNegotiateCookieToServer(ServerBrowser sb, const gsi_char *ip, unsigned short port, int cookie);


/* ServerBrowserSendMessageToServer
------------------
Sends a game-specific message to a server  */
SBError ServerBrowserSendMessageToServer(ServerBrowser sb, const gsi_char *ip, unsigned short port, const char *data, int len);

/* ServerBrowserConnectToServer
------------------
Attempts to connect to the server, using natneg if necessary */
SBError ServerBrowserConnectToServer(ServerBrowser sb, SBServer server, SBConnectToServerCallback callback);


/* Comparision types for the ServerBrowserSort function
int - assume the values are int and do an integer compare
float - assume the values are float and do a flot compare
strcase - assume the values are strings and do a case sensitive compare
stricase - assume the values are strings and do a case insensitive compare */
typedef enum {sbcm_int, sbcm_float, sbcm_strcase, sbcm_stricase} SBCompareMode;


/* ServerBrowserSort
-----------------
Sort the server list in either ascending or descending order using the 
specified comparemode.
sortkey can be a normal server key, or "ping" or "hostaddr" */
void ServerBrowserSort(ServerBrowser sb, SBBool ascending, const gsi_char *sortkey, SBCompareMode comparemode);

/* ServerBrowserLANSetLocalAddr
-------------------
Sets the network adapter to use for LAN broadcasts (optional) */
void ServerBrowserLANSetLocalAddr(ServerBrowser sb, const char* theAddr);


/*******************
SBServer Object Functions
********************/


// Callback function used for enumerating the keys/values for a server
typedef void (*SBServerKeyEnumFn)(gsi_char *key, gsi_char *value, void *instance);


/* SBServerGetConnectionInfo
----------------
Check if Nat Negotiation is requires, based off whether it is a lan game, public ip present and several other facts. 
Returns an IP string to use for NatNeg, or direct connect if possible
Work for subsequent connection to this server, One of three results will occur
i) Lan game, connect using ipstring
2) Internet game, connect using ipstring
3) nat traversal required, perform nat negotiation using Nat SDK and this ipstring before connecting. 

return sb_true if further processing is required... i.e. NAT.   sb_false if not.
fills an IP string
*/
SBBool SBServerGetConnectionInfo(ServerBrowser gSB, SBServer server, gsi_u16 PortToConnectTo, char *ipstring_out);


/* SBServerHasPrivateAddress
----------------
Indicates whether the master server has provided a private address for this server */
SBBool SBServerHasPrivateAddress(SBServer server);

/* SBServerDirectConnect
----------------
Indicates whether the server supports direct UDP connections (if false, NAT Negotiation is required) */
SBBool SBServerDirectConnect(SBServer server);

/* SBServerGetPing
----------------
Returns the ping for the specified server. Ping is measured as the latency from the time a 
query is sent to the server until the data is returned for that query */
int SBServerGetPing(SBServer server);


/* SBServerGetPublicAddress/SBServerGetPrivateAddress
-------------------
Returns the string, dotted IP address for the specified server 
The "private" version is only valid when the server has a private address available */
char *SBServerGetPublicAddress(SBServer server);
char *SBServerGetPrivateAddress(SBServer server);

/* SBServerGetPublicInetAddress/SBServerGetPrivateInetAddress
-------------------
Returns the network-ordered IP address for the specified server */
unsigned int SBServerGetPublicInetAddress(SBServer server);
unsigned int SBServerGetPrivateInetAddress(SBServer server);

/* SBServerGetPublicQueryPort/SBServerGetPrivateQueryPort
----------------
Returns the "query" port for the specified server. If the game uses a seperate
"game" port, it can be retrieved via: SBServerGetIntValue(server,"hostport",0) */
unsigned short SBServerGetPublicQueryPort(SBServer server);
unsigned short SBServerGetPrivateQueryPort(SBServer server);


/* SBServerHasBasicKeys
----------------
Returns SBTrue if a server has at least basic keys available for it. "Basic" keys
are those indicated in the ServerBrowserUpdate function. */
SBBool SBServerHasBasicKeys(SBServer server);

/* SBServerHasFullKeys
----------------
Returns SBTrue if a server has full keys available for it. This includes all server
rules and player/team keys. */
SBBool SBServerHasFullKeys(SBServer server);

/* SBServerHasValidPing
----------------
Returns SBTrue if a server has a valid ping value for it (otherwise the ping will be 0) */
SBBool SBServerHasValidPing(SBServer server);


/* SBServerGet[]Value
------------------
Returns the value for the specified key. If the key does not exist for the
given server, the default value is returned */
const gsi_char *SBServerGetStringValue(SBServer server, const gsi_char *keyname, const gsi_char *def);
int SBServerGetIntValue(SBServer server, const gsi_char *key, int idefault);
double SBServerGetFloatValue(SBServer server, const gsi_char *key, double fdefault);
SBBool SBServerGetBoolValue(SBServer server, const gsi_char *key, SBBool bdefault);


/* SBServerGetPlayer[]Value / SBServerGetTeam[]Value
------------------
Returns the value for the specified key on the specified player or team. If the key does not exist for the
given server, the default value is returned 
Player keys take the form keyname_N where N is the player index, and team keys take the form
keyname_tN where N is the team index. You should only specify the keyname for the key in the below functions.
*/
const gsi_char *SBServerGetPlayerStringValue(SBServer server, int playernum, const gsi_char *key, const gsi_char *sdefault);
int SBServerGetPlayerIntValue(SBServer server, int playernum, const gsi_char *key, int idefault);
double SBServerGetPlayerFloatValue(SBServer server, int playernum, const gsi_char *key, double fdefault);

const gsi_char *SBServerGetTeamStringValue(SBServer server, int teamnum, const gsi_char *key, const gsi_char *sdefault);
int SBServerGetTeamIntValue(SBServer server, int teamnum, const gsi_char *key, int idefault);
double SBServerGetTeamFloatValue(SBServer server, int teamnum, const gsi_char *key, double fdefault);


/* SBServerEnumKeys 
-----------------
Enumerates the keys/values for a given server by calling KeyEnumFn with each
key/value. The user-defined instance data will be passed to the KeyFn callback */
void SBServerEnumKeys(SBServer server, SBServerKeyEnumFn KeyFn, void *instance);


#ifdef __cplusplus
}
#endif

#endif 
