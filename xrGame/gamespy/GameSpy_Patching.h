#pragma once

#include "GameSpy_FuncDefs.h"

class CGameSpy_Patching
{
private:
	HMODULE	m_hGameSpyDLL;

	void	LoadGameSpy(HMODULE hGameSpyDLL);
public:
	CGameSpy_Patching();
	CGameSpy_Patching(HMODULE hGameSpyDLL);
	~CGameSpy_Patching();

	void CheckForPatch	(bool InformOfNoPatch);
	void PtTrackUsage	(int userID);
private:
	//--------------------- GCD_Client -------------------------------------------	
	GAMESPY_FN_VAR_DECL(bool, ptCheckForPatchA, (
//		int productID,  const char * versionUniqueID,  int distributionID, 
		ptPatchCallback callback, 
		PTBool blocking, 
		void * instance ));
	GAMESPY_FN_VAR_DECL(bool, ptTrackUsageA,	(int userID));
};