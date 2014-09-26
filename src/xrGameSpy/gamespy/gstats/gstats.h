/******
gstats.h
GameSpy Stats/Tracking SDK 
  
Copyright 1999-2007 GameSpy Industries, Inc

******

Please see the GameSpy Stats and Tracking SDK documentation for more info

08-23-00 - DDW
Fixed a problem that prevented opening/closing/re-opening of the connection within
a single session.

*****/

#ifndef _GSTATS_H_
#define _GSTATS_H_


/********
INCLUDES
********/
#include "../common/gsCommon.h"
#include "gbucket.h"


#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
	// Warnings are generated because we store function ptrs into a void* array
#pragma warning(disable: 4152)  // function to data ptr
#pragma warning(disable: 4055)  // data to function ptr
#endif


/********
TYPEDEFS
********/


/* The abstracted "game" structure */
typedef struct statsgame_s *statsgame_t;

/* All of the operations you can do on a bucket */
typedef enum {bo_set, bo_add, bo_sub, bo_mult, bo_div, bo_concat, bo_avg} bucketop_t;
#define NUMOPS  7

/* The types of buckets (server info, team info, or player info) */
typedef enum {bl_server, bl_team, bl_player} bucketlevel_t;

/* Init states for async initialization */
typedef enum {init_none, init_failed, init_connecting, init_awaitchallenge, init_awaitsessionkey, init_complete} initstate_t;

/* Used by the bucket operation macros */
typedef void *(*BucketFunc)(bucketset_t set, char *name,void *value);
typedef int (*SetIntFunc)(statsgame_t game,char *name, BucketFunc func,  int value, int index);
typedef double (*SetFloatFunc)(statsgame_t game,char *name, BucketFunc func,  double value, int index);
typedef char *(*SetStringFunc)(statsgame_t game,char *name, BucketFunc func,  char *value, int index);
extern BucketFunc bucketfuncs[NUMOPS];
extern void * bopfuncs[][3];

/********
DEFINES
********/
/* Error codes */
#define GE_NOERROR		0
#define GE_NOSOCKET		1 /* Unable to create a socket */
#define GE_NODNS		2 /* Unable to resolve a DNS name */
#define GE_NOCONNECT	3 /* Unable to connect to stats server, or connection lost */
#define GE_BUSY			4 /* Not used */
#define GE_DATAERROR	5 /* Bad data from the stats server */
#define GE_CONNECTING   6 /* Connect did no immediately complete.  Call InitStatsThink() */
#define GE_TIMEDOUT     7 /* Connect attempt timed out */

/* Types of snapshots, update (any snapshot that is not final) or final */
#define SNAP_UPDATE	0
#define SNAP_FINAL	1

/* If you want to allow disk logging in case the stats server isn't available.
This has SERIOUS security repercussions, so please read the docs before turning this on */
#define ALLOW_DISK

#if defined(NOFILE)
	#undef ALLOW_DISK
#endif /* make sure it's never defined on platforms with no disk! */

/********
VARS
********/

/* You need to fill these in with your game-specific info */
extern char gcd_secret_key[256];
extern char gcd_gamename[256];

/* The hostname of the stats server.
If the app resolves the hostname, an
IP can be stored here before calling
InitStatsConnection */
extern char StatsServerHostname[64];


/********
PROTOTYPES
********/
#ifndef GSI_UNICODE
#define GenerateAuth		GenerateAuthA
#define SendGameSnapShot	SendGameSnapShotA
#define NewPlayer			NewPlayerA
#define NewTeam				NewTeamA
#else
#define GenerateAuth		GenerateAuthW
#define SendGameSnapShot	SendGameSnapShotW
#define NewPlayer			NewPlayerW
#define NewTeam				NewTeamW
#endif

/********
InitStatsConnection

DESCRIPTION
Opens a connection to the stats server. Should be done before calling
NewGame or any of the bucket/snapshot functions. May block for 1-2 secs
while the connection is established so you will want to do this before
gameplay starts or in another thread.

PARAMETERS
gameport: integer port associated with your server (may be the same as
	your developer spec query port). Used only to help players differentiate
	between servers on the same machine (no queries are done on it). If not
	appropriate for your game, pass in 0.

RETURNS
GE_NODNS: Unable to resolve stats server DNS
GE_NOSOCKET: Unable to create data socket
GE_NOCONNECT: Unable to connect to stats server
GE_DATAERROR: Unable to receive challenge from stats server, or bad challenge
GE_NOERROR: Connected to stats server and ready to send data

Note: You can still call ANY of the other Stats SDK functions, even if the
connection fails. If you have disk logging enabled, these calls will be logged
for future sending, otherwise they will be discarded.
*********/
int InitStatsConnection(int gameport);
int InitStatsAsync(int gameport, gsi_time theInitTimeout);
int InitStatsThink();


/********
StatsThink

DESCRIPTION 
Eats up any incoming keep-alive messages that are sent by the stats server.
Returns any errors occur because of a socket problem or if the SDK was
not completely initialized.

RETURNS
1 if no errors occured during read, 0 on all other errors
********/
int StatsThink();

/********
IsStatsConnected

DESCRIPTION
Returns whether or not you are currently connected to the stats server. Even
if your initial connection was successful, you may lose connection later and
want to try to reconnnect

RETURNS
1 if connected, 0 otherwise
*********/
int IsStatsConnected();

/********
CloseStatsConnection

DESCRIPTION
Closes the connection to the stats server. You should do this when done
with the connection.
*********/
void CloseStatsConnection(void);

/********
GetChallenge

DESCRIPTION
Returns a string that should be sent to clients for authentication
(using GenerateAuth). You do not have to free the string when done.
This string will be constant for the entire length of the game and is
generated during the call to NewGame.

PARAMETERS
game: Game to return the challenge string for. If game is NULL, the last
	game created with NewGame will be used.

RETURNS
A string to send to clients so they can authorize. If you game is NULL and
you haven't created a game with NewGame, it returns "NULLGAME".
*********/
char *GetChallenge(statsgame_t game);

/********
GenerateAuth

DESCRIPTION
Should be used on the CLIENT SIDE to generate an authentication reply
(auth_N) for a given challenge and password (CD Key or Profile password)

PARAMETERS
challenge: The challenge string sent by the server. On the server this
	should be generated with GetChallenge
password: The CD Key (un-hashed) or profile password
response: The output authentication string

RETURNS
A pointer to response
*********/
char *GenerateAuth(const char *challenge, const gsi_char *password,/*[out]*/char response[33]);

/********
NewGame

DESCRIPTION
Creates a new game for logging and registers it with the stats server. 
Creates all the game structures, including buckets if needed.

PARAMETERS
usebuckets: Set to 1 for bucket based logging, 0 if you are going to create
	the snapshots yourself. See the SDK for more info.

RETURNS
A pointer to the new game. If you are not connected, and disk logging is
disabled, this will be NULL. You can still pass NULL to any function without
causing any errors.
Note: The last game created by NewGame is stored internally. 
If you only create / use one game at a time, you can simply discard
the return value and pass NULL for game into all of the bucket and snapshot functions.
*********/
statsgame_t NewGame(int usebuckets);

/********
FreeGame

DESCRIPTION
Frees a game and its associated structures (including buckets). You should
send a final snapshot for the game (using SendGameSnapShot with SNAP_FINAL)
before freeing the game.

PARAMETERS
game: The game you want to free. If set to NULL, it will free the last
	game created with NewGame.
*********/
void FreeGame(statsgame_t game);

/********
SendGameSnapShot

DESCRIPTION
Sends a snapshot of information about the current game. If bucket based
logging is enabled the snapshot will be generated from the buckets, otherwise
you should provide it in "snapshot". 

PARAMETERS
game: The game to send a snapshot for. If set to NULL, the last game
	created with NewGame will be used.
snapshot: The snapshot to send. If you are using buckets, this will not be
	used, so you can pass in NULL
final: If this is SNAP_UPDATE, the game is marked as in progress, if it
	is SNAP_FINAL, the game is marked as complete.

RETURNS
GE_DATAERROR: If game is NULL and the last game created by NewGame failed
	(because the connection was lost and disk logging is disabled)
GE_NOCONNECT: If the connection is lost and disk logging is disabled
GE_NOERROR: The update was sent, or disk logging is enabled and the game was logged
*********/
int SendGameSnapShot(statsgame_t game, const gsi_char *snapshot, int final);

/******************************
BUCKET FUNCTION PROTOTYPES
These functions are only used for bucket-based logging
*******************************/

/********
Bucket_____Op

DESCRIPTION
Performs an operation on a bucket for a game. If the bucket doesn't exist already,
the call will set the bucket to whatever "value" is. 
You can always create each bucket explicitly by using bo_set with whatever initial
value you want the bucket to have.
Valid operations include set, add, subtract, multiply, divide, concat, and average.
Each bucket type (int, float, or string) has its own operation function, always call
the same one for each bucket (i.e. don't create a bucket with BucketIntOp then try to
add a float with BucketFloatOp).

PARAMETERS
game: The game to send containing the bucket you want to operate on. 
	If set to NULL, the last game created with NewGame will be used.
name: The name of the bucket to update. Note that for player or team buckets, this name
	does NOT include the "_" or "_t" (e.g. "score" for player score, not "score_N"). The underscore
	and number will be added automatically.
operation: One of the bucketop_t enums defined above
value: Argument for the operation (bucket OP= value, e.g. bucket += value, bucket *= value)
bucketlevel: One of the bucketlevel_t enums defined above. Determines whether you are
	referring to a server, player, or team bucket. Note that you can have seperate buckets of
	each type with the same name (e.g. "score" player bucket for each player and "score" team
	bucket for each team)
index: For player or team buckets, the game index of the player or team (as passed to NewPlayer or
	NewTeam). This will be translated to the actual index internally. 
	Not used for server buckets (bl_server).
*********/
#define BucketIntOp(game, name, operation, value, bucketlevel, index) (((SetIntFunc)bopfuncs[bucketlevel][bt_int])(game,name,bucketfuncs[operation],value,index) )
#define BucketFloatOp(game, name, operation, value, bucketlevel, index) (((SetFloatFunc)bopfuncs[bucketlevel][bt_float])(game,name,bucketfuncs[operation],value,index) )
#define BucketStringOp(game, name, operation, value, bucketlevel, index) (((SetStringFunc)bopfuncs[bucketlevel][bt_string])(game,name,bucketfuncs[operation],value,index) )

/********
NewPlayer

DESCRIPTION
Adds a "player" to the game and assigns them an internal player number. Sets
their connect time to the number of seconds since NewGame was called.

PARAMETERS
game: The game to add the player to. If set to NULL, the last game created
	with NewGame will be used. 
pnum: Your internal reference for this player, use this value in any calls
	to the Bucket___Op functions.
name: The name for this player. If you don't have one yet, set it to empty ("")
	then call: BucketStringOp(game,"player",bo_set,realplayername, bl_player, pnum)
	when you get a realplayername.
**********/
void NewPlayer(statsgame_t game,int pnum, gsi_char *name);

/********
RemovePlayer

DESCRIPTION
Removes a "player" from the game and sets their disconnect time to the
number of seconds since NewGame was called.

PARAMETERS
game: The game to remove the player from. If set to NULL, the last game created
	with NewGame will be used. 
pnum: Your internal reference for this player, use this value in any calls
	to the Bucket___Op functions.
**********/
void RemovePlayer(statsgame_t game,int pnum);

/*********
NewTeam
RemoveTeam

DESCRIPTION
See the player functions above. These function the same, except for teams
**********/
void NewTeam(statsgame_t game,int tnum, gsi_char *name);
void RemoveTeam(statsgame_t game,int tnum);

/*********
GetPlayerIndex
GetTeamIndex

DESCRIPTION
Gets the gstats reference number for that player or team. For
example, if you start the game and players 0, 1, and 2 join, then player 1
leaves, and another player 1 joins, the new player 1 will be referenced 
by gstats as 3. If player 3 joins, it will be referenced as player 4, and so on.
Normally this doesn't matter to you, but if you want to do a key name or key value
that references a player or team number (for example, setting a player's team number),
you need to use the translated values. 

PARAMETERS
game: The game to retrieve the translated value for. If set to NULL,the last game created
	with NewGame will be used.
pnum/tnum: Your internal player or team number (as sent to NewTeam/NewPlayer)
**********/
int GetPlayerIndex(statsgame_t game, int pnum);
int GetTeamIndex(statsgame_t game, int tnum);


#ifdef __cplusplus
}
#endif

#endif
