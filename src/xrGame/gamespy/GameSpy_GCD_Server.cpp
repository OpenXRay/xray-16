#include "StdAfx.h"
#include "GameSpy_GCD_Server.h"
#include "GameSpy_FuncDefs.h"
#include "GameSpy_Base_Defs.h"

CGameSpy_GCD_Server::CGameSpy_GCD_Server()
{
	m_hGameSpyDLL = NULL;

	LPCSTR			g_name	= "xrGameSpy";
	Log				("Loading DLL:",g_name);
	m_hGameSpyDLL			= LoadLibrary	(g_name);
	if (0==m_hGameSpyDLL)	R_CHK			(GetLastError());
	R_ASSERT2		(m_hGameSpyDLL,"GameSpy DLL raised exception during loading or there is no game DLL at all");

	LoadGameSpy(m_hGameSpyDLL);
};
CGameSpy_GCD_Server::CGameSpy_GCD_Server(HMODULE hGameSpyDLL)
{
	m_hGameSpyDLL = NULL;

	LoadGameSpy(hGameSpyDLL);
};
CGameSpy_GCD_Server::~CGameSpy_GCD_Server()
{
	if (m_hGameSpyDLL)
	{
		FreeLibrary(m_hGameSpyDLL);
		m_hGameSpyDLL = NULL;
	}
};

void	CGameSpy_GCD_Server::LoadGameSpy(HMODULE hGameSpyDLL)
{

	GAMESPY_LOAD_FN(xrGS_gcd_init_qr2);
	GAMESPY_LOAD_FN(xrGS_gcd_shutdown);
	GAMESPY_LOAD_FN(xrGS_gcd_authenticate_user);
	GAMESPY_LOAD_FN(xrGS_gcd_reauthenticate_user);
	GAMESPY_LOAD_FN(xrGS_gcd_disconnect_user);
	GAMESPY_LOAD_FN(xrGS_gcd_think);
	GAMESPY_LOAD_FN(xrGS_gcd_getkeyhash);
}

bool	CGameSpy_GCD_Server::Init()
{
	int res = xrGS_gcd_init_qr2(NULL);
	if (res == -1)
	{
		Msg("! xrGS::CDKey : Failes to Initialize!");
		return false;
	};
#ifndef MASTER_GOLD
	Msg("- xrGS::CDKey : Initialized");
#endif // #ifndef MASTER_GOLD
	return true;
};

void	CGameSpy_GCD_Server::ShutDown()
{
	xrGS_gcd_shutdown();
}

void	CGameSpy_GCD_Server::CreateRandomChallenge(char* challenge, int nchars)
{
	if (nchars > GAMESPY_MAXCHALLANGESIZE) nchars = GAMESPY_MAXCHALLANGESIZE;
	challenge[nchars] = 0;
	while (nchars--)
	{
		challenge[nchars] = char('a' + ::Random.randI(26));
	};
}

class GSAuthContext
{
public:
    using ClientAuthCallback = CGameSpy_GCD_Server::ClientAuthCallback;
    using ClientReauthCallback = CGameSpy_GCD_Server::ClientReauthCallback;

    CGameSpy_GCD_Server::ClientAuthCallback &Auth;
    CGameSpy_GCD_Server::ClientReauthCallback &Reauth;

    GSAuthContext(ClientAuthCallback &auth, ClientReauthCallback &reauth) :
        Auth(auth), Reauth(reauth)
    {}
};

//--------------------------- CD Key callbacks -----------------------------------
void __cdecl GSClientAuthCallback(int productid, int localid, int authenticated, char *errmsg, void *instance)
{
    auto ctx = static_cast<GSAuthContext*>(instance);
    if (ctx)
        ctx->Auth(localid, authenticated, errmsg);
};

void __cdecl GSClientReauthCallback(int gameid, int localid, int hint, char *challenge, void *instance)
{
    auto ctx = static_cast<GSAuthContext*>(instance);
    if (ctx)
        ctx->Reauth(localid, hint, challenge);
};

void	CGameSpy_GCD_Server::AuthUser(int localid, u32 userip, char *challenge, char *response, 
    ClientAuthCallback &authCallback, ClientReauthCallback &reauthCallback)
{
    GSAuthContext ctx(authCallback, reauthCallback);
	xrGS_gcd_authenticate_user(localid, userip, challenge, response, GSClientAuthCallback, GSClientReauthCallback, &ctx);
};

void	CGameSpy_GCD_Server::ReAuthUser(int localid, int hint,char *response)
{
	xrGS_gcd_reauthenticate_user(localid, hint, response);
};

void	CGameSpy_GCD_Server::DisconnectUser(int localid)
{
	xrGS_gcd_disconnect_user(localid);
};

void	CGameSpy_GCD_Server::Think()
{
	xrGS_gcd_think();
};

char*	CGameSpy_GCD_Server::GetKeyHash(int localid)
{
	return xrGS_gcd_getkeyhash(localid);
};