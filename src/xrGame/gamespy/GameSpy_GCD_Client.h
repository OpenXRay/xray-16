#pragma once

#include "GameSpy_FuncDefs.h"

class CGameSpy_GCD_Client
{
	private:
		HMODULE	m_hGameSpyDLL;

		void	LoadGameSpy(HMODULE hGameSpyDLL);
	public:
		CGameSpy_GCD_Client();
		CGameSpy_GCD_Client(HMODULE hGameSpyDLL);
		~CGameSpy_GCD_Client();

		void CreateRespond	(char* RespondStr, char* ChallengeStr, u8 Reauth);
private:
	//--------------------- GCD_Client -------------------------------------------	
	GAMESPY_FN_VAR_DECL(void, gcd_compute_response, (char *cdkey, char *challenge,char* response, bool Reauth));
};
