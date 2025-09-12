///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../gsCommon.h"
#include "../gsPlatformUtil.h"

void gsiRevolutionSleep(u32 msec);
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static OSAlarm gAlarm;
static OSThreadQueue gQueue;
static BOOL          gQueueInitialized;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static const char * GOAGetUniqueID_Internal(void)
{
	static char keyval[17];
	u8 aMac[ETH_ALEN];
	int aMacLen;

	// check if we already have the Unique ID
	if(keyval[0])
		return keyval;
	
	aMacLen = ETH_ALEN;
	SOGetInterfaceOpt (NULL, SO_SOL_CONFIG, SO_CONFIG_MAC_ADDRESS,
                   	   aMac, &aMacLen);
	
	// format it
	sprintf(keyval, "%02X%02X%02X%02X%02X%02X0000",
		aMac[0] & 0xFF,
		aMac[1] & 0xFF,
		aMac[2] & 0xFF,
		aMac[3] & 0xFF,
		aMac[4] & 0xFF,
		aMac[5] & 0xFF);

	return keyval;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Time Functions
static char GSIMonthNames[12][3] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char GSIWeekDayNames[7][3] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

time_t gsiTimeInSec(time_t *timer)
{
	time_t t = 0;
	t = (gsi_i32)OSTicksToSeconds(OSGetTime());
	
	if (timer)
		*timer = t;
	
	return t;
}


struct tm *gsiGetGmTime(time_t *theTime)
{
	static struct tm aTimeStruct;
	static struct tm *aRetVal = &aTimeStruct;
	OSCalendarTime aCalTimeStruct;
	
	OSTicksToCalendarTime(*theTime, &aCalTimeStruct);
	
	aRetVal->tm_sec  = aCalTimeStruct.sec;
	aRetVal->tm_min  = aCalTimeStruct.min;
	aRetVal->tm_hour = aCalTimeStruct.hour;
	aRetVal->tm_mday  = aCalTimeStruct.mday;
	aRetVal->tm_mon  = aCalTimeStruct.mon;
	aRetVal->tm_year  = aCalTimeStruct.year - 1900;
	aRetVal->tm_wday  = aCalTimeStruct.wday;
	aRetVal->tm_yday = 0;
	aRetVal->tm_isdst = 0;
	return aRetVal;
}

char *gsiCTime(time_t *theTime)
{
	static char str[26];
	struct tm *ptm = gsiGetGmTime(theTime);

	// e.g.: "Wed Jan 02 02:03:55 1980\n\0"
	sprintf(str, "%s %s %02d %02d:%02d:%02d %d\n",
		GSIWeekDayNames[ptm->tm_wday],
		GSIMonthNames[ptm->tm_mon], ptm->tm_mday,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
		ptm->tm_year + 1900);

	return str;
}

gsi_i64 gsiStringToInt64(const char *theNumberStr)
{
	return atoll(theNumberStr);
}

void gsiInt64ToString(char theNumberStr[33], gsi_i64 theNumber)
{
	// you want to fit the number! 
	// give me a valid string!
	GS_ASSERT(theNumberStr != NULL);

	sprintf(theNumberStr, "%lld", theNumber);
}