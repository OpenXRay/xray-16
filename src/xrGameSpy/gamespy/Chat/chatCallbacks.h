/*
GameSpy Chat SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _CHATCALLBACKS_H_
#define _CHATCALLBACKS_H_

/*************
** INCLUDES **
*************/
#include "chat.h"
#include "chatMain.h"

/************
** DEFINES **
************/
enum
{
	CALLBACK_RAW,
	CALLBACK_DISCONNECTED,
	CALLBACK_PRIVATE_MESSAGE,
	CALLBACK_INVITED,
	CALLBACK_CHANNEL_MESSAGE,
	CALLBACK_KICKED,
	CALLBACK_USER_JOINED,
	CALLBACK_USER_PARTED,
	CALLBACK_USER_CHANGED_NICK,
	CALLBACK_TOPIC_CHANGED,
	CALLBACK_CHANNEL_MODE_CHANGED,
	CALLBACK_USER_MODE_CHANGED,
	CALLBACK_USER_LIST_UPDATED,
	CALLBACK_ENUM_CHANNELS_EACH,
	CALLBACK_ENUM_CHANNELS_ALL,
	CALLBACK_ENTER_CHANNEL,
	CALLBACK_GET_CHANNEL_TOPIC,
	CALLBACK_GET_CHANNEL_MODE,
	CALLBACK_GET_CHANNEL_PASSWORD,
	CALLBACK_ENUM_USERS,
	CALLBACK_GET_USER_INFO,
	CALLBACK_GET_BASIC_USER_INFO,
	CALLBACK_GET_CHANNEL_BASIC_USER_INFO,
	CALLBACK_GET_USER_MODE,
	CALLBACK_ENUM_CHANNEL_BANS,
	CALLBACK_NICK_ERROR,
	CALLBACK_CHANGE_NICK,
	CALLBACK_NEW_USER_LIST,
	CALLBACK_BROADCAST_KEY_CHANGED,
	CALLBACK_GET_GLOBAL_KEYS,
	CALLBACK_GET_CHANNEL_KEYS,
	CALLBACK_AUTHENTICATE_CDKEY,
	CALLBACK_GET_UDPRELAY,
	CALLBACK_NUM
};

/**********
** TYPES **
**********/
typedef struct ciCallbackRawParams
{
	char * raw;
} ciCallbackRawParams;

typedef struct ciCallbackDisconnectedParams
{
	char * reason;
} ciCallbackDisconnectedParams;

typedef struct ciCallbackPrivateMessageParams
{
	char * user;
	char * message;
	int type;
} ciCallbackPrivateMessageParams;

typedef struct ciCallbackInvitedParams
{
	char * channel;
	char * user;
} ciCallbackInvitedParams;

typedef struct ciCallbackChannelMessageParams
{
	char * channel;
	char * user;
	char * message;
	int type;
} ciCallbackChannelMessageParams;

typedef struct ciCallbackKickedParams
{
	char * channel;
	char * user;
	char * reason;
} ciCallbackKickedParams;

typedef struct ciCallbackUserJoinedParams
{
	char * channel;
	char * user;
	int mode;
} ciCallbackUserJoinedParams;

typedef struct ciCallbackUserPartedParams
{
	char * channel;
	char * user;
	int why;
	char * reason;
	char * kicker;
} ciCallbackUserPartedParams;

typedef struct ciCallbackUserChangedNickParams
{
	char * channel;
	char * oldNick;
	char * newNick;
} ciCallbackUserChangedNickParams;

typedef struct ciCallbackTopicChangedParams
{
	char * channel;
	char * topic;
} ciCallbackTopicChangedParams;

typedef struct ciCallbackChannelModeChangedParams
{
	char * channel;
	CHATChannelMode * mode;
} ciCallbackChannelModeChangedParams;

typedef struct ciCallbackUserModeChangedParams
{
	char * channel;
	char * user;
	int mode;
} ciCallbackUserModeChangedParams;

typedef struct ciCallbackUserListUpdatedParams
{
	char * channel;
} ciCallbackUserListUpdatedParams;

typedef struct ciCallbackConnectParams
{
	CHATBool success;
} ciCallbackConnectParams;

typedef struct ciCallbackEnumChannelsEachParams
{
	CHATBool success;
	int index;
	char * channel;
	char * topic;
	int numUsers;
	void * param;
} ciCallbackEnumChannelsEachParams;

typedef struct ciCallbackEnumChannelsAllParams
{
	CHATBool success;
	int numChannels;
	char ** channels;
	char ** topics;
	int * numUsers;
} ciCallbackEnumChannelsAllParams;

typedef struct ciCallbackEnterChannelParams
{
	CHATBool success;
	CHATEnterResult result;
	char * channel;
} ciCallbackEnterChannelParams;

typedef struct ciCallbackGetChannelTopicParams
{
	CHATBool success;
	char * channel;
	char * topic;
} ciCallbackGetChannelTopicParams;

typedef struct ciCallbackGetChannelModeParams
{
	CHATBool success;
	char * channel;
	CHATChannelMode * mode;
} ciCallbackGetChannelModeParams;

typedef struct ciCallbackGetChannelPasswordParams
{
	CHATBool success;
	char * channel;
	CHATBool enabled;
	char * password;
} ciCallbackGetChannelPasswordParams;

typedef struct ciCallbackEnumUsersParams
{
	CHATBool success;
	char * channel;
	int numUsers;
	char ** users;
	int * modes;
} ciCallbackEnumUsersParams;

typedef struct ciCallbackGetUserInfoParams
{
	CHATBool success;
	char * nick;
	char * user;
	char * name;
	char * address;
	int numChannels;
	char ** channels;
} ciCallbackGetUserInfoParams;

typedef struct ciCallbackGetBasicUserInfoParams
{
	CHATBool success;
	char * nick;
	char * user;
	char * address;
} ciCallbackGetBasicUserInfoParams;

typedef struct ciCallbackGetChannelBasicUserInfoParams
{
	CHATBool success;
	char * channel;
	char * nick;
	char * user;
	char * address;
} ciCallbackGetChannelBasicUserInfoParams;

typedef struct ciCallbackGetUserModeParams
{
	CHATBool success;
	char * channel;
	char * user;
	int mode;
} ciCallbackGetUserModeParams;

typedef struct ciCallbackEnumChannelBansParams
{
	CHATBool success;
	char * channel;
	int numBans;
	char ** bans;
} ciCallbackEnumChannelBansParams;

typedef struct ciCallbackNickErrorParams
{
	int type;
	char * nick;
	int numSuggestedNicks;
	char ** suggestedNicks;
} ciCallbackNickErrorParams;

typedef struct ciCallbackChangeNickParams
{
	CHATBool success;
	char * oldNick;
	char * newNick;
} ciCallbackChangeNickParams;

typedef struct ciCallbackNewUserListParams
{
	char * channel;
	int numUsers;
	char ** users;
	int * modes;
} ciCallbackNewUserListParams;

typedef struct ciCallbackBroadcastKeyChangedParams
{
	char * channel;
	char * user;
	char * key;
	char * value;
} ciCallbackBroadcastKeyChangedParams;

typedef struct ciCallbackGetGlobalKeysParams
{
	CHATBool success;
	char * user;
	int num;
	char ** keys;
	char ** values;
} ciCallbackGetGlobalKeysParams;

typedef struct ciCallbackGetChannelKeysParams
{
	CHATBool success;
	char * channel;
	char * user;
	int num;
	char ** keys;
	char ** values;
} ciCallbackGetChannelKeysParams;

typedef struct ciCallbackAuthenticateCDKeyParams
{
	int result;
	char * message;
} ciCallbackAuthenticateCDKeyParams;

typedef struct ciCallbackGetUdpRelayParams
{
	char * channel;
	char * udpIp;
	unsigned short udpPort;
	int udpKey;
} ciCallbackGetUdpRelayParams;

/**************
** FUNCTIONS **
**************/
CHATBool ciInitCallbacks(ciConnection * connection);
void ciCleanupCallbacks(CHAT chat);
#define ciAddCallback(chat, type, callback, callbackParams, param, ID, channel) ciAddCallback_(chat, type, callback, callbackParams, param, ID, channel, sizeof(*callbackParams))
CHATBool ciAddCallback_(CHAT chat, int type, void * callback, void * callbackParams, void * param, int ID, const char * channel, size_t callbackParamsSize);
void ciCallCallbacks(CHAT chat, int ID);
CHATBool ciCheckCallbacksForID(CHAT chat, int ID);

#endif
