#include "StdAfx.h"
#include "GameSpy_Patching.h"
#include "GameSpy_Base_Defs.h"
#include "MainMenu.h"
#include "xrGameSpy/xrGameSpy_MainDefs.h"

CGameSpy_Patching::CGameSpy_Patching()
{
	m_hGameSpyDLL = NULL;

	LPCSTR			g_name	= "xrGameSpy";
	Log				("Loading DLL:",g_name);
	m_hGameSpyDLL			= LoadLibrary	(g_name);
	if (0==m_hGameSpyDLL)	R_CHK			(GetLastError());
	R_ASSERT2		(m_hGameSpyDLL,"GameSpy DLL raised exception during loading or there is no game DLL at all");

	LoadGameSpy(m_hGameSpyDLL);
};
CGameSpy_Patching::CGameSpy_Patching(HMODULE hGameSpyDLL)
{
	m_hGameSpyDLL = NULL;

	LoadGameSpy(hGameSpyDLL);
};
CGameSpy_Patching::~CGameSpy_Patching()
{
	if (m_hGameSpyDLL)
	{
		FreeLibrary(m_hGameSpyDLL);
		m_hGameSpyDLL = NULL;
	}
};
void	CGameSpy_Patching::LoadGameSpy(HMODULE hGameSpyDLL)
{	
	GAMESPY_LOAD_FN(xrGS_ptCheckForPatchA);
	GAMESPY_LOAD_FN(xrGS_ptTrackUsageA);
}


static char const * QueryPatchVersionString(char* dest, u32 dest_size)
{
	HKEY KeyCDKey = 0;
	
	long res = RegOpenKeyEx(REGISTRY_BASE, 
		REGISTRY_PATH, 0, KEY_READ, &KeyCDKey);

	if (res != ERROR_SUCCESS || KeyCDKey == 0)
		return "";

	//string128 SourceID;
	string256 LangID;
	DWORD KeyValueSize = sizeof(LangID);
	DWORD KeyValueType = REG_SZ;

	//RegQueryValueEx(KeyCDKey, REGISTRY_VALUE_LANGUAGE, NULL, &KeyValueType, (LPBYTE)LangID, &KeyValueSize);
	RegQueryValueEx(KeyCDKey, REGISTRY_VALUE_SKU, NULL, &KeyValueType, (LPBYTE)LangID, &KeyValueSize);

	xr_sprintf(dest, dest_size, "-%s", LangID);
		
	RegCloseKey(KeyCDKey);
	return dest;
}

#define PATCH_SUFFIX ".exe"
#define PATCH_SUFFIX_SIZE (sizeof(PATCH_SUFFIX) - 1)
#define APPEND_DWURL_INFO_LEN 256
static char const * ModifyDownloadUrl(char* dest, u32 dest_size, char const * origDownloadUrl)
{
	if (!origDownloadUrl)
		return "";

	xr_strcpy(dest, dest_size, origDownloadUrl);
	u32 url_size = xr_strlen(dest);
	if (url_size < PATCH_SUFFIX_SIZE)
		return dest;

	char* search_ptr = (dest + url_size) - PATCH_SUFFIX_SIZE;
	char* suffix_ptr = NULL;
	while (search_ptr > dest)
	{
		suffix_ptr = strstr(search_ptr, PATCH_SUFFIX);
		if (suffix_ptr)
			break;
		
		search_ptr--;
	}
	if (!suffix_ptr)
		return dest;

	*suffix_ptr = 0;
	string256 tmp_append_str;
	xr_strcat(dest, dest_size, QueryPatchVersionString(tmp_append_str, sizeof(tmp_append_str)));
	xr_strcat(dest, dest_size, PATCH_SUFFIX);
	return dest;
};




bool g_bInformUserThatNoPatchFound = true;
void __cdecl GS_ptPatchCallback ( PTBool available, PTBool mandatory, const char * versionName, int fileID, const char * downloadURL,  void * param )
{
	if (!MainMenu()) return;
	if (!available)
	{
		Msg("No new patches are available.");
		if (g_bInformUserThatNoPatchFound)
			MainMenu()->OnNoNewPatchFound();		
		return;
	};
	Msg("Found NewPatch: %s - %s", versionName, downloadURL);
	u32 new_url_size = APPEND_DWURL_INFO_LEN + (downloadURL ? xr_strlen(downloadURL) : 0);
	char* new_download_url = static_cast<char*>(_alloca(new_url_size));
	char const * new_url = ModifyDownloadUrl(new_download_url, new_url_size, downloadURL);
	Msg("NewPatch url after updating: %s", new_url);
	MainMenu()->OnNewPatchFound(versionName, new_url);
};

void	CGameSpy_Patching::CheckForPatch(bool InformOfNoPatch)
{
	g_bInformUserThatNoPatchFound = InformOfNoPatch;
	bool res =  xrGS_ptCheckForPatchA(
		GS_ptPatchCallback,
		PTFalse,
		this
	);	
	if (!res)
	{
		Msg("! Unable to send query for patch!");

	}
};

void CGameSpy_Patching::PtTrackUsage	(int userID)
{
	xrGS_ptTrackUsageA(userID);
}