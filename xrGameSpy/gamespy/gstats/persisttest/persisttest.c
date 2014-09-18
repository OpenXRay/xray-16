/******
persisttest.c
GameSpy Persistent Storage SDK 
  
Copyright 1999-2007 GameSpy Industries, Inc

******

Please see the GameSpy Persistent Storage SDK for more info

*****/

#include <stdarg.h>
#include "../../common/gsCommon.h"
#include "../../common/gsAvailable.h"
#include "../gpersist.h"

#ifdef UNDER_CE
	void RetailOutputA(CHAR *tszErr, ...);
	#define printf RetailOutputA
#elif defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

/******
We aren't actually hosting a game in this sample, so just set the gameport to 0
*******/
#define MY_GAMEPORT 0

/*******
A CD Key profile is defined by the combination of the nickname and cdkey
Here we have a nick and key to use
********/
#define KEYNICK _T("crt")
#define VALIDCDKEY _T("2dd4-893a-ce85-6411")

/***
A Presence & Messaging account is identified by the profileid and password
These is the profileid for the test@test@gamespy.com account
***/
#define PROFILEID 2446667
#define VALIDPROFILEPASSWORD _T("test")

//a sample data structure we will store on the server
typedef struct playerdata_s
{ 
	char modelnumber;
	int hitpoints;
	unsigned int totaltime;
	char skin[9];
} playerdata_t;

static void PersAuthCallback(int localid, int profileid, int authenticated, gsi_char *errmsg, void *instance)
{
	_tprintf(_T("Auth callback: localid: %d profileid: %d auth: %d err: %s\n"),localid, profileid, authenticated, errmsg);
	/**********
	instance is a pointer to the authcount var. We increment it here to tell the main loop that
	another authentication response came in.
	**********/
	(*(int *)instance)++;

}

static void PersDataCallback(int localid, int profileid, persisttype_t type, int index, int success, time_t modified, char *data, int len, void *instance)
{
	printf("Data get callback: localid: %d profileid: %d success: %d mod: %d len: %d data: %s\n",localid, profileid, success, (int)modified, len, data);
	/*********
	instance is a pointer to the callback counter, increment it
	**********/
	(*(int *)instance)++;
	
	GSI_UNUSED(type);
	GSI_UNUSED(index);
}

static void PlayerDataCallback(int localid, int profileid, persisttype_t type, int index, int success, time_t modified, char *data, int len, void *instance)
{
	playerdata_t pdata;
	/* we copy it off, instead of reading directly, since the data may not be aligned correctly for the SH4/other processors */
	memcpy(&pdata,data, sizeof(playerdata_t));
	if (success)
		printf("playerdata get callback: localid: %d profileid: %d success: %d modified: %d len: %d data:\n\tmodel:%d\n\thitpoints:%d\n\ttotaltime:%d\n\tskin:%s\n",localid, profileid, success, (int)modified, len,pdata.modelnumber,pdata.hitpoints,pdata.totaltime,pdata.skin);
	else
		printf("playerdata get callback: localid: %d profileid: %d success: %d len: %d (FAILED)\n",localid, profileid, success, len);
	/*********
	instance is a pointer to the callback counter, increment it
	**********/
	(*(int *)instance)++;
	
	GSI_UNUSED(type);
	GSI_UNUSED(index);
}


static void PersDataSaveCallback(int localid, int profileid, persisttype_t type, int index, int success, time_t modified, void *instance)
{
	printf("Data save callback: localid: %d profileid: %d success: %d mod: %d\n", localid, profileid, success, (int)modified);
	/*********
	instance is a pointer to the callback counter, increment it
	**********/
	(*(int *)instance)++;
	
	GSI_UNUSED(type);
	GSI_UNUSED(index);
}
static void ProfileCallback(int localid, int profileid, int success, void *instance)
{
	printf("Profile callback: localid: %d profileid: %d success: %d\n",localid, profileid, success);
	/**********
	instance is the player1pid var, lets put the profileid there, or -1 if it wasn't successful
	***********/
	if (success)
		*(int *)instance = profileid;
	else
		*(int *)instance = -1;
}

#ifdef __MWERKS__  // CodeWarrior will warn if not prototyped
	int test_main(int argc, char **argv);
#endif

int test_main(int argc, char **argv)
{
	int result;
	int authcount;
	int callbackct;
	char validate[33], cdkey_hash[33];
	int player1pid;
	GSIACResult ac_result;

	playerdata_t playerdata;

	/*********
	First step, set our game authentication info
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
	Before initializing the connection,
	check that the backend is available.
	*********/
	GSIStartAvailableCheck(_T("gmtest"));
	while((ac_result = GSIAvailableCheckThink()) == GSIACWaiting)
		msleep(5);
	if(ac_result != GSIACAvailable)
	{
		printf("The backend is not available\n");
		return 1;
	}

	/*********
	Next, open the stats connection. This may block for
	a 1-2 seconds, so it should be done before the actual game starts.
	**********/
	result = InitStatsConnection(MY_GAMEPORT);

	if (result != GE_NOERROR)
	{
		printf("InitStatsConnection returned %d\n",result);		
		return 1;
	}

	/********
	Now, lets authenticate two players, one using the a Presence & Messaging
	SDK profileid, and the other using a CDKey SDK CD Key.
	The profile/password and CD Key/nick to use are defined at the top.

	Generally you will only use one method or the other, depending on your game.
	*********/
	/*************
	The first step to authenticate a player is to generate their Authentication token.

	If you are retrieving/setting persistent data on the server, you should first send
	the challenge to the client, have them send back the validation, and then proceed
	with the authentication. In this manner the client never has to send their password
	or CD Key to the server -- only the validation hash.
	If you are retrieving/setting persistent data on the client, it can all be done
	locally, as shown below.
	*************/

	/************
	First, we will authenticate a player using the CD Key system. We pass in the
	plaintext CD Key to GenerateAuth to get the validation token (validate).
	*************/
	GenerateAuth(GetChallenge(NULL),VALIDCDKEY,validate);
	
	/************
	Next, we need to get the hash of the CD Key to along with the validation.
	This can be done easily using GenerateAuth with an empty challenge string
	************/
	GenerateAuth("",VALIDCDKEY,cdkey_hash);

	/***********
	Now, we call PreAuthenticatePlayerCD with the hash and the validation token.
	We pass in a callback that will be called when the operation is complete, along
	with a integer that we will check to know its done.
	We pass in the localid as 1, since we will be authenticating two players before
	checking the return values. By using different localids we can tell them apart.
	************/
	authcount = 0;
	PreAuthenticatePlayerCD(1,KEYNICK,cdkey_hash,validate,PersAuthCallback,&authcount);

	/***********
	While the first authentication is in progress, we'll go ahead and start the second
	one, using a Presence & Messaging SDK profileid / password.
	To generate the new validation token, we'll need to pass in the password for the
	profile we are authenticating.
	Again, if this is done in a client/server setting, with the Persistent Storage
	access being done on the server, and the P&M SDK is used on the client, the
	server will need to send the challenge (GetChallenge(NULL)) to the client, the
	client will create the validation token using GenerateAuth, and send it
	back to the server for use in PreAuthenticatePlayerPM
	***********/
	GenerateAuth(GetChallenge(NULL),VALIDPROFILEPASSWORD,validate);

	/************
	After we get the validation token, we pass it and the profileid of the user
	we are authenticating into PreAuthenticatePlayerPM.
	We pass the same authentication callback as for the first user, but a different
	localid this time.
	************/
	PreAuthenticatePlayerPM(2,PROFILEID,validate,PersAuthCallback,&authcount);

	/***********
	Now that both authentication requests are sent, we need to wait for the results to
	come back. If we were in a game we could continue with the main game loop at this
	point, but since we're not, we'll just loop until we are disconnected or both
	authentications come back
	************/
	while (authcount < 2 && IsStatsConnected())
	{
		PersistThink();
		msleep(10);
	}

	/**********
	Both of the players have now been authenticated.
	When the CD Key player (player 1) was authenticated, the profileid for him
	was returned in the callback, but we didn't save it off. Since we'll need it
	to get/set data for him, lets go ahead and retrieve it using the GetProfileIDFromCD
	function
	**********/
	player1pid = 0;
	GetProfileIDFromCD(1,KEYNICK,cdkey_hash,ProfileCallback,&player1pid);

	/**********
	Again we'll wait until the call completes. We check if player1pid is still 0, since
	it gets set to the profileid in the callback (or -1 if a failure)
	***********/
	while (player1pid == 0 && IsStatsConnected())
	{
		PersistThink();
		msleep(10);
	}

	/**********
	Now we have everything we need to start getting / setting data for the two
	users. First we'll clear both user's public data, to make sure it's empty
	before we continue
	***********/
	callbackct = 0;
	SetPersistData(1,player1pid,pd_public_rw,0,"",0,PersDataSaveCallback,&callbackct);
	SetPersistData(2,PROFILEID,pd_public_rw,0,"",0,PersDataSaveCallback,&callbackct);
	
	/**********
	We'll use the callbackct var (passed in to the callbacks) as a way to easily determine
	when the calls are complete
	***********/
	while (callbackct < 2 && IsStatsConnected())
	{
		PersistThink();
		msleep(10);
	}

	
	/***********
	Lets read the data back, just to prove it's cleared
	************/
	callbackct = 0;
	GetPersistData(1,player1pid,pd_public_rw,0,PersDataCallback,&callbackct);
	GetPersistData(2,PROFILEID,pd_public_rw,0,PersDataCallback,&callbackct);
	while (callbackct < 2 && IsStatsConnected())
	{
		PersistThink();
		msleep(10);
	}

	/************
	Now lets set some REAL data!
	For player1 we will use ascii key\value pairs, for player2 we will store
	binary structures. Note that if you store binary structures, YOU are responsible
	for making sure things like byte order, structure size, and contents do not change
	between versions or platforms of your game that may access the same data
	************/
	callbackct = 0;
	playerdata.hitpoints = 600;
	playerdata.modelnumber = 5;
	playerdata.totaltime = 5000;
	strcpy(playerdata.skin,"darkstar");
	SetPersistData(1,player1pid,pd_public_rw,0,(char *)&playerdata,sizeof(playerdata_t),PersDataSaveCallback,&callbackct);
	SetPersistDataValues(2,PROFILEID,pd_public_rw,0,_T("\\key1\\value1\\key2\\value2\\key3\\value3"),PersDataSaveCallback,&callbackct);
	while (callbackct < 2 && IsStatsConnected())
	{
		PersistThink();
		msleep(10);
	}

	/***********
	Lets read the data back, to see what's there!
	We'll use a custom callback for player1 to print out the playerdata struct that's read back.
	************/
	callbackct = 0;
	GetPersistData(1,player1pid,pd_public_rw,0,PlayerDataCallback,&callbackct);
	GetPersistData(2,PROFILEID,pd_public_rw,0,PersDataCallback,&callbackct);
	while (callbackct < 2 && IsStatsConnected())
	{
		PersistThink();
		msleep(10);
	}

	/***********
	Now, lets manipulate player2's data a bit using the SetPersistDataValues functions.
	We'll also print it at each stage, so you can see the changes as they occur

	First, we'll update the values of key1 and key3 (but not key2) and add keys 4 and 5.
	Your keys don't have to be numbered like this, this is just an example
	************/
	callbackct = 0;
	SetPersistDataValues(2,PROFILEID,pd_public_rw,0,_T("\\key1\\valueA\\key3\\valueB\\key4\\value400\\key5\\value500"),
		PersDataSaveCallback,&callbackct);
	GetPersistData(2,PROFILEID,pd_public_rw,0,PersDataCallback,&callbackct);

	/***********
	Now we'll clear key 2, and update keys 3 and 4, and add two other keys
	************/
	SetPersistDataValues(2,PROFILEID,pd_public_rw,0,_T("\\key2\\\\key3\\valueC\\key4\\value401\\mykeyname\\mykeyvalue\\otherkey\\othervalue"),
		PersDataSaveCallback,&callbackct);
	GetPersistData(2,PROFILEID,pd_public_rw,0,PersDataCallback,&callbackct);

	/***********
	Finally, lets see how we can use GetPersistDataValues to only return a subset
	of the keys that are stored. For example, we will only return keys 1,2,3,and 4
	The order you list them in does not matter
	************/
	GetPersistDataValues(2,PROFILEID,pd_public_rw,0,_T("\\key3\\key2\\key4\\key1"),PersDataCallback,&callbackct);

	/***********
	We've now sent off 5 requests without actually checking the results. The results are already coming in,
	we just need to think a while to get them all
	************/
	while (callbackct < 5 && IsStatsConnected())
	{
		PersistThink();
		msleep(100);
	}
	
	/**********
	When we're done we'll close off the connection
	***********/	
	CloseStatsConnection();

	GSI_UNUSED(argc);
	GSI_UNUSED(argv);

	return 0;
}
