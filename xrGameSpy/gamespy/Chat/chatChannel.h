/*
GameSpy Chat SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _CHATCHANNEL_H_
#define _CHATCHANNEL_H_

/*************
** INCLUDES **
*************/
#include "chat.h"

/************
** DEFINES **
************/

/**************
** FUNCTIONS **
**************/
CHATBool ciInitChannels(ciConnection * connection);
void ciCleanupChannels(CHAT chat);
void ciChannelEntering(CHAT chat, const char * channel);
CHATBool ciIsEnteringChannel(CHAT chat, const char * channel);
void ciChannelEntered(CHAT chat, const char * channel, chatChannelCallbacks * callbacks);
void ciChannelLeft(CHAT chat, const char * channel);
chatChannelCallbacks * ciGetChannelCallbacks(CHAT chat, const char * channel);
typedef void (* ciChannelListUsersCallback)(CHAT chat, const char * channel, int numUsers, const char ** users, int * modes, void * param);
void ciChannelListUsers(CHAT chat, const char * channel, ciChannelListUsersCallback callback, void * param);
CHATBool ciInChannel(CHAT chat, const char * channel);
CHATBool ciGetChannelMode(CHAT chat, const char * channel, CHATChannelMode * mode);
void ciSetChannelMode(CHAT chat, const char * channel, CHATChannelMode * mode);
void ciSetChannelPassword(CHAT chat, const char * channel, const char * password);
const char * ciGetChannelPassword(CHAT chat, const char * channel);
void ciJoinCallbackCalled(CHAT chat, const char * channel);
CHATBool ciWasJoinCallbackCalled(CHAT chat, const char * channel);
void ciSetChannelTopic(CHAT chat, const char * channel, const char * topic);
const char * ciGetChannelTopic(CHAT chat, const char * channel);
int ciGetChannelNumUsers(CHAT chat, const char * channel);

void ciUserEnteredChannel(CHAT chat, const char * nick, const char * channel, int mode, const char * user, const char * address);
void ciUserLeftChannel(CHAT chat, const char * user, const char * channel);
void ciUserChangedNick(CHAT chat, const char * oldNick, const char * newNick);
void ciUserChangedMode(CHAT chat, const char * user, const char * channel, int mode, CHATBool enabled);
typedef void (* ciUserEnumChannelsCallback)(CHAT chat, const char * user, const char * channel, void * param);
void ciUserEnumChannels(CHAT chat, const char * user, ciUserEnumChannelsCallback callback, void * param);
CHATBool ciUserInChannel(CHAT chat, const char * channel, const char * user);
int ciGetUserMode(CHAT chat, const char * channel, const char * user);
void ciEnumJoinedChannels(CHAT chat, chatEnumJoinedChannelsCallback callback, void * param);
void ciSetUserBasicInfo(CHAT chat, const char * nick, const char * user, const char * address);
void ciClearAllUsers(CHAT chat);

// Because these return pointers to SDK memory, we must have a widestring version
// so we can return a pointer to widestring data
// DO NOT CHANGE nick to an unsigned short*, nicks are internally store as char*
CHATBool ciGetUserBasicInfoA(CHAT chat, const char * nick, const char ** user, const char ** address);
CHATBool ciGetUserBasicInfoW(CHAT chat, const char * nick, const unsigned short ** user, const unsigned short ** address);

#endif
