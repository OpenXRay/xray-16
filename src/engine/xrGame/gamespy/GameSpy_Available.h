#pragma once

#include "GameSpy_FuncDefs.h"

class CGameSpy_Available
{
private:
	HMODULE	m_hGameSpyDLL;

	void	LoadGameSpy(HMODULE hGameSpyDLL);
public:
	CGameSpy_Available();
	CGameSpy_Available(HMODULE hGameSpyDLL);
	~CGameSpy_Available();

	bool	CheckAvailableServices	(shared_str& resultstr);
private:
	//------------------------------- GameSpy_Available ---------------------------
//	GAMESPY_FN_VAR_DECL(void,	GSIStartAvailableCheck, (const char * gamename));
	GAMESPY_FN_VAR_DECL(void,	GSIStartAvailableCheckA, ());
	GAMESPY_FN_VAR_DECL(GSIACResult, GSIAvailableCheckThink, ());
	GAMESPY_FN_VAR_DECL(void, msleep, (unsigned long msec));
public:
	GAMESPY_FN_VAR_DECL(int, GetQueryVersion, ());
};
