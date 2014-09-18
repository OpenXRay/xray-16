/*
gpiConnect.c
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4267) //lines: 379, 415, 739
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS


//INCLUDES
//////////
#include <stdio.h>
#include <stdlib.h>
#include "gpi.h"
#include <string.h>


//DEFINES
/////////
// Connection Manager Address.
//////////////////////////////
#define GPI_CONNECTION_MANAGER_NAME    "gpcm." GSI_DOMAIN_NAME
#define GPI_CONNECTION_MANAGER_PORT    29900

#define GPI_UDP_HEADER "gamespygp"
// Random String stuff.
///////////////////////
#define RANDSTRING      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
//this is off by one
//#define RANDOMCHAR()    (RANDSTRING[(rand() * sizeof(RANDSTRING)) / (RAND_MAX + 1)])
#define RANDOMCHAR()    (RANDSTRING[rand() % (sizeof(RANDSTRING) - 1)])

//GLOBALS
/////////
char GPConnectionManagerHostname[64] = GPI_CONNECTION_MANAGER_NAME;

//FUNCTIONS
///////////
static void randomString(
  char * buffer,
  int numChars
)
{
	int i;

	for(i = 0 ; i < numChars ; i++)
		buffer[i] = RANDOMCHAR();
	buffer[i] = '\0';
}

static GPResult
gpiStartConnect(
  GPConnection * connection,
  GPIOperation * operation
)
{
	struct sockaddr_in address;
	int rcode;
	//int len;
	GPIConnection * iconnection = (GPIConnection*)*connection;
	struct hostent * host;
	
	GSUdpErrorCode anError;
	strncpy(iconnection->mHeader, GPI_UDP_HEADER, GS_UDP_MSG_HEADER_LEN);

	if (!gsUdpEngineIsInitialized())
	{
		unsigned short peerPort = GPI_PEER_PORT;
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Notice, 
			"Initializing UDP Layer\n");
		anError = gsUdpEngineInitialize(peerPort, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		if (anError != GS_UDP_NO_ERROR)
		{
			while (anError != GS_UDP_NO_ERROR && peerPort < GPI_PEER_PORT + 100)
			{
				gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Comment, 
					"Port %d failed, trying next port\n", peerPort);
				anError = gsUdpEngineInitialize(++peerPort, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			}
			if (anError != GS_UDP_NO_ERROR)
			{
				gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_HotError, 
					"Tryed all 100 ports after default port, giving up.\n");
				CallbackFatalError(connection, GP_NETWORK_ERROR, GP_UDP_LAYER, 
					"There was error starting the UDP layer.");
			}
		}	
		if (!iconnection->firewall)
		{
			iconnection->peerPort = peerPort;
		}
	}
	else
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Notice, 
			"UDP Layer already initialized, using existing port.\n");
		iconnection->peerPort = gsUdpEngineGetLocalPort();
	}
	anError = gsUdpEngineAddMsgHandler(iconnection->mHeader, iconnection->mHeader, NULL, gpiPeerAcceptedCallback, gpiPeerLeftCallback, 
		gpiPeerPingReplyCallback, gpiPeerMessageCallback, connection);
	if (anError != GS_UDP_NO_ERROR)
	{
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_UDP_LAYER, "There was an error starting the UDP Layer.");
	}
	if(iconnection->firewall)
	{
		
		/*
		// Create the peer listening socket.
		////////////////////////////////////
		iconnection->peerSocket = socket(AF_INET, SOCK_STREAM, 0);
		if(iconnection->peerSocket == INVALID_SOCKET)
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error creating a socket.");

		// Make it non-blocking.
		////////////////////////
		rcode = SetSockBlocking(iconnection->peerSocket,0);
		if (rcode == 0)
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error making a socket non-blocking.");
		// Bind the socket.
		///////////////////
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		rcode = bind(iconnection->peerSocket, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
		if(gsiSocketIsError(rcode))
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error binding a socket.");

		// Start listening on the socket.
		/////////////////////////////////
		rcode = listen(iconnection->peerSocket, SOMAXCONN);
		if(gsiSocketIsError(rcode))
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error listening on a socket.");

		// Get the socket's port.
		/////////////////////////
		len = sizeof(struct sockaddr_in);
		rcode = getsockname(iconnection->peerSocket, (struct sockaddr *)&address, &len);

		if (gsiSocketIsError(rcode))
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error getting a socket's addres.");
		iconnection->peerPort = address.sin_port;
		*/
		
			
		iconnection->peerPort = 0;
	}
	/*
	else
	{
		// Deprecated TCP code; Replaced by UDP Layer
		// No local port.
		/////////////////
		//iconnection->peerSocket = INVALID_SOCKET;
		
		// Set to nothing because NN will determine this
		//////////////////////////
		//iconnection->peerPort = 0;
	}
	*/
	

	
	// Create the cm socket.
	////////////////////////
	iconnection->cmSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(iconnection->cmSocket == INVALID_SOCKET)
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error creating a socket.");

	// Make it non-blocking.
	////////////////////////
	rcode = SetSockBlocking(iconnection->cmSocket,0);
	if(rcode == 0)
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error making a socket non-blocking.");
/* 
	// Bind the socket.
	///////////////////
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	rcode = bind(iconnection->cmSocket, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
	if (gsiSocketIsError(rcode))
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error binding a socket.");
*/
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	// Get the server host.
	///////////////////////
	if (inet_addr(GPConnectionManagerHostname) == INADDR_NONE)
	{
		host = gethostbyname(GPConnectionManagerHostname);
		if(host == NULL)
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "Could not resolve connection mananger host name.");
		address.sin_addr.s_addr = *(unsigned int *)host->h_addr_list[0];
		//printf("Resolved Hostname and copied address:  %s\n", inet_ntoa(address.sin_addr));
	}
	else
	{
		address.sin_addr.s_addr = inet_addr(GPConnectionManagerHostname);
		//printf("Using hardcoded address: %s", GPConnectionManagerHostname);
	}

	// Connect the socket.
	//////////////////////
	assert(address.sin_addr.s_addr != 0);
	address.sin_port = htons(GPI_CONNECTION_MANAGER_PORT);
	rcode = connect(iconnection->cmSocket, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
	if (gsiSocketIsError(rcode))
	{
		int error = GOAGetLastError(iconnection->cmSocket);
		if((error != WSAEWOULDBLOCK) && (error != WSAEINPROGRESS) && (error != WSAETIMEDOUT))
		{
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error connecting a socket.");
		}
	}

	// We're waiting for the connect to complete.
	/////////////////////////////////////////////
	operation->state = GPI_CONNECTING;
	iconnection->connectState = GPI_CONNECTING;

	return GP_NO_ERROR;
}

GPResult
gpiConnect(
  GPConnection * connection,
  const char nick[GP_NICK_LEN],
  const char uniquenick[GP_UNIQUENICK_LEN],
  const char email[GP_EMAIL_LEN],
  const char password[GP_PASSWORD_LEN],
  const char authtoken[GP_AUTHTOKEN_LEN],
  const char partnerchallenge[GP_PARTNERCHALLENGE_LEN],
  const char cdkey[GP_CDKEY_LEN],
  GPEnum firewall,
  GPIBool newuser,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnectData * data;
	GPIOperation * operation;
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPResult result;

	// Reset if this connection was already used.
	/////////////////////////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		CHECK_RESULT(gpiReset(connection));

	// Error check.
	///////////////
	if(iconnection->connectState != GPI_NOT_CONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "Invalid connection.");

	// Get the firewall setting.
	////////////////////////////
#if defined(GS_WIRELESS_DEVICE)
	GSI_UNUSED(firewall);
	iconnection->firewall = GPITrue;
#else
	switch(firewall)
	{
	case GP_FIREWALL:
		iconnection->firewall = GPITrue;
		break;
	case GP_NO_FIREWALL:
		iconnection->firewall = GPIFalse;
		break;
	default:
		Error(connection, GP_PARAMETER_ERROR, "Invalid firewall.");
	}
#endif

	// Get the nick, uniquenick, email, and password.
	/////////////////////////////////////////////////
	strzcpy(iconnection->nick, nick, GP_NICK_LEN);
	strzcpy(iconnection->uniquenick, uniquenick, GP_UNIQUENICK_LEN);
	strzcpy(iconnection->email, email, GP_EMAIL_LEN);
	strzcpy(iconnection->password, password, GP_PASSWORD_LEN);

#ifdef GSI_UNICODE
	// Create the _W version in addition
	UTF8ToUCS2StringLen(iconnection->nick, iconnection->nick_W, GP_NICK_LEN);
	UTF8ToUCS2StringLen(iconnection->uniquenick, iconnection->uniquenick_W, GP_UNIQUENICK_LEN);
	UTF8ToUCS2StringLen(iconnection->email, iconnection->email_W, GP_EMAIL_LEN);
	UTF8ToUCS2StringLen(iconnection->password, iconnection->password_W, GP_PASSWORD_LEN);
#endif

	// Lowercase the email.
	///////////////////////
	_strlwr(iconnection->email);

#ifdef GSI_UNICODE
	// Update the UCS2 version (emails are ASCII anyhow so lowercasing didn't data)
	AsciiToUCS2String(iconnection->email, iconnection->email_W);
#endif

	// Create a connect operation data struct.
	//////////////////////////////////////////
	data = (GPIConnectData *)gsimalloc(sizeof(GPIConnectData));
	if(data == NULL)
		Error(connection, GP_MEMORY_ERROR, "Out of memory.");
	memset(data, 0, sizeof(GPIConnectData));

	// Check for new user.
	//////////////////////
	data->newuser = newuser;

	// Store pre-auth data.
	///////////////////////
	if(authtoken[0] && partnerchallenge[0])
	{
		strzcpy(data->authtoken, authtoken, GP_AUTHTOKEN_LEN);
		strzcpy(data->partnerchallenge, partnerchallenge, GP_PARTNERCHALLENGE_LEN);
	}

	// Store cdkey if we have one.
	//////////////////////////////
	if(cdkey)
		strzcpy(data->cdkey, cdkey, GP_CDKEY_LEN);

	// Add the operation to the list.
	/////////////////////////////////
	CHECK_RESULT(gpiAddOperation(connection, GPI_CONNECT, data, &operation, blocking, callback, param));

	// Start it.
	////////////
	result = gpiStartConnect(connection, operation);
	if(result != GP_NO_ERROR)
	{
		operation->result = result;
		gpiFailedOpCallback(connection, operation);
		gpiDisconnect(connection, GPIFalse);
		return result;
	}

	// Process it if blocking.
	//////////////////////////
	if(operation->blocking)
		CHECK_RESULT(gpiProcess(connection, operation->id));

	return GP_NO_ERROR;
}

static GPResult
gpiSendLogin(
  GPConnection * connection,
  GPIConnectData * data
)
{
	char buffer[512];
	char response[33];
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIProfile * profile;
	char * passphrase;
	char userBuffer[GP_NICK_LEN + GP_EMAIL_LEN];
	char partnerBuffer[11];
	char * user;

	// Construct the user challenge.
	////////////////////////////////
	randomString(data->userChallenge, sizeof(data->userChallenge) - 1);

	// Hash the password.
	/////////////////////
	if(data->partnerchallenge[0])
		passphrase = data->partnerchallenge;
	else
		passphrase = iconnection->password;
	MD5Digest((unsigned char*)passphrase, strlen(passphrase), data->passwordHash);

	// Construct the user.
	//////////////////////
	if(iconnection->partnerID != GP_PARTNERID_GAMESPY)
	{
		sprintf(partnerBuffer, "%d@", iconnection->partnerID);
	}
	else
	{
		// GS ID's do not stash the partner ID in the auth challenge to support legacy clients.
		strcpy(partnerBuffer, "");
	}

	if(data->authtoken[0])
		user = data->authtoken;
	else if(iconnection->uniquenick[0])
	{
		sprintf(userBuffer, "%s%s", partnerBuffer, iconnection->uniquenick);
		user = userBuffer;
	}
	else
	{
		sprintf(userBuffer, "%s%s@%s", partnerBuffer, iconnection->nick, iconnection->email);
		user = userBuffer;
	}

	// Construct the response.
	//////////////////////////
	sprintf(buffer, "%s%s%s%s%s%s",
		data->passwordHash,
		"                                                ",
		user,
		data->userChallenge,
		data->serverChallenge,
		data->passwordHash);
	MD5Digest((unsigned char *)buffer, strlen(buffer), response);

	// Check for an existing profile.
	/////////////////////////////////
	if(iconnection->infoCaching)
	{
		gpiFindProfileByUser(connection, iconnection->nick, iconnection->email, &profile);
		if(profile != NULL)
		{
			// Get the userid and profileid.
			////////////////////////////////
			iconnection->userid = profile->userId;
			iconnection->profileid = profile->profileId;
		}
	}

	// Construct the outgoing message.
	//////////////////////////////////
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\login\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\challenge\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, data->userChallenge);
	if(data->authtoken[0])
	{
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\authtoken\\");
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, data->authtoken);
	}
	else if(iconnection->uniquenick[0])
	{
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\uniquenick\\");
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->uniquenick);
	}
	else
	{
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\user\\");
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->nick);
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "@");
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->email);
	}
	if(iconnection->userid != 0)
	{
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\userid\\");
		gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->userid);
	}
	if(iconnection->profileid != 0)
	{
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\profileid\\");
		gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->profileid);
	}
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\partnerid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->partnerID);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\response\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, response);
	if(iconnection->firewall == GP_FIREWALL)
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\firewall\\1");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\port\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->peerPort);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\productid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->productID);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\gamename\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, __GSIACGamename);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\namespaceid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->namespaceID);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sdkrevision\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, GPI_SDKREV);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\quiet\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->quietModeFlags);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\id\\1");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

	return GP_NO_ERROR;
}

static GPResult
gpiSendNewuser(
  GPConnection * connection,
  GPIConnectData * data
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	size_t i;
	const int useAlternateEncoding = 1;

	// Encrypt the password (xor with random values)
	char passwordenc[GP_PASSWORDENC_LEN];
	gpiEncodeString(iconnection->password, passwordenc);

	// Construct the outgoing message.
	//////////////////////////////////
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\newuser\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\email\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->email);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\nick\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->nick);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\passwordenc\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, passwordenc);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\productid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->productID);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\gamename\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, __GSIACGamename);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\namespaceid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->namespaceID);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\uniquenick\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->uniquenick);
	if(data->cdkey[0])
	{
		// Encrypt the cdkey (xor with random values)
		char cdkeyxor[GP_CDKEY_LEN];
		char cdkeyenc[GP_CDKEYENC_LEN];
		size_t cdkeylen = strlen(data->cdkey);
		
		Util_RandSeed((unsigned long)GP_XOR_SEED);
		for (i=0; i < cdkeylen; i++)
		{
			// XOR each character with the next rand
			char aRand = (char)Util_RandInt(0, 0xFF);
			cdkeyxor[i] = (char)(data->cdkey[i] ^ aRand);
		}
		cdkeyxor[i] = '\0';

		// Base 64 it (printable chars only)
		B64Encode(cdkeyxor, cdkeyenc, (int)cdkeylen, useAlternateEncoding);

		//gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\cdkey\\");
		//gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, data->cdkey);
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\cdkeyenc\\");
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, cdkeyenc);
	}
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\partnerid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->partnerID);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\id\\1");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

	return GP_NO_ERROR;
}

GPResult
gpiProcessConnect(
  GPConnection * connection,
  GPIOperation * operation,
  const char * input
)
{
	char buffer[512];
	char check[33];
	char uniquenick[GP_UNIQUENICK_LEN];
	GPIConnectData * data;
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPICallback callback;
	GPIProfile * profile;
	char userBuffer[GP_NICK_LEN + GP_EMAIL_LEN];
	char partnerBuffer[11];
	char * user;

	// Check for an error.
	//////////////////////
	if(gpiCheckForError(connection, input, GPIFalse))
	{
		// Is this a deleted profile?
		/////////////////////////////
		if((iconnection->errorCode == GP_LOGIN_PROFILE_DELETED) && iconnection->profileid)
		{
			// Remove this profile object.
			//////////////////////////////
			gpiRemoveProfileByID(connection, iconnection->profileid);

			// If we have the profileid/userid cached, lose them.
			/////////////////////////////////////////////////////
			iconnection->userid = 0;
			iconnection->profileid = 0;
		}
		// Check for creating an existing profile.
		//////////////////////////////////////////
		else if(iconnection->errorCode == GP_NEWUSER_BAD_NICK)
		{
			// Store the pid.
			/////////////////
			if(gpiValueForKey(input, "\\pid\\", buffer, sizeof(buffer)))
				iconnection->profileid = atoi(buffer);
		}

		// Call the callbacks.
		//////////////////////
		CallbackFatalError(connection, GP_SERVER_ERROR, iconnection->errorCode, iconnection->errorString);
	}

	// Get a pointer to the data.
	/////////////////////////////
	data = (GPIConnectData*)operation->data;

	switch(operation->state)
	{
	case GPI_CONNECTING:
		// This should be \lc\1.
		////////////////////////
		if(strncmp(input, "\\lc\\1", 5) != 0)
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");

		// Get the server challenge.
		////////////////////////////
		if(!gpiValueForKey(input, "\\challenge\\", data->serverChallenge, sizeof(data->serverChallenge)))
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");

		// Check if this is a new user.
		///////////////////////////////
		if(data->newuser)
		{
			// Send a new user message.
			///////////////////////////
			CHECK_RESULT(gpiSendNewuser(connection, data));

			// Update the operation's state.
			////////////////////////////////
			operation->state = GPI_REQUESTING;
		}
		else
		{
			// Send a login message.
			////////////////////////
			CHECK_RESULT(gpiSendLogin(connection, data));

			// Update the operation's state.
			////////////////////////////////
			operation->state = GPI_LOGIN;
		}

		break;

	case GPI_REQUESTING:
		// This should be \nur\.
		////////////////////////
		if(strncmp(input, "\\nur\\", 5) != 0)
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");

		// Get the userid.
		//////////////////
		if(!gpiValueForKey(input, "\\userid\\", buffer, sizeof(buffer)))
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexepected data was received from the server.");
		iconnection->userid = atoi(buffer);

		// Get the profileid.
		/////////////////////
		if(!gpiValueForKey(input, "\\profileid\\", buffer, sizeof(buffer)))
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexepected data was received from the server.");
		iconnection->profileid = atoi(buffer);

		// Send a login request.
		////////////////////////
		CHECK_RESULT(gpiSendLogin(connection, data));

		// Update the operation's state.
		////////////////////////////////
		operation->state = GPI_LOGIN;

		break;
		
	case GPI_LOGIN:
		// This should be \lc\2.
		////////////////////////
		if(strncmp(input, "\\lc\\2", 5) != 0)
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");

		// Get the sesskey.
		///////////////////
		if(!gpiValueForKey(input, "\\sesskey\\", buffer, sizeof(buffer)))
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexepected data was received from the server.");
		iconnection->sessKey = atoi(buffer);

		// Get the userid.
		//////////////////
		if(!gpiValueForKey(input, "\\userid\\", buffer, sizeof(buffer)))
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexepected data was received from the server.");
		iconnection->userid = atoi(buffer);

		// Get the profileid.
		/////////////////////
		if(!gpiValueForKey(input, "\\profileid\\", buffer, sizeof(buffer)))
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexepected data was received from the server.");
		iconnection->profileid = atoi(buffer);

		// Get the uniquenick.
		//////////////////////
		if(!gpiValueForKey(input, "\\uniquenick\\", uniquenick, sizeof(uniquenick)))
			uniquenick[0] = '\0';

		// Get the loginticket.
		//////////////////////
		if(!gpiValueForKey(input, "\\lt\\", iconnection->loginTicket, sizeof(iconnection->loginTicket)))
			iconnection->loginTicket[0] = '\0';


		// Construct the user.
		//////////////////////
		if(iconnection->partnerID != GP_PARTNERID_GAMESPY)
		{
			sprintf(partnerBuffer, "%d@", iconnection->partnerID);
		}
		else
		{
			// GS ID's do not stash the partner ID in the auth challenge to support legacy clients.
			strcpy(partnerBuffer, "");
		}

		if(data->authtoken[0])
			user = data->authtoken;
		else if(iconnection->uniquenick[0])  
		{
			sprintf(userBuffer, "%s%s", partnerBuffer, iconnection->uniquenick);
			user = userBuffer;
		}
		else
		{
			sprintf(userBuffer, "%s%s@%s", partnerBuffer, iconnection->nick, iconnection->email);
			user = userBuffer;
		}

		// Construct the check.
		///////////////////////
		sprintf(buffer, "%s%s%s%s%s%s",
			data->passwordHash,
			"                                                ",
			user,
			data->serverChallenge,
			data->userChallenge,
			data->passwordHash);
		MD5Digest((unsigned char *)buffer, strlen(buffer), check);

		// Get the proof.
		/////////////////
		if(!gpiValueForKey(input, "\\proof\\", buffer, sizeof(buffer)))
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexepected data was received from the server.");

		// Check the server authentication.
		///////////////////////////////////
		if(memcmp(check, buffer, 32) != 0)
			CallbackFatalError(connection, GP_NETWORK_ERROR, GP_LOGIN_SERVER_AUTH_FAILED, "Could not authenticate server.");

		// Add the local profile to the list.
		/////////////////////////////////////
		if(iconnection->infoCaching)
		{
			profile = gpiProfileListAdd(connection, iconnection->profileid);
			profile->profileId = iconnection->profileid;
			profile->userId = iconnection->userid;
		}

		// Set the connect state.
		/////////////////////////
		iconnection->connectState = GPI_CONNECTED;

		// Call the connect-response callback.
		//////////////////////////////////////
		callback = operation->callback;
		if(callback.callback != NULL)
		{
			GPConnectResponseArg * arg;
			arg = (GPConnectResponseArg *)gsimalloc(sizeof(GPConnectResponseArg));
			if(arg == NULL)
				Error(connection, GP_MEMORY_ERROR, "Out of memory.");
			memset(arg, 0, sizeof(GPConnectResponseArg));

			arg->profile = (GPProfile)iconnection->profileid;
			arg->result = GP_NO_ERROR;
#ifndef GSI_UNICODE
			strzcpy(arg->uniquenick, uniquenick, GP_UNIQUENICK_LEN);
#else
			UTF8ToUCS2StringLen(uniquenick, arg->uniquenick, GP_UNIQUENICK_LEN);
#endif
			
			CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
		}

		// This operation is complete.
		//////////////////////////////
		gpiRemoveOperation(connection, operation);

		// Get the local profile's info.
		////////////////////////////////
#if 0
		gpiAddOperation(connection, GPI_GET_INFO, NULL, &operation, GP_NON_BLOCKING, NULL, NULL);
		gpiSendGetInfo(connection, iconnection->profileid, operation->id);
#endif


#ifdef _PS3
		// We just connected, so setup buddy sync && start NP init
        // For future, we can limit syncs by setting flags to turn on/off here
		//////////////////////////////////////////////////////////////////////
		iconnection->npPerformBuddySync = gsi_true;
        iconnection->npPerformBlockSync = gsi_true;
		iconnection->loginTime = current_time();

		if (!iconnection->npInitialized)
			gpiInitializeNpBasic(connection);
#endif

		break;
		
	default:
		break;
	}
	
	return GP_NO_ERROR;
}

GPResult
gpiCheckConnect(
  GPConnection * connection
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	int state;
	
	// Check if the connection is completed.
	////////////////////////////////////////
	CHECK_RESULT(gpiCheckSocketConnect(connection, iconnection->cmSocket, &state));
	
	// Check for a failed attempt.
	//////////////////////////////
	if(state == GPI_DISCONNECTED)
		CallbackFatalError(connection, GP_SERVER_ERROR, GP_LOGIN_CONNECTION_FAILED, "The server has refused the connection.");

	// Check if not finished connecting.
	////////////////////////////////////
	if(state == GPI_NOT_CONNECTED)
		return GP_NO_ERROR;
	
	// We're now negotiating the connection.
	////////////////////////////////////////
	assert(state == GPI_CONNECTED);
	iconnection->connectState = GPI_NEGOTIATING;

	return GP_NO_ERROR;
}

static GPIBool
gpiDisconnectCleanupProfile(
  GPConnection * connection,
  GPIProfile * profile,
  void * data
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GSI_UNUSED(data);

    // Even if we cache buddy/block info, free it up to get rid of mem
    // leaks, just don't remove the profile until we save the cache.
    //////////////////////////////////////////////////////////////////
	if(profile->buddyStatus)
	{
        profile->buddyOrBlockCache = gsi_true;
        freeclear(profile->buddyStatus->statusString);
        freeclear(profile->buddyStatus->locationString);
        freeclear(profile->buddyStatus);	
	}
	if (profile->buddyStatusInfo)
	{
        profile->buddyOrBlockCache = gsi_true;
        freeclear(profile->buddyStatusInfo->richStatus);
        freeclear(profile->buddyStatusInfo->gameType);
        freeclear(profile->buddyStatusInfo->gameVariant);
        freeclear(profile->buddyStatusInfo->gameMapName);
        if (profile->buddyStatusInfo->extendedInfoKeys)
        {
            ArrayFree(profile->buddyStatusInfo->extendedInfoKeys);
            profile->buddyStatusInfo->extendedInfoKeys = NULL;
        }
        freeclear(profile->buddyStatusInfo);
	}
    if (profile->blocked)
        profile->buddyOrBlockCache = gsi_true;

	freeclear(profile->authSig);
	freeclear(profile->peerSig);
	profile->requestCount = 0;

	// Remove Profile if:
	//    (there is no info to cache) or
	//    (we only cache buddies/blocked and the user is not a buddy or a block)
	if ((!profile->cache) ||
		(iconnection->infoCachingBuddyAndBlockOnly==GPITrue && !profile->buddyOrBlockCache))
	{
		gpiRemoveProfile(connection, profile);
		return GPIFalse;
	}

	return GPITrue;
}

void
gpiDisconnect(
  GPConnection * connection,
  GPIBool tellServer
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIPeer * peer;
	GPIPeer * delPeer;
	GPIBool connClosed;

	// Check if we're already disconnected.
	// PANTS|05.15.00
	///////////////////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		return;

	// Skip most of this stuff if we never actually connected.
	// PANTS|05.16.00
	//////////////////////////////////////////////////////////
	if(iconnection->connectState != GPI_NOT_CONNECTED)
	{
		// Are we connected?
		////////////////////
		if(tellServer && (iconnection->connectState == GPI_CONNECTED))
		{
			// Send the disconnect.
			///////////////////////
			gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\logout\\\\sesskey\\");
			gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
			gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
		}

		// Always flush remaining messages.
		// PANTS|05.16.00
		///////////////////////////////////
		gpiSendFromBuffer(connection, iconnection->cmSocket, &iconnection->outputBuffer, &connClosed, GPITrue, "CM");

		// Cleanup the connection.
		//////////////////////////
		if(iconnection->cmSocket != INVALID_SOCKET)
		{
			shutdown(iconnection->cmSocket, 2);
			closesocket(iconnection->cmSocket);
			iconnection->cmSocket = INVALID_SOCKET;
		}
		
		if(/*iconnection->peerSocket != INVALID_SOCKET*/ gsUdpEngineIsInitialized())
		{
			//shutdown(iconnection->peerSocket, 2);
			//closesocket(iconnection->peerSocket);
			//iconnection->peerSocket = INVALID_SOCKET;
			gsUdpEngineRemoveMsgHandler(iconnection->mHeader);
			if (gsUdpEngineNoMoreMsgHandlers() && gsUdpEngineNoApp())
				gsUdpEngineShutdown();
		}

		// We're disconnected.
		//////////////////////
		iconnection->connectState = GPI_DISCONNECTED;

		// Don't keep the userid/profileid.
		///////////////////////////////////
		iconnection->userid = 0;
		iconnection->profileid = 0;
	}
	
	// freeclear all the memory.
	///////////////////////
	freeclear(iconnection->socketBuffer.buffer);
	freeclear(iconnection->inputBuffer);
	freeclear(iconnection->outputBuffer.buffer);
	freeclear(iconnection->updateproBuffer.buffer);
	freeclear(iconnection->updateuiBuffer.buffer);
	while(iconnection->operationList != NULL)
		gpiRemoveOperation(connection, iconnection->operationList);
	iconnection->operationList = NULL;
	for(peer = iconnection->peerList ; peer != NULL ; )
	{
		delPeer = peer;
		peer = peer->pnext;
		gpiDestroyPeer(connection, delPeer);
	}
	iconnection->peerList = NULL;
	
	// Cleanup buddies.
	// This is not optimal - because we can't continue the mapping
	// after freeing a profile, we need to start it all over again.
	///////////////////////////////////////////////////////////////
	while(!gpiProfileMap(connection, gpiDisconnectCleanupProfile, NULL))  { };
}
