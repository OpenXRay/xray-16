#include "stdafx.h"
#include "GameSpy_GP.h"
#include "account_manager.h"
#include "login_manager.h"
#include "../MainMenu.h"		//in case of fatal error, deleting profile class instance

shared_str CGameSpy_GP::TryToTranslate(GPResult const & res)
{
	VERIFY(res != GP_NO_ERROR);
	LPCSTR tmp_string = NULL;
	switch (res)
	{
	case GP_MEMORY_ERROR:
		{
			STRCONCAT(tmp_string, "mp_gp_memory_error");
		}break;
	case GP_PARAMETER_ERROR:
		{
			STRCONCAT(tmp_string, "mp_gp_parameter_error");
		}break;
	case GP_NETWORK_ERROR:
		{
			STRCONCAT(tmp_string, "mp_gp_network_error");
		}break;
	case GP_SERVER_ERROR:
		{
			STRCONCAT(tmp_string, "mp_gp_server_error");
		}
	default:
		{
			string16 digit_dest;
			_itoa_s(res, digit_dest, 10);
			STRCONCAT(
				tmp_string,
				"mp_gp_unknown_error_",
				digit_dest
			);
		}
	}
	return tmp_string;
}

CGameSpy_GP::CGameSpy_GP(HMODULE hGameSpyDLL)
{
	m_GPConnection	= NULL;	//GPConnection type is: void*
	LoadGameSpyGP	(hGameSpyDLL);
	Init			();
}

CGameSpy_GP::~CGameSpy_GP()
{
	ShutDown	();
}

bool CGameSpy_GP::Init()
{
	GPResult init_res = xrGS_gpInitialize(&m_GPConnection);
	VERIFY2(init_res == GP_NO_ERROR, "GameSpy GP: failed to initialize");
	if (init_res != GP_NO_ERROR)
	{
		Msg("! GameSpy GP: failed to initialize, error code: %d", init_res);
		return false;
	}
	xrGS_gpSetCallback(&m_GPConnection, GP_ERROR, &CGameSpy_GP::OnGameSpyErrorCb, this);
	return true;
}

void CGameSpy_GP::Think()
{
	if (!m_GPConnection)
	{
		Msg("! GameSpy GP ERROR: GameSpy GP connection not ititialized");
		return;
	}
	GPResult process_res = xrGS_gpProcess(&m_GPConnection);
	if (process_res != GP_NO_ERROR)
	{
		Msg("! GameSpy GP ERROR: process failed: %d", process_res);
	}
}

void CGameSpy_GP::ShutDown()
{
	if (m_GPConnection)
		xrGS_gpDestroy(&m_GPConnection);
}

GPResult CGameSpy_GP::NewUser(shared_str const & nick,
							  shared_str const & unique_nick,
							  shared_str const & email,
							  shared_str const & password,
							  GPCallback callback,
							  void * param)
{
	return xrGS_gpNewUserA(
		&m_GPConnection,
		nick.c_str(),
		unique_nick.c_str(),
		email.c_str(),
		password.c_str(),
		NULL,
		GP_NON_BLOCKING,
		callback,
		param
	);
}

GPResult CGameSpy_GP::ProfileSearch(shared_str const & nick,
								   shared_str const & unique_nick,
								   shared_str const & email,
								   GPCallback callback,
								   void * param)
{
	return xrGS_gpProfileSearchA(
		&m_GPConnection,
		nick.c_str(),
		unique_nick.c_str(),
		email.c_str(),
		NULL,
		NULL,
		0,
		GP_NON_BLOCKING,
		callback,
		param
	);
}

GPResult CGameSpy_GP::GetUserNicks(shared_str const & email,
								  shared_str const & password,
								  GPCallback callback,
								  void * param)
{
	return xrGS_gpGetUserNicksA(
		&m_GPConnection,
		email.c_str(),
		password.c_str(),
		GP_NON_BLOCKING,
		callback,
		param
	);
}

GPResult CGameSpy_GP::SuggestUNicks(shared_str const & desired_unick,
									GPCallback callback,
									void * param)
{
	return xrGS_gpSuggestUniqueNickA(
		&m_GPConnection,
		desired_unick.c_str(),
		GP_NON_BLOCKING,
		callback,
		param
	);
}

GPResult CGameSpy_GP::DeleteProfile(GPCallback callback,
									void * param)
{
	return xrGS_gpDeleteProfile(&m_GPConnection, callback, param);
}


GPResult CGameSpy_GP::Connect(shared_str const & email,
							  shared_str const & nick,
							  shared_str const & password,
							  GPCallback callback,
							  void * param)
{
	return xrGS_gpConnectA(
		&m_GPConnection,
		nick.c_str(),
		email.c_str(),
		password.c_str(),
		GP_FIREWALL,
		GP_NON_BLOCKING,
		callback,
		param
	);
}

void CGameSpy_GP::Disconnect()
{
	xrGS_gpDisconnect(&m_GPConnection);
}

GPResult CGameSpy_GP::GetLoginTicket(char loginTicket[GP_LOGIN_TICKET_LEN])
{
	return xrGS_gpGetLoginTicket(&m_GPConnection, loginTicket);
}

GPResult CGameSpy_GP::SetUniqueNick(shared_str const & unique_nick,
									GPCallback  callback,
									void * param)
{
	VERIFY(unique_nick.size() < GP_UNIQUENICK_LEN);
	return xrGS_gpRegisterUniqueNickA(
		&m_GPConnection,
		unique_nick.c_str(),
		NULL,
		GP_NON_BLOCKING,
		callback,
		param
	);
}

void CGameSpy_GP::LoadGameSpyGP(HMODULE hGameSpyDLL)
{
	GAMESPY_LOAD_FN(xrGS_gpInitialize);
	GAMESPY_LOAD_FN(xrGS_gpDestroy);
	GAMESPY_LOAD_FN(xrGS_gpProcess);
	GAMESPY_LOAD_FN(xrGS_gpSetCallback);
	GAMESPY_LOAD_FN(xrGS_gpNewUserA);
	GAMESPY_LOAD_FN(xrGS_gpNewProfileA);
	GAMESPY_LOAD_FN(xrGS_gpProfileSearchA);
	GAMESPY_LOAD_FN(xrGS_gpConnectA);
	GAMESPY_LOAD_FN(xrGS_gpDisconnect);
	GAMESPY_LOAD_FN(xrGS_gpGetUserNicksA);
	GAMESPY_LOAD_FN(xrGS_gpSuggestUniqueNickA);
	GAMESPY_LOAD_FN(xrGS_gpDeleteProfile);
	GAMESPY_LOAD_FN(xrGS_gpGetLoginTicket);
	GAMESPY_LOAD_FN(xrGS_gpRegisterUniqueNickA);
}

void __cdecl CGameSpy_GP::OnGameSpyErrorCb(GPConnection * connection,
										   void * arg,
										   void * param)
{
	GPErrorArg* earg		= static_cast<GPErrorArg*>(arg);
	VERIFY(earg);
	char const * error_descr = earg->errorString ? earg->errorString : "unknown";
	if (earg->fatal)
	{
		Msg("! GameSpy FATAL GP ERROR: error code: %d, description: %s",
			earg->errorCode,
			error_descr);
		//MainMenu()->GetLoginMngr()->delete_profile_obj();
		return;
	}
	Msg("! GameSpy GP ERROR: error code: %d, description: %s", 
		earg->errorCode,
		error_descr);
}