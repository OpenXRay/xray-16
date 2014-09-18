#include "StdAfx.h"
#include "GameSpy_Patching.h"
#include "GameSpy_Base_Defs.h"

CGameSpy_Patching::CGameSpy_Patching()
{
	hGameSpyDLL = NULL;

	LoadGameSpy();
};

CGameSpy_Patching::~CGameSpy_Patching()
{
	if (hGameSpyDLL)
	{
		FreeLibrary(hGameSpyDLL);
		hGameSpyDLL = NULL;
	}
};
void	CGameSpy_Patching::LoadGameSpy()
{
	LPCSTR			g_name	= "xrGameSpy.dll";
	Log				("Loading DLL:",g_name);
	hGameSpyDLL			= LoadLibrary	(g_name);
	if (0==hGameSpyDLL)	R_CHK			(GetLastError());
	R_ASSERT2		(hGameSpyDLL,"GameSpy DLL raised exception during loading or there is no game DLL at all");

	GAMESPY_LOAD_FN(xrGS_ptCheckForPatch);
}

void __cdecl GS_ptPatchCallback ( PTBool available, PTBool mandatory, const char * versionName, int fileID, const char * downloadURL,  void * param )
{
	int x=0;
	x=x;
};

void	CGameSpy_Patching::CheckForPatch()
{
	/*
	bool res =  xrGS_ptCheckForPatch(GAMESPY_PRODUCTID, 
		GAMESPY_PATCHING_VERSIONUNIQUE_ID, 
		GAMESPY_PATCHING_DISTRIBUTION_ID,
		GS_ptPatchCallback,
		PTTrue,
		this
		);
		*/

	bool res =  xrGS_ptCheckForPatch(10953, 
		"test_version_1", 
		0,
		GS_ptPatchCallback,
		PTTrue,
		this
		);
	int x=0;
	x=x;
};