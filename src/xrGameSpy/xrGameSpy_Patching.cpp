#include "stdafx.h"
#include "windows.h"
#include "xrGameSpy_MainDefs.h"

#include "xrGameSpy_Patching.h"

extern const char*	GetGameVersion		();
extern			int GetGameDistribution	();

XRGAMESPY_API bool xrGS_ptCheckForPatchA(
										//int productID,  const gsi_char * versionUniqueID,  int distributionID, 
										ptPatchCallback callback, 
										PTBool blocking, 
										void * instance )
{
	//	return ptCheckForPatch(productID, versionUniqueID, distributionID, callback, blocking, instance )!=PTFalse;
	return ptCheckForPatchA(GAMESPY_PRODUCTID, 
//		GAME_VERSION,
		GetGameVersion(),
//		GAMESPY_PATCHING_VERSIONUNIQUE_ID, 
//		GAMESPY_PATCHING_DISTRIBUTION_ID, 
		GetGameDistribution(),
		callback, blocking, instance ) != PTFalse;
};

XRGAMESPY_API bool xrGS_ptTrackUsageA(int userID)
{
	return ptTrackUsageA(
		userID,
		GAMESPY_PRODUCTID,
		GetGameVersion(),
		GetGameDistribution(),
		PTFalse) != PTFalse;
}