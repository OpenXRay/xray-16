#pragma once

#include "../../xrGameSpy/GameSpy/Common/gsCommon.h"
#include "../../xrGameSpy/GameSpy/Common/gsAvailable.h"
#include "../../xrGameSpy/GameSpy/ghttp/ghttp.h"

#include "../../xrGameSpy/GameSpy/qr2/qr2.h"
#include "../../xrGameSpy/GameSpy/gcdkey/gcdkeyc.h"
#include "../../xrGameSpy/GameSpy/gcdkey/gcdkeys.h"
#include "../../xrGameSpy/GameSpy/serverbrowsing/sb_serverbrowsing.h"

#include "../../xrGameSpy/GameSpy/pt/pt.h"
#include "../../xrGameSpy/GameSpy/gp/gp.h"
#include "../../xrGameSpy/GameSpy/sake/sake.h"
#include "../../xrGameSpy/GameSpy/sc/sc.h"

#undef max

extern "C" {

#define GAMESPY_TFN_DECL(r, f, p) typedef DLL_API r __cdecl t_fn_xrGS_##f p

};

#define GAMESPY_FN_VAR_DECL(r, f, p) GAMESPY_TFN_DECL(r, f, p); t_fn_xrGS_##f* xrGS_##f;
#define GAMESPY_LOAD_FN(f)    f = (t_fn_##f*)GetProcAddress(hGameSpyDLL, #f); R_ASSERT2(f, "No such func in xrGameSpy.dll");
