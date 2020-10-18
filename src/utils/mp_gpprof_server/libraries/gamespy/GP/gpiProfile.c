/*
gpiProfile.c
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4312) //lines: 1164, 1186, 1265
#pragma warning(disable: 4311) //lines: 1231
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS


//INCLUDES
//////////
#include "gpi.h"

//DEFINES
/////////
#define GPI_PROFILE_GROW_SIZE        16
#define GPI_PROFILE_CACHE_VERSION    2

// GLOBALS
//////////
static char GPIInfoCacheFilename[FILENAME_MAX + 1] = "gp.info";

//FUNCTIONS
///////////
static int
gpiProfilesTableHash(
  const void *arg,
  int numBuckets
)
{
	const GPIProfile * profile = (const GPIProfile *)arg;
	return (profile->profileId % numBuckets);
}

static int
gpiProfilesTableCompare(
  const void * arg1,
  const void * arg2
)
{
  const GPIProfile * profile1 = (const GPIProfile *)arg1;
  const GPIProfile * profile2 = (const GPIProfile *)arg2;
	return (profile1->profileId - profile2->profileId);
}

static void
gpiProfilesTableFree(
  void *arg
)
{
	GPIProfile * profile = (GPIProfile *)arg;
	if(profile->buddyStatus)
	{
		freeclear(profile->buddyStatus->statusString);
		freeclear(profile->buddyStatus->locationString);
		freeclear(profile->buddyStatus);
	}
	if (profile->buddyStatusInfo)
	{
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
	gpiFreeInfoCache(profile);
	freeclear(profile->authSig);
	freeclear(profile->peerSig);
}

GPIBool
gpiInitProfiles(
  GPConnection * connection
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	iconnection->profileList.numBuddies = 0;
    iconnection->profileList.numBlocked = 0;
	iconnection->profileList.num = 0;
	iconnection->profileList.profileTable = TableNew(
		sizeof(GPIProfile),
		32,
		gpiProfilesTableHash,
		gpiProfilesTableCompare,
		gpiProfilesTableFree);
	if(!iconnection->profileList.profileTable)
		return GPIFalse;

	return GPITrue;
}

#ifndef NOFILE

static GPResult
gpiOpenDiskProfiles(
  GPConnection * connection,
  GPIBool write,
  GPIBool * failed
)
{
	FILE * fp = NULL;
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Open the file.
	/////////////////
	if(write)
		fp = fopen(GPIInfoCacheFilename, "wt");
	else
		fp = fopen(GPIInfoCacheFilename, "rt");
	if(fp == NULL)
	{
		*failed = GPITrue;
		return GP_NO_ERROR;
	}

	// Excellent.
	/////////////
	iconnection->diskCache = fp;
	*failed = GPIFalse;

	return GP_NO_ERROR;
	
	GSI_UNUSED(write);
}

static void
gpiCloseDiskProfiles(
  GPConnection * connection
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Close the file.
	//////////////////
	fclose(iconnection->diskCache);
	iconnection->diskCache = NULL;

	return;
}

static GPResult
gpiReadDiskKeyValue(
  GPConnection * connection,
  GPIBool * failed,
  char key[512],
  char value[512]
)
{
	int c;
	FILE * fp;
	GPIConnection * iconnection = (GPIConnection*)*connection;
	int i;

	// Grab the file pointer.
	/////////////////////////
	fp = iconnection->diskCache;

	// Read the key.
	////////////////
	i = 0;
	do
	{
		if(i == 512)
		{
			*failed = GPITrue;
			return GP_NO_ERROR;
		}
		c = fgetc(fp);
		if((c == EOF) || (c == '\n'))
		{
			*failed = GPITrue;
			return GP_NO_ERROR;
		}
		key[i++] = (char)c;
	}
	while(c != '=');
	key[--i] = '\0';

	// Check for no key.
	////////////////////
	if(i == 0)
	{
		*failed = GPITrue;
		return GP_NO_ERROR;
	}

	// Read the value.
	//////////////////
	i = 0;
	do
	{
		if(i == 512)
		{
			*failed = GPITrue;
			return GP_NO_ERROR;
		}
		c = fgetc(fp);
		if(c == EOF)
		{
			c = '\n';
		}
		value[i++] = (char)c;
	}
	while(c != '\n');
	value[--i] = '\0';

	// Done.
	////////
	*failed = GPIFalse;
	return GP_NO_ERROR;
	
	GSI_UNUSED(value);
	GSI_UNUSED(key);
	GSI_UNUSED(connection);
}

static GPResult
gpiReadDiskProfile(
  GPConnection * connection,
  GPIBool * failedOut
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	FILE * fp;
	GPIProfile profile;
	int c;
	int rcode;
	GPIBool failed;
	char key[256];
	char value[256];
	GPIInfoCache infoCache;
	GPIBool valid = GPIFalse;
	GPIProfile * pProfile;
	char nick[GP_NICK_LEN];
	char uniquenick[GP_UNIQUENICK_LEN];
	char email[GP_EMAIL_LEN];
	char firstname[GP_FIRSTNAME_LEN];
	char lastname[GP_LASTNAME_LEN];
	char homepage[GP_HOMEPAGE_LEN];
	char aimname[GP_AIMNAME_LEN];

	// Grab the file pointer.
	/////////////////////////
	fp = iconnection->diskCache;

	// Clear the temp profile.
	//////////////////////////
	memset(&profile, 0, sizeof(GPIProfile));

	// Clear the temp cache.
	////////////////////////
	memset(&infoCache, 0, sizeof(GPIInfoCache));
	infoCache.nick = nick;
	infoCache.uniquenick = uniquenick;
	infoCache.email = email;
	infoCache.firstname = firstname;
	infoCache.lastname = lastname;
	infoCache.homepage = homepage;
	infoCache.aimname = aimname;
	nick[0] = '\0';
	uniquenick[0] = '\0';
	email[0] = '\0';
	firstname[0] = '\0';
	lastname[0] = '\0';
	homepage[0] = '\0';
	aimname[0] = '\0';

	// Read until we hit a [.
	/////////////////////////
	do
	{
		c = fgetc(fp);
		if(c == EOF)
		{
			*failedOut = GPITrue;
			return GP_NO_ERROR;
		}
	}
	while(c != '[');

	// Grab the profileid.
	//////////////////////
	rcode = fscanf(fp, "%d]\n", &profile.profileId);
	if(rcode != 1)
	{
		*failedOut = GPITrue;
		return GP_NO_ERROR;
	}

	gsDebugFormat(GSIDebugCat_GP, GSIDebugType_File, GSIDebugLevel_Comment,
		"Reading profile %d from disk cache:\n", profile.profileId);

	// Read key/value pairs.
	////////////////////////
	do
	{
		CHECK_RESULT(gpiReadDiskKeyValue(connection, &failed, key, value));
		if(!failed)
		{
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_File, GSIDebugLevel_Comment,
				"%d: %s=%s\n", profile.profileId, key, value);

			// Set the data based on the key.
			/////////////////////////////////
			if(strcmp(key, "userid") == 0)
			{
				profile.userId = atoi(value);
			}
			else if(strcmp(key, "nick") == 0)
			{
				strzcpy(infoCache.nick, value, GP_NICK_LEN);
			}
			else if(strcmp(key, "uniquenick") == 0)
			{
				strzcpy(infoCache.uniquenick, value, GP_UNIQUENICK_LEN);
			}
			else if(strcmp(key, "email") == 0)
			{
				strzcpy(infoCache.email, value, GP_EMAIL_LEN);
			}
			else if(strcmp(key, "firstname") == 0)
			{
				strzcpy(infoCache.firstname, value, GP_FIRSTNAME_LEN);
			}
			else if(strcmp(key, "lastname") == 0)
			{
				strzcpy(infoCache.lastname, value, GP_LASTNAME_LEN);
			}
			else if(strcmp(key, "homepage") == 0)
			{
				strzcpy(infoCache.homepage, value, GP_HOMEPAGE_LEN);
			}
			else if(strcmp(key, "icquin") == 0)
			{
				infoCache.icquin = atoi(value);
			}
			else if(strcmp(key, "zipcode") == 0)
			{
				strzcpy(infoCache.zipcode, value, GP_ZIPCODE_LEN);
			}
			else if(strcmp(key, "countrycode") == 0)
			{
				strzcpy(infoCache.countrycode, value, GP_COUNTRYCODE_LEN);
			}
			else if(strcmp(key, "birthday") == 0)
			{
				infoCache.birthday = atoi(value);
			}
			else if(strcmp(key, "birthmonth") == 0)
			{
				infoCache.birthmonth = atoi(value);
			}
			else if(strcmp(key, "birthyear") == 0)
			{
				infoCache.birthyear = atoi(value);
			}
			else if(strcmp(key, "sex") == 0)
			{
				if(toupper(value[0]) == 'M')
					infoCache.sex = GP_MALE;
				else if(toupper(value[0]) == 'F')
					infoCache.sex = GP_FEMALE;
				else
					infoCache.sex = GP_PAT;
			}
			else if(strcmp(key, "publicmask") == 0)
			{
				infoCache.publicmask = atoi(value);
			}
			else if(strcmp(key, "aimname") == 0)
			{
				strzcpy(infoCache.aimname, value, GP_AIMNAME_LEN);
			}
			else if(strcmp(key, "pic") == 0)
			{
				infoCache.pic = atoi(value);
			}
			else if(strcmp(key, "occupationid") == 0)
			{
				infoCache.occupationid = atoi(value);
			}
			else if(strcmp(key, "industryid") == 0)
			{
				infoCache.industryid = atoi(value);
			}
			else if(strcmp(key, "incomeid") == 0)
			{
				infoCache.incomeid = atoi(value);
			}
			else if(strcmp(key, "marriedid") == 0)
			{
				infoCache.marriedid = atoi(value);
			}
			else if(strcmp(key, "childcount") == 0)
			{
				infoCache.childcount = atoi(value);
			}
			else if(strcmp(key, "interests1") == 0)
			{
				infoCache.interests1 = atoi(value);
			}
			else if(strcmp(key, "ownership1") == 0)
			{
				infoCache.ownership1 = atoi(value);
			}
			else if(strcmp(key, "conntypeid") == 0)
			{
				infoCache.conntypeid = atoi(value);
			}
			else if(strcmp(key, "valid") == 0)
			{
				valid = (GPIBool)atoi(value);
			}
			else
			{
				gsDebugFormat(GSIDebugCat_GP, GSIDebugType_File, GSIDebugLevel_HotError,
					"Unrecognized profile key: %s=%s\n", key, value);
			}
		}
	}
	while(!failed);

	// Create a new profile.
	////////////////////////
	pProfile = gpiProfileListAdd(connection, profile.profileId);
	if(pProfile)
	{
		// Copy the profile we've set up into the list.
		///////////////////////////////////////////////
		*pProfile = profile;

		// Copy the info if valid.
		//////////////////////////
		if(valid)
			gpiSetInfoCache(connection, pProfile, &infoCache);
	}
	*failedOut = GPIFalse;
	return GP_NO_ERROR;
	
	GSI_UNUSED(connection);
}

static GPResult
gpiReadVersion(
  const GPConnection * connection,
  int * version
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	FILE * fp;

	// Grab the file pointer.
	/////////////////////////
	fp = iconnection->diskCache;

	// Read the version.
	////////////////////
	if(fscanf(fp, "%d\n", version) != 1)
		*version = 0;

	return GP_NO_ERROR;
	
	GSI_UNUSED(connection);
	GSI_UNUSED(version);
}

static void
gpiWriteVersion(
  GPConnection * connection,
  int version
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	FILE * fp;

	// Grab the file pointer.
	/////////////////////////
	fp = iconnection->diskCache;

	// Write the version.
	/////////////////////
	fprintf(fp, "%d\n", version);

	GSI_UNUSED(connection);
	GSI_UNUSED(version);
}

GPResult
gpiLoadDiskProfiles(
  GPConnection * connection
)
{
	GPIBool failed;
	int count;
	int version = 0;

	// Open the disk cache.
	///////////////////////
	CHECK_RESULT(gpiOpenDiskProfiles(connection, GPIFalse, &failed));
	if(failed)
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_File, GSIDebugLevel_HotError,
			"Failed to open the disk cache file.\n");
		return GP_NO_ERROR;
	}

	// Check the version.
	//////////////////////
	CHECK_RESULT(gpiReadVersion(connection, &version));
	if(version == GPI_PROFILE_CACHE_VERSION)
	{
		// Read profiles.
		/////////////////
		count = 0;
		do
		{
			CHECK_RESULT(gpiReadDiskProfile(connection, &failed));
			if(!failed)
				count++;
		}
		while(!failed);
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_File, GSIDebugLevel_Comment,
			"Loaded %d profiles from disk cache.\n", count);
	}

	// Close the cache.
	///////////////////
	gpiCloseDiskProfiles(connection);

	return GP_NO_ERROR;
}

static GPIBool
gpiSaveDiskProfile(
  GPConnection * connection,
  GPIProfile * profile,
  void * data
)
{
	FILE * fp;
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Grab the file pointer.
	/////////////////////////
	fp = iconnection->diskCache;

	// Write the profile id.
	////////////////////////
	fprintf(fp, "[%d]\n", profile->profileId);

	// Write the userid if not 0.
	/////////////////////////////
	if(profile->userId != 0)
		fprintf(fp, "userid=%d\n", profile->userId);

	// Is the cache valid?
	//////////////////////
	if(profile->cache)
	{
		fprintf(fp, "valid=1\n");

		fprintf(fp, "nick=%s\n", profile->cache->nick);
		fprintf(fp, "uniquenick=%s\n", profile->cache->uniquenick);
		fprintf(fp, "email=%s\n", profile->cache->email);
		fprintf(fp, "firstname=%s\n", profile->cache->firstname);
		fprintf(fp, "lastname=%s\n", profile->cache->lastname);
		fprintf(fp, "homepage=%s\n", profile->cache->homepage);
		fprintf(fp, "icquin=%d\n", profile->cache->icquin);
		fprintf(fp, "zipcode=%s\n", profile->cache->zipcode);
		fprintf(fp, "countrycode=%s\n", profile->cache->countrycode);
		fprintf(fp, "birthday=%d\n", profile->cache->birthday);
		fprintf(fp, "birthmonth=%d\n", profile->cache->birthmonth);
		fprintf(fp, "birthyear=%d\n", profile->cache->birthyear);
		if(profile->cache->sex == GP_MALE)
			fprintf(fp, "sex=Male\n");
		if(profile->cache->sex == GP_FEMALE)
			fprintf(fp, "sex=Female\n");
		if(profile->cache->sex == GP_PAT)
			fprintf(fp, "sex=Pat\n");
		fprintf(fp, "publicmask=%d\n", profile->cache->publicmask);
		fprintf(fp, "aimname=%s\n", profile->cache->aimname);
		fprintf(fp, "pic=%d\n", profile->cache->pic);
		fprintf(fp, "occupationid=%d\n", profile->cache->occupationid);
		fprintf(fp, "industryid=%d\n", profile->cache->industryid);
		fprintf(fp, "incomeid=%d\n", profile->cache->incomeid);
		fprintf(fp, "marriedid=%d\n", profile->cache->marriedid);
		fprintf(fp, "childcount=%d\n", profile->cache->childcount);
		fprintf(fp, "interests1=%d\n", profile->cache->interests1);
		fprintf(fp, "ownership1=%d\n", profile->cache->ownership1);
		fprintf(fp, "conntypeid=%d\n", profile->cache->conntypeid);
	}

	// End this profile.
	////////////////////
	fprintf(fp, "\n");

	GSI_UNUSED(data);
	GSI_UNUSED(connection);
	GSI_UNUSED(profile);

	return GPITrue;
}

GPResult
gpiSaveDiskProfiles(
  GPConnection * connection
)
{
	GPIBool failed;

	// Open the disk cache.
	///////////////////////
	CHECK_RESULT(gpiOpenDiskProfiles(connection, GPITrue, &failed));
	if(failed)
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_File, GSIDebugLevel_HotError,
			"Failed to open the disk cache file.\n");
		return GP_NO_ERROR;
	}

	// Write the version.
	/////////////////////
	gpiWriteVersion(connection, GPI_PROFILE_CACHE_VERSION);

	// Save profiles.
	/////////////////
	gpiProfileMap(connection, gpiSaveDiskProfile, NULL);

	// Close the cache.
	///////////////////
	gpiCloseDiskProfiles(connection);

	return GP_NO_ERROR;
}

#endif

GPResult
gpiProcessNewProfile(
  GPConnection * connection,
  GPIOperation * operation,
  const char * input
)
{
	char buffer[16];
	int pid;
	GPICallback callback;

	// Check for an error.
	//////////////////////
	if(gpiCheckForError(connection, input, GPITrue))
		return GP_SERVER_ERROR;

	// This should be \npr\.
	////////////////////////
	if(strncmp(input, "\\npr\\", 5) != 0)
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");

	// Get the profile id.
	//////////////////////
	if(!gpiValueForKey(input, "\\profileid\\", buffer, sizeof(buffer)))
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");
	pid = atoi(buffer);

	// Call the callback.
	/////////////////////
	callback = operation->callback;
	if(callback.callback != NULL)
	{
		GPNewProfileResponseArg * arg;
		arg = (GPNewProfileResponseArg *)gsimalloc(sizeof(GPNewProfileResponseArg));
		if(arg == NULL)
			Error(connection, GP_MEMORY_ERROR, "Out of memory.");
		
		arg->profile = (GPProfile)pid;
		arg->result = GP_NO_ERROR;

		CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
	}

	// Remove the operation.
	////////////////////////
	gpiRemoveOperation(connection, operation);

	return GP_NO_ERROR;
}

GPIProfile *
gpiProfileListAdd(
  GPConnection * connection,
  int id
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIProfileList * profileList = &iconnection->profileList;
	GPIProfile profile;
	GPIProfile * pProfile;

	assert(id > 0);

	// Check the parameters. 2000.02.14.JED was not checked in release build.
	/////////////////////////////////////////////////////////////////////////
	if(id <= 0)
		return NULL;

	// Check if this id is already in the list.
	///////////////////////////////////////////
	if(gpiGetProfile(connection, (GPProfile)id, &pProfile))
		return pProfile;

	// Setup the new profile.
	/////////////////////////
	memset(&profile, 0, sizeof(GPIProfile));
	profile.profileId = id;
	profile.userId = 0;
	profile.cache = NULL;
	profile.authSig = NULL;
	profile.peerSig = NULL;
	profile.requestCount = 0;

	// Add it to the table.
	///////////////////////
	TableEnter(profileList->profileTable, &profile);

	// One new one.
	///////////////
	profileList->num++;

	// Get a pointer to the profile.
	////////////////////////////////
	if(gpiGetProfile(connection, (GPProfile)id, &pProfile))
		return pProfile;

	// It wasn't added.
	///////////////////
	return NULL;
}

GPIBool
gpiGetProfile(
  GPConnection * connection,
  GPProfile profileid,
  GPIProfile ** pProfile
)
{
	GPIProfile * profile;
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIProfile profileTemp;

	profileTemp.profileId = profileid;
	profile = (GPIProfile *)TableLookup(iconnection->profileList.profileTable, &profileTemp);
	if(pProfile)
		*pProfile = profile;

	return ((profile != NULL) ? GPITrue:GPIFalse);
}

GPResult
gpiNewProfile(
  GPConnection * connection,
  const char nick[31],
  GPEnum replace,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIOperation * operation;
	GPResult result;
	char buffer[31];

	// Error check.
	///////////////
	if(nick == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid nick.");
	if((replace != GP_REPLACE) && (replace != GP_DONT_REPLACE))
		Error(connection, GP_PARAMETER_ERROR, "Invalid replace.");

	// Create a new operation.
	//////////////////////////
	CHECK_RESULT(gpiAddOperation(connection, GPI_NEW_PROFILE, NULL, &operation, blocking, callback, param));

	// Send the request.
	////////////////////
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\newprofile\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\nick\\");
	strzcpy(buffer, nick, GP_NICK_LEN);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, buffer);
	if(replace == GP_REPLACE)
	{
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\replace\\");
		gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, 1);
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\oldnick\\");
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, iconnection->nick);
	}
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\id\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, operation->id);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
/*
	if(string.result != GP_NO_ERROR)
	{
		gpiRemoveOperation(connection, operation);
		return string.result;
	}
*/
	// Process it if blocking.
	//////////////////////////
	if(operation->blocking)
	{
		result = gpiProcess(connection, operation->id);
		if(result != GP_NO_ERROR)
		{
			gpiRemoveOperation(connection, operation);
			return result;
		}
	}

	return GP_NO_ERROR;
}

GPResult gpiProcessDeleteProfle
(
  GPConnection * connection,
  GPIOperation * operation,
  const char * input
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPICallback callback;

	// Check for an error.
	//////////////////////
	if(gpiCheckForError(connection, input, GPITrue))
		return GP_SERVER_ERROR;

	// This should be \dpr\.
	////////////////////////
	if(strncmp(input, "\\dpr\\", 5) != 0)
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");

	
	// Call the callback.
	/////////////////////
	callback = operation->callback;
	if(callback.callback != NULL)
	{
		GPDeleteProfileResponseArg * arg;
		arg = (GPDeleteProfileResponseArg *)gsimalloc(sizeof(GPDeleteProfileResponseArg));
		if(arg == NULL)
			Error(connection, GP_MEMORY_ERROR, "Out of memory.");
		
		arg->profile = iconnection->profileid;
		arg->result = GP_NO_ERROR;

		CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
	}

	// Remove the operation.
	////////////////////////
	gpiRemoveOperation(connection, operation);

	return GP_NO_ERROR;
}


GPResult
gpiDeleteProfile(
  GPConnection * connection,
  GPCallback callback,
  void *param
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIOperation * operation;
	GPResult result;

	CHECK_RESULT(gpiAddOperation(connection, GPI_DELETE_PROFILE, NULL, &operation, GP_BLOCKING, callback, param));
	// Send the message.
	////////////////////
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\delprofile\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\id\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, operation->id);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

	// Remove the profile object.
	/////////////////////////////
	gpiRemoveProfileByID(connection, iconnection->profileid);

	// Disconnect the connection.
	// PANTS|05.16.00
	/////////////////////////////
	iconnection->connectState = GPI_PROFILE_DELETING;	
	result = gpiProcess(connection, operation->id);
	if (result != GP_NO_ERROR)
	{
		gpiRemoveOperation(connection, operation);
		return result;
	}

	gpiDisconnect(connection, GPIFalse);
	return GP_NO_ERROR;
}

void
gpiRemoveProfileByID(
  GPConnection * connection,
  int profileid
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIProfile * profile;

	if(gpiGetProfile(connection, (GPProfile)profileid, &profile))
		TableRemove(iconnection->profileList.profileTable, profile);
}

void
gpiRemoveProfile(
  GPConnection * connection,
  GPIProfile * profile
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	TableRemove(iconnection->profileList.profileTable, profile);
}

typedef struct GPIFindProfileByUserData
{
	char * nick;
	char * email;
	GPIProfile ** profile;
	GPIBool found;
} GPIFindProfileByUserData;

static GPIBool gpiCheckProfileForUser(
  GPConnection * connection,
  GPIProfile * profile,
  void *udata
)
{
	GPIFindProfileByUserData * data = (GPIFindProfileByUserData *)udata;

	GSI_UNUSED(connection);

	// Check for a valid cache.
	///////////////////////////
	if(profile->cache)
	{
		// Check the nick and email.
		////////////////////////////
		if((strcmp(data->nick, profile->cache->nick) == 0) && (strcmp(data->email, profile->cache->email) == 0))
		{
			// Found it.
			////////////
			*data->profile = profile;
			data->found = GPITrue;
			return GPIFalse;
		}
	}

	return GPITrue;
}

GPResult
gpiFindProfileByUser(
  GPConnection * connection,
  char nick[GP_NICK_LEN],
  char email[GP_EMAIL_LEN],
  GPIProfile ** profile
)
{
	GPIFindProfileByUserData data;

	data.nick = nick;
	data.email = email;
	data.profile = profile;
	data.found = GPIFalse;

	gpiProfileMap(connection, gpiCheckProfileForUser, &data);

	if(!data.found)
		*profile = NULL;

	return GP_NO_ERROR;
}

typedef struct GPIProfileMapData
{
	GPConnection * connection;
	gpiProfileMapFunc func;
	void * data;
} GPIProfileMapData;

static int
gpiProfileMapCallback(
  void *arg,
  void *udata
)
{
	GPIProfile * profile = (GPIProfile *)arg;
	GPIProfileMapData * data = (GPIProfileMapData *)udata;
	return (int)data->func(data->connection, profile, data->data);
}

GPIBool
gpiProfileMap(
  GPConnection * connection,
  gpiProfileMapFunc func,
  void * data
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIProfileMapData mapData;

	mapData.connection = connection;
	mapData.func = func;
	mapData.data = data;

	return (!TableMapSafe2(iconnection->profileList.profileTable, gpiProfileMapCallback, &mapData)) ? GPITrue:GPIFalse;
}

typedef struct GPIFindProfileData
{
	int index;
	GPIProfile * profile;
} GPIFindProfileData;

static GPIBool
gpiCheckForBuddy(
  GPConnection * connection,
  GPIProfile * profile,
  void *udata
)
{
	GPIFindProfileData * data = (GPIFindProfileData *)udata;
	if (profile->buddyStatus && (data->index == profile->buddyStatus->buddyIndex))
	{
		data->profile = profile;
		return GPIFalse;
	}
	else if (profile->buddyStatusInfo && (data->index == profile->buddyStatusInfo->buddyIndex))
	{
		data->profile = profile;
		return GPIFalse;
	}

	GSI_UNUSED(connection);

	return GPITrue;
}

GPIProfile *
gpiFindBuddy(
  GPConnection * connection,
  int buddyIndex
)
{
	GPIFindProfileData data;

	data.index = buddyIndex;
	data.profile = NULL;

	gpiProfileMap(connection, gpiCheckForBuddy, &data);

	return data.profile;
}

void gpiRemoveBuddyStatus(GPIBuddyStatus *buddyStatus)
{
	GS_ASSERT(buddyStatus);
	
	freeclear(buddyStatus->locationString);
	freeclear(buddyStatus->statusString);
	freeclear(buddyStatus);
}

void gpiRemoveBuddyStatusInfo(GPIBuddyStatusInfo *buddyStatusInfo)
{
	GS_ASSERT(buddyStatusInfo);
		
	freeclear(buddyStatusInfo->richStatus);
	freeclear(buddyStatusInfo->gameType);
	freeclear(buddyStatusInfo->gameVariant);
	freeclear(buddyStatusInfo->gameMapName);
	ArrayFree(buddyStatusInfo->extendedInfoKeys);
	buddyStatusInfo->extendedInfoKeys = NULL;
	freeclear(buddyStatusInfo);
}

GPIBool
gpiCanFreeProfile(
  GPIProfile * profile
)
{
	return ((profile && !profile->cache && !profile->buddyStatus && !profile->buddyStatusInfo && !profile->peerSig && !profile->authSig && !profile->blocked)) ? GPITrue:GPIFalse;
}

void gpiSetInfoCacheFilename(
  const char filename[FILENAME_MAX + 1]
)
{
	strzcpy(GPIInfoCacheFilename, filename, sizeof(GPIInfoCacheFilename));
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Blocked List functionality
GPResult
gpiAddToBlockedList(
  GPConnection * connection,
  int profileid
)
{
    GPIConnection * iconnection = (GPIConnection*)*connection;
    GPIProfile * profile;
    int index;

    // Check if profile already in list
    ///////////////////////////////////
    if(!gpiGetProfile(connection, profileid, &profile))
    {
        // It's not, so Add profile
        /////////////////////////////////////
        profile = gpiProfileListAdd(connection, profileid);
        if(!profile)
            Error(connection, GP_MEMORY_ERROR, "Out of memory.");
    }
    else
    {
        // Already in the list, nix the buddy status if it's a buddy
        ////////////////////////////////////////////////////////////
        if (profile->buddyStatus)
        {
            index = profile->buddyStatus->buddyIndex;
            freeclear(profile->buddyStatus->statusString);
            freeclear(profile->buddyStatus->locationString);
            freeclear(profile->buddyStatus);
            iconnection->profileList.numBuddies--;
            assert(iconnection->profileList.numBuddies >= 0);
#ifndef _PS2
            gpiProfileMap(connection, gpiFixBuddyIndices, (void *)(unsigned long)index);
#else
            gpiProfileMap(connection, gpiFixBuddyIndices, (void *)index);
#endif
        }
        if (profile->buddyStatusInfo)
        {
            index = profile->buddyStatusInfo->buddyIndex;
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

            iconnection->profileList.numBuddies--;
            assert(iconnection->profileList.numBuddies >= 0);
#ifndef _PS2
            gpiProfileMap(connection, gpiFixBuddyIndices, (void *)(unsigned long)index);
#else
            gpiProfileMap(connection, gpiFixBuddyIndices, (void *)index);
#endif
        }
    }

    // Set profile as blocked if it wasn't already
    //////////////////////////////////////////////
    if (!profile->blocked)
    {
        profile->blocked = gsi_true;
        profile->blockIndex = iconnection->profileList.numBlocked++;

#ifdef _PS3
        // Only perform if profile isn't already locally blocked and the sync isnt taking place
        ///////////////////////////////////////////////////////////////////////////////////////
        if (!iconnection->npSyncLock)
            gpiAddToNpBlockList(connection, profileid);
#endif
    }

    // NOTE: There is no callback for this function simply in order to remain consistent
    // with the existing GP structure. If an error occurs, it gets passed to the error callback
    // else its considered the request was processed correctly.

    // Add to outgoing buffer - have server handle error check if block already exists (consistency)
    /////////////////////////////////////////////////////////////////////////////////////////////////
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\addblock\\\\sesskey\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\profileid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, profileid);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

    return GP_NO_ERROR;
}

static GPIBool
gpiFixBlockIndices(
  GPConnection * connection,
  GPIProfile * profile,
  void * data
)
{
#ifndef _PS2
    int baseIndex = (int)(unsigned long)data;
#else
    int baseIndex = (int)data;
#endif

    GSI_UNUSED(connection);

    if(profile->blocked && (profile->blockIndex > baseIndex))
        profile->blockIndex--;
    return GPITrue;
}

GPResult
gpiRemoveFromBlockedList(
  GPConnection * connection,
  int profileid
)
{
    GPIConnection * iconnection = (GPIConnection*)*connection;
    GPIProfile * profile;
    int index;

    // Grab the profile if already in list
    //////////////////////////////////////
    if(gpiGetProfile(connection, profileid, &profile) && profile->blocked)
    {
        // Set profile as non-blocked 
        /////////////////////////////
        profile->blocked = gsi_false;

        iconnection->profileList.numBlocked--;
        index = profile->blockIndex;

#ifndef _PS2
        gpiProfileMap(connection, gpiFixBlockIndices, (void *)(unsigned long)index);
#else
        gpiProfileMap(connection, gpiFixBlockIndices, (void *)index);
#endif
    }
   
    // NOTE: There is no callback for this function simply in order to remain consistent
    // with the existing GP structure. If an error occurs, it gets passed to the error callback
    // else its considered the request was processed correctly.

    // Add to outgoing buffer - have server handle error check if block already removed (consistency)
    /////////////////////////////////////////////////////////////////////////////////////////////////
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\removeblock\\\\sesskey\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\profileid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, profileid);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

    return GP_NO_ERROR;
}

static GPIBool
gpiCheckForBlock(
  GPConnection * connection,
  GPIProfile * profile,
  void *udata
)
{
    GPIFindProfileData * data = (GPIFindProfileData *)udata;
    if (profile->blocked && data->index == profile->blockIndex)
    {
        data->profile = profile;
        return GPIFalse;
    }

    GSI_UNUSED(connection);

    return GPITrue;
}

GPIProfile *
gpiFindBlockedProfile(
  GPConnection * connection,
  int blockIndex
)
{
    GPIFindProfileData data;

    data.index = blockIndex;
    data.profile = NULL;

    gpiProfileMap(connection, gpiCheckForBlock, &data);

    return data.profile;
}

GPResult
gpiProcessRecvBlockedList(
  GPConnection * connection,
  const char * input
)
{
    int i=0, j=0;
    int num = 0;
    int index = 0;
    char c;
    char *str = NULL;
    char buffer[512];
    GPIProfile * profile;
    GPProfile profileid;
    GPIConnection * iconnection = (GPIConnection*)*connection;

    // Check for an error.
    //////////////////////
    if(gpiCheckForError(connection, input, GPITrue))
        return GP_SERVER_ERROR;

    // Process Block List Retrieval msg - Format like:
    /* ===============================================
       \blk\<num in list>\list\<block list - comma delimited>\final\
       =============================================== */

    if(!gpiValueForKeyWithIndex(input, "\\blk\\", &index, buffer, sizeof(buffer)))
        CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");
    num = atoi(buffer);

    // Check to make sure list is there 
    ///////////////////////////////////
	str = strstr(input, "\\list\\");
    if (str == NULL)
        CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");
    
    // Then increment index to get ready for parsing
    ////////////////////////////////////////////////
    str += 6;
    index += 6;

    for (i=0; i < num; i++)
    {     
        if (i==0)
        {
            // Manually grab first profile in list - comma delimiter
            ////////////////////////////////////////////////////////
            for(j=0 ; (j < sizeof(buffer)) && ((c = str[j]) != '\0') && (c != ',') ; j++)
            {
                buffer[j] = c;
            }
            buffer[min(j, sizeof(buffer) - 1)] = '\0';
            index += j;
        }
        else
        {
            if(!gpiValueForKeyWithIndex(input, ",", &index, buffer, sizeof(buffer)))
                CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");
        }

        profileid = atoi(buffer);

        // Get the profile, adding if needed.
        /////////////////////////////////////
        profile = gpiProfileListAdd(connection, profileid);
        if(!profile)
            Error(connection, GP_MEMORY_ERROR, "Out of memory.");

        // Mark as blocked, increment list counter
        //////////////////////////////////////////
        profile->blocked = gsi_true; 
        profile->blockIndex = iconnection->profileList.numBlocked++;
    }

    return GP_NO_ERROR;
}
