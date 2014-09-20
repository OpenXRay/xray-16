#include "stdafx.h"
#include "windows.h"
#include "xrGameSpy_MainDefs.h"

#include "xrGameSpy_Available.h"

//XRGAMESPY_API void	xrGS_GSIStartAvailableCheck(const gsi_char * gamename)
XRGAMESPY_API void	xrGS_GSIStartAvailableCheckA()
{
//	GSIStartAvailableCheck(gamename);
	GSIStartAvailableCheck(GAMESPY_GAMENAME);
};

XRGAMESPY_API GSIACResult xrGS_GSIAvailableCheckThink()
{
	return GSIAvailableCheckThink();
}

XRGAMESPY_API void xrGS_msleep(gsi_time msec)
{
	msleep(msec);
}
