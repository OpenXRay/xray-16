/***********************
qrcsample.c
GameSpy Query & Reporting SDK 
  
Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

******

 See the ReadMe file for qrcsample info, and consult the GameSpy Query & Reporting 2
 SDK documentation for more information on implementing the qr2 SDK.

************************/

/********
INCLUDES
********/
#include "../qr2.h"

/********
DEFINES
********/
// set some of the fixed server keys
#define GAME_VERSION	_T("2.00")
#define GAME_NAME		_T("gmtest")
#define MAX_PLAYERS		32
#define MAX_TEAMS		2
#define BASE_PORT		11111

// ensure cross-platform compatibility for printf
#ifdef _WIN32_WCE
	void RetailOutputA(CHAR *tszErr, ...);
	#define printf RetailOutputA
#elif defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

// define our additional keys, making sure not to overwrite the reserved standard key ids
// standard keys use 0-NUM_RESERVED_KEYS (defined in qr2regkeys.h)
#define GRAVITY_KEY 100
#define RANKINGON_KEY 101
#define TIME__KEY 102
#define AVGPING_T_KEY 103

/********
TYPDEFS
********/
//representative of a game player structure
typedef struct
{
	gsi_char pname[80];
	int pfrags;
	int pdeaths;
	int ptime;
	int pping;
	int pteam;
} player_t;

//representative of a team structure
typedef struct
{
	gsi_char tname[80];
	int tscore;
	int avgping;

} team_t;

//representative of a game data structure
typedef struct
{
	player_t players[MAX_PLAYERS];
	team_t teams[MAX_TEAMS];
	gsi_char mapname[20];
	gsi_char hostname[120];
	gsi_char gamemode[200];
	gsi_char gametype[30];
	int numteams;
	int numplayers;
	int maxplayers;
	int fraglimit;
	int timelimit;
	int teamplay;
	int rankingson;
	int gravity;
	int hostport;
} gamedata_t;

/********
GLOBAL VARS
********/
// just to give us bogus data
gsi_char *constnames[MAX_PLAYERS]=
{
	_T("Joe Player"), _T("L33t 0n3"), _T("Raptor"), _T("Gr81"),
	_T("Flubber"),    _T("Sarge"),    _T("Void"),   _T("runaway"),
	_T("Ph3ar"),      _T("wh00t"),    _T("gr1nder"),_T("Mace"),
	_T("stacy"),      _T("lamby"),    _T("Thrush"), _T("Leeroy")
};
gamedata_t gamedata;  // to store all the server/player/teamkeys

/********
DEBUG OUTPUT
********/
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

	#ifdef GSI_UNICODE
	static void AppDebug(const unsigned short* format, ...)
	{
        // Construct text, then pass in as ASCII
        unsigned short buf[1024];
        char tmp[2056];
        va_list aList;
        va_start(aList, format);
        vswprintf(buf, 1024, format, aList);

        UCS2ToAsciiString(buf, tmp);
        gsDebugFormat(GSIDebugCat_App, GSIDebugType_Misc, GSIDebugLevel_Notice,
            "%s", tmp);
	}
	#else
	static void AppDebug(const char* format, ...)
	{
		va_list aList;
		va_start(aList, format);
		gsDebugVaList(GSIDebugCat_App, GSIDebugType_Misc, GSIDebugLevel_Notice,
			format, aList);
	}
	#endif
#else
	#define AppDebug _tprintf
#endif

/********
PROTOTYPES - To prevent warnings on codewarrior strict compile
********/
void serverkey_callback(int keyid, qr2_buffer_t outbuf, void *userdata);
void playerkey_callback(int keyid, int index, qr2_buffer_t outbuf, void *userdata);
void teamkey_callback(int keyid, int index, qr2_buffer_t outbuf, void *userdata);
void keylist_callback(qr2_key_type keytype, qr2_keybuffer_t keybuffer, void *userdata);
int  count_callback(qr2_key_type keytype, void *userdata);
void adderror_callback(qr2_error_t error, gsi_char *errmsg, void *userdata);
void nn_callback(int cookie, void *userdata);
void cm_callback(gsi_char *data, int len, void *userdata);
void cc_callback(SOCKET gamesocket, struct sockaddr_in *remoteaddr, void *userdata);
void DoGameStuff(gsi_time totalTime);
int  test_main(int argc, char **argp);

// called when a server key needs to be reported
void serverkey_callback(int keyid, qr2_buffer_t outbuf, void *userdata)
{
	AppDebug("Reporting server keys\n");

	switch (keyid)
	{
	case HOSTNAME_KEY:
		qr2_buffer_add(outbuf, gamedata.hostname);
		break;
	case GAMEVER_KEY:
		qr2_buffer_add(outbuf, GAME_VERSION);
		break;
	case HOSTPORT_KEY:
		qr2_buffer_add_int(outbuf, gamedata.hostport);
		break;
	case MAPNAME_KEY:
		qr2_buffer_add(outbuf, gamedata.mapname);
		break;
	case GAMETYPE_KEY:
		qr2_buffer_add(outbuf, gamedata.gametype);
		break;
	case NUMPLAYERS_KEY:
		qr2_buffer_add_int(outbuf, gamedata.numplayers);
		break;
	case NUMTEAMS_KEY:
		qr2_buffer_add_int(outbuf, gamedata.numteams);
		break;
	case MAXPLAYERS_KEY:
		qr2_buffer_add_int(outbuf, gamedata.maxplayers);
		break;
	case GAMEMODE_KEY:
		qr2_buffer_add(outbuf, gamedata.gamemode);
		break;
	case TEAMPLAY_KEY:
		qr2_buffer_add_int(outbuf, gamedata.teamplay);
		break;
	case FRAGLIMIT_KEY:
		qr2_buffer_add_int(outbuf, gamedata.fraglimit);
		break;
	case TIMELIMIT_KEY:
		qr2_buffer_add_int(outbuf, gamedata.timelimit);
		break;
	case GRAVITY_KEY:
		qr2_buffer_add_int(outbuf, gamedata.gravity);
		break;
	case RANKINGON_KEY:
		qr2_buffer_add_int(outbuf, gamedata.rankingson);
		break;
	default:
		qr2_buffer_add(outbuf, _T(""));
	}
	
	GSI_UNUSED(userdata);
}

// called when a player key needs to be reported
void playerkey_callback(int keyid, int index, qr2_buffer_t outbuf, void *userdata)
{
	AppDebug("Reporting player keys\n");
	
	//check for valid index
	if (index >= gamedata.numplayers)
	{
		qr2_buffer_add(outbuf, _T(""));
		return;
	}
	switch (keyid)
	{
	case PLAYER__KEY:
		qr2_buffer_add(outbuf, gamedata.players[index].pname);
		break;
	case SCORE__KEY:
		qr2_buffer_add_int(outbuf, gamedata.players[index].pfrags);
		break;
	case DEATHS__KEY:
		qr2_buffer_add_int(outbuf, gamedata.players[index].pdeaths);
		break;
	case PING__KEY:
		qr2_buffer_add_int(outbuf, gamedata.players[index].pping);
		break;
	case TEAM__KEY:
		qr2_buffer_add_int(outbuf, gamedata.players[index].pteam);
		break;
	case TIME__KEY:
		qr2_buffer_add_int(outbuf, gamedata.players[index].ptime);
		break;
	default:
		qr2_buffer_add(outbuf, _T(""));
		break;		
	}
	
	GSI_UNUSED(userdata);
}

// called when a team key needs to be reported
void teamkey_callback(int keyid, int index, qr2_buffer_t outbuf, void *userdata)
{
	AppDebug("Reporting team keys\n");

	//check for valid index
	if (index >= gamedata.numteams)
	{
		qr2_buffer_add(outbuf, _T(""));
		return;
	}
	switch (keyid)
	{
	case TEAM_T_KEY:
		qr2_buffer_add(outbuf, gamedata.teams[index].tname);
		break;
	case SCORE_T_KEY:
		qr2_buffer_add_int(outbuf, gamedata.teams[index].tscore);
		break;
	case AVGPING_T_KEY:
		qr2_buffer_add_int(outbuf, gamedata.teams[index].avgping);
		break;
	default:
		qr2_buffer_add(outbuf, _T(""));
		break;		
	}
	
	GSI_UNUSED(userdata);
}	

// called when we need to report the list of keys we report values for
void keylist_callback(qr2_key_type keytype, qr2_keybuffer_t keybuffer, void *userdata)
{
	AppDebug("Reporting keylist\n");

	//need to add all the keys we support
	switch (keytype)
	{
	case key_server:
		qr2_keybuffer_add(keybuffer, HOSTNAME_KEY);
		qr2_keybuffer_add(keybuffer, GAMEVER_KEY);
		qr2_keybuffer_add(keybuffer, HOSTPORT_KEY);
		qr2_keybuffer_add(keybuffer, MAPNAME_KEY);
		qr2_keybuffer_add(keybuffer, GAMETYPE_KEY);
		qr2_keybuffer_add(keybuffer, NUMPLAYERS_KEY);
		qr2_keybuffer_add(keybuffer, NUMTEAMS_KEY);
		qr2_keybuffer_add(keybuffer, MAXPLAYERS_KEY);
		qr2_keybuffer_add(keybuffer, GAMEMODE_KEY);
		qr2_keybuffer_add(keybuffer, TEAMPLAY_KEY);
		qr2_keybuffer_add(keybuffer, FRAGLIMIT_KEY);
		qr2_keybuffer_add(keybuffer, TIMELIMIT_KEY);
		qr2_keybuffer_add(keybuffer, GRAVITY_KEY); //a custom key
		qr2_keybuffer_add(keybuffer, RANKINGON_KEY); //a custom key
		break;
	case key_player:
		qr2_keybuffer_add(keybuffer, PLAYER__KEY);
		qr2_keybuffer_add(keybuffer, SCORE__KEY);
		qr2_keybuffer_add(keybuffer, DEATHS__KEY);
		qr2_keybuffer_add(keybuffer, PING__KEY);
		qr2_keybuffer_add(keybuffer, TEAM__KEY);
		qr2_keybuffer_add(keybuffer, TIME__KEY); //a custom key
		break;
	case key_team:
		qr2_keybuffer_add(keybuffer, TEAM_T_KEY);
		qr2_keybuffer_add(keybuffer, SCORE_T_KEY);
		qr2_keybuffer_add(keybuffer, AVGPING_T_KEY); //a custom key
		break;
	default: break;
	}
	
	GSI_UNUSED(userdata);
}

// called when we need to report the number of players and teams
int count_callback(qr2_key_type keytype, void *userdata)
{
	AppDebug("Reporting number of players/teams\n");

	if (keytype == key_player)
		return gamedata.numplayers;
	else if (keytype == key_team)
		return gamedata.numteams;
	else
		return 0;

	GSI_UNUSED(userdata);
}

// called if our registration with the GameSpy master server failed
void adderror_callback(qr2_error_t error, gsi_char *errmsg, void *userdata)
{
	GS_ASSERT(errmsg)
	AppDebug("Error adding server: %d, %s\n", error, errmsg);
	GSI_UNUSED(userdata);
}

// called when a client wants to connect using nat negotiation 
// (Nat Negotiation must be enabled in qr2_init)
void nn_callback(int cookie, void *userdata)
{
	AppDebug("Got natneg cookie: %d\n", cookie);
	GSI_UNUSED(userdata);
}

// called when a client sends a message to the server through qr2 (not commonly used)
void cm_callback(gsi_char *data, int len, void *userdata)
{
	AppDebug("Got %d bytes from client\n", len);
	GSI_UNUSED(data);
	GSI_UNUSED(userdata);
}

// called when a client has connected
void cc_callback(SOCKET gamesocket, struct sockaddr_in *remoteaddr, void *userdata)
{
	AppDebug("Client connected from %s:%d\n", inet_ntoa(remoteaddr->sin_addr), ntohs(remoteaddr->sin_port));
	GSI_UNUSED(gamesocket);
	GSI_UNUSED(userdata);
}

// initialize the gamedata structure with bogus data
static void init_game(void)
{
	int i;
	AppDebug("Generating game data\n");
	srand((unsigned int) current_time() );
	gamedata.numplayers = rand() % 15 + 1;
	gamedata.maxplayers = MAX_PLAYERS;
	for (i = 0 ; i < gamedata.numplayers ; i++)
	{
		_tcscpy(gamedata.players[i].pname, constnames[i]);
		gamedata.players[i].pfrags = rand() % 32;
		gamedata.players[i].pdeaths = rand() % 32;
		gamedata.players[i].ptime = rand() % 1000;
		gamedata.players[i].pping = rand() % 500;
		gamedata.players[i].pteam = rand() % 2;
	}
	gamedata.numteams = 2;
	for (i = 0 ; i < gamedata.numteams ; i++)
	{
		gamedata.teams[i].tscore = rand() % 500;
		gamedata.teams[i].avgping = rand() % 500;
	}

	_tcscpy(gamedata.teams[0].tname,_T("Red"));
	_tcscpy(gamedata.teams[1].tname,_T("Blue"));
	_tcscpy(gamedata.mapname,_T("gmtmap1"));
	_tcscpy(gamedata.gametype,_T("arena"));
	_tcscpy(gamedata.hostname,_T("GameSpy QR2 Sample"));
	_tcscpy(gamedata.gamemode,_T("openplaying"));

	gamedata.fraglimit = 0;
	gamedata.timelimit = 40;
	gamedata.teamplay = 1;
	gamedata.rankingson = 1;
	gamedata.gravity = 800;
	gamedata.hostport = 25000;
}

// simulate whatever else a game server does 
void DoGameStuff(gsi_time totalTime)
{
	// After 30 seconds, we will change the game map and call qr2_send_statechanged
	// This should only be called after major changes such as mapname or gametype, and
	// cannot be applied more than once every 10 seconds (subsequent calls will be delayed 
	// when necessary)
	static int stateChanged = 0; 
	if (!stateChanged && totalTime > 30000) {
		AppDebug("Mapname changed, calling qr2_send_statechanged\n");
		_tcscpy(gamedata.mapname,_T("gmtmap2"));
		qr2_send_statechanged(NULL);
		stateChanged = 1;
	}

	msleep(10);
}


int test_main(int argc, char **argp)
{			
	/* qr2_init parameters */
	gsi_char  secret_key[9];         // your title's assigned secret key
	gsi_char  ip[255];               // to manually set local IP
	const int isPublic = 1;          // set to '0' for a LAN game
	const int isNatNegSupported = 1; // set to '0' if you don't support Nat Negotiation
	gsi_time  aStartTime = 0;        // for sample, so we don't run forever
	void * userData = NULL;          // optional data that will be passed to the callback functions

	// for debug output on these platforms
#if defined (_PS3) || defined (_PS2) || defined (_PSP) || defined(_NITRO)
	#ifdef GSI_COMMON_DEBUG
		// Define GSI_COMMON_DEBUG if you want to view the SDK debug output
		// Set the SDK debug log file, or set your own handler using gsSetDebugCallback
		//gsSetDebugFile(stdout); // output to console
		gsSetDebugCallback(DebugCallback);

		// Set debug levels
		gsSetDebugLevel(GSIDebugCat_All, GSIDebugType_All, GSIDebugLevel_Verbose);
	#endif
#endif

	//set the secret key, in a semi-obfuscated manner
	secret_key[0] = 'H';
	secret_key[1] = 'A';
	secret_key[2] = '6';
	secret_key[3] = 'z';
	secret_key[4] = 'k';
	secret_key[5] = 'S';
	secret_key[6] = '\0';

	// register our custom keys (you do not have to register the reserved standard keys)
	AppDebug("Registering custom keys\n");
	qr2_register_key(GRAVITY_KEY, _T("gravity")    );
	qr2_register_key(RANKINGON_KEY, _T("rankingon"));
	qr2_register_key(TIME__KEY,     _T("time_")    ); // player keys always end with '_'
	qr2_register_key(AVGPING_T_KEY, _T("avgping_t")); // team keys always end with '_t'

	// create some random game data
	init_game();

	// Check if we want to override our IP  (otherwise qr2 will set for us)	
#ifndef GSI_UNICODE
	if (argc>1)
		strcpy(ip, argp[1]);
#else
	if (argc>1)
		AsciiToUCS2String(argp[1], ip);
#endif

	AppDebug("Initializing SDK; server should show up on the master list within 6-10 sec.\n");
	//Call qr_init with the query port number and gamename, default IP address, and no user data
	//Pass NULL for the qrec parameter (first parameter) as long as you're running a single game 
	//server instance per process
	//Reference gt2nat sample for qr2_init_socket implementation
	if (qr2_init(NULL,argc>1?ip:NULL,BASE_PORT,GAME_NAME, secret_key, isPublic, isNatNegSupported,
		serverkey_callback, playerkey_callback, teamkey_callback,
		keylist_callback, count_callback, adderror_callback, userData) != e_qrnoerror)
	{
		printf("Error starting query sockets\n");
		return -1;
	}

	// Set a function to be called when we receive a game specific message
	qr2_register_clientmessage_callback(NULL, cm_callback);

	// Set a function to be called when we receive a nat negotiation request
	qr2_register_natneg_callback(NULL, nn_callback);

	// Set a function to be called when a client has connected
	qr2_register_clientconnected_callback(NULL, cc_callback);

	// Enter the main loop
	AppDebug("Sample will quit after 60 seconds\n");
	aStartTime = current_time();
	while ((current_time() - aStartTime) < 60000)
	{
		gsi_time totalTime = current_time() - aStartTime; // used to change the game state after 30 seconds

		// An actual game would do something between "thinks"
		DoGameStuff(totalTime);

		//check for / process incoming queries
		//should be called every 10-100 ms; quicker calls produce more accurate ping measurements
		qr2_think(NULL);
	}

	AppDebug("Shutting down - server will be removed from the master server list\n");
	//let gamemaster know we are shutting down (removes dead server entry from the list)
	qr2_shutdown(NULL);

	
#ifdef GSI_UNICODE
	// In Unicode mode we must perform additional cleanup
	qr2_internal_key_list_free();
#endif

	// Finished
	return 0;
}

