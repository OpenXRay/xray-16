/******
statstest.c
GameSpy Stats/Tracking SDK 
  
Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

******

Please see the GameSpy Stats and Tracking SDK for more info

This file demonstrates usage of the various SDK functions by 
simulating a game "host".

******/
#include <stdarg.h>
#include "../gstats.h"
#include "../../common/gsAvailable.h"

#ifdef UNDER_CE
	void RetailOutputA(CHAR *tszErr, ...);
	#define printf RetailOutputA
#elif defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

#define MY_GAMEPORT 2667
#define MY_VERSION 1.23
#define SNAPSHOT_LEN 1500

typedef struct
{
	gsi_char hostname[65];
	gsi_char mapname[30];
	gsi_char gametype[30];
	int numplayers;
	float version;
	//and other misc data

} serverinfo_t;

typedef struct
{
	gsi_char name[32];
	int points;
	int deaths;
	int profileid;
	gsi_char auth[33];
	//and othem misc data
} playerinfo_t;

static char *va(char *format, ...);
static gsi_char *CreateSnapShotFromData(gsi_char *snapshot, serverinfo_t *psinfo, playerinfo_t players[]);

#ifdef __MWERKS__	// CodeWarrior warns if not protoyped
	int test_main(int argc, char **argv);
#endif

int test_main(int argc, char **argv)
{
	gsi_char snapshot[SNAPSHOT_LEN];
	serverinfo_t sinfo;
	playerinfo_t players[64];
	statsgame_t g1, g2;
	GSIACResult aResult = GSIACWaiting;
	int anError = 0;
	gsi_time startTime;

/*********
First step, set our authentication info
We could do:
	strcpy(gcd_gamename,"gmtest");
	strcpy(gcd_secret_key,"HA6zkS");
...but this is more secure:
**********/
	gcd_gamename[0]='g';gcd_gamename[1]='m';gcd_gamename[2]='t';gcd_gamename[3]='e';
	gcd_gamename[4]='s';gcd_gamename[5]='t';gcd_gamename[6]='\0';
	gcd_secret_key[0]='H';gcd_secret_key[1]='A';gcd_secret_key[2]='6';gcd_secret_key[3]='z';
	gcd_secret_key[4]='k';gcd_secret_key[5]='S';gcd_secret_key[6]='\0';

/*********
Make sure we're using the development backend
**********/
	strcpy(StatsServerHostname, "sdkdev." GSI_DOMAIN_NAME);

/*********
Make sure the GameSpy backend services are available
**********/
	GSIStartAvailableCheck(_T("gmtest"));
	while(aResult == GSIACWaiting)
	{
		aResult = GSIAvailableCheckThink();
		msleep(50);
	}
	if (aResult == GSIACUnavailable)
	{
		printf("Backend services are not available\r\n");
		return 0;
	}
	
/*********
Next, open the stats connection. This may block for
a 1-2 seconds, so it should be done before the actual game starts.
**********/
	_tprintf(_T("Connecting to the stats server...\n"));
	startTime = current_time();
	anError = InitStatsAsync(MY_GAMEPORT, 200000); // 20 second timeout
	while(anError == GE_CONNECTING)
	{
		anError = InitStatsThink();
		msleep(50);
	}
	if (anError != GE_NOERROR)
	{
		_tprintf(_T("Failed to connect to stats server: %d\r\n"), anError);
		return 0;
	}
	_tprintf(_T("Connected to stats server (%dms)\n"), current_time() - startTime);


/******************
*******************
SAMPLE 1
NON BUCKET BASED
*******************
******************/

/*********
Now we are ready to record a game. To start with, we'll assume that we are
only going to run one game at a time on this "server". That means we can
discard the return value of NewGame and pass NULL into anything that requires
a statsgame_t.

The first example will not be bucket based, we'll generate the snapshot ourselves.
**********/
	NewGame(0); /* 0 = Not using buckets, manually generate snapshot */
/********
Now we'll "simulate" a server setting up by filling
in some of our structures
*********/
	_tcscpy(sinfo.hostname, _T("My l33t Server"));
	_tcscpy(sinfo.mapname, _T("Level 33"));
	_tcscpy(sinfo.gametype, _T("hunter"));
	sinfo.numplayers = 0;
	sinfo.version = (float)MY_VERSION;

/*******
Lets simulate some players joining
If you are using authentication, you will need to send a challenge
to the player. You can get the challenge to send by calling GetChallenge
********/
	_tcscpy(players[sinfo.numplayers].name,_T("Bob!"));
	players[sinfo.numplayers].points = 0;
	players[sinfo.numplayers].deaths = 0;
	players[sinfo.numplayers].profileid = 32432423;
	//assume we got this value from a challenge reply
	_tcscpy(players[sinfo.numplayers].auth,_T("7cca8e60a13781eebc820a50754f57cd"));
	sinfo.numplayers++;

	_tcscpy(players[sinfo.numplayers].name,_T("Joey"));
	players[sinfo.numplayers].points = 0;
	players[sinfo.numplayers].deaths = 0;
	players[sinfo.numplayers].profileid = 643423;
	//assume we got this value from a challenge reply
	_tcscpy(players[sinfo.numplayers].auth,_T("19ea14d9d92a7fcc635cf5716944d9bc"));
	sinfo.numplayers++;

/*******
Now simulate the players "playing" by giving them some points
********/
	//bob kills joey
	players[0].points++;
	players[1].deaths++;
	//joey kills bob
	players[1].points++;
	players[0].deaths++;
	//etc..
	players[0].points++;
	players[1].deaths++;
	players[1].points++;
	players[0].deaths++;
	players[0].points++;
	players[1].deaths++;
	players[0].points++;
	players[1].deaths++;
/******
Now we are in the middle of the game, so send a snapshot, in case we
"crash" before the game finishes.
******/
	CreateSnapShotFromData(snapshot, &sinfo, players);
/*******
Now send in the snapshot
*******/
	_tprintf(_T("Sending update snapshot\n"));
	SendGameSnapShot(NULL, snapshot, SNAP_UPDATE);

/******
Simulate some more gameplay
*******/
	players[0].points++;
	players[1].deaths++;
	players[1].points++;
	players[0].deaths++;
	players[0].points++;
	players[1].deaths++;
/******
Game is now "done", create another snapshot and send it in as final
*******/
	CreateSnapShotFromData(snapshot, &sinfo, players);
	_tprintf(_T("Sending final snapshot\n"));
	SendGameSnapShot(NULL, snapshot, SNAP_FINAL);

/*******
gsifree the game when done
*******/
	FreeGame(NULL);

/*********
..and that's all there is.
*********/


/******************
*******************
SAMPLE 2
BUCKET BASED
*******************
******************/

/********
Now lets simulate the same thing
using bucket based snapshots instead of building them ourselves.

We are already connected, so we can just create a new game.
**********/
	NewGame(1); /* this time we use buckets */

/********
Now we'll set up the server and player info, this time using
buckets instead of internal structures. Note that you will probably
have copies of this stuff in internal structures as well.
********/
	BucketStringOp(NULL,"hostname",bo_set,"My L33t Server",bl_server, 0);
	BucketStringOp(NULL,"mapname",bo_set,"Level 33",bl_server, 0);
	BucketStringOp(NULL,"gametype",bo_set,"hunted",bl_server, 0);
	BucketFloatOp(NULL,"gamever", bo_set,MY_VERSION, bl_server, 0);

	/* add the players */
	NewPlayer(NULL, 0, _T("Bob!"));
	BucketIntOp(NULL,"pid",bo_set,32432423,bl_player, 0);
	BucketStringOp(NULL,"auth",bo_set,"7cca8e60a13781eebc820a50754f57cd",bl_player, 0);

	NewPlayer(NULL, 1, _T("Joey!"));
	BucketIntOp(NULL,"pid",bo_set,643423,bl_player, 1);
	BucketStringOp(NULL,"auth",bo_set,"19ea14d9d92a7fcc635cf5716944d9bc",bl_player, 1);

/*******
Again we simulate the game, this time updating values in the buckets
********/
	BucketIntOp(NULL, "score",bo_add, 1, bl_player, 0);
	BucketIntOp(NULL, "deaths",bo_add, 1, bl_player, 1);
	BucketIntOp(NULL, "score",bo_add, 1, bl_player, 1);
	BucketIntOp(NULL, "deaths",bo_add, 1, bl_player, 0);
	BucketIntOp(NULL, "score",bo_add, 1, bl_player, 0);
	BucketIntOp(NULL, "deaths",bo_add, 1, bl_player, 1);
	BucketIntOp(NULL, "score",bo_add, 1, bl_player, 1);
	BucketIntOp(NULL, "deaths",bo_add, 1, bl_player, 0);
	BucketIntOp(NULL, "score",bo_add, 1, bl_player, 0);
	BucketIntOp(NULL, "deaths",bo_add, 1, bl_player, 1);
	BucketIntOp(NULL, "score",bo_add, 1, bl_player, 0);
	BucketIntOp(NULL, "deaths",bo_add, 1, bl_player, 1);
/*****
Send our midway snapshot, this time the snapshot is
generated automatically by the buckets
******/
	SendGameSnapShot(NULL, NULL, SNAP_UPDATE);

	/* More of the game */
	BucketIntOp(NULL, "score",bo_add, 1, bl_player, 0);
	BucketIntOp(NULL, "deaths",bo_add, 1, bl_player, 1);
	BucketIntOp(NULL, "score",bo_add, 1, bl_player, 1);
	BucketIntOp(NULL, "deaths",bo_add, 1, bl_player, 0);
	BucketIntOp(NULL, "score",bo_add, 1, bl_player, 0);
	BucketIntOp(NULL, "deaths",bo_add, 1, bl_player, 1);
/*****
Send the final snapshot and gsifree the game, which frees
the buckets as well
******/
	_tprintf(_T("Sending final snapshot\n"));
	SendGameSnapShot(NULL,NULL, SNAP_FINAL);
	FreeGame(NULL);


/******************
*******************
SAMPLE 3
ADVANCED BUCKET BASED
*******************
******************/

/**************
Now lets show some of the advanced features of the system by:
1. Connecting and disconnecting clients
2. Using teams 
3. Recording multiple, simultaneous games
4. Recording new types of data
**************/

/*******
Create a new bucket based game, this time save off the pointer,
since we'll be creating another game before this one is done
*******/
	g1 = NewGame(1);
	
/*********
Start the game running...
*********/
	BucketStringOp(g1,"hostname",bo_set,"Advanced Server",bl_server, 0);
	BucketStringOp(g1,"mapname",bo_set,"Level 39",bl_server, 0);
	BucketStringOp(g1,"gametype",bo_set,"ctf",bl_server, 0);
	BucketFloatOp(g1,"gamever", bo_set,MY_VERSION, bl_server, 0);
	BucketIntOp(g1, "scorelimit", bo_set, 30,bl_server, 0);
	BucketIntOp(g1, "timelimit",bo_set, 30, bl_server, 0);

	NewPlayer(g1, 0, _T("Bob!"));
	BucketIntOp(g1,"pid",bo_set,32432423,bl_player, 0);
	BucketStringOp(g1,"auth",bo_set,"7cca8e60a13781eebc820a50754f57cd",bl_player, 0);

	NewPlayer(g1, 1, _T("Joey"));
	BucketIntOp(g1,"pid",bo_set,643423,bl_player, 1);
	BucketStringOp(g1,"auth",bo_set,"19ea14d9d92a7fcc635cf5716944d9bc",bl_player, 1);

/********
This time create teams and assign the players to them.
Note that we use "GetTeamIndex" to get the translated index
for the team.
**********/
	NewTeam(g1,0, _T("Team number 1"));
	NewTeam(g1,1, _T("Team number 2"));

	BucketIntOp(g1,"team",bo_set,GetTeamIndex(g1, 0),bl_player, 0);
	BucketIntOp(g1,"team",bo_set,GetTeamIndex(g1, 1),bl_player, 1);

/*******
Keep running the game, and watch as players join and leave, etc
********/
	BucketIntOp(g1, "score",bo_add, 1, bl_player, 0);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 1);
	BucketIntOp(g1, "score",bo_add, 1, bl_player, 0);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 1);	

	NewPlayer(g1,2,_T("Jeronimo"));
	BucketIntOp(g1,"pid",bo_set,54684,bl_player, 2);
	BucketStringOp(g1,"auth",bo_set,"6a7d3fee261eb3db7f94f0f02c2c756b",bl_player, 2);
	BucketIntOp(g1,"team",bo_set,GetTeamIndex(g1, 0),bl_player, 2);

	BucketIntOp(g1, "score",bo_add, 1, bl_player, 2);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 1);	

	RemovePlayer(g1, 0);

	BucketIntOp(g1, "score",bo_add, 1, bl_player, 2);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 1);	
	BucketIntOp(g1, "score",bo_add, 1, bl_player, 1);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 2);	

	NewPlayer(g1,0,_T("Killer"));
	BucketIntOp(g1,"pid",bo_set,32323,bl_player, 0);
	BucketStringOp(g1,"auth",bo_set,"9e67d812bde48a6a29ee425c03b26894",bl_player, 0);
	BucketIntOp(g1,"team",bo_set,GetTeamIndex(g1,0),bl_player, 0);

	BucketIntOp(g1, "suicides",bo_add, 1, bl_player, 0);
	BucketIntOp(g1, "score",bo_add, 1, bl_player, 1);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 0);	
	BucketIntOp(g1, "suicides",bo_add, 1, bl_player, 0);

	RemovePlayer(g1, 1);
	RemoveTeam(g1, 1);
	BucketIntOp(g1, "score",bo_add, 1, bl_player, 2);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 0);	
	BucketIntOp(g1, "score",bo_add, 1, bl_player, 2);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 0);	

	NewPlayer(g1,1,_T("Killer")); /* you can have two players with the same name */
	BucketIntOp(g1,"pid",bo_set,32323,bl_player, 1);
	BucketStringOp(g1,"auth",bo_set,"dd975866a2c346814433e523bf8eda0e",bl_player, 1);
	NewTeam(g1, 1, _T("Another Team"));
	BucketIntOp(g1,"team",bo_set,GetTeamIndex(g1, 1),bl_player, 1);
/******
Send a snapshot in the middle of the game..
******/
	_tprintf(_T("Sending update snapshot\n"));
	SendGameSnapShot(g1,NULL, SNAP_UPDATE);
	BucketIntOp(g1, "captures",bo_add, 1, bl_team, 0);

	BucketIntOp(g1, "score",bo_add, 1, bl_player, 1);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 2);	
	BucketIntOp(g1, "score",bo_add, 1, bl_player, 2);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 0);	

/**********
While this game is going on, create a new one! Keep updating
data for both games. For the second game we are going to keep
a "doom square" style kill list of who-killed-who
***********/
	g2 = NewGame(1);
	
	BucketStringOp(g2,"hostname",bo_set,"Another Server",bl_server, 0);
	BucketStringOp(g2,"mapname",bo_set,"Level 40",bl_server, 0);
	BucketStringOp(g2,"gametype",bo_set,"ctf",bl_server, 0);
	BucketFloatOp(g2,"gamever", bo_set,MY_VERSION, bl_server, 0);
	BucketIntOp(g2, "scorelimit", bo_set, 30,bl_server, 0);
	BucketIntOp(g2, "timelimit",bo_set, 30, bl_server, 0);

	NewPlayer(g2, 0, _T("James"));
	BucketIntOp(g2,"pid",bo_set,353352,bl_player, 0);
	BucketStringOp(g2,"auth",bo_set,"eea4c47e3d4a2f68dffab1b084e382b7",bl_player, 0);
	NewTeam(g2, 0, _T("Team Rocket"));
	BucketIntOp(g2,"team",bo_set,GetTeamIndex(g2,0),bl_player, 0);


	NewPlayer(g2, 1, _T("Jesse"));
	BucketIntOp(g2,"pid",bo_set,5564554,bl_player, 1);
	BucketStringOp(g2,"auth",bo_set,"d837be1371484bcbde9885dfc384b861",bl_player, 1);
	BucketIntOp(g2,"team",bo_set,GetTeamIndex(g2,0),bl_player, 1);

	BucketIntOp(g1, "score",bo_add, 1, bl_player, 0);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 2);	
	BucketIntOp(g1, "score",bo_add, 1, bl_player, 2);
	BucketIntOp(g1, "deaths",bo_add, 1, bl_player, 1);	
	BucketIntOp(g1, "captures",bo_add, 1, bl_team, 0);

	NewPlayer(g2, 2, _T("Ash"));
	BucketIntOp(g2,"pid",bo_set,6767565,bl_player, 2);
	BucketStringOp(g2,"auth",bo_set,"417f3e186ca5ae0fa32665db3ddf54c8",bl_player, 2);
	NewTeam(g2, 1, _T("Team Pika"));
	BucketIntOp(g2,"team",bo_set,GetTeamIndex(g2,1),bl_player, 2);
	
	NewPlayer(g2, 3, _T("Pika"));
	BucketIntOp(g2,"pid",bo_set,6767565,bl_player, 3);
	BucketStringOp(g2,"auth",bo_set,"417f3e186ca5ae0fa32665db3ddf54c8",bl_player, 3);
	BucketIntOp(g2,"team",bo_set,GetTeamIndex(g2,1),bl_player, 3);

	BucketIntOp(g2, "score",bo_add, 1, bl_player, 0);
	BucketIntOp(g2, "deaths",bo_add, 1, bl_player, 2);	
	BucketIntOp(g2,va("kills%d",GetPlayerIndex(g2,2)), bo_add, 1, bl_player, 0);
	BucketIntOp(g2, "score",bo_add, 1, bl_player, 2);
	BucketIntOp(g2, "deaths",bo_add, 1, bl_player, 1);	
	BucketIntOp(g2,va("kills%d",GetPlayerIndex(g2,1)), bo_add, 1, bl_player, 2);
	BucketIntOp(g2, "captures",bo_add, 1, bl_team, 1);

/***********
Close out the first game
***********/
	_tprintf(_T("Sending final snapshot\n"));
	SendGameSnapShot(g1,NULL, SNAP_FINAL);
	FreeGame(g1);

	/* more action in the second game */
	BucketIntOp(g2, "score",bo_add, 1, bl_player, 1);
	BucketIntOp(g2, "deaths",bo_add, 1, bl_player, 2);	
	BucketIntOp(g2,va("kills%d",GetPlayerIndex(g2,2)), bo_add, 1, bl_player, 1);
	BucketIntOp(g2, "score",bo_add, 1, bl_player, 3);
	BucketIntOp(g2, "deaths",bo_add, 1, bl_player, 0);	
	BucketIntOp(g2,va("kills%d",GetPlayerIndex(g2,0)), bo_add, 1, bl_player, 3);
	BucketIntOp(g2, "score",bo_add, 1, bl_player, 3);
	BucketIntOp(g2, "deaths",bo_add, 1, bl_player, 0);	
	BucketIntOp(g2,va("kills%d",GetPlayerIndex(g2,0)), bo_add, 1, bl_player, 3);
	BucketIntOp(g2, "captures",bo_add, 1, bl_team, 1);
/**********
Close out the second game
**********/
	_tprintf(_T("Sending final snapshot\n"));
	SendGameSnapShot(g2,NULL, SNAP_FINAL);
	FreeGame(g2);

/****************
Those are all the games for now, so go ahead and close
the connection
**************/

	_tprintf(_T("Closing stats connection\n"));
	CloseStatsConnection();


	GSI_UNUSED(argc);
	GSI_UNUSED(argv);

	return 0;
}



/* Helper for key name formatting */
static char	*va(char *format, ...)
{
	va_list		argptr;
	static char		string[1024];
	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);
	return string;	
}


/* Helper function to create a snapshot from the above structures when not using
bucket based snapshots */
static gsi_char *CreateSnapShotFromData(gsi_char *snapshot, serverinfo_t *psinfo, playerinfo_t players[])
{
	int i;
	size_t len;

	_tsnprintf(snapshot, SNAPSHOT_LEN, _T("\\hostname\\%s\\mapname\\%s\\gametype\\%s\\gamever\\%f"),
		psinfo->hostname, psinfo->mapname, psinfo->gametype, psinfo->version);
	for (i = 0 ; i < psinfo->numplayers ; i++)
	{
		len = _tcslen(snapshot);
		_tsnprintf(snapshot + len, SNAPSHOT_LEN - len,
			_T("\\player_%d\\%s\\points_%d\\%d\\deaths_%d\\%d\\pid_%d\\%d\\auth_%d\\%s"),
			i, players[i].name, i, players[i].points, i, players[i].deaths, i, players[i].profileid,i, players[i].auth);
	}
	return snapshot;
}
