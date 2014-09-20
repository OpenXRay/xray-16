#pragma once
#include "xrGameSpy_MainDefs.h"
#include "GameSpy/pt/pt.h"

extern "C"
{
	EXPORT_FN_DECL(bool, ptCheckForPatchA, (
//		int productID,  const gsi_char * versionUniqueID,  int distributionID, 
		ptPatchCallback callback, 
		PTBool blocking, 
		void * instance ));

	EXPORT_FN_DECL(bool, ptTrackUsageA,	(int userID));
}