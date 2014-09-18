#pragma once

#include "GameSpy_FuncDefs.h"
#define	GAMESPY_MAXCHALLANGESIZE	32

class CGameSpy_GCD_Server
{
	private:
		HMODULE	m_hGameSpyDLL;

		void	LoadGameSpy(HMODULE hGameSpyDLL);
	public:
		CGameSpy_GCD_Server();
		CGameSpy_GCD_Server(HMODULE hGameSpyDLL);
		~CGameSpy_GCD_Server();		

		bool	Init();
		void	ShutDown();
		void	CreateRandomChallenge(char* challenge, int nchars);
		void	AuthUser(int localid, unsigned int userip, char *challenge, char *response, void *instance);
		void	ReAuthUser(int localid, int hint, char *response);
		void	DisconnectUser(int localid);
		void	Think();
		char*	GetKeyHash(int localid);
private:
	//--------------------- GCD_Server -------------------------------------------
	GAMESPY_FN_VAR_DECL(int, gcd_init_qr2, (void* qrec));
	GAMESPY_FN_VAR_DECL(void, gcd_shutdown, (void));
	GAMESPY_FN_VAR_DECL(void, gcd_authenticate_user, (int localid, unsigned int userip, char *challenge, char *response, 
							AuthCallBackFn authfn, RefreshAuthCallBackFn refreshfn, void *instance));
	GAMESPY_FN_VAR_DECL(void, gcd_reauthenticate_user, (int localid, int hint, const char *response));
	GAMESPY_FN_VAR_DECL(void, gcd_disconnect_user, (int localid));
	GAMESPY_FN_VAR_DECL(void, gcd_think, (void));
	GAMESPY_FN_VAR_DECL(char*,gcd_getkeyhash,(int localid));
};

