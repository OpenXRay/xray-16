/******
gstats.c
GameSpy Stats/Tracking SDK 
GameSpy Persistent Storage SDK 

Copyright 1999-2007 GameSpy Industries, Inc

******

Please see the GameSpy Stats and Tracking SDK documentation for more info

******/

#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4267) //lines: 207
#pragma warning(disable: 4244) //lines: 1536, 1537, 1538
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS


/********
INCLUDES
********/
#include "gstats.h"
#include "gpersist.h"
#include "../common/gsAvailable.h"
#include "../darray.h"
#include "../md5.h"


#ifdef __cplusplus
extern "C" {
#endif


/********
TYPEDEFS
********/
struct statsgame_s 
{
	int connid;
	int sesskey;
	int usebuckets;
	bucketset_t buckets;
	char challenge[9];
	DArray playernums; //for player number translation
	DArray teamnums; //for team number translation
	int totalplayers, totalteams;
	gsi_time sttime;

};

typedef enum {rt_authcb, rt_datacb, rt_savecb, rt_profilecb}  reqtype_t;
typedef struct 
{
	reqtype_t reqtype;
	int localid;
	int profileid;
	persisttype_t pdtype;
	int pdindex;
	void *instance;
	void *callback;

} serverreq_t;


/********
PROTOTYPES
********/
static int ServerOpInt(statsgame_t game,char *name, BucketFunc func,  int value, int index);
static double ServerOpFloat(statsgame_t game,char *name, BucketFunc func,  double value, int index);
static char *ServerOpString(statsgame_t game,char *name, BucketFunc func,  char *value, int index);

static int TeamOpInt(statsgame_t game,char *name, BucketFunc func,  int value, int index);
static double TeamOpFloat(statsgame_t game,char *name, BucketFunc func,  double value, int index);
static char *TeamOpString(statsgame_t game,char *name, BucketFunc func,  char *value, int index);

static int PlayerOpInt(statsgame_t game,char *name, BucketFunc func,  int value, int index);
static double PlayerOpFloat(statsgame_t game,char *name, BucketFunc func,  double value, int index);
static char *PlayerOpString(statsgame_t game,char *name, BucketFunc func,  char *value, int index);

static char *CreateBucketSnapShot(bucketset_t buckets);

#ifdef ALLOW_DISK
static void CheckDiskFile();
static void DiskWrite(char *line, int len);
#endif
static void InternalInit();
static int SendChallengeResponse(const char *indata, int gameport);
static int RecvSessionKey();
static int DoSend(char *data, int len);
static void xcode_buf(char *buf, int len);
static int g_crc32(char *s, int len);
static void create_challenge(int challenge, char chstr[9]);
static char *value_for_key(const char *s, const char *key);
static char *value_for_key_safe(const char *s, const char *key);
static int get_sockaddrin(const char *host, int port, struct sockaddr_in *saddr, struct hostent **savehent);
/**************
PERSISTENT STORAGE PROTOTYPES
**************/
static void AddRequestCallback(reqtype_t reqtype, int localid, int profileid, persisttype_t pdtype, int pdindex, void *callback, void *instance);
static void SendPlayerAuthRequest(char *data, int len, int localid, PersAuthCallbackFn callback, void *instance);
static void SendPlayerAuthRequest(char *data, int len, int localid, PersAuthCallbackFn callback, void *instance);
static int SocketReadable(SOCKET s);
static char *FindFinal(char *buff, int len);
static int FindRequest(reqtype_t reqtype, int localid, int profileid);
static void ProcessPlayerAuth(const char *buf, int len);
static void ProcessGetPid(const char *buf, int len);
static void ProcessGetData(const char *buf, int len);
static void ProcessSetData(const char *buf, int len);
static void ProcessStatement(char *buff, int len);
static int ProcessInBuffer(char *buff, int len);
static void CallReqCallback(int reqindex, int success, time_t modified, char *data, int length);
static void ClosePendingCallbacks();
static void SetPersistDataHelper(int localid, int profileid, persisttype_t type, int index, const char *data, int len, PersDataSaveCallbackFn callback, void *instance, int kvset);
void GetPersistDataValuesA(int localid, int profileid, persisttype_t type, int index, char *keys, PersDataCallbackFn callback, void *instance);
void GetPersistDataValuesModifiedA(int localid, int profileid, persisttype_t type, int index, time_t modifiedsince, char *keys, PersDataCallbackFn callback, void *instance);

/********
DEFINES
********/
//#define SSHOST "207.199.80.230"
#define SSHOST "gamestats." GSI_DOMAIN_NAME
#define SSPORT 29920

#define FIXGAME(g,r) if (g == NULL) g = g_statsgame; if (g == NULL) return r;
#define DoFunc(f,g, n, v, t, r) \
	if (g == NULL) g = g_statsgame; \
	if (!g) r = v; \
	else { \
	r = f(g->buckets, n, v); \
	if (!r) \
	r = BucketNew(g->buckets, n, t, v); }
#define DOXCODE(b, l, e) enc = e; xcode_buf(b,l);


/********
VARS
********/
char gcd_gamename[256] = "";
char gcd_secret_key[256] = "";
static statsgame_t g_statsgame = NULL;
static int connid = 0;
static int sesskey = 0;
static SOCKET sock = INVALID_SOCKET;
/*	#define enc1 "GameSpy 3D"
	#define enc2 "Industries"
	#define enc3 "ProjectAphex"
	#define STATSFILE "gstats.dat" */
/* A couple vars to help avoid the string table */
static char enc1[16] = {'\0','a','m','e','S','p','y','3','D','\0'};
static char enc3[16] = {'\0','r','o','j','e','c','t','A','p','h','e','x','\0'};

#ifdef ALLOW_DISK
static char statsfile[16] = {'\0','s','t','a','t','s','.','d','a','t','\0'};
static char enc2[16]= {'\0','n','d','u','s','t','r','i','e','s','\0'};
#endif

static char finalstr[10] = {'\0','f','i','n','a','l','\\','\0'};
static char *enc = enc1;
static int internal_init = 0;
static char *rcvbuffer = NULL;
static int rcvmax = 0;
static int rcvlen = 0;
// Changed By Saad Nader, 09-16-2004
// Due to confliction with MacOS X
///////////////////////////////////////////
static int stats_initstate = init_none;
static int gameport = 0;

static gsi_time initstart = 0;
static gsi_time inittimeout = 20000;  // 20 seconds

char StatsServerHostname[64] = SSHOST;

static DArray serverreqs = NULL; //for pre-authentication requests


BucketFunc bucketfuncs[NUMOPS] = 
{BucketSet, BucketAdd, BucketSub, BucketMult, BucketDiv, BucketConcat, BucketAvg};

void * bopfuncs[][3] = 
{
		{ServerOpInt, ServerOpFloat, ServerOpString},
		{TeamOpInt, TeamOpFloat, TeamOpString},
		{PlayerOpInt, PlayerOpFloat, PlayerOpString},
};

/****************************************************************************/
/* PUBLIC FUNCTIONS */
/****************************************************************************/
#define RAWSIZE 128
char *GenerateAuthA(const char *challenge, const char *password, char response[33])
{
	char rawout[RAWSIZE];
	
	/* check to make sure we weren't passed a huge pass/challenge */
	if (strlen(password) + strlen(challenge) + 20>= RAWSIZE)
	{
		strcpy(response,"CD Key or challenge too long");
		return response;
	}

	/* response = MD5(pass + challenge) */
	sprintf(rawout, "%s%s",password, challenge );

	/* do the response md5 */
	MD5Digest((unsigned char *)rawout, strlen(rawout), response);
	return response;
}
#ifdef GSI_UNICODE
char *GenerateAuthW(const char* challenge, const unsigned short *password, char response[33])
{
	char* password_A	= UCS2ToUTF8StringAlloc(password);
	GenerateAuthA(challenge, password_A, response);
	gsifree(password_A);
	return response;
}
#endif

/****************************************************************************/
int InitStatsAsync(int theGamePort, gsi_time theInitTimeout)
{
	struct sockaddr_in saddr;
	char tempHostname[128];
	int  ret;
		
	gameport = theGamePort;

	if (theInitTimeout != 0)
		inittimeout = theInitTimeout;

	/* check if the backend is available */
	if(__GSIACResult != GSIACAvailable)
		return GE_NOSOCKET;

	/* Init our hidden strings if needed */
	if (!internal_init)
		InternalInit();

	SocketStartUp();
	sesskey = (int)current_time();

	/* Get connected */
	if (sock != INVALID_SOCKET)
		CloseStatsConnection();

	rcvlen = 0; //make sure ther receive buffer is cleared

	if (inet_addr(StatsServerHostname) == INADDR_NONE)
	{
		strcpy(tempHostname, gcd_gamename);
		strcat(tempHostname,".");
		strcat(tempHostname,StatsServerHostname);
	} else
		strcpy(tempHostname, StatsServerHostname); //it's already been resolved
	
	if (get_sockaddrin(tempHostname,SSPORT,&saddr,NULL) == 0)
		return GE_NODNS;

#ifdef INSOCK
	sock = socket ( AF_INET, SOCK_STREAM, 0 );
#else
	sock = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP );
#endif 
	if (sock == INVALID_SOCKET)
		return GE_NOSOCKET;

	SetSockBlocking(sock, 0);

	ret = connect(sock, (struct sockaddr*)&saddr, sizeof(saddr));
	if (gsiSocketIsError(ret))
	{
		int anError = GOAGetLastError(sock);
		if ((anError != WSAEWOULDBLOCK) && (anError != WSAETIMEDOUT) && (anError != WSAEINPROGRESS))
		{
			stats_initstate = init_failed;
			closesocket(sock);
			return GE_NOCONNECT;
		}
	}

	// allocate the recv buffer
	rcvbuffer = gsimalloc(64);
	if (rcvbuffer == NULL)
		return GE_NOCONNECT; // add a new error code for out of mem?

	rcvmax = 64;
	rcvlen = 0;

	initstart = current_time();
	stats_initstate = init_connecting;
	return GE_CONNECTING;
}

/****************************************************************************/
int InitStatsThink()
{
	switch(stats_initstate)
	{
	case init_failed:          return GE_NOCONNECT;
	case init_connecting:      
		{
			// Check if socket is writeable yet
			int aWriteFlag = 0;
			int aExceptFlag = 0;
			int aResult = GSISocketSelect(sock, NULL, &aWriteFlag, &aExceptFlag);
			if ((gsiSocketIsError(aResult)) ||                   // socket error
				(aResult == 1 && aExceptFlag == 1))  // exception
			{
				stats_initstate = init_failed;
				CloseStatsConnection();
				return GE_NOCONNECT;
			}
			else if (aResult == 0) // no progress yet
			{
				// Should we continue to wait?
				if (current_time() - initstart > inittimeout)
				{
					stats_initstate = init_failed;
					CloseStatsConnection();
					return GE_TIMEDOUT;
				}
				else
					return GE_CONNECTING;
			}

			// Otherwise connected
			assert(aResult == 1 && aWriteFlag == 1);
			stats_initstate = init_awaitchallenge;
			// fall through
		}
	case init_awaitchallenge:
		{
			int ret = 0;
			
			// Try to receive data
			if (!CanReceiveOnSocket(sock))
			{
				// should we continue to wait?
				if (current_time() - initstart > inittimeout)
				{
					stats_initstate = init_failed;
					CloseStatsConnection();
					return GE_TIMEDOUT;
				}
				return GE_CONNECTING;
			}

			// Receive the 38 byte challenge
			ret = recv(sock, rcvbuffer+rcvlen, rcvmax-rcvlen, 0);
			if (gsiSocketIsError(ret))
			{
				stats_initstate = init_failed;
				CloseStatsConnection();
				return GE_NOCONNECT;
			}
			rcvlen += ret;
			rcvmax -= ret;

			// need at least 38 bytes
			if (rcvlen < 38)
				return GE_CONNECTING;

			// Process challenge
			rcvbuffer[rcvlen] = '\0';
			stats_initstate = init_awaitsessionkey;

			/* Decode it */
			DOXCODE(rcvbuffer, rcvlen, enc1);
			/* Send a response */
			ret = SendChallengeResponse(rcvbuffer, gameport);
			if (ret != GE_NOERROR)
			{
				stats_initstate = init_failed;
				CloseStatsConnection();
				return ret;
			}

			stats_initstate = init_awaitsessionkey;

			// clear receive buffer for next stage
			rcvmax += rcvlen;   // reclaim the used bytes as free space
			rcvlen  = 0;
			memset(rcvbuffer, 0, (unsigned int)rcvmax);

			// fall through
		}
	case init_awaitsessionkey:
		{
			int ret = 0;
			
			// Try to receive data
			if (!CanReceiveOnSocket(sock))
			{
				// should we continue to wait?
				if (current_time() - initstart > inittimeout)
				{
					stats_initstate = init_failed;
					CloseStatsConnection();
					return GE_TIMEDOUT;
				}
				return GE_CONNECTING;
			}

			ret = RecvSessionKey();
			if (ret != GE_NOERROR)
			{
				stats_initstate = init_failed;
				CloseStatsConnection();
				return ret;
			}

			// Init complete
			// Clear the receive buffer
			rcvmax += rcvlen;
			rcvlen  = 0;
			memset(rcvbuffer, 0, (unsigned int)rcvmax);

			#ifdef ALLOW_DISK
			/* Check for old data */
			CheckDiskFile();
			#endif

			stats_initstate = init_complete;

			// fall through
		}
	case init_complete:
		return GE_NOERROR;

	default:
		return GE_NOCONNECT;
	};
}


/****************************************************************************/
// Blocking version of InitStatsAsync, for backwards compatability
int InitStatsConnection(int gameport)
{
	int aResult = InitStatsAsync(gameport, 0);
	while (aResult == GE_CONNECTING)
	{
		aResult = InitStatsThink();
		msleep(5);
	}
	return aResult;
}

/****************************************************************************/
void CloseStatsConnection()
{
	if (sock != INVALID_SOCKET)
	{
		shutdown(sock,2);
		closesocket(sock);
	}
	sock = INVALID_SOCKET;
	//call any pending callbacks with the data as lost
	ClosePendingCallbacks();
	if (rcvbuffer != NULL)
	{
		gsifree(rcvbuffer);
		rcvbuffer = NULL;
		rcvmax = 0;
		rcvlen = 0;
	}

}

/****************************************************************************/
int IsStatsConnected()
{
	return (sock != INVALID_SOCKET);
}

/****************************************************************************/
#define CHALLENGEXOR 0x38F371E6
char *GetChallenge(statsgame_t game)
{
	static char challenge[9];
	if (game == NULL)
		game = g_statsgame;
	if (game == NULL)
	{
		create_challenge(connid ^ CHALLENGEXOR,challenge);
		return challenge;
	}
	return game->challenge;
}

/****************************************************************************/
statsgame_t NewGame(int usebuckets)
{
	statsgame_t game = (statsgame_t)gsimalloc(sizeof (struct statsgame_s));
	char data[256];
	int len;

	if (!internal_init)
		InternalInit();
	game->connid = connid;
	game->sesskey = sesskey++;
	game->buckets = NULL;
	game->playernums = NULL;
	game->teamnums = NULL;
	game->usebuckets = usebuckets;
	/* If connected, try to send */
	if (sock != INVALID_SOCKET)
	{ 
		char respformat[] = "\xC\x1C\xA\x1D\x2\x2\x19\x24\x2C\x34\x6\x17\x3E\x1C\x6\xE\x39\x46\x10\x1D\x3\xD\x16\xB\x3B\x17\x16\x36\x40\x7";
		//"\newgame\\connid\%d\sesskey\%d"
		DOXCODE(respformat, sizeof(respformat)-1, enc3);
		len = sprintf(data,respformat,game->connid, game->sesskey);
		len = DoSend(data, len);
		if (len <= 0)
		{
			CloseStatsConnection();
		}
		create_challenge(game->connid ^ CHALLENGEXOR,game->challenge);
	}
	/* If send failed then write to disk */
	if (sock == INVALID_SOCKET) 
	{
#ifdef ALLOW_DISK
		char respformat[] = "\xC\x1C\xA\x1D\x2\x2\x19\x24\x2C\x34\x16\x1D\x23\x1\x4\xF\x1C\x3F\x51\x25\x2C\xB\xD\x19\x3C\x1E\xA\x4\x2\x6\x28\x64\x14";
		// "\newgame\\sesskey\%d\challenge\%d";

		DOXCODE(respformat, sizeof(respformat)-1, enc3);
		len = sprintf(data,respformat,game->sesskey, game->sesskey ^ CHALLENGEXOR);
		DiskWrite(data, len);
		game->connid = 0;
		create_challenge(game->sesskey ^ CHALLENGEXOR,game->challenge);

#else
		gsifree(game);
		game = NULL;
#endif
	
	}

	if (game && game->usebuckets)
	{
		game->buckets = NewBucketSet();
		game->playernums = ArrayNew(sizeof(int),32,NULL);
		game->teamnums = ArrayNew(sizeof(int),2,NULL);
		game->totalplayers = game->totalteams = 0;
	}
	if (game)
		game->sttime = current_time();
	g_statsgame = game;
	return game;
	
}


/****************************************************************************/
void FreeGame(statsgame_t game)
{
	if (!game || game == g_statsgame)
	{
		game = g_statsgame;
		g_statsgame = NULL;
	}
	if (!game)
		return;
	if (game->usebuckets)
	{
		if (game->buckets != NULL)
			FreeBucketSet(game->buckets);
		if (game->playernums != NULL)
			ArrayFree(game->playernums);
		if (game->teamnums != NULL)
			ArrayFree(game->teamnums);
	}
	gsifree(game);
}

/****************************************************************************/
int SendGameSnapShotA(statsgame_t game, const char *snapshot, int final)
{
	int snaplen;
	int len;
	int ret = GE_NOERROR;
	char *snapcopy;
	char *data;
	FIXGAME(game, GE_DATAERROR);

	/* If using buckets, get the data out of the buckets */
	if (game->usebuckets)
		snapcopy = CreateBucketSnapShot(game->buckets);
	else
		snapcopy = goastrdup(snapshot);
	snaplen = (int)strlen(snapcopy);

	data = (char *)gsimalloc((unsigned int)snaplen + 256);

	/* Escape the data */
	while (snaplen--)
		if (snapcopy[snaplen] == '\\')
			snapcopy[snaplen] = '\x1';

	/* If connected, try to send it */
	if (sock != INVALID_SOCKET)
	{
		// Updated response format to contain connid
		//char respformat[] = "\xC\x7\x1F\xE\x2\x2\x19\x24\x2C\x34\x16\x1D\x23\x1\x4\xF\x1C\x3F\x51\x25\x2C\xC\xA\x16\x35\x2E\x4A\xE\x39\x4\x15\x2C\x15\xC\x4\xC\x31\x2E\x4A\x19";
		char respformat[] = "\xC\x7\x1F\xE\x2\x2\x19\x24\x2C\x34\x16\x1D\x23\x1\x4\xF\x1C\x3F\x51\x25\x2C\xB\xA\x16\x3E\x1B\xB\x36\x40\x7\x28\x25\x1F\x6\x00\x24\x75\x16\x33\xD\x4\xE\x11\x25\x11\x1C\x4\x24\x75\x1";
		//	"\updgame\\sesskey\%d\done\%d\gamedata\%s"
		// The above string is now: 
		// "\updgame\\sesskey\%d\connid\%d\done\%d\gamedata\%s"
		DOXCODE(respformat, sizeof(respformat)-1, enc3);
		len = sprintf(data, respformat, game->sesskey, game->connid, final, snapcopy);
		snaplen = DoSend(data, len);
		/* If the send failed, close the socket */
		if (snaplen <= 0)
		{
			CloseStatsConnection();
		}
	}
	/* If not connected, or send failed, return error or log to disk */
	if (sock == INVALID_SOCKET)
	{
#ifdef ALLOW_DISK
		char respformat[] = "\xC\x7\x1F\xE\x2\x2\x19\x24\x2C\x34\x16\x1D\x23\x1\x4\xF\x1C\x3F\x51\x25\x2C\xB\xA\x16\x3E\x1B\xB\x36\x40\x7\x28\x25\x1F\x6\x0\x24\x75\x16\x33\xD\x4\xE\x11\x25\x11\x1C\x4\x24\x75\x1\x33\xE\x9\x3F\x45";
		//"\updgame\\sesskey\%d\connid\%d\done\%d\gamedata\%s\dl\1"
		DOXCODE(respformat, sizeof(respformat)-1, enc3);
		len = sprintf(data, respformat, game->sesskey, game->connid, final, snapcopy);
		DiskWrite(data, len);
#else
		ret = GE_NOCONNECT;
#endif
	}
	gsifree(snapcopy);
	gsifree(data);
	return ret;
}
#ifdef GSI_UNICODE
int SendGameSnapShotW(statsgame_t game, const unsigned short*snapshot, int final)
{
	char* snapshot_A = UCS2ToUTF8StringAlloc(snapshot);
	int result = SendGameSnapShotA(game, snapshot_A, final);
	gsifree(snapshot_A);
	return result;
}
#endif

/****************************************************************************/
void NewPlayerA(statsgame_t game, int pnum, char *name)
{
	int i = -1;
	FIXGAME(game, ;)
	while (pnum >= ArrayLength(game->playernums))
		ArrayAppend(game->playernums, &i);
	i = game->totalplayers++;
	/* update the pnum array */
	ArrayReplaceAt(game->playernums,&i, pnum);
	BucketIntOp(game, "ctime",bo_set,(int)(current_time() - game->sttime) / 1000,bl_player,pnum);
	BucketStringOp(game,"player",bo_set,name, bl_player,pnum);

}
#ifdef GSI_UNICODE
void NewPlayerW(statsgame_t game, int pnum, unsigned short *name)
{
	char* name_A = UCS2ToUTF8StringAlloc(name);
	NewPlayerA(game, pnum, name_A);
	gsifree(name_A);
}
#endif

/****************************************************************************/
void RemovePlayer(statsgame_t game,int pnum)
{
	FIXGAME(game, ;);
	BucketIntOp(game,"dtime",bo_set,(int)(current_time() - game->sttime) / 1000, bl_player, pnum);
}

/****************************************************************************/
void NewTeamA(statsgame_t game,int tnum, char *name)
{
	int i = -1;
	FIXGAME(game, ;)
	while (tnum >= ArrayLength(game->teamnums))
		ArrayAppend(game->teamnums, &i);
	i = game->totalteams++;
	/* update the tnum array */
	ArrayReplaceAt(game->teamnums,&i, tnum);
	BucketIntOp(game, "ctime",bo_set,(int)(current_time() - game->sttime) / 1000,bl_team,tnum);
	BucketStringOp(game,"team",bo_set,name, bl_team,tnum);
}
#ifdef GSI_UNICODE
void NewTeamW(statsgame_t game,int tnum, unsigned short *name)
{
	char* name_A = UCS2ToUTF8StringAlloc(name);
	NewTeamA(game, tnum, name_A);
	gsifree(name_A);
}
#endif

/****************************************************************************/
void RemoveTeam(statsgame_t game, int tnum)
{
	FIXGAME(game, ;);
	BucketIntOp(game,"dtime",bo_set,(int)(current_time() - game->sttime) / 1000, bl_team, tnum);
}

/****************************************************************************
 * PERSISTENT STORAGE FUNCTIONS
 ****************************************************************************/

/****************************************************************************/
void PreAuthenticatePlayerPartner(int localid, const char * authtoken, const char *challengeresponse, PersAuthCallbackFn callback, void *instance)
{
	char respformat[] = "\xC\x13\x1A\x1E\xD\x13\x28\x1D\x11\x1D\x11\x10\x24\x1D\x04\x0F\x0B\x3F\x51\x32\x2C\x1A\x00\x0B\x20\x2E\x4A\x19\x39\x0F\x1D\x25\x2C\x4D\x01";
	//\authp\\authtoken\%s\resp\%s\lid\%d";
	int len;
	char data[256];

	DOXCODE(respformat, sizeof(respformat)-1, enc3);
	len = sprintf(data, respformat, authtoken, challengeresponse, localid);

	SendPlayerAuthRequest(data, len, localid, callback, instance);
}

								  
/****************************************************************************/
void PreAuthenticatePlayerPM(int localid, int profileid,  const char *challengeresponse, PersAuthCallbackFn callback, void *instance)
{
	char respformat[] = "\xC\x13\x1A\x1E\xD\x13\x28\x1D\x0\x1\x1\x24\x75\x16\x33\x18\x0\x10\x4\x1D\x55\x1B\x39\x14\x39\x16\x33\x4F\x1";
	//\authp\\pid\%d\resp\%s\lid\%d
	int len;
	char data[256];
	
	DOXCODE(respformat, sizeof(respformat)-1, enc3);
	len = sprintf(data, respformat, profileid, challengeresponse,localid);

	SendPlayerAuthRequest(data, len, localid, callback, instance);

}

/****************************************************************************/
void PreAuthenticatePlayerCDA(int localid, const char *nick, const char *keyhash, const char *challengeresponse, PersAuthCallbackFn callback, void *instance)
{
	char respformat[] = "\xC\x13\x1A\x1E\xD\x13\x28\x1D\x1E\x1\x6\x13\xC\x57\x1C\x36\xE\x6\xD\x29\x11\x1B\xD\x24\x75\x1\x33\x18\x0\x10\x4\x1D\x55\x1B\x39\x14\x39\x16\x33\x4F\x1";
	//\authp\\nick\%s\keyhash\%s\resp\%s\lid\%d
	int len;
	char data[256];
	
	DOXCODE(respformat, sizeof(respformat)-1, enc3);
	len = sprintf(data, respformat, nick, keyhash, challengeresponse,localid);

	SendPlayerAuthRequest(data, len, localid, callback, instance);

}
#ifdef GSI_UNICODE
void PreAuthenticatePlayerCDW(int localid, const unsigned short *nick, const char *keyhash, const char *challengeresponse, PersAuthCallbackFn callback, void *instance)
{
	char* nick_A = UCS2ToUTF8StringAlloc(nick);
	PreAuthenticatePlayerCDA(localid, nick_A, keyhash, challengeresponse, callback, instance);
	gsifree(nick_A);
}
#endif

/****************************************************************************/
void GetProfileIDFromCDA(int localid, const char *nick, const char *keyhash, ProfileCallbackFn callback, void *instance)
{
	char respformat[] = "\xC\x15\xA\x1E\x15\xA\x10\x1D\x2C\x6\xC\x1B\x3B\x2E\x4A\x19\x39\x8\x11\x38\x18\x9\x16\x10\xC\x57\x1C\x36\x9\xA\x10\x1D\x55\xC";
	//\getpid\\nick\%s\keyhash\%s\lid\%d
	int len;
	char data[512];
	DOXCODE(respformat, sizeof(respformat)-1, enc3);
	len = sprintf(data, respformat, nick, keyhash,localid);

	if (sock != INVALID_SOCKET)
		len	= DoSend(data, len);

	/* If the send failed, close the socket */
	if (len <= 0)
	{
		CloseStatsConnection();
		if (callback)
			callback(0,-1,0,instance);
	} else
	{ /* set up the callback */
		AddRequestCallback(rt_profilecb, localid, 0,(persisttype_t)0,0,callback, instance);
	}
}
#ifdef GSI_UNICODE
void GetProfileIDFromCDW(int localid, const unsigned short *nick, const char *keyhash, ProfileCallbackFn callback, void *instance)
{
	char* nick_A = UCS2ToUTF8StringAlloc(nick);
	GetProfileIDFromCDA(localid, nick_A, keyhash, callback, instance);
	gsifree(nick_A);
}
#endif

/****************************************************************************/
void GetPersistData(int localid, int profileid, persisttype_t type, int index, PersDataCallbackFn callback, void *instance)
{
	GetPersistDataValuesModifiedA(localid,profileid, type,index,0,"",callback, instance);
}

void GetPersistDataModified(int localid, int profileid, persisttype_t type, int index, time_t modifiedsince, PersDataCallbackFn callback, void *instance)
{
	GetPersistDataValuesModifiedA(localid,profileid, type,index, modifiedsince, "",callback, instance);
}

/****************************************************************************/
void SetPersistData(int localid, int profileid, persisttype_t type, int index, const char *data, int len, PersDataSaveCallbackFn callback, void *instance)
{
	SetPersistDataHelper(localid, profileid, type, index,  data, len, callback, instance, 0);
}

/****************************************************************************/
void GetPersistDataValuesModifiedA(int localid, int profileid, persisttype_t type, int index, time_t modifiedsince, char *keys, PersDataCallbackFn callback, void *instance)
{
	char respformat[] = "\xC\x15\xA\x1E\x15\x7\x28\x1D\x0\x1\x1\x24\x75\x16\x33\x1A\x11\x1A\x4\x24\x2C\x4D\x1\x24\x34\x1B\x1\xE\x0\x1B\x28\x64\x14\x34\xE\x1D\x29\x1\x33\x4F\x16\x3F\x18\x28\x14\x34\x40\x1C";
	char modformat[] = {'\\','m','o','d','\\','%','d','\0'}; //\\mod\\%d
	//\getpd\\pid\%d\ptype\%d\dindex\%d\keys\%s\lid\%d
	int len;
	char data[512];
	char tempkeys[256];
	char *p;

	DOXCODE(respformat, sizeof(respformat)-1, enc3);
	strcpy(tempkeys, keys);
	//replace the \ chars with #1
	for (p = tempkeys; *p != 0; p++)
		if (*p == '\\')
			*p = '\x1';
	
	len = sprintf(data, respformat, profileid, type, index, tempkeys, localid);
	if (modifiedsince != 0) //append it
	{
		len += sprintf(data + len, modformat, modifiedsince);
	}

	if (sock != INVALID_SOCKET)
		len	= DoSend(data, len);

	/* If the send failed, close the socket */
	if (len <= 0)
	{
		CloseStatsConnection();
		if (callback)
			callback(localid, profileid,type, index, 0,0,"",0,instance);
	} else
	{ /* set up the callback */
		AddRequestCallback(rt_datacb, localid, profileid,type, index, callback, instance);
	}
}

void GetPersistDataValuesA(int localid, int profileid, persisttype_t type, int index, char *keys, PersDataCallbackFn callback, void *instance)
{
	GetPersistDataValuesModifiedA(localid,profileid, type,index, 0, keys,callback, instance);
}

#ifdef GSI_UNICODE
void GetPersistDataValuesModifiedW(int localid, int profileid, persisttype_t type, int index, time_t modifiedsince, unsigned short*keys, PersDataCallbackFn callback, void *instance)
{
	char* keys_A = UCS2ToUTF8StringAlloc(keys);
	GetPersistDataValuesModifiedA(localid, profileid, type, index, modifiedsince, keys_A, callback, instance);
	gsifree(keys_A);
}
#endif

#ifdef GSI_UNICODE
void GetPersistDataValuesW(int localid, int profileid, persisttype_t type, int index, unsigned short*keys, PersDataCallbackFn callback, void *instance)
{
	GetPersistDataValuesModifiedW(localid,profileid, type,index, 0, keys,callback, instance);
}
#endif

/****************************************************************************/
void SetPersistDataValuesA(int localid, int profileid, persisttype_t type, int index, const char *keyvalues, PersDataSaveCallbackFn callback, void *instance)
{
	SetPersistDataHelper(localid, profileid, type, index, keyvalues, (int)strlen(keyvalues) + 1, callback, instance, 1);
}
#ifdef GSI_UNICODE
void SetPersistDataValuesW(int localid, int profileid, persisttype_t type, int index, const unsigned short *keyvalues, PersDataSaveCallbackFn callback, void *instance)
{
	char* keyvalues_A = UCS2ToUTF8StringAlloc(keyvalues);
	SetPersistDataValuesA(localid, profileid, type, index, keyvalues_A, callback, instance);
	gsifree(keyvalues_A);
}
#endif

/****************************************************************************/
int PersistThink()
{
	int len;
	int processed;

	if (sock == INVALID_SOCKET)
		return 0;

	if (stats_initstate != init_complete)
		return 0;

	while (SocketReadable(sock))
	{
		if (rcvmax - rcvlen < 128) //make sure there are at least 128 bytes gsifree in the buffer
		{
			if (rcvmax < 256)
				rcvmax = 256;
			else
				rcvmax *= 2;
			rcvbuffer = gsirealloc(rcvbuffer, (unsigned int)(rcvmax+1));
			if (rcvbuffer == NULL)
				return 0; //errcon
		}
		len = recv(sock, rcvbuffer + rcvlen, rcvmax - rcvlen, 0);		
		if (len <= 0) //lost the connection
		{
			CloseStatsConnection();
			return 0;
		}
		rcvlen += len;
		rcvbuffer[rcvlen] = 0;

		processed = ProcessInBuffer(rcvbuffer, rcvlen);
		if (processed == rcvlen) //then we can just zero it
			rcvlen = 0;
		else
		{
			//shift the remaining data down
			memmove(rcvbuffer,rcvbuffer + processed, (unsigned int)(rcvlen - processed));
			rcvlen -= processed;
		}
		
		
	}
	if (sock == INVALID_SOCKET)
		return 0;
	else
		return 1;

}

/****************************************************************************/
int StatsThink()
{
	return PersistThink();
}

/****************************************************************************
 * UTILITY FUNCTIONS
 ****************************************************************************/

void InternalInit()
{
	internal_init = 1;
	enc1[0] = 'G';
	enc3[0] = 'P';
	finalstr[0] = '\\';

#ifdef ALLOW_DISK
	statsfile[0] = 'g';
	enc2[0] = 'I';
#endif
}


static int SendChallengeResponse(const char *indata, int gameport)
{
	static char challengestr[] = {'\0','h','a','l','l','e','n','g','e','\0'};
	char *challenge;
	char resp[128];
	char md5val[33];

	/* make this harder to find in the string table */
	char respformat[] = "\xC\x13\x1A\x1E\xD\x3F\x28\x26\x11\x5\x0\x16\x31\x1F\xA\x36\x40\x10\x28\x33\x15\x1B\x15\x17\x3E\x1\xA\x36\x40\x10\x28\x31\x1F\x1A\x11\x24\x75\x16\x33\x3\x1\x3F\x45";
 	/*  \auth\\gamename\%s\response\%s\port\%d\id\1 */
	int len;

	challengestr[0] = 'c';
	challenge = value_for_key(indata,challengestr );
	if (challenge == NULL)
	{
		closesocket(sock);
		return GE_DATAERROR;
	}
	
	len = sprintf(resp, "%d%s",g_crc32(challenge,(int)strlen(challenge)), gcd_secret_key);
	
	MD5Digest((unsigned char *)resp, (unsigned int)len, md5val);
	DOXCODE(respformat, sizeof(respformat)-1, enc3);
	len = sprintf(resp,respformat,gcd_gamename, md5val, gameport);
	
	if ( DoSend(resp, len) <= 0 )
	{
		closesocket(sock);
		return GE_NOCONNECT;
	}

	return GE_NOERROR;
}


// 09-13-2004 BED Unused parameter removed
static int RecvSessionKey()
{
	/* get the response */
	static char sesskeystr[] = {'\0','e','s','s','k','e','y','\0'};
	char resp[128];
	char *stext;
	int len = (int)recv(sock, resp,128,0);
	if (gsiSocketIsError(len))
	{
		int anError = GOAGetLastError(sock);
		closesocket(sock);

		if ((anError != WSAEWOULDBLOCK) && (anError != WSAETIMEDOUT) && (anError != WSAEINPROGRESS))
			return GE_NOCONNECT;
		else
			return GE_DATAERROR; //temp fix in case len == -1, SOCKET_ERROR
	}

	resp[len] = 0;
	DOXCODE(resp, len, enc1);
	sesskeystr[0] = 's';
	stext = value_for_key(resp, sesskeystr);
	if (stext == NULL)
	{
		closesocket(sock);
		return GE_DATAERROR;
	} else
		connid = atoi(stext);

	return GE_NOERROR;
}



static int DoSend(char *data, int len)
{
	int sent = 0;

	DOXCODE(data,len, enc1);
	strcpy(data+len,finalstr);

	// Loop to make sure async send goes through!
	while(sent < (len+7))
	{
		// Send remaining data
		int ret = send(sock, (data+sent), (len+7-sent), 0);
		if (gsiSocketIsError(ret))
		{
			int anError = GOAGetLastError(sock);
			if ((anError != WSAEWOULDBLOCK) && (anError != WSAETIMEDOUT) && (anError != WSAEINPROGRESS))
				return anError;
		}
		else if (ret == 0)
		{
			// socket was closed
			return -1; 
		}
		else
			sent += ret;
	};

	return sent;
}



#ifdef ALLOW_DISK
/* Note: lots of this is byte order and type size specific, but it shouldn't
matter since the data is read/written always on the same machine */
#define DISKLENXOR 0x70F33A5F
static void CheckDiskFile()
{
	FILE *f;
	char *line;
	char *alldata;
	int len, check, alllen, fsize;
	char filemode[3];
	/* hide our file access from the string table */
	filemode[0] = 'r'; filemode[1] = 'b'; filemode[2] = 0;
	f = fopen(statsfile,filemode);
	if (!f)
		return;
	/* get the size */
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	/* make room for the whole thing */
	alldata = (char *)gsimalloc(fsize + 2); 
	alldata[0] = 0;
	alllen = 0;
	while (!feof(f) && !ferror(f))
	{
		/* read the check and line values */
		if (fread(&check,sizeof(check),1,f) == 0 ||
			fread(&len,sizeof(len),1,f) == 0)
			break;
		len ^= DISKLENXOR;
		line = (char *)gsimalloc(len + 1);
		/* read the data */
		if (fread(line, 1, len, f) != (size_t)len)
			break;
		line[len] = 0;
		/* decode for checking */
		DOXCODE(line, len, enc2);
		/* double "check" */
		if (check != g_crc32(line, len))
		{
			gsifree(line);
			break;
		}
		/* encode for xmission */
		DOXCODE(line, len, enc1);
		memcpy(alldata + alllen, line, len);
		alllen += len;
		memcpy(alldata + alllen, finalstr, 7);
		alllen += 7;
		gsifree(line);
	}
	fclose(f);
	/* try to send */
	len = send(sock, alldata, alllen, 0);
	if (len <= 0)
	{
		closesocket(sock);
		sock = INVALID_SOCKET;
	} else
		remove(statsfile);
}

static void DiskWrite(char *line, int len)
{
	FILE *f;
	int check;
	int temp;
	char filemode[3];
	/* hide our file access from the string table */
	filemode[0] = 'a'; filemode[1] = 'b'; filemode[2] = 0;
	f = fopen(statsfile,filemode);
	if (!f)
		return;
	check = g_crc32(line, len);
	fwrite(&check, sizeof(check),1,f);
	temp = len ^ DISKLENXOR;
	fwrite(&temp, sizeof(temp), 1, f);
	DOXCODE(line, len, enc2);
	fwrite(line, 1, len, f);
	fclose(f);
}

#endif /* ALLOW_DISK */

/* simple xor encoding */
static void xcode_buf(char *buf, int len)
{
	int i;
	char *pos = enc;

	for (i = 0 ; i < len ; i++)
	{
		buf[i] ^= *pos++;
		if (*pos == 0)
			pos = enc;
	}
}

#define MULTIPLIER -1664117991
static int g_crc32(char *s, int len)
{
	int i;
    int hashcode = 0;
   
    for (i = 0; i < len; i++)
	  hashcode = hashcode * MULTIPLIER + s[i];
    return hashcode;
}

static void create_challenge(int challenge, char chstr[9])
{
	char *p = chstr;
	sprintf(chstr, "%08x",challenge);
	
	while (*p != 0)
	{
		*p = (char)((*p) + ('A' - '0') + (p-chstr));
		p++;
	}
}

/* value_for_key: this returns a value for a certain key in s, where s is a string
containing key\value pairs. If the key does not exist, it returns  NULL. 
Note: the value is stored in a common buffer. If you want to keep it, make a copy! */
static char *value_for_key(const char *s, const char *key)
{
	static int valueindex;
	char *pos,*pos2;
	char keyspec[256]="\\";
	static char value[2][256];

	valueindex ^= 1;
	strcat(keyspec,key);
	strcat(keyspec,"\\");
	pos = strstr(s,keyspec);
	if (!pos)
		return NULL;
	pos += strlen(keyspec);
	pos2 = value[valueindex];
	while (*pos && *pos != '\\')
		*pos2++ = *pos++;
	*pos2 = '\0';
	return value[valueindex];
}

/* like value_for_key, but returns an empty string instead of NULL in the not-found case */
static char *value_for_key_safe(const char *s, const char *key)
{
	char *temp;

	temp = value_for_key(s, key);
	if (!temp)
		return "";
	else
		return temp;
}

/* Return a sockaddrin for the given host (numeric or DNS) and port)
Returns the hostent in savehent if it is not NULL */
static int get_sockaddrin(const char *host, int port, struct sockaddr_in *saddr, struct hostent **savehent)
{
	struct hostent *hent = NULL;

	memset(saddr,0,sizeof(struct sockaddr_in));
	saddr->sin_family = AF_INET;
	saddr->sin_port = htons((unsigned short)port);
	if (host == NULL)
		saddr->sin_addr.s_addr = INADDR_ANY;
	else
		saddr->sin_addr.s_addr = inet_addr(host);
	
	if (saddr->sin_addr.s_addr == INADDR_NONE)
	{
		hent = gethostbyname(host);
		if (!hent)
			return 0;
		saddr->sin_addr.s_addr = *(unsigned int *)hent->h_addr_list[0];
	}
	if (savehent != NULL)
		*savehent = hent;
	return 1;
} 

/* adds a request callback to the list */
static void AddRequestCallback(reqtype_t reqtype, int localid, int profileid, persisttype_t pdtype, int pdindex, void *callback, void *instance)
{
	serverreq_t req;
	
	req.callback = callback;
	req.instance = instance;
	req.localid = localid;
	req.reqtype = reqtype;
	req.profileid = profileid;
	req.pdtype = pdtype;
	req.pdindex = pdindex;
	if (serverreqs == NULL) //create the callback array
	{
		serverreqs = ArrayNew(sizeof(serverreq_t),2,NULL);
	}
	ArrayAppend(serverreqs,&req);
}

/* sends the player authentication request (GP or CD) */
static void SendPlayerAuthRequest(char *data, int len, int localid, PersAuthCallbackFn callback, void *instance)
{
	int sentlen = 0;
	char connerror[] = "\x13\x1D\x1\x4\x0\x0\x0\x28\x1F\x6\x45\x34\x3F\x1\x1B";
	//"Connection Lost"

	if (sock != INVALID_SOCKET)
		sentlen	= DoSend(data, len);

	/* If the send failed, close the socket */
	if (sentlen <= 0)
	{
		CloseStatsConnection();
		DOXCODE(connerror, sizeof(connerror)-1, enc3);
		if (callback)
		{
#ifndef GSI_UNICODE
			callback(localid, 0,0, connerror ,instance);
#else
			unsigned short connerror_W[] = L"\x13\x1D\x1\x4\x0\x0\x0\x28\x1F\x6\x45\x34\x3F\x1\x1B";
			callback(localid, 0, 0, connerror_W, instance);
#endif
		}
	} else
	{ /* set up the callback */
		AddRequestCallback(rt_authcb, localid, 0,(persisttype_t)0,0,callback, instance);
	}
}

/* send a set request, if kvset, then only those keys/values will bet updated */
static void SetPersistDataHelper(int localid, int profileid, persisttype_t type, int index, const char *data, int len, PersDataSaveCallbackFn callback, void *instance, int kvset)
{
	char respformat[] = "\xC\x1\xA\x1E\x15\x7\x28\x1D\x0\x1\x1\x24\x75\x16\x33\x1A\x11\x1A\x4\x24\x2C\x4D\x1\x24\x34\x1B\x1\xE\x0\x1B\x28\x64\x14\x34\xE\xE\xC\x57\xB\x36\x9\xA\x10\x1D\x55\xC\x39\x14\x35\x1C\x8\x1E\xD\x3F\x51\x25\x2C\xC\x4\xC\x31\x2E";
		//\setpd\\pid\%d\ptype\%d\dindex\%d\kv\%d\lid\%d\length\%d\data\ --
	int tlen;
	char tdata[512];
	char *senddata;
	
	DOXCODE(respformat, sizeof(respformat)-1, enc3);

	if (type == pd_private_ro || type ==  pd_public_ro)
	{ //can't set read-only types, check that client side
		if (callback)
			callback(localid, profileid, type, index, 0, 0, instance);
		return;
	}
	
	tlen = sprintf(tdata, respformat, profileid, type, index, kvset, localid, len);
	if (tlen + len < 480) //we have enough room to put it in the data block
	{
		memcpy(tdata + tlen, data, (unsigned int)len);
		senddata = tdata;

	} else //need to alloc a temp buffer
	{
		senddata = (char *)gsimalloc((unsigned int)(len + tlen + 256));
		memcpy(senddata, tdata, (unsigned int)tlen);
		memcpy(senddata + tlen, data, (unsigned int)len);
	}

	if (sock != INVALID_SOCKET)
		tlen = DoSend(senddata, tlen + len);

	/* If the send failed, close the socket */
	if (tlen <= 0)
	{
		CloseStatsConnection();
		if (callback)
			callback(localid, profileid, type, index, 0, 0, instance);
	} else
	{ /* set up the callback */
		AddRequestCallback(rt_savecb, localid, profileid,type, index, callback, instance);
	}
	if (senddata != tdata) //if we alloc'd before sending
		gsifree(senddata);
}

/* returns 1 if the socket is readable, 0 otherwise */
static int SocketReadable(SOCKET s)
{
	return CanReceiveOnSocket(s);
/*
	fd_set set;
	struct timeval tv = {0,0};
	int n;
	
	if (s == INVALID_SOCKET)
		return 0;

	FD_ZERO(&set);
	FD_SET(s, &set);

	n = select(FD_SETSIZE, &set, NULL, NULL, &tv);
	
	return n;
*/
}

/* find the \final\ string */
static char *FindFinal(char *buff, int len)
{
	char *pos = buff;
	
	while (pos - buff < len - 6)
	{
		if (pos[0] == '\\' && 
			pos[1] == 'f'  && 
			pos[2] == 'i'  && 
			pos[3] == 'n'  && 
			pos[4] == 'a'  && 
			pos[5] == 'l'  && 
			pos[6] == '\\')
		{
			return pos;
		} else
			pos++;
	}
	return NULL;	
}

/* find the request in the callback list */
static int FindRequest(reqtype_t reqtype, int localid, int profileid)
{
	int i;
	serverreq_t *req;

	if (serverreqs == NULL)
		return -1;
	for (i = 0 ; i < ArrayLength(serverreqs); i++)
	{
		req = (serverreq_t *)ArrayNth(serverreqs, i);
		if (req->reqtype == reqtype && req->localid == localid && req->profileid == profileid)
			return i;
	}
	return -1;
	
}

/* process the playerauth result */
static void ProcessPlayerAuth(const char *buf, int len)
{
	// \\pauthr\\100000\\lid\\1
	int reqindex;
	char *errmsg;
	int pid;
	int lid;
	pid = atoi(value_for_key_safe(buf,"pauthr"));
	lid = atoi(value_for_key_safe(buf,"lid"));
	errmsg = value_for_key_safe(buf,"errmsg");
	reqindex = FindRequest(rt_authcb,lid,0);
	if (reqindex == -1)
		return;
	((serverreq_t *)ArrayNth(serverreqs, reqindex))->profileid = pid;
	CallReqCallback(reqindex,(pid > 0),0, errmsg,0);
	
	GSI_UNUSED(len);
}

/* process the get profileid result */
static void ProcessGetPid(const char *buf, int len)
{
	// \\getpidr\\100000\\lid\\1
	int reqindex;
	int pid;
	int lid;
	pid = atoi(value_for_key_safe(buf,"getpidr"));
	lid = atoi(value_for_key_safe(buf,"lid"));
	reqindex = FindRequest(rt_profilecb,lid,0);
	if (reqindex == -1)
		return;
	((serverreq_t *)ArrayNth(serverreqs, reqindex))->profileid = pid;
	CallReqCallback(reqindex,(pid > 0),0,NULL,0);
	
	GSI_UNUSED(len);
}

/* process the get data result */
static void ProcessGetData(const char *buf, int len)
{
	// \\getpdr\\1\\lid\\1\\mod\\1234\\length\\5\\data\\mydata\\final
	int reqindex;
	int pid;
	int lid;
	int success;
	int length;
	time_t modified;
	char *data;
	success = atoi(value_for_key_safe(buf,"getpdr"));
	lid = atoi(value_for_key_safe(buf,"lid"));
	pid = atoi(value_for_key_safe(buf,"pid"));
	modified = atoi(value_for_key_safe(buf,"mod"));
	reqindex = FindRequest(rt_datacb,lid,pid);
	if (reqindex == -1)
		return;
	length = atoi(value_for_key_safe(buf,"length"));
	data = strstr(buf,"\\data\\");
	if (!data)
	{
		length = 0;
		data = "";
	} else
		data += 6; //skip the key
	CallReqCallback(reqindex,success,modified, data,length);
	
	GSI_UNUSED(len);
}

/* process the set data result */
static void ProcessSetData(const char *buf, int len)
{
	// \\setpdr\\1\\lid\\2\\pid\\100000\\mod\\12345
	int reqindex;
	int pid;
	int lid;
	int success;
	int modified;
	success = atoi(value_for_key_safe(buf,"setpdr"));
	pid = atoi(value_for_key_safe(buf,"pid"));
	lid = atoi(value_for_key_safe(buf,"lid"));
	modified = atoi(value_for_key_safe(buf,"mod"));
	reqindex = FindRequest(rt_savecb,lid,pid);
	if (reqindex == -1)
		return;
	CallReqCallback(reqindex,success,modified,NULL,0);
	
	GSI_UNUSED(len);
}

/* process a single statement */
static void ProcessStatement(char *buff, int len)
{
	//determine the type

	buff[len] = 0;
	//	printf("GOT: %s\n",buff);
	if (strncmp(buff,"\\pauthr\\",8) == 0)
	{
		ProcessPlayerAuth(buff, len);
	} else if (strncmp(buff,"\\getpidr\\",9) == 0)
	{
		ProcessGetPid(buff, len);
	} else if (strncmp(buff,"\\getpidr\\",9) == 0)
	{
		ProcessGetPid(buff, len);
	} else if (strncmp(buff,"\\getpdr\\",8) == 0)
	{
		ProcessGetData(buff, len);
	} else if (strncmp(buff,"\\setpdr\\",8) == 0)
	{
		ProcessSetData(buff, len);
	}
	
}


/* processes statements in the buffer and returns amount processed */
// 09-13-2004 BED Modified loop to silence compiler warning
static int ProcessInBuffer(char *buff, int len)
{
	char *pos;
	int oldlen = len;

	pos = FindFinal(buff, len);
	//while (len > 0 && (pos = FindFinal(buff, len)))
	while ((len > 0) && (pos != NULL))
	{
		DOXCODE(buff,pos - buff, enc1);
		ProcessStatement(buff, pos - buff);
		len -= (pos - buff) + 7;
		buff = pos + 7; //skip the final
		if (len>0)
			pos = FindFinal(buff, len);
	}
	return oldlen - len; //amount processed
}

/* call a single callback function */
static void CallReqCallback(int reqindex, int success, time_t modified, char *data, int length)
{
	serverreq_t *req;
	if (reqindex < 0 || reqindex >= ArrayLength(serverreqs))
		return;
	req = (serverreq_t *)ArrayNth(serverreqs, reqindex);
	if (req->callback)
		switch (req->reqtype)
		{
		case rt_authcb:
#ifndef GSI_UNICODE
			((PersAuthCallbackFn )req->callback)(req->localid, req->profileid, success, data, req->instance);
#else
			{
				unsigned short* data_W = UTF8ToUCS2StringAlloc(data);
				((PersAuthCallbackFn )req->callback)(req->localid, req->profileid, success, data_W, req->instance);
				gsifree(data_W);
			}
#endif
			break;
		case rt_datacb:
			((PersDataCallbackFn )req->callback)(req->localid,req->profileid,req->pdtype, req->pdindex, success, modified, data, length, req->instance);
			break;
		case rt_savecb:
			((PersDataSaveCallbackFn )req->callback)(req->localid, req->profileid, req->pdtype, req->pdindex,success, modified, req->instance);
			break;
		case rt_profilecb:
			((ProfileCallbackFn )req->callback)(req->localid, req->profileid, success, req->instance);
			break;
		}
	ArrayDeleteAt(serverreqs,reqindex);
}

/* if we get disconnected while callbacks are still pending, make sure we
call all of them, with a success of 0 */
static void ClosePendingCallbacks()
{
	int i;

	if (serverreqs == NULL)
		return;
	for (i = ArrayLength(serverreqs) - 1 ; i >= 0 ; i--)
	{
		char connerror[] = "\x13\x1D\x1\x4\x0\x0\x0\x28\x1F\x6\x45\x34\x3F\x1\x1B";
	//"Connection Lost"
		DOXCODE(connerror, sizeof(connerror)-1, enc3);

		CallReqCallback(i,0,0,connerror,0);
		
	}
	ArrayFree(serverreqs);
	serverreqs = NULL;
}



/****************************************************************************/
/* BUCKET FUNCTIONS */
/****************************************************************************/
int GetTeamIndex(statsgame_t game, int tnum)
{
	FIXGAME(game, tnum);
	return *(int *)ArrayNth(game->teamnums,tnum);
}
int GetPlayerIndex(statsgame_t game, int pnum)
{
	FIXGAME(game, pnum);
	return *(int *)ArrayNth(game->playernums,pnum);
}

static int ServerOpInt(statsgame_t game,char *name, BucketFunc func,  int value, int index)
{
	int *ret;
	DoFunc(func, game, name, &value, bt_int, ret);
	
	GSI_UNUSED(index);
	return *(int *)ret;
}
static double ServerOpFloat(statsgame_t game,char *name, BucketFunc func,  double value, int index)
{
	double *ret;
	DoFunc(func, game, name, &value, bt_float, ret);

	GSI_UNUSED(index);
	return *(double *)ret;
}
static char *ServerOpString(statsgame_t game,char *name, BucketFunc func,  char *value, int index)
{
	char *ret;
	DoFunc(func, game, name, value, bt_string, ret);

	GSI_UNUSED(index);
	return ret;
}

static int TeamOpInt(statsgame_t game,char *name, BucketFunc func,  int value, int index)
{
	char fullname[64];
	sprintf(fullname, "%s_t%d",name, GetTeamIndex(game, index));
	return ServerOpInt(game, fullname, func, value, index);
}
static double TeamOpFloat(statsgame_t game,char *name, BucketFunc func,  double value, int index)
{
	char fullname[64];
	sprintf(fullname, "%s_t%d",name, GetTeamIndex(game, index));
	return ServerOpFloat(game, fullname, func, value, index);
}
static char *TeamOpString(statsgame_t game,char *name, BucketFunc func,  char *value, int index)
{
	char fullname[64];
	sprintf(fullname, "%s_t%d",name, GetTeamIndex(game, index));
	return ServerOpString(game, fullname, func, value, index);
}

static int PlayerOpInt(statsgame_t game,char *name, BucketFunc func,  int value, int index)
{
	char fullname[64];
	sprintf(fullname, "%s_%d",name, GetPlayerIndex(game, index));
	return ServerOpInt(game, fullname, func, value, index);
}
static double PlayerOpFloat(statsgame_t game,char *name, BucketFunc func,  double value, int index)
{
	char fullname[64];
	sprintf(fullname, "%s_%d",name, GetPlayerIndex(game, index));
	return ServerOpFloat(game, fullname, func, value, index);
}
static char *PlayerOpString(statsgame_t game,char *name, BucketFunc func,  char *value, int index)
{
	char fullname[64];
	sprintf(fullname, "%s_%d",name, GetPlayerIndex(game, index));
	return ServerOpString(game, fullname, func, value, index);
}

static char *CreateBucketSnapShot(bucketset_t buckets)
{
	return DumpBucketSet(buckets);
}

#ifdef __cplusplus
}
#endif
