#include "../gsCommon.h"

gsi_i64 gsiStringToInt64(const char *theNumberStr)
{
	return _atoi64(theNumberStr);
}

void gsiInt64ToString(char theNumberStr[33], gsi_i64 theNumber)
{
	// you want to fit the number! 
	// give me a valid string!
	GS_ASSERT(theNumberStr != NULL);
	
#if _MSC_VER > 1300
	sprintf(theNumberStr, "%lld", theNumber);
#else 
	sprintf(theNumberStr, "%I64d", theNumber);
#endif
}
