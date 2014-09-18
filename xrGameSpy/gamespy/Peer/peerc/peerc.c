// GameSpy Peer SDK C Test App
// Dan "Mr. Pants" Schoenblum
// dan@gamespy.com

/*********
INCLUDES 
*********/
#include "../peer.h"
#include "../../common/gsStringUtil.h"
#include "../../common/gsAvailable.h"

/********
DEFINES
********/
#define TITLE		_T("gmtest")

// ensure cross-platform compatibility for printf
#ifdef UNDER_CE
	void RetailOutputA(CHAR *tszErr, ...);
	#define printf RetailOutputA
#elif defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

#define LOOP_SECONDS 60   

#define NICK_SIZE 32   // size of nick array, below

/*************
GLOBAL VARS
*************/
gsi_char nick[NICK_SIZE]; // our nickname for peerConnect

// RoomType to string 
gsi_char RtoS[3][16] =
{
	_T("TitleRoom"),
	_T("GroupRoom"),
	_T("StagingRoom")
};

PEERBool groupRoomCallbackDone = PEERFalse;  


static const gsi_char * ResultToString (PEERJoinResult result)
{
	switch(result)
	{
	case PEERJoinSuccess:
		return _T("Success");
	case PEERFullRoom:
		return _T("Full");
	case PEERInviteOnlyRoom:
		return _T("Invite Only");
	case PEERBannedFromRoom:
		return _T("Banned");
	case PEERBadPassword:
		return _T("Bad Password");
	case PEERAlreadyInRoom:
		return _T("Already in room");
	case PEERNoTitleSet:
		return _T("No Title");
	case PEERNoConnection:
		return _T("No Connection");
	case PEERAutoMatching:
		return _T("Auto Matching");
	case PEERJoinFailed:
		return _T("Join Failed");
	}

	//ASSERT(0);
	return _T("<UNKNOWN RESULT>");
}

static void JoinRoomCallback
(
 PEER peer,
 PEERBool success,
 PEERJoinResult result,
 RoomType roomType,
 void * param
 )
{
	if(!success)
	{
		_tprintf(_T("Join failure: %s"), ResultToString(result));
		return;
	}
	else
	{
		_tprintf(_T("Joined %s.\n"), RtoS[roomType]);
		/*dlg->FillPlayerList(roomType);
		if(roomType == StagingRoom)
		{
			dlg->m_name = peerGetRoomName(peer, StagingRoom);
			dlg->UpdateData(FALSE);
		}*/
	}

	GSI_UNUSED(param);
	GSI_UNUSED(roomType);
	GSI_UNUSED(peer);
}

static void CreateStagingRoomCallback
(
 PEER peer,
 PEERBool success,
 PEERJoinResult result,
 RoomType roomType,
 void * param
 )
{
	if (success)
		_tprintf(_T("Staging Room Created\n"));

	GSI_UNUSED(param);
	GSI_UNUSED(roomType);
	GSI_UNUSED(result);
	GSI_UNUSED(peer);
}

static void DisconnectedCallback
(
	PEER peer,
	const gsi_char * reason,
	void * param
)
{
	_tprintf(_T("Disconnected: %s\n"), reason);
	
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
}

static void RoomMessageCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	const gsi_char * message,
	MessageType messageType,
	void * param
)
{
	_tprintf(_T(" (%s) %s: %s\n"), RtoS[roomType], nick, message);
	//if(strcasecmp(message, _T("quit")) == 0)
	//	stop = PEERTrue;
	//else if((strlen(message) > 5) && (strncasecmp(message, _T("echo"), 4) == 0))
	//	peerMessageRoom(peer, roomType, message + 5, messageType);

	GSI_UNUSED(param);
	GSI_UNUSED(peer);
	GSI_UNUSED(messageType);
}

static void RoomUTMCallback
(
	PEER peer, 
	RoomType roomType, 
	const gsi_char * nick, 
	const gsi_char * command, 
	const gsi_char * parameters,
	PEERBool authenticated, 
	void * param 
)
{
	GSI_UNUSED(peer);
	GSI_UNUSED(roomType);
	GSI_UNUSED(nick);
	GSI_UNUSED(command);
	GSI_UNUSED(parameters);
	GSI_UNUSED(authenticated);
	GSI_UNUSED(param);
}

static void PlayerMessageCallback
(
	PEER peer,
	const gsi_char * nick,
	const gsi_char * message,
	MessageType messageType,
	void * param
)
{
	_tprintf(_T("(PRIVATE) %s: %s\n"), nick, message);
	
	GSI_UNUSED(peer);
	GSI_UNUSED(messageType);
	GSI_UNUSED(param);
}

static void ReadyChangedCallback
(
	PEER peer,
	const gsi_char * nick,
	PEERBool ready,
	void * param
)
{
	if(ready)
		_tprintf(_T("%s is ready\n"), nick);
	else
		_tprintf(_T("%s is not ready\n"), nick);

	GSI_UNUSED(peer);
	GSI_UNUSED(param);
}

static void GameStartedCallback
(
	PEER peer,
	SBServer server,
	const gsi_char * message,
	void * param
)
{
	_tprintf(_T("The game is starting at %s\n"), SBServerGetPublicAddress(server));
	if(message && message[0])
		_tprintf(_T(": %s\n"), message);
	else
		_tprintf(_T("\n"));
		
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
}

static void PlayerJoinedCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	void * param
)
{
	_tprintf(_T("%s joined the %s\n"), nick, RtoS[roomType]);
	
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
}

static void PlayerLeftCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * nick,
	const gsi_char * reason,
	void * param
)
{
	_tprintf(_T("%s left the %s\n"), nick, RtoS[roomType]);
	
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
	GSI_UNUSED(reason);
}

static void PlayerChangedNickCallback
(
	PEER peer,
	RoomType roomType,
	const gsi_char * oldNick,
	const gsi_char * newNick,
	void * param
)
{
	_tprintf(_T("%s in %s changed nicks to %s\n"), oldNick, RtoS[roomType], newNick);
	
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
}

PEERBool connectSuccess;
static void ConnectCallback
(
	PEER peer,
	PEERBool success,
	int failureReason,
	void * param
)
{
	connectSuccess = success;
	
	GSI_UNUSED(peer);
	GSI_UNUSED(success);
	GSI_UNUSED(failureReason);
	GSI_UNUSED(param);
}

static void EnumPlayersCallback
(
	PEER peer,
	PEERBool success,
	RoomType roomType,
	int index,
	const gsi_char * nick,
	int flags,
	void * param
)
{
	if(!success)
	{
		_tprintf(_T("Enum %s players failed\n"), RtoS[roomType]);
		return;
	}

	if(index == -1)
	{
		_tprintf(_T("--------------------\n"));
		return;
	}

	_tprintf(_T("%d: %s\n"), index, nick);
	/*if(flags & PEER_FLAG_OP)
		_tprintf(_T(" (host)\n"));
	else
		_tprintf(_T("\n"));*/
		

	GSI_UNUSED(peer);
	GSI_UNUSED(param);
	GSI_UNUSED(flags);
}
/*
int gStartListing;
static void ListingGamesCallback
(
	PEER peer,
	PEERBool success,
	const gsi_char * name,
	SBServer server,
	PEERBool staging,
	int msg,
	int progress,
	void * param
)
{
	if(success)
	{
		gsi_char *msgname;
		switch (msg)
		{
		case PEER_CLEAR:
			msgname = _T("PEER_CLEAR");
			break;
		case PEER_ADD:
			msgname = _T("PEER_ADD");
			if (SBServerHasBasicKeys(server))
				_tprintf(_T("  firewall server name: %s numplayers: %d ping: %d firewall: %s\n"), SBServerGetStringValue(server,_T("hostname"),_T("UNKNOWN")), SBServerGetIntValue(server,_T("numplayers"),0), SBServerGetPing(server), SBServerDirectConnect(server)==SBTrue?_T("NO"):_T("YES"));
			if (SBServerHasFullKeys(server))
				_tprintf(_T("  server gametype: %s \n"), SBServerGetStringValue(server, _T("gametype"), _T("unknown")));
			break;
		case PEER_UPDATE:
			msgname = _T("PEER_UPDATE");
			if(server)
			{
				_tprintf(_T("  update server name: %s numplayers: %d ping: %d firewall: %s\n"), SBServerGetStringValue(server,_T("hostname"),_T("UNKNOWN")), SBServerGetIntValue(server,_T("numplayers"),0), SBServerGetPing(server), SBServerDirectConnect(server)==SBTrue?_T("NO"):_T("YES"));
				_tprintf(_T("  server gametype: %s \n"), SBServerGetStringValue(server, _T("gametype"), _T("unknown")));
			}
			break;
		case PEER_REMOVE:
			msgname = _T("PEER_REMOVE");
			break;
		case PEER_COMPLETE:
			msgname = _T("PEER_COMPLETE");
			gStartListing = 1;
			break;
		default:
			msgname = _T("ERROR!");
		}
		//_tprintf("game: %s\n", msgname);
		//if(server)
		//	_tprintf("  server name: %s numplayers: %d ping: %d\n", SBServerGetStringValue(server,_T("hostname"),_T("UNKNOWN")), SBServerGetIntValue(server,_T("numplayers"),0), SBServerGetPing(server));
	}

	GSI_UNUSED(peer);
	GSI_UNUSED(name);
	GSI_UNUSED(staging);
	GSI_UNUSED(progress);
	GSI_UNUSED(param);
}
*/
static void ListGroupRoomsCallback
(
	PEER peer,
	PEERBool success,
	int groupID,
	SBServer server,
	const gsi_char * name,
	int numWaiting,
	int maxWaiting,
	int numGames,
	int numPlaying,
	void * param
)
{
	if(success && (groupID > 0))
	{
		_tprintf(_T("  %s\n"), name);
		_tprintf(_T("    Players in room: %d\n"), numWaiting);
		_tprintf(_T("    Max players in room: %d\n"), maxWaiting);
		_tprintf(_T("    Games: %d\n"), numGames);
		_tprintf(_T("    Players in games: %d\n"), numPlaying);
	}
	else
		groupRoomCallbackDone = PEERTrue;  // if groupID is set to 0 it means all group rooms have been listed

	GSI_UNUSED(peer);
	GSI_UNUSED(server);
	GSI_UNUSED(numWaiting);
	GSI_UNUSED(maxWaiting);
	GSI_UNUSED(numGames);
	GSI_UNUSED(numPlaying);
	GSI_UNUSED(param);
}

PEERBool joinSuccess;
static void JoinCallback
(
	PEER peer,
	PEERBool success,
	PEERJoinResult result,
	RoomType roomType,
	void * param
)
{
	joinSuccess = success;

	GSI_UNUSED(peer);
	GSI_UNUSED(result);
	GSI_UNUSED(roomType);
	GSI_UNUSED(param);
}

static void NickErrorCallback
(
	PEER peer,
	int type,
	const gsi_char * badNick,
	int numSuggestedNicks,
	const gsi_char ** suggestedNicks,
	void * param
)
{
	static int errCount;

	if(errCount < 20)
	{
		_tsnprintf(nick,NICK_SIZE,_T("peerc%u"),(unsigned int)current_time());
		nick[NICK_SIZE - 1] = '\0';
		peerRetryWithNick(peer, nick);
	}
	else
	{
		//peerDisconnect(peer);
		peerRetryWithNick(peer, NULL);
	}
	errCount++;
	
	GSI_UNUSED(type);
	GSI_UNUSED(badNick);
	GSI_UNUSED(suggestedNicks);
	GSI_UNUSED(numSuggestedNicks);
	GSI_UNUSED(param);
}
/*
static void AutoMatchStatusCallback(PEER thePeer, PEERAutoMatchStatus theStatus, void* theParam)
{
	_tprintf(_T("Automatch status: %d\r\n"), theStatus);
	GSI_UNUSED(thePeer);
	GSI_UNUSED(theStatus);
	GSI_UNUSED(theParam);
}

static int AutoMatchRateCallback(PEER thePeer, SBServer theServer, void* theParam)
{
	GSI_UNUSED(thePeer);
	GSI_UNUSED(theServer);
	GSI_UNUSED(theParam);
	return 0;
}
*/
static void RoomKeyChangedCallback(PEER thePeer, RoomType theType, const gsi_char* theNick, const gsi_char* theKey, const gsi_char* theValue, void* theParam)
{
	GSI_UNUSED(thePeer);
	GSI_UNUSED(theType);
	GSI_UNUSED(theNick);
	GSI_UNUSED(theKey);
	GSI_UNUSED(theValue);
	GSI_UNUSED(theParam);
}

static void QRServerKeyCallback
(
	PEER peer,
	int key,
	qr2_buffer_t buffer,
	void * param
)
{
#if VERBOSE
	gsi_char verbose[256];
	_stprintf(verbose, _T("QR_SERVER_KEY | %d\n"), key);
	OutputDebugString(verbose);
#endif

	switch(key)
	{
	case HOSTNAME_KEY:
		_tprintf(_T(" Server Key callback is being called for HOSTNAME_KEY\n"));
		qr2_buffer_add(buffer, _T("My Game"));
		break;
	case NUMPLAYERS_KEY:
		_tprintf(_T(" Server Key callback is being called for NUMPLAYERS_KEY\n"));
		qr2_buffer_add_int(buffer, 1);
		break;
	case GAMEMODE_KEY:
		_tprintf(_T(" Server Key callback is being called for GAMEMODE_KEY\n"));
		qr2_buffer_add(buffer, _T("openplaying"));
		break;
	case HOSTPORT_KEY:
		_tprintf(_T(" Server Key callback is being called for HOSTPORT_KEY\n"));
		qr2_buffer_add_int(buffer, 15151);
		break;
	case MAPNAME_KEY:
		_tprintf(_T(" Server Key callback is being called for MAPNAME_KEY\n"));
		qr2_buffer_add(buffer, _T("Big Crate Room"));
		break;
	case GAMETYPE_KEY:
		_tprintf(_T(" Server Key callback is being called for GAMETYPE_KEY\n"));
		qr2_buffer_add(buffer, _T("Friendly"));
		break;
	case TIMELIMIT_KEY:
		_tprintf(_T(" Server Key callback is being called for TIMELIMIT_KEY\n"));
		qr2_buffer_add_int(buffer, 100);
		break;
	case FRAGLIMIT_KEY:
		_tprintf(_T(" Server Key callback is being called for FRAGLIMIT_KEY\n"));
		qr2_buffer_add_int(buffer, 0);
		break;
	case TEAMPLAY_KEY:
		_tprintf(_T(" Server Key callback is being called for TEAMPLAY_KEY\n"));
		qr2_buffer_add_int(buffer, 0);
		break;
	default:
		qr2_buffer_add(buffer, _T(""));
		break;
	}
	
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
}

static void QRKeyListCallback
(
	PEER peer,
	qr2_key_type type,
	qr2_keybuffer_t keyBuffer,
	void * param
)
{
	// register the keys we use
	switch(type)
	{
	case key_server:
		_tprintf(_T(" Key List Callback is being called for server keys\n"));
		if(!peerIsAutoMatching(peer))
		{
			qr2_keybuffer_add(keyBuffer, HOSTPORT_KEY);
			qr2_keybuffer_add(keyBuffer, MAPNAME_KEY);
			qr2_keybuffer_add(keyBuffer, GAMETYPE_KEY);
			qr2_keybuffer_add(keyBuffer, TIMELIMIT_KEY);
			qr2_keybuffer_add(keyBuffer, FRAGLIMIT_KEY);
			qr2_keybuffer_add(keyBuffer, TEAMPLAY_KEY);
		}
		break;
	case key_player:
		_tprintf(_T(" Key List Callback is being called for player keys\n"));
		// no custom player keys
		break;
	case key_team:
		_tprintf(_T(" Key List Callback is being called for team keys\n"));
		// no custom team keys
		break;
	default: break;
	}
	
	GSI_UNUSED(param);
}




#ifdef __MWERKS__ // CodeWarrior will warn if not prototyped
	int test_main(int argc, char **argv);
#endif

int test_main(int argc, char **argv)
{
	PEER peer;  // peer object (initialized with peerInitialize

	/* peerInitialize */   
	PEERCallbacks callbacks;  // we will need to add all of the supported callbacks

	/* peerSetTitle parameters */
	gsi_char  secret_key[9];      // your title's assigned secret key
	int maxUpdates = 10;  // max server queries the SDK will send out at a time
	PEERBool natNeg = PEERFalse;  // nat negotiation will not be supported
	PEERBool pingRooms[NumRooms];  
	PEERBool crossPingRooms[NumRooms];

	/* peerConnect parameters */
	int profileID = 0;  // we will not be using gp accounts, so this will be ignored
	void * userData = NULL;  // optional data passed to peerConnectCallback
	PEERBool blocking = PEERTrue; // true means function runs synchronously
	PEERBool non_blocking = PEERFalse; // false means function runs asynchronously

	int newbiesGroupID = 2;

	gsi_char * serverName = _T("UberNoobServer");
	int maxPlayers = 2;
	gsi_char * noPassword = _T("");

	GSIACResult result; // used for backend availability check
	gsi_time startTime;

	// set our nickname to a random string so that we'll have different nicks when running multiple instances of peerc
	_tsnprintf(nick,NICK_SIZE,_T("peerc%u"),(unsigned int)current_time());
	nick[NICK_SIZE - 1] = '\0';

	// set the secret key, in a semi-obfuscated manner
	secret_key[0] = 'H';
	secret_key[1] = 'A';
	secret_key[2] = '6';
	secret_key[3] = 'z';
	secret_key[4] = 'k';
	secret_key[5] = 'S';
	secret_key[6] = '\0';

	// check that the game's backend is available
	GSIStartAvailableCheck(_T("gmtest"));
	while((result = GSIAvailableCheckThink()) == GSIACWaiting)
		msleep(5);
	if(result != GSIACAvailable)
	{
		_tprintf(_T("The backend is not available\n"));
		return 1;
	}

	// set the callbacks
	memset(&callbacks, 0, sizeof(PEERCallbacks));
	callbacks.disconnected = DisconnectedCallback;
	//callbacks.qrNatNegotiateCallback
	callbacks.gameStarted = GameStartedCallback;
	callbacks.playerChangedNick = PlayerChangedNickCallback;
	callbacks.playerJoined = PlayerJoinedCallback;
	callbacks.playerLeft = PlayerLeftCallback;
	callbacks.readyChanged = ReadyChangedCallback;
	callbacks.roomMessage = RoomMessageCallback;
	callbacks.playerMessage = PlayerMessageCallback;
	callbacks.roomUTM = RoomUTMCallback;
	callbacks.roomKeyChanged = RoomKeyChangedCallback;
	callbacks.qrKeyList = QRKeyListCallback;
	callbacks.qrServerKey = QRServerKeyCallback;
	callbacks.param = NULL;

	// initialize peer object, specifying the supported callbacks
	peer = peerInitialize(&callbacks);
	if(!peer)
	{
		_tprintf(_T("Failed to init\n"));
		return 1;
	}

	// ping/cross-ping in every room
	pingRooms[TitleRoom] = PEERTrue;
	pingRooms[GroupRoom] = PEERTrue;
	pingRooms[StagingRoom] = PEERTrue;
	crossPingRooms[TitleRoom] = PEERTrue;
	crossPingRooms[GroupRoom] = PEERTrue;
	crossPingRooms[StagingRoom] = PEERTrue;

	// set the title
	if(!peerSetTitle(peer, TITLE, secret_key, TITLE, secret_key, 0, maxUpdates, natNeg, pingRooms, crossPingRooms))
	{
		peerShutdown(peer);
		_tprintf(_T("Failed to set the title\n"));
		return 1;
	}

	// connect to the chat server
	_tprintf(_T("Connecting as %s..."), nick);
	peerConnect(peer, nick, profileID, NickErrorCallback, ConnectCallback, userData, blocking);
	if(!connectSuccess)
	{
		peerShutdown(peer);
		_tprintf(_T("Failed to connect\n"));
		return 1;
	}
	printf("Connected\n\n");

	// join the title room
	printf("Joining title room...");
	peerJoinTitleRoom(peer, NULL, JoinCallback, NULL, PEERTrue);
	if(!joinSuccess)
	{
		peerDisconnect(peer);
		peerShutdown(peer);
		_tprintf(_T("Failed to join the title room\n"));
		return 1;
	}
	printf("Joined\n\n");

	// list the group rooms
	printf("Listing group rooms:\n");
	peerListGroupRooms(peer, _T(""), ListGroupRoomsCallback, userData, non_blocking);

	while (!groupRoomCallbackDone)
	{
		peerThink(peer);
		msleep(10);
	}

	// send a chat message to the room
	printf("\nSending message to the Title Room...\n");
	peerMessageRoom(peer, TitleRoom, _T("Hi everyone in the Title Room!\n"), NormalMessage);

	// Loop for a while
	startTime = current_time();
	while((current_time() - startTime) < (1000))
	{
		peerThink(peer);
		msleep(10);
	} 

	_tprintf(_T("\nJoining Group Room 'Newbies'..."));
	peerJoinGroupRoom(peer, newbiesGroupID, JoinRoomCallback, userData, blocking);

	_tprintf(_T("\nPlayers in Group Room: \n"));
	peerEnumPlayers(peer, GroupRoom, EnumPlayersCallback, NULL);

	_tprintf(_T("Hosting Staging Room...\n"));
	peerCreateStagingRoom(peer, serverName, maxPlayers, noPassword, CreateStagingRoomCallback, userData, blocking);

	// Loop for a while
	startTime = current_time();
	while((current_time() - startTime) < (1000))
	{
		peerThink(peer);
		msleep(10);
	}

    //peerStartAutoMatch(peer, 3, _T(""), AutoMatchStatusCallback, AutoMatchRateCallback, NULL, PEERFalse);

	//peerStopListingGames(peer);

	// Leave the title room
	peerLeaveRoom(peer, TitleRoom, NULL);

	// Stop auto match if it's in progress
	//peerStopAutoMatch(peer);

	// disconnect local client from chat server (peerShutdown must still be called)
	peerDisconnect(peer);

	peerShutdown(peer); // clean up (free internal sdk memory) 

	GSI_UNUSED(argc);
	GSI_UNUSED(argv);
	return 0;
}
