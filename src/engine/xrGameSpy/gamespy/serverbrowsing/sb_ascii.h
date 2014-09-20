/******
GameSpy Server Browsing SDK
  
Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

******

 Please see the GameSpy Server Browsing SDK documentation for more 
 information

******/

// PROTOTYPES FOR ASCII VERSIONS
// This is required to silence CodeWarrior warnings about functions not having a prototype


#ifndef _SB_ASCII_H
#define _SB_ASCII_H

#include "../common/gsCommon.h"

#ifdef __cplusplus
extern "C" {
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
ServerBrowser ServerBrowserNewA(const char *queryForGamename, const char *queryFromGamename, const char *queryFromKey, int queryFromVersion, int maxConcUpdates, int queryVersion, SBBool lanBrowse, ServerBrowserCallback callback, void *instance);


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
SBError ServerBrowserUpdateA(ServerBrowser sb, SBBool async, SBBool disconnectOnComplete, const unsigned char *basicFields, int numBasicFields, const char *serverFilter);
SBError ServerBrowserLimitUpdateA(ServerBrowser sb, SBBool async, SBBool disconnectOnComplete, const unsigned char *basicFields, int numBasicFields, const char *serverFilter, int maxServers);


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
SBError ServerBrowserAuxUpdateIPA(ServerBrowser sb, const char *ip, unsigned short port, SBBool viaMaster, SBBool async, SBBool fullUpdate);

/* ServerBrowserRemoveIP
-------------------
Removes a server from the list given an IP and query port */
void ServerBrowserRemoveIPA(ServerBrowser sb, const char *ip, unsigned short port);

/* ServerBrowserErrorDesc
-------------------
Returns a human-readable error string for the given error code. */
const char *ServerBrowserErrorDescA(ServerBrowser sb, SBError error);

/* ServerBrowserListQueryError
-------------------
When a list query error occurs, as indicated by the sbc_queryerror callback, this function allows you to
obtain the human-readable error string for the error (generally these errors are caused by errors in the 
filter string) */
const char *ServerBrowserListQueryErrorA(ServerBrowser sb);


/* ServerBrowserSendNatNegotiateCookieToServer
------------------
Sends a cookie value to the server for use with NAT Negotiation */
SBError ServerBrowserSendNatNegotiateCookieToServerA(ServerBrowser sb, const char *ip, unsigned short port, int cookie);


/* ServerBrowserSendMessageToServer
------------------
Sends a game-specific message to a server  */
SBError ServerBrowserSendMessageToServerA(ServerBrowser sb, const char *ip, unsigned short port, const char *data, int len);


/* ServerBrowserSort
-----------------
Sort the server list in either ascending or descending order using the 
specified comparemode.
sortkey can be a normal server key, or "ping" or "hostaddr" */
void ServerBrowserSortA(ServerBrowser sb, SBBool ascending, const char *sortkey, SBCompareMode comparemode);

/*******************
SBServer Object Functions
********************/


/* SBServerGetPublicAddress/SBServerGetPrivateAddress
-------------------
Returns the string, dotted IP address for the specified server 
The "private" version is only valid when the server has a private address available */
char *SBServerGetPublicAddress(SBServer server);
char *SBServerGetPrivateAddress(SBServer server);


/* SBServerGet[]Value
------------------
Returns the value for the specified key. If the key does not exist for the
given server, the default value is returned */
const char *SBServerGetStringValueA(SBServer server, const char *keyname, const char *def);
int SBServerGetIntValueA(SBServer server, const char *key, int idefault);
double SBServerGetFloatValueA(SBServer server, const char *key, double fdefault);
SBBool SBServerGetBoolValueA(SBServer server, const char *key, SBBool bdefault);


/* SBServerGetPlayer[]Value / SBServerGetTeam[]Value
------------------
Returns the value for the specified key on the specified player or team. If the key does not exist for the
given server, the default value is returned 
Player keys take the form keyname_N where N is the player index, and team keys take the form
keyname_tN where N is the team index. You should only specify the keyname for the key in the below functions.
*/
const char *SBServerGetPlayerStringValueA(SBServer server, int playernum, const char *key, const char *sdefault);
int SBServerGetPlayerIntValueA(SBServer server, int playernum, const char *key, int idefault);
double SBServerGetPlayerFloatValueA(SBServer server, int playernum, const char *key, double fdefault);

const char *SBServerGetTeamStringValueA(SBServer server, int teamnum, const char *key, const char *sdefault);
int SBServerGetTeamIntValueA(SBServer server, int teamnum, const char *key, int idefault);
double SBServerGetTeamFloatValueA(SBServer server, int teamnum, const char *key, double fdefault);


#ifdef __cplusplus
}
#endif

#endif 
