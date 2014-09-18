 /*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _PEER_H_
#define _PEER_H_

/*************
** INCLUDES **
*************/
#include "../common/gsCommon.h"
#include "../Chat/chat.h"
#include "../qr2/qr2.h"
#include "../serverbrowsing/sb_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/************
** DEFINES **
************/
// Defines for the msg param that's passed into peerListingGamesCallback().
// PANTS-04.20.00-changed from PI_* to PEER_*
// PANTS-09.26.02-added PEER_COMPLETE
///////////////////////////////////////////////////////////////////////////
#define PEER_ADD        0  // a server is being added
#define PEER_UPDATE     1  // a server has been updated
#define PEER_REMOVE     2  // a server has been removed
#define PEER_CLEAR      3  // all the servers have been cleared
#define PEER_COMPLETE   4  // the initial listing of servers is complete

// Nick errors, for peerNickErrorCallback.
//////////////////////////////////////////
#define PEER_NICK_OK                0
#define PEER_IN_USE                 1  // the nick is already being used
#define PEER_INVALID                2  // the nick contains invalid characters
#define PEER_UNIQUENICK_EXPIRED     3  // the uniquenick for this account has expired
#define PEER_NO_UNIQUENICK          4  // there is no uniquenick for this account
#define PEER_INVALID_UNIQUENICK     5  // the uniquenick to associate with the account is invalid or in use
#define PEER_NICK_TOO_LONG          6  // the nick was too long 
	
// Possible values for the failureReason passed to the peerConnectCallback.
///////////////////////////////////////////////////////////////////////////
#define PEER_DISCONNECTED           0  // disconnected from, or unable to connect to, the server
#define PEER_NICK_ERROR             1  // a nick error was either ignored or not handled
#define PEER_LOGIN_FAILED           2  // the login info passed to peerConnectLogin was not valid

// Maximum length of a room password, including the terminating NUL.
////////////////////////////////////////////////////////////////////
#define PEER_PASSWORD_LEN     24

// Each player can have various flags set for each room they are in.
////////////////////////////////////////////////////////////////////
#define PEER_FLAG_STAGING     0x01  // s
#define PEER_FLAG_READY       0x02  // r
#define PEER_FLAG_PLAYING     0x04  // g
#define PEER_FLAG_AWAY        0x08  // a
#define PEER_FLAG_HOST        0x10  // h
#define PEER_FLAG_OP          0x20
#define PEER_FLAG_VOICE       0x40

// Bitfield reporting options for peerStartGame.
////////////////////////////////////////////////
#define PEER_KEEP_REPORTING      0  // Continue reporting.
#define PEER_STOP_REPORTING      1  // Stop reporting.  Cannot be used with other options.
#define PEER_REPORT_INFO         2  // Continue reporting server keys (as if it were not playing).
#define PEER_REPORT_PLAYERS      4  // Continue reporting player keys (as if it were not playing).

/**********
** TYPES **
**********/
// The peer object.
///////////////////
typedef void * PEER;

// Boolean.
///////////
typedef enum
{
	PEERFalse,
	PEERTrue
} PEERBool;

// Types of rooms.
//////////////////
typedef enum
{
	TitleRoom,  // The main room for a game.
	GroupRoom,  // A room which is, in general, for a particular type of gameplay (team, dm, etc.).
	StagingRoom,  // A room where players meet before starting a game.
	NumRooms
} RoomType;

// Types of messages. These have the same
// values as their CHAT SDK counterparts.
// PANTS-01.08.01
/////////////////////////////////////////
typedef enum
{
	NormalMessage,
	ActionMessage,
	NoticeMessage
} MessageType;

// Possible results when attempting to join a room.
// Passed into peerJoinRoomCallback().
///////////////////////////////////////////////////
typedef enum
{
	PEERJoinSuccess,     // The room was joined.

	PEERFullRoom,        // The room is full.
	PEERInviteOnlyRoom,  // The room is invite only.
	PEERBannedFromRoom,  // The local user is banned from the room.
	PEERBadPassword,     // An incorrect password (or none) was given for a passworded room.

	PEERAlreadyInRoom,   // The local user is already in or entering a room of the same type.
	PEERNoTitleSet,      // Can't join a room if no title is set.
	PEERNoConnection,    // Can't join a room if there's no chat connection.
	PEERAutoMatching,    // The user can't join a staging room during an auto match attempt.

	PEERJoinFailed       // Generic failure.
} PEERJoinResult;

// Possible status values passed to the peerAutoMatchStatusCallback.
// If PEERFailed, the match failed, otherwise this is the current status
// of the automatch attempt.
////////////////////////////////////////////////////////////////////////
typedef enum
{
	PEERFailed,      // The automatch attempt failed.

	PEERSearching,   // Searching for a match (active).
	PEERWaiting,     // Waiting for a match (passive).
	PEERStaging,     // In a staging room with at least one other player, possibly waiting for more.
	PEERReady,       // All players are in the staging room, the game is ready to be launched.
	PEERComplete     // The game is launching, the automatch attempt is now complete.
	                 // The player is still in the staging room.
} PEERAutoMatchStatus;

/**************
** CALLBACKS **
**************/
// Called when the connection to the server gets disconnected.
//////////////////////////////////////////////////////////////
typedef void (* peerDisconnectedCallback)
(
	PEER peer,  // The peer object.
	const gsi_char * reason,  // The reason for the disconnection.
	void * param  // User-data.
);

// Called when a message is sent to a room the local player is in.
//////////////////////////////////////////////////////////////////
typedef void (* peerRoomMessageCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room that the message was in.
	const gsi_char * nick,  // The nick of the player who sent the message.
	const gsi_char * message,  // The text of the message.
	MessageType messageType,  // The type of message.
	void * param  // User-data.
);

// Called when a UTM is sent to a room the local player is in.
//////////////////////////////////////////////////////////////
typedef void (* peerRoomUTMCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room that the UTM was in.
	const gsi_char * nick,  // The nick of the player who sent the UTM.
	const gsi_char * command, // The UTM command for this message.
	const gsi_char * parameters,  // Any parameters for this UTM.
	PEERBool authenticated,  // True if this has been authenticated by the server.
	void * param  // User-data.
);

// Called when the name of a room the player is in changes.
// The new name can be checked with peerGetRoomName.
// PANTS|09.11.00
///////////////////////////////////////////////////////////
typedef void (* peerRoomNameChangedCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room that the name changed in.
	void * param  // User-data
);

// Called when a room's mode changes.
// PANTS|04.17.00
///////////////////////////////////////////////////////////
typedef void (* peerRoomModeChangedCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room that the name changed in.
	CHATChannelMode * mode,  // The current mode for this room.
	void * param  // User-data
);

// Called when a private message is received from another player.
/////////////////////////////////////////////////////////////////
typedef void (* peerPlayerMessageCallback)
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The nick of the player who sent the message.
	const gsi_char * message,  // The text of the message.
	MessageType messageType,  // The type of message.
	void * param  // User-data
);

// Called when a private UTM is received from another player.
/////////////////////////////////////////////////////////////
typedef void (* peerPlayerUTMCallback)
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The nick of the player who sent the UTM.
	const gsi_char * command, // The UTM command for this message.
	const gsi_char * parameters,  // Any parameters for this UTM.
	PEERBool authenticated,  // True if this has been authenticated by the server.
	void * param  // User-data
);

// Called when a player's ready state changes,
// from a call to peerSetReady().
//////////////////////////////////////////////
typedef void (* peerReadyChangedCallback)
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The nick of the player who's ready state changed.
	PEERBool ready,  // The player's new ready state.
	void * param  // User-data.
);

// Called when the host of a staging room launches the game,
// with a call to peerStartGame().
// The public and private IPs and ports of the server can
// be obtained from the server object.
////////////////////////////////////////////////////////////
typedef void (* peerGameStartedCallback)
(
	PEER peer,  // The peer object.
	SBServer server,  // A server object representing this host.
	const gsi_char * message,  // A message that was passed into peerStartGame().
	void * param  // User-data.
);

// A player joined a room.
//////////////////////////
typedef void (* peerPlayerJoinedCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room that the player joined.
	const gsi_char * nick,  // The nick of the player that joined.
	void * param  // User-data.
);

// A player left a room.
////////////////////////
typedef void (* peerPlayerLeftCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room that the player left.
	const gsi_char * nick,  // The nick of the player that left.
	const gsi_char * reason,  // The reason the player left.
	void * param  // User-data.
);

// The local player was kicked from a room.
///////////////////////////////////////////
typedef void (* peerKickedCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room that the player was kicked from.
	const gsi_char * nick,  // The nick of the player that did the kicking.
	const gsi_char * reason,  // An optional reason for the kick.
	void * param  // User-data.
);

// The entire player list for this room has been updated.
/////////////////////////////////////////////////////////
typedef void (* peerNewPlayerListCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room.
	void * param  // User-data
);

// A player in one of the rooms changed his nick.
/////////////////////////////////////////////////
typedef void (* peerPlayerChangedNickCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of the room the nick changed was in.
	const gsi_char * oldNick,  // The player's old nick.
	const gsi_char * newNick,  // The player's new nick.
	void * param  // User-data.
);

// The IP and ProfileID for this player has just been received.
// PANTS|01.08.01
// This gets called for all players (who are using peer) in a room
// shortly after joining.  It will be called with nick==NULL after
// getting info for all the players.
//////////////////////////////////////////////////////////////////
typedef void (* peerPlayerInfoCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room the info was gotten in.
	const gsi_char * nick,  // The nick of the player the info is for.
	unsigned int IP,  // The player's IP.
	int profileID,  // The player's profile ID.
	void * param  // User-data.
);

// This gets called when a player's flags have changed.
///////////////////////////////////////////////////////
typedef void (* peerPlayerFlagsChangedCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room the flags were changed in.
	const gsi_char * nick,  // The player whose flags have changed.
	int oldFlags,  // The player's old flags.
	int newFlags,  // The player's new flags.
	void * param  // User-data
);

// An updated ping for a player, who may be in any room(s).
///////////////////////////////////////////////////////////
typedef void (* peerPingCallback)
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The other player's nick.
	int ping,  // The ping.
	void * param  // User-data.
);

// An updated cross-ping between two players in the staging room.
/////////////////////////////////////////////////////////////////
typedef void (* peerCrossPingCallback)
(
	PEER peer,  // The peer object.
	const gsi_char * nick1,  // The first player's nick.
	const gsi_char * nick2,  // The second player's nick.
	int crossPing,  // The cross-ping.
	void * param  // User-data.
);

// This is called for watch keys when a room is joined, for
// watch keys when another player joins, and for any newly 
// set watch keys.
///////////////////////////////////////////////////////////
typedef void (* peerGlobalKeyChangedCallback)
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The player whose key changed.
	const gsi_char * key,  // The key.
	const gsi_char * value,  // The value.
	void * param  // User-data.
);

// This is called for watch keys when a room is joined, for
// watch keys when another player joins, for any newly 
// set watch keys, and when a broadcast key is changed.
///////////////////////////////////////////////////////////
typedef void (* peerRoomKeyChangedCallback)
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room the player is in.
	const gsi_char * nick,  // The player whose key changed.
	const gsi_char * key,  // The key.
	const gsi_char * value,  // The value.
	void * param  // User-data.
);

// Called to report QR server keys.
// Use qr2_buffer_add or qr2_buffer_add_int to
// add this key's information to the buffer.
//////////////////////////////////////////////
typedef void (* peerQRServerKeyCallback)
(
	PEER peer,  // The peer object.
	int key,  // The key for which to report information.
	qr2_buffer_t buffer,  // Fill in the information using this buffer.
	void * param  // User-data.
);

// Called to report QR player keys.
// Use qr2_buffer_add or qr2_buffer_add_int to
// add this key's information to the buffer.
//////////////////////////////////////////////
typedef void (* peerQRPlayerKeyCallback)
(
	PEER peer,  // The peer object.
	int key,  // The key for which to report information.
	int index,  // The index of the player for which to report info.
	qr2_buffer_t buffer,  // Fill in the information using this buffer.
	void * param  // User-data.
);

// Called to report QR team keys.
// Use qr2_buffer_add or qr2_buffer_add_int to
// add this key's information to the buffer.
//////////////////////////////////////////////
typedef void (* peerQRTeamKeyCallback)
(
	PEER peer,  // The peer object.
	int key,  // The key for which to report information.
	int index,  // The index of the team for which to report info.
	qr2_buffer_t buffer,  // Fill in the information using this buffer.
	void * param  // User-data.
);

// Called to get a list of keys to be reported.
// Use qr2_keybuffer_add() to add keys.
// The following keys do not need to be added, as peer already adds them:
// GAMENAME_KEY, HOSTNAME_KEY, NUMPLAYERS_KEY, MAXPLAYERS_KEY,
// GAMEMODE_KEY, PASSWORD_KEY, GROUPID_KEY, PLAYER__KEY, PING__KEY.
/////////////////////////////////////////////////////////////////////////
typedef void (* peerQRKeyListCallback)
(
	PEER peer,  // The peer object.
	qr2_key_type type,  // The type of keys being asked for (key_server, key_player, or key_team).
	qr2_keybuffer_t keyBuffer,  // Fill in the keys using this buffer.
	void * param  // User-data.
);

// Called to get a count of the number of players or teams.
///////////////////////////////////////////////////////////
typedef int (* peerQRCountCallback)
(
	PEER peer,  // The peer object.
	qr2_key_type type,  // The type of count to return (key_player or key_team).
	void * param  // User-data.
);

// Called when there is an error reporting the server.
//////////////////////////////////////////////////////
typedef void (* peerQRAddErrorCallback)
(
	PEER peer,  // The peer object.
	qr2_error_t error,  // The type of error.
	gsi_char * errorString,  // A text string containing the error.
	void * param  // User-data.
);

// Called when hosting a server and a nat-negotiate cookie is received.
///////////////////////////////////////////////////////////////////////
typedef void (* peerQRNatNegotiateCallback)
(
	PEER peer,  // The peer object.
	int cookie,  // A cookie sent from a potential client.
	void * param  // User-data.
);

// Called when hosting a server with the server's public reporting address.
///////////////////////////////////////////////////////////////////////////
typedef void (* peerQRPublicAddressCallback)
(
	PEER peer,  // The peer object.
	unsigned int ip,  // The public reporting IP
	unsigned short port,  // The public reporting port
	void * param  // User-data.
);

// This struct gets passed into peerInitialize().
// param will be passed as the last parameter to each of the callbacks.
///////////////////////////////////////////////////////////////////////
typedef struct PEERCallbacks
{
	peerDisconnectedCallback disconnected;
	peerRoomMessageCallback roomMessage;
	peerRoomUTMCallback roomUTM;
	peerRoomNameChangedCallback roomNameChanged;  // PANTS|09.11.00
	peerRoomModeChangedCallback roomModeChanged;  // PANTS|04.17.01
	peerPlayerMessageCallback playerMessage;
	peerPlayerUTMCallback playerUTM;
	peerReadyChangedCallback readyChanged;
	peerGameStartedCallback gameStarted;
	peerPlayerJoinedCallback playerJoined;
	peerPlayerLeftCallback playerLeft;
	peerKickedCallback kicked;
	peerNewPlayerListCallback newPlayerList;
	peerPlayerChangedNickCallback playerChangedNick;
	peerPlayerInfoCallback playerInfo;  // PANTS|01.08.01
	peerPlayerFlagsChangedCallback playerFlagsChanged;  // PANTS|03.12.01
	peerPingCallback ping;
	peerCrossPingCallback crossPing;
	peerGlobalKeyChangedCallback globalKeyChanged;
	peerRoomKeyChangedCallback roomKeyChanged;
	peerQRServerKeyCallback qrServerKey;
	peerQRPlayerKeyCallback qrPlayerKey;
	peerQRTeamKeyCallback qrTeamKey;
	peerQRKeyListCallback qrKeyList;
	peerQRCountCallback qrCount;
	peerQRAddErrorCallback qrAddError;
	peerQRNatNegotiateCallback qrNatNegotiateCallback;
	peerQRPublicAddressCallback qrPublicAddressCallback;
	void * param;
} PEERCallbacks;

/************
** UNICODE **
************/
#ifndef GSI_UNICODE
#define peerConnect					peerConnectA
#define peerConnectLogin            peerConnectLoginA
#define peerConnectPreAuth          peerConnectPreAuthA
#define peerRetryWithNick			peerRetryWithNickA
#define peerRegisterUniqueNick      peerRegisterUniqueNickA
#define peerSetTitle				peerSetTitleA
#define peerGetTitle				peerGetTitleA
#define peerGetNick					peerGetNickA
#define peerFixNick                 peerFixNickA
#define peerTranslateNick           peerTranslateNickA
#define peerChangeNick				peerChangeNickA
#define peerSetAwayMode				peerSetAwayModeA
#define peerParseQuery				peerParseQueryA
#define peerAuthenticateCDKey		peerAuthenticateCDKeyA
#define peerJoinTitleRoom			peerJoinTitleRoomA
#define peerJoinStagingRoom			peerJoinStagingRoomA
#define peerJoinStagingRoomByChannel	peerJoinStagingRoomByChannelA
#define peerCreateStagingRoom		peerCreateStagingRoomA
#define peerCreateStagingRoomWithSocket	peerCreateStagingRoomWithSocketA
#define peerLeaveRoom				peerLeaveRoomA
#define peerListGroupRooms			peerListGroupRoomsA
#define peerStartListingGames		peerStartListingGamesA
#define peerMessageRoom				peerMessageRoomA
#define peerUTMRoom					peerUTMRoomA
#define peerSetPassword				peerSetPasswordA
#define peerSetRoomName				peerSetRoomNameA
#define peerGetRoomName				peerGetRoomNameA
#define peerGetRoomChannel			peerGetRoomChannelA
#define peerSetTitleRoomChannel		peerSetTitleRoomChannelA
#define peerMessagePlayer			peerMessagePlayerA
#define peerUTMPlayer				peerUTMPlayerA
#define peerKickPlayer				peerKickPlayerA
#define peerGetPlayerPing			peerGetPlayerPingA
#define peerGetPlayersCrossPing		peerGetPlayersCrossPingA
#define peerPingPlayer				peerPingPlayerA 
#define peerGetPlayerInfoNoWait		peerGetPlayerInfoNoWaitA
#define peerGetPlayerInfo			peerGetPlayerInfoA
#define peerGetPlayerProfileID		peerGetPlayerProfileIDA
#define peerGetPlayerIP				peerGetPlayerIPA
#define peerIsPlayerHost			peerIsPlayerHostA
#define peerGetPlayerFlags			peerGetPlayerFlagsA
#define peerGetReady				peerGetReadyA
#define peerStartGame				peerStartGameA
#define peerSetGlobalKeys			peerSetGlobalKeysA
#define peerGetPlayerGlobalKeys		peerGetPlayerGlobalKeysA
#define peerGetRoomGlobalKeys		peerGetRoomGlobalKeysA
#define peerSetRoomKeys				peerSetRoomKeysA
#define peerGetRoomKeys				peerGetRoomKeysA
#define peerSetGlobalWatchKeys		peerSetGlobalWatchKeysA
#define peerSetRoomWatchKeys		peerSetRoomWatchKeysA
#define peerGetGlobalWatchKey		peerGetGlobalWatchKeyA
#define peerGetRoomWatchKey			peerGetRoomWatchKeyA
#define peerStartAutoMatch			peerStartAutoMatchA
#define peerStartAutoMatchWithSocket	peerStartAutoMatchWithSocketA
#else
#define peerConnect					peerConnectW
#define peerConnectLogin            peerConnectLoginW
#define peerConnectPreAuth          peerConnectPreAuthW
#define peerRetryWithNick			peerRetryWithNickW
#define peerRegisterUniqueNick      peerRegisterUniqueNickW
#define peerSetTitle				peerSetTitleW
#define peerGetTitle				peerGetTitleW
#define peerGetNick					peerGetNickW
#define peerFixNick                 peerFixNickW
#define peerTranslateNick           peerTranslateNickW
#define peerChangeNick				peerChangeNickW
#define peerSetAwayMode				peerSetAwayModeW
#define peerParseQuery				peerParseQueryA
#define peerAuthenticateCDKey		peerAuthenticateCDKeyW
#define peerJoinTitleRoom			peerJoinTitleRoomW
#define peerJoinStagingRoom			peerJoinStagingRoomW
#define peerJoinStagingRoomByChannel	peerJoinStagingRoomByChannelW
#define peerCreateStagingRoom		peerCreateStagingRoomW
#define peerCreateStagingRoomWithSocket	peerCreateStagingRoomWithSocketW
#define peerLeaveRoom				peerLeaveRoomW
#define peerListGroupRooms			peerListGroupRoomsW
#define peerStartListingGames		peerStartListingGamesW
#define peerMessageRoom				peerMessageRoomW
#define peerUTMRoom					peerUTMRoomW
#define peerSetPassword				peerSetPasswordW
#define peerSetRoomName				peerSetRoomNameW
#define peerGetRoomName				peerGetRoomNameW
#define peerGetRoomChannel			peerGetRoomChannelW
#define peerSetTitleRoomChannel		peerSetTitleRoomChannelW
#define peerMessagePlayer			peerMessagePlayerW
#define peerUTMPlayer				peerUTMPlayerW
#define peerKickPlayer				peerKickPlayerW
#define peerGetPlayerPing			peerGetPlayerPingW
#define peerGetPlayersCrossPing		peerGetPlayersCrossPingW
#define peerPingPlayer				peerPingPlayerW
#define peerGetPlayerInfoNoWait		peerGetPlayerInfoNoWaitW
#define peerGetPlayerInfo			peerGetPlayerInfoW
#define peerGetPlayerProfileID		peerGetPlayerProfileIDW
#define peerGetPlayerIP				peerGetPlayerIPW
#define peerIsPlayerHost			peerIsPlayerHostW
#define peerGetPlayerFlags			peerGetPlayerFlagsW
#define peerGetReady				peerGetReadyW
#define peerStartGame				peerStartGameW
#define peerSetGlobalKeys			peerSetGlobalKeysW
#define peerGetPlayerGlobalKeys		peerGetPlayerGlobalKeysW
#define peerGetRoomGlobalKeys		peerGetRoomGlobalKeysW
#define peerSetRoomKeys				peerSetRoomKeysW
#define peerGetRoomKeys				peerGetRoomKeysW
#define peerSetGlobalWatchKeys		peerSetGlobalWatchKeysW
#define peerSetRoomWatchKeys		peerSetRoomWatchKeysW
#define peerGetGlobalWatchKey		peerGetGlobalWatchKeyW
#define peerGetRoomWatchKey			peerGetRoomWatchKeyW
#define peerStartAutoMatch			peerStartAutoMatchW
#define peerStartAutoMatchWithSocket	peerStartAutoMatchWithSocketW
#endif

/************
** GENERAL **
************/
// This creates and intializes a peer object.
// NULL is returned in case of an error, otherwise a peer
// object is returned.
// If a peer object is returned, peerShutdown() must be called
// to cleanup the object
///////////////////////////////////////////////////////////////
PEER peerInitialize
(
	PEERCallbacks * callbacks  // Global callbacks.
);

// This gets called when the connection attempt finishes.
/////////////////////////////////////////////////////////
typedef void (* peerConnectCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // PEERTrue if success, PEERFalse if failure.
	int failureReason,  // If failure, the reason for it (PEER_DISCONNECTED, etc.)
	void * param  // User-data.
);

// This gets called if there is an error with
//   the nickname while connecting.
// Call peerRetryWithNick() to try another nick.  If it
//   is called with a NULL nick, then the connect will be
//   stopped, and peerConnectCallback will be called with
//   failure.
// The suggested nicks are only provided if type
//   is PEER_INVALID_UNIQUENICK.
/////////////////////////////////////////////////////////
typedef void (* peerNickErrorCallback)
(
	PEER peer,  // The peer object.
	int type,  // The type of nick error (PEER_IN_USE, PEER_INVALID, etc.)
	const gsi_char * nick,  // The bad nick.
	int numSuggestedNicks,  // The number of suggested nicks.
	const gsi_char ** suggestedNicks,  // The array of nicks.
	void * param  // User-data.
);

// This connects a peer object to a chat server.
// Call peerDisconnect() to close the connection.
// A title must be set with peerSetTitle before connecting.
///////////////////////////////////////////////////////////
void peerConnect
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The nick to connect with.
	int profileID,  // The profileID, or 0 if no profileID.
	peerNickErrorCallback nickErrorCallback,  // Called if nick error.
	peerConnectCallback connectCallback,  // Called on complete.
	void * param,  // User-data.
	PEERBool blocking   // If PEERTrue, don't return until finished.
);

// Same as peerConnect, but also authenticates using a uniquenick
// and password or an email, profilenick, and password.
/////////////////////////////////////////////////////////////////
void peerConnectLogin
(
	PEER peer,  // The peer object.
	int namespaceID,  // The namespace in which to login.
	const gsi_char * email,  // The email to login with.
	const gsi_char * profilenick,  // The profile to login with.
	const gsi_char * uniquenick,  // The uniquenick to login with.
	const gsi_char * password,  // The password for the login account.
	peerNickErrorCallback nickErrorCallback,  // Called if nick error.
	peerConnectCallback connectCallback,  // Called on complete.
	void * param,  // User-data.
	PEERBool blocking   // If PEERTrue, don't return until finished.
);

// Same as peerConnect, but also authenticates using an authtoken
// and partnerchallenge from a partner account system.
/////////////////////////////////////////////////////////////////
void peerConnectPreAuth
(
	PEER peer,  // The peer object.
	const gsi_char * authtoken,  // The authtoken for this login.
	const gsi_char * partnerchallenge,  // The partner challenge for this login.
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
void peerRetryWithNick
(
	PEER peer,
	const gsi_char * nick
);

// Register a uniquenick.
// Should be called in response to the peerNickErrorCallback being called
// with a type of PEER_UNIQUENICK_EXPIRED or PEER_NO_UNIQUENICK.
// If the uniquenick cannot be registered, the peerNickErrorCallback will
// be called again with a type of PEER_IN_USE or PEER_INVALID.
/////////////////////////////////////////////////////////////////////////
void peerRegisterUniqueNick
(
	PEER peer,
	int namespaceID,
	const gsi_char * uniquenick,
	const gsi_char * cdkey
);

// Returns true if peer is connected.
// PANTS|09.11.00
/////////////////////////////////////
PEERBool peerIsConnected(PEER peer);

// Sets the current title.
// A title must be set before connecting.
// Returns PEERFalse if an error, or PEERTrue if success.
// For most games the title/qrSecretKey and
// sbName/sbSecretKey pairs will be the same.
/////////////////////////////////////////////////////////
PEERBool peerSetTitle
(
	PEER peer,  // The peer object.
	const gsi_char * title,  // The title to make current (ie., ut, gmtest).
	const gsi_char * qrSecretKey,  // The queryreporting secret key.
	const gsi_char * sbName,  // The serverbrowsing name.
	const gsi_char * sbSecretKey,  // The serverbrowsing secret key.
	int sbGameVersion,  // The version of the game doing the browsing.
	int sbMaxUpdates,  // The maximum number of concurent updates (10-15 for modem users, 20-30 for high-bandwidth).
	PEERBool natNegotiate,  // PEERTrue if the title supports GameSpy's NAT-negotiation technology (or another similar technology).
	PEERBool pingRooms[NumRooms],  // To do pings int a room, set it to PEERTrue.
	PEERBool crossPingRooms[NumRooms]  // To do cross-pings in a room, set it to PEERTrue.
);

// Resets peer to no title.
///////////////////////////
void peerClearTitle(PEER peer);

// Gets the currently set title.
// Returns NULL if no title is set.
///////////////////////////////////
const gsi_char * peerGetTitle(PEER peer);

// Disconnect peer from the chat server.
////////////////////////////////////////
void peerDisconnect(PEER peer);

// Shutdown peer.
// The peer object should not be used again after this call.
////////////////////////////////////////////////////////////
void peerShutdown(PEER peer);

// Let's peer think.
// This should be called at least every ~10ms,
// typically, within the program's main loop.
//////////////////////////////////////////////
void peerThink(PEER peer);

// Get the chat object associated with this peer object.
// This returns NULL if peer isn't connected.
////////////////////////////////////////////////////////
CHAT peerGetChat(PEER peer);

// Get the local user's nickname.
/////////////////////////////////
const gsi_char * peerGetNick(PEER peer);

// Replaces any invalid characters in nick with underscores.
////////////////////////////////////////////////////////////
void peerFixNick
(
	gsi_char * newNick,
	const gsi_char * oldNick
);

// Removes the namespace extension from a chat unique nick.
// Returns the nick if it ended with the extension.
// Otherwise returns NULL.
///////////////////////////////////////////////////////////
const gsi_char * peerTranslateNick
(
	gsi_char * nick,  // The nick to translate
	const gsi_char * extension  // The extension to be removed.
);

// Gets the local IP address.  If called before
// the peer object is connected, will return 0.
// DEPRECATED - Use peerGetPublicIP.
///////////////////////////////////////////////
#define peerGetLocalIP  peerGetPublicIP

// Gets the local public IP address.  If called before
// the peer object is connected, will return 0.
//////////////////////////////////////////////////////
unsigned int peerGetPublicIP(PEER peer);

// Gets the local private IP address.
/////////////////////////////////////
unsigned int peerGetPrivateIP(PEER peer);

// Gets the local userID.
// Only valid if connected with peerConnectLogin or peerConnectPreAuth.
///////////////////////////////////////////////////////////////////////
int peerGetUserID(PEER peer);

// Gets the local profileID.
// Only valid if connected with peerConnectLogin or peerConnectPreAuth.
///////////////////////////////////////////////////////////////////////
int peerGetProfileID(PEER peer);

// This gets called when an attempt to change nicks is finished.
////////////////////////////////////////////////////////////////
typedef void (* peerChangeNickCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // PEERTrue if success, PEERFalse if failure.
	const gsi_char * oldNick,  // The old nickname.
	const gsi_char * newNick,  // The new nickname.
	void * param  // User-data.
);

// Changes the user's nickname.
///////////////////////////////
void peerChangeNick
(
	PEER peer,  // The peer object.
	const gsi_char * newNick,  // The nickname to which to change.
	peerChangeNickCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Causes Peer to not automatically leave the
// room the next time peerSetTitle or peerClearTitle
// is called.  No effect if a title isn't set.  When
// the next title is set, the flag is cleared.
// Only TitleRoom is currently supported.
// This function is normally not needed.
////////////////////////////////////////////////////
void peerStayInRoom
(
	PEER peer,  // The peer object.
	RoomType roomType  // Only TitleRoom is currently supproted.
);

// Turns quiet mode on/off.
///////////////////////////
void peerSetQuietMode
(
	PEER peer,  // The peer object.
	PEERBool quiet  // If PEERTrue, enable quiet mode.
);

// Sets the away mode.
// If an empty string or NULL, away mode is off.
// If a valid string, away mode is on.
////////////////////////////////////////////////
void peerSetAwayMode
(
	PEER peer,  // The peer object.
	const gsi_char * reason  // The away reason.  If NULL or "", not away.
);

// When using peerStartReportingWithSocket or peerCreateStagingRoomWithSocket,
// any qureries received on the sockets need to be passed to the SDK.  Pass
// the data using this function.
//////////////////////////////////////////////////////////////////////////////
void peerParseQuery
(
	PEER peer,  // The peer object.
	char * query,  // String of query data.
	int len,  // The length of the string, not including the NUL.
	struct sockaddr * sender  // The address the query was received from.
);

// This gets called when an attempt to authenticate a CD key is finished.
// If the result is 1, the CD key was authenticated.  Otherwise, the CD
// key was not authenticated.
/////////////////////////////////////////////////////////////////////////
typedef void (* peerAuthenticateCDKeyCallback)
(
	PEER peer,  // The peer object.
	int result,  // 1 if authenticated, otherwise not authenticated.
	const gsi_char * message,  // A string representing the result.
	void * param  // User-data.
);

// Sends a CD Key to the chat server for authentication.
// The callback gets called when the chat server responds.
// This call is required by some games to enter chat rooms.
// It must be called after connecting to the chat server, but
// before entering a room.
/////////////////////////////////////////////////////////////
void peerAuthenticateCDKey
(
	PEER peer,  // The peer object.
	const gsi_char * cdkey,  // The CD key to authenticate.
	peerAuthenticateCDKeyCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Sends a nat-negotiate cookie to a server.
////////////////////////////////////////////
void peerSendNatNegotiateCookie
(
	PEER peer,  // The peer object.
	unsigned int ip,  // IP (network byte order) of the server to send the cookie to.
	unsigned short port,  // Port (host byte order) of the server to send the cookie to.
	int cookie  // The cookie to send.
);

// Sends a client message to a QR2 server.
//////////////////////////////////////////
void peerSendMessageToServer
(
	PEER peer,
	unsigned int ip,
	unsigned short port,
	const char * data,
	int len
);

// If always is PEERTrue, then players' IP and profile ID will
// always be requested when joining a room.  This info is normally
// only requested if pings are set in a room.  Use this function
// to always request the information.
//////////////////////////////////////////////////////////////////
void peerAlwaysGetPlayerInfo
(
	PEER peer,  // The peer object.
	PEERBool always  // If true, always get player info.
);

/**********
** ROOMS **
**********/
// This gets called when an attempt to join or create a room has finished.
//////////////////////////////////////////////////////////////////////////
typedef void (* peerJoinRoomCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // PEERTrue if success, PEERFalse if failure.
	PEERJoinResult result,  // The result of the attempt.
	RoomType roomType,  // The type of room joined/created.
	void * param  // User-data.
);

// Joins the currently selected title's title room.
///////////////////////////////////////////////////
void peerJoinTitleRoom
(
	PEER peer,  // The peer object.
	const gsi_char password[PEER_PASSWORD_LEN],  // An optional password, normally NULL.
	peerJoinRoomCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Joins a group room.
// The groupID comes from the peerListGroupRoomsCallback.
/////////////////////////////////////////////////////////
void peerJoinGroupRoom
(
	PEER peer,  // The peer object.
	int groupID,  // The ID for the group to join.
	peerJoinRoomCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking // If PEERTrue, don't return until finished.
);

// Sets the group ID.
// Used to get a list of games in a group room and/or report or create a game
// in a particular group, without actually being in that group room.  This is
// not needed if already using peerJoinGroupRoom.
// Setting a group ID of 0 is equivalent to leaving the group room.
/////////////////////////////////////////////////////////////////////////////
void peerSetGroupID
(
	PEER peer,  // The peer object.
	int groupID  // The group ID to set
);

// Returns the current group ID.
// The group ID is set with either peerJoinGroupRoom or peerSetGroupID.
///////////////////////////////////////////////////////////////////////
int peerGetGroupID
(
	PEER peer  // The peer object.
);

// Joins a staging room.
// server is one of the server objects passed to peerListingGamesCallback().
// This call will only work if staging==PEERTrue for the server.
// PANTS|09.11.00 - The password is only needed for passworded rooms.
// PANTS|03.15.01 - No longer requires you to be actively listing games.
////////////////////////////////////////////////////////////////////////////
void peerJoinStagingRoom
(
	PEER peer,  // The peer object.
	SBServer server,  // The server passed into peerlistingGamesCallback().
	const gsi_char password[PEER_PASSWORD_LEN],  // The password of the room being joined.  Can be NULL or "".
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
void peerJoinStagingRoomByChannel
(
	PEER peer,  // The peer object.
	const gsi_char * channel,  // The channel to join.
	const gsi_char password[PEER_PASSWORD_LEN],  // The password of the room being joined.  Can be NULL or "".
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
void peerCreateStagingRoom
(
	PEER peer,  // The peer object.
	const gsi_char * name,  // The name of the room.
	int maxPlayers,  // The max number of players allowed in the room.
	const gsi_char password[PEER_PASSWORD_LEN],  // An optional password for the staging room
	peerJoinRoomCallback callback,  // Called when finished.
	void * param,  // Passed to the callback.
	PEERBool blocking // If PEERTrue, don't return until finished.
);

// Same as peerCreateStagingRoom, but uses the provided socket for
// sending heartbeats and query replies.  This allows the game
// to share a socket with the peer SDK, which can make hosting
// games behind a NAT proxy possible.
//////////////////////////////////////////////////////////////////
void peerCreateStagingRoomWithSocket
(
	PEER peer,  // The peer object.
	const gsi_char * name,  // The name of the room.
	int maxPlayers,  // The max number of players allowed in the room.
	const gsi_char password[PEER_PASSWORD_LEN],  // An optional password for the staging room
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
void peerLeaveRoom
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room you want to leave (TitleRoom, GroupRoom, or StagingRoom).
	const gsi_char * reason  // The reason the player is leaving (can be NULL).  PANTS|03.13.01
);

// Gets called once for each group room when listing group rooms.
// After this has been called for each group room, it will be
// called one more time with groupID==0 and name==NULL.
/////////////////////////////////////////////////////////////////
typedef void (* peerListGroupRoomsCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // PEERTrue if success, PEERFalse if failure.
	int groupID,  // A unique ID for this group.
	SBServer server,  // The server object for this group room.
	const gsi_char * name,  // The group room's name.
	int numWaiting,  // The number of players in the room.
	int maxWaiting,  // The maximum number of players allowed in the room.
	int numGames,  // The number of games either staging or running in the group.
	int numPlaying,  // The total number of players in games in the group.
	void * param  // User-data.
);

// List all the groups rooms for the currently set title.
// The fields parameter allows you to request extra info
//   on each group room.
/////////////////////////////////////////////////////////
void peerListGroupRooms
(
	PEER peer,  // The peer object.
	const gsi_char * fields,  // A backslash delimited list of fields.
	peerListGroupRoomsCallback callback,  // Called for each group room.
	void * param,  // Passed to the callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Called with info on games being listed.
// Used to maintain a list of running games and staging rooms.
// The server object is a unique way of identifying each game.
// It can also be used with the calls in the "SBServer Object Functions" section
//   of sb_serverbrowsing.h to find out more info about the server.
// If staging==PEERTrue, the game hasn't started yet, it's still in the staging room
//   use peerJoinStagingRoom() to join the staging room, or if staging==peerfalse
//   use the server object to get the game's IP and port to join with.
// During the _intial_ listing of games, progress is the percentage (0-100) of the
//   games that have been added.  Once the initial listing is completed,
//   progress will always be 100.
// PANTS|09.11.00 - The "password" key will be set to 1 for games that are
// passworded.  This can be checked with ServerGetIntValue(server, "password", 0).
/////////////////////////////////////////////////////////////////////////////////////
typedef void (* peerListingGamesCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // PEERTrue if success, PEERFalse if failure.
	const gsi_char * name,  // The name of the game being listed.
	SBServer server,  // The server object for this game.
	PEERBool staging,  // If PEERTrue, this is a staging room and not a running game.
	int msg,  // The type of message this is.
		// PEER_CLEAR:
		//   Clear the list.  This has the same effect as if this was called
		//   with PEER_REMOVE for every server listed.
		// PEER_ADD:
		//   This is a new server. Add it to the list.
		// PEER_UPDATE:
		//   This server is already on the list, and its been updated.
		// PEER_REMOVE:
		//   Remove this server from the list.  The server object for this server
		//   should not be used again after this callback returns.
	int progress,  // The percent of servers that have been added.
	void * param  // User-data.
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
void peerStartListingGames
(
	PEER peer,  // The peer object.
	const unsigned char * fields,  // An array of registered QR2 keys to request from servers.
	int numFields,  // The number of keys in the fields array.
	const gsi_char * filter,  // A SQL-like rule filter.
	peerListingGamesCallback callback,  // Called when finished.
	void * param  // Passed to the callback.
);

// Stop games from being listed. This does NOT clear the games list.
// So all games listed are still considered valid.  They stay valid
// until either another call to peerStartListingGames or until the
// title is cleared (or a new one set).
////////////////////////////////////////////////////////////////////
void peerStopListingGames
(
	PEER peer  // The peer object.
);

// Queries the game server for the latest information.
// If fullUpdate is PEERTrue, all of the keys will be requested.
// If fullUpdate is PEERFalse, only the keys passed to
// peerStartListingGames as the fields parameter will be requested.
///////////////////////////////////////////////////////////////////
void peerUpdateGame
(
	PEER peer,  // The peer object.
	SBServer server,  // The server object for the game to update.
	PEERBool fullUpdate  // If true, get all of the game's keys.
);

// 08-26-2004 Added by Saad Nader
// Queries the game server for the latest information via the master 
// server.
// If fullUpdate is PEERTrue, all of the keys will be requested.
// If fullUpdate is PEERFalse, only the keys passed to
// peerStartListingGames as the fields parameter will be requested.
///////////////////////////////////////////////////////////////////
void peerUpdateGameByMaster
(
	PEER peer,  // The peer object.
	SBServer server,  // The server object for the game to update.
	PEERBool fullUpdate  // If true, get all of the game's keys.
);

// Send a ping to the server
// ICMP echo reply will be sent as response.  The firewall 
// or NAT between server and internet does not have to accept 
// ICMP echo requests.
///////////////////////////////////////////////////////////////////
void peerUpdateGamePing(PEER peer, SBServer server);

// Send a message to a room.
////////////////////////////
void peerMessageRoom
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to send the message to.
	const gsi_char * message,  // The message.
	MessageType messageType  // The type of message.
);

// Send a UTM to a room.
////////////////////////
void peerUTMRoom
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to send the UTM to.
	const gsi_char * command,  // The command.
	const gsi_char * parameters,  // The UTM's parameters.
	PEERBool authenticate  // If true, the server will authenticate this UTM (should normally be false).
);

// Set a password in a room you're hosting.
// The only roomType currently supported is StagingRoom.
// This will only work if the player is hosting the room.
// If password is NULL or "", the password will be cleared.
///////////////////////////////////////////////////////////
void peerSetPassword
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room in which to set the password.
	const gsi_char password[PEER_PASSWORD_LEN]  // The password to set.
);

// Set the name of a room you're hosting.
// The only roomType currently supported is StagingRoom.
// PANTS|09.11.00
////////////////////////////////////////////////////////
void peerSetRoomName
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room in which to set the name.
	const gsi_char * name  // The new name
);

// Get a room's name (the channel's title).
// Returns NULL if not in the room.
///////////////////////////////////////////
const gsi_char * peerGetRoomName
(
	PEER peer,  // The peer object.
	RoomType roomType  // The room to get the name for.
);

// Get's the chat channel associated with the room.
// Returns NULL if not in the room.
///////////////////////////////////////////////////
const gsi_char * peerGetRoomChannel
(
	PEER peer,  // The peer object.
	RoomType roomType  // The room to get the channel for.
);

// Returns PEERTrue if in the room.
///////////////////////////////////
PEERBool peerInRoom
(
	PEER peer,  // The peer object.
	RoomType roomType  // The room to check for.
);

// Use this channel for the title room for the currently set title.
// This function is normally not needed.
///////////////////////////////////////////////////////////////////
void peerSetTitleRoomChannel
(
	PEER peer,  // The peer object.
	const gsi_char * channel  // The channel to use for the title room.
);

// Use this to get the SBServer object for the staging room host.
// This will return NULL if the local player is not in a staging room,
// is the staging room host, or joined using peerJoinStagingRoomByChannel.
//////////////////////////////////////////////////////////////////////////
SBServer peerGetHostServer(PEER peer);

// Update the maximum number of players for a staging room
/////////////////////////////////////////////////////////
void peerSetStagingRoomMaxPlayers
(
	PEER peer,
	int maxPlayers
);

/************
** PLAYERS **
************/
// Called for each player in a room being enumerated, and once
//   when finished, with index==-1 and nick==NULL.
//////////////////////////////////////////////////////////////
typedef void (* peerEnumPlayersCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // PEERTrue if success, PEERFalse if failure.
	RoomType roomType,  // The room whose players are being enumerated.
	int index,  // The index of the player (0 to (N - 1)).  -1 when finished.
	const gsi_char * nick,  // The nick of the player.
	int flags,  // This player's flags (see #define's above).  PANTS|03.12.01
	void * param  // User-data.
);

// Enumerates through the players in a room.
////////////////////////////////////////////
void peerEnumPlayers
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to enum the players in.
	peerEnumPlayersCallback callback,  // Called when finished.
	void * param  // Passed to callback.
);

// Send a message to a player.
//////////////////////////////
void peerMessagePlayer
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The nick of the player to send the message to.
	const gsi_char * message,  // The message to send.
	MessageType messageType  // The type of message.
);

// Send a UTM to a player.
//////////////////////////
void peerUTMPlayer
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The nick of the player to send the UTM to.
	const gsi_char * command,  // The command.
	const gsi_char * parameters,  // The UTM's parameters.
	PEERBool authenticate  // If true, the server will authenticate this UTM (should normally be false).
);

// Kick a player from a room.
// Can only be called by a player with host privileges.
///////////////////////////////////////////////////////
void peerKickPlayer
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to kick the player from.
	const gsi_char * nick,  // The nick of the player to kick.
	const gsi_char * reason  // An optional reason for kicking the player
);

// Gets a player's ping (between the local machine and the player's machine).
// Returns PEERFalse if we don't have or can't get the player's ping.
/////////////////////////////////////////////////////////////////////////////
PEERBool peerGetPlayerPing
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The player to get the ping for.
	int * ping  // The player's ping is stored here, if we have it.
);

// Gets the cross-ping between 2 players.
// Returns PEERFalse if we don't have or can't get the player's cross-ping.
///////////////////////////////////////////////////////////////////////////
PEERBool peerGetPlayersCrossPing
(
	PEER peer,  // The peer object.
	const gsi_char * nick1,  // The first player.
	const gsi_char * nick2,  // The second player.
	int * crossPing  // The cross-ping is stored here, if we have it.
);

// Peer already automatically pings all players that are in ping rooms.
// This function does a one-time ping of a remote player in a non-ping room.
// However pings must be enabled in at least one room for this to work,
// otherwise Peer will not open the UDP ping socket.
// Also, peerAlwaysGetPlayerInfo() must be enabled so that Peer has IPs for
// players that are only in non-ping rooms.
////////////////////////////////////////////////////////////////////////////
PEERBool peerPingPlayer
(
	PEER peer,  // The peer object.
	const gsi_char * nick  // The player to ping.
);

// Gets a player's info immediately.
// Returns PEERFalse if the info is no available.
/////////////////////////////////////////////////
PEERBool peerGetPlayerInfoNoWait
(
	PEER peer,
	const gsi_char * nick,
	unsigned int * IP,
	int * profileID
);

// Called as a result of a call to peerGetPlayerInfo().
///////////////////////////////////////////////////////
typedef void (* peerGetPlayerInfoCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // PEERTrue if success, PEERFalse if failure.
	const gsi_char * nick,  // The player's nick.
	unsigned int IP,  // The player's IP, in network byte order.
	int profileID,  // The player's profile ID.
	void * param  // User-data.
);

// Called to get a player's IP and profile ID.
//////////////////////////////////////////////
void peerGetPlayerInfo
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The player's nick.
	peerGetPlayerInfoCallback callback,  // Called when finished.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Called as a result of a call to peerGetPlayerProfileID().
////////////////////////////////////////////////////////////
typedef void (* peerGetPlayerProfileIDCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // PEERTrue if success, PEERFalse if failure.
	const gsi_char * nick,  // The player's nick.
	int profileID,  // The player's profile ID.
	void * param  // User-data.
);

// Called to get a player's profile ID.
// DEPRECATED - Use peerGetPlayerInfo.
///////////////////////////////////////
void peerGetPlayerProfileID
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The player's nick.
	peerGetPlayerProfileIDCallback callback,  // Called when finished.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Called as a result of a call to peerGetPlayerIP().
/////////////////////////////////////////////////////
typedef void (* peerGetPlayerIPCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // PEERTrue if success, PEERFalse if failure.
	const gsi_char * nick,  // The player's nick.
	unsigned int IP,  // The player's IP, in network byte order.  PANTS|09.11.00 - was unsigned long
	void * param  // User-data.
);

// Called to get a player's IP.
// DEPRECATED - Use peerGetPlayerInfo.
//////////////////////////////////////
void peerGetPlayerIP
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The player's nick.
	peerGetPlayerIPCallback callback,  // Called when finished.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Checks if a player is a host (has ops).
// Returns PEERTrue if yes, PEERFalse if no.
////////////////////////////////////////////
PEERBool peerIsPlayerHost
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The player's nick.
	RoomType roomType  // The room to check in.
);

// Gets a player's flags in a room.  Returns PEERFalse if
// the local player or this player is not in the room.
/////////////////////////////////////////////////////////
PEERBool peerGetPlayerFlags
(
	PEER peer,
	const gsi_char * nick,
	RoomType roomType,
	int * flags
);

/*********
** GAME **
*********/
/**************** Disabled *************************/
// Returns the qr2 record used to report
// Primarily used for CD Key integration to prevent
// game intrusion and and post staging hacks
////////////////////////////////////////////////////
//qr2_t peerGetReportingRecord(PEER peer);

// Sets the local player's ready state.
// PEERTrue if ready, PEERFalse if not ready.
/////////////////////////////////////////////
void peerSetReady
(
	PEER peer,  // The peer object.
	PEERBool ready  // Ready or not.
);

// Gets a player's ready state.
///////////////////////////////
PEERBool peerGetReady
(
	PEER peer,  // The peer object.
	const gsi_char * nick, // The player's nick.
	PEERBool * ready  // The player's ready state gets stored in here.
);

// Checks if all the player's in the staging room are ready.
////////////////////////////////////////////////////////////
PEERBool peerAreAllReady
(
	PEER peer  // The peer object.
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
void peerStartGame
(
	PEER peer,  // The peer object.
	const gsi_char * message,  // A message to send to everyone.
	int reportingOptions  // Bitfield flags used to set reporting options.
);

// Starts server reporting, without creating a staging room.
// Call peerStopGame() to stop reporting.
////////////////////////////////////////////////////////////////
PEERBool peerStartReporting
(
	PEER peer  // The peer object.
);

// Same as peerStartReporting, but uses the provided socket for
// sending heartbeats and query replies.  This allows the game
// to share a socket with the peer SDK, , which can make hosting
// games behind a NAT proxy possible.
////////////////////////////////////////////////////////////////
PEERBool peerStartReportingWithSocket
(
	PEER peer,  // The peer object.
	SOCKET socket,  // The socket to be used for reporting.
	unsigned short port  // The local port to which the socket is bound.
);

// Mark the local player as playing.
// Use this if the player is in a game not
// started by peer, but he should be marked as playing.
///////////////////////////////////////////////////////
void peerStartPlaying
(
	PEER peer  // The peer object.
);

// Check to see if the local player is playing.
///////////////////////////////////////////////
PEERBool peerIsPlaying
(
	PEER peer  // The peer object.
);

// Needs to be called by the host when the game has stopped.
////////////////////////////////////////////////////////////
void peerStopGame
(
	PEER peer  // The peer object.
);

// Call this when hosting a staging room or a game to force peer
// to report the game again.  An example of when to call this is
// when a player joins or leaves a game.
// PANTS|09.11.00
///////////////////////////////////////////////////////////////////
void peerStateChanged
(
	PEER peer  // The peer object.
);

/*********
** KEYS **
*********/
// Set global keys on the local player.
///////////////////////////////////////
void peerSetGlobalKeys
(
	PEER peer,  // The peer object.
	int num,  // The number of keys to set.
	const gsi_char ** keys,  // The keys to set.
	const gsi_char ** values  // The values for the keys.
);

// Called with a player's global keys.
//////////////////////////////////////
typedef void (* peerGetGlobalKeysCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // If PEERFalse, unable to get the keys.
	const gsi_char * nick,  // The player the keys are for.
	int num,  // The number of keys.
	const gsi_char ** keys,  // The keys got.
	const gsi_char ** values,  // The values for the keys.
	void * param  // User-data.
);

// Get a player's global keys.
//////////////////////////////
void peerGetPlayerGlobalKeys
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The player to get the keys for.
	int num,  // The number of keys.
	const gsi_char ** keys,  // The keys to get.
	peerGetGlobalKeysCallback callback,  // Called with the keys.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Get the global keys for all players in a room we're in.
//////////////////////////////////////////////////////////
void peerGetRoomGlobalKeys
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to get the keys in.
	int num,  // The number of keys.
	const gsi_char ** keys,  // The keys to get.
	peerGetGlobalKeysCallback callback,  // Called with the keys.
	void * param,  // Passed to callback.
	PEERBool blocking  // If PEERTrue, don't return until finished.
);

// Set the room keys for a player.
// Use NULL or "" to set keys on the room itself.
/////////////////////////////////////////////////
void peerSetRoomKeys
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to set the keys in.
	const gsi_char * nick,  // The player to set the keys on (NULL or "" for the room).
	int num,  // The number of keys.
	const gsi_char ** keys,  // The keys to set.
	const gsi_char ** values  // The values to set.
);

// Called with a player's room keys.
////////////////////////////////////
typedef void (* peerGetRoomKeysCallback)
(
	PEER peer,  // The peer object.
	PEERBool success,  // If PEERFalse, unable to get the keys.
	RoomType roomType,  // The room the keys are in.
	const gsi_char * nick,  // The player the keys are for, or NULL for the room.
	int num,  // The number of keys.
	gsi_char ** keys,  // The keys.
	gsi_char ** values,  // The values for the keys.
	void * param  // User-data.
);

// Get the room keys for either a single player of an entire room.
// Use "*" for the player to get the keys for the entire room.
// Use NULL or "" for the player to get keys on the room itself.
//////////////////////////////////////////////////////////////////
void peerGetRoomKeys
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to get the keys in.
	const gsi_char * nick,  // The player to get the keys for.
	int num,  // The number of keys.
	const gsi_char ** keys,  // The keys to get.
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
void peerSetGlobalWatchKeys
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room to set the watch keys for.
	int num,  // The number of keys.
	const gsi_char ** keys,  // The keys to watch for.
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
void peerSetRoomWatchKeys
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The type of room to set the watch keys for.
	int num,  // The number of keys.
	const gsi_char ** keys,  // The keys to watch for.
	PEERBool addKeys  // If PEERTrue, add these keys to the existing room watch keys for this room.
);

// Get the global watch key for a particular player.
// If the key isn't cached locally (either because it isn't
// a watch key or just isn't yet known), NULL will be returned.
// If the key is empty, "" will be returned.
///////////////////////////////////////////////////////////////
const gsi_char * peerGetGlobalWatchKey
(
	PEER peer,  // The peer object.
	const gsi_char * nick,  // The player to get the key for.
	const gsi_char * key  // The key to get.
);

// Get the room watch key for a particular player in a room.
// If the key isn't cached locally (either because it isn't
// a watch key or just isn't yet known), NULL will be returned.
// If the key is empty, "" will be returned.
///////////////////////////////////////////////////////////////
const gsi_char * peerGetRoomWatchKey
(
	PEER peer,  // The peer object.
	RoomType roomType,  // The room to get the key in.
	const gsi_char * nick,  // The player to get the key for.
	const gsi_char * key  // The key to get.
);

/**************
** AUTOMATCH **
**************/

// Called during a automatch attempt to inform the app of the status.
/////////////////////////////////////////////////////////////////////
typedef void (* peerAutoMatchStatusCallback)
(
	PEER peer,  // The peer object.
	PEERAutoMatchStatus status,  // The current status.
	void * param  // User-data.
);

// This callback may be called during the automatch attempt.
// Peer uses the callback to ask the application how well a particular
// match fits what the user is looking for.  The application checks the match
// and then returns a rating for it.  If the rating is <=0 the match is
// considered unsuitable and Peer will not attempt that match.  If the rating
// is >0 this match is considered suitable.  The higher the rating, up to a
// maximum of INT_MAX, the more suitable the match.  The exact scheme used
// to generate a rating is entirely up to the application.  The match ratings
// are used by Peer both to check if a particular match meets the user's wishes
// and to compare matches to each other.  Peer will first try to match with the
// highest rated possible match, then the second highest, etc.
///////////////////////////////////////////////////////////////////////////////
typedef int (* peerAutoMatchRateCallback)
(
	PEER peer,  // The peer object.
	SBServer match,  // Possible match to rate.
	void * param  // User-data.
);

// Used to start a automatch attempt.
// The filter contains any hard criteria.  This is used to narrow down the list
// of possible matches to those the user might consider.
// The status callback will be called as the attempt progresses, and the rate
// callback will be used to sort possible matches.
/////////////////////////////////////////////////////////////////////////////////
void peerStartAutoMatch
(
	PEER peer,  // The peer object.
	int maxPlayers,  // The total number of players to match (including the local player).
	const gsi_char * filter,  // Hard criteria - filters out servers.
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
void peerStartAutoMatchWithSocket
(
	PEER peer,  // The peer object.
	int maxPlayers,  // The total number of players to match (including the local player).
	const gsi_char * filter,  // Hard criteria - filters out servers.
	SOCKET socket,  // The socket to be used for reporting.
	unsigned short port,  // The local port to which the socket is bound.
	peerAutoMatchStatusCallback statusCallback,  // Called as the attempt status changes.
	peerAutoMatchRateCallback rateCallback,  // Used to rate possible matches.
	void * param,  // User-data.
	PEERBool blocking   // If PEERTrue, don't return until finished.
);

// Stops an automatch attempt in progress.
// The user will not automatically be removed from any staging room he is in.
/////////////////////////////////////////////////////////////////////////////
void peerStopAutoMatch(PEER peer);

// Checks if a automatch attempt is in progress.
////////////////////////////////////////////////
PEERBool peerIsAutoMatching(PEER peer);

// Gets the current status of the automatch attempt.
// If no attempt is in progress, PEERFailed will be returned.
/////////////////////////////////////////////////////////////
PEERAutoMatchStatus peerGetAutoMatchStatus(PEER peer);

#ifdef __cplusplus
}
#endif

#endif
