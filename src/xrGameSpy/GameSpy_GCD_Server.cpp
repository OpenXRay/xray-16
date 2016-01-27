#include "StdAfx.h"
#include "GameSpy_GCD_Server.h"

bool	CGameSpy_GCD_Server::Init()
{
	int res = gcd_init_qr2(NULL, GAMESPY_GAMEID);
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
    gcd_shutdown();
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
	gcd_authenticate_user(GAMESPY_GAMEID, localid, userip, challenge, response,
        GSClientAuthCallback, GSClientReauthCallback, &ctx);
};

void	CGameSpy_GCD_Server::ReAuthUser(int localid, int hint,char *response)
{
	gcd_process_reauth(GAMESPY_GAMEID, localid, hint, response);
};

void	CGameSpy_GCD_Server::DisconnectUser(int localid)
{
	gcd_disconnect_user(GAMESPY_GAMEID, localid);
};

void	CGameSpy_GCD_Server::Think()
{
    gcd_think();
};

char*	CGameSpy_GCD_Server::GetKeyHash(int localid)
{
	return gcd_getkeyhash(GAMESPY_GAMEID, localid);
};