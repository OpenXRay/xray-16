#include "StdAfx.h"
#include "GameSpy_GCD_Client.h"
#include "GameSpy_Base_Defs.h"

#define	REGISTRY_CDKEY_STR	"Software\\S.T.A.L.K.E.R\\CDKey"

CGameSpy_GCD_Client::CGameSpy_GCD_Client()
{
	m_hGameSpyDLL = NULL;

	LPCSTR			g_name	= "xrGameSpy.dll";
	Log				("Loading DLL:",g_name);
	m_hGameSpyDLL			= LoadLibrary	(g_name);
	if (0==m_hGameSpyDLL)	R_CHK			(GetLastError());
	R_ASSERT2		(m_hGameSpyDLL,"GameSpy DLL raised exception during loading or there is no game DLL at all");

	LoadGameSpy(m_hGameSpyDLL);
};
CGameSpy_GCD_Client::CGameSpy_GCD_Client(HMODULE hGameSpyDLL)
{
	m_hGameSpyDLL = NULL;

	LoadGameSpy(hGameSpyDLL);
};

CGameSpy_GCD_Client::~CGameSpy_GCD_Client()
{
	if (m_hGameSpyDLL)
	{
		FreeLibrary(m_hGameSpyDLL);
		m_hGameSpyDLL = NULL;
	}
};
void	CGameSpy_GCD_Client::LoadGameSpy(HMODULE hGameSpyDLL)
{

	GAMESPY_LOAD_FN(xrGS_gcd_compute_response);	
}

string64	gsCDKey = "";
extern	void	GetCDKey_FromRegistry(char* CDKeyStr);
void CGameSpy_GCD_Client::CreateRespond	(char* RespondStr, char* ChallengeStr, u8 Reauth)
{
	string512	CDKey = "";
	GetCDKey_FromRegistry(CDKey);
	xrGS_gcd_compute_response(_strupr(CDKey), ChallengeStr, RespondStr, (Reauth == 1));
}