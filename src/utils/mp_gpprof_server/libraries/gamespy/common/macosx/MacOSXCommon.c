#include "../gsCommon.h"
#include "../gsMemory.h"
#include "../gsDebug.h"

// sample common entry point
extern int test_main(int argc, char ** argp); 

// Debug output
#ifdef GSI_COMMON_DEBUG
static void DebugCallback(GSIDebugCategory theCat, GSIDebugType theType,
						  GSIDebugLevel theLevel, const char * theTokenStr,
						  va_list theParamList)

{
	GSI_UNUSED(theLevel);
	{
		static char    string[256];
		vsprintf(string, theTokenStr, theParamList); 			
		printf(string);
	}
	printf("[%s][%s] ", 
		   gGSIDebugCatStrings[theCat], 
		   gGSIDebugTypeStrings[theType]);
	
	vprintf(theTokenStr, theParamList);
}
#endif

// Common entry point
int main(int argc, char** argp)
{
	int		ret		= 0;
	// set up memanager
	//void	*heap	= (void*)gsiMemManagedInit();
	
#ifdef GSI_COMMON_DEBUG
	// Set up debugging
	gsSetDebugCallback(DebugCallback);
	gsSetDebugLevel(GSIDebugCat_All, GSIDebugType_All,    GSIDebugLevel_Verbose);
#endif
	
	ret = test_main(argc, argp);
	
	//gsiMemManagedClose(heap);
	
	return ret;
}
