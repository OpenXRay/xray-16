#if defined(_PS3)

#include <sys/system_types.h>
#include <sys/types.h>
#include <netex/net.h>
#include <netex/libnetctl.h>

#include "../gsCommon.h"

// This needs to be set during interface start
extern int gNetInterfaceID;

#if defined(_PS3) && defined(UNIQUEID)
	static const char * GetMAC(void)
	{
		// Get the MAC address using the interface control
		static union CellNetCtlInfo gCellNetInfo;
		int r;
		// Get MAC
		// to do, cache this, and do this once at init.
		r = cellNetCtlGetInfo(	CELL_NET_CTL_INFO_ETHER_ADDR,&gCellNetInfo	);				
		if (r == CELL_OK)		
		{

			return (const char *)&gCellNetInfo.ether_addr;
		}// else error
		return NULL;
	}

	static const char * GOAGetUniqueID_Internal(void)
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
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // _PS3 only
