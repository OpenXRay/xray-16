// GameSpy Presence and Messaging SDK Stress Test
// Dan "Mr. Pants" Schoenblum
// Copyright 2000 GameSpy Industries, Inc

/*************
** INCLUDES **
*************/
#include <limits.h>
#include "../gp.h"
#include "../../common/gsAvailable.h"

/************
** DEFINES **
************/
//#define CONNECTION_MANAGER            "localhost"

#define PROFILES_MAX                  10000

#define LOG_FILE                      "gpstress.log"

#define NEXT_OP_TIME_MIN              2000000
#define NEXT_OP_TIME_MAX              2000000

#define IS_INSTANT_OP(op)             (((op) >= FIRST_INSTANT_OP) && ((op) <= LAST_INSTANT_OP))

#define CONNECT_TIME_MIN              20000
#define CONNECT_TIME_MAX              20000

#define SHUTDOWN_DISCONNECTS_PER_SEC  50

#define DEFAULT_RUN_TIME              (2 * 60 * 60 * 1000)
#define DEFAULT_MAX_CONNECTED         100

#define SHOW_INFO_TIME                1000

#define DEFAULT_CONNECTS_PER_SEC      15

#define MAX_OTHERS                    20

#define PRODUCT_ID_MAX                18

#define VALIDATION_MAX_NICKS          512

#define MAX_OUTSTANDING_CONNECTIONS   1

/**********
** TYPES **
**********/
#ifndef __cplusplus
typedef enum
{
	false,
	true
} bool;
#endif

typedef enum
{
	// No active operation.
	///////////////////////
	OpNull = -1,

	// These operations all take a ceratin amount of time.
	//////////////////////////////////////////////////////
	OpConnect,
	OpNewProfile,
	OpGetInfo,
#ifdef TEST_SEARCH
	OpProfileSearch,
	OpFindPlayers,
#endif

	// These are all instant operations.
	////////////////////////////////////
#define FIRST_INSTANT_OP OpSetInfo
	OpSetInfo,
	OpBuddyRequest,
	OpDeleteBuddy,
	OpSetStatus1,  // Do setstatus 5x as often as other ops
	OpSetStatus2,
	OpSetStatus3,
	OpSetStatus4,
	OpSetStatus5,
	OpSendBuddyMessage,
	OpSetInvitable,
#define LAST_INSTANT_OP OpSetInvitable

	// The number of possible operations.
	/////////////////////////////////////
	NumOps
} Op;

typedef struct
{
	// The index of this profile in the list.
	/////////////////////////////////////////
	int index;

	// Loaded profile info.
	///////////////////////
	int userID;
	char email[GP_EMAIL_LEN];
	char password[GP_PASSWORD_LEN];
	int profileID;
	char nick[GP_NICK_LEN];
	bool invalid;

	// GP stuff.
	////////////
	GPConnection gp;               // The profile's GP object.
	bool gpInitialized;            // False until gpInitialize is called for this profile.
	unsigned long disconnectTime;  // Disconnect at this time.
	GPProfile others[MAX_OTHERS];  // Remote profiles that this profile knows about.
	int numOthers;                 // The number of others.
	unsigned long connectTime;     // When the connect attempt was made.
	bool connected;                // True when gpConnect finished successfully.

	// Op data.
	///////////
	Op activeOp;               // The active operation, OpNull between operations.
	unsigned long nextOpTime;  // At this time, pick a new random operation.
	int completedOps;          // Total number of operations completed.
} Profile;

typedef struct
{
	GPResult result;
	char nicks[VALIDATION_MAX_NICKS][GP_NICK_LEN];
	int numNicks;
	Profile * profile;
} ValidationData;

/************
** GLOBALS **
************/
char profilesFile[MAX_PATH];         // The file from which to load the profiles.
Profile profiles[PROFILES_MAX];      // All of the loaded profiles.
int numProfiles;                     // The number of loaded profiles.
int numConnections;                  // The number of profiles that are initialized/connected.
int maxConnections;                  // The maximum number of connected profiles.
int highestConnections;              // The highest number of connections.
int totalConnections;                // The total number of connections.
int numConnected;                    // The number of profiles that are actually logged in.
int highestConnected;                // The highest simultaneous connected.
int totalConnected;                  // The total number connected over the entire run.
int connectsPerSecond;               // Number of connection attempts per second.
unsigned long lastConnectTime;       // The last time there was a connect.
unsigned long startShutdownTime;     // When to start shutting down.
bool shuttingDown;                   // True if we're in the process of shutting down.
unsigned long runTime;               // How long to run for.

/********************
** PROFILE LOADING **
********************/
// This warning needs to be fixed for this file
// Disabling for now
#pragma warning ( disable: 4702 )
#pragma warning ( disable: 4127 )
bool ReadField(FILE * fp, char * field)
{
	char * str = field;
	int c;
	*str = '\0';
	while(1)
	{
		// Get a char.
		//////////////
		c = fgetc(fp);

		// Check for end of field.
		//////////////////////////
		if((c == ',') || (c == '\n') || (c == -1))
		{
			// Check for no chars read.
			///////////////////////////
			if(field == str)
				return false;

			// Cap it off and return.
			/////////////////////////
			*str = '\0';
			return true;
		}

		*str++ = (char)c;
	}

	return false;
}

bool ReadProfile(FILE * fp, Profile * profile)
{
	char intBuffer[16];

	// Read the user id.
	////////////////////
	if(!ReadField(fp, intBuffer))
		return false;
	profile->userID = atoi(intBuffer);
	assert(profile->userID);

	// Read the email.
	//////////////////
	ReadField(fp, profile->email);

	// Read the password.
	/////////////////////
	ReadField(fp, profile->password);

	// Read the profile id.
	///////////////////////
	ReadField(fp, intBuffer);
	profile->profileID = atoi(intBuffer);
	assert(profile->profileID);

	// Read the nick.
	/////////////////
	ReadField(fp, profile->nick);

	return true;
}

bool LoadProfiles(void)
{
#if 0
	FILE * fp;

	// Open the file.
	/////////////////
	fp = fopen(profilesFile, "rt");
	if(!fp)
	{
		printf("Failed to open the profiles file (%s)!\n", profilesFile);
		return false;
	}

	// Read the profiles.
	/////////////////////
	while(ReadProfile(fp, &profiles[numProfiles]))
	{
		// One more.
		////////////
		numProfiles++;

		// Check for too many.
		//////////////////////
		if(numProfiles == PROFILES_MAX)
		{
			printf("Too many profiles in the profile file!\nIncrease PROFILES_MAX (%d)\n", PROFILES_MAX);
			return false;
		}
	}

	// Close the file.
	//////////////////
	fclose(fp);

	printf("Loaded %d profiles from %s\n", numProfiles, profilesFile);
#else
	int i;

	for(i = 0 ; i < maxConnections ; i++)
	{
		strcpy(profiles[i].nick, "gpstress");
		sprintf(profiles[i].email, "gpstress%04d@gamespy.com", i);
		strcpy(profiles[i].password, "gpstress");
	}

	numProfiles = i;
#endif

	return true;
}

/*********************
** RANDOM FUNCTIONS **
**********************/
char RandomChar(void)
{
	static const char characters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789!@#$%^&*()_+-=~`<,>.?/:;\"'{[}]|\\";
	int index;

	index = (rand() % (sizeof(characters) - 1));

	return characters[index];
}

char * RandomString(char * buffer)
{
	int i;
	int len;

	// Random length.
	/////////////////
	len = ((rand() % 10) + 5);

	// Set random chars.
	////////////////////
	for(i = 0 ; i < len ; i++)
		buffer[i] = RandomChar();
	buffer[i] = '\0';

	return buffer;
}

// min & max are inclusive.
///////////////////////////
int RandomInt(int min, int max)
{
	int range = ((max - min) + 1);
	return (min + ((rand() * range) / (RAND_MAX + 1)));
}

bool RandomBool(void)
{
	return (bool)RandomInt(0, 1);
}

GPProfile RandomOther(Profile * profile)
{
	int nIndex;

	assert(profile->numOthers > 0);

	nIndex = RandomInt(0, profile->numOthers - 1);
	assert(nIndex >= 0);
	assert(nIndex < profile->numOthers);

	return profile->others[nIndex];
}

void RandomServer(void)
{
#if 0
	strcpy(GPConnectionManagerHostname, "mrpants");
#else
	static const char * servers[] =
	{
#if 1
		"mrpants",
		"mrpants1"
#else
		"aphexgp1",
		"aphexgp2"
#endif
	};

	const char * server = servers[RandomInt(0, (sizeof(servers) / sizeof(*servers)) - 1)];
	strcpy(GPConnectionManagerHostname, server);
#endif
}

/************
** GENERAL **
************/
void MessageOther(Profile * profile, GPProfile other)
{
#if 1
	int pid;

	// Filter out certain profiles.
	///////////////////////////////
	gpIDFromProfile(&profile->gp, other, &pid);
	switch(pid)
	{
	case 100001: // Mr. Pants@dan@gamespy.com
	case 100002: // walla@bryan@gamespy.com
	case 100013: // lumberjack@jason@gamespy.com
		return;
	}

	gpSendBuddyMessage(&profile->gp, other, "STRESS-TEST");
#endif
}

void Log(const char * buffer)
{
	FILE * fp;

	static char timeBuffer[64];
	time_t thetime;
	struct tm *ltime;
	time(&thetime);
	ltime = localtime(&thetime);
	sprintf(timeBuffer, "%-2.2d/%-2.2d/%-2.2d %-2.2d:%-2.2d:%-2.2d: ", ltime->tm_mon+1, ltime->tm_mday, ltime->tm_year % 100, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);

	fp = fopen(LOG_FILE, "at");
	if(fp)
	{
		fprintf(fp, "%s%s", timeBuffer, buffer);
		fclose(fp);
	}

	OutputDebugString(timeBuffer);
	OutputDebugString(buffer);

	printf("%s%s", timeBuffer, buffer);
}

/************************
** GP GLOBAL CALLBACKS **
************************/
void GetInfoCallback(GPConnection * connnection, GPGetInfoResponseArg * arg, Profile * profile);
void AddOther(Profile * profile, GPProfile gpProfile);
void PrintOp(Profile * profile, const char * opString);

void ErrorCallback(GPConnection * connection, GPErrorArg * arg, Profile * profile)
{
	char * errorCodeString;
	char * resultString;
	static char buffer[4098];

	// Ignore errors that we're expecting to generate (due to randomness).
	//////////////////////////////////////////////////////////////////////
	switch(arg->errorCode)
	{
	case GP_ADDBUDDY_ALREADY_BUDDY:
	case GP_BM_NOT_BUDDY:
		return;
	}

	// Get a string for the result.
	///////////////////////////////
#define RESULT(x) case x: resultString = #x; break;
	switch(arg->result)
	{
	RESULT(GP_NO_ERROR)
	RESULT(GP_MEMORY_ERROR)
	RESULT(GP_PARAMETER_ERROR)
	RESULT(GP_NETWORK_ERROR)
	RESULT(GP_SERVER_ERROR)
	default:
		resultString = "Unknown result!\n";
	}

	// Get a string for the error code.
	///////////////////////////////////
#define ERRORCODE(x) case x: errorCodeString = #x; break;
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
	ERRORCODE(GP_LOGIN_BAD_UNIQUENICK)
	ERRORCODE(GP_LOGIN_BAD_PREAUTH)
	ERRORCODE(GP_NEWUSER)
	ERRORCODE(GP_NEWUSER_BAD_NICK)
	ERRORCODE(GP_NEWUSER_BAD_PASSWORD)
	ERRORCODE(GP_NEWUSER_UNIQUENICK_INVALID)
	ERRORCODE(GP_NEWUSER_UNIQUENICK_INUSE)
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
	ERRORCODE(GP_CHECK)
	ERRORCODE(GP_CHECK_BAD_EMAIL)
	ERRORCODE(GP_CHECK_BAD_NICK)
	ERRORCODE(GP_CHECK_BAD_PASSWORD)
	ERRORCODE(GP_REVOKE)
	ERRORCODE(GP_REVOKE_NOT_BUDDY)
	ERRORCODE(GP_REGISTERUNIQUENICK)
	ERRORCODE(GP_REGISTERUNIQUENICK_TAKEN)
	ERRORCODE(GP_REGISTERUNIQUENICK_RESERVED)
	ERRORCODE(GP_REGISTERUNIQUENICK_BAD_NAMESPACE)
	default:
		errorCodeString = "Unknown error code!\n";
	}

	// Print out the info.
	//////////////////////
	if(arg->fatal)
		sprintf(buffer, "%04d: FATAL ERROR", profile->index);
	else
		sprintf(buffer, "%04d: ERROR", profile->index);
	sprintf(buffer + strlen(buffer), ", RESULT: %s (%d)", resultString, arg->result);
	sprintf(buffer + strlen(buffer), ", ERROR CODE: %s (0x%X)", errorCodeString, arg->errorCode);
	sprintf(buffer + strlen(buffer), ", ERROR STRING: %s\n", arg->errorString);
	Log(buffer);

	// Disconnect.
	//////////////
	profile->disconnectTime = 0;
	GSI_UNUSED(connection);
}

void RecvBuddyStatusCallback(GPConnection * connection, GPRecvBuddyStatusArg * arg, Profile * profile)
{
	PrintOp(profile, "RecvBuddyStatusCallback");

#if 1
	// Get info half the time.
	//////////////////////////
	if(RandomBool())
		gpGetInfo(&profile->gp, arg->profile, GP_DONT_CHECK_CACHE, GP_NON_BLOCKING, GetInfoCallback, profile);

	// Send a message sometimes.
	////////////////////////////
	if(RandomInt(0, 9) == 0)
		MessageOther(profile, arg->profile);

	// Add the other.
	/////////////////
	AddOther(profile, arg->profile);
#endif
	GSI_UNUSED(connection);
}

#if 1
void RecvBuddyRequestCallback(GPConnection * connection, GPRecvBuddyRequestArg * arg, Profile * profile)
{
	PrintOp(profile, "RecvBuddyRequestCallback");

	// Get info half the time.
	//////////////////////////
	if(RandomBool())
		gpGetInfo(&profile->gp, arg->profile, GP_DONT_CHECK_CACHE, GP_NON_BLOCKING, GetInfoCallback, profile);

	// Accept it half the time.
	///////////////////////////
	if(RandomBool())
		gpAuthBuddyRequest(&profile->gp, arg->profile);

	// Add the other.
	/////////////////
	AddOther(profile, arg->profile);
	GSI_UNUSED(connection);
}

void RecvBuddyMessageCallback(GPConnection * connection, GPRecvBuddyMessageArg * arg, Profile * profile)
{
	PrintOp(profile, "RecvBuddyMessageCallback");

	// Get info half the time.
	//////////////////////////
	if(RandomBool())
		gpGetInfo(&profile->gp, arg->profile, GP_DONT_CHECK_CACHE, GP_NON_BLOCKING, GetInfoCallback, profile);

	// Reply sometimes.
	///////////////////
	if(RandomInt(0, 9) == 0)
		MessageOther(profile, arg->profile);

	// Add the other.
	/////////////////
	AddOther(profile, arg->profile);
	GSI_UNUSED(connection);
}

void RecvGameInviteCallback(GPConnection * connection, GPRecvGameInviteArg * arg, Profile * profile)
{
	PrintOp(profile, "RecvGameInviteCallback");

	// Get info half the time.
	//////////////////////////
	if(RandomBool())
		gpGetInfo(&profile->gp, arg->profile, GP_DONT_CHECK_CACHE, GP_NON_BLOCKING, GetInfoCallback, profile);

	// Send a message sometimes.
	////////////////////////////
	if(RandomInt(0, 9) == 0)
		MessageOther(profile, arg->profile);

	// Add the other.
	/////////////////
	AddOther(profile, arg->profile);
	GSI_UNUSED(connection);
}
#endif

/********************
** GP OP CALLBACKS **
********************/
void EndOp(Profile * profile);

const int RemoteAuthProfiles[] =
{
	58214083,
	58214074,
	58214075,
	58214076,
	58214077,
	58214078,
	58214079,
	58214080,
	58214081,
	58214082
};

void ConnectCallback(GPConnection * connection, GPConnectResponseArg * arg, Profile * profile)
{
	if(arg->result == GP_NO_ERROR)
	{
		// Finished connecting.
		///////////////////////
		profile->connected = true;
		numConnected++;
		totalConnected++;
		highestConnected = max(numConnected, highestConnected);

		if(arg->profile != RemoteAuthProfiles[profile->index % 10])
		{
			char buffer[4096];
			gsi_time ms = (current_time() - profile->connectTime);
			sprintf(buffer, "%d: XXX Connection ProfileID Error (%d != %d) [%d]: %d.%03ds\n", totalConnected, arg->profile, RemoteAuthProfiles[profile->index % 10], (profile->index % 10), ms / 1000, ms % 1000);
			Log(buffer);
		}

#if 1
		// log the connection
		{
			char buffer[256];
			gsi_time ms = (current_time() - profile->connectTime);
			sprintf(buffer, "%sConnected: %d time: %d.%03ds\n", (ms>=1000)?"XX ":"", totalConnected, ms / 1000, ms % 1000);
			Log(buffer);
		}
#endif

#if 1
		// always disconnect right away
		profile->disconnectTime = 0;
#endif
	}
	else
	{
#if 1
		// log the error
		{
			char buffer[4096];
			gsi_time ms = (current_time() - profile->connectTime);
			sprintf(buffer, "%d: XXX Connection Failed [%d]: %d.%03ds\n", totalConnected, (profile->index % 10), ms / 1000, ms % 1000);
			Log(buffer);
		}
#endif
		// Disconnect this profile.
		///////////////////////////
		profile->disconnectTime = 0;
	}

	// End the op.
	//////////////
	EndOp(profile);
	GSI_UNUSED(connection);
}

void GetInfoCallback(GPConnection * connection, GPGetInfoResponseArg * arg, Profile * profile)
{
	if(arg->result != GP_NO_ERROR)
	{
		printf("%04d: gpGetInfo failed\n", profile->index);
		return;
	}
	GSI_UNUSED(connection);
}

void FindPlayersCallback(GPConnection * connection, GPFindPlayersResponseArg * arg, Profile * profile)
{
	int i;

	if(arg->result != GP_NO_ERROR)
	{
		printf("%04d: gpFindPlayers failed\n", profile->index);
		return;
	}

	// Get all of their info.
	/////////////////////////
	for(i = 0 ; i < arg->numMatches ; i++)
		gpGetInfo(&profile->gp, arg->matches[i].profile, GP_DONT_CHECK_CACHE, GP_NON_BLOCKING, GetInfoCallback, profile);

	// Invite someone.
	//////////////////
	if(arg->numMatches > 0)
	{
		int index = RandomInt(0, arg->numMatches - 1);
		int productID = RandomInt(1, PRODUCT_ID_MAX);

		gpInvitePlayer(&profile->gp, arg->matches[index].profile, productID,NULL);
	}
	GSI_UNUSED(connection);
}

void ProfileSearchCallback(GPConnection * connection, GPProfileSearchResponseArg * arg, Profile * profile)
{
	int i;

	if(arg->result != GP_NO_ERROR)
	{
		printf("%04d: gpProfileSearch failed\n", profile->index);
		return;
	}

	// Get all of their info.
	/////////////////////////
	for(i = 0 ; i < arg->numMatches ; i++)
		gpGetInfo(&profile->gp, arg->matches[i].profile, GP_DONT_CHECK_CACHE, GP_NON_BLOCKING, GetInfoCallback, profile);
	GSI_UNUSED(connection);
}

/*****************
** OP FUNCTIONS **
*****************/
void AddOther(Profile * profile, GPProfile gpProfile)
{
	int i;

	// Check for full.
	//////////////////
	if(profile->numOthers == MAX_OTHERS)
		return;

	// Check if we already have this one.
	/////////////////////////////////////
	for(i = 0 ; i < profile->numOthers ; i++)
		if(profile->others[i] == gpProfile)
			return;

	// Add it.
	//////////
	profile->others[profile->numOthers] = gpProfile;
	profile->numOthers++;
}

unsigned long GetRandomOpTime(void)
{
	// Between NEXT_OP_TIME_MIN and NEXT_OP_TIME_MAX.
	/////////////////////////////////////////////////
	return RandomInt(NEXT_OP_TIME_MIN, NEXT_OP_TIME_MAX);
}

Op GetRandomOp(void)
{
	// Between 1 (skip OpConnect) and (NumOps - 1).
	///////////////////////////////////////////////
	Op op = (Op)(((rand() * (NumOps - 1)) / (RAND_MAX + 1)) + 1);

	assert(op > OpConnect);
	assert(op < NumOps);

	return op;
}

void NewOp(Profile * profile)
{
	// Is this the first op?
	////////////////////////
	if(profile->completedOps == 0)
	{
		// The first op is always connect.
		//////////////////////////////////
		profile->activeOp = OpConnect;
	}
	else
	{
#if 1
		// Pick a random op.
		////////////////////
		profile->activeOp = GetRandomOp();
#else
		// Always do status.
		////////////////////
		profile->activeOp = OpSetStatus1;
#endif
	}
}

void EndOp(Profile * profile)
{
	// No active op.
	////////////////
	profile->activeOp = OpNull;

	// Set the next op time.
	////////////////////////
	profile->nextOpTime = (current_time() + GetRandomOpTime());

	// One more op completed.
	/////////////////////////
	profile->completedOps++;
}

void PrintOp(Profile * profile, const char * opString)
{
#if 0
	static char buffer[256];
	sprintf(buffer, "%04d: %s\n", profile->index, opString);
	OutputDebugString(buffer);
#endif
	GSI_UNUSED(profile);
	GSI_UNUSED(opString);
}

void StartOp(Profile * profile)
{
	int i;
	int num;
	int intArray[16];
	char string1[16];
	char string2[16];

	// Handle based on op.
	//////////////////////
	switch(profile->activeOp)
	{
	case OpNull:
		assert(0);
		printf("%04d: Tried to start null op\n", profile->index);
		break;

	case OpConnect:
		PrintOp(profile, "OpConnect");

		// Start the connect.
		/////////////////////
#if 0
		RandomServer();
#endif
#if 0
		gpConnect(&profile->gp, profile->nick, profile->email, profile->password, (RandomInt(0, 9) == 0)?GP_FIREWALL:GP_NO_FIREWALL, GP_NON_BLOCKING, ConnectCallback, profile);
#else
		{
			const char * authtokens[] =
			{
				"GMT/61LHe4Yu+pHI0eDXu8GMdc51iXjsNvcz1GGoWJ9orpnbZIbp3CVBQddeSZOSq58",
				"GMTcUM+iiQuzaw8crHxBFPCVV6J9V14KnSNHnHbQIXGwN6XpKv97yqSbOqxrgIWZCeZ",
				"GMTxiKFKsnGdOeBaJxmJOQBNSqbm9zOjbu/cTiqpBJp1pqqOAnI1Jh81TPlWmFuvt0h",
				"GMTqUg6QEerwnwi87fZQMcfzQ0xA/zNC96VCtx6GavTv7wzDtGpOWxJHUkm20qMxVsg",
				"GMTV2uHalvcDgtvBsz62QF1ifbCJJ93QONO9TNGVGlDghs6ocL2qOiQVqNJS7LvTZ39",
				"GMTq9Z7Tg/tkEMzqEODuQdNSQPgt5MdAwH26+lQGHliL9n3nH0b0D38fTFWRbkmk4K9",
				"GMTwHyinDuomW0XmQswNyVpgdlQvnuGCR1it+kHCnUDmR5CG3svSufrDcjuHdlwfVON",
				"GMTBZgPikENuiEON0by04RMR112ShZsDctzVbqljZ3ZufDvyugygztRlKIM9gyLwSdy",
				"GMTmHNpmNufV9WuGIGET8E0Xd//56eYWO4mDaD+4KebWQWvc8CqWbqSeII6x9BxQMxQ",
				"GMTyI447fNIYVh1vdI6aErka4TzJcYWctTU/Dzi2GK+4J3Vf/VAqQOs43R73I7dyX+u"
			};
			const char * partnerchallenges[] =
			{
				"WI[A)IuA",
				"RNd$)q)0",
				"LQTi;4#X",
				"AOA)(57=",
				"ob=0XiWJ",
				"mRnTENJ@",
				"9%@A+NKA",
				"L=0O)Lg2",
				"YR6[Zz%B",
				"$kWZ%]~@"
			};
			int tokenIndex = (profile->index % 10);
			gpConnectPreAuthenticated(&profile->gp, authtokens[tokenIndex], partnerchallenges[tokenIndex], GP_FIREWALL, GP_NON_BLOCKING, ConnectCallback, profile);
		}
#endif
		break;

	case OpNewProfile:
		PrintOp(profile, "OpNewProfile");

		// Pretend this op didn't happen, another will be picked next frame.
		////////////////////////////////////////////////////////////////////
		profile->activeOp = OpNull;
		break;

	case OpGetInfo:
		PrintOp(profile, "OpGetInfo");

		// Do we know of any other profiles?
		////////////////////////////////////
		if(profile->numOthers > 0)
			gpGetInfo(&profile->gp, RandomOther(profile), GP_DONT_CHECK_CACHE, GP_NON_BLOCKING, GetInfoCallback, profile);
		else
			profile->activeOp = OpNull;
		break;

#ifdef TEST_SEARCH
	case OpProfileSearch:
		PrintOp(profile, "OpProfileSearch");

		// Do a search.
		///////////////
		num = RandomInt(0, 3);
		if(num == 0)
			gpProfileSearch(&profile->gp, "pants", "dan@gamespy.com", NULL, NULL, 0, GP_NON_BLOCKING, ProfileSearchCallback, profile);
		else if(num == 1)
			gpProfileSearch(&profile->gp, "crt", NULL, NULL, NULL, 0, GP_NON_BLOCKING, ProfileSearchCallback, profile);
		else
			gpProfileSearch(&profile->gp, "STRESS-TEST", NULL, NULL, NULL, 0, GP_NON_BLOCKING, ProfileSearchCallback, profile);
		break;

	case OpFindPlayers:
		PrintOp(profile, "OpFindPlayers");

		// Find some players.
		/////////////////////
		gpFindPlayers(&profile->gp, RandomInt(1, PRODUCT_ID_MAX), GP_NON_BLOCKING, FindPlayersCallback, profile);
		break;
#endif

	case OpSetInfo:
		PrintOp(profile, "OpSetInfo");

		// Set some random info.
		////////////////////////
		gpSetInfos(&profile->gp, GP_FIRSTNAME, "STRESS-TEST");
		gpSetInfos(&profile->gp, GP_LASTNAME, "STRESS-TEST");
		gpSetInfoi(&profile->gp, GP_ICQUIN, RandomInt(100000, 40000000));
		gpSetInfos(&profile->gp, GP_HOMEPAGE, "STRESS-TEST");
		gpSetInfod(&profile->gp, GP_BIRTHDAY, RandomInt(1, 28), RandomInt(1, 12), RandomInt(1940, 1990));
		break;

	case OpBuddyRequest:
		PrintOp(profile, "OpBuddyRequest");

#if 1
		// Pretend this op didn't happen, another will be picked next frame.
		////////////////////////////////////////////////////////////////////
		profile->activeOp = OpNull;
#else
		// Send someone a buddy request.
		////////////////////////////////
		if(profile->numOthers > 0)
			gpSendBuddyRequest(&profile->gp, RandomOther(profile), "STRESS-TEST");
#endif
		break;

	case OpDeleteBuddy:
		PrintOp(profile, "OpDeleteBuddy");

		// Pretend this op didn't happen, another will be picked next frame.
		////////////////////////////////////////////////////////////////////
		profile->activeOp = OpNull;
		break;

	case OpSetStatus1:
	case OpSetStatus2:
	case OpSetStatus3:
	case OpSetStatus4:
	case OpSetStatus5:
		PrintOp(profile, "OpSetStatus");

		// Set the status to some random stuff.
		///////////////////////////////////////
		gpSetStatus(&profile->gp, GP_ONLINE, RandomString(string1), RandomString(string2));
		break;

	case OpSendBuddyMessage:
		PrintOp(profile, "OpSendBuddyMessage");

		// Send someone a message.
		/////////////////////////
		if(profile->numOthers > 0)
			MessageOther(profile, RandomOther(profile));
		break;

	case OpSetInvitable:
		PrintOp(profile, "OpSetInvitable");

		// Get the num games.
		/////////////////////
		num = RandomInt(0, 10);

		// Get the product ids.
		///////////////////////
		for(i = 0 ; i < num ; i++)
			intArray[i] = RandomInt(1, PRODUCT_ID_MAX);

		// Set the games.
		/////////////////
		gpSetInvitableGames(&profile->gp, num, intArray);
		break;

	default:
		assert(0);
		printf("%04d: Tried to start unknown op: %d\n", profile->index, profile->activeOp);
		break;
	}

	// Was this an instant op?
	//////////////////////////
	if(IS_INSTANT_OP(profile->activeOp))
	{
		// End it.
		//////////
		EndOp(profile);
	}
}

/**********************
** PROFILE FUNCTIONS **
**********************/
void ProcessProfile(Profile * profile)
{
	DWORD now = current_time();

	// Is GP initialized?
	/////////////////////
	if(profile->gpInitialized)
	{
		// Do GP processing.
		////////////////////
		gpProcess(&profile->gp);

		// Has a connection attempt been going for 2 minutes?
		/////////////////////////////////////////////////////
		if((profile->activeOp == OpConnect) && ((current_time() - profile->connectTime) > (200 * 60 * 1000)))
		{
			char buffer[256];
			gsi_time ms = (current_time() - profile->connectTime);
			sprintf(buffer, "%04d: XXX Excessive Connect Time: %d.%03ds\n", profile->index, ms / 1000, ms % 1000);
			Log(buffer);
		}

		// Check for disconnect.
		////////////////////////
		if((now > profile->disconnectTime) && ((profile->activeOp != OpConnect) || shuttingDown))
		{
			// Cleanup GP for this profile.
			///////////////////////////////
			if(profile->connected)
			{
				numConnected--;
				gpDisconnect(&profile->gp);
			}
			gpDestroy(&profile->gp);

			// We're no longer initialized/connected.
			/////////////////////////////////////////
			profile->gpInitialized = false;
			numConnections--;
		}
		// Is there no active op?
		/////////////////////////
		else if(!shuttingDown && (profile->activeOp == OpNull))
		{
			// Is it time for a new op?
			///////////////////////////
#if 1
			if(now > profile->nextOpTime)
#else
			if((now > profile->nextOpTime) && (profile->completedOps == 0))
#endif
			{
				// Select a new op.
				///////////////////
				NewOp(profile);

				// Start the op.
				////////////////
				StartOp(profile);
			}
		}
	}
}

Profile * GetUninitializedProfile(void)
{
	int index;

	// Loop until we get an uninitialized (and valid) profile.
	//////////////////////////////////////////////////////////
	do
	{
		// Get an index between 0 and (numProfiles - 1).
		////////////////////////////////////////////////
		index = ((rand() * numProfiles) / (RAND_MAX + 1));
		assert(index >= 0);
		assert(index < numProfiles);
	}
	while(profiles[index].gpInitialized || profiles[index].invalid);

	return &profiles[index];
}

void InitializeProfile(Profile * profile)
{
	GPResult result;

	// Init the profile's GP object.
	////////////////////////////////
	result = gpInitialize(&profile->gp, 0, 0, GP_PARTNERID_GAMESPY);
	if(result != GP_NO_ERROR)
	{
		printf("%04d: Failed to initialize GP for %s@%s\n", profile->index, profile->nick, profile->email);
		return;
	}

	// Set the GP global callbacks.
	///////////////////////////////
	gpSetCallback(&profile->gp, GP_ERROR, ErrorCallback, profile);
	gpSetCallback(&profile->gp, GP_RECV_BUDDY_STATUS, RecvBuddyStatusCallback, profile);
#if 1
	gpSetCallback(&profile->gp, GP_RECV_BUDDY_REQUEST, RecvBuddyRequestCallback, profile);
	gpSetCallback(&profile->gp, GP_RECV_BUDDY_MESSAGE, RecvBuddyMessageCallback, profile);
	gpSetCallback(&profile->gp, GP_RECV_GAME_INVITE, RecvGameInviteCallback, profile);
#endif

	// We're initialized.
	/////////////////////
	profile->gpInitialized = true;

	// Not connected yet.
	/////////////////////
	profile->connected = false;

	// Remember when we tried to connect.
	/////////////////////////////////////
	profile->connectTime = current_time();

	// Haven't checked yet.
	///////////////////////
	profile->invalid = false;

	// When to disconnect.
	//////////////////////
	profile->disconnectTime = current_time();
	profile->disconnectTime += (((rand() * (CONNECT_TIME_MAX - CONNECT_TIME_MIN)) / RAND_MAX) + CONNECT_TIME_MIN);

	// No others yet.
	/////////////////
	profile->numOthers = 0;

	// Op stuff.
	////////////
	profile->activeOp = OpNull;
	profile->nextOpTime = 0;
	profile->completedOps = 0;

	// It's a connection, but we're not connected.
	//////////////////////////////////////////////
	numConnections++;
	totalConnections++;
	highestConnections = max(highestConnections, numConnections);
}

/***************
** VALIDATION **
***************/
void GetUserNicksCallback(GPConnection * connection, GPGetUserNicksResponseArg * arg, ValidationData * validationData)
{
	int i;

	// Copy the result.
	///////////////////
	validationData->result = arg->result;

	// Check for error.
	///////////////////
	if(arg->result != GP_NO_ERROR)
	{
		printf("%04d: gpGetUserNicks failed\n", validationData->profile->index);
		return;
	}

	// Copy the nicks.
	//////////////////
	validationData->numNicks = min(arg->numNicks, VALIDATION_MAX_NICKS);
	for(i = 0 ; i < arg->numNicks ; i++)
		strcpy(validationData->nicks[i], arg->nicks[i]);
	GSI_UNUSED(connection);
}

void NewUserResponse(GPConnection * connection, GPNewUserResponseArg * arg, void * param)
{
	GSI_UNUSED(connection);
	GSI_UNUSED(arg);
	GSI_UNUSED(param);
}

bool Validate(void)
{
	int i;
	GPConnection gp;
	char email[GP_EMAIL_LEN] = "";
	Profile * profile;
	ValidationData validationData;

	validationData.profile = NULL;
	validationData.result = GP_NO_ERROR;
	validationData.numNicks = 0;
	// Set this here so ctrl-c can cancel the validation.
	/////////////////////////////////////////////////////
	startShutdownTime = ULONG_MAX;

	// Init GP.
	///////////
	if(gpInitialize(&gp, 0, 0, GP_PARTNERID_GAMESPY) != GP_NO_ERROR)
	{
		printf("Failed to initialize GP for validation\n");
		return false;
	}
/*
	for(i = 41 ; i <= 1000 ; i++)
	{
		sprintf(email, "gpstress%04d@gamespy.com", i);
		gpNewUser(&gp, "gpstress", email, "gpstress", GP_BLOCKING, NewUserResponse, NULL);
		printf("%d\n", i);
	}
*/
	// Loop through all the profiles.
	/////////////////////////////////
	for(i = 0 ; i < numProfiles ; i++)
	{
		// Check for shutdown.
		//////////////////////
		if(current_time() > startShutdownTime)
			return false;

		// Cache the profile.
		/////////////////////
		profile = &profiles[i];

		// Is the e-mail address different than the previous one?
		/////////////////////////////////////////////////////////
		if(strcmp(email, profile->email) != 0)
		{
			// Copy the e-mail.
			///////////////////
			strcpy(email, profile->email);

			// Put the profile in the validation struct.
			////////////////////////////////////////////
			validationData.profile = profile;

			// Show what we're doing.
			/////////////////////////
			printf("%04d: Getting nicks for: %s\n", profile->index, email);

			// Get the nicks for this e-mail.
			/////////////////////////////////
			gpGetUserNicks(&gp, email, profile->password, GP_BLOCKING, GetUserNicksCallback, &validationData);
		}

		// Only do this if the validation succeeded.
		////////////////////////////////////////////
		if(validationData.result == GP_NO_ERROR)
		{
			int n;
			bool match = false;

			// Loop through the nicks.
			//////////////////////////
			for(n = 0 ; (n < validationData.numNicks) && !match ; n++)
			{
				// Does this nick match the one being validated?
				////////////////////////////////////////////////
				if(strcasecmp(profile->nick, validationData.nicks[n]) == 0)
					match = true;
			}

			// Set invalid based on match.
			//////////////////////////////
			profile->invalid = !match;
		}
		else
		{
			// Probably invalid e-mail.
			///////////////////////////
			profile->invalid = true;
		}

		// Show if we got something invalid.
		////////////////////////////////////
		if(profile->invalid)
		{
			static char buffer[256];
			sprintf(buffer, "%s (%d) : %04d: Invalid profile (%s@%s)\n", profilesFile, (profile->index + 1), profile->index, profile->nick, profile->email);
			printf(buffer);
			OutputDebugString(buffer);
		}
	}

	return true;
}

/*******************
** MAIN FUNCTIONS **
*******************/
#ifdef WIN32
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
	// Start shutting down soon.
	////////////////////////////
	startShutdownTime = 0;

	GSI_UNUSED(dwCtrlType);
	// Handled.
	return TRUE;
}
#endif

bool Initialize(void)
{
	int i;
	Profile * profile;
	DWORD now;
	GSIACResult aResult = GSIACWaiting;

	// Seed the random number generator.
	////////////////////////////////////
	srand(time(NULL));

	// Perform the availability check
	GSIStartAvailableCheck("gmtest");
	while(aResult == GSIACWaiting)
		aResult = GSIAvailableCheckThink();
	if (aResult != GSIACAvailable)
	{
		printf("Backend services are not available.\r\n");
		return false;
	}


#ifdef WIN32
	// Set the ctrl-c handler.
	//////////////////////////
	SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);
#endif

	// Set our own hostname.
	////////////////////////
#ifdef CONNECTION_MANAGER
	if(CONNECTION_MANAGER[0])
		strcpy(GPConnectionManagerHostname, CONNECTION_MANAGER);
#endif

	// Load the profiles.
	/////////////////////
	if(!LoadProfiles())
		return false;

	// Cap off the max connected profiles.
	//////////////////////////////////////
	if(maxConnections > numProfiles)
		maxConnections = numProfiles;

	// Get the current time.
	////////////////////////
	now = current_time();

	// Initalize profile stuff.
	///////////////////////////
	for(i = 0 ; i < numProfiles ; i++)
	{
		// Cache the profile pointer.
		/////////////////////////////.
		profile = &profiles[i];

		// Set its index.
		/////////////////
		profile->index = i;

		// No op yet.
		/////////////
		profile->activeOp = OpNull;

		// Print out a message every 100.
		/////////////////////////////////
#if 0
		if(((i + 1) % 100) == 0)
		{
			printf("%d/%d profiles initialized\n", i + 1, numProfiles);
			msleep(1);
		}
#endif
	}

	return true;
}

void Shutdown(void)
{
	int i;

	// Cleanup all the GP objects.
	//////////////////////////////
	for(i = 0 ; i < numProfiles ; i++)
	{
		// Check if GP was intialized.
		//////////////////////////////
		if(profiles[i].gpInitialized)
		{
			// Destroy the GP object.
			/////////////////////////
			gpDestroy(&profiles[i].gp);

			// Print out a message every 100.
			/////////////////////////////////
			if(((i + 1) % 100) == 0)
			{
				printf("%d/%d profiles destroyed\n", i + 1, numProfiles);
				msleep(1);
			}
		}
	}
}

void StartShutdown(unsigned long now)
{
	Profile * profile;
	int i;

	// Loop through all the profiles.
	/////////////////////////////////
	for(i = 0 ; i < numProfiles ; i++)
	{
		// Cache the profile pointer.
		/////////////////////////////
		profile = &profiles[i];

		// Check if its GP object is initialized.
		/////////////////////////////////////////
		if(profile->gpInitialized)
		{
#if 0
			// Is it not actually connected yet?
			////////////////////////////////////
			if(!profile->connected)
			{
				char buffer[64];

				// How long has it been waiting?
				////////////////////////////////
				sprintf(buffer, "%04d: Waiting %ds for connection attempt\n", profile->index, (current_time() - profile->connectTime) / 1000);
				Log(buffer);
			}
#endif

			// Set when to disconnect, 
			//////////////////////////
			profile->disconnectTime = (now + ((rand() * ((numConnections / SHUTDOWN_DISCONNECTS_PER_SEC) * 1000)) / RAND_MAX));
		}
	}

	// We're shutting down, don't add new connections, etc.
	///////////////////////////////////////////////////////
	shuttingDown = true;
}

void Frame(unsigned long now)
{
	int i;

	// Is it time to connect a new profile?
	///////////////////////////////////////
	if(!shuttingDown && (numConnections < maxConnections) && ((numConnections - numConnected) < MAX_OUTSTANDING_CONNECTIONS))
	{
		Profile * profile;
		unsigned long timeDiff;
		int numConnects;

		// How long since the last connect?
		///////////////////////////////////
		timeDiff = (now - lastConnectTime);

		// How many connects should we do?
		//////////////////////////////////
		numConnects = ((timeDiff * connectsPerSecond) / 1000);
		numConnects = min(numConnects, (MAX_OUTSTANDING_CONNECTIONS - (numConnections - numConnected)));

		// Do the connects.
		///////////////////
		for(i = 0 ; i < numConnects ; i++)
		{
			// Need to check because it can change in the loop.
			///////////////////////////////////////////////////
			if(numConnections < maxConnections)
			{
				// Pick a random profile.
				/////////////////////////
				profile = GetUninitializedProfile();

				// Init it.
				///////////
				InitializeProfile(profile);

				// Remember when we did the connect(s).
				///////////////////////////////////////
				lastConnectTime = now;
			}
		}
	}

	// Loop through all the profiles.
	/////////////////////////////////
	for(i = 0 ; i < numProfiles ; i++)
	{
		// Process the profile.
		///////////////////////
		ProcessProfile(&profiles[i]);
	}
}

void Run(void)
{
	bool done;
	unsigned long now;
#if 0
	unsigned long nextInfoTime = 0;
#endif
	// Get the current time.
	////////////////////////
	now = current_time();

	// Set when to shut down.
	/////////////////////////
	startShutdownTime = (now + runTime);

	// Make up the last connect time.
	/////////////////////////////////
	lastConnectTime = (now - 100);

	// Loop until we're done.
	/////////////////////////
	for(done = false ; !done ; )
	{
		// Get the current time.
		////////////////////////
		now = current_time();

		// Are we shutting down?
		////////////////////////
		if(shuttingDown)
		{
			// Are we done?
			///////////////
			if(numConnections == 0)
			{
				done = true;
			}
		}
		else if(now > startShutdownTime)
		{
			// Time to start shutting down.
			///////////////////////////////
			StartShutdown(now);
		}

#if 0
		// Is it time to print some info?
		/////////////////////////////////
		if(now > nextInfoTime)
		{
			static char buffer[256];

			// Set when to next show info.
			//////////////////////////////
			nextInfoTime = (now + SHOW_INFO_TIME);

			// Show the connection numbers.
			///////////////////////////////
			sprintf(buffer, "INFO: Connections: %d current, %d highest, %d total\n", numConnections, highestConnections, totalConnections);
			Log(buffer);

			// Show the connected numbers.
			//////////////////////////////
			sprintf(buffer, "INFO: Connected:   %d current, %d highest, %d total\n", numConnected, highestConnected, totalConnected);
			Log(buffer);

			// Show that we're shutting down, or when we're shutting down.
			//////////////////////////////////////////////////////////////
			if(shuttingDown)
				sprintf(buffer, "INFO: Shutting down\n");
			else
			{
				unsigned long totalSeconds = ((startShutdownTime - now) / 1000);
				unsigned long hours, minutes, seconds;
				seconds = (totalSeconds % 60);
				minutes = (totalSeconds / 60 % 60);
				hours = (totalSeconds / 60 / 60);
				sprintf(buffer, "INFO: Shut down in %d:%02d:%02d\n", hours, minutes, seconds);
			}
			Log(buffer);
		}
#endif

		// Run a frame.
		///////////////
		Frame(now);

		// Take a break.
		////////////////
		msleep(10);
	}
}

bool ParseArgs(int argc, char ** argv)
{
	int i;
/*
	// Check for the file index.
	////////////////////////////
	if(argc < 2)
	{
		printf("Usage: %s <index 0-9>\n", argv[0]);
		return false;
	}
	sprintf(profilesFile, "stress%d.txt", atoi(argv[1]));
*/
	// Set defaults.
	////////////////
	maxConnections = DEFAULT_MAX_CONNECTED;
	runTime = DEFAULT_RUN_TIME;
	connectsPerSecond = DEFAULT_CONNECTS_PER_SEC;

	// Check args.
	//////////////
	for(i = 1 ; i < argc ; i++)
	{
		// Check for max connected profiles.
		////////////////////////////////////
		if(strcasecmp(argv[i], "-m") == 0)
		{
			if(++i < argc)
			{
				maxConnections = atoi(argv[i]);
				if(maxConnections <= 0)
					maxConnections = DEFAULT_MAX_CONNECTED;
			}
		}
		// Check for how long to run.
		/////////////////////////////
		else if(strcasecmp(argv[i], "-t") == 0)
		{
			if(++i < argc)
			{
				runTime = (atoi(argv[i]) * 1000);
				if(runTime <= 0)
					runTime = DEFAULT_RUN_TIME;
			}
		}
		// Check for connections per second.
		////////////////////////////////////
		else if(strcasecmp(argv[i], "-s") == 0)
		{
			if(++i < argc)
			{
				connectsPerSecond = atoi(argv[i]);
				if(connectsPerSecond <= 0)
					connectsPerSecond = DEFAULT_CONNECTS_PER_SEC;
			}
		}
	}

	return true;
}

int main(int argc, char ** argv)
{
	// Parse args.
	//////////////
	if(!ParseArgs(argc, argv))
		return 1;

	// Initialize.
	//////////////
	if(!Initialize())
		return 1;

#if 0
	// Validate the loaded profiles.
	////////////////////////////////
	if(!Validate())
		return 1;
#endif

	// Do all that stuff.
	/////////////////////
	Run();

	// Cleanup.
	///////////
	Shutdown();

	return 0;
}