 /*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEERASCII_H_
#define _PEERASCII_H_

// PROTOTYPES OF ASCII FUNCTIONS
// The only purpose of this file is to silence the CodeWarrior
// "function not prototyped" warnings when using GSI_UNICODE mode

#ifdef __cplusplus
extern "C" {
#endif


/************
** GENERAL **
************/
// This connects a peer object to a chat server.
// Call peerDisconnect() to close the connection.
// A title must be set with peerSetTitle before connecting.
///////////////////////////////////////////////////////////
void peerConnectA
(
	PEER peer,  // The peer object.
	const char * nick,  // The nick to connect with.
	int profileID,  // The profileID, or 0 if no profileID.
	peerNickErrorCallback nickErrorCallback,  // Called if nick error.
	peerConnectCallback connectCallback,  // Called on complete.
	void * param,  // User-data.
	PEERBool blocking   // If PEERTrue, don't return until finished.
);

// Same as peerConnect, but also authenticates using a uniquenick
// and password or an email, profilenick, and password.
/////////////////////////////////////////////////////////////////
void peerConnectLoginA
(
	PEER peer,  // The peer object.
	int namespaceID,  // The namespace in which to login.
	const char * email,  // The email to login with.
	const char * profilenick,  // The profile to login with.
	const char * uniquenick,  // The uniquenick to login with.
	const char * password,  // The password for the login account.
	peerNickErrorCallback nickErrorCallback,  // Called if nick error.
	peerConnectCallback connectCallback,  // Called on complete.
	void * param,  // User-data.
	PEERBool blocking   // If PEERTrue, don't return until finished.
);

// Same as peerConnect, but also authenticates using an authtoken
// and partnerchallenge from a partner account system.
/////////////////////////////////////////////////////////////////
void peerConnectPreAuthA
(
	PEER peer,  // The peer object.
	const char * authtoken,  // The authtoken for this login.
	const char * partnerchallenge,  // The partner challenge for this login.
	peerNickErrorCallback nickErrorCallback,  // Called if nick error.
	peerConnectCallback connectCallback,  // Called on complete.
	void * param,  // User-data.
	PEERBool blocking   // If PEERTrue, don't return until finished.
);

// If peerNickErrorCallback is called, call this to
// try and continue the connection with a new nickname.
// If there is an error with this nick, the
// peerNickErrCallback will be called again.
///////////////////////////////////////////////////////
void peerRetryWithNickA
(
	PEER peer,
	const char * nick
);

// Register a uniquenick.
// Should be called in response to the peerNickErrorCallback being called
// with a type of PEER_UNIQUENICK_EXPIRED or PEER_NO_UNIQUENICK.
// If the uniquenick cannot be registered, the peerNickErrorCallback will
// be called again with a type of PEER_IN_USE or PEER_INVALID.
/////////////////////////////////////////////////////////////////////////
void peerRegisterUniqueNickA
(
	PEER peer,
	int namespaceID,
	const char * uniquenick,
	const char * cdkey
);

// Sets the current title.
// A title must be set before connecting.
// Returns PEERFalse if an error, or PEERTrue if success.
// For most games the title/qrSecretKey and
// sbName/sbSecretKey pairs will be the same.
/////////////////////////////////////////////////////////
PEERBool peerSetTitleA
(
	PEER peer,  // The peer object.
	const char * title,  // The title to make current (ie., ut, gmtest).
	const char * qrSecretKey,  // The queryreporting secret key.
	const char * sbName,  // The serverbrowsing name.
	const char * sbSecretKey,  // The serverbrowsing secret key.
	int sbGameVersion,  // The version of the game doing the browsing.
	int sbMaxUpdates,  // The maximum number of concurent updates (10-15 for modem users, 20-30 for high-bandwidth).
	PEERBool natNegotiate,  // PEERTrue if the title supports GameSpy's NAT-negotiation technology (or another similar technology).
	PEERBool pingRooms[NumRooms],  // To do pings int a room, set it to PEERTrue.
	PEERBool crossPingRooms[NumRooms]  // To do cross-pings in a room, set it to PEERTrue.
);

// Gets the currently set title.
// Returns NULL if no title is set.
///////////////////////////////////
const char * peerGetTitleA(PEER peer);

// Get the local user's nickname.
/////////////////////////////////
const char * peerGetNickA(PEER peer);

// Replaces any invalid characters in nick with underscores.
////////////////////////////////////////////////////////////
void peerFixNickA
(
	char * newNick,
	const char * oldNick
);

// Removes the namespace extension from a chat unique nick.
// Returns the nick if it ended with the extension.
// Otherwise returns NULL.
///////////////////////////////////////////////////////////
const char * peerTranslateNickA
(
	char * nick,  // The nick to translate
	const char * extension  // The extension to be removed.
);

// Changes the user's nickname.
///////////////////////////////
void peerChangeNickA
(
	PEER peer,  // The peer object.
	const char * newNick,  // The nickname to which to change.
	peerChangeNickCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Sets the away mode.
// If an empty string or NULL, away mode is off.
// If a valid string, away mode is on.
////////////////////////////////////////////////
void peerSetAwayModeA
(
	PEER peer,  // The peer object.
	const char * reason  // The away reason.  If NULL or "", not away.
);

// When using peerStartReportingWithSocket or peerCreateStagingRoomWithSocket,
// any qureries received on the sockets need to be passed to the SDK.  Pass
// the data using this function.
//////////////////////////////////////////////////////////////////////////////
void peerParseQueryA
(
	PEER peer,  // The peer object.
	char * query,  // String of query data.
	int len,  // The length of the string, not including the NUL.
	struct sockaddr * sender  // The address the query was received from.
);

// Sends a CD Key to the chat server for authentication.
// The callback gets called when the chat server responds.
// This call is required by some games to enter chat rooms.
// It must be called after connecting to the chat server, but
// before entering a room.
/////////////////////////////////////////////////////////////
void peerAuthenticateCDKeyA
(
	PEER peer,  // The peer object.
	const char * cdkey,  // The CD key to authenticate.
	peerAuthenticateCDKeyCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

/**********
** ROOMS **
**********/
// Joins the currently selected title's title room.
///////////////////////////////////////////////////
void peerJoinTitleRoomA
(
	PEER peer,  // The peer object.
	const char password[PEER_PASSWORD_LEN],  // An optional password, normally NULL.
	peerJoinRoomCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Joins a staging room.
// server is one of the server objects passed to peerListingGamesCallback().
// This call will only work if staging==PEERTrue for the server.
// PANTS|09.11.00 - The password is only needed for passworded rooms.
// PANTS|03.15.01 - No longer requires you to be actively listing games.
////////////////////////////////////////////////////////////////////////////
void peerJoinStagingRoomA
(
	PEER peer,  // The peer object.
	SBServer server,  // The server passed into peerlistingGamesCallback().
	const char password[PEER_PASSWORD_LEN],  // The password of the room being joined.  Can be NULL or "".
	peerJoinRoomCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking   // If PEERTrue, don't return until finished.
);

// Joins a staging room by its channel name.
// Same as peerJoinStagingRoom, except it takes the staging room's
// channel name instead of an SBServer object.  Also, when the
// peerGameStartedCallback is called, the server paramter passed to
// it will be NULL.
///////////////////////////////////////////////////////////////////
void peerJoinStagingRoomByChannelA
(
	PEER peer,  // The peer object.
	const char * channel,  // The channel to join.
	const char password[PEER_PASSWORD_LEN],  // The password of the room being joined.  Can be NULL or "".
	peerJoinRoomCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking   // If PEERTrue, don't return until finished.
);

// Creates a new staging room, with the local player hosting.
// PANTS|09.11.00 - If the password parameter is not NULL
// or "", this will create a passworded room.  The same
// case-sensitive password needs to be passed into
// peerJoinStagingRoom() for other player's to join the room.
// PANTS|09.11.00 - The staging room will be reported as part
// of whatever group room the local player was in when the
// room was created.  Leaving the group room will not affect
// what group the staging room is reported as part of.
/////////////////////////////////////////////////////////////
void peerCreateStagingRoomA
(
	PEER peer,  // The peer object.
	const char * name,  // The name of the room.
	int maxPlayers,  // The max number of players allowed in the room.
	const char password[PEER_PASSWORD_LEN],  // An optional password for the staging room
	peerJoinRoomCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking // If PEERTrue, don't return until finished.
);

// Same as peerCreateStagingRoom, but uses the provided socket for
// sending heartbeats and query replies.  This allows the game
// to share a socket with the peer SDK, which can make hosting
// games behind a NAT proxy possible.
//////////////////////////////////////////////////////////////////
void peerCreateStagingRoomWithSocketA
(
	PEER peer,  // The peer object.
	const char * name,  // The name of the room.
	int maxPlayers,  // The max number of players allowed in the room.
	const char password[PEER_PASSWORD_LEN],  // An optional password for the staging room
	SOCKET socket,  // The socket to be used for reporting.
	unsigned short port,  // The local port to which the socket is bound.
	peerJoinRoomCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking // If PEERTrue, don't return until finished.
);

// Leave a room.
// PANTS|09.11.00 - You can now leave a group room
// without being forcibly removed from a staging room.
//////////////////////////////////////////////////////
void peerLeaveRoomA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room you want to leave (TitleRoom, GroupRoom, or StagingRoom).
	const char * reason  // The reason the player is leaving (can be NULL).  PANTS|03.13.01
);

// List all the groups rooms for the currently set title.
// The fields parameter allows you to request extra info
//   on each group room.
/////////////////////////////////////////////////////////
void peerListGroupRoomsA
(
	PEER peer,  // The peer object.
	const char * fields,  // A backslash delimited list of fields.
	peerListGroupRoomsCallback callback,  // Called for each group room.
	void * param,  // Passed to the callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Start listing the currently running games and staging rooms.
// This is used to maintain a list that can presented to the user,
//   so they can pick a game (or staging room) to join.
// Games and staging rooms are filtered based on what group the local
//   user is in.  If the local user isn't in a group, then only games
//   and staging rooms that aren't part of any group are listed.
// The fields determine which info to request from the server.  These
//   must be registered QR2 keys (see qr2regkeys.h).  HOSTNAME_KEY and
//   GAMEMODE_KEY are automatically requested by Peer, so do not need
//   to included.
/////////////////////////////////////////////////////////////////////
void peerStartListingGamesA
(
	PEER peer,  // The peer object.
	const unsigned char * fields,  // An array of registered QR2 keys to request from servers.
	int numFields,  // The number of keys in the fields array.
	const char * filter,  // A SQL-like rule filter.
	peerListingGamesCallback callback,  // Called when finished.
	void * param  // Passed to the callback.
);

// Send a message to a room.
////////////////////////////
void peerMessageRoomA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to send the message to.
	const char * message,  // The message.
	MessageType messageType  // The type of message.
);

// Send a UTM to a room.
////////////////////////
void peerUTMRoomA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to send the UTM to.
	const char * command,  // The command.
	const char * parameters,  // The UTM's parameters.
	PEERBool authenticate  // If true, the server will authenticate this UTM (should normally be false).
);

// Set a password in a room you're hosting.
// The only roomType currently supported is StagingRoom.
// This will only work if the player is hosting the room.
// If password is NULL or "", the password will be cleared.
///////////////////////////////////////////////////////////
void peerSetPasswordA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room in which to set the password.
	const char password[PEER_PASSWORD_LEN]  // The password to set.
);

// Set the name of a room you're hosting.
// The only roomType currently supported is StagingRoom.
// PANTS|09.11.00
////////////////////////////////////////////////////////
void peerSetRoomNameA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room in which to set the name.
	const char * name  // The new name
);

// Get a room's name (the channel's title).
// Returns NULL if not in the room.
///////////////////////////////////////////
const char * peerGetRoomNameA
(
	PEER peer,  // The peer object.
	RoomType roomType  // The room to get the name for.
);

// Get's the chat channel associated with the room.
// Returns NULL if not in the room.
///////////////////////////////////////////////////
const char * peerGetRoomChannelA
(
	PEER peer,  // The peer object.
	RoomType roomType  // The room to get the channel for.
);

// Use this channel for the title room for the currently set title.
// This function is normally not needed.
///////////////////////////////////////////////////////////////////
void peerSetTitleRoomChannelA
(
	PEER peer,  // The peer object.
	const char * channel  // The channel to use for the title room.
);


/************
** PLAYERS **
************/
// Send a message to a player.
//////////////////////////////
void peerMessagePlayerA
(
	PEER peer,  // The peer object.
	const char * nick,  // The nick of the player to send the message to.
	const char * message,  // The message to send.
	MessageType messageType  // The type of message.
);

// Send a UTM to a player.
//////////////////////////
void peerUTMPlayerA
(
	PEER peer,  // The peer object.
	const char * nick,  // The nick of the player to send the UTM to.
	const char * command,  // The command.
	const char * parameters,  // The UTM's parameters.
	PEERBool authenticate  // If true, the server will authenticate this UTM (should normally be false).
);

// Kick a player from a room.
// Can only be called by a player with host privileges.
///////////////////////////////////////////////////////
void peerKickPlayerA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to kick the player from.
	const char * nick,  // The nick of the player to kick.
	const char * reason  // An optional reason for kicking the player
);

// Gets a player's ping (between the local machine and the player's machine).
// Returns PEERFalse if we don't have or can't get the player's ping.
/////////////////////////////////////////////////////////////////////////////
PEERBool peerGetPlayerPingA
(
	PEER peer,  // The peer object.
	const char * nick,  // The player to get the ping for.
	int * ping  // The player's ping is stored here, if we have it.
);

// Gets the cross-ping between 2 players.
// Returns PEERFalse if we don't have or can't get the player's cross-ping.
///////////////////////////////////////////////////////////////////////////
PEERBool peerGetPlayersCrossPingA
(
	PEER peer,  // The peer object.
	const char * nick1,  // The first player.
	const char * nick2,  // The second player.
	int * crossPing  // The cross-ping is stored here, if we have it.
);

// Peer already automatically pings all players that are in ping rooms.
// This function does a one-time ping of a remote player in a non-ping room.
// However pings must be enabled in at least one room for this to work,
// otherwise Peer will not open the UDP ping socket.
// Also, peerAlwaysGetPlayerInfo() must be enabled so that Peer has IPs for
// players that are only in non-ping rooms.
////////////////////////////////////////////////////////////////////////////
PEERBool peerPingPlayerA
(
	PEER peer,  // The peer object.
	const char * nick  // The player to ping.
);

// Gets a player's info immediately.
// Returns PEERFalse if the info is no available.
/////////////////////////////////////////////////
PEERBool peerGetPlayerInfoNoWaitA
(
	PEER peer,
	const char * nick,
	unsigned int * IP,
	int * profileID
);

// Called to get a player's IP and profile ID.
//////////////////////////////////////////////
void peerGetPlayerInfoA
(
	PEER peer,  // The peer object.
	const char * nick,  // The player's nick.
	peerGetPlayerInfoCallback callback,  // Called when finished.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Called to get a player's profile ID.
// DEPRECATED - Use peerGetPlayerInfo.
///////////////////////////////////////
void peerGetPlayerProfileIDA
(
	PEER peer,  // The peer object.
	const char * nick,  // The player's nick.
	peerGetPlayerProfileIDCallback callback,  // Called when finished.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Called to get a player's IP.
// DEPRECATED - Use peerGetPlayerInfo.
//////////////////////////////////////
void peerGetPlayerIPA
(
	PEER peer,  // The peer object.
	const char * nick,  // The player's nick.
	peerGetPlayerIPCallback callback,  // Called when finished.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Checks if a player is a host (has ops).
// Returns PEERTrue if yes, PEERFalse if no.
////////////////////////////////////////////
PEERBool peerIsPlayerHostA
(
	PEER peer,  // The peer object.
	const char * nick,  // The player's nick.
	RoomType roomType  // The room to check in.
);

// Gets a player's flags in a room.  Returns PEERFalse if
// the local player or this player is not in the room.
/////////////////////////////////////////////////////////
PEERBool peerGetPlayerFlagsA
(
	PEER peer,
	const char * nick,
	RoomType roomType,
	int * flags
);

/*********
** GAME **
*********/
// Gets a player's ready state.
///////////////////////////////
PEERBool peerGetReadyA
(
	PEER peer,  // The peer object.
	const char * nick, // The player's nick.
	PEERBool * ready  // The player's ready state gets stored in here.
);

// Called only by a staging room host to start the game.
// All the other people in the staging room will have their
//   peerGameStartedCallback() called.
// The message gets passed to everyone in the peerGameStartedCallback(), and
//   can be used to pass information such as a special port or password.
// If (reportingOptions & PEER_STOP_REPORTING), Peer will stop game reporting,
//   so the program is responsible for any server reporting.
// If !(reportingOptions & PEER_STOP_REPORTING), Peer will continue doing 
//   game reporting, and calling the program-supplied callbacks.  If
//   (reportingOptions & PEER_REPORT_INFO), all server keys will still be
//   reported.  If (reportingOptions & PEER_REPORT_PLAYERS), all player keys
//   will still be reported.
/////////////////////////////////////////////////////////////////////////////
void peerStartGameA
(
	PEER peer,  // The peer object.
	const char * message,  // A message to send to everyone.
	int reportingOptions  // Bitfield flags used to set reporting options.
);

/*********
** KEYS **
*********/
// Set global keys on the local player.
///////////////////////////////////////
void peerSetGlobalKeysA
(
	PEER peer,  // The peer object.
	int num,  // The number of keys to set.
	const char ** keys,  // The keys to set.
	const char ** values  // The values for the keys.
);


// Get a player's global keys.
//////////////////////////////
void peerGetPlayerGlobalKeysA
(
	PEER peer,  // The peer object.
	const char * nick,  // The player to get the keys for.
	int num,  // The number of keys.
	const char ** keys,  // The keys to get.
	peerGetGlobalKeysCallback callback,  // Called with the keys.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Get the global keys for all players in a room we're in.
//////////////////////////////////////////////////////////
void peerGetRoomGlobalKeysA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to get the keys in.
	int num,  // The number of keys.
	const char ** keys,  // The keys to get.
	peerGetGlobalKeysCallback callback,  // Called with the keys.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Set the room keys for a player.
// Use NULL or "" to set keys on the room itself.
/////////////////////////////////////////////////
void peerSetRoomKeysA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to set the keys in.
	const char * nick,  // The player to set the keys on (NULL or "" for the room).
	int num,  // The number of keys.
	const char ** keys,  // The keys to set.
	const char ** values  // The values to set.
);

// Get the room keys for either a single player of an entire room.
// Use "*" for the player to get the keys for the entire room.
// Use NULL or "" for the player to get keys on the room itself.
//////////////////////////////////////////////////////////////////
void peerGetRoomKeysA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to get the keys in.
	const char * nick,  // The player to get the keys for.
	int num,  // The number of keys.
	const char ** keys,  // The keys to get.
	peerGetRoomKeysCallback callback,  // Called with the keys.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Set the global watch keys for a room type.
// If addKeys is set to PEERTrue, the keys will be
// added to the current global watch keys for this room.
// If addKeys is PEERFalse, these will replace any existing
// global watch keys for this room.
// When entering a room of the given type, peer will get and
// cache these keys for all players in the room.
////////////////////////////////////////////////////////////
void peerSetGlobalWatchKeysA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room to set the watch keys for.
	int num,  // The number of keys.
	const char ** keys,  // The keys to watch for.
	PEERBool addKeys  // If PEERTrue, add these keys to the existing global watch keys for this room.
);

// Set the room watch keys for a room type.
// If addKeys is set to PEERTrue, the keys will be
// added to the current room watch keys for this room.
// If addKeys is PEERFalse, these will replace any existing
// room watch keys for this room.
// When entering a room of the given type, peer will get and
// cache these keys for all players in the room.
////////////////////////////////////////////////////////////
void peerSetRoomWatchKeysA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room to set the watch keys for.
	int num,  // The number of keys.
	const char ** keys,  // The keys to watch for.
	PEERBool addKeys  // If PEERTrue, add these keys to the existing room watch keys for this room.
);

// Get the global watch key for a particular player.
// If the key isn't cached locally (either because it isn't
// a watch key or just isn't yet known), NULL will be returned.
// If the key is empty, "" will be returned.
///////////////////////////////////////////////////////////////
const char * peerGetGlobalWatchKeyA
(
	PEER peer,  // The peer object.
	const char * nick,  // The player to get the key for.
	const char * key  // The key to get.
);

// Get the room watch key for a particular player in a room.
// If the key isn't cached locally (either because it isn't
// a watch key or just isn't yet known), NULL will be returned.
// If the key is empty, "" will be returned.
///////////////////////////////////////////////////////////////
const char * peerGetRoomWatchKeyA
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to get the key in.
	const char * nick,  // The player to get the key for.
	const char * key  // The key to get.
);

/**************
** AUTOMATCH **
**************/

// Used to start a automatch attempt.
// The filter contains any hard criteria.  This is used to narrow down the list
// of possible matches to those the user might consider.
// The status callback will be called as the attempt progresses, and the rate
// callback will be used to sort possible matches.
/////////////////////////////////////////////////////////////////////////////////
void peerStartAutoMatchA
(
	PEER peer,  // The peer object.
	int maxPlayers,  // The total number of players to match (including the local player).
	const char * filter,  // Hard criteria - filters out servers.
	peerAutoMatchStatusCallback statusCallback,  // Called as the attempt status changes.
	peerAutoMatchRateCallback rateCallback,  // Used to rate possible matches.
	void * param,  // User-data.
	PEERBool blocking   // If PEERTrue, don't return until finished.
);

// Same as peerStartAutoMatch, but uses the provided socket for
// sending heartbeats and query replies.  This allows the game
// to share a socket with the peer SDK, which can make hosting
// games behind a NAT proxy possible.
////////////////////////////////////////////////////////////////
void peerStartAutoMatchWithSocketA
(
	PEER peer,  // The peer object.
	int maxPlayers,  // The total number of players to match (including the local player).
	const char * filter,  // Hard criteria - filters out servers.
	SOCKET socket,  // The socket to be used for reporting.
	unsigned short port,  // The local port to which the socket is bound.
	peerAutoMatchStatusCallback statusCallback,  // Called as the attempt status changes.
	peerAutoMatchRateCallback rateCallback,  // Used to rate possible matches.
	void * param,  // User-data.
	PEERBool blocking   // If PEERTrue, don't return until finished.
);


#ifdef __cplusplus
}
#endif

#endif
