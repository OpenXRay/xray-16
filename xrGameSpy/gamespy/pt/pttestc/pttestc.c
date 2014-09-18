#include "../pt.h"
#include "../../ghttp/ghttp.h"
#include "../../common/gsAvailable.h"
#include "../../common/gsStringUtil.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef UNDER_CE
	void RetailOutputA(CHAR *tszErr, ...);
	#define printf RetailOutputA
#elif defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

#define MRPANTS_UID       100001

#define GMTEST_GAMENAME   _T("gmtest")
#define GMTEST_PID        21
#define GMTEST_VID_0      _T("gmtest Alpha 0.01")
#define GMTEST_VID_1      _T("gmtest Alpha 0.02")
#define GMTEST_VID_2      _T("gmtest Beta 1")
#define GMTEST_VID_3      _T("gmtest Beta 2")
#define GMTEST_VID_4      _T("gmtest 1.0")
#define GMTEST_VID_5      _T("gmtest 1.1")
#define GMTEST_VID_6      _T("gmtest 2.0")

#ifdef GSI_COMMON_DEBUG
	static void DebugCallback(GSIDebugCategory theCat, GSIDebugType theType,
	                          GSIDebugLevel theLevel, const char * theTokenStr,
	                          va_list theParamList)
	{
		GSI_UNUSED(theLevel);

		printf("[%s][%s] ", 
				gGSIDebugCatStrings[theCat], 
				gGSIDebugTypeStrings[theType]);

		vprintf(theTokenStr, theParamList);
	}
#endif

static const gsi_char * BoolToString
(
	PTBool b
)
{
	if(b)
		return _T("Yes");
	return _T("No");
}

static void PatchCallback
(
	PTBool available,
	PTBool mandatory,
	const gsi_char * versionName,
	int fileID,
	const gsi_char * downloadURL,
	void * param
)
{
	_tprintf(_T("available = %s\n"), BoolToString(available));
	if(!available)
		return;
	_tprintf(_T("mandatory = %s\n"), BoolToString(mandatory));
	_tprintf(_T("versionName = %s\n"), versionName);
	_tprintf(_T("fileID = %d\n"), fileID);
	_tprintf(_T("downloadURL = %s\n"), downloadURL);
	
	GSI_UNUSED(param);
}

#ifdef __MWERKS__ // CodeWarrior will warn if not prototyped
	int test_main(int argc, char **argv);
#endif

int test_main(int argc, char **argv)
{
	unsigned long start_time;
	GSIACResult result;

#ifdef GSI_COMMON_DEBUG
	// Define GSI_COMMON_DEBUG if you want to view the SDK debug output
	// Set the SDK debug log file, or set your own handler using gsSetDebugCallback
	//gsSetDebugFile(stdout); // output to console
	gsSetDebugCallback(DebugCallback);

	// Set some debug levels
	gsSetDebugLevel(GSIDebugCat_All, GSIDebugType_All, GSIDebugLevel_Verbose);
	//gsSetDebugLevel(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Verbose);   // Show Detailed data on network traffic
	//gsSetDebugLevel(GSIDebugCat_App, GSIDebugType_All, GSIDebugLevel_Hardcore);  // Show All app comment
#endif

	// check that the game's backend is available
	GSIStartAvailableCheck(GMTEST_GAMENAME);
	while((result = GSIAvailableCheckThink()) == GSIACWaiting)
		msleep(5);
	if(result != GSIACAvailable)
	{
		printf("The backend is not available\n");
		return 1;
	}

	if(!ptTrackUsage(MRPANTS_UID, GMTEST_PID, GMTEST_VID_0, 0, PTFalse))
		_tprintf(_T("Failed to track usage\n"));
	else
		_tprintf(_T("ptTrackUsage successful\n"));

	if(!ptCheckForPatch(GMTEST_PID, GMTEST_VID_1, 0, PatchCallback, PTFalse, NULL))
		_tprintf(_T("Failed to check for patch\n"));

	start_time = current_time();
	while((current_time() - start_time) < 20000)
	{
		ghttpThink();
		msleep(1);
	}

	// Must call this to free internal memory
	ghttpCleanup();

	GSI_UNUSED(argc);
	GSI_UNUSED(argv);

	return 0;
}
