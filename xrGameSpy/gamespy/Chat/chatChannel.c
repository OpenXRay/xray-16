/*
GameSpy Chat SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

/*************
** INCLUDES **
*************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "chatMain.h"
#include "chatChannel.h"
#include "chatCallbacks.h"

#if defined(_WIN32)
// Compiler warns when explicitly casting from function* to void*
#pragma warning(disable:4054)
#endif

/************
** DEFINES **
************/
#define CHANNEL_HASH_BUCKETS    7
#define USER_HASH_BUCKETS      61
#define MAX_CHANNEL           257
#define MAX_TOPIC             128
#define MAX_NAME              128
#define MAX_CACHED_USER        24
#define MAX_CACHED_ADDRESS     64

#define ASSERT_CHANNEL()   assert(channel != NULL); assert(channel[0] != '\0'); assert(strlen(channel) < MAX_CHANNEL);
#define ASSERT_USER(user)  assert(user != NULL); assert(user[0] != '\0'); assert(strlen(user) < MAX_NAME);
#define ASSERT_STR(str)    assert(str != NULL); assert(str[0] != '\0');
#define ASSERT_MODE(mode)  assert((mode >= 0) && (mode <= 3));

/**********
** TYPES **
**********/
typedef struct ciChatChannel
{
	char name[MAX_CHANNEL];
	chatChannelCallbacks callbacks;

	HashTable users;
	
	CHATChannelMode mode;
	CHATBool gotMode;

	char * password;

	CHATBool joinCallbackCalled;

	char topic[MAX_TOPIC];
} ciChatChannel;

typedef struct ciChatUser
{
	char name[MAX_NAME];
	char user[MAX_CACHED_USER];
	char address[MAX_CACHED_ADDRESS];
#ifdef GSI_UNICODE
	// must store a unicode versions since a pointer to them is
	// given to the developer application
	unsigned short userW[MAX_CACHED_USER];
	unsigned short addressW[MAX_CACHED_ADDRESS];
#endif
	CHATBool gotUserAndAddress;
	int mode;
} ciChatUser;

typedef struct ciUserEnumChannelsData
{
	CHAT chat;
	ciChatUser * user;
	ciUserEnumChannelsCallback callback;
	void * param;
} ciUserEnumChannelsData;

typedef struct ciUserChangedNickData
{
	CHAT chat;
	const char * oldNick;
	const char * newNick;
} ciUserChangedNickData;

typedef struct ciChannelListUsersData
{
	CHAT chat;
	int numUsers;
	char ** users;
	int * modes;
} ciChannelListUsersData;

typedef struct ciEnumJoinedChannelsData
{
	CHAT chat;
	chatEnumJoinedChannelsCallback callback;
	void * param;
	int index;
} ciEnumJoinedChannelsData;

typedef struct ciSetUserBasicInfoData
{
	ciChatUser * chatUser;
	char * user;
	char * address;
} ciSetUserBasicInfoData;

typedef struct ciGetUserBasicInfoData
{
	CHATBool found;
	ciChatUser * chatUser;
	char * user;
	char * address;
#ifdef GSI_UNICODE  
	// must store a unicode version since ptr to address is given to dev app
	unsigned short * userW;
	unsigned short * addressW;
#endif
} ciGetUserBasicInfoData;

typedef struct ciClearAllUsersData
{
	CHAT chat;
	ciChatChannel * channel;
} ciClearAllUsersData;

/***************
** PROTOTYPES **
****************/
// CodeWarrior will warn if this is not prototyped
int GS_STATIC_CALLBACK ciEnteringChannelComparator(const void *param1, const void *param2);


/**************
** FUNCTIONS **
**************/
static int ciHashString(const char * str, int numBuckets)
{
	unsigned int hash;
	int c;
	
	ASSERT_STR(str);

	hash = 0;
	while((c = *str++) != '\0')
		hash += (unsigned int)tolower(c);

	return ((int)hash % numBuckets);
}

static ciChatChannel * ciGetChannel(ciConnection * connection, const char * channel)
{
	ciChatChannel * chatChannel;
	ciChatChannel channelTemp;

	strzcpy(channelTemp.name, channel, MAX_CHANNEL);
	chatChannel = (ciChatChannel *)TableLookup(connection->channelTable, &channelTemp);

	return chatChannel;
}

/**********************
** CHANNEL CALLBACKS **
**********************/
static int ciChannelTableHashFn(const void * elem, int numBuckets)
{
	int hash;

	assert(elem != NULL);
	assert(numBuckets > 0);

	hash = ciHashString(((ciChatChannel *)elem)->name, numBuckets);

	return hash;
}

static int GS_STATIC_CALLBACK ciChannelTableCompareFn(const void * elem1, const void * elem2)
{
	const char * str1;
	const char * str2;

	assert(elem1 != NULL);
	assert(elem2 != NULL);

	str1 = ((ciChatChannel *)elem1)->name;
	str2 = ((ciChatChannel *)elem2)->name;
	assert(str1 != NULL);
	assert(str2 != NULL);
	ASSERT_STR(str1);
	ASSERT_STR(str2);

	return strcasecmp(str1, str2);
}

static void ciChannelTableElementFreeFn(void * elem)
{
	ciChatChannel * channel;

	assert(elem != NULL);

	channel = (ciChatChannel *)elem;
	gsifree(channel->password);
	if(channel->users)
		TableFree(channel->users);
}

/*******************
** USER CALLBACKS **
*******************/
static int ciUserTableHashFn(const void * elem, int numBuckets)
{
	int hash;

	assert(elem != NULL);
	assert(numBuckets > 0);

	hash = ciHashString(((ciChatUser *)elem)->name, numBuckets);

	return hash;
}

static int GS_STATIC_CALLBACK ciUserTableCompareFn(const void * elem1, const void * elem2)
{
	const char * str1;
	const char * str2;

	assert(elem1 != NULL);
	assert(elem2 != NULL);

	// Sept 20, 2004 - Bill Dewey
	// A NULL parameter has been causing Arcade to crash.  (FogBugz 2825)
	// This doesn't solve the real problem (elem is invalid), 
	// it just tries to keep Arcade going
	if (elem1 == NULL || elem2 == NULL)
	{
		// A NULL user is automatically "less than"
		if (elem1 == NULL && elem2 == NULL)
			return 0;
		else if (elem1 == NULL)
			return -1;
		else // if (elem2 == NULL)
			return 1;
	}

	str1 = ((ciChatUser *)elem1)->name;
	str2 = ((ciChatUser *)elem2)->name;
	ASSERT_STR(str1);
	ASSERT_STR(str2);

	return strcasecmp(str1, str2);
}

static void ciUserTableElementFreeFn(void * elem)
{
	assert(elem != NULL);
	
	GSI_UNUSED(elem);

	// if anything is done in here that changes the structure
	// ciUserChangeNickMap() must be updated
}

/**********************
** CHANNEL FUNCTIONS **
**********************/
CHATBool ciInitChannels(ciConnection * connection)
{
	connection->channelTable = TableNew2(sizeof(ciChatChannel), CHANNEL_HASH_BUCKETS, 2, ciChannelTableHashFn, ciChannelTableCompareFn, ciChannelTableElementFreeFn);
	if(connection->channelTable == NULL)
		return CHATFalse;

	connection->enteringChannelList = ArrayNew(sizeof(ciChatChannel), 0, NULL);
	if(connection->enteringChannelList == NULL)
	{
		TableFree(connection->channelTable);
		return CHATFalse;
	}

	return CHATTrue;
}

void ciCleanupChannels(CHAT chat)
{
	CONNECTION;

	if(connection->channelTable != NULL)
		TableFree(connection->channelTable);

	if(connection->enteringChannelList != NULL)
		ArrayFree(connection->enteringChannelList);
}

void ciChannelEntering(CHAT chat, const char * channel)
{
	ciChatChannel chatChannel;

	CONNECTION;

	// Setup the channel.
	/////////////////////
	memset(&chatChannel, 0, sizeof(ciChatChannel));
	strzcpy(chatChannel.name, channel, MAX_CHANNEL);

	// Add the channel to the list.
	///////////////////////////////
	ArrayAppend(connection->enteringChannelList, &chatChannel);
}

int GS_STATIC_CALLBACK ciEnteringChannelComparator(const void *param1, const void *param2)
{
	ciChatChannel * channel1 = (ciChatChannel *)param1;
	ciChatChannel * channel2 = (ciChatChannel *)param2;
	return strcasecmp(channel1->name, channel2->name);
}

CHATBool ciIsEnteringChannel(CHAT chat, const char * channel)
{
	int i;
	int count;
	ciChatChannel * chatChannel;

	CONNECTION;

	count = ArrayLength(connection->enteringChannelList);
	for(i = 0 ; i < count ; i++)
	{
		chatChannel = (ciChatChannel *)ArrayNth(connection->enteringChannelList, i);
		assert(chatChannel);
		if(strcasecmp(chatChannel->name, channel) == 0)
			return CHATTrue;
	}

	return CHATFalse;
}

void ciChannelEntered(CHAT chat, const char * channel, chatChannelCallbacks * callbacks)
{
	ciChatChannel chatChannel;
	char * password;
	int index;
	CONNECTION;

	ASSERT_CHANNEL();
	assert(callbacks != NULL);

	// Setup an empty password.
	///////////////////////////
	password = (char *)gsimalloc(2);
	if(password == NULL)
		return; //ERRCON
	strcpy(password, "");

	// Setup the channel.
	/////////////////////
	memset(&chatChannel, 0, sizeof(ciChatChannel));
	chatChannel.callbacks = *callbacks;
	strzcpy(chatChannel.name, channel, MAX_CHANNEL);
	chatChannel.users = TableNew2(sizeof(ciChatUser), USER_HASH_BUCKETS, 2, ciUserTableHashFn, ciUserTableCompareFn, ciUserTableElementFreeFn);
	if(chatChannel.users == NULL)
		return; //ERRCON
	chatChannel.gotMode = CHATFalse;
	chatChannel.password = password;
	chatChannel.joinCallbackCalled = CHATFalse;
	chatChannel.topic[0] = '\0';

	// Check if this one is in the entering list.
	/////////////////////////////////////////////
	index = ArraySearch(connection->enteringChannelList, &chatChannel, ciEnteringChannelComparator, 0, 0);
	if(index != NOT_FOUND)
		ArrayRemoveAt(connection->enteringChannelList, index);

	// Add the channel to the table.
	////////////////////////////////
	TableEnter(connection->channelTable, &chatChannel);
}

void ciChannelLeft(CHAT chat, const char * channel)
{
	ciChatChannel chatChannel;
	int index;
	//int rcode;
	CONNECTION;
	
	ASSERT_CHANNEL();

	// Setup a temp channel with the same name.
	///////////////////////////////////////////
	strzcpy(chatChannel.name, channel, MAX_CHANNEL);

	// Check if this one is in the entering list.
	/////////////////////////////////////////////
	index = ArraySearch(connection->enteringChannelList, &chatChannel, ciEnteringChannelComparator, 0, 0);
	if(index != NOT_FOUND)
	{
		// Remove it from the entering list.
		////////////////////////////////////
		ArrayRemoveAt(connection->enteringChannelList, index);
	}
	else
	{
		// Remove based on the name.
		////////////////////////////
		//rcode = TableRemove(connection->channelTable, &chatChannel);
		TableRemove(connection->channelTable, &chatChannel);

		// This will assert if we don't think we're in this channel.
		////////////////////////////////////////////////////////////
		//assert(rcode != 0);
	}
}

chatChannelCallbacks * ciGetChannelCallbacks(CHAT chat, const char * channel)
{
	ciChatChannel * chatChannel;
	CONNECTION;
	
	ASSERT_CHANNEL();

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return NULL; //ERRCON

	return &chatChannel->callbacks;
}

static void ciChannelListUsersMap(void * elem, void * clientData)
{
	ciChatUser * user;
	ciChannelListUsersData * data;
	void * tempPtr;

	assert(elem != NULL);
	assert(clientData != NULL);

	// Get the user.
	////////////////
	user = (ciChatUser *)elem;
	ASSERT_USER(user->name);

	// Get the data.
	////////////////
	data = (ciChannelListUsersData *)clientData;
	assert(data->numUsers >= 0);
#ifdef _DEBUG
	{
	int i;
	for(i = 0 ; i < data->numUsers ; i++)
	{
		ASSERT_USER(data->users[i]);
		ASSERT_MODE(data->modes[i]);
	}
	}
#endif

	// Resize the arrays.
	// TODO: resize in increments.
	//////////////////////////////
	tempPtr = gsirealloc(data->users, sizeof(char *) * (data->numUsers + 1));
	if(tempPtr == NULL)
	{
		assert(0);
		return; //ERRCON
	}
	data->users = (char **)tempPtr;
	tempPtr = gsirealloc(data->modes, sizeof(int) * (data->numUsers + 1));
	if(tempPtr == NULL)
	{
		assert(0);
		return; //ERRCON
	}
	data->modes = (int *)tempPtr;

	// Fill in the data.
	////////////////////
	data->users[data->numUsers] = user->name;
	data->modes[data->numUsers] = user->mode;
	data->numUsers++;
}

void ciChannelListUsers(CHAT chat, const char * channel, ciChannelListUsersCallback callback, void * param)
{
	ciChatChannel * chatChannel;
	ciChannelListUsersData data;
	CONNECTION;

	ASSERT_CHANNEL();
	assert(callback != NULL);

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return; //ERRCON

	// Enum through the users.
	//////////////////////////
	data.chat = chat;
	data.numUsers = 0;
	data.users = NULL;
	data.modes = NULL;
	TableMap(chatChannel->users, ciChannelListUsersMap, &data);

	// Call the callback.
	/////////////////////
	callback(chat, channel, data.numUsers, (const char **)data.users, data.modes, param);

	// gsifree the memory.
	///////////////////
	gsifree(data.users);
	gsifree(data.modes);
}

CHATBool ciInChannel(CHAT chat, const char * channel)
{
	CONNECTION;

	if(ciGetChannel(connection, channel) == NULL)
		return CHATFalse;

	return CHATTrue;
}

CHATBool ciGetChannelMode(CHAT chat, const char * channel, CHATChannelMode * mode)
{
	ciChatChannel * chatChannel;
	CONNECTION;
	
	ASSERT_CHANNEL();

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return CHATFalse; //ERRCON

	// Did we get the mode yet?
	///////////////////////////
	if(!chatChannel->gotMode)
		return CHATFalse; //ERRCON

	// Copy the mode.
	/////////////////
	memcpy(mode, &chatChannel->mode, sizeof(CHATChannelMode));

	return CHATTrue;
}

void ciSetChannelMode(CHAT chat, const char * channel, CHATChannelMode * mode)
{
	ciChatChannel * chatChannel;
	CONNECTION;
	
	ASSERT_CHANNEL();

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return; //ERRCON

	// Set the gotMode flag.
	////////////////////////
	chatChannel->gotMode = CHATTrue;

	// Copy the mode.
	/////////////////
	memcpy(&chatChannel->mode, mode, sizeof(CHATChannelMode));
}

void ciSetChannelPassword(CHAT chat, const char * channel, const char * password)
{
	int len;
	ciChatChannel * chatChannel;
	CONNECTION;
	
	ASSERT_CHANNEL();

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return; //ERRCON

	// gsifree the old password.
	/////////////////////////
	gsifree(chatChannel->password);

	// Set the password.
	////////////////////
	if(password == NULL)
		password = "";
	len = (int)(strlen(password) + 1);
	chatChannel->password = (char *)gsimalloc((unsigned int)len);
	if(chatChannel->password == NULL)
		return; //ERRCON
	memcpy(chatChannel->password, password, (unsigned int)len);
}

const char * ciGetChannelPassword(CHAT chat, const char * channel)
{
	ciChatChannel * chatChannel;
	CONNECTION;
	
	ASSERT_CHANNEL();

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return NULL; //ERRCON

	// Return the password.
	///////////////////////
	return chatChannel->password;
}

void ciJoinCallbackCalled(CHAT chat, const char * channel)
{
	ciChatChannel * chatChannel;
	CONNECTION;

	ASSERT_CHANNEL();

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return; //ERRCON

	// Callback was called.
	///////////////////////
	chatChannel->joinCallbackCalled = CHATTrue;
}

CHATBool ciWasJoinCallbackCalled(CHAT chat, const char * channel)
{
	ciChatChannel * chatChannel;
	CONNECTION;

	ASSERT_CHANNEL();

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return CHATFalse; //ERRCON

	return chatChannel->joinCallbackCalled;
}

void ciSetChannelTopic(CHAT chat, const char * channel, const char * topic)
{
	ciChatChannel * chatChannel;
	CONNECTION;
	
	ASSERT_CHANNEL();

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return; //ERRCON

	// Set the topic.
	/////////////////
	strzcpy(chatChannel->topic, topic, MAX_TOPIC);
}

const char * ciGetChannelTopic(CHAT chat, const char * channel)
{
	ciChatChannel * chatChannel;
	CONNECTION;
	
	ASSERT_CHANNEL();

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return NULL; //ERRCON

	return chatChannel->topic;
}

int ciGetChannelNumUsers(CHAT chat, const char * channel)
{
	ciChatChannel * chatChannel;
	CONNECTION;
	
	ASSERT_CHANNEL();

	// Find this channel.
	/////////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return -1;

	return TableCount(chatChannel->users);
}

/*******************
** USER FUNCTIONS **
*******************/
void ciUserEnteredChannel(CHAT chat, const char * name, const char * channel, int mode, const char * user, const char * address)
{
	ciChatUser chatUser;
	ciChatChannel * chatChannel;
	CONNECTION;

	ASSERT_USER(name);
	ASSERT_STR(channel);
	ASSERT_MODE(mode);

	// Get the channel.
	///////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(!chatChannel)
		return;

	// Setup the user.
	//////////////////
	memset(&chatUser, 0, sizeof(ciChatUser));
	strzcpy(chatUser.name, name, MAX_NAME);
	if(user && address)
	{
		strzcpy(chatUser.user, user, MAX_CACHED_USER);
		strzcpy(chatUser.address, address, MAX_CACHED_ADDRESS);
		chatUser.gotUserAndAddress = CHATTrue;

#ifdef GSI_UNICODE // update the unicode versions
		AsciiToUCS2String(chatUser.user, chatUser.userW);
		AsciiToUCS2String(chatUser.address, chatUser.addressW);
#endif
	}
	else
	{
		chatUser.gotUserAndAddress = CHATFalse;
	}
	chatUser.mode = mode;

	// Add it to the channel.
	/////////////////////////
	TableEnter(chatChannel->users, &chatUser);

	// Check that we're in.
	///////////////////////
	assert(TableLookup(chatChannel->users, &chatUser) != NULL);
}

void ciUserLeftChannel(CHAT chat, const char * user, const char * channel)
{
	ciChatUser chatUser;
	ciChatChannel * chatChannel;
	CONNECTION;

	ASSERT_USER(user);
	ASSERT_STR(channel);

	// Get the channel.
	///////////////////
	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return; //ERRCON

	// Setup a temp user with the same name.
	////////////////////////////////////////
	strzcpy(chatUser.name, user, MAX_NAME);

	// Remove it.
	/////////////
	TableRemove(chatChannel->users, &chatUser);
}

static void ciUserEnumChannelsMap(void * elem, void * clientData)
{
	ciChatChannel * channel;
	ciChatUser * user;
	ciUserEnumChannelsData * data;

	assert(elem != NULL);
	assert(clientData != NULL);

	// Get the channel.
	///////////////////
	channel = (ciChatChannel *)elem;
	assert(channel->users != NULL);

	// Get the user and callback.
	/////////////////////////////
	data = (ciUserEnumChannelsData *)clientData;
	assert(data->user != NULL);
	assert(data->user->name[0] != '\0');
	assert(data->callback != NULL);

	// Check for the user.
	//////////////////////
	user = (ciChatUser *)TableLookup(channel->users, data->user);
	if(user != NULL)
	{
		// Call the callback.
		/////////////////////
		data->callback(data->chat, data->user->name, channel->name, data->param);
	}
}

void ciUserEnumChannels(CHAT chat, const char * user, ciUserEnumChannelsCallback callback, void * param)
{
	ciChatUser chatUser;
	ciUserEnumChannelsData data;
	CONNECTION;

	ASSERT_USER(user);
	assert(callback != NULL);

	strzcpy(chatUser.name, user, MAX_NAME);
	data.chat = chat;
	data.user = &chatUser;
	data.callback = callback;
	data.param = param;

	// Enum through channels looking for this user.
	///////////////////////////////////////////////
	TableMap(connection->channelTable, ciUserEnumChannelsMap, &data);
}

static void ciUserChangeNickMap(void * elem, void * clientData)
{
	ciChatChannel * channel;
	ciChatUser tempUser;
	ciChatUser * user;
	ciUserChangedNickData * data;
	ciCallbackUserChangedNickParams params;
	int rcode;

	assert(elem != NULL);
	assert(clientData != NULL);

	// Get the channel.
	///////////////////
	channel = (ciChatChannel *)elem;
	assert(channel->users != NULL);

	// Get the data.
	////////////////
	data = (ciUserChangedNickData *)clientData;
	ASSERT_USER(data->newNick);
	ASSERT_USER(data->oldNick);

	// Check for the user.
	//////////////////////
	user = (ciChatUser *)TableLookup(channel->users, data->oldNick);
	if(user != NULL)
	{
		memcpy(&tempUser, user, sizeof(ciChatUser));

		// Remove the old user.
		///////////////////////
		rcode = TableRemove(channel->users, user);
		assert(rcode != 0);
		user = &tempUser;

		// Update the nick.
		///////////////////
		strzcpy(user->name, data->newNick, MAX_NAME);

		// Add it back.
		///////////////
		TableEnter(channel->users, user);

		// Was the join callback called?
		////////////////////////////////
		if(ciWasJoinCallbackCalled(data->chat, channel->name))  //PANTS - 03.01.00 - check if the join callback was called
		{
			// Add the callback.
			////////////////////
			if(channel->callbacks.userChangedNick != NULL)
			{
				params.channel = channel->name;
				params.oldNick = (char *)data->oldNick;
				params.newNick = (char *)data->newNick;
				ciAddCallback(data->chat, CALLBACK_USER_CHANGED_NICK, (void*)channel->callbacks.userChangedNick, &params, channel->callbacks.param, 0, channel->name);
			}
		}
	}
	GSI_UNUSED(rcode);
}

void ciUserChangedNick(CHAT chat, const char * oldNick, const char * newNick)
{
	ciUserChangedNickData data;
	CONNECTION;

	ASSERT_USER(oldNick);
	ASSERT_USER(newNick);

	data.chat = chat;
	data.oldNick = oldNick;
	data.newNick = newNick;

	// Enum through channels looking for this user.
	///////////////////////////////////////////////
	TableMap(connection->channelTable, ciUserChangeNickMap, &data);
}

void ciUserChangedMode(CHAT chat, const char * user, const char * channel, int mode, CHATBool enabled)
{
	ciChatChannel channelTemp;
	ciChatChannel * chatChannel;
	ciChatUser userTemp;
	ciChatUser * chatUser;
	ciCallbackUserModeChangedParams params;
	CONNECTION;

	ASSERT_USER(user);
	ASSERT_STR(channel);
	ASSERT_MODE(mode);

	// Find the channel.
	////////////////////
	strzcpy(channelTemp.name, channel, MAX_CHANNEL);
	chatChannel = (ciChatChannel *)TableLookup(connection->channelTable, &channelTemp);
	if(chatChannel == NULL)
		return; //ERRCON

	// Find the user.
	/////////////////
	strzcpy(userTemp.name, user, MAX_NAME);
	chatUser = (ciChatUser *)TableLookup(chatChannel->users, &userTemp);
	if(chatUser == NULL)
		return; //ERRCON

	// Change the mode.
	///////////////////
	if(enabled)
		chatUser->mode |= mode;
	else
		chatUser->mode &= ~mode;

	// Add the callback.
	////////////////////
	if(chatChannel->callbacks.userModeChanged != NULL)
	{
		params.channel = (char *)channel;
		params.user = (char *)user;
		params.mode = chatUser->mode;
		ciAddCallback(chat, CALLBACK_USER_MODE_CHANGED, (void*)chatChannel->callbacks.userModeChanged, &params, chatChannel->callbacks.param, 0, channel);
	}
}

CHATBool ciUserInChannel(CHAT chat, const char * channel, const char * user)
{
	ciChatChannel * chatChannel;
	ciChatUser chatUser;
	CONNECTION;

	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return CHATFalse;

	strzcpy(chatUser.name, user, MAX_NAME);
	if(TableLookup(chatChannel->users, &chatUser) == NULL)
		return CHATFalse;

	return CHATTrue;
}

int ciGetUserMode(CHAT chat, const char * channel, const char * user)
{
	ciChatChannel * chatChannel;
	ciChatUser userTemp;
	ciChatUser * chatUser;
	CONNECTION;

	chatChannel = ciGetChannel(connection, channel);
	if(chatChannel == NULL)
		return -1; //ERRCON

	strzcpy(userTemp.name, user, MAX_NAME);
	chatUser = (ciChatUser *)TableLookup(chatChannel->users, &userTemp);
	if(chatUser == NULL)
		return -1; //ERRCON

	return chatUser->mode;
}


static void ciEnumJoinedChannelsMap(void * elem, void * clientData)
{
	ciEnumJoinedChannelsData * data;
	ciChatChannel * channel;
	assert(elem != NULL);
	assert(clientData != NULL);

	// Get the channel.
	///////////////////
	channel = (ciChatChannel *)elem;

	// Get the callback & param
	/////////////////////////////
	data = (ciEnumJoinedChannelsData *)clientData;
	assert(data->callback != NULL);

	// Call the callback.
	/////////////////////
#ifdef GSI_UNICODE
	{
		unsigned short* name_W = UTF8ToUCS2StringAlloc(channel->name);
		data->callback(data->chat, data->index++, name_W, data->param);
		gsifree(name_W);
	}
#else
	data->callback(data->chat, data->index++, channel->name, data->param);
#endif
}

// Enumerates the channels that we are joined to
//////////////////////////////////////////////////////
void ciEnumJoinedChannels(CHAT chat,
					  chatEnumJoinedChannelsCallback callback,
					  void * param)
{
	ciEnumJoinedChannelsData data;
	CONNECTION;
	CONNECTED;
	data.callback = callback;
	data.param = param;
	data.index = 0;
	data.chat = chat;

	TableMap(connection->channelTable,ciEnumJoinedChannelsMap,&data);
}

static void ciSetUserBasicInfoMap(void * elem, void * clientData)
{
	ciChatChannel * channel;
	ciChatUser * user;
	ciSetUserBasicInfoData * data;

	assert(elem != NULL);
	assert(clientData != NULL);

	// Get the data.
	////////////////
	data = (ciSetUserBasicInfoData *)clientData;

	// Get the channel.
	///////////////////
	channel = (ciChatChannel *)elem;
	assert(channel->users != NULL);

	// Check for the user.
	//////////////////////
	user = (ciChatUser *)TableLookup(channel->users, data->chatUser);
	if(user != NULL)
	{
		// Found it.
		////////////
		strzcpy(user->user, data->user, MAX_CACHED_USER);
		strzcpy(user->address, data->address, MAX_CACHED_ADDRESS);
		user->gotUserAndAddress = CHATTrue;

#ifdef GSI_UNICODE
		// Store a unicode version since we pass a raw pointer to the developer
		AsciiToUCS2String(user->user, user->userW);
		AsciiToUCS2String(user->address, user->addressW);
#endif
	}
}

void ciSetUserBasicInfo(CHAT chat, const char * nick, const char * user, const char * address)
{
	ciChatUser chatUser;
	ciSetUserBasicInfoData data;
	CONNECTION;

	ASSERT_USER(nick);

	strzcpy(chatUser.name, nick, MAX_NAME);
	data.chatUser = &chatUser;
	data.user = (char *)user;
	data.address = (char *)address;

	// Enum through channels looking for this user.
	///////////////////////////////////////////////
	TableMap(connection->channelTable, ciSetUserBasicInfoMap, &data);
}

static void ciGetUserBasicInfoMap(void * elem, void * clientData)
{
	ciChatChannel * channel;
	ciChatUser * user;
	ciGetUserBasicInfoData * data;

	assert(elem != NULL);
	assert(clientData != NULL);

	// Get the data.
	////////////////
	data = (ciGetUserBasicInfoData *)clientData;

	// Did we already find the user?
	// Keep looking if we don't have a real address yet.
	////////////////////////////////////////////////////
	if(data->found && (strcmp(data->address, "*") != 0))
		return;

	// Get the channel.
	///////////////////
	channel = (ciChatChannel *)elem;
	assert(channel->users != NULL);

	// Check for the user.
	//////////////////////
	user = (ciChatUser *)TableLookup(channel->users, data->chatUser);
	if(user != NULL)
	{
		if(user->gotUserAndAddress)
		{
			// Found it.
			////////////
			data->found = CHATTrue;
			data->user = user->user;
			data->address = user->address;
#ifdef GSI_UNICODE
			data->userW = user->userW;
			data->addressW = user->addressW;
#endif
		}
	}
}

CHATBool ciGetUserBasicInfoA(CHAT chat, const char * nick, const char ** user, const char ** address)
{
	ciChatUser chatUser;
	ciGetUserBasicInfoData data;
	CONNECTION;

	ASSERT_USER(nick);

	strzcpy(chatUser.name, nick, MAX_NAME);
	data.chatUser = &chatUser;
	data.found = CHATFalse;

	// Enum through channels looking for this user.
	///////////////////////////////////////////////
	TableMap(connection->channelTable, ciGetUserBasicInfoMap, &data);

	if(!data.found)
		return CHATFalse;

	if(user)
		*user = data.user;
	if(address)
		*address = data.address;

	return CHATTrue;
}

#ifdef GSI_UNICODE
// Because we're returning a pointer to SDK memory, we have to 
// duplicate the lookup to return the address of the unsigned short versions
CHATBool ciGetUserBasicInfoW(CHAT chat, const char * nick, const unsigned short ** user, const unsigned short ** address)
{
	ciChatUser chatUser;
	ciGetUserBasicInfoData data;
	CONNECTION;

	ASSERT_USER(nick);

	strzcpy(chatUser.name, nick, MAX_NAME);
	data.chatUser = &chatUser;
	data.found = CHATFalse;

	// Enum through channels looking for this user.
	///////////////////////////////////////////////
	TableMap(connection->channelTable, ciGetUserBasicInfoMap, &data);

	if(!data.found)
		return CHATFalse;

	if(user)
		*user = data.userW;
	if(address)
		*address = data.addressW;
	return CHATTrue;
}
#endif

static void ciClearAllUsersUsersMap(void * elem, void * clientData)
{
	ciClearAllUsersData * data;
	ciChatUser * user;
	ciChatChannel * channel;

	assert(elem != NULL);
	assert(clientData != NULL);

	// Get the user.
	////////////////
	user = (ciChatUser *)elem;

	// Get the data.
	////////////////
	data = (ciClearAllUsersData *)clientData;
	channel = data->channel;

	// Remove the user from the channel.
	////////////////////////////////////
	TableRemove(channel->users, user);
}

static void ciClearAllUsersChannelMap(void * elem, void * clientData)
{
	ciChatChannel * channel;
	ciClearAllUsersData data;
	CHAT chat;

	assert(elem != NULL);
	assert(clientData != NULL);

	// Get the channel.
	///////////////////
	channel = (ciChatChannel *)elem;
	assert(channel->users != NULL);

	// Get the chat object.
	///////////////////////
	chat = (CHAT)clientData;

	// Setup the data.
	//////////////////
	data.chat = chat;
	data.channel = channel;

	// Remove all the users in this channel.
	////////////////////////////////////////
	TableMapSafe(channel->users, ciClearAllUsersUsersMap, &data);

	// Call the user list updated callback.
	///////////////////////////////////////
	if(channel->callbacks.userListUpdated != NULL)
	{
		ciCallbackUserListUpdatedParams params;
		params.channel = channel->name;
		ciAddCallback(chat, CALLBACK_USER_LIST_UPDATED, (void*)channel->callbacks.userListUpdated, &params, channel->callbacks.param, 0, channel->name);
	}
}

void ciClearAllUsers(CHAT chat)
{
	CONNECTION;

	// Go through all the channels.
	///////////////////////////////
	TableMap(connection->channelTable, ciClearAllUsersChannelMap, chat);
}
