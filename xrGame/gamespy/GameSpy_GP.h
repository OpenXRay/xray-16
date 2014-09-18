#ifndef GAMESPY_GP_H
#define GAMESPY_GP_H

#include "GameSpy_FuncDefs.h"

#include "../../xrGameSpy/GameSpy/GP/gp.h"

class CGameSpy_GP
{
public:
	explicit						CGameSpy_GP		(HMODULE hGameSpyDLL);
									~CGameSpy_GP	();
				
				bool				Init			();
				void				ShutDown		();

				void				Think			();
	
	//public wrappers to API functions
				GPResult			NewUser			(shared_str const & nick,
													 shared_str const & unique_nick,
													 shared_str const & email,
													 shared_str const & password,
													 GPCallback callback,
													 void * param);
				GPResult			GetUserNicks	(shared_str const & email,
													 shared_str const & password,
													 GPCallback callback,
													 void * param);
				GPResult			Connect			(shared_str const & email,
													 shared_str const & nick,
													 shared_str const & password,
													 GPCallback callback,
													 void * param);
				void				Disconnect		();
				
				GPResult			ProfileSearch	(shared_str const & nick,
													 shared_str const & unique_nick,
													 shared_str const & email,
													 GPCallback callback,
													 void * param);
				GPResult			SuggestUNicks	(shared_str const & desired_unick,
													 GPCallback callback,
													 void * param);
				GPResult			DeleteProfile	(GPCallback callback,
													 void * param);
				GPResult			GetLoginTicket	(char loginTicket[GP_LOGIN_TICKET_LEN]);
				GPResult			SetUniqueNick	(shared_str const & unique_nick,
													 GPCallback  callback,
													 void * param);
	static		shared_str			TryToTranslate	(GPResult const & res);
private:
	GPConnection					m_GPConnection;

	GAMESPY_FN_VAR_DECL(GPResult,	gpInitialize,			(GPConnection * connection));
	GAMESPY_FN_VAR_DECL(void,		gpDestroy,				(GPConnection * connection));
	GAMESPY_FN_VAR_DECL(GPResult,	gpProcess,				(GPConnection * connection));
	GAMESPY_FN_VAR_DECL(GPResult,	gpSetCallback,			(GPConnection * connection,
															 GPEnum func,
															 GPCallback callback,
															 void * param));
	GAMESPY_FN_VAR_DECL(GPResult,	gpNewUserA,				(GPConnection * connection,
															 const gsi_char nick[GP_NICK_LEN],
															 const gsi_char uniquenick[GP_UNIQUENICK_LEN],
												 			 const gsi_char email[GP_EMAIL_LEN],
												 			 const gsi_char password[GP_PASSWORD_LEN],
												 			 const gsi_char cdkey[GP_CDKEY_LEN],
												 			 GPEnum blocking,
												 			 GPCallback callback,
												 			 void * param));
	GAMESPY_FN_VAR_DECL(GPResult,	gpNewProfileA,			(GPConnection * connection,
															 const char nick[GP_NICK_LEN],
															 GPEnum replace,
															 GPEnum blocking,
															 GPCallback callback,
															 void * param));
	GAMESPY_FN_VAR_DECL(GPResult,	gpProfileSearchA,		(GPConnection * connection,
															 const gsi_char nick[GP_NICK_LEN],
															 const gsi_char uniquenick[GP_UNIQUENICK_LEN],
															 const gsi_char email[GP_EMAIL_LEN],
															 const gsi_char firstname[GP_FIRSTNAME_LEN],
															 const gsi_char lastname[GP_LASTNAME_LEN],
															 int icquin,
															 GPEnum blocking,
															 GPCallback callback,
															 void * param));
	GAMESPY_FN_VAR_DECL(GPResult,	gpGetUserNicksA,		(GPConnection * connection,
															 const gsi_char email[GP_EMAIL_LEN],
															 const gsi_char password[GP_PASSWORD_LEN],
															 GPEnum blocking,
															 GPCallback callback,
															 void * param));
	GAMESPY_FN_VAR_DECL(GPResult,	gpConnectA,				(GPConnection * connection,
															 const gsi_char nick[GP_NICK_LEN],
															 const gsi_char email[GP_EMAIL_LEN],
															 const gsi_char password[GP_PASSWORD_LEN],
															 GPEnum firewall,
															 GPEnum blocking,
															 GPCallback callback,
															 void * param));
	GAMESPY_FN_VAR_DECL(void,		gpDisconnect,			(GPConnection * connection));
	GAMESPY_FN_VAR_DECL(GPResult,	gpSuggestUniqueNickA,	(GPConnection * connection,
															 const gsi_char desirednick[GP_UNIQUENICK_LEN],
															 GPEnum blocking,
															 GPCallback callback,
															 void * param));
	GAMESPY_FN_VAR_DECL(GPResult,	gpDeleteProfile,		(GPConnection * connection,
															 GPCallback callback,
															 void * arg));
	GAMESPY_FN_VAR_DECL(GPResult,	gpGetLoginTicket,		(GPConnection * connection,
															 char loginTicket[GP_LOGIN_TICKET_LEN]));
	GAMESPY_FN_VAR_DECL(GPResult,	gpRegisterUniqueNickA,	(GPConnection * connection,
															 const gsi_char uniquenick[GP_UNIQUENICK_LEN],
															 const gsi_char cdkey[GP_CDKEY_LEN],
															 GPEnum blocking,
															 GPCallback callback,
															 void * param));
	
	void							LoadGameSpyGP			(HMODULE hGameSpyDLL);
	//main callbacks
	static void __cdecl				OnGameSpyErrorCb		(GPConnection * connection,
															 void * arg,
															 void * param);
}; //CGameSpy_GP

#endif //#ifndef GAMESPY_GP_H