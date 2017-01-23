#pragma once
#include "Common/Platform.hpp"

#ifdef XRGAMESPY_EXPORTS
#define XRGAMESPY_API XR_EXPORT
#else
#define XRGAMESPY_API XR_IMPORT
#endif

#include <GameSpy/Common/gsCommon.h>
#include <GameSpy/Common/gsAvailable.h>
#include <GameSpy/ghttp/ghttp.h>
#include <GameSpy/qr2/qr2.h>
#include <GameSpy/gcdkey/gcdkeyc.h>
#include <GameSpy/gcdkey/gcdkeys.h>
#include <GameSpy/serverbrowsing/sb_serverbrowsing.h>
#include <GameSpy/pt/pt.h>
#include <GameSpy/gp/gp.h>
#include <GameSpy/sake/sake.h>
#include <GameSpy/sc/sc.h>
#undef max

#include "xrGameSpy/xrGameSpy_MainDefs.h"
#include "xrGameSpy/GameSpy_Available.h"
#include "xrGameSpy/GameSpy_Browser.h"
#include "xrGameSpy/GameSpy_QR2.h"
#include "xrGameSpy/GameSpy_GCD_Client.h"
#include "xrGameSpy/GameSpy_GCD_Server.h"

XRGAMESPY_API const char* GetGameVersion();
XRGAMESPY_API int GetGameDistribution();
XRGAMESPY_API void GetGameID(int *GameID, int verID);

void FillSecretKey(char *secretKey);
