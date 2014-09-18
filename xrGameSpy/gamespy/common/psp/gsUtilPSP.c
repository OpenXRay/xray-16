#if defined(_PSP)
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../gsCommon.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if !defined(GSI_NO_THREADS)

static void gsiResolveHostnameThread(void * arg)
{
	int result = 0;
	int resolverID = 0;
	char buf[1024]; // PSP documentation recommends 1024
	in_addr addr;
	GSIResolveHostnameHandle * info = (GSIResolveHostnameHandle)arg;

	result = sceNetResolverCreate(&resolverID, buf, sizeof(buf));
	if (result < 0)
	{
		// failed to create resolver, did you call sceNetResolverInit() ?
		info->ip = GSI_ERROR_RESOLVING_HOSTNAME;
		return -1;
	}
	else
	{
		// this will block until completed
		result = sceNetResolverStartNtoA(resolverID, info->hostname, &addr, &info->ip, GSI_RESOLVER_TIMEOUT, GSI_RESOLVER_RETRY);
		if (result < 0)
			info->ip = GSI_ERROR_RESOLVING_HOSTNAME;
		sceNetResolverDelete(resolverID);
	}
}

int gsiStartResolvingHostname(const char * hostname, GSIResolveHostnameHandle * handle)
{
	GSIResolveHostnameInfo * info;

	// allocate a handle
	info = (GSIResolveHostnameInfo *)gsimalloc(sizeof(GSIResolveHostnameInfo));
	if(!info)
		return -1;

	// make a copy of the hostname so the thread has access to it
	info->hostname = goastrdup(hostname);
	if(!info->hostname)
	{
		gsifree(info);
		return -1;
	}

	// not resolved yet
	info->finishedResolving = 0;

	// start the thread
	if(gsiStartThread(gsiResolveHostnameThread, (0x1000), info, &info->threadID) == -1)
	{
		gsifree(info->hostname);
		gsifree(info);
		return -1;
	}

	// set the handle to the info
	*handle = info;

	return 0;
}

void gsiCancelResolvingHostname(GSIResolveHostnameHandle handle)
{
	if (0 == handle->finishedResolving)
	{
		sceNetResolverStop(handle->resolverID); // safe to call from separate thread
		gsiCancelThread(handle->threadID);
	}
}

unsigned int gsiGetResolvedIP(GSIResolveHostnameHandle handle)
{
	return handle->ip;
}

#endif // (GSI_NO_THREADS)


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(UNIQUEID)

static const char * GetMAC(void)
{
	static struct SceNetEtherAddr mac;
	int result = 0;

	result = sceNetGetLocalEtherAddr(&mac);
	if (result == 0)
		return (char *)&mac.data;
	else
		return NULL;
}

const char * GOAGetUniqueID_Internal(void)
{
	static char keyval[17];
	const char * MAC;

	// check if we already have the Unique ID
	if(keyval[0])
		return keyval;

	// get the MAC
	MAC = GetMAC();
	if(!MAC)
	{
		// error getting the MAC
		static char errorMAC[6] = { 1, 2, 3, 4, 5, 6 };
		MAC = errorMAC;
	}

	// format it
	sprintf(keyval, "%02X%02X%02X%02X%02X%02X0000",
		MAC[0] & 0xFF,
		MAC[1] & 0xFF,
		MAC[2] & 0xFF,
		MAC[3] & 0xFF,
		MAC[4] & 0xFF,
		MAC[5] & 0xFF);

	return keyval;
}

#endif

//NEED These again since MAX INTEGRAL BITS is not defined 
#define GSI_MIN_I64       LONG_LONG_MIN
#define GSI_MAX_I64       LONG_LONG_MAX
#define GSI_MAX_U64       ULONG_LONG_MAX
/* flag values */
#define FL_UNSIGNED   1       /* strtouq called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

gsi_i64 gsiStringToInt64(const char *theNumberStr)
{
	const char *p;
	char c;
	gsi_i64 number;
	unsigned digval;
	gsi_u64 maxval;
	int flags = 0;
	// Added for compatibility reasons
	int ibase = 10;
	
	p = theNumberStr; // p is our scanning pointer
	number = 0;       // start with zero

	// read char 
	c = *p++;            

	// skip whitespace 
	while ( isspace(c) )
		c = *p++;        

	 
	if (c == '-') {
		flags |= FL_NEG;    // remember minus sign
		c = *p++;
	}
	// skip sign
	else if (c == '+')
		c = *p++;        

	if (ibase == 0) 
	{
		// determine base free-lance, based on first two chars of
		// string
		if (c != '0')
			ibase = 10;
		else if (*p == 'x' || *p == 'X')
			ibase = 16;
		else
			ibase = 8;
	}

	if (ibase == 16) 
	{
		// we might have 0x in front of number; remove if there
		if (c == '0' && (*p == 'x' || *p == 'X')) {
			++p;
			c = *p++;    /* advance past prefix */
		}
	}

	// if our number exceeds this, we will overflow on multiply 
	maxval = GSI_MAX_U64 / ibase;


	// exit in middle of loop
	for (;;) 
	{    
		// convert c to value
		if ( isdigit(c) )
			digval = c - '0';
		else if ( isalpha(c) )
			digval = toupper(c) - 'A' + 10;
		else
			break;

		// exit loop if bad digit found
		if (digval >= (unsigned)ibase)
			break;        

		/* record the fact we have read one digit */
		flags |= FL_READDIGIT;

		// we now need to compute number = number * base + digval,
		// but we need to know if overflow occurred.  This requires
		// a tricky pre-check.

		if (number < maxval || (number == maxval &&
			(gsi_u64)digval <= GSI_MAX_U64 % ibase)) 
		{
				// we won't overflow, go ahead and multiply
				number = number * ibase + digval;
		}
		else 
		{
			// we have overflowed, set the flag
			flags |= FL_OVERFLOW;
			break;
		}

		c = *p++;        /* read next digit */
	}

	--p;                /* point to place that stopped scan */
	
	if (!(flags & FL_READDIGIT)) {
		/* no number there; return 0 and point to beginning of
		string */
		number = 0L;        /* return 0 */
	}
	else if ( (flags & FL_OVERFLOW) ||
		( !(flags & FL_UNSIGNED) &&
		( ( (flags & FL_NEG) && (number > GSI_MIN_I64) ) ||
		( !(flags & FL_NEG) && (number > GSI_MAX_I64) ) ) ) )
	{
		/* overflow or signed overflow occurred */
		errno = ERANGE;
		if ( flags & FL_NEG )
			number = GSI_MIN_I64;
		else
			number = GSI_MAX_I64;
	}
	
	if (flags & FL_NEG)
		/* negate result if there was a neg sign */
		number = -number;

	return number;            /* done. */
}

void gsiInt64ToString(char theNumberStr[33], gsi_i64 theNumber)
{
	// you want to fit the number! 
	// give me a valid string!
	GS_ASSERT(theNumberStr != NULL);

	sprintf(theNumberStr, "%lld", theNumber);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // _PSP only
