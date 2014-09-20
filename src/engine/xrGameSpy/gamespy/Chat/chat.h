/*
GameSpy Chat SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _CHAT_H_
#define _CHAT_H_

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../common/gsCommon.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

/************
** DEFINES **
************/
// User and channel message types.
//////////////////////////////////
#define CHAT_MESSAGE        0
#define CHAT_ACTION         1
#define CHAT_NOTICE         2
#define CHAT_UTM            3
#define CHAT_ATM            4

// User modes.
// PANTS|03.12.01 - These are now bitflags!
// Both CHAT_VOICE and CHAT_OP can be set at the same time.
///////////////////////////////////////////////////////////
#define CHAT_NORMAL         0
#define CHAT_VOICE          1
#define CHAT_OP             2

// Part reasons (see the chatUserParted callback).
//////////////////////////////////////////////////
#define CHAT_LEFT           0  // The user left the channel.
#define CHAT_QUIT           1  // The user quit the chat network.
#define CHAT_KICKED         2  // The user was kicked from the channel.
#define CHAT_KILLED         3  // The user was kicked off the chat network.

// Possible nick errors while connecting.
/////////////////////////////////////////
#define CHAT_NICK_OK                0
#define CHAT_IN_USE                 1
#define CHAT_INVALID                2
#define CHAT_UNIQUENICK_EXPIRED     3
#define CHAT_NO_UNIQUENICK          4
#define CHAT_INVALID_UNIQUENICK     5
#define CHAT_NICK_TOO_LONG          6

// Reasons why a connect attempt could fail.
////////////////////////////////////////////
#define CHAT_DISCONNECTED           0
#define CHAT_NICK_ERROR             1
#define CHAT_LOGIN_FAILED           2

/**********
** TYPES **
**********/
// Boolean type.
////////////////
typedef enum { CHATFalse, CHATTrue } CHATBool;

// A CHAT object represents a client connection to a chat server.
/////////////////////////////////////////////////////////////////
typedef void * CHAT;

// Object representing a channel's mode.
////////////////////////////////////////
typedef struct CHATChannelMode
{
	CHATBool InviteOnly;
	CHATBool Private;
	CHATBool Secret;
	CHATBool Moderated;
	CHATBool NoExternalMessages;
	CHATBool OnlyOpsChangeTopic;
	CHATBool OpsObeyChannelLimit;
	int Limit;
} CHATChannelMode;

// The result of a channel enter attempt,
// passed into the chatEnterChannelCallback().
//////////////////////////////////////////////
typedef enum
{
	CHATEnterSuccess,        // The channel was successfully entered.
	CHATBadChannelName,      // The channel name was invalid.
	CHATChannelIsFull,       // The channel is at its user limit.
	CHATInviteOnlyChannel,   // The channel is invite only.
	CHATBannedFromChannel,   // The local user is banned from this channel.
	CHATBadChannelPassword,  // The channel has a password, and a bad password (or none) was given.
	CHATTooManyChannels,     // The server won't allow this user in any more channels.
	CHATEnterTimedOut,       // The attempt to enter timed out.
	CHATBadChannelMask       // Not sure if any servers use this, or what it means! (ERR_BADCHANMASK)
} CHATEnterResult;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef GSI_UNICODE
#define chatConnect					chatConnectA
#define chatConnectSpecial			chatConnectSpecialA
#define chatConnectSecure			chatConnectSecureA
#define chatConnectLogin			chatConnectLoginA
#define chatConnectPreAuth			chatConnectPreAuthA
#define chatRetryWithNick			chatRetryWithNickA
#define chatRegisterUniqueNick		chatRegisterUniqueNickA
#define chatSendRaw					chatSendRawA
#define chatChangeNick				chatChangeNickA
#define chatFixNick					chatFixNickA
#define chatTranslateNick			chatTranslateNickA
#define chatAuthenticateCDKey		chatAuthenticateCDKeyA
#define chatEnumChannels			chatEnumChannelsA
#define chatEnterChannel			chatEnterChannelA
#define chatLeaveChannel			chatLeaveChannelA
#define chatSendChannelMessage		chatSendChannelMessageA
#define chatSetChannelTopic			chatSetChannelTopicA
#define chatGetChannelTopic			chatGetChannelTopicA
#define chatSetChannelMode			chatSetChannelModeA
#define chatGetChannelMode			chatGetChannelModeA
#define chatSetChannelPassword		chatSetChannelPasswordA	
#define chatGetChannelPassword		chatGetChannelPasswordA		
#define chatSetChannelLimit			chatSetChannelLimitA	
#define chatEnumChannelBans			chatEnumChannelBansA
#define chatAddChannelBan			chatAddChannelBanA
#define chatRemoveChannelBan		chatRemoveChannelBanA
#define chatSetChannelGroup			chatSetChannelGroupA
#define chatGetChannelNumUsers		chatGetChannelNumUsersA
#define chatInChannel				chatInChannelA
#define chatEnumUsers				chatEnumUsersA
#define chatSendUserMessage			chatSendUserMessageA
#define chatGetUserInfo				chatGetUserInfoA
#define chatGetBasicUserInfo		chatGetBasicUserInfoA
#define chatGetBasicUserInfoNoWait	chatGetBasicUserInfoNoWaitA
#define chatGetChannelBasicUserInfo	chatGetChannelBasicUserInfoA
#define chatInviteUser				chatInviteUserA
#define chatKickUser				chatKickUserA
#define chatBanUser					chatBanUserA
#define chatSetUserMode				chatSetUserModeA
#define chatGetUserMode				chatGetUserModeA
#define chatGetUserModeNoWait		chatGetUserModeNoWaitA
#define chatSetGlobalKeys			chatSetGlobalKeysA
#define chatSetChannelKeys			chatSetChannelKeysA
#define chatGetGlobalKeys			chatGetGlobalKeysA
#define chatGetChannelKeys			chatGetChannelKeysA
#define chatGetNick					chatGetNickA
#define chatGetUdpRelay				chatGetUdpRelayA
#else
#define chatConnect					chatConnectW
#define chatConnectSpecial			chatConnectSpecialW
#define chatConnectSecure			chatConnectSecureW
#define chatConnectLogin			chatConnectLoginW
#define chatConnectPreAuth			chatConnectPreAuthW
#define chatRetryWithNick			chatRetryWithNickW
#define chatRegisterUniqueNick		chatRegisterUniqueNickW
#define chatSendRaw					chatSendRawW
#define chatChangeNick				chatChangeNickW
#define chatFixNick					chatFixNickW
#define chatTranslateNick			chatTranslateNickW
#define chatAuthenticateCDKey		chatAuthenticateCDKeyW
#define chatEnumChannels			chatEnumChannelsW
#define chatEnterChannel			chatEnterChannelW
#define chatLeaveChannel			chatLeaveChannelW
#define chatSendChannelMessage		chatSendChannelMessageW
#define chatSetChannelTopic			chatSetChannelTopicW
#define chatGetChannelTopic			chatGetChannelTopicW
#define chatSetChannelMode			chatSetChannelModeW
#define chatGetChannelMode			chatGetChannelModeW
#define chatSetChannelPassword		chatSetChannelPasswordW
#define chatGetChannelPassword		chatGetChannelPasswordW
#define chatSetChannelLimit			chatSetChannelLimitW
#define chatEnumChannelBans			chatEnumChannelBansW
#define chatAddChannelBan			chatAddChannelBanW
#define chatRemoveChannelBan		chatRemoveChannelBanW
#define chatSetChannelGroup			chatSetChannelGroupW
#define chatGetChannelNumUsers		chatGetChannelNumUsersW
#define chatInChannel				chatInChannelW
#define chatEnumUsers				chatEnumUsersW
#define chatSendUserMessage			chatSendUserMessageW
#define chatGetUserInfo				chatGetUserInfoW
#define chatGetBasicUserInfo		chatGetBasicUserInfoW
#define chatGetBasicUserInfoNoWait	chatGetBasicUserInfoNoWaitW
#define chatGetChannelBasicUserInfo	chatGetChannelBasicUserInfoW
#define chatInviteUser				chatInviteUserW
#define chatKickUser				chatKickUserW
#define chatBanUser					chatBanUserW
#define chatSetUserMode				chatSetUserModeW
#define chatGetUserMode				chatGetUserModeW
#define chatGetUserModeNoWait		chatGetUserModeNoWaitW
#define chatSetGlobalKeys			chatSetGlobalKeysW
#define chatSetChannelKeys			chatSetChannelKeysW
#define chatGetGlobalKeys			chatGetGlobalKeysW
#define chatGetChannelKeys			chatGetChannelKeysW
#define chatGetNick					chatGetNickW
#define chatGetUdpRelay				chatGetUdpRelayW
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/**********************
** GLOBALS CALLBACKS **
**********************/
// Gets raw incoming network traffic.
/////////////////////////////////////
typedef void (* chatRaw)(CHAT chat,
						 const gsi_char * raw,
						 void * param);

// Called when the client has been disconnected.
////////////////////////////////////////////////
typedef void (* chatDisconnected)(CHAT chat,
								  const gsi_char * reason,
								  void * param);

// Called when a private message from another user is received.
// If user==NULL, this is a message from the server.
///////////////////////////////////////////////////////////////
typedef void (* chatPrivateMessage)(CHAT chat,
									const gsi_char * user,
									const gsi_char * message,
									int type,  // See defined message types above.
									void * param);

// Called when invited into a channel.
//////////////////////////////////////
typedef void (* chatInvited)(CHAT chat,
							 const gsi_char * channel,
							 const gsi_char * user,
							 void * param);

// A connection's global callbacks.
///////////////////////////////////
typedef struct chatGlobalCallbacks
{
	chatRaw raw;
	chatDisconnected disconnected;
	chatPrivateMessage privateMessage;
	chatInvited invited;
	void * param;
} chatGlobalCallbacks;

/**********************
** CHANNEL CALLBACKS **
**********************/
// Called when a message is received in a channel.
//////////////////////////////////////////////////
typedef void (* chatChannelMessage)(CHAT chat,
									const gsi_char * channel,
									const gsi_char * user,
									const gsi_char * message,
									int type,  // See defined message types above.
									void * param);

// Called when the local client is kicked from a channel.
/////////////////////////////////////////////////////////
typedef void (* chatKicked)(CHAT chat,
							const gsi_char * channel,
							const gsi_char * user,
							const gsi_char * reason,
							void * param);

// Called when a user joins a channel we're in.
///////////////////////////////////////////////
typedef void (* chatUserJoined)(CHAT chat,
								const gsi_char * channel,
								const gsi_char * user,
								int mode,    // See defined user modes above.
								void * param);

// Called when a user parts a channel we're in.
///////////////////////////////////////////////
typedef void (* chatUserParted)(CHAT chat,
								const gsi_char * channel,
								const gsi_char * user,
								int why,    // See defined part reasons above.
								const gsi_char * reason,
								const gsi_char * kicker,
								void * param);

// Called when a user in a channel we're in changes nicks.
//////////////////////////////////////////////////////////
typedef void (* chatUserChangedNick)(CHAT chat,
									 const gsi_char * channel,
									 const gsi_char * oldNick,
									 const gsi_char * newNick,
									 void * param);

// Called when the topic changes in a channel we're in.
///////////////////////////////////////////////////////
typedef void (* chatTopicChanged)(CHAT chat,
								  const gsi_char * channel,
								  const gsi_char * topic,
								  void * param);

// Called when the mode changes in a channel we're in.
//////////////////////////////////////////////////////
typedef void (* chatChannelModeChanged)(CHAT chat,
										const gsi_char * channel,
										CHATChannelMode * mode,
										void * param);

// Called when a user's mode changes in a channel we're in.
///////////////////////////////////////////////////////////
typedef void (* chatUserModeChanged)(CHAT chat,
									 const gsi_char * channel,
									 const gsi_char * user,
									 int mode,  // See defined user modes above.
									 void * param);

// Called when the user list changes (due to a join or a part) in a channel we're in.
/////////////////////////////////////////////////////////////////////////////////////
typedef void (* chatUserListUpdated)(CHAT chat,
									 const gsi_char * channel,
									 void * param);

// Called when the chat server sends an entire new user list for a channel we're in.
////////////////////////////////////////////////////////////////////////////////////
typedef void (* chatNewUserList)(CHAT chat,
								 const gsi_char * channel,
								 int num,
								 const gsi_char ** users,
								 int * modes,
								 void * param);

// Called when a user changes a broadcast key in a channel we're in.
////////////////////////////////////////////////////////////////////
typedef void (* chatBroadcastKeyChanged)(CHAT chat,
										 const gsi_char * channel,
										 const gsi_char * user,
										 const gsi_char * key,
										 const gsi_char * value,
										 void * param);

// A channel's callbacks.
/////////////////////////
typedef struct chatChannelCallbacks
{
	chatChannelMessage channelMessage;
	chatKicked kicked;
	chatUserJoined userJoined;
	chatUserParted userParted;
	chatUserChangedNick userChangedNick;
	chatTopicChanged topicChanged;
	chatChannelModeChanged channelModeChanged;
	chatUserModeChanged userModeChanged;
	chatUserListUpdated userListUpdated;
	chatNewUserList newUserList;
	chatBroadcastKeyChanged broadcastKeyChanged;
	void * param;
} chatChannelCallbacks;

/************
** GENERAL **
************/
// Called when a connect attempt completes.
// failureReason is only set if success is CHATFalse.
/////////////////////////////////////////////////////
typedef void (* chatConnectCallback)(CHAT chat,
									 CHATBool success,
									 int failureReason,  // CHAT_DISCONNECTED, CHAT_NICK_ERROR, etc.
									 void * param);
// Called if there is an error with the nick while connecting.
// To retry with a new nick, call chatRetryWithNick.
// Otherwise, call chatDisconnect to stop the connection.
// Suggested nicks are only provided if type is CHAT_INVALID_UNIQUENICK.
////////////////////////////////////////////////////////////////////////
typedef void (* chatNickErrorCallback)(CHAT chat,
									   int type,  // CHAT_IN_USE, CHAT_INVALID, etc.
									   const gsi_char * nick,
									   int numSuggestedNicks,
									   const gsi_char ** suggestedNicks,
									   void * param);
typedef void (* chatFillInUserCallback)(CHAT chat,
										unsigned int IP, // PANTS|08.21.00 - changed from unsigned long
										gsi_char user[128],
										void * param);
// Connects you to a chat server and returns a CHAT object.
///////////////////////////////////////////////////////////
CHAT chatConnect(const gsi_char * serverAddress,
				 int port,
                 const gsi_char * nick,
				 const gsi_char * user,
				 const gsi_char * name,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking);
CHAT chatConnectSpecial(const gsi_char * serverAddress,
				 int port,
                 const gsi_char * nick,
				 const gsi_char * name,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking);
CHAT chatConnectSecure(const gsi_char * serverAddress,
				 int port,
                 const gsi_char * nick,
				 const gsi_char * name,
				 const gsi_char * gamename,
				 const gsi_char * secretKey,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking);
CHAT chatConnectLogin(const gsi_char * serverAddress,
				 int port,
				 int namespaceID,
				 const gsi_char * email,
				 const gsi_char * profilenick,
				 const gsi_char * uniquenick,
				 const gsi_char * password,
				 const gsi_char * name,
				 const gsi_char * gamename,
				 const gsi_char * secretKey,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking);
CHAT chatConnectPreAuth(const gsi_char * serverAddress,
				 int port,
				 const gsi_char * authtoken,
				 const gsi_char * partnerchallenge,
				 const gsi_char * name,
				 const gsi_char * gamename,
				 const gsi_char * secretKey,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking);
// If the chatNickErrorCallback gets called, then this can be called
// with a new nick to retry.  If this isn't called, the connection can be
// disconnected with chatDisconnect.  If the new nick is successful, then
// the chatConnectCallback will get called.  If there's another nick 
// error, the chatNickErrorCallback will get called again.
/////////////////////////////////////////////////////////////////////////
void chatRetryWithNick(CHAT chat,
					   const gsi_char * nick);
// Register a uniquenick.
// Should be called in response to the chatNickErrorCallback being called
// with a type of CHAT_UNIQUENICK_EXPIRED or CHAT_NO_UNIQUENICK.
// If the uniquenick cannot be registered, the chatNickErrorCallback will
// be called again with a type of CHAT_IN_USE or CHAT_INVALID.
/////////////////////////////////////////////////////////////////////////
void chatRegisterUniqueNick(CHAT chat,
							int namespaceID,
							const gsi_char * uniquenick,
							const gsi_char * cdkey);

// Disconnect the chat connection.
//////////////////////////////////
void chatDisconnect(CHAT chat);

// Processes the chat connection.
/////////////////////////////////
void chatThink(CHAT chat);

// Sends raw data, without any interpretation.
//////////////////////////////////////////////
void chatSendRaw(CHAT chat,
				 const gsi_char * command);

// Called as a result of a nick change attempt.
///////////////////////////////////////////////
typedef void (* chatChangeNickCallback)(CHAT chat,
										CHATBool success,
										const gsi_char * oldNick,
										const gsi_char * newNick,
										void * param);
// Change the local user's nick.
////////////////////////////////
void chatChangeNick(CHAT chat,
					const gsi_char * newNick,
					chatChangeNickCallback callback,
					void * param,
					CHATBool blocking);

// Get our local nickname.
//////////////////////////
const gsi_char * chatGetNick(CHAT chat);

// Copies the oldNick to the newNick, replacing any invalid characters with legal ones.
///////////////////////////////////////////////////////////////////////////////////////
void chatFixNick(gsi_char * newNick,
				 const gsi_char * oldNick);

// Removes the namespace extension from a chat nick.
////////////////////////////////////////////////////
const gsi_char * chatTranslateNick(gsi_char * nick,
								   const gsi_char * extension);

// Gets the local userID.
// Only valid if connected with chatConnectLogin or chatConnectPreAuth.
///////////////////////////////////////////////////////////////////////
int chatGetUserID(CHAT chat);

// Gets the local profileID.
// Only valid if connected with chatConnectLogin or chatConnectPreAuth.
///////////////////////////////////////////////////////////////////////
int chatGetProfileID(CHAT chat);

// Turn on/off quiet mode.
//////////////////////////
void chatSetQuietMode(CHAT chat,
					  CHATBool quiet);

// Called as a result of an authenticate CD key attempt.
////////////////////////////////////////////////////////
typedef void (* chatAuthenticateCDKeyCallback)(CHAT chat,
											   int result,
											   const gsi_char * message,
											   void * param);

// Attempts to authenticates a CD key.
//////////////////////////////////////
void chatAuthenticateCDKey(CHAT chat,
						   const gsi_char * cdkey,
						   chatAuthenticateCDKeyCallback callback,
						   void * param,
						   CHATBool blocking);

/*************
** CHANNELS **
*************/
// Gets called for each channel enumerated.
///////////////////////////////////////////
typedef void (* chatEnumChannelsCallbackEach)(CHAT chat,
											  CHATBool success,
											  int index,
											  const gsi_char * channel,
											  const gsi_char * topic,
											  int numUsers,
											  void * param);
// Gets called after all channels have been enumerated.
///////////////////////////////////////////////////////
typedef void (* chatEnumChannelsCallbackAll)(CHAT chat,
											 CHATBool success,
											 int numChannels,
											 const gsi_char ** channels,
											 const gsi_char ** topics,
											 int * numUsers,
											 void * param);
// Enumerates the channels available on a chat server.
//////////////////////////////////////////////////////
void chatEnumChannels(CHAT chat,
					  const gsi_char * filter,
					  chatEnumChannelsCallbackEach callbackEach,
					  chatEnumChannelsCallbackAll callbackAll,
					  void * param,
					  CHATBool blocking);

// Gets called for each channel enumerated.
///////////////////////////////////////////
typedef void (* chatEnumJoinedChannelsCallback)(CHAT chat,
											  int index,
											  const gsi_char * channel,
											  void * param);

// Enumerates the channels that we are joined to
//////////////////////////////////////////////////////
void chatEnumJoinedChannels(CHAT chat,
					  chatEnumJoinedChannelsCallback callback,
					  void * param);



// Gets called when a channel has been entered.
///////////////////////////////////////////////
typedef void (* chatEnterChannelCallback)(CHAT chat,
										  CHATBool success,
										  CHATEnterResult result,
										  const gsi_char * channel,
										  void * param);
// Enters a channel.
////////////////////
void chatEnterChannel(CHAT chat,
					  const gsi_char * channel,
					  const gsi_char * password,
					  chatChannelCallbacks * callbacks,
					  chatEnterChannelCallback callback,
					  void * param,
					  CHATBool blocking);

// Leaves a channel.
////////////////////
void chatLeaveChannel(CHAT chat,
					  const gsi_char * channel,
					  const gsi_char * reason);  // PANTS|03.13.01

// Sends a message to a channel.
////////////////////////////////
void chatSendChannelMessage(CHAT chat,
							const gsi_char * channel,
							const gsi_char * message,
							int type);

// Sets the topic in a channel.
///////////////////////////////
void chatSetChannelTopic(CHAT chat,
						 const gsi_char * channel,
						 const gsi_char * topic);

// Gets called when a channel's topic has been retrieved.
/////////////////////////////////////////////////////////
typedef void (* chatGetChannelTopicCallback)(CHAT chat,
											 CHATBool success,
											 const gsi_char * channel,
											 const gsi_char * topic,
											 void * param);
// Gets a channel's topic.
//////////////////////////
void chatGetChannelTopic(CHAT chat,
						 const gsi_char * channel,
						 chatGetChannelTopicCallback callback,
						 void * param,
						 CHATBool blocking);

// Sets a channel's mode.
/////////////////////////
void chatSetChannelMode(CHAT chat,
						const gsi_char * channel,
						CHATChannelMode * mode);

// Gets called when a channel's mode has been retrieved.
////////////////////////////////////////////////////////
typedef void (* chatGetChannelModeCallback)(CHAT chat,
											CHATBool success,
											const gsi_char * channel,
											CHATChannelMode * mode,
											void * param);
// Gets a channel's mode.
/////////////////////////
void chatGetChannelMode(CHAT chat,
						const gsi_char * channel,
						chatGetChannelModeCallback callback,
						void * param,
						CHATBool blocking);

// Sets the password in a channel.
//////////////////////////////////
void chatSetChannelPassword(CHAT chat,
							const gsi_char * channel,
							CHATBool enable,
							const gsi_char * password);

// Called when the channel's password has been retrieved.
/////////////////////////////////////////////////////////
typedef void (* chatGetChannelPasswordCallback)(CHAT chat,
												CHATBool success,
												const gsi_char * channel,
												CHATBool enabled,
												const gsi_char * password,
												void * param);
// Gets the password in a channel.
//////////////////////////////////
void chatGetChannelPassword(CHAT chat,
							const gsi_char * channel,
							chatGetChannelPasswordCallback callback,
							void * param,
							CHATBool blocking);

// Set the maximum number of users allowed in a channel.
////////////////////////////////////////////////////////
void chatSetChannelLimit(CHAT chat,
						 const gsi_char * channel,
						 int limit);

// Called with the list of bans in a channel.
/////////////////////////////////////////////
typedef void (* chatEnumChannelBansCallback)(CHAT chat,
											 CHATBool success,
											 const gsi_char * channel,
											 int numBans,
											 const gsi_char ** bans,
											 void * param);
// Enumerate through the bans in a channel.
///////////////////////////////////////////
void chatEnumChannelBans(CHAT chat,
						 const gsi_char * channel,
						 chatEnumChannelBansCallback callback,
						 void * param,
						 CHATBool blocking);

// Adds a channel ban.
//////////////////////
void chatAddChannelBan(CHAT chat,
					   const gsi_char * channel,
					   const gsi_char * ban);

// Removes a ban string from a channel.
///////////////////////////////////////
void chatRemoveChannelBan(CHAT chat,
						  const gsi_char * channel,
						  const gsi_char * ban);

// Set the group this channel is a part of.
///////////////////////////////////////////
void chatSetChannelGroup(CHAT chat,
						 const gsi_char * channel,
						 const gsi_char * group);

// Get the number of users in the channel.
// Returns -1 if we are not in the channel.
///////////////////////////////////////////
int chatGetChannelNumUsers(CHAT chat,
						   const gsi_char * channel);


// Returns CHATTrue if we are in the channel
///////////////////////////////////////////
CHATBool chatInChannel(CHAT chat,
						   const gsi_char * channel);


/**********
** USERS **
**********/
// Called with the list of users in a channel.
//////////////////////////////////////////////
typedef void (* chatEnumUsersCallback)(CHAT chat,
									   CHATBool success,
									   const gsi_char * channel, //PANTS|02.11.00|added paramater
									   int numUsers,
									   const gsi_char ** users,
									   int * modes,
									   void * param);
// Enumerate through the users in a channel.
////////////////////////////////////////////
void chatEnumUsers(CHAT chat,
				   const gsi_char * channel,
				   chatEnumUsersCallback callback,
				   void * param,
				   CHATBool blocking);

// Send a private message to a user.
////////////////////////////////////
void chatSendUserMessage(CHAT chat,
						 const gsi_char * user,
						 const gsi_char * message,
						 int type);

// Called with a user's info.
/////////////////////////////
typedef void (* chatGetUserInfoCallback)(CHAT chat,
										 CHATBool success,
										 const gsi_char * nick,  //PANTS|02.14.2000|added nick and user
										 const gsi_char * user,
										 const gsi_char * name,
										 const gsi_char * address,
										 int numChannels,
										 const gsi_char ** channels,
										 void * param);
// Get a user's info.
/////////////////////
void chatGetUserInfo(CHAT chat,
					 const gsi_char * user,
					 chatGetUserInfoCallback callback,
					 void * param,
					 CHATBool blocking);

// Called with a user's basic info.
///////////////////////////////////
typedef void (* chatGetBasicUserInfoCallback)(CHAT chat,
											  CHATBool success,
											  const gsi_char * nick,
											  const gsi_char * user,
											  const gsi_char * address,
											  void * param);

// Get some basic info on the user.
// PANTS|12.08.2000
///////////////////////////////////
void chatGetBasicUserInfo(CHAT chat,
						  const gsi_char * user,
						  chatGetBasicUserInfoCallback callback,
						  void * param,
						  CHATBool blocking);

// Get basic info without waiting.
// Returns CHATFalse if the info isn't available.
/////////////////////////////////////////////////
CHATBool chatGetBasicUserInfoNoWait(CHAT chat,
									const gsi_char * nick,
									const gsi_char ** user,
									const gsi_char ** address);

// Called with a user's basic info for everyone in a channel.
// Called with a NULL nick/user/address at the end.
/////////////////////////////////////////////////////////////
typedef void (* chatGetChannelBasicUserInfoCallback)(CHAT chat,
													 CHATBool success,
													 const gsi_char * channel,
													 const gsi_char * nick,
													 const gsi_char * user,
													 const gsi_char * address,
													 void * param);

// Get basic info on all the users in a channel.
// PANTS|12.19.00
////////////////////////////////////////////////
void chatGetChannelBasicUserInfo(CHAT chat,
								 const gsi_char * channel,
								 chatGetChannelBasicUserInfoCallback callback,
								 void * param,
								 CHATBool blocking);

// Invite a user into a channel.
////////////////////////////////
void chatInviteUser(CHAT chat,
					const gsi_char * channel,
					const gsi_char * user);

// Kick a user from a channel.
//////////////////////////////
void chatKickUser(CHAT chat,
				  const gsi_char * channel,
				  const gsi_char * user,
				  const gsi_char * reason);

// Ban a user from a channel.
/////////////////////////////
void chatBanUser(CHAT chat,
				 const gsi_char * channel,
				 const gsi_char * user);

// Sets a user's mode in a channel.
///////////////////////////////////
void chatSetUserMode(CHAT chat,
					 const gsi_char * channel,
					 const gsi_char * user,
					 int mode);

// Called with the user's mode.
///////////////////////////////
typedef void (* chatGetUserModeCallback)(CHAT chat,
										 CHATBool success,
										 const gsi_char * channel,
										 const gsi_char * user,
										 int mode,
										 void * param);
// Gets a user's mode in a channel.
///////////////////////////////////
void chatGetUserMode(CHAT chat,
					 const gsi_char * channel,
					 const gsi_char * user,
					 chatGetUserModeCallback callback,
					 void * param,
					 CHATBool blocking);

// Gets a user's mode in a channel.
///////////////////////////////////
CHATBool chatGetUserModeNoWait(CHAT chat,
							   const gsi_char * channel,
							   const gsi_char * user,
							   int * mode);


// Called in response to a request for the UDP relay for a channel
////////////////////////////////////////////////////////////////////
typedef void (* chatGetUdpRelayCallback)(CHAT chat,
										 const gsi_char * channel,
										 const gsi_char * udpIp,
										 unsigned short udpPort,
										 int udpKey,
										 void * param);

// Get the UDP relay address for a channel
///////////////////////////////////
void chatGetUdpRelay(CHAT chat,
					 const gsi_char * channel,
					 chatGetUdpRelayCallback callback,
					 void * param,
					 CHATBool blocking);


/*********
** KEYS **
*********/
// Sets global key/values for the local user.
// Set a value to NULL or "" to clear that key.
///////////////////////////////////////////////
void chatSetGlobalKeys(CHAT chat,
					   int num,
					   const gsi_char ** keys,
					   const gsi_char ** values);

// Called with a user's global key/values.
// If used for a set of users, will be
// called with user==NULL when done.
//////////////////////////////////////////
typedef void (* chatGetGlobalKeysCallback)(CHAT chat,
										   CHATBool success,
										   const gsi_char * user,
										   int num,
										   const gsi_char ** keys,
										   const gsi_char ** values,
										   void * param);
// Gets global key/values for a user or users.
// To get the global key/values for one user, pass in that
// user's nick as the target.  To get the global key/values
// for every user in a channel, use the channel name as the target.
///////////////////////////////////////////////////////////////////
void chatGetGlobalKeys(CHAT chat,
					   const gsi_char * target,
					   int num,
					   const gsi_char ** keys,
					   chatGetGlobalKeysCallback callback,
					   void * param,
					   CHATBool blocking);

// Sets channel key/values.
// If user is NULL or "", the keys will be set on the channel.
// Otherwise, they will be set on the user,
// Only ops can set channel keys on other users.
// Set a value to NULL or "" to clear that key.
//////////////////////////////////////////////////////////////
void chatSetChannelKeys(CHAT chat,
						const gsi_char * channel,
						const gsi_char * user,
						int num,
						const gsi_char ** keys,
						const gsi_char ** values);

// Called with a user's channel key/values, or a channel's key/values.
// If used for a set of users, will be called with user==NULL when done.
// If used for a channel, will be called once with user==NULL.
////////////////////////////////////////////////////////////////////////
typedef void (* chatGetChannelKeysCallback)(CHAT chat,
											CHATBool success,
											const gsi_char * channel,
											const gsi_char * user,
											int num,
											const gsi_char ** keys,
											const gsi_char ** values,
											void * param);
// Gets channel key/values for a user or users, or for a channel.
// To get the channel key/values for every user in
// a channel, pass in "*" as the user.  To get the keys for a channel,
// pass in NULL or "".
//////////////////////////////////////////////////////////////////////
void chatGetChannelKeys(CHAT chat,
						const gsi_char * channel,
						const gsi_char * user,
						int num,
						const gsi_char ** keys,
						chatGetChannelKeysCallback callback,
						void * param,
						CHATBool blocking);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// This ASCII versions must be available even when GSI_UNICODE is defined
#ifdef GSI_UNICODE
CHATBool chatGetBasicUserInfoNoWaitA(CHAT chat,
									const char * nick,
									const char ** user,
									const char ** address);
#endif
/*
void chatGetBasicUserInfoA(CHAT chat,
						  const char * user,
						  chatGetBasicUserInfoCallback callback,
						  void * param,
						  CHATBool blocking);
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}	// extern "C"
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // _CHAT_H_
