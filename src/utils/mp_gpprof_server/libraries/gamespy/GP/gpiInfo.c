/*
gpiInfo.c
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

//INCLUDES
//////////
#include "gpi.h"

//FUNCTIONS
///////////
static GPIBool
gpiIsValidDate(
  int day,
  int month,
  int year
)
{
	// Check for a blank.
	/////////////////////
	if((day == 0) && (month == 0) && (year == 0))
		return GPITrue;

	// Check for negatives.
	///////////////////////
	if((day < 0) || (month < 0) || (year < 0))
		return GPIFalse;

	// Validate the day of the month.
	/////////////////////////////////
	switch(month)
	{
	// No month.
	////////////
	case 0:
		// Can't specify a day without a month.
		///////////////////////////////////////
		if(day != 0)
			return GPIFalse;
		break;

	// 31-day month.
	////////////////
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		if(day > 31)
			return GPIFalse;
		break;

	// 30-day month.
	////////////////
	case 4:
	case 6:
	case 9:
	case 11:
		if(day > 30)
			return GPIFalse;
		break;

	// 28/29-day month.
	///////////////////
	case 2:
		// Leap year?
		/////////////
		if((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0))
		{
			if(day > 29)
				return GPIFalse;
		}
		else
		{
			if(day > 28)
				return GPIFalse;
		}
		break;

	// Invalid month.
	/////////////////
	default:
		return GPIFalse;
	}

	// Check that the date is in the valid range.
	// 01.01.1900 - 06.06.2079
	// PANTS|02.14.2000
	/////////////////////////////////////////////
	if(year < 1900)
		return GPIFalse;
	if(year > 2079)
		return GPIFalse;
	if(year == 2079)
	{
		if(month > 6)
			return GPIFalse;
		if((month == 6) && (day > 6))
			return GPIFalse;
	}

	return GPITrue;
}

static GPResult
gpiDateToInt(
  GPConnection * connection,
  int * date,
  int day,
  int month,
  int year
)
{
	int temp;
	
	// Pack the day/month/year into an int.
	// 31-22: day
	// 23-16: month
	// 15-00: year
	///////////////////////////////////////
	
	// Error check.
	///////////////
	assert(gpiIsValidDate(day, month, year));
	if(!gpiIsValidDate(day, month, year))
		Error(connection, GP_PARAMETER_ERROR, "Invalid date.");

	// Pack!
	////////
	temp = 0;
	temp |= (day << 24);
	temp |= (month << 16);
	temp |= year;

	// Set it.
	//////////
	*date = temp;

	return GP_NO_ERROR;
}

static GPResult
gpiIntToDate(
  GPConnection * connection,
  int date,
  int * day,
  int * month,
  int * year
)
{
	int d;
	int m;
	int y;

	// Unpack the int into a day/month/year.
	// 31-22: day
	// 23-16: month
	// 15-00: year
	////////////////////////////////////////

	// Split up the date.
	/////////////////////
	d = ((date >> 24) & 0xFF);
	m = ((date >> 16) & 0xFF);
	y = (date & 0xFFFF);

	// Error check.
	///////////////
	assert(gpiIsValidDate(d, m, y));
	if(!gpiIsValidDate(d, m, y))
		Error(connection, GP_PARAMETER_ERROR, "Invalid date.");

	// It's all good.
	/////////////////
	*day = d;
	*month = m;
	*year = y;

	return GP_NO_ERROR;
}

void
gpiInfoCacheToArg(
  const GPIInfoCache * cache,
  GPGetInfoResponseArg * arg
)
{
#ifndef GSI_UNICODE
	// Copy....
	///////////
	if(cache->nick)
		strzcpy(arg->nick, cache->nick, GP_NICK_LEN);
	else
		arg->nick[0] = '\0';
	if(cache->uniquenick)
		strzcpy(arg->uniquenick, cache->uniquenick, GP_UNIQUENICK_LEN);
	else
		arg->uniquenick[0] = '\0';
	if(cache->email)
		strzcpy(arg->email, cache->email, GP_EMAIL_LEN);
	else
		arg->email[0] = '\0';
	if(cache->firstname)
		strzcpy(arg->firstname, cache->firstname, GP_FIRSTNAME_LEN);
	else
		arg->firstname[0] = '\0';
	if(cache->lastname)
		strzcpy(arg->lastname, cache->lastname, GP_LASTNAME_LEN);
	else
		arg->lastname[0] = '\0';
	if(cache->homepage)
		strzcpy(arg->homepage, cache->homepage, GP_HOMEPAGE_LEN);
	else
		arg->homepage[0] = '\0';
	arg->icquin = cache->icquin;
	strzcpy(arg->zipcode, cache->zipcode, GP_ZIPCODE_LEN);
	strzcpy(arg->countrycode, cache->countrycode, GP_COUNTRYCODE_LEN);
	arg->longitude = cache->longitude;
	arg->latitude = cache->latitude;
	if(cache->place)
		strzcpy(arg->place, cache->place, GP_PLACE_LEN);
	else
		arg->place[0] = '\0';
	arg->birthday = cache->birthday;
	arg->birthmonth = cache->birthmonth;
	arg->birthyear = cache->birthyear;
	arg->sex = (GPEnum)cache->sex;
	arg->publicmask = (GPEnum)cache->publicmask;
	if(cache->aimname)
		strzcpy(arg->aimname, cache->aimname, GP_AIMNAME_LEN);
	else
		arg->aimname[0] = '\0';
#else
	// Copy....
	///////////
	if(cache->nick)
		UTF8ToUCS2StringLen(cache->nick, arg->nick, GP_NICK_LEN);
	else
		arg->nick[0] = '\0';
	if(cache->uniquenick)
		UTF8ToUCS2StringLen(cache->uniquenick, arg->uniquenick, GP_UNIQUENICK_LEN);
	else
		arg->uniquenick[0] = '\0';
	if(cache->email)
		UTF8ToUCS2StringLen(cache->email, arg->email, GP_EMAIL_LEN);
	else
		arg->email[0] = '\0';
	if(cache->firstname)
		UTF8ToUCS2StringLen(cache->firstname, arg->firstname, GP_FIRSTNAME_LEN);
	else
		arg->firstname[0] = '\0';
	if(cache->lastname)
		UTF8ToUCS2StringLen(cache->lastname, arg->lastname, GP_LASTNAME_LEN);
	else
		arg->lastname[0] = '\0';
	if(cache->homepage)
		UTF8ToUCS2StringLen(cache->homepage, arg->homepage, GP_HOMEPAGE_LEN);
	else
		arg->homepage[0] = '\0';
	UTF8ToUCS2StringLen(cache->zipcode, arg->zipcode, GP_ZIPCODE_LEN);
	UTF8ToUCS2StringLen(cache->countrycode, arg->countrycode, GP_COUNTRYCODE_LEN);
	if(cache->place)
		UTF8ToUCS2StringLen(cache->place, arg->place, GP_PLACE_LEN);
	else
		arg->place[0] = '\0';
	if(cache->aimname)
		UTF8ToUCS2StringLen(cache->aimname, arg->aimname, GP_AIMNAME_LEN);
	else
		arg->aimname[0] = '\0';
#endif

	// Non string members
	arg->icquin = cache->icquin;
	arg->longitude = cache->longitude;
	arg->latitude = cache->latitude;

	arg->birthday = cache->birthday;
	arg->birthmonth = cache->birthmonth;
	arg->birthyear = cache->birthyear;
	arg->sex = (GPEnum)cache->sex;
	arg->publicmask = (GPEnum)cache->publicmask;

	arg->pic = cache->pic;
	arg->occupationid = cache->occupationid;
	arg->industryid = cache->industryid;
	arg->incomeid = cache->incomeid;
	arg->marriedid = cache->marriedid;
	arg->childcount = cache->childcount;
	arg->interests1 = cache->interests1;
	arg->ownership1 = cache->ownership1;
	arg->conntypeid = cache->conntypeid;
}

GPResult
gpiProcessGetInfo(
  GPConnection * connection,
  GPIOperation * operation,
  const char * input
)
{
	GPIInfoCache infoCache;
	char buffer[64];
	int profileid;
	GPIProfile * profile;
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPICallback callback;
	GPIPeer * peer;
	char nick[GP_NICK_LEN];
	char uniquenick[GP_UNIQUENICK_LEN];
	char email[GP_EMAIL_LEN];
	char firstname[GP_FIRSTNAME_LEN];
	char lastname[GP_LASTNAME_LEN];
	char homepage[GP_HOMEPAGE_LEN];
	char aimname[GP_AIMNAME_LEN];
	GPIBool saveSig;

	// Check for an error.
	//////////////////////
	if(gpiCheckForError(connection, input, GPITrue))
		return GP_SERVER_ERROR;

	// This should be \pi\.
	///////////////////////
	if(strncmp(input, "\\pi\\", 4) != 0)
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");

	// Get the profile id.
	//////////////////////
	if(!gpiValueForKey(input, "\\profileid\\", buffer, sizeof(buffer)))
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");
	profileid = atoi(buffer);
	assert(profileid > 0);

	// Get the profile - we might not have a profile object.
	////////////////////////////////////////////////////////
	gpiGetProfile(connection, (GPProfile)profileid, &profile);

	// Setup the info cache.
	////////////////////////
	memset(&infoCache, 0, sizeof(GPIInfoCache));
	infoCache.nick = nick;
	infoCache.uniquenick = uniquenick;
	infoCache.email = email;
	infoCache.firstname = firstname;
	infoCache.lastname = lastname;
	infoCache.homepage = homepage;
	infoCache.aimname = aimname;

	// Parse the info.
	//////////////////
	if(!gpiValueForKey(input, "\\nick\\", infoCache.nick, GP_NICK_LEN))
		infoCache.nick[0] = '\0';
	if(!gpiValueForKey(input, "\\uniquenick\\", infoCache.uniquenick, GP_UNIQUENICK_LEN))
		infoCache.uniquenick[0] = '\0';
	if(!gpiValueForKey(input, "\\email\\", infoCache.email, GP_EMAIL_LEN))
		infoCache.email[0] = '\0';
	if(!gpiValueForKey(input, "\\firstname\\", infoCache.firstname, GP_FIRSTNAME_LEN))
		infoCache.firstname[0] = '\0';
	if(!gpiValueForKey(input, "\\lastname\\", infoCache.lastname, GP_LASTNAME_LEN))
		infoCache.lastname[0] = '\0';
	if(!gpiValueForKey(input, "\\icquin\\", buffer, sizeof(buffer)))
		infoCache.icquin = -1;
	else
		infoCache.icquin = atoi(buffer);
	if(!gpiValueForKey(input, "\\homepage\\", infoCache.homepage, GP_HOMEPAGE_LEN))
		infoCache.homepage[0] = '\0';
	if(!gpiValueForKey(input, "\\zipcode\\", infoCache.zipcode, sizeof(infoCache.zipcode)))
		infoCache.zipcode[0] = '\0';
	if(!gpiValueForKey(input, "\\countrycode\\", infoCache.countrycode, sizeof(infoCache.countrycode)))
		infoCache.countrycode[0] = '\0';
	if(!gpiValueForKey(input, "\\lon\\", buffer, sizeof(buffer)))
		infoCache.longitude = 0;
	else
		infoCache.longitude = (float)atof(buffer);
	if(!gpiValueForKey(input, "\\lat\\", buffer, sizeof(buffer)))
		infoCache.latitude = 0;
	else
		infoCache.latitude = (float)atof(buffer);
	if(!gpiValueForKey(input, "\\loc\\", infoCache.place, GP_PLACE_LEN))
		infoCache.place[0] = '\0';
	if(!gpiValueForKey(input, "\\birthday\\", buffer, sizeof(buffer)))
	{
		infoCache.birthday = 0;
		infoCache.birthmonth = 0;
		infoCache.birthyear = 0;
	}
	else
	{
		CHECK_RESULT(gpiIntToDate(connection, atoi(buffer), &infoCache.birthday, &infoCache.birthmonth, &infoCache.birthyear));
	}
	if(!gpiValueForKey(input, "\\sex\\", buffer, sizeof(buffer)))
		infoCache.sex = GP_PAT;
	else if(buffer[0] == '0')
		infoCache.sex = GP_MALE;
	else if(buffer[0] == '1')
		infoCache.sex = GP_FEMALE;
	else
		infoCache.sex = GP_PAT;
	if(!gpiValueForKey(input, "\\pmask\\", buffer, sizeof(buffer)))
		infoCache.publicmask = 0xFFFFFFFF;
	else
		infoCache.publicmask = atoi(buffer);
	if(!gpiValueForKey(input, "\\aim\\", infoCache.aimname, GP_AIMNAME_LEN))
		infoCache.aimname[0] = '\0';
	if(!gpiValueForKey(input, "\\pic\\", buffer, sizeof(buffer)))
		infoCache.pic = 0;
	else
		infoCache.pic = atoi(buffer);
	if(!gpiValueForKey(input, "\\occ\\", buffer, sizeof(buffer)))
		infoCache.occupationid = 0;
	else
		infoCache.occupationid = atoi(buffer);
	if(!gpiValueForKey(input, "\\ind\\", buffer, sizeof(buffer)))
		infoCache.industryid = 0;
	else
		infoCache.industryid = atoi(buffer);
	if(!gpiValueForKey(input, "\\inc\\", buffer, sizeof(buffer)))
		infoCache.incomeid = 0;
	else
		infoCache.incomeid = atoi(buffer);
	if(!gpiValueForKey(input, "\\mar\\", buffer, sizeof(buffer)))
		infoCache.marriedid = 0;
	else
		infoCache.marriedid = atoi(buffer);
	if(!gpiValueForKey(input, "\\chc\\", buffer, sizeof(buffer)))
		infoCache.childcount = 0;
	else
		infoCache.childcount = atoi(buffer);
	if(!gpiValueForKey(input, "\\i1\\", buffer, sizeof(buffer)))
		infoCache.interests1 = 0;
	else
		infoCache.interests1 = atoi(buffer);
	if(!gpiValueForKey(input, "\\o1\\", buffer, sizeof(buffer)))
		infoCache.ownership1 = 0;
	else
		infoCache.ownership1 = atoi(buffer);
	if(!gpiValueForKey(input, "\\conn\\", buffer, sizeof(buffer)))
		infoCache.conntypeid = 0;
	else
		infoCache.conntypeid = atoi(buffer);

	// Get the peer sig.
	////////////////////
	if(!gpiValueForKey(input, "\\sig\\", buffer, sizeof(buffer)))
		CallbackFatalError(connection, GP_NETWORK_ERROR, GP_PARSE, "Unexpected data was received from the server.");

	saveSig = iconnection->infoCaching;

	// Is there a pending peer connection looking for a sig?
	////////////////////////////////////////////////////////
	for(peer = iconnection->peerList ; peer != NULL ; peer = peer->pnext)
	{
		// Is it the same profile?
		//////////////////////////
		if(peer->profile == profileid)
		{
			// Is it getting the sig?
			/////////////////////////
			if(peer->state == GPI_PEER_GETTING_SIG)
			{
				// We need to make sure there's an actual profile object.
				/////////////////////////////////////////////////////////
				if(!profile)
					profile = gpiProfileListAdd(connection, profileid);

				// It got it.
				/////////////
				peer->state = GPI_PEER_GOT_SIG;

				saveSig = GPITrue;
			}
		}
	}

	// Cache info?
	//////////////
	if(!profile && iconnection->infoCaching)
		profile = gpiProfileListAdd(connection, profileid);

	// Set the peer sig.
	////////////////////
	if(saveSig)
	{
		freeclear(profile->peerSig);
		profile->peerSig = goastrdup(buffer);
	}

	// Caching info?
	////////////////
	if(iconnection->infoCaching)
		gpiSetInfoCache(connection, profile, &infoCache);

	// Call the callback.
	/////////////////////
	callback = operation->callback;
	if(callback.callback != NULL)
	{
		GPGetInfoResponseArg * arg;
		arg = (GPGetInfoResponseArg *)gsimalloc(sizeof(GPGetInfoResponseArg));
		if(arg == NULL)
			Error(connection, GP_MEMORY_ERROR, "Out of memory.");
		
		gpiInfoCacheToArg(&infoCache, arg);
		arg->result = GP_NO_ERROR;
		arg->profile = (GPProfile)profileid;

		CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
	}

	// This operation is complete.
	//////////////////////////////
	gpiRemoveOperation(connection, operation);

	return GP_NO_ERROR;
}

GPResult
gpiAddLocalInfo(
  GPConnection * connection,
  GPIBuffer * buffer
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Add updatepro info.
	//////////////////////
	if(iconnection->updateproBuffer.len > 0)
	{
		gpiAppendStringToBuffer(connection, buffer, "\\updatepro\\\\sesskey\\");
		gpiAppendIntToBuffer(connection, buffer, iconnection->sessKey);
		gpiAppendStringToBuffer(connection, buffer, iconnection->updateproBuffer.buffer);
		gpiAppendStringToBuffer(connection, buffer, "\\partnerid\\");
		gpiAppendIntToBuffer(connection, buffer, iconnection->partnerID);
		gpiAppendStringToBuffer(connection, buffer, "\\final\\");

		iconnection->updateproBuffer.len = 0;
	}

	// Add updateui info.
	//////////////////////
	if(iconnection->updateuiBuffer.len > 0)
	{
		gpiAppendStringToBuffer(connection, buffer, "\\updateui\\\\sesskey\\");
		gpiAppendIntToBuffer(connection, buffer, iconnection->sessKey);
		gpiAppendStringToBuffer(connection, buffer, iconnection->updateuiBuffer.buffer);
		gpiAppendStringToBuffer(connection, buffer, "\\final\\");

		iconnection->updateuiBuffer.len = 0;
	}

	return GP_NO_ERROR;
}

static GPResult
gpiSendLocalInfo(
  GPConnection * connection,
  const char * info,
  const char * value
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	CHECK_RESULT(gpiAppendStringToBuffer(connection, &iconnection->updateproBuffer, info));
	CHECK_RESULT(gpiAppendStringToBuffer(connection, &iconnection->updateproBuffer, value));

	return GP_NO_ERROR;
}

static GPResult
gpiSendUserInfo(
  GPConnection * connection,
  const char * info,
  const char * value
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	CHECK_RESULT(gpiAppendStringToBuffer(connection, &iconnection->updateuiBuffer, info));
	CHECK_RESULT(gpiAppendStringToBuffer(connection, &iconnection->updateuiBuffer, value));

	return GP_NO_ERROR;
}

GPResult
gpiSetInfoi(
  GPConnection * connection, 
  GPEnum info, 
  int value
)
{
	char intValue[16];

	// Check the info param.
	////////////////////////
	switch(info)
	{
	case GP_ZIPCODE:
		// Error check.
		///////////////
		if(value < 0)
			Error(connection, GP_PARAMETER_ERROR, "Invalid zipcode.");

		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\zipcode\\", intValue));

		break;

	case GP_SEX:
		// Check the sex type.
		//////////////////////
		switch(value)
		{
		case GP_MALE:
			CHECK_RESULT(gpiSendLocalInfo(connection, "\\sex\\", "0"));
			break;

		case GP_FEMALE:
			CHECK_RESULT(gpiSendLocalInfo(connection, "\\sex\\", "1"));
			break;

		case GP_PAT:
			CHECK_RESULT(gpiSendLocalInfo(connection, "\\sex\\", "2"));
			break;

		default:
			Error(connection, GP_PARAMETER_ERROR, "Invalid sex.");
		}

		break;

	case GP_ICQUIN:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\icquin\\", intValue));

		break;
		
	case GP_CPUBRANDID:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendUserInfo(connection, "\\cpubrandid\\", intValue));

		break;

	case GP_CPUSPEED:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendUserInfo(connection, "\\cpuspeed\\", intValue));

		break;

	case GP_MEMORY:
		// Divide by 16.
		////////////////
		value /= 16;

		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendUserInfo(connection, "\\memory\\", intValue));

		break;

	case GP_VIDEOCARD1RAM:
		// Divide by 4.
		///////////////
		value /= 4;

		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendUserInfo(connection, "\\videocard1ram\\", intValue));

		break;

	case GP_VIDEOCARD2RAM:
		// Divide by 4.
		///////////////
		value /= 4;

		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendUserInfo(connection, "\\videocard2ram\\", intValue));

		break;

	case GP_CONNECTIONID:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendUserInfo(connection, "\\connectionid\\", intValue));

		break;

	case GP_CONNECTIONSPEED:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendUserInfo(connection, "\\connectionspeed\\", intValue));

		break;

	case GP_HASNETWORK:
		// A boolean.
		/////////////
		if(value)
			value = 1;

		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendUserInfo(connection, "\\hasnetwork\\", intValue));

		break;

	case GP_PIC:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\pic\\", intValue));

		break;

	case GP_OCCUPATIONID:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\occ\\", intValue));

		break;

	case GP_INDUSTRYID:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\ind\\", intValue));

		break;

	case GP_INCOMEID:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\inc\\", intValue));

		break;

	case GP_MARRIEDID:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\mar\\", intValue));

		break;

	case GP_CHILDCOUNT:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\chc\\", intValue));

		break;

	case GP_INTERESTS1:
		// Convert it to a string.
		//////////////////////////
		sprintf(intValue,"%d",value);

		// Send it to the server.
		/////////////////////////
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\i1\\", intValue));

		break;

	default:
		Error(connection, GP_PARAMETER_ERROR, "Invalid info.");
	}

	return GP_NO_ERROR;
}

GPResult
gpiSetInfos(
  GPConnection * connection, 
  GPEnum info, 
  const char * value
)
{
	
	GPIConnection * iconnection = (GPIConnection*)*connection;
	char buffer[256];
	char sex;
	
	//password encryption stuff
	char passwordenc[GP_PASSWORDENC_LEN];
	
	// Error check.
	///////////////
	if(value == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid value.");

	// Check the info param.
	////////////////////////
	switch(info)
	{
	case GP_NICK:
		if(!value[0])
			Error(connection, GP_PARAMETER_ERROR, "Invalid value.");
		strzcpy(buffer, value, GP_NICK_LEN);
		strzcpy(iconnection->nick, buffer, GP_NICK_LEN);
#ifdef GSI_UNICODE
		UTF8ToUCS2StringLen(iconnection->nick, iconnection->nick_W, GP_NICK_LEN); // update the UCS2 version
#endif
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\nick\\", buffer));
		break;

	case GP_UNIQUENICK:
		if(!value[0])
			Error(connection, GP_PARAMETER_ERROR, "Invalid value.");
		strzcpy(buffer, value, GP_UNIQUENICK_LEN);
		strzcpy(iconnection->uniquenick, buffer, GP_UNIQUENICK_LEN);
#ifdef GSI_UNICODE
		UTF8ToUCS2StringLen(iconnection->uniquenick, iconnection->uniquenick_W, GP_UNIQUENICK_LEN);
#endif
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\uniquenick\\", buffer));
		break;

	case GP_EMAIL:
		if(!value[0])
			Error(connection, GP_PARAMETER_ERROR, "Invalid value.");
		strzcpy(buffer, value, GP_EMAIL_LEN);
		_strlwr(buffer);
		strzcpy(iconnection->email, buffer, GP_EMAIL_LEN);
#ifdef GSI_UNICODE
		UTF8ToUCS2StringLen(iconnection->email, iconnection->email_W, GP_EMAIL_LEN);
#endif
		CHECK_RESULT(gpiSendUserInfo(connection, "\\email\\", buffer));
		break;

	case GP_PASSWORD:
		if(!value[0])
			Error(connection, GP_PARAMETER_ERROR, "Invalid value.");
		strzcpy(buffer, value, GP_PASSWORD_LEN);
		strzcpy(iconnection->password, buffer, GP_PASSWORD_LEN);
#ifdef GSI_UNICODE
		UTF8ToUCS2StringLen(iconnection->password, iconnection->password_W, GP_PASSWORD_LEN);
#endif
		gpiEncodeString(iconnection->password, passwordenc);
		CHECK_RESULT(gpiSendUserInfo(connection, "\\passwordenc\\", passwordenc));
		break;

	case GP_FIRSTNAME:
		strzcpy(buffer, value, GP_FIRSTNAME_LEN);
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\firstname\\", buffer));
		break;

	case GP_LASTNAME:
		strzcpy(buffer, value, GP_LASTNAME_LEN);
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\lastname\\", buffer));
		break;

	case GP_HOMEPAGE:
		strzcpy(buffer, value, GP_HOMEPAGE_LEN);
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\homepage\\", buffer));
		break;

	case GP_ZIPCODE:
		strzcpy(buffer, value, GP_ZIPCODE_LEN);
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\zipcode\\", buffer));
		break;

	case GP_COUNTRYCODE:
		// Error check.
		///////////////
		if(strlen(value) != 2)
			Error(connection, GP_PARAMETER_ERROR, "Invalid countrycode.");

		strzcpy(buffer, value, GP_COUNTRYCODE_LEN);
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\countrycode\\", buffer));
		break;

	case GP_SEX:
		sex = (char)toupper(value[0]);
		if(sex == 'M')
			strcpy(buffer, "0");
		else if(sex == 'F')
			strcpy(buffer, "1");
		else
			strcpy(buffer, "2");

		CHECK_RESULT(gpiSendLocalInfo(connection, "\\sex\\", buffer));
		break;

	case GP_ICQUIN:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\icquin\\", buffer));
		break;

	case GP_CPUSPEED:
		CHECK_RESULT(gpiSetInfoi(connection, GP_CPUSPEED, atoi(value)));
		break;

	case GP_MEMORY:
		CHECK_RESULT(gpiSetInfoi(connection, GP_MEMORY, atoi(value)));
		break;

	case GP_VIDEOCARD1STRING:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\videocard1string\\", buffer));
		break;

	case GP_VIDEOCARD1RAM:
		CHECK_RESULT(gpiSetInfoi(connection, GP_VIDEOCARD1RAM, atoi(value)));
		break;

	case GP_VIDEOCARD2STRING:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\videocard2string\\", buffer));
		break;

	case GP_VIDEOCARD2RAM:
		CHECK_RESULT(gpiSetInfoi(connection, GP_VIDEOCARD2RAM, atoi(value)));
		break;

	case GP_CONNECTIONSPEED:
		CHECK_RESULT(gpiSetInfoi(connection, GP_CONNECTIONSPEED, atoi(value)));
		break;

	case GP_HASNETWORK:
		CHECK_RESULT(gpiSetInfoi(connection, GP_HASNETWORK, atoi(value)));
		break;

	case GP_OSSTRING:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\osstring\\", buffer));
		break;

	case GP_AIMNAME:
		strzcpy(buffer, value, GP_AIMNAME_LEN);
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\aim\\", buffer));
		break;

	case GP_PIC:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\pic\\", buffer));
		break;

	case GP_OCCUPATIONID:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\occ\\", buffer));
		break;

	case GP_INDUSTRYID:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\ind\\", buffer));
		break;

	case GP_INCOMEID:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\inc\\", buffer));
		break;

	case GP_MARRIEDID:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\mar\\", buffer));
		break;

	case GP_CHILDCOUNT:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\chc\\", buffer));
		break;

	case GP_INTERESTS1:
		strzcpy(buffer, value, sizeof(buffer));
		CHECK_RESULT(gpiSendLocalInfo(connection, "\\i1\\", buffer));
		break;

	default:
		Error(connection, GP_PARAMETER_ERROR, "Invalid info.");
	}

	return GP_NO_ERROR;
}

GPResult
gpiSetInfod(
  GPConnection * connection, 
  GPEnum info, 
  int day,
  int month,
  int year
)
{
	int date;
	char intValue[16];

	// Birthday is the only date supported.
	///////////////////////////////////////
	if(info != GP_BIRTHDAY)
		Error(connection, GP_PARAMETER_ERROR, "Invalid info.");

	// Convert the date into our internal format.
	/////////////////////////////////////////////
	CHECK_RESULT(gpiDateToInt(connection, &date, day, month, year));

	// Convert the int to a string.
	///////////////////////////////
	sprintf(intValue,"%d",date);

	// Send the date.
	/////////////////
	CHECK_RESULT(gpiSendLocalInfo(connection, "\\birthday\\", intValue));

	return GP_NO_ERROR;
}

GPResult
gpiSetInfoMask(
  GPConnection * connection, 
  GPEnum mask
)
{
	char buffer[16];

	// Convert the mask to a string.
	////////////////////////////////
	sprintf(buffer,"%d",mask);

	// Send it.
	///////////
	CHECK_RESULT(gpiSendLocalInfo(connection, "\\publicmask\\", buffer));

	return GP_NO_ERROR;

}

GPResult
gpiSendGetInfo(
  GPConnection * connection,
  int profileid,
  int operationid
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\getprofile\\\\sesskey\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\profileid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, profileid);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\id\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, operationid);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
	
	return GP_NO_ERROR;
}

GPResult
gpiGetInfo(
  GPConnection * connection,
  GPProfile profile, 
  GPEnum checkCache,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIProfile * pProfile;
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIOperation * operation = NULL;
	GPIBool useCache;
	GPResult result;
	int id;

	// Check checkCache.
	////////////////////
	useCache = (checkCache == GP_CHECK_CACHE) ? GPITrue:GPIFalse;

	// Check the info cache state.
	//////////////////////////////
	if(!iconnection->infoCaching)
		useCache = GPIFalse;

	// Check for using cached info.
	///////////////////////////////
	if(callback && useCache && gpiGetProfile(connection, profile, &pProfile) && pProfile->cache)
	{
		GPICallback gpiCallback;
		GPGetInfoResponseArg * arg;

		arg = (GPGetInfoResponseArg *)gsimalloc(sizeof(GPGetInfoResponseArg));
		if(arg == NULL)
			Error(connection, GP_MEMORY_ERROR, "Out of memory.");

		gpiInfoCacheToArg(pProfile->cache, arg);
		arg->result = GP_NO_ERROR;
		arg->profile = profile;
		
		gpiCallback.callback = callback;
		gpiCallback.param = param;

		// Add a dummy operation.
		/////////////////////////
		CHECK_RESULT(gpiAddOperation(connection, GPI_GET_INFO, NULL, &operation, GP_BLOCKING, callback, param));
		id = operation->id;

		// Add the callback.
		////////////////////
		CHECK_RESULT(gpiAddCallback(connection, gpiCallback, arg, operation, 0));

		// Remove the dummy operation.
		//////////////////////////////
		gpiRemoveOperation(connection, operation);
	}
	else
	{
		// Add the operation.
		/////////////////////
		CHECK_RESULT(gpiAddOperation(connection, GPI_GET_INFO, NULL, &operation, blocking, callback, param));
		id = operation->id;

		// Send a request for info.
		///////////////////////////
		result = gpiSendGetInfo(connection, profile, operation->id);
		CHECK_RESULT(result);
	}

	// Process it if blocking.
	//////////////////////////
	if(blocking)
	{
		result = gpiProcess(connection, id);
		CHECK_RESULT(result);
	}

	return GP_NO_ERROR;
}

GPResult
gpiGetInfoNoWait(
  GPConnection * connection,
  GPProfile profile,
  GPGetInfoResponseArg * arg
)
{
	GPIProfile * pProfile;
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Check the info cache state.
	//////////////////////////////
	if(!iconnection->infoCaching)
		return GP_NETWORK_ERROR;

	// Check to see if we have the info cached.
	///////////////////////////////////////////
	if(!gpiGetProfile(connection, profile, &pProfile) || !pProfile->cache)
		return GP_NETWORK_ERROR;

	// Fill in the arg.
	///////////////////
	gpiInfoCacheToArg(pProfile->cache, arg);
	arg->result = GP_NO_ERROR;
	arg->profile = profile;

	return GP_NO_ERROR;
}

GPIBool
gpiSetInfoCache(
  GPConnection * connection,
  pGPIProfile  profile,
  const GPIInfoCache * cache
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Check if we're caching info.
	///////////////////////////////
	if(!iconnection->infoCaching)
		return GPITrue;

	// Free any old cached info.
	////////////////////////////
	gpiFreeInfoCache(profile);

	// Allocate the new info.
	/////////////////////////
	profile->cache = (GPIInfoCache *)gsimalloc(sizeof(GPIInfoCache));

	// Copy in the new info.
	////////////////////////
	if(profile->cache)
	{
		*profile->cache = *cache;
		profile->cache->nick = goastrdup(cache->nick);
		profile->cache->uniquenick = goastrdup(cache->uniquenick);
		profile->cache->email = goastrdup(cache->email);
		profile->cache->firstname = goastrdup(cache->firstname);
		profile->cache->lastname = goastrdup(cache->lastname);
		profile->cache->homepage = goastrdup(cache->homepage);
		profile->cache->aimname = goastrdup(cache->aimname);
	}

	return (profile->cache != NULL) ? GPITrue:GPIFalse;
}

void
gpiFreeInfoCache(
  pGPIProfile  profile
)
{
	if(!profile->cache)
		return;
	
	freeclear(profile->cache->nick);
	freeclear(profile->cache->uniquenick);
	freeclear(profile->cache->email);
	freeclear(profile->cache->firstname);
	freeclear(profile->cache->lastname);
	freeclear(profile->cache->homepage);
	freeclear(profile->cache->aimname);
	freeclear(profile->cache);
}
