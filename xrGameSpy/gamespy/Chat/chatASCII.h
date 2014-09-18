/*
GameSpy Chat SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _CHATASCII_H_
#define _CHATASCII_H_

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "chat.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif


// Connects you to a chat server and returns a CHAT object.
///////////////////////////////////////////////////////////
CHAT chatConnectA(const char * serverAddress,
				 int port,
                 const char * nick,
				 const char * user,
				 const char * name,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking);
CHAT chatConnectSpecialA(const char * serverAddress,
				 int port,
                 const char * nick,
				 const char * name,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking);
CHAT chatConnectSecureA(const char * serverAddress,
				 int port,
                 const char * nick,
				 const char * name,
				 const char * gamename,
				 const char * secretKey,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking);
CHAT chatConnectLoginA(const char * serverAddress,
				 int port,
				 int namespaceID,
				 const char * email,
				 const char * profilenick,
				 const char * uniquenick,
				 const char * password,
				 const char * name,
				 const char * gamename,
				 const char * secretKey,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking);
CHAT chatConnectPreAuthA(const char * serverAddress,
				 int port,
				 const char * authtoken,
				 const char * partnerchallenge,
				 const char * name,
				 const char * gamename,
				 const char * secretKey,
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
void chatRetryWithNickA(CHAT chat,
					   const char * nick);

// Register a uniquenick.
// Should be called in response to the chatNickErrorCallback being called
// with a type of CHAT_UNIQUENICK_EXPIRED or CHAT_NO_UNIQUENICK.
// If the uniquenick cannot be registered, the chatNickErrorCallback will
// be called again with a type of CHAT_IN_USE or CHAT_INVALID.
/////////////////////////////////////////////////////////////////////////
void chatRegisterUniqueNickA(CHAT chat,
							int namespaceID,
							const char * uniquenick,
							const char * cdkey);

// Sends raw data, without any interpretation.
//////////////////////////////////////////////
void chatSendRawA(CHAT chat,
				 const char * command);

// Change the local user's nick.
////////////////////////////////
void chatChangeNickA(CHAT chat,
					const char * newNick,
					chatChangeNickCallback callback,
					void * param,
					CHATBool blocking);

// Get our local nickname.
//////////////////////////
const char * chatGetNickA(CHAT chat);

// Copies the oldNick to the newNick, replacing any invalid characters with legal ones.
///////////////////////////////////////////////////////////////////////////////////////
void chatFixNickA(char * newNick,
				 const char * oldNick);

// Removes the namespace extension from a chat nick.
////////////////////////////////////////////////////
const char * chatTranslateNickA(char * nick,
								const char * extension);

// Attempts to authenticates a CD key.
//////////////////////////////////////
void chatAuthenticateCDKeyA(CHAT chat,
						   const char * cdkey,
						   chatAuthenticateCDKeyCallback callback,
						   void * param,
						   CHATBool blocking);

/*************
** CHANNELS **
*************/
// Enumerates the channels available on a chat server.
//////////////////////////////////////////////////////
void chatEnumChannelsA(CHAT chat,
					  const char * filter,
					  chatEnumChannelsCallbackEach callbackEach,
					  chatEnumChannelsCallbackAll callbackAll,
					  void * param,
					  CHATBool blocking);

// Enters a channel.
////////////////////
void chatEnterChannelA(CHAT chat,
					  const char * channel,
					  const char * password,
					  chatChannelCallbacks * callbacks,
					  chatEnterChannelCallback callback,
					  void * param,
					  CHATBool blocking);

// Leaves a channel.
////////////////////
void chatLeaveChannelA(CHAT chat,
					  const char * channel,
					  const char * reason);  // PANTS|03.13.01

// Sends a message to a channel.
////////////////////////////////
void chatSendChannelMessageA(CHAT chat,
							const char * channel,
							const char * message,
							int type);

// Sets the topic in a channel.
///////////////////////////////
void chatSetChannelTopicA(CHAT chat,
						 const char * channel,
						 const char * topic);

// Gets a channel's topic.
//////////////////////////
void chatGetChannelTopicA(CHAT chat,
						 const char * channel,
						 chatGetChannelTopicCallback callback,
						 void * param,
						 CHATBool blocking);

// Sets a channel's mode.
/////////////////////////
void chatSetChannelModeA(CHAT chat,
						const char * channel,
						CHATChannelMode * mode);

// Gets a channel's mode.
/////////////////////////
void chatGetChannelModeA(CHAT chat,
						const char * channel,
						chatGetChannelModeCallback callback,
						void * param,
						CHATBool blocking);

// Sets the password in a channel.
//////////////////////////////////
void chatSetChannelPasswordA(CHAT chat,
							const char * channel,
							CHATBool enable,
							const char * password);

// Gets the password in a channel.
//////////////////////////////////
void chatGetChannelPasswordA(CHAT chat,
							const char * channel,
							chatGetChannelPasswordCallback callback,
							void * param,
							CHATBool blocking);

// Set the maximum number of users allowed in a channel.
////////////////////////////////////////////////////////
void chatSetChannelLimitA(CHAT chat,
						 const char * channel,
						 int limit);

// Enumerate through the bans in a channel.
///////////////////////////////////////////
void chatEnumChannelBansA(CHAT chat,
						 const char * channel,
						 chatEnumChannelBansCallback callback,
						 void * param,
						 CHATBool blocking);

// Adds a channel ban.
//////////////////////
void chatAddChannelBanA(CHAT chat,
					   const char * channel,
					   const char * ban);

// Removes a ban string from a channel.
///////////////////////////////////////
void chatRemoveChannelBanA(CHAT chat,
						  const char * channel,
						  const char * ban);

// Set the group this channel is a part of.
///////////////////////////////////////////
void chatSetChannelGroupA(CHAT chat,
						 const char * channel,
						 const char * group);

// Get the number of users in the channel.
// Returns -1 if we are not in the channel.
///////////////////////////////////////////
int chatGetChannelNumUsersA(CHAT chat,
						   const char * channel);


// Returns CHATTrue if we are in the channel
///////////////////////////////////////////
CHATBool chatInChannelA(CHAT chat,
						   const char * channel);


/**********
** USERS **
**********/
// Enumerate through the users in a channel.
////////////////////////////////////////////
void chatEnumUsersA(CHAT chat,
				   const char * channel,
				   chatEnumUsersCallback callback,
				   void * param,
				   CHATBool blocking);

// Send a private message to a user.
////////////////////////////////////
void chatSendUserMessageA(CHAT chat,
						 const char * user,
						 const char * message,
						 int type);

// Get a user's info.
/////////////////////
void chatGetUserInfoA(CHAT chat,
					 const char * user,
					 chatGetUserInfoCallback callback,
					 void * param,
					 CHATBool blocking);


// Get some basic info on the user.
// PANTS|12.08.2000
///////////////////////////////////
void chatGetBasicUserInfoA(CHAT chat,
						  const char * user,
						  chatGetBasicUserInfoCallback callback,
						  void * param,
						  CHATBool blocking);

// Get basic info without waiting.
// Returns CHATFalse if the info isn't available.
/////////////////////////////////////////////////
CHATBool chatGetBasicUserInfoNoWaitA(CHAT chat,
									const char * nick,
									const char ** user,
									const char ** address);


// Get basic info on all the users in a channel.
// PANTS|12.19.00
////////////////////////////////////////////////
void chatGetChannelBasicUserInfoA(CHAT chat,
								 const char * channel,
								 chatGetChannelBasicUserInfoCallback callback,
								 void * param,
								 CHATBool blocking);

// Invite a user into a channel.
////////////////////////////////
void chatInviteUserA(CHAT chat,
					const char * channel,
					const char * user);

// Kick a user from a channel.
//////////////////////////////
void chatKickUserA(CHAT chat,
				  const char * channel,
				  const char * user,
				  const char * reason);

// Ban a user from a channel.
/////////////////////////////
void chatBanUserA(CHAT chat,
				 const char * channel,
				 const char * user);

// Sets a user's mode in a channel.
///////////////////////////////////
void chatSetUserModeA(CHAT chat,
					 const char * channel,
					 const char * user,
					 int mode);

// Gets a user's mode in a channel.
///////////////////////////////////
void chatGetUserModeA(CHAT chat,
					 const char * channel,
					 const char * user,
					 chatGetUserModeCallback callback,
					 void * param,
					 CHATBool blocking);

// Gets a user's mode in a channel.
///////////////////////////////////
CHATBool chatGetUserModeNoWaitA(CHAT chat,
							   const char * channel,
							   const char * user,
							   int * mode);

// Get the UDP relay address for a channel
///////////////////////////////////
void chatGetUdpRelayA(CHAT chat,
					 const char * channel,
					 chatGetUdpRelayCallback callback,
					 void * param,
					 CHATBool blocking);
/*********
** KEYS **
*********/
// Sets global key/values for the local user.
// Set a value to NULL or "" to clear that key.
///////////////////////////////////////////////
void chatSetGlobalKeysA(CHAT chat,
					   int num,
					   const char ** keys,
					   const char ** values);

// Gets global key/values for a user or users.
// To get the global key/values for one user, pass in that
// user's nick as the target.  To get the global key/values
// for every user in a channel, use the channel name as the target.
///////////////////////////////////////////////////////////////////
void chatGetGlobalKeysA(CHAT chat,
					   const char * target,
					   int num,
					   const char ** keys,
					   chatGetGlobalKeysCallback callback,
					   void * param,
					   CHATBool blocking);

// Sets channel key/values.
// If user is NULL or "", the keys will be set on the channel.
// Otherwise, they will be set on the user,
// Only ops can set channel keys on other users.
// Set a value to NULL or "" to clear that key.
//////////////////////////////////////////////////////////////
void chatSetChannelKeysA(CHAT chat,
						const char * channel,
						const char * user,
						int num,
						const char ** keys,
						const char ** values);

// Gets channel key/values for a user or users, or for a channel.
// To get the channel key/values for every user in
// a channel, pass in "*" as the user.  To get the keys for a channel,
// pass in NULL or "".
//////////////////////////////////////////////////////////////////////
void chatGetChannelKeysA(CHAT chat,
						const char * channel,
						const char * user,
						int num,
						const char ** keys,
						chatGetChannelKeysCallback callback,
						void * param,
						CHATBool blocking);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// This ASCII versions must be available even when GSI_UNICODE is defined
CHATBool chatGetBasicUserInfoNoWaitA(CHAT chat,
									const char * nick,
									const char ** user,
									const char ** address);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}	// extern "C"
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // _CHATASCII_H_
