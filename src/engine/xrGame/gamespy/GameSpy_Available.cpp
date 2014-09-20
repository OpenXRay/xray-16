#include "StdAfx.h"
#include "GameSpy_Available.h"
#include "GameSpy_Base_Defs.h"

CGameSpy_Available::CGameSpy_Available()
{
	m_hGameSpyDLL = NULL;
	//-----------------------------------------------
	LPCSTR			g_name	= "xrGameSpy.dll";
	Log				("Loading DLL:",g_name);
	m_hGameSpyDLL			= LoadLibrary	(g_name);
	if (0==m_hGameSpyDLL)	R_CHK			(GetLastError());
	R_ASSERT2		(m_hGameSpyDLL,"GameSpy DLL raised exception during loading or there is no game DLL at all");
	//-----------------------------------------------
	LoadGameSpy(m_hGameSpyDLL );
};

CGameSpy_Available::CGameSpy_Available(HMODULE hGameSpyDLL)
{
	m_hGameSpyDLL = NULL;
	LoadGameSpy(hGameSpyDLL);
}

CGameSpy_Available::~CGameSpy_Available()
{
	if (m_hGameSpyDLL)
	{
		FreeLibrary(m_hGameSpyDLL);
		m_hGameSpyDLL = NULL;
	}
};

void	CGameSpy_Available::LoadGameSpy(HMODULE hGameSpyDLL)
{
	GAMESPY_LOAD_FN(xrGS_GSIStartAvailableCheckA);
	GAMESPY_LOAD_FN(xrGS_GSIAvailableCheckThink);
	GAMESPY_LOAD_FN(xrGS_msleep);
	GAMESPY_LOAD_FN(xrGS_GetQueryVersion);
}

bool	CGameSpy_Available::CheckAvailableServices		(shared_str& resultstr)
{
	GSIACResult result;
	xrGS_GSIStartAvailableCheckA();

	while((result = xrGS_GSIAvailableCheckThink()) == GSIACWaiting)
		xrGS_msleep(5);

	if(result != GSIACAvailable)
	{
		switch (result)
		{
		case GSIACUnavailable:
			{
				resultstr = "! Online Services for STALKER are no longer available.";
			}break;
		case GSIACTemporarilyUnavailable:
			{
				resultstr = "! Online Services for STALKER are temporarily down for maintenance.";
			}break;
		}
		return false;
	}
	else
	{
		resultstr = "Success";
	};
	return true;
};