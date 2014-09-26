#include "stdafx.h"
#include "windows.h"
#include "xrGameSpy_MainDefs.h"

#include "xrGameSpy_CDKey.h"

#include "GameSpy/gcdkey/gcdkeyc.h"
#include "GameSpy/gcdkey/gcdkeys.h"




XRGAMESPY_API void xrGS_gcd_compute_response(char *cdkey, char *challenge,char* response, bool Reauth)
{
	gcd_compute_response(cdkey, challenge,response, Reauth ? CDResponseMethod_REAUTH : CDResponseMethod_NEWAUTH);
};

XRGAMESPY_API int xrGS_gcd_init_qr2(qr2_t qrec)
{
	return gcd_init_qr2(qrec, GAMESPY_GAMEID);
};
XRGAMESPY_API void xrGS_gcd_shutdown(void)
{
	gcd_shutdown();
}
XRGAMESPY_API void xrGS_gcd_authenticate_user(int localid, unsigned int userip, char *challenge, char *response, 
						   AuthCallBackFn authfn, RefreshAuthCallBackFn refreshfn, void *instance)
{
	gcd_authenticate_user(GAMESPY_GAMEID, localid, userip, challenge, response, authfn, refreshfn, instance);
}

XRGAMESPY_API void xrGS_gcd_reauthenticate_user(int localid, int hint, const char *response)
{	
	gcd_process_reauth(GAMESPY_GAMEID, localid, hint, response);
}

XRGAMESPY_API void xrGS_gcd_disconnect_user(int localid)
{
	gcd_disconnect_user(GAMESPY_GAMEID, localid);
}
XRGAMESPY_API void xrGS_gcd_think(void)
{
	gcd_think();
}

XRGAMESPY_API char* xrGS_gcd_getkeyhash(int localid)
{
	return gcd_getkeyhash(GAMESPY_GAMEID,localid);
}