#pragma once

#include "xrCore/xrCore.h"
#include "xrGameSpy/xrGameSpy.h"

#define	GAMESPY_MAXCHALLANGESIZE	32

class XRGAMESPY_API CGameSpy_GCD_Server
{
public:
    using ClientAuthCallback = fastdelegate::FastDelegate<void(int localId, int authenticated, char *errmsg)>;
    using ClientReauthCallback = fastdelegate::FastDelegate<void(int localId, int hint, char *challenge)>;
	bool	Init();
	void	ShutDown();
	void	CreateRandomChallenge(char* challenge, int nchars);
	void	AuthUser(int localid, u32 userip, char *challenge, char *response,
        ClientAuthCallback &authCallback, ClientReauthCallback &reauthCallback);
	void	ReAuthUser(int localid, int hint, char *response);
	void	DisconnectUser(int localid);
	void	Think();
	char*	GetKeyHash(int localid);
};
