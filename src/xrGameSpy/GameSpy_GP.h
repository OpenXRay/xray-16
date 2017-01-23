#ifndef GAMESPY_GP_H
#define GAMESPY_GP_H

#include "xrCore/xrCore.h"
#include "xrGameSpy/xrGameSpy.h"

class XRGAMESPY_API CGameSpy_GP
{
public:
	CGameSpy_GP		();
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
	//main callbacks
	static void __cdecl				OnGameSpyErrorCb		(GPConnection * connection,
															 void * arg,
															 void * param);
}; //CGameSpy_GP

#endif //#ifndef GAMESPY_GP_H
