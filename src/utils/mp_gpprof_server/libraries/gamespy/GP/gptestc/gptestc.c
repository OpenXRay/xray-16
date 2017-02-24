#include "../gp.h"
#include "../../common/gsAvailable.h"

#if defined(_WIN32) && !defined(UNDER_CE)
	#include <conio.h>
#endif

#ifdef UNDER_CE
	void RetailOutputA(CHAR *tszErr, ...);
	#define printf RetailOutputA
#elif defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

#if defined(_WIN32)
// disable the warning about our while(1) statement
#pragma warning(disable:4127)
#endif

#ifdef __MWERKS__	// CodeWarrior will warn if function is not prototyped
int test_main(int argc, char **argv);
#endif 

#define GPTC_PRODUCTID 0
#define GPTC_GAMENAME   _T("gmtest")
#define GPTC_NICK1      _T("gptestc1")
#define GPTC_NICK2      _T("gptestc2")
#define GPTC_NICK3      _T("gptestc3")
#define GPTC_EMAIL1     _T("gptestc@gptestc.com")
#define GPTC_EMAIL2     _T("gptestc2@gptestc.com")
#define GPTC_EMAIL3     _T("gptestc3@gptestc.com")
#define GPTC_PASSWORD   _T("gptestc")
#define GPTC_PID1       2957553
#define GPTC_PID2       3052160
#define GPTC_PID3       118305038
#define GPTC_FIREWALL_OPTION   GP_FIREWALL

#define CHECK_GP_RESULT(func, errString) if(func != GP_NO_ERROR) { printf("%s\n", errString); /*return 1;*/ }

GPConnection * pconn;
GPProfile other;
GPProfile otherBlock;
int otherIndex = -1;
int appState = -1;
gsi_bool receivedLastMessage = gsi_false;
gsi_bool gotStoodUp = gsi_false;
gsi_bool blockTesting = gsi_false;
int noComLineArgs;
int namespaceIds[GP_MAX_NAMESPACEIDS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};


#ifdef GSI_COMMON_DEBUG
	static void DebugCallback(GSIDebugCategory theCat, GSIDebugType theType,
	                          GSIDebugLevel theLevel, const char * theTokenStr,
	                          va_list theParamList)
	{
		GSI_UNUSED(theLevel);

		printf("[%s][%s] ", 
				gGSIDebugCatStrings[theCat], 
				gGSIDebugTypeStrings[theType]);

		vprintf(theTokenStr, theParamList);
	}
#endif

static void Error(GPConnection * pconnection, GPErrorArg * arg, void * param)
{
	gsi_char * errorCodeString;
	gsi_char * resultString;

#define RESULT(x) case x: resultString = _T(#x); break;
	switch(arg->result)
	{
	RESULT(GP_NO_ERROR)
	RESULT(GP_MEMORY_ERROR)
	RESULT(GP_PARAMETER_ERROR)
	RESULT(GP_NETWORK_ERROR)
	RESULT(GP_SERVER_ERROR)
	default:
		resultString = _T("Unknown result!\n");
	}

#define ERRORCODE(x) case x: errorCodeString = _T(#x); break;
	switch(arg->errorCode)
	{
	ERRORCODE(GP_GENERAL)
	ERRORCODE(GP_PARSE)
	ERRORCODE(GP_NOT_LOGGED_IN)
	ERRORCODE(GP_BAD_SESSKEY)
	ERRORCODE(GP_DATABASE)
	ERRORCODE(GP_NETWORK)
	ERRORCODE(GP_FORCED_DISCONNECT)
	ERRORCODE(GP_CONNECTION_CLOSED)
	ERRORCODE(GP_LOGIN)
	ERRORCODE(GP_LOGIN_TIMEOUT)
	ERRORCODE(GP_LOGIN_BAD_NICK)
	ERRORCODE(GP_LOGIN_BAD_EMAIL)
	ERRORCODE(GP_LOGIN_BAD_PASSWORD)
	ERRORCODE(GP_LOGIN_BAD_PROFILE)
	ERRORCODE(GP_LOGIN_PROFILE_DELETED)
	ERRORCODE(GP_LOGIN_CONNECTION_FAILED)
	ERRORCODE(GP_LOGIN_SERVER_AUTH_FAILED)
	ERRORCODE(GP_NEWUSER)
	ERRORCODE(GP_NEWUSER_BAD_NICK)
	ERRORCODE(GP_NEWUSER_BAD_PASSWORD)
	ERRORCODE(GP_UPDATEUI)
	ERRORCODE(GP_UPDATEUI_BAD_EMAIL)
	ERRORCODE(GP_NEWPROFILE)
	ERRORCODE(GP_NEWPROFILE_BAD_NICK)
	ERRORCODE(GP_NEWPROFILE_BAD_OLD_NICK)
	ERRORCODE(GP_UPDATEPRO)
	ERRORCODE(GP_UPDATEPRO_BAD_NICK)
	ERRORCODE(GP_ADDBUDDY)
	ERRORCODE(GP_ADDBUDDY_BAD_FROM)
	ERRORCODE(GP_ADDBUDDY_BAD_NEW)
	ERRORCODE(GP_ADDBUDDY_ALREADY_BUDDY)
	ERRORCODE(GP_AUTHADD)
	ERRORCODE(GP_AUTHADD_BAD_FROM)
	ERRORCODE(GP_AUTHADD_BAD_SIG)
	ERRORCODE(GP_STATUS)
	ERRORCODE(GP_BM)
	ERRORCODE(GP_BM_NOT_BUDDY)
	ERRORCODE(GP_GETPROFILE)
	ERRORCODE(GP_GETPROFILE_BAD_PROFILE)
	ERRORCODE(GP_DELBUDDY)
	ERRORCODE(GP_DELBUDDY_NOT_BUDDY)
	ERRORCODE(GP_DELPROFILE)
	ERRORCODE(GP_DELPROFILE_LAST_PROFILE)
	ERRORCODE(GP_SEARCH)
	ERRORCODE(GP_SEARCH_CONNECTION_FAILED)
	default:
		errorCodeString = _T("Unknown error code!\n");
	}

	if(arg->fatal)
	{
		printf( "-----------\n");
		printf( "FATAL ERROR\n");
		printf( "-----------\n");
	}
	else
	{
		printf( "-----\n");
		printf( "ERROR\n");
		printf( "-----\n");
	}
	_tprintf( _T("RESULT: %s (%d)\n"), resultString, arg->result);
	_tprintf( _T("ERROR CODE: %s (0x%X)\n"), errorCodeString, arg->errorCode);
	_tprintf( _T("ERROR STRING: %s\n"), arg->errorString);
	
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

gsi_char whois[GP_NICK_LEN];
static void Whois(GPConnection * pconnection, GPGetInfoResponseArg * arg, void * param)
{
	if(arg->result == GP_NO_ERROR)
	{
		_tcscpy(whois, arg->nick);
		/*if (_tcscmp(arg->email, "")) {
			_tcscat(whois, _T("@"));
			_tcscat(whois, arg->email);
		}*/
	}
	else
		printf( "WHOIS FAILED\n");
	
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

static void ConnectResponse(GPConnection * pconnection, GPConnectResponseArg * arg, void * param)
{
	if(arg->result == GP_NO_ERROR)
		printf("Connected\n");
	else
		printf( "CONNECT FAILED\n");

	gpSetStatus(pconnection, (GPEnum)GP_ONLINE, _T("Not Ready"), _T("gptestc"));

	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

static void ProfileSearchResponse(GPConnection * pconnection, GPProfileSearchResponseArg * arg, void * param)
{
	GPResult result;
	int i;
	if(arg->result == GP_NO_ERROR)
	{
		if(arg->numMatches > 0)
		{
			for(i = 0 ; i < arg->numMatches ; i++)
			{
				result = gpGetInfo(pconn, arg->matches[i].profile, GP_DONT_CHECK_CACHE, GP_BLOCKING, (GPCallback)Whois, NULL);
				if(result != GP_NO_ERROR)
					printf("  gpGetInfo failed\n");
				else
					_tprintf(_T("  Found: %s\n"), whois);
			}
		}
		else
			printf( "  NO MATCHES\n");
	}
	else
		printf( "  SEARCH FAILED\n");

	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

int msgCount = 0;
static void RecvBuddyMessage(GPConnection * pconnection, void * theArg, void * param)
{
	GPRecvBuddyMessageArg *arg;
	GPResult result;

	arg = (GPRecvBuddyMessageArg *)theArg;

	result = gpGetInfo(pconn, arg->profile, GP_DONT_CHECK_CACHE, GP_BLOCKING, (GPCallback)Whois, NULL);
	if(result != GP_NO_ERROR)
		printf(" gpGetInfo failed\n");
	else 
		_tprintf(_T(" Received buddy message: %s: %s\n"), whois, arg->message);

	if (!(_tcscmp(arg->message, "5_Hello!"))) {
		receivedLastMessage = gsi_true;
	}
		
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}


static void GetInfoResponse(GPConnection * pconnection, GPGetInfoResponseArg * arg, void * param)
{
	//_tprintf(_T(" First Name: %s Last Name: %s (%s@%s)\n"), arg->firstname, arg->lastname, arg->nick, arg->email);
	_tprintf(_T(" First Name: %s Last Name: %s (%s)\n"), arg->firstname, arg->lastname, arg->nick);
	
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

void RecvBuddyStatus(GPConnection * connection, void * arg_, void * param)
{
	GPRecvBuddyStatusArg * arg = (GPRecvBuddyStatusArg *)arg_;
	GPBuddyStatus status;
	static char* statusToString[6] =
	{
		"GP_OFFLINE",
		"GP_ONLINE",
		"GP_PLAYING",
		"GP_STAGING",
		"GP_CHATTING",
		"GP_AWAY"
	};

	printf(" Buddy index: %d\n", arg->index);

	if (arg->profile == other) 
	{
		if (appState < 0)
			appState = 0;
		otherIndex = arg->index;
		gpGetBuddyStatus(connection, arg->index, &status);
		if (status.status == 1 && _tcscmp(status.statusString, "Ready") && _tcscmp(status.statusString, "BlockTime")) {
			if (appState < 1) {
				appState = 1;
				//printf("Buddy is online!\n");
				CHECK_GP_RESULT(gpSetStatus(pconn, (GPEnum) GP_ONLINE, _T("Ready"), _T("gptestc")), "gpSetStatus failed");
				//printf("Now wait until buddy is ready to message...\n");
			}
		}
		else if (status.status == 1 && !(_tcscmp(status.statusString, "Ready"))) {
			if (appState < 2) {
				appState = 2;
				//printf("Buddy is ready to message!\n");
				CHECK_GP_RESULT(gpSetStatus(pconn, (GPEnum) GP_ONLINE, _T("Ready"), _T("gptestc")), "gpSetStatus failed");
			}
		}
		else if (status.status == 1 && !(_tcscmp(status.statusString, "BlockTime"))) {
			if (appState < 3)
				appState = 3;
			//printf("Buddy is ready to block test!\n");
		}
	}
	printf(" ");
	gpGetInfo(connection, arg->profile, GP_DONT_CHECK_CACHE, GP_BLOCKING, (GPCallback)GetInfoResponse, NULL);

	gpGetBuddyStatus(connection, arg->index, &status);

	printf("  Status: %s, Status String: %s, Location String: %s, IP: %d, Port: %d\n", statusToString[status.status], status.statusString, status.locationString, status.ip, status.port);

	GSI_UNUSED(param);
}

void RecvBuddyRequest(GPConnection * connection, void * arg_, void * param)
{
	GPRecvBuddyRequestArg * arg = (GPRecvBuddyRequestArg *)arg_;
	
	gsi_char buddy1[50];
	gsi_char buddy2[50];

	gpGetInfo(connection, arg->profile, GP_CHECK_CACHE, GP_BLOCKING, (GPCallback)Whois, NULL);
	printf("\nBuddy Request from %s\n", whois);
#ifdef GSI_UNICODE
	_stprintf(buddy1, sizeof(buddy1), _T("%s"), GPTC_NICK1);
	_stprintf(buddy2, sizeof(buddy2), _T("%s"), GPTC_NICK2);
#else
	_stprintf(buddy1, _T("%s"), GPTC_NICK1);
	_stprintf(buddy2, _T("%s"), GPTC_NICK2);
#endif
	
	if (!_tcscmp(whois, buddy1) || !_tcscmp(whois, buddy2))
	{
		printf("Authorizing buddy request\n");
		gpAuthBuddyRequest(connection, arg->profile);
		CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
		if (blockTesting) {
			appState = 4;
			if (!noComLineArgs) {
				gpSendBuddyRequest(pconn, other, _T("testing"));
				CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
			}
		}
	}
	else
	{
		printf("Denying buddy request\n");
		gpDenyBuddyRequest(connection, arg->profile);
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void RecvBuddyRevoke(GPConnection * connection, void * arg_, void * param)
{
	printf("Buddy Revoke received...\n");

	if (appState == 2) {
		appState = 3;
	}
	
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
	GSI_UNUSED(arg_);
}

static void printBlockedList()
{    
	int i=0;
	int numBlocked;
	int pid;

	gpGetNumBlocked(pconn, &numBlocked);
	printf("Get blocked list: num = %d\n", numBlocked);

	for (i=0; i<numBlocked; i++)
	{
		gpGetBlockedProfile(pconn, i, &pid);
		printf("[%d]: %d\n", i+1, pid);
		gpGetInfo(pconn, pid, GP_CHECK_CACHE, GP_BLOCKING, (GPCallback)Whois, NULL);
	}
}

int test_main(int argc, char **argv)
{
	gsi_char * nick;
	gsi_char * email;
	gsi_char * password;
	gsi_char messageToSend[10];
	GPConnection connection;
	GSIACResult acResult = GSIACWaiting;
	int totalTime;
	int messagesSent;

#ifdef GSI_COMMON_DEBUG
	// Define GSI_COMMON_DEBUG if you want to view the SDK debug output
	// Set the SDK debug log file, or set your own handler using gsSetDebugCallback
	//gsSetDebugFile(stdout); // output to console
	gsSetDebugCallback(DebugCallback);

	// Set some debug levels
	gsSetDebugLevel(GSIDebugCat_All, GSIDebugType_All, GSIDebugLevel_Debug);
	//gsSetDebugLevel(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Verbose);   // Show Detailed data on network traffic
	//gsSetDebugLevel(GSIDebugCat_App, GSIDebugType_All, GSIDebugLevel_Hardcore);  // Show All app comment
#endif

	GSI_UNUSED(argv);

	// check that the game's backend is available
	GSIStartAvailableCheck(GPTC_GAMENAME);
	while((acResult = GSIAvailableCheckThink()) == GSIACWaiting)
		msleep(5);
	if(acResult != GSIACAvailable)
	{
		printf("The backend is not available\n");
		return 1;
	}

	pconn = &connection;

	//argc = 2;

	if(argc == 1)
	{
		noComLineArgs = 1;
		nick = GPTC_NICK1;
		email = GPTC_EMAIL1;
	}
	else
	{
		noComLineArgs = 0;
		nick = GPTC_NICK2;
		email = GPTC_EMAIL2;
	}
	_tprintf(_T("Using email= %s, nick= %s\r\n"), email, nick);

	password = GPTC_PASSWORD;

	//INITIALIZE
	////////////
	CHECK_GP_RESULT(gpInitialize(pconn, GPTC_PRODUCTID, 0, GP_PARTNERID_GAMESPY), "gpInitialize failed");
	gpEnable(pconn, GP_INFO_CACHING_BUDDY_AND_BLOCK_ONLY);
	printf("\nInitialized\n");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_ERROR, (GPCallback)Error, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_RECV_BUDDY_MESSAGE, &RecvBuddyMessage, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_RECV_BUDDY_STATUS, &RecvBuddyStatus, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_RECV_BUDDY_REQUEST, &RecvBuddyRequest, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_RECV_BUDDY_REVOKE, &RecvBuddyRevoke, NULL), "gpSetCallback failed");

	//CONNECT
	/////////
	printf("\nConnecting...\n");
	CHECK_GP_RESULT(gpConnect(pconn, nick, email, password, GPTC_FIREWALL_OPTION, GP_BLOCKING, (GPCallback)ConnectResponse, NULL), "gpConnect failed");

	//SEARCH (blocking)
	///////////////////
	printf("\nSearching (blocking)\n");
	printf(" Search on email (dan@gamespy.com)\n");
	CHECK_GP_RESULT(gpProfileSearch(pconn, _T(""), _T(""), _T("dan@gamespy.com"), _T(""), _T(""), 0, GP_BLOCKING, (GPCallback)ProfileSearchResponse, NULL), "gpProfileSearch failed");
	printf(" Search on nick ('crt')\n");
	CHECK_GP_RESULT(gpProfileSearch(pconn, _T("crt"), _T(""), _T(""), _T(""), _T(""), 0, GP_BLOCKING, (GPCallback)ProfileSearchResponse, NULL), "gpProfileSearch failed");
	printf(" Search on nick and email ('gptestc1', gptestc@gptestc.com)\n");
	CHECK_GP_RESULT(gpProfileSearch(pconn, GPTC_NICK1, _T(""), GPTC_EMAIL1, _T(""), _T(""), 0, GP_BLOCKING, (GPCallback)ProfileSearchResponse, NULL), "gpProfileSearch failed");
	printf(" Search on unique nick for different namespaces...\n");
	CHECK_GP_RESULT(gpProfileSearchUniquenick(pconn, GPTC_NICK1, namespaceIds, GP_MAX_NAMESPACEIDS, GP_BLOCKING, (GPCallback)ProfileSearchResponse, NULL),
		"gpProfileSearchUniquenick failed" );

	if(noComLineArgs)
	{
		CHECK_GP_RESULT(gpProfileFromID(pconn, &other, GPTC_PID2), "gpProfileFromID failed");
	}
	else
	{
		CHECK_GP_RESULT(gpProfileFromID(pconn, &other, GPTC_PID1), "gpProfileFromID failed");
	}
	
	//GET INFO
	//////////
	printf("\nGetting Info\n");
	CHECK_GP_RESULT(gpGetInfo(pconn, GPTC_PID1, GP_DONT_CHECK_CACHE, GP_BLOCKING, (GPCallback)GetInfoResponse, NULL), "gpGetInfo failed");

//-----------------------------
	//BUDDY STUFF
	/////////

	printf("\nRetrieving buddy info (and processing any old buddy messages...)\n");
	totalTime = 0;
	while(appState < 2)
	{
		CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
		msleep(50);
		totalTime += 50;
		if (totalTime > 10000) 
		{
			if (appState == -1) // buddy is not actually on our buddy list 
			{ 
				if (noComLineArgs) 
					printf(" %s is not on our buddy list! Sending him a buddy request and waiting for a response...\n", GPTC_NICK2);
				else 
					printf("%s is not on our buddy list! Sending him a buddy request and waiting for a response...\n", GPTC_NICK1);

				gpSendBuddyRequest(pconn, other, _T("testing"));
				
				totalTime = 0;
				while (totalTime < 10000 && appState != 2) {
					CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
					msleep(50);
					totalTime += 50;
				}
				totalTime = 12001;
				if (appState != 2)
				{
					if (noComLineArgs) 
						printf("\n%s never showed up =(\n", GPTC_NICK2);
					else 
						printf("\n%s never showed up =(\n", GPTC_NICK1);
					gotStoodUp = gsi_true;
					break;
				}
			}
			else // buddy is on our buddy list but did not come online and set status to "Ready"
			{
				if (noComLineArgs) 
					printf("\n%s never showed up =(\n", GPTC_NICK2);
				else 
					printf("\n%s never showed up =(\n", GPTC_NICK1);
				gotStoodUp = gsi_true;
				break;
			}
		}
	}

	if (!gotStoodUp) 
	{
		printf("\nSending messages to buddy (and receiving messages from him)\n");
		totalTime = 0;
		messagesSent = 0;
		while(messagesSent < 5)
		{
			CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
			msleep(50);
			totalTime += 50;
			if (totalTime % 1000 == 0) 
			{
#ifdef GSI_UNICODE
				_stprintf(messageToSend, sizeof(messageToSend), _T("%d_Hello!"), (int)(totalTime/1000));
#else
				_stprintf(messageToSend, _T("%d_Hello!"), (int)(totalTime/1000));
#endif
				CHECK_GP_RESULT(gpSendBuddyMessage(pconn, other, messageToSend), "gpSendBuddyMessage failed");
				messagesSent++;
			}
		}

		while(!receivedLastMessage)
		{
			CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
			msleep(50);
		}

		CHECK_GP_RESULT(gpSetStatus(pconn, (GPEnum) GP_ONLINE, _T("BlockTime"), _T("gptestc")), "gpSetStatus failed");
	
		while(appState != 3)
		{
			CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
			msleep(50);
		}
	}
	else
		appState = 3;

	blockTesting = gsi_true;
	if (noComLineArgs) {
		// block buddy 
		if (gotStoodUp) {
			CHECK_GP_RESULT(gpProfileFromID(pconn, &other, GPTC_PID3), "gpProfileFromID failed");
			printf("\nBlocking %s\n", GPTC_NICK3);
		}
		else
			printf("\nBlocking %s\n", GPTC_NICK2);

		CHECK_GP_RESULT(gpAddToBlockedList(pconn, other), "gpAddToBlockedList failed");
		CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
		printBlockedList();

		if (!gotStoodUp) {
			printf("Checking if %s is on our buddy list...\n", GPTC_NICK2);
			if (gpIsBuddy(pconn, other))
				printf("Yup.\n");
			else
				printf("Nope.\n");

			printf("Wait while %s tries to message us...\n", GPTC_NICK2);
			totalTime = 0;
			while (totalTime < 5000) {
				CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
				msleep(50);
				totalTime += 50;
			}
		}
		if (!gotStoodUp)
			printf("\nUnblocking %s\n", GPTC_NICK2);
		else 
			printf("\nUnblocking %s\n", GPTC_NICK3);
		CHECK_GP_RESULT(gpRemoveFromBlockedList(pconn, other), "gpRemoveFromBlockedList");
		CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
		printBlockedList();

		CHECK_GP_RESULT(gpSetStatus(pconn, (GPEnum) GP_ONLINE, _T("DoneBlockTesting"), _T("gptestc")), "gpSetStatus failed");

		if (!gotStoodUp) {
			printf("Sending a buddy request to %s\n", GPTC_NICK2);
			gpSendBuddyRequest(pconn, other, _T("testing"));

			printf("Waiting to get buddy requested back...\n");
			while (appState != 4) {
				CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
				msleep(50);
			}
		}
	}
	else {
		if (!gotStoodUp) {
			printf("Waiting for %s to block us...\n", GPTC_NICK1);
			while (gpIsBuddy(pconn, other)) {
				CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
				msleep(50);
			}
			printf("We have been blocked. Trying to send buddy message to %s\n", GPTC_NICK1);
			CHECK_GP_RESULT(gpSendBuddyMessage(pconn, other, _T("Why did you block me?")), "gpSendBuddyMessage failed");
			
			printf("Waiting to get unblocked and buddy requested...\n");
			while (appState != 4) {
				CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
				msleep(50);
			}
			CHECK_GP_RESULT(gpSetStatus(pconn, (GPEnum) GP_ONLINE, _T("DoneBlockTesting"), _T("gptestc")), "gpSetStatus failed");
		}
	}

	printf("\nDONE - Press any key to exit.\n\n"); 

#if defined(_WIN32) && !defined(UNDER_CE)
	while (1)
	{
		CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");
		msleep(10);
		if (_kbhit()) {
			break;
		}
	}
#endif

//#if defined(_WIN32) && !defined(UNDER_CE)
	//DISCONNECT
	////////////
	gpDisconnect(pconn);
	printf("Disconnected\n");

	//DESTROY
	/////////
	gpDestroy(pconn);
	printf("Destroyed\n");
	return 0;
//#endif
}


