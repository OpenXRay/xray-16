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
#include "chat.h"
#include "chatMain.h"
#include "chatASCII.h"
#include "chatSocket.h"
#include "chatHandlers.h"
#include "chatChannel.h"
#include "chatCallbacks.h"


#if defined(_WIN32)
// Silence the warning about explicitly casting a function* to a void*
#pragma warning(disable:4054)
#endif

/************
** GLOBALS **
************/
// This can be overridden using an extern to pass a different versionID number to the chat server as part of the crypt negotiation
int ciVersionID = 1;

/************
** DEFINES **
************/
#define CI_DO_BLOCKING      if(blocking)\
							{\
								do{\
									ciThink(chat, ID);\
									msleep(10);\
								}while(ciCheckForID(chat, ID));\
							}

#define ASSERT_CHANNEL()    assert(channel != NULL); assert(channel[0] != '\0');
#define ASSERT_NICK()       assert(nick != NULL); assert(nick[0] != '\0'); assert(strlen(nick) < MAX_NICK);
#define ASSERT_USER(user)   assert(user != NULL); assert(user[0] != '\0'); assert(strlen(user) < MAX_USER);
#define ASSERT_MESSAGE()    assert(message != NULL); assert(message[0] != '\0');
#define ASSERT_TYPE(type)   assert((type == CHAT_MESSAGE) || (type == CHAT_ACTION) || (type == CHAT_NOTICE) || (type == CHAT_UTM) || (type == CHAT_ATM));
#define ASSERT_PASSWORD()   assert(password != NULL); assert(password[0] != '\0');
#define ASSERT_BAN()        assert(ban != NULL); assert(ban [0] != '\0');

#define CI_NUM_TRANSLATED_NICKS   2

/**********
** TYPES **
**********/

typedef struct ciEnumUsersData
{
	chatEnumUsersCallback callback;
	void * param;
} ciEnumUsersData;

/**************
** FUNCTIONS **
**************/
static CHATBool ciProcessServerMessage(CHAT chat, const ciServerMessage * message)
{
	int i;

	assert(message != NULL);

	// Figure out what type of message this is.
	///////////////////////////////////////////
	for(i = 0 ; i < numServerMessageTypes ; i++)
	{
		// Does the type match?
		///////////////////////
		if(strcasecmp(message->command, serverMessageTypes[i].command) == 0)
		{
			// Is there a handler?
			//////////////////////
			if(serverMessageTypes[i].handler != NULL)
			{
				// Call the handler.
				////////////////////
				serverMessageTypes[i].handler(chat, message);
			}

			return CHATTrue;
		}
	}

	// Didn't find a match.
	///////////////////////
	return CHATFalse;  //ERRCON
}

static CHATBool ciCheckForID(CHAT chat, int ID)
{
	return (CHATBool)(ciCheckFiltersForID(chat, ID) || ciCheckCallbacksForID(chat, ID));
}

void ciHandleDisconnect(CHAT chat, const char * reason)
{
	CHATBool connecting;
	CONNECTION;

	// Check if we've already handled this.
	///////////////////////////////////////
	if(connection->disconnected)
		return;

	// Keep track of if we are trying to connect.
	/////////////////////////////////////////////
	connecting = connection->connecting;

	// Not connected anymore.
	/////////////////////////
	connection->connected = CHATFalse;
	connection->connecting = CHATFalse;
	connection->disconnected = CHATTrue;

	// If we're still connecting, let the app know the attempt failed.
	//////////////////////////////////////////////////////////////////
	if(connection->connecting)
	{
		// Call the callback.
		/////////////////////
		if(connection->connectCallback != NULL)
			connection->connectCallback(chat, CHATFalse, CHAT_DISCONNECTED, connection->connectParam);
	}
	// Otherwise call the global callback.
	//////////////////////////////////////
	else if(connection->globalCallbacks.disconnected != NULL)
	{
		ciCallbackDisconnectedParams params;
		params.reason = (char *)reason;
		ciAddCallback(chat, CALLBACK_DISCONNECTED, (void*)connection->globalCallbacks.disconnected, &params, connection->globalCallbacks.param, 0, NULL);
	}
	GSI_UNUSED(connecting);
}

static void ciThink(CHAT chat, int ID)
{
	ciServerMessage * message;
	CONNECTION;

	// Is the socket connected?
	///////////////////////////
	if(connection->chatSocket.connectState == ciConnected)
	{
		// Do processing.
		/////////////////
		ciSocketThink(&connection->chatSocket);

		// Check received messages.
		///////////////////////////
		while((message = ciSocketRecv(&connection->chatSocket)) != NULL)
		{
			// Call the raw callback.
			/////////////////////////
			if(connection->globalCallbacks.raw != NULL)
			{
				ciCallbackRawParams params;
				params.raw = message->message;
				ciAddCallback(chat, CALLBACK_RAW, (void*)connection->globalCallbacks.raw, &params, connection->globalCallbacks.param, 0, NULL);
			}

			// Process the message.
			///////////////////////
			ciProcessServerMessage(chat, message);
		}

		// Have we lost connection?
		///////////////////////////
		if(connection->chatSocket.connectState == ciDisconnected)
		{
			ciHandleDisconnect(chat, "Disconnected");
		}
	}

	// Let the filters think.
	/////////////////////////
	ciFilterThink(chat);

	// Call callbacks.
	//////////////////
	ciCallCallbacks(chat, ID);

}

/************
** GENERAL **
************/
void ciSendNick(CHAT chat)
{
	const char * nick;
	CONNECTION;

	// Handle based on login type.
	//////////////////////////////
	if(connection->loginType == CINoLogin)
	{
		
		// 10-13-2004: changed by Saad Nader
		// check for nick length and for an invalid nick.
		/////////////////////////////////////////////////

		int validateNick = ciNickIsValid(connection->nick);
		if (validateNick != CHAT_NICK_OK)
		{
			ciNickError(chat, validateNick, connection->nick, 0, NULL);
			return;
		}
		
		// Use the provided nick.
		/////////////////////////
		nick = connection->nick;
	}
	else if((connection->loginType == CIProfileLogin) && (connection->namespaceID == 0))
	{
		
		// 10-13-2004: changed by Saad Nader
		// check for nick length and for an invalid nick.
		/////////////////////////////////////////////////

		int validateNick = ciNickIsValid(connection->profilenick);
		if (validateNick != CHAT_NICK_OK)
		{
			ciNickError(chat, validateNick, connection->profilenick, 0, NULL);
			return;
		}
		
		// Use the profile's nick.
		//////////////////////////
		nick = connection->profilenick;
	}
	else
	{
		// The server will use the uniquenick.
		//////////////////////////////////////
		nick = "*";
	}

	// Send the nick.
	/////////////////
	ciSocketSendf(&connection->chatSocket, "NICK %s", nick);
}

void ciSendUser(CHAT chat)
{
	CONNECTION;

	// Send the user.
	/////////////////
	ciSocketSendf(&connection->chatSocket, "USER %s %s %s :%s",
		connection->user,
		"127.0.0.1",
		connection->server,
		connection->name);
}

void ciSendNickAndUser(CHAT chat)
{
	ciSendUser(chat);
	ciSendNick(chat);
}

void ciSendLogin(CHAT chat)
{
	char passwordHash[33];
	CONNECTION;

	// If it's pre-auth, send it.
	/////////////////////////////
	if(connection->loginType == CIPreAuthLogin)
	{
		ciSocketSendf(&connection->chatSocket, "LOGINPREAUTH %s %s",
			connection->authtoken,
			connection->partnerchallenge);

		return;
	}

	// For uniquenick or profile logins, we need to MD5 the password.
	/////////////////////////////////////////////////////////////////
	MD5Digest((unsigned char *)connection->password, strlen(connection->password), passwordHash);

	// Send the login message based on type.
	////////////////////////////////////////
	if(connection->loginType == CIUniqueNickLogin)
	{
		ciSocketSendf(&connection->chatSocket, "LOGIN %d %s %s",
			connection->namespaceID,
			connection->uniquenick,
			passwordHash);
	}
	else if(connection->loginType == CIProfileLogin)
	{
		ciSocketSendf(&connection->chatSocket, "LOGIN %d * %s :%s@%s",
			connection->namespaceID,
			passwordHash,
			connection->profilenick,
			connection->email);
	}
	else
	{
		// If we get here, the login type is invalid or isn't being handled properly.
		/////////////////////////////////////////////////////////////////////////////
		assert(0);
	}
}

static CHAT chatConnectDoit(CILoginType loginType,
				 const char * serverAddress,
				 int port,
                 const char * nick,
				 const char * user,
				 const char * name,
				 int namespaceID,
				 const char * email,
				 const char * profilenick,
				 const char * uniquenick,
				 const char * password,
				 const char * authtoken,
				 const char * partnerchallenge,
				 const char * gamename,
				 const char * secretKey,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking)
{
	ciConnection * connection;
	const char * socketNick = "";
	
	//Added default server address and port
	//assert(serverAddress != NULL);
	assert(callbacks != NULL);
	assert(connectCallback != NULL);
	
	// Check the arguments based on the login type.
	///////////////////////////////////////////////
	if(loginType == CINoLogin)
	{
		ASSERT_NICK();
		if(!nick || !nick[0])
			return NULL;
		socketNick = nick;
	}
	else if(loginType == CIUniqueNickLogin)
	{
		assert(namespaceID > 0);
		if(namespaceID <= 0)
			return NULL;
		assert(uniquenick && uniquenick[0]);
		if(!uniquenick || !uniquenick[0])
			return NULL;
		assert(password && password[0]);
		if(!password || !password[0])
			return NULL;
		socketNick = uniquenick;
	}
	else if(loginType == CIProfileLogin)
	{
		assert(namespaceID >= 0);
		if(namespaceID < 0)
			return NULL;
		assert(email && email[0]);
		if(!email || !email[0])
			return NULL;
		assert(profilenick && profilenick[0]);
		if(!profilenick || !profilenick[0])
			return NULL;
		assert(password && password[0]);
		if(!password || !password[0])
			return NULL;
		socketNick = profilenick;
	}
	else if(loginType == CIPreAuthLogin)
	{
		assert(authtoken && authtoken[0]);
		if(!authtoken || !authtoken[0])
			return NULL;
		assert(partnerchallenge && partnerchallenge[0]);
		if(!partnerchallenge || !partnerchallenge[0])
			return NULL;
		socketNick = "preauth";
	}
	if(loginType != CINoLogin)
	{
		assert(gamename && gamename[0]);
		if(!gamename || !gamename[0])
			return NULL;
		assert(secretKey && secretKey[0]);
		if(!secretKey || !secretKey[0])
			return NULL;
	}

	// Init sockets.
	////////////////
	SocketStartUp();

	// Create a connection object.
	//////////////////////////////
	connection = (ciConnection *)gsimalloc(sizeof(ciConnection));
	if(connection == NULL)
		return NULL;  //ERRCON

	// Initialize the connection.
	/////////////////////////////
	memset(connection, 0, sizeof(ciConnection));
	connection->loginType = loginType;
	if(nick)
		strzcpy(connection->nick, nick, MAX_NICK);
	if(user)
		strzcpy(connection->user, user, MAX_USER);
#ifdef GSI_UNICODE // store a unicode version of the nick and user 
	UTF8ToUCS2String(connection->nick, connection->nickW);
	UTF8ToUCS2String(connection->user, connection->userW);
#endif
	if(name)
		strzcpy(connection->name, name, MAX_NAME);
	connection->namespaceID = namespaceID;
	if(email)
		strzcpy(connection->email, email, MAX_EMAIL);
	if(profilenick)
		strzcpy(connection->profilenick, profilenick, MAX_PROFILENICK);
	if(uniquenick)
		strzcpy(connection->uniquenick, uniquenick, MAX_UNIQUENICK);
	if(password)
		strzcpy(connection->password, password, MAX_PASSWORD);
	if(authtoken)
		strzcpy(connection->authtoken, authtoken, MAX_AUTHTOKEN);
	if(partnerchallenge)
		strzcpy(connection->partnerchallenge, partnerchallenge, MAX_PARTNERCHALLENGE);
	strzcpy(connection->server, serverAddress?serverAddress:CI_DEFAULT_SERVER_ADDRESS, MAX_SERVER);
	connection->port = port?port:CI_DEFUILT_SERVER_PORT;
	connection->globalCallbacks = *callbacks;
	connection->nextID = 1;
	connection->connecting = CHATTrue;
	connection->quiet = CHATFalse;

	// Initialize the channel table.
	////////////////////////////////
	if(!ciInitChannels(connection))
	{
		gsifree(connection);
		SocketShutDown();
		return NULL; //ERRCON
	}

	// Initialize the callbacks list.
	/////////////////////////////////
	if(!ciInitCallbacks(connection))
	{
		ciCleanupChannels((CHAT)connection);
		gsifree(connection);
		SocketShutDown();
		return NULL; //ERRCON
	}

	// Initialize the socket.
	/////////////////////////
	if(!ciSocketInit(&connection->chatSocket, socketNick))
	{
		ciCleanupCallbacks((CHAT)connection);
		ciCleanupChannels((CHAT)connection);
		gsifree(connection);
		SocketShutDown();
		return NULL; //ERRCON
	}

	// Connect the socket.
	//////////////////////
	if(!ciSocketConnect(&connection->chatSocket, connection->server, connection->port))
	{
		ciSocketDisconnect(&connection->chatSocket);
		ciCleanupCallbacks((CHAT)connection);
		ciCleanupChannels((CHAT)connection);
		gsifree(connection);
		SocketShutDown();
		return NULL; //ERRCON
	}

	// Special stuff for MS Chat server.
	////////////////////////////////////
	//ciSocketSend(&connection->chatSocket, "MODE ISIRCX");
	//ciSocketSend(&connection->chatSocket, "IRCX");

	// Set the callback info.
	/////////////////////////
	connection->nickErrorCallback = nickErrorCallback;
	connection->fillInUserCallback = fillInUserCallback;
	connection->connectCallback = connectCallback;
	connection->connectParam = param;
	
	// Check for a secure connection.
	/////////////////////////////////
	if(gamename && gamename[0] && secretKey && secretKey[0])
	{
		// Save the game secret key.
		////////////////////////////
		strzcpy(connection->secretKey, secretKey, MAX_SECRETKEY);

		// Get the random keys.
		///////////////////////
		ciSocketSendf(&connection->chatSocket, "CRYPT des %d %s", ciVersionID, gamename);
	}
	else if(connection->fillInUserCallback)
	{
		// Get the IP.
		//////////////
		ciSocketSend(&connection->chatSocket, "USRIP");
	}
	else
	{
		// Send the nick and user.
		//////////////////////////
		ciSendNickAndUser((CHAT)connection);
	}

	// Do blocking.
	///////////////
	if(blocking)
	{
		// While we're connecting.
		//////////////////////////
		do
		{
			ciThink((CHAT)connection, 0);
			msleep(10);
		} while(connection->connecting);

		// Check if the connect failed.
		///////////////////////////////
		if(!connection->connected)
		{
			// Disconnect the connection.
			/////////////////////////////
			chatDisconnect((CHAT)connection);
			connection = NULL;
		}
	}
	
	return (CHAT)connection;
}

CHAT chatConnectA(const char * serverAddress,
				 int port,
                 const char * nick,
				 const char * user,
				 const char * name,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking)
{
	return chatConnectDoit(CINoLogin,
		serverAddress,
		port,
		nick,
		user,
		name,
		0,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		callbacks,
		nickErrorCallback,
		NULL,
		connectCallback,
		param,
		blocking);
}
#ifdef GSI_UNICODE
CHAT chatConnectW(const unsigned short * serverAddress,
				 int port,
                 const unsigned short * nick,
				 const unsigned short * user,
				 const unsigned short * name,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking)
{
	char* serverAddress_A = (char*)UCS2ToUTF8StringAlloc(serverAddress);
	char* nick_A = (char*)UCS2ToUTF8StringAlloc(nick);
	char* user_A = (char*)UCS2ToUTF8StringAlloc(user);
	char* name_A = (char*)UCS2ToUTF8StringAlloc(name);

	CHAT aChat = chatConnectA(serverAddress_A,
		port,
		nick_A,
		user_A,
		name_A,
		callbacks,
		nickErrorCallback,
		connectCallback,
		param,
		blocking);

	gsifree(serverAddress_A);
	gsifree(nick_A);
	gsifree(user_A);
	gsifree(name_A);

	return aChat;
}
#endif

CHAT chatConnectSpecialA(const char * serverAddress,
				 int port,
                 const char * nick,
				 const char * name,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking)
{
	return chatConnectDoit(CINoLogin,
		serverAddress,
		port,
		nick,
		NULL,
		name,
		0,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		callbacks,
		nickErrorCallback,
		fillInUserCallback,
		connectCallback,
		param,
		blocking);
}
#ifdef GSI_UNICODE
CHAT chatConnectSpecialW(const unsigned short * serverAddress,
				 int port,
                 const unsigned short * nick,
				 const unsigned short  * name,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking)
{
	char* serverAddress_A = (char*)UCS2ToUTF8StringAlloc(serverAddress);
	char* nick_A = (char*)UCS2ToUTF8StringAlloc(nick);
	char* name_A = (char*)UCS2ToUTF8StringAlloc(name);
	CHAT aChat = chatConnectSpecialA(serverAddress_A, port, nick_A, name_A, callbacks, nickErrorCallback, fillInUserCallback, connectCallback, param, blocking);
	gsifree(serverAddress_A);
	gsifree(nick_A);
	gsifree(name_A);

	return aChat;
}
#endif

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
                 CHATBool blocking)
{
	return chatConnectDoit(CINoLogin,
		serverAddress,
		port,
		nick,
		NULL,
		name,
		0,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		gamename,
		secretKey,
		callbacks,
		nickErrorCallback,
		fillInUserCallback,
		connectCallback,
		param,
		blocking);
}
#ifdef GSI_UNICODE
CHAT chatConnectSecureW(const unsigned short * serverAddress,
				 int port,
                 const unsigned short * nick,
				 const unsigned short * name,
				 const unsigned short * gamename,
				 const unsigned short * secretKey,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking)
{
	char* serverAddress_A = (char*)UCS2ToUTF8StringAlloc(serverAddress);
	char* nick_A = (char*)UCS2ToUTF8StringAlloc(nick);
	char* name_A = (char*)UCS2ToUTF8StringAlloc(name);
	char* gamename_A = (char*)UCS2ToUTF8StringAlloc(gamename);
	char* secretKey_A = (char*)UCS2ToUTF8StringAlloc(secretKey);
	CHAT aChat = chatConnectSecureA(serverAddress_A, port, nick_A, name_A, gamename_A, secretKey_A, callbacks, nickErrorCallback, fillInUserCallback, connectCallback, param, blocking);
	gsifree(serverAddress_A);
	gsifree(nick_A);
	gsifree(name_A);
	gsifree(gamename_A);
	gsifree(secretKey_A);

	return aChat;
}
#endif

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
                 CHATBool blocking)
{
	return chatConnectDoit((uniquenick && uniquenick[0])?CIUniqueNickLogin:CIProfileLogin,
		serverAddress,
		port,
		NULL,
		NULL,
		name,
		namespaceID,
		email,
		profilenick,
		uniquenick,
		password,
		NULL,
		NULL,
		gamename,
		secretKey,
		callbacks,
		nickErrorCallback,
		fillInUserCallback,
		connectCallback,
		param,
		blocking);
}
#ifdef GSI_UNICODE
CHAT chatConnectLoginW(const unsigned short * serverAddress,
				 int port,
				 int namespaceID,
				 const unsigned short * email,
				 const unsigned short * profilenick,
				 const unsigned short * uniquenick,
				 const unsigned short * password,
				 const unsigned short * name,
				 const unsigned short * gamename,
				 const unsigned short * secretKey,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking)
{
	char* serverAddress_A = (char*)UCS2ToUTF8StringAlloc(serverAddress);
	char* email_A = (char*)UCS2ToUTF8StringAlloc(email);
	char* profilenick_A = (char*)UCS2ToUTF8StringAlloc(profilenick);
	char* uniquenick_A = (char*)UCS2ToUTF8StringAlloc(uniquenick);
	char* password_A = (char*)UCS2ToUTF8StringAlloc(password);
	char* name_A = (char*)UCS2ToUTF8StringAlloc(name);
	char* gamename_A = (char*)UCS2ToUTF8StringAlloc(gamename);
	char* secretKey_A = (char*)UCS2ToUTF8StringAlloc(secretKey);
	CHAT aChat= chatConnectLoginA(serverAddress_A, port, namespaceID, email_A, profilenick_A, uniquenick_A, password_A, name_A, gamename_A, secretKey_A, callbacks, nickErrorCallback, fillInUserCallback, connectCallback, param, blocking);
	gsifree(serverAddress_A);
	gsifree(email_A);
	gsifree(profilenick_A);
	gsifree(uniquenick_A);
	gsifree(name_A);
	gsifree(gamename_A);
	gsifree(secretKey_A);

	return aChat;
}
#endif

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
                 CHATBool blocking)
{
	return chatConnectDoit(CIPreAuthLogin,
		serverAddress,
		port,
		NULL,
		NULL,
		name,
		0,
		NULL,
		NULL,
		NULL,
		NULL,
		authtoken,
		partnerchallenge,
		gamename,
		secretKey,
		callbacks,
		nickErrorCallback,
		fillInUserCallback,
		connectCallback,
		param,
		blocking);
}
#ifdef GSI_UNICODE
CHAT chatConnectPreAuthW(const unsigned short * serverAddress,
				 int port,
				 const unsigned short * authtoken,
				 const unsigned short * partnerchallenge,
				 const unsigned short * name,
				 const unsigned short * gamename,
				 const unsigned short * secretKey,
				 chatGlobalCallbacks * callbacks,
				 chatNickErrorCallback nickErrorCallback,
				 chatFillInUserCallback fillInUserCallback,
                 chatConnectCallback connectCallback,
                 void * param,
                 CHATBool blocking)
{
	char* serverAddress_A = (char*)UCS2ToUTF8StringAlloc(serverAddress);
	char* authtoken_A  = (char*)UCS2ToUTF8StringAlloc(authtoken);
	char* partnerchallenge_A = (char*)UCS2ToUTF8StringAlloc(partnerchallenge);
	char* name_A = (char*)UCS2ToUTF8StringAlloc(name);
	char* gamename_A = (char*)UCS2ToUTF8StringAlloc(gamename);
	char* secretKey_A = (char*)UCS2ToUTF8StringAlloc(secretKey);
	CHAT aChat = chatConnectPreAuthA(serverAddress_A, port, authtoken_A, partnerchallenge_A, name_A, gamename_A, secretKey_A, callbacks, nickErrorCallback, fillInUserCallback, connectCallback, param, blocking);
	gsifree(serverAddress_A);
	gsifree(authtoken_A);
	gsifree(partnerchallenge_A);
	gsifree(name_A);
	gsifree(gamename_A);
	gsifree(secretKey_A);

	return aChat;
}
#endif

void chatRetryWithNickA(CHAT chat,
					   const char * nick)
{
	int validateNick;
	CONNECTION;
	
	// Are we already connected?
	////////////////////////////
	if(connection->connected)
		return;

	// A NULL nick means stop retrying and disconnect
	if (nick == NULL)
	{
		connection->connecting = CHATFalse;

		// Call the callback.  (Failed to connect)
		/////////////////////
		if(connection->connectCallback != NULL)
			connection->connectCallback(chat, CHATFalse, CHAT_NICK_ERROR, connection->connectParam);

		return;
	}

	// Copy the new nick.
	/////////////////////
	strzcpy(connection->nick, nick, MAX_NICK);
#ifdef GSI_UNICODE // store a unicode version of the nick
	AsciiToUCS2String(connection->nick, connection->nickW);
#endif

	// Check for a bad nick.
	////////////////////////
	validateNick = ciNickIsValid(nick);
	if (validateNick != CHAT_NICK_OK)
	{
		ciNickError(chat, validateNick, nick, 0, NULL);
		return;
	}

	// Send the new nick.
	/////////////////////
	ciSocketSendf(&connection->chatSocket, "NICK :%s", nick);

}
#ifdef GSI_UNICODE
void chatRetryWithNickW(CHAT chat,
					   const unsigned short * nick)
{
	char* nick_A = (char*)UCS2ToUTF8StringAlloc(nick);
	chatRetryWithNickA(chat, nick_A);
	gsifree(nick_A);
}
#endif

void chatRegisterUniqueNickA(CHAT chat,
							int namespaceID,
							const char * uniquenick,
							const char * cdkey)
{
	CONNECTION;

	// Are we already connected?
	////////////////////////////
	if(connection->connected)
		return;

	// A NULL nick means stop trying and disconnect.
	////////////////////////////////////////////////
	if(uniquenick == NULL)
	{
		connection->connecting = CHATFalse;

		// Call the callback.
		/////////////////////
		if(connection->connectCallback != NULL)
			connection->connectCallback(chat, CHATFalse, CHAT_NICK_ERROR, connection->connectParam);

		return;
	}

	// CDKey is optional.
	/////////////////////
	if(!cdkey)
		cdkey = "";

	// Send the message.
	////////////////////
	ciSocketSendf(&connection->chatSocket, "REGISTERNICK %d %s %s", namespaceID, uniquenick, cdkey);

	// Save the uniquenick we're trying to use.
	///////////////////////////////////////////
	strzcpy(connection->uniquenick, uniquenick, MAX_UNIQUENICK);
}
#ifdef GSI_UNICODE
void chatRegisterUniqueNickW(CHAT chat,
							int namespaceID,
							const unsigned short * uniquenick,
							const unsigned short * cdkey)
{
	char* uniquenick_A = (char*)UCS2ToUTF8StringAlloc(uniquenick);
	char* cdkey_A = (char*)UCS2ToUTF8StringAlloc(cdkey);
	chatRegisterUniqueNickA(chat, namespaceID, uniquenick_A, cdkey_A);
	gsifree(uniquenick_A);
	gsifree(cdkey_A);
}
#endif

void chatDisconnect(CHAT chat)
{
	CONNECTION;

	// Cleanup all the filters first.
	/////////////////////////////////
	ciCleanupFilters(chat);

	// Call the disconnected callback if we haven't already.
	////////////////////////////////////////////////////////
	if(!connection->disconnected && connection->globalCallbacks.disconnected)
#ifdef GSI_UNICODE
		connection->globalCallbacks.disconnected(chat, L"", connection->globalCallbacks.param);
#else
		connection->globalCallbacks.disconnected(chat, "", connection->globalCallbacks.param);
#endif

	// Are we connected.
	////////////////////
	if(connection->connected)
	{
		ciSocketSend(&connection->chatSocket, "QUIT :Later!");
		ciSocketThink(&connection->chatSocket);
	}

	// gsifree the channel table.
	//////////////////////////
	ciCleanupChannels(chat);

	// Cleanup the callbacks list.
	//////////////////////////////
	ciCleanupCallbacks(chat);

	// Shutdown the chat socket.
	////////////////////////////
	ciSocketDisconnect(&connection->chatSocket);

	// gsifree the memory.
	///////////////////
	gsifree(chat);

	// Shutdown sockets.
	////////////////////
	SocketShutDown();
}

void chatThink(CHAT chat)
{
	ciThink(chat, 0);
}

void chatSendRawA(CHAT chat,
				 const char * command)
{
	CONNECTION;
	if(!connection || (!connection->connected && !connection->connecting))
		return;

	ciSocketSend(&connection->chatSocket, command);
}
#ifdef GSI_UNICODE
void chatSendRawW(CHAT chat,
				 const unsigned short* command)
{
	char* command_A = (char*)UCS2ToUTF8StringAlloc(command);
	chatSendRawA(chat, command_A);
	gsifree(command_A);
}
#endif

void chatChangeNickA(CHAT chat,
					const char * newNick,
					chatChangeNickCallback callback,
					void * param,
					CHATBool blocking)
{
	int ID;
	CHATBool success = CHATTrue;
	
	CONNECTION;
	CONNECTED;

	assert(newNick);
	assert(newNick[0]);
	assert(strlen(newNick) < MAX_NICK);
	assert(callback);
	assert(connection->connected);

	// chatRetryWithNick should be called while connecting.
	///////////////////////////////////////////////////////
	if(!connection->connected)
		return;

	// No nick.
	///////////
	if(!newNick || !newNick[0])
		success = CHATFalse;
	
	// 10-13-2004: Added By Saad Nader
	// check for long or invalid chars in new nick.
	///////////////////////////////////////////////
	if (ciNickIsValid(newNick) != CHAT_NICK_OK)
	{
		success = CHATFalse;
	}

	// Check for same nick.
	///////////////////////
	if(success && (strcasecmp(newNick, connection->nick) == 0))
		success = CHATFalse;

	// Call the callback?
	/////////////////////
	if(!success)
	{
		if(callback)
		{
			ciCallbackChangeNickParams params;
			params.success = success;
			params.oldNick = connection->nick;
			params.newNick = (char *)newNick;
			ID = ciGetNextID(chat);
			ciAddCallback(chat, CALLBACK_CHANGE_NICK, (void*)callback, &params, param, ID, NULL);

			CI_DO_BLOCKING;
		}

		return;
	}

	// Send the request.
	////////////////////
	ciSocketSendf(&connection->chatSocket, "NICK :%s", newNick);

	ID = ciAddNICKFilter(chat, connection->nick, newNick, callback, param);

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatChangeNickW(CHAT chat,
					const unsigned short * newNick,
					chatChangeNickCallback callback,
					void * param,
					CHATBool blocking)
{
	char* newNick_A = (char*)UCS2ToUTF8StringAlloc(newNick);
	chatChangeNickA(chat, newNick_A, callback, param, blocking);
	gsifree(newNick_A);
}
#endif

const char * chatGetNickA(CHAT chat)
{
	CONNECTION;

	if(!connection->connected) 
		return "";

	return connection->nick;
}
#ifdef GSI_UNICODE
const unsigned short * chatGetNickW(CHAT chat)
{
	CONNECTION;

	if(!connection->connected) 
		return L"";

	return connection->nickW;
}
#endif

void chatFixNickA(char * newNick,
				 const char * oldNick)
{
	int c;
	char oldNickCopy[MAX_CHAT_NICK];
	char *pOldNick = oldNickCopy;
	assert(oldNick);
	assert(newNick);
	strzcpy(oldNickCopy, oldNick, MAX_CHAT_NICK);
	//if(isdigit(*oldNick) || (*oldNick == '-'))
	// 10-14-2004 Changed by Saad Nader
	// Using the nickname rules for unique nicks 
	// commented out the previous rules
	////////////////////////////////////////////////
	if(*pOldNick == '@' || *pOldNick== '#' || *pOldNick== '+' || *pOldNick == ':')
		*newNick++ = '_';

	while((c = *pOldNick++) != '\0')
	{
		if(!strchr(VALID_NICK_CHARS, c))
			c = '_';

		*newNick++ = (char)c;
	}
	*newNick = '\0';

}
#ifdef GSI_UNICODE
void chatFixNickW(unsigned short* newNick,
				  const unsigned short* oldNick)
{
	char* oldNick_A = (char*)UCS2ToUTF8StringAlloc(newNick);
	char newNick_A[MAX_NICK];

	chatFixNickA(newNick_A, oldNick_A);

	UTF8ToUCS2String(newNick_A, newNick);

	gsifree(oldNick_A);
	
	GSI_UNUSED(oldNick);
}
#endif

const char * chatTranslateNickA(char * nick,
								const char * extension)
{
	int nickLen;
	int extensionLen;

	assert(nick);
	assert(extension);

	nickLen = (int)strlen(nick);
	extensionLen = (int)strlen(extension);

	if((extensionLen < nickLen) && (strcasecmp(nick + nickLen - extensionLen, extension) == 0))
	{
		nick[nickLen - extensionLen] = '\0';
		return nick;
	}

	return NULL;
}
#ifdef GSI_UNICODE
const unsigned short * chatTranslateNickW(unsigned short * nick,
										  const unsigned short * extension)
{
	char nick_A[MAX_NICK];
	char extension_A[MAX_NICK];
	const char * translatedNick_A;

	assert(nick);
	assert(extension);

	UCS2ToAsciiString(nick, nick_A);
	UCS2ToAsciiString(extension, extension_A);

	translatedNick_A = chatTranslateNickA(nick_A, extension_A);

	if(translatedNick_A)
	{
		AsciiToUCS2String(translatedNick_A, nick);
		return nick;
	}

	return NULL;
}
#endif

int chatGetUserID(CHAT chat)
{
	CONNECTION;

	return connection->userID;
}

int chatGetProfileID(CHAT chat)
{
	CONNECTION;

	return connection->profileID;
}

static void ciSetQuietModeEnumJoinedChannelsA(CHAT chat,
											 int index,
											 const char * channel,
											 void * param)
{
	// Setup a filter.
	//////////////////
	ciAddUNQUIETFilter(chat, channel);
	
	GSI_UNUSED(index);
	GSI_UNUSED(param);
}
#ifdef GSI_UNICODE
static void ciSetQuietModeEnumJoinedChannelsW(CHAT chat,
											 int index,
											 const unsigned short * channel,
											 void * param)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	ciSetQuietModeEnumJoinedChannelsA(chat, index, channel_A, param);
	gsifree(channel_A);
}
#endif

void chatSetQuietMode(CHAT chat,
					  CHATBool quiet)
{
	CONNECTION;
	CONNECTED;

	// Check if its the current mode.
	/////////////////////////////////
	if(connection->quiet == quiet)
		return;

	// Send the message.
	////////////////////
	if(quiet)
		ciSocketSendf(&connection->chatSocket, "MODE %s +q", connection->nick);
	else
		ciSocketSendf(&connection->chatSocket, "MODE %s -q", connection->nick);

	// Set the mode.
	////////////////
	connection->quiet = quiet;

	// Are we disabling it?
	///////////////////////
	if(!quiet)
	{
		// Clear all the player lists.
		//////////////////////////////
		ciClearAllUsers(chat);

		// Setup a filter for each joined channel.
		//////////////////////////////////////////
#ifdef GSI_UNICODE
		ciEnumJoinedChannels(chat, ciSetQuietModeEnumJoinedChannelsW, NULL);
#else
		ciEnumJoinedChannels(chat, ciSetQuietModeEnumJoinedChannelsA, NULL);
#endif
	}
}

void chatAuthenticateCDKeyA(CHAT chat,
						   const char * cdkey,
						   chatAuthenticateCDKeyCallback callback,
						   void * param,
						   CHATBool blocking)
{
	int ID;
	CHATBool success = CHATTrue;

	CONNECTION;
	CONNECTED;

	assert(cdkey);
	assert(cdkey[0]);
	assert(callback);
	assert(connection->connected);

	// Check we're connected.
	/////////////////////////
	if(!connection->connected)
		return;

	// No key.
	//////////
	if(!cdkey || !cdkey[0])
		success = CHATFalse;

	// Call the callback?
	/////////////////////
	if(!success)
	{
		if(callback)
		{
			ciCallbackAuthenticateCDKeyParams params;
			params.result = 0;
			params.message = "";
			ID = ciGetNextID(chat);
			ciAddCallback(chat, CALLBACK_AUTHENTICATE_CDKEY, (void*)callback, &params, param, ID, NULL);

			CI_DO_BLOCKING;
		}

		return;
	}

	// Send the request.
	////////////////////
	ciSocketSendf(&connection->chatSocket, "CDKEY %s", cdkey);

	ID = ciAddCDKEYFilter(chat, callback, param);

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatAuthenticateCDKeyW(CHAT chat,
						   const unsigned short* cdkey,
						   chatAuthenticateCDKeyCallback callback,
						   void * param,
						   CHATBool blocking)
{
	char* cdkey_A = (char*)UCS2ToUTF8StringAlloc(cdkey);
	chatAuthenticateCDKeyA(chat, cdkey_A, callback, param, blocking);
	gsifree(cdkey_A);
}
#endif

/*************
** CHANNELS **
*************/
void chatEnumChannelsA(CHAT chat,
					  const char * filter,
					  chatEnumChannelsCallbackEach callbackEach,
					  chatEnumChannelsCallbackAll callbackAll,
					  void * param,
					  CHATBool blocking)
{
	int ID;
	CONNECTION;
	CONNECTED;

	assert((callbackAll != NULL) || (callbackEach != NULL));

	if(!filter)
		filter = "";

	ciSocketSendf(&connection->chatSocket, "LIST %s", filter);

	ID = ciAddLISTFilter(chat, callbackEach, callbackAll, param);

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatEnumChannelsW(CHAT chat,
					  const unsigned short * filter,
					  chatEnumChannelsCallbackEach callbackEach,
					  chatEnumChannelsCallbackAll callbackAll,
					  void * param,
					  CHATBool blocking)
{
	char* filter_A = (char*)UCS2ToUTF8StringAlloc(filter);
	chatEnumChannelsA(chat, filter_A, callbackEach, callbackAll, param, blocking);
	gsifree(filter_A);
}
#endif

void chatEnterChannelA(CHAT chat,
					  const char * channel,
					  const char * password,
					  chatChannelCallbacks * callbacks,
					  chatEnterChannelCallback callback,
					  void * param,
					  CHATBool blocking)
{
	int ID;
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	assert(callbacks != NULL);

	if(password == NULL)
		password = "";

	ciSocketSendf(&connection->chatSocket, "JOIN %s %s", channel, password);

	ID = ciAddJOINFilter(chat, channel, callback, param, callbacks, password);

	// Entering.
	////////////
	ciChannelEntering(chat, channel);

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatEnterChannelW(CHAT chat,
					  const unsigned short * channel,
					  const unsigned short * password,
					  chatChannelCallbacks * callbacks,
					  chatEnterChannelCallback callback,
					  void * param,
					  CHATBool blocking)
{
	char* channel_A =	(char*)UCS2ToUTF8StringAlloc(channel);
	char* password_A = 	(char*)UCS2ToUTF8StringAlloc(password);
	chatEnterChannelA(chat, channel_A, password_A, callbacks, callback, param, blocking);
	gsifree(channel_A);
	gsifree(password_A);
}
#endif

void chatLeaveChannelA(CHAT chat,
					  const char * channel,
					  const char * reason)
{
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();

	if(!reason)
		reason = "";

	ciSocketSendf(&connection->chatSocket, "PART %s :%s", channel, reason);

	// Left the channel.
	////////////////////
	ciChannelLeft(chat, channel);
}
#ifdef GSI_UNICODE
void chatLeaveChannelW(CHAT chat,
					  const unsigned short * channel,
					  const unsigned short* reason)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* reason_A = (char*)UCS2ToUTF8StringAlloc(reason);
	chatLeaveChannelA(chat, channel_A, reason_A);
	gsifree(channel_A);
	gsifree(reason_A);
}
#endif

void chatSendChannelMessageA(CHAT chat,
							const char * channel,
							const char * message,
							int type)
{
	chatChannelCallbacks * callbacks;
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	ASSERT_TYPE(type);

	if (!message || !message[0])
		return;
	if(type == CHAT_MESSAGE)
		ciSocketSendf(&connection->chatSocket, "PRIVMSG %s :%s", channel, message);
	else if(type == CHAT_ACTION)
		ciSocketSendf(&connection->chatSocket, "PRIVMSG %s :\001ACTION %s\001", channel, message);
	else if(type == CHAT_NOTICE)
		ciSocketSendf(&connection->chatSocket, "NOTICE %s :%s", channel, message);
	else if(type == CHAT_UTM)
		ciSocketSendf(&connection->chatSocket, "UTM %s :%s", channel, message);
	else if(type == CHAT_ATM)
		ciSocketSendf(&connection->chatSocket, "ATM %s :%s", channel, message);
	else
		return;

	// We don't get these back, so call the callbacks.
	//////////////////////////////////////////////////
	callbacks = ciGetChannelCallbacks(chat, channel);
	if(callbacks != NULL)
	{
		ciCallbackChannelMessageParams params;
		params.channel = (char *)channel;
		params.user = connection->nick;
		params.message = (char *)message;
		params.type = type;
		ciAddCallback(chat, CALLBACK_CHANNEL_MESSAGE, (void*)callbacks->channelMessage, &params, callbacks->param, 0, channel);
	}
}
#ifdef GSI_UNICODE
void chatSendChannelMessageW(CHAT chat,
							const unsigned short * channel,
							const unsigned short * message,
							int type)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* message_A = (char*)UCS2ToUTF8StringAlloc(message);
	chatSendChannelMessageA(chat, channel_A, message_A, type);
	gsifree(channel_A);
	gsifree(message_A);
}
#endif

void chatSetChannelTopicA(CHAT chat,
						 const char * channel,
						 const char * topic)
{
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();

	if(topic == NULL)
		topic = "";

	ciSocketSendf(&connection->chatSocket, "TOPIC %s :%s", channel, topic);
}
#ifdef GSI_UNICODE
void chatSetChannelTopicW(CHAT chat,
						 const unsigned short * channel,
						 const unsigned short * topic)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* topic_A = (char*)UCS2ToUTF8StringAlloc(topic);
	chatSetChannelTopicA(chat, channel_A, topic_A);
	gsifree(channel_A);
	gsifree(topic_A);
}
#endif

void chatGetChannelTopicA(CHAT chat,
						 const char * channel,
						 chatGetChannelTopicCallback callback,
						 void * param,
						 CHATBool blocking)
{
	int ID;
	const char * topic;
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	assert(callback != NULL);

	// Check if we already have the topic.
	//////////////////////////////////////
	topic = ciGetChannelTopic(chat, channel);
	if(topic)
	{
		ciCallbackGetChannelTopicParams params;

		ID = ciGetNextID(chat);

		params.success = CHATTrue;
		params.channel = (char *)channel;
		params.topic = (char *)topic;
		ciAddCallback(chat, CALLBACK_GET_CHANNEL_TOPIC, (void*)callback, &params, param, ID, channel);
	}
	else
	{
		ciSocketSendf(&connection->chatSocket, "TOPIC %s", channel);

		ID = ciAddTOPICFilter(chat, channel, callback, param);
	}

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatGetChannelTopicW(CHAT chat,
						 const unsigned short * channel,
						 chatGetChannelTopicCallback callback,
						 void * param,
						 CHATBool blocking)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	chatGetChannelTopicA(chat, channel_A, callback, param, blocking);
	gsifree(channel_A);
}
#endif

void chatSetChannelModeA(CHAT chat,
						const char * channel,
						CHATChannelMode * mode)
{
	char buffer[64];

	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	assert(mode != NULL);

	// Build the mode string.
	/////////////////////////
	strcpy(buffer, "XiXpXsXmXnXtXlXe");
	if(mode->InviteOnly)
		buffer[0] = '+';
	else
		buffer[0] = '-';
	if(mode->Private)
		buffer[2] = '+';
	else
		buffer[2] = '-';
	if(mode->Secret)
		buffer[4] = '+';
	else
		buffer[4] = '-';
	if(mode->Moderated)
		buffer[6] = '+';
	else
		buffer[6] = '-';
	if(mode->NoExternalMessages)
		buffer[8] = '+';
	else
		buffer[8] = '-';
	if(mode->OnlyOpsChangeTopic)
		buffer[10] = '+';
	else
		buffer[10] = '-';
	if(mode->Limit > 0)
		buffer[12] = '+';
	else
		buffer[12] = '-';
	if(mode->OpsObeyChannelLimit)
		buffer[14] = '+';
	else
		buffer[14] = '-';

	// Add limit if needed.
	///////////////////////
	if(mode->Limit > 0)
		sprintf(&buffer[strlen(buffer)], " %d", mode->Limit);

	ciSocketSendf(&connection->chatSocket, "MODE %s %s", channel, buffer);
}
#ifdef GSI_UNICODE
void chatSetChannelModeW(CHAT chat,
						const unsigned short * channel,
						CHATChannelMode * mode)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	chatSetChannelModeA(chat, channel_A, mode);
	gsifree(channel_A);
}
#endif

void chatGetChannelModeA(CHAT chat,
						const char * channel,
						chatGetChannelModeCallback callback,
						void * param,
						CHATBool blocking)
{
	int ID;
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	assert(callback != NULL);

	// Are we in this channel?
	//////////////////////////
	if(ciInChannel(chat, channel))
	{
		CHATChannelMode mode;

		// Get the mode locally.
		////////////////////////
		if(ciGetChannelMode(chat, channel, &mode))
		{
			ciCallbackGetChannelModeParams params;

			// Get an ID.
			/////////////
			ID = ciGetNextID(chat);

			// Add the callback.
			////////////////////
			params.success = CHATTrue;
			params.channel = (char *)channel;
			params.mode = &mode;
			ciAddCallback(chat, CALLBACK_GET_CHANNEL_MODE, (void*)callback, &params, param, ID, NULL);

			CI_DO_BLOCKING;

			return;
		}
	}

	ciSocketSendf(&connection->chatSocket, "MODE %s", channel);

	ID = ciAddCMODEFilter(chat, channel, callback, param);

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatGetChannelModeW(CHAT chat,
						const unsigned short * channel,
						chatGetChannelModeCallback callback,
						void * param,
						CHATBool blocking)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	chatGetChannelModeA(chat, channel_A, callback, param, blocking);
	gsifree(channel_A);
}
#endif

void chatSetChannelPasswordA(CHAT chat,
							const char * channel,
							CHATBool enable,
							const char * password)
{
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	ASSERT_PASSWORD();

	if(enable)
		ciSocketSendf(&connection->chatSocket, "MODE %s +k %s", channel, password);
	else
		ciSocketSendf(&connection->chatSocket, "MODE %s -k %s", channel, password);
}
#ifdef GSI_UNICODE
void chatSetChannelPasswordW(CHAT chat,
							const unsigned short * channel,
							CHATBool enable,
							const unsigned short * password)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* password_A = (char*)UCS2ToUTF8StringAlloc(password);
	chatSetChannelPasswordA(chat, channel_A, enable, password_A);
	gsifree(channel_A);
	gsifree(password_A);
}
#endif

void chatGetChannelPasswordA(CHAT chat,
							const char * channel,
							chatGetChannelPasswordCallback callback,
							void * param,
							CHATBool blocking)
{
	ciCallbackGetChannelPasswordParams params;
	const char * password;
	int ID;
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	assert(callback != NULL);

	// Check that we're in the channel.
	///////////////////////////////////
	if(!ciInChannel(chat, channel))
		return; //ERRCON

	// Get the password.
	////////////////////
	password = ciGetChannelPassword(chat, channel);
	assert(password != NULL);

	// Get an ID.
	/////////////
	ID = ciGetNextID(chat);

	// Add the callback.
	////////////////////
	params.success = CHATTrue;
	params.channel = (char *)channel;
	params.enabled = CHATTrue;
	params.password = (char *)password;
	ciAddCallback(chat, CALLBACK_GET_CHANNEL_PASSWORD, (void*)callback, &params, param, ID, NULL);

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatGetChannelPasswordW(CHAT chat,
							const unsigned short * channel,
							chatGetChannelPasswordCallback callback,
							void * param,
							CHATBool blocking)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	chatGetChannelPasswordA(chat, channel_A, callback, param, blocking);
	gsifree(channel_A);
}
#endif

void chatSetChannelLimitA(CHAT chat,
						 const char * channel,
						 int limit)
{
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	assert(limit >= 0);

	if(limit)
		ciSocketSendf(&connection->chatSocket, "MODE %s +l %d", channel, limit);
	else
		ciSocketSendf(&connection->chatSocket, "MODE %s -l", channel);
}
#ifdef GSI_UNICODE
void chatSetChannelLimitW(CHAT chat,
						 const unsigned short * channel,
						 int limit)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	chatSetChannelLimitA(chat, channel_A, limit);
	gsifree(channel_A);
}
#endif

void chatEnumChannelBansA(CHAT chat,
						 const char * channel,
						 chatEnumChannelBansCallback callback,
						 void * param,
						 CHATBool blocking)
{
	int ID;
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	assert(callback != NULL);

	ciSocketSendf(&connection->chatSocket, "MODE %s +b", channel);

	ID = ciAddGETBANFilter(chat, channel, callback, param);

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatEnumChannelBansW(CHAT chat,
						 const unsigned short * channel,
						 chatEnumChannelBansCallback callback,
						 void * param,
						 CHATBool blocking)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	chatEnumChannelBansA(chat, channel_A, callback, param, blocking);
	gsifree(channel_A);
}
#endif

void chatAddChannelBanA(CHAT chat,
					   const char * channel,
					   const char * ban)
{
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	ASSERT_BAN();

	ciSocketSendf(&connection->chatSocket, "MODE %s +b %s", channel, ban);
}
#ifdef GSI_UNICODE
void chatAddChannelBanW(CHAT chat,
					   const unsigned short * channel,
					   const unsigned short * ban)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* ban_A = (char*)UCS2ToUTF8StringAlloc(ban);
	chatAddChannelBanA(chat, channel_A, ban_A);
	gsifree(channel_A);
	gsifree(ban_A);
}
#endif

void chatRemoveChannelBanA(CHAT chat,
						  const char * channel,
						  const char * ban)
{
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	ASSERT_BAN();

	ciSocketSendf(&connection->chatSocket, "MODE %s -b %s", channel, ban);
}
#ifdef GSI_UNICODE
void chatRemoveChannelBanW(CHAT chat,
						  const unsigned short * channel,
						  const unsigned short * ban)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* ban_A = (char*)UCS2ToUTF8StringAlloc(ban);
	chatRemoveChannelBanA(chat, channel_A, ban_A);
	gsifree(channel_A);
	gsifree(ban_A);
}
#endif

void chatSetChannelGroupA(CHAT chat,
						 const char * channel,
						 const char * group)
{
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();

	// No way to clear the group.
	/////////////////////////////
	if(!group || !group[0])
		return;

	ciSocketSendf(&connection->chatSocket, "SETGROUP %s %s", channel, group);
}
#ifdef GSI_UNICODE
void chatSetChannelGroupW(CHAT chat,
						  const unsigned short * channel,
						  const unsigned short* group)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* group_A = (char*)UCS2ToUTF8StringAlloc(group);
	chatSetChannelGroupA(chat, channel_A, group_A);
	gsifree(channel_A);
	gsifree(group_A);
}
#endif

int chatGetChannelNumUsersA(CHAT chat,
						   const char * channel)
{
	CONNECTION;
	if(!connection->connected)
		return -1;

	ASSERT_CHANNEL();

	if(!channel || !channel[0])
		return -1;

	if(!ciInChannel(chat, channel))
		return -1;

	return ciGetChannelNumUsers(chat, channel);
}
#ifdef GSI_UNICODE
int chatGetChannelNumUsersW(CHAT chat,
						   const unsigned short * channel)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	int result = chatGetChannelNumUsersA(chat, channel_A);
	gsifree(channel_A);

	return result;
}
#endif

CHATBool chatInChannelA(CHAT chat,
					   const char * channel)

{
	CONNECTION;
	if(!connection->connected)
		return CHATFalse;

	ASSERT_CHANNEL();

	if(!channel || !channel[0])
		return CHATFalse;

	return ciInChannel(chat, channel);
}
#ifdef GSI_UNICODE
CHATBool chatInChannelW(CHAT chat,
					    const unsigned short * channel)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	CHATBool result = chatInChannelA(chat, channel_A);
	gsifree(channel_A);

	return result;
}
#endif


/**********
** USERS **
**********/
static void ciEnumUsersCallback(CHAT chat, const char * channel, int numUsers, const char ** users, int * modes, void * param)
{
	ciEnumUsersData * data;
	CONNECTION;

	// Check the args.
	//////////////////
	ASSERT_CHANNEL();
	assert(numUsers >= 0);
#ifdef _DEBUG
	{
	int i;
	if(numUsers > 0)
	{
		assert(users != NULL);
		assert(modes != NULL);
	}
	for(i = 0 ; i < numUsers ; i++)
	{
		ASSERT_USER(users[i]);
		ASSERT_TYPE(modes[i]);
	}
	}
#endif
	assert(param != NULL);

	// Get the data.
	////////////////
	data = (ciEnumUsersData *)param;
	assert(data->callback != NULL);

	// Call the callback directly.
	//////////////////////////////
#ifdef GSI_UNICODE
	{
		unsigned short* channel_W = UTF8ToUCS2StringAlloc(channel);
		unsigned short** users_W = UTF8ToUCS2StringArrayAlloc(users, numUsers);
		data->callback(chat, CHATTrue, channel_W, numUsers, (const unsigned short**)users_W, modes, data->param);
		gsifree(channel_W);
		while(numUsers-- > 0)
			gsifree(users_W[numUsers]);
		gsifree(users_W);
	}
#else
	data->callback(chat, CHATTrue, channel, numUsers, users, modes, data->param);
#endif
}

void chatEnumUsersA(CHAT chat,
				   const char * channel,
				   chatEnumUsersCallback callback,
				   void * param,
				   CHATBool blocking)
{
	int ID;
	ciEnumUsersData data;
	CONNECTION;
	CONNECTED;

	//ASSERT_CHANNEL();
	assert(callback != NULL);

	if(channel == NULL)
		channel = "";

	// Is there a channel specified?
	////////////////////////////////
	if(channel[0] != '\0')
	{
		// Check if we have this one locally.
		/////////////////////////////////////
		if(ciInChannel(chat, channel))
		{
			// Get the users in the channel.
			////////////////////////////////
			data.callback = callback;
			data.param = param;
			ciChannelListUsers(chat, channel, ciEnumUsersCallback, &data);

			return;
		}
	}

	ciSocketSendf(&connection->chatSocket, "NAMES %s", channel);

	// Channel needs to be empty, not NULL, for the filter.
	///////////////////////////////////////////////////////
	if(!channel[0])
		channel = NULL;

	ID = ciAddNAMESFilter(chat, channel, callback, param);
	
	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatEnumUsersW(CHAT chat,
				   const unsigned short * channel,
				   chatEnumUsersCallback callback,
				   void * param,
				   CHATBool blocking)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	chatEnumUsersA(chat, channel_A, callback, param, blocking);
	gsifree(channel_A);
}
#endif


// Enumerates the channels that we are joined to
//////////////////////////////////////////////////////
void chatEnumJoinedChannels(CHAT chat,
					  chatEnumJoinedChannelsCallback callback,
					  void * param)
{
	ciEnumJoinedChannels(chat, callback, param);
}

void chatSendUserMessageA(CHAT chat,
						 const char * user,
						 const char * message,
						 int type)
{
	CONNECTION;
	CONNECTED;

	ASSERT_USER(user);
	ASSERT_TYPE(type);

	if (!message || message[0] == 0)
		return;	

	if(type == CHAT_MESSAGE)
		ciSocketSendf(&connection->chatSocket, "PRIVMSG %s :%s", user, message);
	else if(type == CHAT_ACTION)
		ciSocketSendf(&connection->chatSocket, "PRIVMSG %s :\001ACTION %s\001", user, message);
	else if(type == CHAT_NOTICE)
		ciSocketSendf(&connection->chatSocket, "NOTICE %s :%s", user, message);
	else if(type == CHAT_UTM)
		ciSocketSendf(&connection->chatSocket, "UTM %s :%s", user, message);
	else if(type == CHAT_ATM)
		ciSocketSendf(&connection->chatSocket, "ATM %s :%s", user, message);
}
#ifdef GSI_UNICODE
void chatSendUserMessageW(CHAT chat,
						 const unsigned short * user,
						 const unsigned short * message,
						 int type)
{
	char* user_A = (char*)UCS2ToUTF8StringAlloc(user);
	char* message_A = (char*)UCS2ToUTF8StringAlloc(message);
	chatSendUserMessageA(chat, user_A, message_A, type);
	gsifree(user_A);
	gsifree(message_A);
}
#endif

void chatGetUserInfoA(CHAT chat,
					 const char * user,
					 chatGetUserInfoCallback callback,
					 void * param,
					 CHATBool blocking)
{
	int ID;
	CONNECTION;
	CONNECTED;

	ASSERT_USER(user);
	assert(callback != NULL);

	ciSocketSendf(&connection->chatSocket, "WHOIS %s", user);

	ID = ciAddWHOISFilter(chat, user, callback, param);

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatGetUserInfoW(CHAT chat,
					 const unsigned short * user,
					 chatGetUserInfoCallback callback,
					 void * param,
					 CHATBool blocking)
{
	char* user_A = (char*)UCS2ToUTF8StringAlloc(user);
	chatGetUserInfoA(chat, user_A, callback, param, blocking);
	gsifree(user_A);
}
#endif

void chatGetBasicUserInfoA(CHAT chat,
						  const char * nick,
						  chatGetBasicUserInfoCallback callback,
						  void * param,
						  CHATBool blocking)
{
	int ID;
	const char * user;
	const char * address;
	CONNECTION;
	CONNECTED;

	ASSERT_USER(nick);
	assert(callback != NULL);

	// Check if we already have it.
	///////////////////////////////
	if(ciGetUserBasicInfoA(chat, nick, &user, &address))
	{
		ciCallbackGetBasicUserInfoParams params;

		params.success = CHATTrue;
		params.nick = (char *)nick;
		params.user = (char *)user;
		params.address = (char *)address;

		ID = ciGetNextID(chat);

		ciAddCallback(chat, CALLBACK_GET_BASIC_USER_INFO, (void*)callback, &params, param, ID, NULL);
	}
	else
	{
		ciSocketSendf(&connection->chatSocket, "WHO %s", nick);

		ID = ciAddWHOFilter(chat, nick, callback, param);
	}

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatGetBasicUserInfoW(CHAT chat,
						  const unsigned short * nick,
						  chatGetBasicUserInfoCallback callback,
						  void * param,
						  CHATBool blocking)
{
	char* nick_A = (char*)UCS2ToUTF8StringAlloc(nick);
	chatGetBasicUserInfoA(chat, nick_A, callback, param, blocking);
	gsifree(nick_A);
}
#endif

CHATBool chatGetBasicUserInfoNoWaitA(CHAT chat,
									const char * nick,
									const char ** user,
									const char ** address)
{
	CONNECTION;
	// 2002.Feb.28.JED - added additional check, was blowing up in GSA
	if(!connection)
		return CHATFalse;
	if(!connection->connected)
		return CHATFalse;

	ASSERT_USER(nick);

	return ciGetUserBasicInfoA(chat, nick, user, address);
}
#ifdef GSI_UNICODE
CHATBool chatGetBasicUserInfoNoWaitW(CHAT chat,
									const unsigned short * nick, 
									const unsigned short ** user,
									const unsigned short ** address)
{
	char nick_A[MAX_NICK];

	CONNECTION;
	// 2002.Feb.28.JED - added additional check, was blowing up in GSA
	if(!connection)
		return CHATFalse;
	if(!connection->connected)
		return CHATFalse;

	assert(nick);

	UCS2ToAsciiString(nick, nick_A);
	return ciGetUserBasicInfoW(chat, nick_A, user, address);
}
#endif

void chatGetChannelBasicUserInfoA(CHAT chat,
								 const char * channel,
								 chatGetChannelBasicUserInfoCallback callback,
								 void * param,
								 CHATBool blocking)
{
	int ID;
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	assert(callback != NULL);

	ciSocketSendf(&connection->chatSocket, "WHO %s", channel);

	ID = ciAddCWHOFilter(chat, channel, callback, param);

	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatGetChannelBasicUserInfoW(CHAT chat,
								 const unsigned short * channel,
								 chatGetChannelBasicUserInfoCallback callback,
								 void * param,
								 CHATBool blocking)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	chatGetChannelBasicUserInfoA(chat, channel_A, callback, param, blocking);
	gsifree(channel_A);
}
#endif

void chatInviteUserA(CHAT chat,
					const char * channel,
					const char * user)
{
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	ASSERT_USER(user);

	ciSocketSendf(&connection->chatSocket, "INVITE %s %s", user, channel);
}
#ifdef GSI_UNICODE
void chatInviteUserW(CHAT chat,
					const unsigned short * channel,
					const unsigned short * user)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* user_A = (char*)UCS2ToUTF8StringAlloc(user);
	chatInviteUserA(chat, channel_A, user_A);
	gsifree(channel_A);
	gsifree(user_A);
}
#endif

void chatKickUserA(CHAT chat,
				  const char * channel,
				  const char * user,
				  const char * reason)
{
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	ASSERT_USER(user);

	if(reason == NULL)
		reason = "";

	ciSocketSendf(&connection->chatSocket, "KICK %s %s :%s", channel, user, reason);
}
#ifdef GSI_UNICODE
void chatKickUserW(CHAT chat,
				  const unsigned short * channel,
				  const unsigned short * user,
				  const unsigned short * reason)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* user_A = (char*)UCS2ToUTF8StringAlloc(user);
	char* reason_A = (char*)UCS2ToUTF8StringAlloc(reason);
	chatKickUserA(chat, channel_A, user_A, reason_A);
	gsifree(channel_A);
	gsifree(user_A);
	gsifree(reason_A);
}
#endif

void chatBanUserA(CHAT chat,
				 const char * channel,
				 const char * user)
{
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	ASSERT_USER(user);

	ciSocketSendf(&connection->chatSocket, "WHOIS %s", user);

	ciAddBANFilter(chat, user, channel);
}
#ifdef GSI_UNICODE
void chatBanUserW(CHAT chat,
				  const unsigned short * channel,
				  const unsigned short * user)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* user_A = (char*)UCS2ToUTF8StringAlloc(user);
	chatBanUserA(chat, channel_A, user_A);
	gsifree(channel_A);
	gsifree(user_A);
}
#endif

void chatSetUserModeA(CHAT chat,
					 const char * channel,
					 const char * user,
					 int mode)
{
	int sign;

	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	ASSERT_USER(user);
	ASSERT_TYPE(mode);

	sign = (mode & CHAT_OP)?'+':'-';
	ciSocketSendf(&connection->chatSocket, "MODE %s %co %s", channel, sign, user);

	sign = (mode & CHAT_VOICE)?'+':'-';
	ciSocketSendf(&connection->chatSocket, "MODE %s %cv %s", channel, sign, user);
}
#ifdef GSI_UNICODE
void chatSetUserModeW(CHAT chat,
					 const unsigned short * channel,
					 const unsigned short * user,
					 int mode)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* user_A = (char*)UCS2ToUTF8StringAlloc(user);
	chatSetUserModeA(chat, channel_A, user_A, mode);
	gsifree(channel_A);
	gsifree(user_A);
}
#endif

void chatGetUserModeA(CHAT chat,
					 const char * channel,
					 const char * user,
					 chatGetUserModeCallback callback,
					 void * param,
					 CHATBool blocking)
{
	int ID;
	int mode;
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	ASSERT_USER(user);
	assert(callback != NULL);

	// Get the mode.
	////////////////
	mode = ciGetUserMode(chat, channel, user);
	if(mode != -1)
	{
		ciCallbackGetUserModeParams params;
		params.success = CHATTrue;
		params.channel = (char *)channel;
		params.user = (char *)user;
		params.mode = mode;

		ID = ciGetNextID(chat);
		ciAddCallback(chat, CALLBACK_GET_USER_MODE, (void*)callback, &params, param, ID, NULL);

		CI_DO_BLOCKING;
	}

	ciSocketSendf(&connection->chatSocket, "WHO %s", user);

	ID = ciAddUMODEFilter(chat, user, channel, callback, param);
	
	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatGetUserModeW(CHAT chat,
					 const unsigned short * channel,
					 const unsigned short * user,
					 chatGetUserModeCallback callback,
					 void * param,
					 CHATBool blocking)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* user_A = (char*)UCS2ToUTF8StringAlloc(user);
	chatGetUserModeA(chat, channel_A, user_A, callback, param, blocking);
	gsifree(channel_A);
	gsifree(user_A);
}
#endif

CHATBool chatGetUserModeNoWaitA(CHAT chat,
							   const char * channel,
							   const char * user,
							   int * mode)
{
	CONNECTION;
	if(!connection->connected)
		return CHATFalse;

	ASSERT_CHANNEL();
	ASSERT_USER(user);
	assert(mode);

	// Get the mode.
	////////////////
	*mode = ciGetUserMode(chat, channel, user);

	return (CHATBool)(*mode != -1);
}
#ifdef GSI_UNICODE
CHATBool chatGetUserModeNoWaitW(CHAT chat,
							   const unsigned short * channel,
							   const unsigned short  * user,
							   int * mode)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	char* user_A	= (char*)UCS2ToUTF8StringAlloc(user);
	CHATBool result = chatGetUserModeNoWaitA(chat, channel_A, user_A, mode);
	gsifree(channel_A);
	gsifree(user_A);
	return result;
}
#endif


void chatGetUdpRelayA(CHAT chat,
					 const char * channel,
					 chatGetUdpRelayCallback callback,
					 void * param,
					 CHATBool blocking)
{
	int ID;
	CONNECTION;
	CONNECTED;

	ASSERT_CHANNEL();
	assert(callback != NULL);

	ciSocketSendf(&connection->chatSocket, "GETUDPRELAY %s", channel);

	ID = ciAddGETUDPRELAYFilter(chat, channel, callback, param);

	CI_DO_BLOCKING;
}

#ifdef GSI_UNICODE
void chatGetUdpRelayW(CHAT chat,
					 const unsigned short * channel,
					 chatGetUdpRelayCallback callback,
					 void * param,
					 CHATBool blocking)
{
	char* channel_A = (char*)UCS2ToUTF8StringAlloc(channel);
	chatGetUdpRelayA(chat, channel_A, callback, param, blocking);
	gsifree(channel_A);
}
#endif

/*********
** KEYS **
*********/
void chatSetGlobalKeysA(CHAT chat,
					   int num,
					   const char ** keys,
					   const char ** values)
{
	char buffer[512];
	const char * key;
	const char * value;
	int i;
	CONNECTION;
	CONNECTED;

	if(!keys || !values)
		return;

	strcpy(buffer, "SETKEY :");
	for(i = 0 ; i < num ; i++)
	{
		key = keys[i];
		if(!key || !key[0])
			return;
		value = values[i];
		if(!value)
			value = "";
		sprintf(buffer + strlen(buffer), "\\%s\\%s", key, value);
	}

	ciSocketSend(&connection->chatSocket, buffer);
}
#ifdef GSI_UNICODE
void chatSetGlobalKeysW(CHAT chat,
					   int num,
					   const unsigned short ** keys,
					   const unsigned short ** values)
{
	char** keys_A	= (char**)UCS2ToUTF8StringArrayAlloc(keys, num);
	char** values_A = (char**)UCS2ToUTF8StringArrayAlloc(values, num);
	int i = 0;
	chatSetGlobalKeysA(chat, num, (const char**)keys_A, (const char**)values_A);
	for (; i < num; i++)
	{
		gsifree(keys_A[i]);
		gsifree(values_A[i]);
	}
	gsifree(keys_A);
	gsifree(values_A);
}
#endif

static char * ciRandomCookie()
{
	static char cookie[4];
	static int nextCookie = 0;

	sprintf(cookie, "%03d", nextCookie++);
	nextCookie %= 1000;

	return cookie;
}

static void ciSendGetKey(CHAT chat,
						 const char * target,
						 const char * cookie,
						 int num,
						 const char ** keys)
{
	char buffer[512];
	int len;
	int i;
	int j;
	int keyLen;

	CONNECTION;

	assert(target && target[0]);
	assert(cookie && cookie[0]);
	assert(num >= 1);
	assert(keys);

	// Start off the buffer.
	////////////////////////
	sprintf(buffer, "GETKEY %s %s 0 :", target, cookie);
	len = (int)strlen(buffer);

	// Add the keys.
	////////////////
	for(i = 0 ; i < num ; i++)
	{
		// Check for a blank.
		/////////////////////
		if(!keys[i] || !keys[i][0])
			continue;

		// Check lengths.
		/////////////////
		keyLen = (int)strlen(keys[i]);
		if((len + keyLen + 1) >= (int)sizeof(buffer))
			return;

		// Add the key.
		///////////////
		buffer[len++] = '\\';
		memcpy(buffer + len, keys[i], (unsigned int)keyLen);
		for(j = len ; j < (len + keyLen) ; j++)
			if(buffer[j] == '\\')
				buffer[j] = '/';
		len += keyLen;
		buffer[len] = '\0';
	}

	// Send it.
	///////////
	ciSocketSend(&connection->chatSocket, buffer);
}

void chatGetGlobalKeysA(CHAT chat,
					   const char * target,
					   int num,
					   const char ** keys,
					   chatGetGlobalKeysCallback callback,
					   void * param,
					   CHATBool blocking)
{
	char * cookie;
	const char * channel;
	int ID;
	CONNECTION;
	CONNECTED;

	assert(num >= 0);
	assert(keys);

	if(!target || !target[0])
		target = connection->nick;

	// Get a cookie.
	////////////////
	cookie = ciRandomCookie();

	// Send the request.
	////////////////////
	ciSendGetKey(chat, target, cookie, num, keys);

	// Check if this is a channel or a user.
	////////////////////////////////////////
	if(target[0] == '#')
		channel = target;
	else
		channel = NULL;

	ID = ciAddGETKEYFilter(chat, cookie, num, keys, channel, callback, param);
	
	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatGetGlobalKeysW(CHAT chat,
					   const unsigned short * target,
					   int num,
					   const unsigned short ** keys,
					   chatGetGlobalKeysCallback callback,
					   void * param,
					   CHATBool blocking)
{
	char* target_A;
	char** keys_A;
	int i = 0;

	assert(target);
	assert(keys);

	target_A	= (char*)UCS2ToUTF8StringAlloc(target);
	keys_A		= (char**)UCS2ToUTF8StringArrayAlloc(keys, num);

	chatGetGlobalKeysA(chat, target_A, num, (const char**)keys_A, callback, param, blocking);
	gsifree(target_A);

	for (; i < num; i++)
		gsifree(keys_A[i]);
	gsifree(keys_A);
}
#endif

void chatSetChannelKeysA(CHAT chat,
						const char * channel,
						const char * user,
						int num,
						const char ** keys,
						const char ** values)
{
	char buffer[512];
	const char * value;
	int i;
	CONNECTION;
	CONNECTED;

	if(!user || !user[0])
		sprintf(buffer, "SETCHANKEY %s :", channel);
	else
		sprintf(buffer, "SETCKEY %s %s :", channel, user);
	for(i = 0 ; i < num ; i++)
	{
		value = values[i];
		if(!value)
			value = "";
		sprintf(buffer + strlen(buffer), "\\%s\\%s", keys[i], value);
	}

	ciSocketSend(&connection->chatSocket, buffer);
}
#ifdef GSI_UNICODE
void chatSetChannelKeysW(CHAT chat,
						const unsigned short * channel,
						const unsigned short * user,
						int num,
						const unsigned short ** keys,
						const unsigned short ** values)
{
	char* channel_A;
	char* user_A;
	char** keys_A;
	char** values_A;
	int i = 0;

	channel_A	= (char*)UCS2ToUTF8StringAlloc(channel);
	user_A		= (char*)UCS2ToUTF8StringAlloc(user);
	keys_A		= (char**)UCS2ToUTF8StringArrayAlloc(keys, num);
	values_A	= (char**)UCS2ToUTF8StringArrayAlloc(values, num);

	chatSetChannelKeysA(chat, channel_A, user_A, num, (const char**)keys_A, (const char**)values_A);

	gsifree(channel_A);
	gsifree(user_A);

	for (; i < num; i++)
	{
		gsifree(keys_A[i]);
		gsifree(values_A[i]);
	}

	gsifree(keys_A);
	gsifree(values_A);
}
#endif

static CHATBool ciSendGetChannelKey(CHAT chat,
									const char * channel,
									const char * nick,
									const char * cookie,
									int num,
									const char ** keys)
{
	char buffer[512];
	int len;
	int i;
	int j;
	int keyLen;
	CHATBool getBrocastKeys = CHATFalse;

	CONNECTION;

	assert(channel && channel[0]);
	assert(cookie && cookie[0]);
	assert(!num || keys);

	// Start off the buffer.
	////////////////////////
	if(!nick || !nick[0])
		sprintf(buffer, "GETCHANKEY %s %s 0 :", channel, cookie);
	else
		sprintf(buffer, "GETCKEY %s %s %s 0 :", channel, nick, cookie);
	len = (int)strlen(buffer);

	// Add the keys.
	////////////////
	for(i = 0 ; i < num ; i++)
	{
		// Check for a blank.
		/////////////////////
		if(!keys[i] || !keys[i][0])
			continue;

		// Check for b_*.
		/////////////////
		if(strcmp(keys[i], "b_*") == 0)
		{
			getBrocastKeys = CHATTrue;
			continue;
		}

		// Check lengths.
		/////////////////
		keyLen = (int)strlen(keys[i]);
		if((len + keyLen + 1) >= (int)sizeof(buffer))
			continue;

		// Add the key.
		///////////////
		buffer[len++] = '\\';
		memcpy(buffer + len, keys[i], (unsigned int)keyLen);
		for(j = len ; j < (len + keyLen) ; j++)
			if(buffer[j] == '\\')
				buffer[j] = '/';
		len += keyLen;
		buffer[len] = '\0';
	}

	// Check for broadcast keys.
	////////////////////////////
	if(getBrocastKeys)
	{
		if((len + 4) < (int)sizeof(buffer))
		{
			strcpy(buffer + len, "\\b_*");
			len += 4;
		}
	}
	
	// Check for requesting all keys on a room.
	///////////////////////////////////////////
	if(!num && (!nick || !nick[0]))
	{
		strcpy(buffer + len, "*");
		len++;
	}

	// Send it.
	///////////
	ciSocketSend(&connection->chatSocket, buffer);

	return getBrocastKeys;
}

void chatGetChannelKeysA(CHAT chat,
						const char * channel,
						const char * user,
						int num,
						const char ** keys,
						chatGetChannelKeysCallback callback,
						void * param,
						CHATBool blocking)
{
	char * cookie;
	int ID;
	CHATBool getBroadcastKeys;
	CONNECTION;
	CONNECTED;

	assert(num >= 0);
	assert(!num || keys);

	// Get a cookie.
	////////////////
	cookie = ciRandomCookie();

	// Send the request.
	////////////////////
	getBroadcastKeys = ciSendGetChannelKey(chat, channel, user, cookie, num, keys);

	if(!user || !user[0])
		ID = ciAddGETCHANKEYFilter(chat, cookie, num, keys, getBroadcastKeys, callback, param);
	else
		ID = ciAddGETCKEYFilter(chat, cookie, num, keys, (CHATBool)(strcmp(user, "*") == 0), getBroadcastKeys, callback, param);
	
	CI_DO_BLOCKING;
}
#ifdef GSI_UNICODE
void chatGetChannelKeysW(CHAT chat,
						const unsigned short * channel,
						const unsigned short * user,
						int num,
						const unsigned short ** keys,
						chatGetChannelKeysCallback callback,
						void * param,
						CHATBool blocking)
{
	char* channel_A;
	char* user_A;
	char** keys_A;
	int i = 0;

	channel_A	= (char*)UCS2ToUTF8StringAlloc(channel);
	user_A		= (char*)UCS2ToUTF8StringAlloc(user);
	keys_A		= (char**)UCS2ToUTF8StringArrayAlloc(keys, num);

	chatGetChannelKeysA(chat, channel_A, user_A, num, (const char**)keys_A, callback, param, blocking);

	gsifree(channel_A);
	gsifree(user_A);

	for (; i < num; i++)
		gsifree(keys_A[i]);

	gsifree(keys_A);
}
#endif

// Check if a given nickname is valid.  Looks for illegal IRC characters.
// [in] nick		-  The nickname to validate
int ciNickIsValid(const char* nick)
{
	if (strlen(nick) >= MAX_CHAT_NICK)
		return CHAT_NICK_TOO_LONG;

	// Empty nick is invalid
	if ((NULL == nick) || ('\0' == *nick))
		return CHAT_INVALID;


	// 10-14-2004 Changed by Saad Nader
	// Using the nickname rules for unique nicks 
	// commented out the previous rules
	////////////////////////////////////////////////
	// Nick can't start with a number or a '+', '@', '#'
	//if(isdigit(*oldNick) || (*oldNick == '-'))

	if(*nick == '@' || *nick == '#' || *nick == '+' || *nick == ':')
		return CHAT_INVALID;

	// Make sure each character is valid
	while(*nick != '\0') 
	{
		// If the character isn't in the valid set, the nick is not valid
		if (NULL == strchr(VALID_NICK_CHARS,*nick++))
			return CHAT_INVALID;
	}
	
	return CHAT_NICK_OK;
}

/****************
** NICK ERRORS **
****************/
void ciNickError(CHAT chat, int type, const char * nick, int numSuggestedNicks, char ** suggestedNicks)
{
	CONNECTION;

	// Check if there's a nick-in-use callback.
	///////////////////////////////////////////
	if(connection->nickErrorCallback)
	{
		ciCallbackNickErrorParams params;

		// Add the callback.
		////////////////////
		memset(&params, 0, sizeof(ciCallbackNickErrorParams));
		params.type = type;
		params.nick = (char *)nick;
		params.numSuggestedNicks = numSuggestedNicks;
		params.suggestedNicks = suggestedNicks;
		ciAddCallback(chat, CALLBACK_NICK_ERROR, (void*)connection->nickErrorCallback, &params, connection->connectParam, 0, NULL);
	}
	else
	{
		// There's no callback, disconnect.
		///////////////////////////////////
		connection->connecting = CHATFalse;

		// Call the callback.
		/////////////////////
		if(connection->connectCallback != NULL)
			connection->connectCallback(chat, CHATFalse, CHAT_NICK_ERROR, connection->connectParam);
	}
}

