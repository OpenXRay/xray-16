/*
GameSpy Chat SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _CHATHANDLERS_H_
#define _CHATHANDLERS_H_

/*************
** INCLUDES **
*************/
#include "chat.h"
#include "chatSocket.h"


/**********
** TYPES **
**********/
typedef struct ciServerMessageType
{
	char * command;
	void (* handler)(CHAT chat, const ciServerMessage * message);
} ciServerMessageType;

typedef char ** ciCommands;
typedef struct ciServerMessageFilter
{
	int type;
	gsi_time timeout;

	char * name;
	char * name2;

	void * callback;
	void * callback2;
	void * param;

	void * data;

	int ID;

	struct ciServerMessageFilter * pnext;
} ciServerMessageFilter;

/************
** GLOBALS **
************/
extern ciServerMessageType serverMessageTypes[];
extern int numServerMessageTypes;

/**************
** FUNCTIONS **
**************/
void ciFilterThink(CHAT chat);
void ciCleanupFilters(CHAT chat);
int ciAddLISTFilter(CHAT chat, chatEnumChannelsCallbackEach callbackEach, chatEnumChannelsCallbackAll callbackAll, void * param);
int ciAddJOINFilter(CHAT chat, const char * channel, chatEnterChannelCallback callback, void * param, chatChannelCallbacks * callbacks, const char * password);
int ciAddTOPICFilter(CHAT chat, const char * channel, chatGetChannelTopicCallback callback, void * param);
int ciAddNAMESFilter(CHAT chat, const char * channel, chatEnumUsersCallback callback, void * param);
int ciAddWHOISFilter(CHAT chat, const char * user, chatGetUserInfoCallback callback, void * param);
int ciAddWHOFilter(CHAT chat, const char * user, chatGetBasicUserInfoCallback callback, void * param);
int ciAddCWHOFilter(CHAT chat, const char * channel, chatGetChannelBasicUserInfoCallback callback, void * param);
int ciAddCMODEFilter(CHAT chat, const char * channel, chatGetChannelModeCallback callback, void * param);
int ciAddUMODEFilter(CHAT chat, const char * user, const char * channel, chatGetUserModeCallback callback, void * param);
int ciAddBANFilter(CHAT chat, const char * user, const char * channel);
int ciAddGETBANFilter(CHAT chat, const char * channel, chatEnumChannelBansCallback callback, void * param);
int ciAddNICKFilter(CHAT chat, const char * oldNick, const char * newNick, chatChangeNickCallback callback, void * param);
int ciAddUNQUIETFilter(CHAT chat, const char * channel);
int ciAddGETKEYFilter(CHAT chat, const char * cookie, int num, const char ** keys, const char * channel, chatGetGlobalKeysCallback callback, void * param);
int ciAddGETCKEYFilter(CHAT chat, const char * cookie, int num, const char ** keys, CHATBool channel, CHATBool getBroadcastKeys, chatGetChannelKeysCallback callback, void * param);
int ciAddGETCHANKEYFilter(CHAT chat, const char * cookie, int num, const char ** keys, CHATBool getBroadcastKeys, chatGetChannelKeysCallback callback, void * param);
int ciAddCDKEYFilter(CHAT chat, chatAuthenticateCDKeyCallback callback, void * param);
int ciAddGETUDPRELAYFilter(CHAT chat, const char * channel, chatGetUdpRelayCallback callback, void * param);
int ciGetNextID(CHAT chat);
CHATBool ciCheckFiltersForID(CHAT chat, int ID);

#endif
