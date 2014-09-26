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
#include "chatMain.h"
#include "chatCallbacks.h"
#include "chatChannel.h"

#if defined(_WIN32)
// warning about casting void* to function*
#pragma warning(disable:4055)
#endif

/************
** DEFINES **
************/
#define ASSERT_DATA(data)   assert(data != NULL); assert(data->type >= 0); assert(data->type < CALLBACK_NUM); assert(data->callback != NULL); assert(data->callbackParams != NULL);  assert(data->ID >= 0);
#define RAW                 callbackParams->raw
#define REASON              callbackParams->reason
#define USER                callbackParams->user
#define MESSAGE             callbackParams->message
#define TYPE                callbackParams->type
#define CHANNEL             callbackParams->channel
#define KICKER              callbackParams->kicker
#define KICKEE              callbackParams->kickee
#define TOPIC               callbackParams->topic
#define MODE                callbackParams->mode
#define SUCCESS             callbackParams->success
#define INDEX               callbackParams->index
#define NUM_USERS           callbackParams->numUsers
#define NUM_CHANNELS        callbackParams->numChannels
#define CHANNELS            callbackParams->channels
#define TOPICS              callbackParams->topics
#define ENABLED             callbackParams->enabled
#define PASSWORD            callbackParams->password
#define USERS               callbackParams->users
#define MODES               callbackParams->modes
#define ADDRESS             callbackParams->address
#define WHY                 callbackParams->why
#define OLD_NICK            callbackParams->oldNick
#define NEW_NICK            callbackParams->newNick
#define NUM_BANS            callbackParams->numBans
#define BANS                callbackParams->bans
#define NICK                callbackParams->nick
#define NAME                callbackParams->name
#define NUM                 callbackParams->num
#define KEY                 callbackParams->key
#define KEYS                callbackParams->keys
#define VALUE               callbackParams->value
#define VALUES              callbackParams->values
#define RESULT              callbackParams->result
#define NUM_SUGGESTED_NICKS callbackParams->numSuggestedNicks
#define SUGGESTED_NICKS     callbackParams->suggestedNicks
#define COPY(param)         if(srcParams->param != NULL)\
							{\
								destParams->param = goastrdup(srcParams->param);\
								if(destParams->param == NULL)\
								{\
									gsifree(destParams);\
									gsifree(data.channel);\
					/*ERRCON*/		return CHATFalse;\
								}\
							} else { destParams->param = srcParams->param; } // remove dangling "if" danger
#define COPY_MODE()         if(srcParams->mode != NULL)\
							{\
								destParams->mode = (CHATChannelMode *)gsimalloc(sizeof(CHATChannelMode));\
								if(destParams->mode == NULL)\
								{\
									gsifree(destParams);\
									gsifree(data.channel);\
					/*ERRCON*/		return CHATFalse;\
								}\
								memcpy(destParams->mode, srcParams->mode, sizeof(CHATChannelMode));\
							} else {} // remove dangling "if" danger
#define COPY_STR_ARRAY(array, num)  assert(srcParams->num >= 0);\
									if(!srcParams->array)\
										destParams->array = NULL;\
									else\
									{\
										destParams->array = (char **)gsimalloc(sizeof(char *) * srcParams->num);\
										if(destParams->array == NULL)\
										{\
											gsifree(destParams);\
											gsifree(data.channel);\
							/*ERRCON*/		return CHATFalse;\
										}\
										for(i = 0 ; i < srcParams->num ; i++)\
										{\
											if(srcParams->array[i] == NULL)\
												destParams->array[i] = NULL;\
											else\
											{\
												destParams->array[i] = goastrdup(srcParams->array[i]);\
												if(destParams->array[i] == NULL)\
												{\
													for(i-- ; i >= 0 ; i--)\
														gsifree(destParams->array[i]);\
													gsifree(destParams->array);\
													gsifree(destParams);\
													gsifree(data.channel);\
													return CHATFalse;\
												}\
											}\
										}\
									}
#define COPY_INT_ARRAY(array, num)  assert(srcParams->num >= 0);\
                                    if(srcParams->num > 0)\
									{\
										assert(srcParams->array != NULL);\
										len = (int)(sizeof(int) * srcParams->num);\
										destParams->array = (int *)gsimalloc((unsigned int)len);\
										if(destParams->array == NULL)\
										{\
											gsifree(destParams);\
											gsifree(data.channel);\
							/*ERRCON*/		return CHATFalse;\
										}\
										memcpy(destParams->array, srcParams->array, (unsigned int)len);\
									} else {} // remove dangling "if" danger

#define FREE_STRING_ARRAY(array, num)	if (num > 0 && array != NULL)\
										{\
											int i = 0;\
											for (; i < num; i++)\
												gsifree(array[i]);\
											gsifree(array);\
										} else {} // remove dangling "if" danger
/**********
** TYPES **
**********/
typedef struct ciCallbackData
{
	int type;
	void * callback;
	void * callbackParams;
	void * param;
	int ID;
	char * channel;
} ciCallbackData;

/**************
** FUNCTIONS **
**************/
static void ciCallbacksArrayElementFreeFn(void * elem)
{
	ciCallbackData * data = (ciCallbackData *)elem;
	GS_ASSERT(data != NULL);
	if (data->channel)
		gsifree(data->channel);
}

static void ciFreeCallbackData(ciCallbackData * data)
{
	ASSERT_DATA(data);

	// Find which type of callback it is.
	/////////////////////////////////////
	switch(data->type)
	{
	case CALLBACK_RAW:
	{
		ciCallbackRawParams * callbackParams = (ciCallbackRawParams *)data->callbackParams;
		gsifree(RAW);
		break;
	}

	case CALLBACK_DISCONNECTED:
	{
		ciCallbackDisconnectedParams * callbackParams = (ciCallbackDisconnectedParams *)data->callbackParams;
		gsifree(REASON);
		break;
	}

	case CALLBACK_PRIVATE_MESSAGE:
	{
		ciCallbackPrivateMessageParams * callbackParams = (ciCallbackPrivateMessageParams *)data->callbackParams;
		gsifree(USER);
		gsifree(MESSAGE);
		break;
	}

	case CALLBACK_INVITED:
	{
		ciCallbackInvitedParams * callbackParams = (ciCallbackInvitedParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(USER);
		break;
	}

	case CALLBACK_CHANNEL_MESSAGE:
	{
		ciCallbackChannelMessageParams * callbackParams = (ciCallbackChannelMessageParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(USER);
		gsifree(MESSAGE);
		break;
	}

	case CALLBACK_KICKED:
	{
		ciCallbackKickedParams * callbackParams = (ciCallbackKickedParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(USER);
		gsifree(REASON);
		break;
	}

	case CALLBACK_USER_JOINED:
	{
		ciCallbackUserJoinedParams * callbackParams = (ciCallbackUserJoinedParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(USER);
		break;
	}

	case CALLBACK_USER_PARTED:
	{
		ciCallbackUserPartedParams * callbackParams = (ciCallbackUserPartedParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(USER);
		gsifree(REASON);
		gsifree(KICKER);
		break;
	}

	case CALLBACK_USER_CHANGED_NICK:
	{
		ciCallbackUserChangedNickParams * callbackParams = (ciCallbackUserChangedNickParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(OLD_NICK);
		gsifree(NEW_NICK);
		break;
	}

	case CALLBACK_TOPIC_CHANGED:
	{
		ciCallbackTopicChangedParams * callbackParams = (ciCallbackTopicChangedParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(TOPIC);
		break;
	}

	case CALLBACK_CHANNEL_MODE_CHANGED:
	{
		ciCallbackChannelModeChangedParams * callbackParams = (ciCallbackChannelModeChangedParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(MODE);
		break;
	}

	case CALLBACK_USER_MODE_CHANGED:
	{
		ciCallbackUserModeChangedParams * callbackParams = (ciCallbackUserModeChangedParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(USER);
		break;
	}

	case CALLBACK_USER_LIST_UPDATED:
	{
		ciCallbackUserListUpdatedParams * callbackParams = (ciCallbackUserListUpdatedParams *)data->callbackParams;
		gsifree(CHANNEL);
		break;
	}

	case CALLBACK_ENUM_CHANNELS_EACH:
	{
		ciCallbackEnumChannelsEachParams * callbackParams = (ciCallbackEnumChannelsEachParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(TOPIC);
		break;
	}

	case CALLBACK_ENUM_CHANNELS_ALL:
	{
		int i;
		ciCallbackEnumChannelsAllParams * callbackParams = (ciCallbackEnumChannelsAllParams *)data->callbackParams;
		for(i = 0 ; i < NUM_CHANNELS ; i++)
		{
			gsifree(CHANNELS[i]);
			gsifree(TOPICS[i]);
		}
		gsifree(CHANNELS);
		gsifree(TOPICS);
		gsifree(NUM_USERS);
		break;
	}

	case CALLBACK_ENTER_CHANNEL:
	{
		ciCallbackEnterChannelParams * callbackParams = (ciCallbackEnterChannelParams *)data->callbackParams;
		gsifree(CHANNEL);
		break;
	}

	case CALLBACK_GET_CHANNEL_TOPIC:
	{
		ciCallbackGetChannelTopicParams * callbackParams = (ciCallbackGetChannelTopicParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(TOPIC);
		break;
	}

	case CALLBACK_GET_CHANNEL_MODE:
	{
		ciCallbackGetChannelModeParams * callbackParams = (ciCallbackGetChannelModeParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(MODE);
		break;
	}

	case CALLBACK_GET_UDPRELAY:
	{
		ciCallbackGetUdpRelayParams * callbackParams = (ciCallbackGetUdpRelayParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(callbackParams->udpIp);
		break;
	}

	case CALLBACK_GET_CHANNEL_PASSWORD:
	{
		ciCallbackGetChannelPasswordParams * callbackParams = (ciCallbackGetChannelPasswordParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(PASSWORD);
		break;
	}

	case CALLBACK_ENUM_USERS:
	{
		int i;
		ciCallbackEnumUsersParams * callbackParams = (ciCallbackEnumUsersParams *)data->callbackParams;
		gsifree(CHANNEL);
		for(i = 0 ; i < NUM_USERS ; i++)
			gsifree(USERS[i]);
		gsifree(USERS);
		gsifree(MODES);
		break;
	}

	case CALLBACK_GET_USER_INFO:
	{
		int i;
		ciCallbackGetUserInfoParams * callbackParams = (ciCallbackGetUserInfoParams *)data->callbackParams;
		gsifree(NICK);
		gsifree(USER);
		gsifree(NAME);
		gsifree(ADDRESS);
		for(i = 0 ; i < NUM_CHANNELS ; i++)
			gsifree(CHANNELS[i]);
		gsifree(CHANNELS);
		break;
	}

	case CALLBACK_GET_BASIC_USER_INFO:
	{
		ciCallbackGetBasicUserInfoParams * callbackParams = (ciCallbackGetBasicUserInfoParams *)data->callbackParams;
		gsifree(NICK);
		gsifree(USER);
		gsifree(ADDRESS);
		break;
	}

	case CALLBACK_GET_CHANNEL_BASIC_USER_INFO:
	{
		ciCallbackGetChannelBasicUserInfoParams * callbackParams = (ciCallbackGetChannelBasicUserInfoParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(NICK);
		gsifree(USER);
		gsifree(ADDRESS);
		break;
	}

	case CALLBACK_GET_USER_MODE:
	{
		ciCallbackGetUserModeParams * callbackParams = (ciCallbackGetUserModeParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(USER);
		break;
	}

	case CALLBACK_ENUM_CHANNEL_BANS:
	{
		int i;
		ciCallbackEnumChannelBansParams * callbackParams = (ciCallbackEnumChannelBansParams *)data->callbackParams;
		gsifree(CHANNEL);
		for(i = 0 ; i < NUM_BANS ; i++)
			gsifree(BANS[i]);
		gsifree(BANS);
		break;
	}

	case CALLBACK_NICK_ERROR:
	{
		int i;
		ciCallbackNickErrorParams * callbackParams = (ciCallbackNickErrorParams *)data->callbackParams;
		gsifree(NICK);
		for(i = 0 ; i < NUM_SUGGESTED_NICKS; i++)
			gsifree(SUGGESTED_NICKS[i]);
		gsifree(SUGGESTED_NICKS);
		break;
	}

	case CALLBACK_CHANGE_NICK:
	{
		ciCallbackChangeNickParams * callbackParams = (ciCallbackChangeNickParams *)data->callbackParams;
		gsifree(OLD_NICK);
		gsifree(NEW_NICK);
		break;
	}

	case CALLBACK_NEW_USER_LIST:
	{
		int i;
		ciCallbackNewUserListParams * callbackParams = (ciCallbackNewUserListParams *)data->callbackParams;
		gsifree(CHANNEL);
		for(i = 0 ; i < NUM_USERS ; i++)
			gsifree(USERS[i]);
		gsifree(USERS);
		gsifree(MODES);
		break;
	}

	case CALLBACK_BROADCAST_KEY_CHANGED:
	{
		 ciCallbackBroadcastKeyChangedParams * callbackParams = (ciCallbackBroadcastKeyChangedParams *)data->callbackParams;
		 gsifree(CHANNEL);
		 gsifree(USER);
		 gsifree(KEY);
		 gsifree(VALUE);
		 break;
	}

	case CALLBACK_GET_GLOBAL_KEYS:
	{
		int i;
		ciCallbackGetGlobalKeysParams * callbackParams = (ciCallbackGetGlobalKeysParams *)data->callbackParams;
		gsifree(USER);
		for(i = 0 ; i < NUM ; i++)
		{
			gsifree(KEYS[i]);
			if(VALUES)
				gsifree(VALUES[i]);
		}
		gsifree(KEYS);
		gsifree(VALUES);
		break;
	}

	case CALLBACK_GET_CHANNEL_KEYS:
	{
		int i;
		ciCallbackGetChannelKeysParams * callbackParams = (ciCallbackGetChannelKeysParams *)data->callbackParams;
		gsifree(CHANNEL);
		gsifree(USER);
		for(i = 0 ; i < NUM ; i++)
		{
			gsifree(KEYS[i]);
			if(VALUES)
				gsifree(VALUES[i]);
		}
		gsifree(KEYS);
		gsifree(VALUES);
		break;
	}

	case CALLBACK_AUTHENTICATE_CDKEY:
	{
		ciCallbackAuthenticateCDKeyParams * callbackParams = (ciCallbackAuthenticateCDKeyParams *)data->callbackParams;
		gsifree(MESSAGE);
		break;
	}

	default:
		// The type for this callback is messed up.
		///////////////////////////////////////////
		assert(0);
	}

	// gsifree the params structure.
	/////////////////////////////
	gsifree(data->callbackParams);
}

CHATBool ciInitCallbacks(ciConnection * connection)
{
	// Setup the darray.
	////////////////////
	connection->callbackList = ArrayNew(sizeof(ciCallbackData), 128, ciCallbacksArrayElementFreeFn);
	if(connection->callbackList == NULL)
		return CHATFalse;

	return CHATTrue;
}

void ciCleanupCallbacks(CHAT chat)
{
	CONNECTION;

	// Cleanup.
	///////////
	if(connection->callbackList != NULL)
	{
		ciCallbackData * data;
		int len;
		int i;

		// Get the number of callbacks.
		///////////////////////////////
		len = ArrayLength(connection->callbackList);

		// gsifree the data.
		/////////////////
		for(i = 0 ; i < len ; i++)
		{
			data = (ciCallbackData *)ArrayNth(connection->callbackList, i);
			ASSERT_DATA(data);

			// gsifree the data.
			/////////////////
			ciFreeCallbackData(data);
		}

		// gsifree the list.
		/////////////////
		ArrayFree(connection->callbackList);
	}
}

CHATBool ciAddCallback_(CHAT chat, int type, void * callback, void * callbackParams, void * param, int ID, const char * channel, size_t callbackParamsSize)
{
	ciCallbackData data;
	int len;
	int i;
	CONNECTION;

	assert(type >= 0);
	assert(type < CALLBACK_NUM);
	assert(connection->callbackList != NULL);
	assert(callback != NULL);
	assert(callbackParams != NULL);
	assert(callbackParamsSize > 0);
	assert(ID >= 0);
#ifdef _DEBUG
	if(channel != NULL)
		assert(channel[0] != '\0');
#endif

	// Setup the data.
	//////////////////
	memset(&data, 0, sizeof(ciCallbackData));
	data.type = type;
	data.callback = callback;
#ifndef _DEBUG
	data.callbackParams = gsimalloc(callbackParamsSize);
#else
	data.callbackParams = gsimalloc(callbackParamsSize + 64);
	memset(data.callbackParams, 0xC4, callbackParamsSize + 64);
#endif
	if(data.callbackParams == NULL)
		return CHATFalse; //ERRCON
	memcpy(data.callbackParams, callbackParams, callbackParamsSize);
	data.param = param;
	data.ID = ID;
	if(channel == NULL)
		data.channel = NULL;
	else
	{
		len = (int)(strlen(channel) + 1);
		data.channel = (char *)gsimalloc((unsigned int)len);
		if(data.channel == NULL)
		{
			gsifree(data.callbackParams);
			return CHATFalse; //ERRCON
		}
		memcpy(data.channel, channel, (unsigned int)len);
	}

	// Find which type of callback it is.
	/////////////////////////////////////
	switch(data.type)
	{
	case CALLBACK_RAW:
	{
		ciCallbackRawParams * destParams = (ciCallbackRawParams *)data.callbackParams;
		ciCallbackRawParams * srcParams = (ciCallbackRawParams *)callbackParams;
		COPY(raw);
		break;
	}

	case CALLBACK_DISCONNECTED:
	{
		ciCallbackDisconnectedParams * destParams = (ciCallbackDisconnectedParams *)data.callbackParams;
		ciCallbackDisconnectedParams * srcParams = (ciCallbackDisconnectedParams *)callbackParams;
		COPY(reason);
		break;
	}

	case CALLBACK_PRIVATE_MESSAGE:
	{
		ciCallbackPrivateMessageParams * destParams = (ciCallbackPrivateMessageParams *)data.callbackParams;
		ciCallbackPrivateMessageParams * srcParams = (ciCallbackPrivateMessageParams *)callbackParams;
		COPY(user);
		COPY(message);
		break;
	}

	case CALLBACK_INVITED:
	{
		ciCallbackInvitedParams * destParams = (ciCallbackInvitedParams *)data.callbackParams;
		ciCallbackInvitedParams * srcParams = (ciCallbackInvitedParams *)callbackParams;
		COPY(channel);
		COPY(user);
		break;
	}

	case CALLBACK_CHANNEL_MESSAGE:
	{
		ciCallbackChannelMessageParams * destParams = (ciCallbackChannelMessageParams *)data.callbackParams;
		ciCallbackChannelMessageParams * srcParams = (ciCallbackChannelMessageParams *)callbackParams;
		COPY(channel);
		COPY(user);
		COPY(message);
		break;
	}

	case CALLBACK_KICKED:
	{
		ciCallbackKickedParams * destParams = (ciCallbackKickedParams *)data.callbackParams;
		ciCallbackKickedParams * srcParams = (ciCallbackKickedParams *)callbackParams;
		COPY(channel);
		COPY(user);
		COPY(reason);
		break;
	}

	case CALLBACK_USER_JOINED:
	{
		ciCallbackUserJoinedParams * destParams = (ciCallbackUserJoinedParams *)data.callbackParams;
		ciCallbackUserJoinedParams * srcParams = (ciCallbackUserJoinedParams *)callbackParams;
		COPY(channel);
		COPY(user);
		break;
	}

	case CALLBACK_USER_PARTED:
	{
		ciCallbackUserPartedParams * destParams = (ciCallbackUserPartedParams *)data.callbackParams;
		ciCallbackUserPartedParams * srcParams = (ciCallbackUserPartedParams *)callbackParams;
		COPY(channel);
		COPY(user);
		COPY(reason);
		COPY(kicker);
		break;
	}

	case CALLBACK_USER_CHANGED_NICK:
	{
		ciCallbackUserChangedNickParams * destParams = (ciCallbackUserChangedNickParams *)data.callbackParams;
		ciCallbackUserChangedNickParams * srcParams = (ciCallbackUserChangedNickParams *)callbackParams;
		COPY(channel);
		COPY(oldNick);
		COPY(newNick);
		break;
	}

	case CALLBACK_TOPIC_CHANGED:
	{
		ciCallbackTopicChangedParams * destParams = (ciCallbackTopicChangedParams *)data.callbackParams;
		ciCallbackTopicChangedParams * srcParams = (ciCallbackTopicChangedParams *)callbackParams;
		COPY(channel);
		COPY(topic);
		break;
	}

	case CALLBACK_CHANNEL_MODE_CHANGED:
	{
		ciCallbackChannelModeChangedParams * destParams = (ciCallbackChannelModeChangedParams *)data.callbackParams;
		ciCallbackChannelModeChangedParams * srcParams = (ciCallbackChannelModeChangedParams *)callbackParams;
		COPY(channel);
		COPY_MODE();
		break;
	}

	case CALLBACK_USER_MODE_CHANGED:
	{
		ciCallbackUserModeChangedParams * destParams = (ciCallbackUserModeChangedParams *)data.callbackParams;
		ciCallbackUserModeChangedParams * srcParams = (ciCallbackUserModeChangedParams *)callbackParams;
		COPY(channel);
		COPY(user);
		break;
	}

	case CALLBACK_USER_LIST_UPDATED:
	{
		ciCallbackUserListUpdatedParams * destParams = (ciCallbackUserListUpdatedParams *)data.callbackParams;
		ciCallbackUserListUpdatedParams * srcParams = (ciCallbackUserListUpdatedParams *)callbackParams;
		COPY(channel);
		break;
	}

	case CALLBACK_ENUM_CHANNELS_EACH:
	{
		ciCallbackEnumChannelsEachParams * destParams = (ciCallbackEnumChannelsEachParams *)data.callbackParams;
		ciCallbackEnumChannelsEachParams * srcParams = (ciCallbackEnumChannelsEachParams *)callbackParams;
		COPY(channel);
		COPY(topic);
		break;
	}

	case CALLBACK_ENUM_CHANNELS_ALL:
	{
		ciCallbackEnumChannelsAllParams * destParams = (ciCallbackEnumChannelsAllParams *)data.callbackParams;
		ciCallbackEnumChannelsAllParams * srcParams = (ciCallbackEnumChannelsAllParams *)callbackParams;
		COPY_STR_ARRAY(channels, numChannels);
		COPY_STR_ARRAY(topics, numChannels);
		COPY_INT_ARRAY(numUsers, numChannels);
		break;
	}

	case CALLBACK_ENTER_CHANNEL:
	{
		ciCallbackEnterChannelParams * destParams = (ciCallbackEnterChannelParams *)data.callbackParams;
		ciCallbackEnterChannelParams * srcParams = (ciCallbackEnterChannelParams *)callbackParams;
		COPY(channel);
		break;
	}

	case CALLBACK_GET_CHANNEL_TOPIC:
	{
		ciCallbackGetChannelTopicParams * destParams = (ciCallbackGetChannelTopicParams *)data.callbackParams;
		ciCallbackGetChannelTopicParams * srcParams = (ciCallbackGetChannelTopicParams *)callbackParams;
		COPY(channel);
		COPY(topic);
		break;
	}

	case CALLBACK_GET_CHANNEL_MODE:
	{
		ciCallbackGetChannelModeParams * destParams = (ciCallbackGetChannelModeParams *)data.callbackParams;
		ciCallbackGetChannelModeParams * srcParams = (ciCallbackGetChannelModeParams *)callbackParams;
		COPY(channel);
		COPY_MODE();
		break;
	}

	case CALLBACK_GET_UDPRELAY:
	{
		ciCallbackGetUdpRelayParams * destParams = (ciCallbackGetUdpRelayParams *)data.callbackParams;
		ciCallbackGetUdpRelayParams * srcParams = (ciCallbackGetUdpRelayParams *)callbackParams;
		COPY(channel);
		COPY(udpIp);
		break;
	}

	case CALLBACK_GET_CHANNEL_PASSWORD:
	{
		ciCallbackGetChannelPasswordParams * destParams = (ciCallbackGetChannelPasswordParams *)data.callbackParams;
		ciCallbackGetChannelPasswordParams * srcParams = (ciCallbackGetChannelPasswordParams *)callbackParams;
		COPY(channel);
		COPY(password);
		break;
	}

	case CALLBACK_ENUM_USERS:
	{
		ciCallbackEnumUsersParams * destParams = (ciCallbackEnumUsersParams *)data.callbackParams;
		ciCallbackEnumUsersParams * srcParams = (ciCallbackEnumUsersParams *)callbackParams;
		COPY(channel);
		COPY_STR_ARRAY(users, numUsers);
		COPY_INT_ARRAY(modes, numUsers);
		break;
	}

	case CALLBACK_GET_USER_INFO:
	{
		ciCallbackGetUserInfoParams * destParams = (ciCallbackGetUserInfoParams *)data.callbackParams;
		ciCallbackGetUserInfoParams * srcParams = (ciCallbackGetUserInfoParams *)callbackParams;
		COPY(nick);
		COPY(user);
		COPY(name);
		COPY(address);
		COPY_STR_ARRAY(channels, numChannels);
		break;
	}

	case CALLBACK_GET_BASIC_USER_INFO:
	{
		ciCallbackGetBasicUserInfoParams * destParams = (ciCallbackGetBasicUserInfoParams *)data.callbackParams;
		ciCallbackGetBasicUserInfoParams * srcParams = (ciCallbackGetBasicUserInfoParams *)callbackParams;
		COPY(nick);
		COPY(user);
		COPY(address);
		break;
	}

	case CALLBACK_GET_CHANNEL_BASIC_USER_INFO:
	{
		ciCallbackGetChannelBasicUserInfoParams * destParams = (ciCallbackGetChannelBasicUserInfoParams *)data.callbackParams;
		ciCallbackGetChannelBasicUserInfoParams * srcParams = (ciCallbackGetChannelBasicUserInfoParams *)callbackParams;
		COPY(channel);
		COPY(nick);
		COPY(user);
		COPY(address);
		break;
	}

	case CALLBACK_GET_USER_MODE:
	{
		ciCallbackGetUserModeParams * destParams = (ciCallbackGetUserModeParams *)data.callbackParams;
		ciCallbackGetUserModeParams * srcParams = (ciCallbackGetUserModeParams *)callbackParams;
		COPY(channel);
		COPY(user);
		break;
	}

	case CALLBACK_ENUM_CHANNEL_BANS:
	{
		ciCallbackEnumChannelBansParams * destParams = (ciCallbackEnumChannelBansParams *)data.callbackParams;
		ciCallbackEnumChannelBansParams * srcParams = (ciCallbackEnumChannelBansParams *)callbackParams;
		COPY(channel);
		COPY_STR_ARRAY(bans, numBans);
		break;
	}

	case CALLBACK_NICK_ERROR:
	{
		ciCallbackNickErrorParams * destParams = (ciCallbackNickErrorParams *)data.callbackParams;
		ciCallbackNickErrorParams * srcParams = (ciCallbackNickErrorParams *)callbackParams;
		COPY(nick);
		COPY_STR_ARRAY(suggestedNicks, numSuggestedNicks);
		break;
	}

	case CALLBACK_CHANGE_NICK:
	{
		ciCallbackChangeNickParams * destParams = (ciCallbackChangeNickParams *)data.callbackParams;
		ciCallbackChangeNickParams * srcParams = (ciCallbackChangeNickParams *)callbackParams;
		COPY(oldNick);
		COPY(newNick);
		break;
	}

	case CALLBACK_NEW_USER_LIST:
	{
		ciCallbackNewUserListParams * destParams = (ciCallbackNewUserListParams *)data.callbackParams;
		ciCallbackNewUserListParams * srcParams = (ciCallbackNewUserListParams *)callbackParams;
		COPY(channel);
		COPY_STR_ARRAY(users, numUsers);
		COPY_INT_ARRAY(modes, numUsers);
		break;
	}

	case CALLBACK_BROADCAST_KEY_CHANGED:
	{
		ciCallbackBroadcastKeyChangedParams * destParams = (ciCallbackBroadcastKeyChangedParams *)data.callbackParams;
		ciCallbackBroadcastKeyChangedParams * srcParams = (ciCallbackBroadcastKeyChangedParams *)callbackParams;
		COPY(channel);
		COPY(user);
		COPY(key);
		COPY(value);
		break;
	}

	case CALLBACK_GET_GLOBAL_KEYS:
	{
		ciCallbackGetGlobalKeysParams * destParams = (ciCallbackGetGlobalKeysParams *)data.callbackParams;
		ciCallbackGetGlobalKeysParams * srcParams = (ciCallbackGetGlobalKeysParams *)callbackParams;
		COPY(user);
		COPY_STR_ARRAY(keys, num);
		COPY_STR_ARRAY(values, num);
		break;
	}

	case CALLBACK_GET_CHANNEL_KEYS:
	{
		ciCallbackGetChannelKeysParams * destParams = (ciCallbackGetChannelKeysParams *)data.callbackParams;
		ciCallbackGetChannelKeysParams * srcParams = (ciCallbackGetChannelKeysParams *)callbackParams;
		COPY(channel);
		COPY(user);
		COPY_STR_ARRAY(keys, num);
		COPY_STR_ARRAY(values, num);
		break;
	}

	case CALLBACK_AUTHENTICATE_CDKEY:
	{
		ciCallbackAuthenticateCDKeyParams * destParams = (ciCallbackAuthenticateCDKeyParams *)data.callbackParams;
		ciCallbackAuthenticateCDKeyParams * srcParams = (ciCallbackAuthenticateCDKeyParams *)callbackParams;
		COPY(message);
		break;
	}

	default:
		// The type for this callback is messed up.
		///////////////////////////////////////////
		assert(0);
	}

	// Add it to the array.
	///////////////////////
	ArrayAppend(connection->callbackList, &data);

	return CHATTrue;
}

static void ciCallCallback(CHAT chat, ciCallbackData * data)
{
	void * param;
	//CONNECTION;

	ASSERT_DATA(data);

	// Cache the param.
	///////////////////
	param = data->param;

	// Find which type of callback it is.
	/////////////////////////////////////
	switch(data->type)
	{
	case CALLBACK_RAW:
	{
		ciCallbackRawParams * callbackParams = (ciCallbackRawParams *)data->callbackParams;
		chatRaw callback = (chatRaw)data->callback;
#ifndef GSI_UNICODE
		callback(chat, RAW, param);
		break;
#else
		// UNICODE: Copy all the params, call the callback, free the params
		unsigned short* raw_W = UTF8ToUCS2StringAlloc(RAW);
		callback(chat, raw_W, param);
		gsifree(raw_W);
		break;
#endif
	}

	case CALLBACK_DISCONNECTED:
	{
		ciCallbackDisconnectedParams * callbackParams = (ciCallbackDisconnectedParams *)data->callbackParams;
		chatDisconnected callback = (chatDisconnected)data->callback;
#ifndef GSI_UNICODE
		callback(chat, REASON, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* reason_W = UTF8ToUCS2StringAlloc(REASON);
		callback(chat, reason_W, param);
		gsifree(reason_W);
		break;
#endif
	}

	case CALLBACK_PRIVATE_MESSAGE:
	{
		ciCallbackPrivateMessageParams * callbackParams = (ciCallbackPrivateMessageParams *)data->callbackParams;
		chatPrivateMessage callback = (chatPrivateMessage)data->callback;
#ifndef GSI_UNICODE
		callback(chat, USER, MESSAGE, TYPE, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* user_W		= UTF8ToUCS2StringAlloc(USER);
		unsigned short* message_W	= UTF8ToUCS2StringAlloc(MESSAGE);
		callback(chat, user_W, message_W, TYPE, param);
		gsifree(user_W);
		gsifree(message_W);
		break;
#endif
	}

	case CALLBACK_INVITED:
	{
		ciCallbackInvitedParams * callbackParams = (ciCallbackInvitedParams *)data->callbackParams;
		chatInvited callback = (chatInvited)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, USER, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W = UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* user_W = UTF8ToUCS2StringAlloc(USER);
		callback(chat, channel_W, user_W, param);
		gsifree(channel_W);
		gsifree(user_W);
		break;
#endif
	}

	case CALLBACK_CHANNEL_MESSAGE:
	{
		ciCallbackChannelMessageParams * callbackParams = (ciCallbackChannelMessageParams *)data->callbackParams;
		chatChannelMessage callback = (chatChannelMessage)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, USER, MESSAGE, TYPE, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* user_W		= UTF8ToUCS2StringAlloc(USER);
		unsigned short* message_W	= UTF8ToUCS2StringAlloc(MESSAGE);
		callback(chat, channel_W, user_W, message_W, TYPE, param);
		gsifree(channel_W);
		gsifree(user_W);
		gsifree(message_W);
		break;
#endif
	}

	case CALLBACK_KICKED:
	{
		ciCallbackKickedParams * callbackParams = (ciCallbackKickedParams *)data->callbackParams;
		chatKicked callback = (chatKicked)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, USER, REASON, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* user_W		= UTF8ToUCS2StringAlloc(USER);
		unsigned short* reason_W	= UTF8ToUCS2StringAlloc(REASON);
		callback(chat, channel_W, user_W, reason_W, param);
		gsifree(channel_W);
		gsifree(user_W);
		gsifree(reason_W);
		break;
#endif
	}

	case CALLBACK_USER_JOINED:
	{
		ciCallbackUserJoinedParams * callbackParams = (ciCallbackUserJoinedParams *)data->callbackParams;
		chatUserJoined callback = (chatUserJoined)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, USER, MODE, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* user_W		= UTF8ToUCS2StringAlloc(USER);
		callback(chat, channel_W, user_W, MODE, param);
		gsifree(channel_W);
		gsifree(user_W);
		break;
#endif

	}

	case CALLBACK_USER_PARTED:
	{
		ciCallbackUserPartedParams * callbackParams = (ciCallbackUserPartedParams *)data->callbackParams;
		chatUserParted callback = (chatUserParted)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, USER, WHY, REASON, KICKER, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* user_W		= UTF8ToUCS2StringAlloc(USER);
		unsigned short* reason_W	= UTF8ToUCS2StringAlloc(REASON);
		unsigned short* kicker_W	= UTF8ToUCS2StringAlloc(KICKER);
		callback(chat, channel_W, user_W, WHY, reason_W, kicker_W, param);
		gsifree(channel_W);
		gsifree(user_W);
		gsifree(reason_W);
		gsifree(kicker_W);
		break;
#endif
	}

	case CALLBACK_USER_CHANGED_NICK:
	{
		ciCallbackUserChangedNickParams * callbackParams = (ciCallbackUserChangedNickParams *)data->callbackParams;
		chatUserChangedNick callback = (chatUserChangedNick)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, OLD_NICK, NEW_NICK, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* oldnick_W	= UTF8ToUCS2StringAlloc(OLD_NICK);
		unsigned short* newnick_W	= UTF8ToUCS2StringAlloc(NEW_NICK);
		callback(chat, channel_W, oldnick_W, newnick_W, param);
		gsifree(channel_W);
		gsifree(oldnick_W);
		gsifree(newnick_W);
		break;
#endif
	}

	case CALLBACK_TOPIC_CHANGED:
	{
		ciCallbackTopicChangedParams * callbackParams = (ciCallbackTopicChangedParams *)data->callbackParams;
		chatTopicChanged callback = (chatTopicChanged)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, TOPIC, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* topic_W		= UTF8ToUCS2StringAlloc(TOPIC);
		callback(chat, channel_W, topic_W, param);
		gsifree(channel_W);
		gsifree(topic_W);
		break;
#endif
	}

	case CALLBACK_CHANNEL_MODE_CHANGED:
	{
		ciCallbackChannelModeChangedParams * callbackParams = (ciCallbackChannelModeChangedParams *)data->callbackParams;
		chatChannelModeChanged callback = (chatChannelModeChanged)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, MODE, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		callback(chat, channel_W, MODE, param);
		gsifree(channel_W);
		break;
#endif
	}

	case CALLBACK_USER_MODE_CHANGED:
	{
		ciCallbackUserModeChangedParams * callbackParams = (ciCallbackUserModeChangedParams *)data->callbackParams;
		chatUserModeChanged callback = (chatUserModeChanged)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, USER, MODE, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* user_W		= UTF8ToUCS2StringAlloc(USER);
		callback(chat, channel_W, user_W, MODE, param);
		gsifree(channel_W);
		gsifree(user_W);
		break;
#endif
	}

	case CALLBACK_USER_LIST_UPDATED:
	{
		ciCallbackUserListUpdatedParams * callbackParams = (ciCallbackUserListUpdatedParams *)data->callbackParams;
		chatUserListUpdated callback = (chatUserListUpdated)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W = UTF8ToUCS2StringAlloc(CHANNEL);
		callback(chat, channel_W, param);
		gsifree(channel_W);
		break;
#endif
	}

	case CALLBACK_ENUM_CHANNELS_EACH:
	{
		ciCallbackEnumChannelsEachParams * callbackParams = (ciCallbackEnumChannelsEachParams *)data->callbackParams;
		chatEnumChannelsCallbackEach callback = (chatEnumChannelsCallbackEach)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, INDEX, CHANNEL, TOPIC, NUM_USERS, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W = UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* topic_W = UTF8ToUCS2StringAlloc(TOPIC);
		callback(chat, SUCCESS, INDEX, channel_W, topic_W, NUM_USERS, param);
		gsifree(channel_W);
		gsifree(topic_W);
		break;
#endif
	}

	case CALLBACK_ENUM_CHANNELS_ALL:
	{
		ciCallbackEnumChannelsAllParams * callbackParams = (ciCallbackEnumChannelsAllParams *)data->callbackParams;
		chatEnumChannelsCallbackAll callback = (chatEnumChannelsCallbackAll)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, NUM_CHANNELS, (const char **)CHANNELS, (const char **)TOPICS, NUM_USERS, param);
		break;
#else
		unsigned short** channels_W = UTF8ToUCS2StringArrayAlloc((const char **)CHANNELS, NUM_CHANNELS);
		unsigned short** topics_W	= UTF8ToUCS2StringArrayAlloc((const char **)TOPICS, NUM_CHANNELS);
		callback(chat, SUCCESS, NUM_CHANNELS, (const unsigned short**)channels_W, (const unsigned short**)topics_W, NUM_USERS, param);
		FREE_STRING_ARRAY(channels_W, NUM_CHANNELS);
		FREE_STRING_ARRAY(topics_W, NUM_CHANNELS);
		break;
#endif
	}

	case CALLBACK_ENTER_CHANNEL:
	{
		ciCallbackEnterChannelParams * callbackParams = (ciCallbackEnterChannelParams *)data->callbackParams;
		chatEnterChannelCallback callback = (chatEnterChannelCallback)data->callback;

		// Call this before the callback so funcs called within the callback know.
		//////////////////////////////////////////////////////////////////////////
		ciJoinCallbackCalled(chat, CHANNEL);

#ifndef GSI_UNICODE
		callback(chat, SUCCESS, RESULT, CHANNEL, param);
		break;
#else
		{
			// Copy all the params, call the callback, free the params
			unsigned short* channel_W = UTF8ToUCS2StringAlloc(CHANNEL);
			callback(chat, SUCCESS, RESULT, channel_W, param);
			gsifree(channel_W);
			break;
		}
#endif
	}

	case CALLBACK_GET_CHANNEL_TOPIC:
	{
		ciCallbackGetChannelTopicParams * callbackParams = (ciCallbackGetChannelTopicParams *)data->callbackParams;
		chatGetChannelTopicCallback callback = (chatGetChannelTopicCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, CHANNEL, TOPIC, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W = UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* topic_W = UTF8ToUCS2StringAlloc(TOPIC);
		callback(chat, SUCCESS, channel_W, topic_W, param);
		gsifree(channel_W);
		gsifree(topic_W);
		break;
#endif
	}

	case CALLBACK_GET_CHANNEL_MODE:
	{
		ciCallbackGetChannelModeParams * callbackParams = (ciCallbackGetChannelModeParams *)data->callbackParams;
		chatGetChannelModeCallback callback = (chatGetChannelModeCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, CHANNEL, MODE, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W = UTF8ToUCS2StringAlloc(CHANNEL);
		callback(chat, SUCCESS, channel_W, MODE, param);
		gsifree(channel_W);
		break;
#endif
	}

	case CALLBACK_GET_UDPRELAY:
	{
		ciCallbackGetUdpRelayParams * callbackParams = (ciCallbackGetUdpRelayParams *)data->callbackParams;
		chatGetUdpRelayCallback callback = (chatGetUdpRelayCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, callbackParams->udpIp, callbackParams->udpPort, callbackParams->udpKey, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W = UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* udpIp_W = UTF8ToUCS2StringAlloc(callbackParams->udpIp);
		callback(chat, channel_W, udpIp_W, callbackParams->udpPort, callbackParams->udpKey, param);
		gsifree(channel_W);
		gsifree(udpIp_W);
		break;
#endif
	}

	case CALLBACK_GET_CHANNEL_PASSWORD:
	{
		ciCallbackGetChannelPasswordParams * callbackParams = (ciCallbackGetChannelPasswordParams *)data->callbackParams;
		chatGetChannelPasswordCallback callback = (chatGetChannelPasswordCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, CHANNEL, ENABLED, PASSWORD, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* password_W	= UTF8ToUCS2StringAlloc(PASSWORD);
		callback(chat, SUCCESS, channel_W, ENABLED, password_W, param);
		gsifree(channel_W);
		gsifree(password_W);
		break;
#endif
	}

	case CALLBACK_ENUM_USERS:
	{
		ciCallbackEnumUsersParams * callbackParams = (ciCallbackEnumUsersParams *)data->callbackParams;
		chatEnumUsersCallback callback = (chatEnumUsersCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, CHANNEL, NUM_USERS, (const char **)USERS, MODES, param);
		break;
#else
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short** users_W	= UTF8ToUCS2StringArrayAlloc((const char **)USERS, NUM_USERS);
		callback(chat, SUCCESS, channel_W, NUM_USERS, (const unsigned short**)users_W, MODES, param);
		gsifree(channel_W);
		FREE_STRING_ARRAY(users_W, NUM_USERS);
		break;
#endif
	}

	case CALLBACK_GET_USER_INFO:
	{
		ciCallbackGetUserInfoParams * callbackParams = (ciCallbackGetUserInfoParams *)data->callbackParams;
		chatGetUserInfoCallback callback = (chatGetUserInfoCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, NICK, USER, NAME, ADDRESS, NUM_CHANNELS, (const char **)CHANNELS, param);
		break;
#else
		unsigned short* nick_W	= UTF8ToUCS2StringAlloc(NICK);
		unsigned short* user_W	= UTF8ToUCS2StringAlloc(USER);
		unsigned short* name_W	= UTF8ToUCS2StringAlloc(NAME);
		unsigned short* address_W	= UTF8ToUCS2StringAlloc(ADDRESS);
		unsigned short** channels_W	= UTF8ToUCS2StringArrayAlloc((const char **)CHANNELS, NUM_CHANNELS);
		callback(chat, SUCCESS, nick_W, user_W, name_W, address_W, NUM_CHANNELS, (const unsigned short**)channels_W, param);
		gsifree(nick_W);
		gsifree(user_W);
		gsifree(name_W);
		gsifree(address_W);
		FREE_STRING_ARRAY(channels_W, NUM_CHANNELS);
		break;
#endif
	}

	case CALLBACK_GET_BASIC_USER_INFO:
	{
		ciCallbackGetBasicUserInfoParams * callbackParams = (ciCallbackGetBasicUserInfoParams *)data->callbackParams;
		chatGetBasicUserInfoCallback callback = (chatGetBasicUserInfoCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, NICK, USER, ADDRESS, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* nick_W		= UTF8ToUCS2StringAlloc(NICK);
		unsigned short* user_W		= UTF8ToUCS2StringAlloc(USER);
		unsigned short* address_W	= UTF8ToUCS2StringAlloc(ADDRESS);
		callback(chat, SUCCESS, nick_W, user_W, address_W, param);
		gsifree(nick_W);
		gsifree(user_W);
		gsifree(address_W);
		break;
#endif
	}

	case CALLBACK_GET_CHANNEL_BASIC_USER_INFO:
	{
		ciCallbackGetChannelBasicUserInfoParams * callbackParams = (ciCallbackGetChannelBasicUserInfoParams *)data->callbackParams;
		chatGetChannelBasicUserInfoCallback callback = (chatGetChannelBasicUserInfoCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, CHANNEL, NICK, USER, ADDRESS, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* nick_W		= UTF8ToUCS2StringAlloc(NICK);
		unsigned short* user_W		= UTF8ToUCS2StringAlloc(USER);
		unsigned short* address_W	= UTF8ToUCS2StringAlloc(ADDRESS);
		callback(chat, SUCCESS, channel_W, nick_W, user_W, address_W, param);
		gsifree(channel_W);
		gsifree(nick_W);
		gsifree(user_W);
		gsifree(address_W);
		break;
#endif
	}

	case CALLBACK_GET_USER_MODE:
	{
		ciCallbackGetUserModeParams * callbackParams = (ciCallbackGetUserModeParams *)data->callbackParams;
		chatGetUserModeCallback callback = (chatGetUserModeCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, CHANNEL, USER, MODE, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* user_W		= UTF8ToUCS2StringAlloc(USER);
		callback(chat, SUCCESS, channel_W, user_W, MODE, param);
		gsifree(channel_W);
		gsifree(user_W);
		break;
#endif
	}

	case CALLBACK_ENUM_CHANNEL_BANS:
	{
		ciCallbackEnumChannelBansParams * callbackParams = (ciCallbackEnumChannelBansParams *)data->callbackParams;
		chatEnumChannelBansCallback callback = (chatEnumChannelBansCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, CHANNEL, NUM_BANS, (const char **)BANS, param);
		break;
#else
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short** bans_W		= UTF8ToUCS2StringArrayAlloc((const char **)BANS, NUM_BANS);
		callback(chat, SUCCESS, channel_W, NUM_BANS, (const unsigned short**)bans_W, param);
		gsifree(channel_W);
		FREE_STRING_ARRAY(bans_W, NUM_BANS);
		break;
#endif
	}

	case CALLBACK_NICK_ERROR:
	{
		ciCallbackNickErrorParams * callbackParams = (ciCallbackNickErrorParams *)data->callbackParams;
		chatNickErrorCallback callback = (chatNickErrorCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, TYPE, NICK, NUM_SUGGESTED_NICKS, (const char**)SUGGESTED_NICKS, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* nick_W	= UTF8ToUCS2StringAlloc(NICK);
		unsigned short** suggestedNicks_W = UTF8ToUCS2StringArrayAlloc((const char **)SUGGESTED_NICKS, NUM_SUGGESTED_NICKS);
		callback(chat, TYPE, nick_W, NUM_SUGGESTED_NICKS, (const unsigned short**)suggestedNicks_W, param);
		gsifree(nick_W);
		FREE_STRING_ARRAY(suggestedNicks_W, NUM_SUGGESTED_NICKS);
		break;
#endif
	}

	case CALLBACK_CHANGE_NICK:
	{
		ciCallbackChangeNickParams * callbackParams = (ciCallbackChangeNickParams *)data->callbackParams;
		chatChangeNickCallback callback = (chatChangeNickCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, SUCCESS, OLD_NICK, NEW_NICK, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* oldnick_W	= UTF8ToUCS2StringAlloc(OLD_NICK);
		unsigned short* newnick_W	= UTF8ToUCS2StringAlloc(NEW_NICK);
		callback(chat, SUCCESS, oldnick_W, newnick_W, param);
		gsifree(oldnick_W);
		gsifree(newnick_W);
		break;
#endif
	}

	case CALLBACK_NEW_USER_LIST:
	{
		ciCallbackNewUserListParams * callbackParams = (ciCallbackNewUserListParams *)data->callbackParams;
		chatNewUserList callback = (chatNewUserList)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, NUM_USERS, (const char **)USERS, MODES, param);
		break;
#else
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short** users_W	= UTF8ToUCS2StringArrayAlloc((const char**)USERS, NUM_USERS);
		callback(chat, channel_W, NUM_USERS, (const unsigned short**)users_W, MODES, param);
		gsifree(channel_W);
		FREE_STRING_ARRAY(users_W, NUM_USERS);
		break;
#endif
	}

	case CALLBACK_BROADCAST_KEY_CHANGED:
	{
		ciCallbackBroadcastKeyChangedParams * callbackParams = (ciCallbackBroadcastKeyChangedParams *)data->callbackParams;
		chatBroadcastKeyChanged callback = (chatBroadcastKeyChanged)data->callback;
#ifndef GSI_UNICODE
		callback(chat, CHANNEL, USER, KEY, VALUE, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short* user_W		= UTF8ToUCS2StringAlloc(USER);
		unsigned short* key_W		= UTF8ToUCS2StringAlloc(KEY);
		unsigned short* value_W		= UTF8ToUCS2StringAlloc(VALUE);
		callback(chat, channel_W, user_W, key_W, value_W, param);
		gsifree(channel_W);
		gsifree(user_W);
		gsifree(key_W);
		gsifree(value_W);
		break;
#endif
	}

	case CALLBACK_GET_GLOBAL_KEYS:
	{
		 ciCallbackGetGlobalKeysParams * callbackParams = (ciCallbackGetGlobalKeysParams *)data->callbackParams;
		 chatGetGlobalKeysCallback callback = (chatGetGlobalKeysCallback)data->callback;
#ifndef GSI_UNICODE
		 callback(chat, SUCCESS, USER, NUM, (const char **)KEYS, (const char **)VALUES, param);
		 break;
#else
		unsigned short*  user_W		= UTF8ToUCS2StringAlloc(USER);
		unsigned short** keys_W		= UTF8ToUCS2StringArrayAlloc((const char**)KEYS, NUM);
		unsigned short** values_W	= UTF8ToUCS2StringArrayAlloc((const char**)VALUES, NUM);
		callback(chat, SUCCESS, user_W, NUM, (const unsigned short**)keys_W, (const unsigned short**)values_W, param);
		gsifree(user_W);
		FREE_STRING_ARRAY(keys_W, NUM);
		FREE_STRING_ARRAY(values_W, NUM);
		break;
#endif
	}

	case CALLBACK_GET_CHANNEL_KEYS:
	{
		 ciCallbackGetChannelKeysParams * callbackParams = (ciCallbackGetChannelKeysParams *)data->callbackParams;
		 chatGetChannelKeysCallback callback = (chatGetChannelKeysCallback)data->callback;
#ifndef GSI_UNICODE
		 callback(chat, SUCCESS, CHANNEL, USER, NUM, (const char **)KEYS, (const char **)VALUES, param);
		 break;
#else
		unsigned short*  channel_W	= UTF8ToUCS2StringAlloc(CHANNEL);
		unsigned short*  user_W		= UTF8ToUCS2StringAlloc(USER);
		unsigned short** keys_W		= UTF8ToUCS2StringArrayAlloc((const char**)KEYS, NUM);
		unsigned short** values_W	= UTF8ToUCS2StringArrayAlloc((const char**)VALUES, NUM);
		callback(chat, SUCCESS, channel_W, user_W, NUM, (const unsigned short**)keys_W, (const unsigned short**)values_W, param);
		gsifree(channel_W);
		gsifree(user_W);
		FREE_STRING_ARRAY(keys_W, NUM);
		FREE_STRING_ARRAY(values_W, NUM);
		break;
#endif
	}

	case CALLBACK_AUTHENTICATE_CDKEY:
	{
		ciCallbackAuthenticateCDKeyParams * callbackParams = (ciCallbackAuthenticateCDKeyParams *)data->callbackParams;
		chatAuthenticateCDKeyCallback callback = (chatAuthenticateCDKeyCallback)data->callback;
#ifndef GSI_UNICODE
		callback(chat, RESULT, MESSAGE, param);
		break;
#else
		// Copy all the params, call the callback, free the params
		unsigned short* message_W	= UTF8ToUCS2StringAlloc(MESSAGE);
		callback(chat, RESULT, message_W, param);
		gsifree(message_W);
		break;
#endif
	}

	default:
		// The type for this callback is messed up.
		///////////////////////////////////////////
		assert(0);
	}

	// gsifree the data.
	/////////////////
	ciFreeCallbackData(data);
}

void ciCallCallbacks(CHAT chat, int ID)
{
	ciCallbackData * data;
	ciCallbackData dataCopy;
	int skip;
	CONNECTION;

	// Call the callbacks.
	//////////////////////
	for(skip = 0 ; ArrayLength(connection->callbackList) > skip ; )
	{
		// Get the callback.
		///////////////////
		data = (ciCallbackData *)ArrayNth(connection->callbackList, skip);
		ASSERT_DATA(data);

		// Does this depend on a channel we're not in?
		//////////////////////////////////////////////
		if((data->channel != NULL) && !ciInChannel(chat, data->channel))
		{
			// gsifree the data.
			/////////////////
			ciFreeCallbackData(data);

			// Kill it.
			///////////
			ArrayDeleteAt(connection->callbackList, skip);
		}
		else
		{
			// Check if this callback depends on the join callback having been called.
			// Also, if blocking, only call that callback.
			//////////////////////////////////////////////////////////////////////////
			if(((data->channel == NULL) || ciWasJoinCallbackCalled(chat, data->channel)) &&
				((ID == 0) || (data->ID == ID)))
			{
				// Copy the data so we can gsifree it before calling the callback.
				///////////////////////////////////////////////////////////////
				dataCopy = *data;

				// gsifree it.
				///////////
				ArrayDeleteAt(connection->callbackList, skip);

				// Call the callback.
				/////////////////////
				ciCallCallback(chat, &dataCopy);

				// Was this the blocking callback?
				//////////////////////////////////
				if(ID != 0)
					return;
			}
			else
			{
				// Increment the skip, because it's still in the array.
				///////////////////////////////////////////////////////
				skip++;
			}
		}
	}
}

static int ciGetCallbackIndexByID(CHAT chat, int ID)
{
	ciCallbackData * data;
	int i;
	int len;
	CONNECTION;

	// Get the array length.
	////////////////////////
	len = ArrayLength(connection->callbackList);

	// Loop through the callbacks.
	//////////////////////////////
	for(i = 0 ; i < len ; i++)
	{
		// Get the callback.
		////////////////////
		data = (ciCallbackData *)ArrayNth(connection->callbackList, i);
		ASSERT_DATA(data);

		// Check for an ID match.
		/////////////////////////
		if(data->ID == ID)
			return i;
	}

	// Didn't find one.
	///////////////////
	return -1;
}

CHATBool ciCheckCallbacksForID(CHAT chat, int ID)
{
	if(ciGetCallbackIndexByID(chat, ID) == -1)
		return CHATFalse;

	return CHATTrue;
}
