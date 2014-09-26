// GameSpy Chat SDK C Test App
// Dan "Mr. Pants" Schoenblum
// dan@gamespy.com

/*************
** INCLUDES **
*************/
#include "../chat.h"
#include "../../common/gsStringUtil.h"

#ifdef UNDER_CE
	void RetailOutputA(CHAR *tszErr, ...);
	#define printf RetailOutputA
#elif defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

#define MAX_MESSAGE_SIZE    200
#define CHAT_NICK_SIZE      128

/************
** GLOBALS **
************/
// mj Nov 7th, zero out to known state globals.
int port					=	0;
CHAT chat					=	{0};
gsi_char serverAddress[128]	=	{0};
gsi_char chatNick[128]		=	{0};
gsi_char chatUser[128]		=	{0};
gsi_char chatName[128]		=	{0};
gsi_char chatChannel[128]	=	{0};
gsi_char gamename[128]		=	{0};
gsi_char secretKey[128]		=	{0};
CHATBool quit				=	CHATFalse;

#ifdef __MWERKS__	// CodeWarrior will warn if functions not prototyped
/***************
** PROTOTYPES **
***************/
int test_main(int argc, char **argv);
#endif

/**************
** FUNCTIONS **
**************/
#ifdef GSI_UNICODE
	#define _tstrcasecmp WideCaseCompare
	#define _tstrncasecmp WideCaseNCompare
#else
	#define _tstrcasecmp strcasecmp
	#define _tstrncasecmp strncasecmp
#endif

// Simulate case insensitive compare functions
#if defined(GSI_UNICODE)
int WideCaseCompare(const unsigned short* s1, const unsigned short* s2);
int WideCaseNCompare(const unsigned short* s1, const unsigned short* s2, size_t count);

int WideCaseCompare(const unsigned short* s1, const unsigned short* s2)
{
	char s1_A[512];
	char s2_A[512];
	UCS2ToAsciiString(s1, s1_A);
	UCS2ToAsciiString(s2, s2_A);
	return strcasecmp(s1_A, s2_A);
}

int WideCaseNCompare(const unsigned short* s1, const unsigned short* s2, size_t count)
{
	char s1_A[512];
	char s2_A[512];
	unsigned short temp[512];

	// null terminate
	temp[count+1] = 0; 

	// Copy to temp buffer, then convert to ascii
	memcpy(temp, s1, count * sizeof(unsigned short));
	UCS2ToAsciiString(temp, s1_A);

	memcpy(temp, s2, count * sizeof(unsigned short));
	UCS2ToAsciiString(temp, s2_A);

	return strncasecmp(s1_A, s2_A, count);
}

#endif


static void Raw(CHAT chat, const gsi_char * raw, void * param)
{
	_tprintf(_T("RAW: %s\n"), raw);
	
	GSI_UNUSED(chat);
	GSI_UNUSED(param);
}

static void Disconnected(CHAT chat, const gsi_char * reason, void * param)
{
	_tprintf(_T("Disconnected: %s\n"), reason);
	
	GSI_UNUSED(chat);
	GSI_UNUSED(param);
}

static void ChangedNickCallback(CHAT chat, CHATBool success, const gsi_char * oldNick, const gsi_char * newNick, void * param)
{
	if(success)
	{
		_tprintf(_T("Successfully changed"));
		_tcscpy(chatNick, newNick);
	}
	else
		_tprintf(_T("Failed to change"));
	_tprintf(_T(" nick from %s to %s\n"), oldNick, newNick);
	
	GSI_UNUSED(chat);
	GSI_UNUSED(param);
}

static void PrivateMessage(CHAT chat, const gsi_char * user, const gsi_char * message, int type, void * param)
{
	_tprintf(_T("Private message from %s: %s\n"), user, message);

	// Nick change?
	///////////////
	if(_tstrncasecmp(_T("nick"), message, 4) == 0)
	{
		chatChangeNick(chat, &message[5], ChangedNickCallback, NULL, CHATFalse);
	}
	
	GSI_UNUSED(type);
	GSI_UNUSED(param);
}

static void Invited(CHAT chat, const gsi_char * channel, const gsi_char * user, void * param)
{
	_tprintf(_T("Invited by %s to %s\n"), user, channel);
	
	GSI_UNUSED(chat);
	GSI_UNUSED(param);
}

static void ChannelMessage(CHAT chat, const gsi_char * channel, const gsi_char * user, const gsi_char * message, int type, void * param)
{
	gsi_char buffer[MAX_MESSAGE_SIZE];

	_tprintf(_T("%s, in %s, said \"%s\"\n"), user, channel, message);

	// Is this from us?
	///////////////////
	if(_tstrcasecmp(user, chatNick) == 0)
		return;

	// Is it a command?
	///////////////////
	if(message[0] == '!')
	{
		message++;
		if(!_tstrcasecmp(message, _T("quit")) || !_tstrcasecmp(message, _T("exit")))
			quit = CHATTrue;
		return;
	}
	_tsnprintf(buffer, MAX_MESSAGE_SIZE, _T("%s: I agree"), user);
	buffer[MAX_MESSAGE_SIZE - 1] = '\0';
	chatSendChannelMessage(chat, channel, buffer, CHAT_MESSAGE);
	
	GSI_UNUSED(type);
	GSI_UNUSED(param);
}

static void Kicked(CHAT chat, const gsi_char * channel, const gsi_char * user, const gsi_char * reason, void * param)
{
	_tprintf(_T("Kicked from %s by %s: %s\n"), channel, user, reason);

	GSI_UNUSED(chat);
	GSI_UNUSED(param);
}

static void UserJoined(CHAT chat, const gsi_char * channel, const gsi_char * user, int mode, void * param)
{
	_tprintf(_T("%s joined %s"), user, channel);

	GSI_UNUSED(chat);
	GSI_UNUSED(param);
	GSI_UNUSED(mode);
}

static void UserParted(CHAT chat, const gsi_char * channel, const gsi_char * user, int why, const gsi_char * reason, const gsi_char * kicker, void * param)
{
	if(why == CHAT_LEFT)
		_tprintf(_T("%s left %s\n"), user, channel);
	else if(why == CHAT_QUIT)
		_tprintf(_T("%s quit: %s\n"), user, reason);
	else if(why == CHAT_KICKED)
		_tprintf(_T("%s was kicked from %s by %s: %s"), user, channel, kicker, reason);
	else if(why == CHAT_KILLED)
		_tprintf(_T("%s was killed: %s\n"), user, reason);
	else
		_tprintf(_T("UserParted() called with unknown part-type\n"));
	
	GSI_UNUSED(chat);
	GSI_UNUSED(param);
}

static void UserChangedNick(CHAT chat, const gsi_char * channel, const gsi_char * oldNick, const gsi_char * newNick, void * param)
{
	_tprintf(_T("%s changed nicks to %s\n"), oldNick, newNick);
	
	GSI_UNUSED(chat);
	GSI_UNUSED(channel);
	GSI_UNUSED(param);
}

static void UserModeChanged(CHAT chat, const gsi_char * channel, const gsi_char * user, int mode, void * param)
{
	_tprintf(_T("%s's new mode in %s is "), user, channel);;
	if(mode == CHAT_VOICE)
		_tprintf(_T("voice\n"));
	else if(mode == CHAT_OP)
		_tprintf(_T("ops\n"));
	else if(mode == (CHAT_VOICE | CHAT_OP))
		_tprintf(_T("voice+ops\n"));
	
	GSI_UNUSED(chat);
	GSI_UNUSED(param);
}

static void TopicChanged(CHAT chat, const gsi_char * channel, const gsi_char * topic, void * param)
{
	_tprintf(_T("The topic in %s changed to %s\n"), channel, topic);
	
	GSI_UNUSED(chat);
	GSI_UNUSED(param);
}

static void ChannelModeChanged(CHAT chat, const gsi_char * channel, CHATChannelMode * mode, void * param)
{
	_tprintf(_T("The mode in %s has changed:\n"), channel);
	_tprintf(_T("   InviteOnly: %d\n"), mode->InviteOnly);
	_tprintf(_T("   Private: %d\n"), mode->Private);
	_tprintf(_T("   Secret: %d\n"), mode->Secret);
	_tprintf(_T("   Moderated: %d\n"), mode->Moderated);
	_tprintf(_T("   NoExternalMessages: %d\n"), mode->NoExternalMessages);
	_tprintf(_T("   OnlyOpsChangeTopic: %d\n"), mode->OnlyOpsChangeTopic);
	_tprintf(_T("   Limit: "));
	if(mode->Limit == 0)
		_tprintf(_T("N/A\n"));
	else
		_tprintf(_T("%d\n"), mode->Limit);
		
	GSI_UNUSED(chat);
	GSI_UNUSED(param);
}

static void UserListUpdated(CHAT chat, const gsi_char * channel, void * param)
{
	_tprintf(_T("User list updated\n"));
	
	GSI_UNUSED(chat);
	GSI_UNUSED(channel);
	GSI_UNUSED(param);
}

static void ConnectCallback(CHAT chat, CHATBool success, int failureReason, void * param)
{
	if (success == CHATFalse)
		_tprintf(_T("Failed to connect (%d)\n"), failureReason);
	else
		_tprintf(_T("Connected\n"));
	GSI_UNUSED(chat);
	GSI_UNUSED(success);
	GSI_UNUSED(param);
}


static void FillInUserCallback(CHAT chat, unsigned int IP, gsi_char user[128], void * param)
{
	_tcscpy(user, chatUser);
	
	GSI_UNUSED(chat);
	GSI_UNUSED(IP);
	GSI_UNUSED(param);
}

static void NickErrorCallback(CHAT chat, int type, const gsi_char * nick, int numSuggestedNicks, const gsi_char ** suggestedNicks, void * param)
{
	if(type == CHAT_IN_USE)
	{
		_tprintf(_T("The nick %s is already being used.\n"), nick);
		_tsnprintf(chatNick,CHAT_NICK_SIZE,_T("ChatC%lu"),(unsigned long)current_time());
		chatNick[CHAT_NICK_SIZE - 1] = '\0';
		chatRetryWithNick(chat, chatNick);
	}
	else if(type == CHAT_INVALID)
	{
		_tprintf(_T("The nick %s is invalid!\n"), nick);
		// chatDisconnect(chat); THIS CRASHES
		
		// 10-14-2004: Added By Saad Nader
		// this is necessary as the function will fail if a new nick is not retries.
		////////////////////////////////////////////////////////////////////////////
		_tsnprintf(chatNick,CHAT_NICK_SIZE,_T("ChatC%lu"),(unsigned long)current_time());
		chatNick[CHAT_NICK_SIZE - 1] = '\0';
		chatRetryWithNick(chat, chatNick);
	}
	else if((type == CHAT_UNIQUENICK_EXPIRED) || (type == CHAT_NO_UNIQUENICK))
	{
		_tprintf(_T("This account has no uniquenick or an expired uniquenick!\n"));

		chatRegisterUniqueNick(chat, 2, _T("MrPants"), _T(""));
	}
	else if(type == CHAT_INVALID_UNIQUENICK)
	{
		int i;

		_tprintf(_T("The uniquenick %s is invalid or in use\n"), nick);
		_tprintf(_T("There are %d suggested nicks:\n"), numSuggestedNicks);

		for(i = 0 ; i < numSuggestedNicks ; i++)
			_tprintf(_T("   %s\n"), suggestedNicks[i]);
	}

	// 10-14-2004: Added By Saad Nader
	// added for the addition of a new error code.
	////////////////////////////////////////////////////////////////////////////
	else if(type == CHAT_NICK_TOO_LONG)
	{
		_tprintf(_T("The nick %s is too long.\n"), nick);
		_tsnprintf(chatNick,CHAT_NICK_SIZE,_T("ChatC%lu"),(unsigned long)current_time());
		chatNick[CHAT_NICK_SIZE - 1] = '\0';
		chatRetryWithNick(chat, chatNick);
	}
	GSI_UNUSED(param);
}

CHATBool enterChannelSuccess;
static void EnterChannelCallback(CHAT chat, CHATBool success, CHATEnterResult result, const gsi_char * channel, void * param)
{
	enterChannelSuccess = success;
	
	GSI_UNUSED(chat);
	GSI_UNUSED(result);
	GSI_UNUSED(channel);
	GSI_UNUSED(param);
}

static void GetUserInfoCallback(CHAT chat, CHATBool success, const gsi_char * nick, const gsi_char * user, const gsi_char * name, const gsi_char * address, int numChannels, const gsi_char ** channels, void * param)
{
	int i;

	if(!success)
	{
		_tprintf(_T("GetUserInfo failed\n"));
		return;
	}

	_tprintf(_T("%s's Info:\n"), nick);
	_tprintf(_T("   User: %s\n"), user);
	_tprintf(_T("   Name: %s\n"), name);
	_tprintf(_T("   Address: %s\n"), address);
	_tprintf(_T("   Channels (%d):\n"), numChannels);
	for(i = 0 ; i < numChannels ; i++)
		_tprintf(_T("      %s\n"), channels[i]);
	
	GSI_UNUSED(chat);
	GSI_UNUSED(param);
}
/*
static void EnumChannelsAllCallback(CHAT chat, CHATBool success, int numChannels, const gsi_char ** channel, const gsi_char ** topic, int* numUsers, void * param)
{
	GSI_UNUSED(chat);
	GSI_UNUSED(success);
	GSI_UNUSED(numChannels);
	GSI_UNUSED(channel);
	GSI_UNUSED(topic);
	GSI_UNUSED(numUsers);
	GSI_UNUSED(param);
}
*/
static void EnumUsersCallback(CHAT chat, CHATBool success, const gsi_char * channel, int numUsers, const gsi_char ** users, int * modes, void * param)
{
	int i;

	if(!success)
	{
		_tprintf(_T("EnumUsers failed\n"));
		return;
	}

	for(i = 0 ; i < numUsers ; i++)
		chatGetUserInfo(chat, users[i], GetUserInfoCallback, NULL, CHATFalse);
		
	GSI_UNUSED(channel);
	GSI_UNUSED(modes);
	GSI_UNUSED(param);
}

int test_main(int argc, char **argv)
{
	int i;
	chatGlobalCallbacks globalCallbacks;
	chatChannelCallbacks channelCallbacks;
	unsigned long stopTime;


	// Set default options.
	///////////////////////
	// SDK takes care of default server address and port now
	//_tcscpy(serverAddress, _T("peerchat." GSI_DOMAIN_NAME));
	//port = 6667;
	_tsnprintf(chatNick,CHAT_NICK_SIZE,_T("ChatC%lu"),(unsigned long)current_time() % 1000);
	chatNick[CHAT_NICK_SIZE - 1] = '\0';
	_tcscpy(chatUser, _T("ChatCUser"));
	_tcscpy(chatName, _T("ChatCName"));
	_tcscpy(chatChannel, _T("#GSP!gmtest"));
	_tcscpy(gamename, _T("gmtest"));
	secretKey[0] = 'H';
	secretKey[1] = 'A';
	secretKey[2] = '6';
	secretKey[3] = 'z';
	secretKey[4] = 'k';
	secretKey[5] = 'S';
	secretKey[6] = '\0';

	// Go through command-line options.
	///////////////////////////////////
	for(i = 1 ; i < argc ; i++)
	{
		if((argv[i][0] == '-') && ((i + 1) < argc))
		{
			switch(argv[i][1])
			{
			case 's':
#ifndef GSI_UNICODE
				strcpy(serverAddress, argv[++i]);
#else
				AsciiToUCS2String(argv[++i], serverAddress);
#endif
				break;
			case 'p':
				port = atoi(argv[++i]);
				break;
			case 'n':
#ifndef GSI_UNICODE
				strcpy(chatNick, argv[++i]);
#else
				AsciiToUCS2String(argv[++i], chatNick);
#endif
				break;
			case 'u':
#ifndef GSI_UNICODE
				strcpy(chatUser, argv[++i]);
#else
				AsciiToUCS2String(argv[++i], chatUser);
#endif
				break;
			case 'c':
#ifndef GSI_UNICODE
				strcpy(chatChannel, argv[++i]);
#else
				AsciiToUCS2String(argv[++i], chatChannel);
#endif
				break;
			default:
				_tprintf(_T("Error parsing command-line: %s\n"), argv[i]);
				return 1;
			}
		}
		else
		{
			_tprintf(_T("Error parsing command-line: %s\n"), argv[i]);
			return 1;
		}
	}

	// Set global callbacks.
	////////////////////////
	memset(&globalCallbacks, 0, sizeof(chatGlobalCallbacks));
	globalCallbacks.raw = Raw;
	globalCallbacks.disconnected = Disconnected;
	globalCallbacks.privateMessage = PrivateMessage;
	globalCallbacks.invited = Invited;
	globalCallbacks.param = NULL;

	// Connect.
	///////////
	chat = chatConnectSecure(serverAddress[0]?serverAddress:NULL, port, chatNick, chatName, gamename, secretKey, &globalCallbacks, NickErrorCallback, FillInUserCallback, ConnectCallback, NULL, CHATTrue);
	if(!chat)
	{
		_tprintf(_T("Connect failed\n"));
		return 1;
	}

	// Set channel callbacks.
	/////////////////////////
	memset(&channelCallbacks, 0, sizeof(chatChannelCallbacks));
	channelCallbacks.channelMessage = ChannelMessage;
	channelCallbacks.channelModeChanged = ChannelModeChanged;
	channelCallbacks.kicked = Kicked;
	channelCallbacks.topicChanged = TopicChanged;
	channelCallbacks.userParted = UserParted;
	channelCallbacks.userJoined = UserJoined;
	channelCallbacks.userListUpdated = UserListUpdated;
	channelCallbacks.userModeChanged = UserModeChanged;
	channelCallbacks.userChangedNick = UserChangedNick;
	channelCallbacks.param = NULL;

	// Join.
	////////
	chatEnterChannel(chat, chatChannel, NULL, &channelCallbacks, EnterChannelCallback, NULL, CHATTrue);
	if(!enterChannelSuccess)
	{
		_tprintf(_T("Enter Channel failed\n"));
		return 1;
	}

	// Say hi.
	//////////
	chatSendChannelMessage(chat, chatChannel, _T("Hi"), CHAT_MESSAGE);


	// Enum through the players.
	////////////////////////////
	chatEnumUsers(chat, chatChannel, EnumUsersCallback, NULL, CHATFalse);

	// Stay for a while.
	////////////////////
	stopTime = (current_time() + 60000);
	do
	{
		chatThink(chat);
		msleep(50);
	}
	while(!quit && (current_time() < stopTime));

	// Say bye.
	///////////
	chatSendChannelMessage(chat, chatChannel, _T("Bye"), CHAT_MESSAGE);

	// Leave.
	/////////
	chatLeaveChannel(chat, chatChannel, NULL);

	// Disconnect.
	//////////////
	chatDisconnect(chat);
	_tprintf(_T("All Done!\n"));
	return 0;
}
