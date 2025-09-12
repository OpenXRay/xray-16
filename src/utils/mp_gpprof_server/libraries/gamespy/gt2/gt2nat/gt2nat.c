/******
gt2nat.c
GameSpy Transport 2 SDK 
  
Copyright 2000 GameSpy Industries, Inc

******

 This sample demonstrates sharing a UDP socket with the Query & Reporting 2 SDK 
 to enable developers to create games that can be hosted behind a NAT.

 Please see the GameSpy Query & Reporting 2 SDK documentation for more 
 information

******/


/********
INCLUDES
********/
#include "../gt2.h"
#include "../../qr2/qr2.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) && !defined(UNDER_CE)
#include <conio.h>
#endif

/********
DEFINES
********/
#define QR2_GAME_VERSION "2.00"
#define QR2_GAME_NAME "gmtest"
#define QR2_MAX_PLAYERS 32
#define QR2_BASE_PORT 26900
#define QR2_RANKINGSON_KEY 100

#ifdef _WIN32_WCE
void RetailOutputA(CHAR *tszErr, ...);
#define printf RetailOutputA
#endif

/********
TYPDEFS
********/
//representative of a game player structure
typedef struct
{
	char pname[80];
	int pfrags;
	int pdeaths;
	int pskill;
	int pping;
	char pteam[80];
} player_t;

//representative of a game data structure
typedef struct
{
	player_t players[QR2_MAX_PLAYERS];
	char mapname[20];
	char hostname[120];
	char gamemode[200];
	char gametype[30];
	int locationid;
	int numplayers;
	int maxplayers;
	int fraglimit;
	int timelimit;
	int teamplay;
	int rankingson;
	int hostport;
} gamedata_t;

/********
GLOBAL VARS
********/

//just to give us bogus data
char *constnames[QR2_MAX_PLAYERS]={"Joe Player","L33t 0n3","Raptor","Gr81","Flubber","Sarge","Void","runaway","Ph3ar","wh00t","gr1nder","Mace","stacy","lamby","Thrush"};
gamedata_t gamedata;

// Called when a server key needs to be reported
void serverkey_callback(int keyid, qr2_buffer_t outbuf, void *userdata)
{
	switch (keyid)
	{
	case HOSTNAME_KEY:
		qr2_buffer_add(outbuf, gamedata.hostname);
		break;
	case GAMEVER_KEY:
		qr2_buffer_add(outbuf, QR2_GAME_VERSION);
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
	case QR2_RANKINGSON_KEY:
		qr2_buffer_add_int(outbuf, gamedata.rankingson);
		break;
	default:
		qr2_buffer_add(outbuf, _T(""));
	}
	
	GSI_UNUSED(userdata);
}

// Called when a player key needs to be reported
void playerkey_callback(int keyid, int index, qr2_buffer_t outbuf, void *userdata)
{
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
	case SKILL__KEY:
		qr2_buffer_add_int(outbuf, gamedata.players[index].pskill);
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
		qr2_buffer_add(outbuf, gamedata.players[index].pteam);
		break;
	default:
		qr2_buffer_add(outbuf, _T(""));
		break;		
	}
	
	GSI_UNUSED(userdata);
}

// Called when a team key needs to be reported
void teamkey_callback(int keyid, int index, qr2_buffer_t outbuf, void *userdata)
{
	qr2_buffer_add(outbuf, _T(""));

	GSI_UNUSED(userdata);
	GSI_UNUSED(index);
	GSI_UNUSED(keyid);
}	

// Called when we need to report the list of keys we report values for
void keylist_callback(qr2_key_type keytype, qr2_keybuffer_t keybuffer, void *userdata)
{
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
		break;
	case key_player:
		qr2_keybuffer_add(keybuffer, PLAYER__KEY);
		qr2_keybuffer_add(keybuffer, SCORE__KEY);
		qr2_keybuffer_add(keybuffer, SKILL__KEY);
		qr2_keybuffer_add(keybuffer, DEATHS__KEY);
		qr2_keybuffer_add(keybuffer, PING__KEY);
		qr2_keybuffer_add(keybuffer, TEAM__KEY);
		break;
	case key_team:
		break;
	}
	
	GSI_UNUSED(userdata);
}

// Called when we need to report the number of players and teams
int count_callback(qr2_key_type keytype, void *userdata)
{
	if (keytype == key_player)
		return gamedata.numplayers;
	else if (keytype == key_team)
		return 0;
	else
		return 0;
		
	GSI_UNUSED(userdata);
}

// Called if our registration with the GameSpy master server failed
void adderror_callback(qr2_error_t error, gsi_char *errmsg, void *userdata)
{
	_tprintf(_T("Error adding server: %d, %s\n"), error, errmsg);
	
	GSI_UNUSED(userdata);
}

/***********
init_game
Initialize the sample data structures with bogus data
************/
static void init_game(void)
{
	int i;
	int team;

	srand((unsigned int) current_time() );
	gamedata.numplayers = rand() % 15;
	gamedata.maxplayers = QR2_MAX_PLAYERS;
	for (i = 0 ; i < gamedata.numplayers ; i++)
	{
		strcpy(gamedata.players[i].pname, constnames[i]);
		gamedata.players[i].pfrags = rand() % 32;
		gamedata.players[i].pdeaths = rand() % 32;
		gamedata.players[i].pskill = rand() % 1000;
		gamedata.players[i].pping = rand() % 500;
		team = rand() % 3;
		if (team == 0)
			strcpy(gamedata.players[i].pteam,"Red");
		else if (team == 1)
			strcpy(gamedata.players[i].pteam,"Blue");
		else if (team == 2)
			strcpy(gamedata.players[i].pteam,"");
	}
	strcpy(gamedata.mapname,"gmtmap1");
	strcpy(gamedata.gametype,"arena");
	strcpy(gamedata.hostname,"GameMaster Arena Server");
	strcpy(gamedata.gamemode,"openplaying");
	gamedata.fraglimit = 0;
	gamedata.timelimit = 40;
	gamedata.teamplay = 1;
	gamedata.locationid = 1;
	gamedata.rankingson = 1;
	gamedata.hostport = 25000;
}

/*******
 DoGameStuff
Simulate whatever else a game server does 
********/
void DoGameStuff(void)
{
	msleep(10);
}

GT2Bool UnrecognizedMessageCallback(GT2Socket socket, unsigned int ip, unsigned short port, GT2Byte * message, int len)
{
	static char buffer[8 * 1024];
	struct sockaddr_in saddr;

	if(!len || !message || ((message[0] != QR_MAGIC_1) && (message[1] != QR_MAGIC_2) && (message[0] != '\\')))
		return GT2False;

	// we want to make sure it is NUL-terminated
	len = min(len, (sizeof(buffer) - 1));
	memcpy(buffer, message, len);
	buffer[len] = '\0';

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = ip;
	saddr.sin_port = htons(port);
	//qr_parse_query(NULL, buffer, len, (struct sockaddr *)&saddr);
	qr2_parse_query(NULL, buffer, len, (struct sockaddr *)&saddr);
	return GT2True;

	GSI_UNUSED(socket);
}

void ConnectAttemptCallback
(
	GT2Socket socket,
	GT2Connection connection,
	unsigned int ip,
	unsigned short port,
	int latency,
	GT2Byte * message,
	int len
)
{
	printf("Connection attempt from %s (%d ping)\n", gt2AddressToString(ip, port, NULL), latency);

	gt2Reject(connection, NULL, 0);

	GSI_UNUSED(len);
	GSI_UNUSED(message);
	GSI_UNUSED(socket);
}

/*******************
 main
Simulates a main program loop
First, initializes the Q&R items, then enters a main loop
*****************/
#if defined(_PS2)
int test_main(int argc, char **argp)
#else
int main(int argc, char **argp)
#endif
{
	char secret_key[9];
	GT2Socket socket;
	GT2Result result;
	int natNegotiate = 1;
	result = gt2CreateSocket(&socket, gt2AddressToString(0, QR2_BASE_PORT, NULL), 0, 0, NULL);
	if(result != GT2Success)
		return -1;

	gt2Listen(socket, ConnectAttemptCallback);
	
	//set the secret key, in a semi-obfuscated manner
	secret_key[0] = 'H';
	secret_key[1] = 'A';
	secret_key[2] = '6';
	secret_key[3] = 'z';
	secret_key[4] = 'k';
	secret_key[5] = 'S';
	secret_key[6] = '\0';
	
	qr2_register_key(QR2_RANKINGSON_KEY, _T("rankingson"));
	/*
	//call qr_init_socket with the socket and gamename
	if (qr_init_socket(NULL,gt2GetSocketSOCKET(socket), QR2_GAME_NAME, secret_key, basic_callback, 
		info_callback, rules_callback, players_callback, NULL) != 0)
	{
		printf("Error starting Q&R SDK\n");
		return -1;
	}
	*/

	// call the qr2_init_socket with the socket and gamename
	if (qr2_init_socket(NULL, gt2GetSocketSOCKET(socket), QR2_BASE_PORT, QR2_GAME_NAME, secret_key, 1, natNegotiate,
						serverkey_callback, playerkey_callback, teamkey_callback, keylist_callback, count_callback,
						adderror_callback, NULL) != e_qrnoerror)

	{
		printf("Error starting QR2 SDK\n");
		return -1;
	}
	// set the unrecognized message callback
	gt2SetUnrecognizedMessageCallback(socket, UnrecognizedMessageCallback);

	init_game();

	printf("Press any key to quit\n");
#if defined(_WIN32) && !defined(UNDER_CE)
	while (!_kbhit())
#else
	while (1)
#endif
	{
		DoGameStuff();
		//process our game networking
		gt2Think(socket);
		//check for / process incoming queries
		//qr_process_queries(NULL);
		qr2_think(NULL);
	}
	//let gamemaster know we are shutting down
	strcpy(gamedata.gamemode,"exiting");
	//qr_send_exiting(NULL);
	//qr_shutdown(NULL);
	qr2_shutdown(NULL);
	gt2CloseSocket(socket);
	return 0;

	GSI_UNUSED(argp);
	GSI_UNUSED(argc);
}
