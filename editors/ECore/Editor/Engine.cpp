// Engine.cpp: implementation of the CEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Engine.h"
#include "ui_main.h"

xrDispatchTable	PSGP;                
CEngine	Engine;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEngine::CEngine()
{
	pSettings = 0;
}

CEngine::~CEngine()
{

}

LPCSTR CEngine::LastWindowsError()
{
	static string1024 errmsg_buf;
    LPCSTR err=0;

    u32 hr=GetLastError();
	if (hr!=0) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,hr,0,errmsg_buf,1024,0);
        err = errmsg_buf;
	}
    return err;
}

extern void __stdcall xrSkin1W_x86	(vertRender* D, vertBoned1W* S, u32 vCount, CBoneInstance* Bones);
extern void __stdcall xrSkin2W_x86	(vertRender* D, vertBoned2W* S, u32 vCount, CBoneInstance* Bones);

void CEngine::Initialize(void)
{
	// Other stuff
	string_path              fn;
    strconcat               (sizeof(fn),fn,UI->EditorName(),".log");
    FS.update_path			(fn,_local_root_,fn);

#ifdef _EDITOR
	// Bind PSGP
	ZeroMemory				(&PSGP,sizeof(PSGP));
	hPSGP		            = LoadLibrary("xrCPU_Pipe.dll");
	R_ASSERT2	            (hPSGP,"Can't find 'xrCPU_Pipe.dll'");

	xrBinder* bindCPU	    = (xrBinder*)GetProcAddress(hPSGP,"xrBind_PSGP");	R_ASSERT(bindCPU);
	bindCPU		            (&PSGP, CPU::ID.feature /*& CPU::ID.os_support*/);
    // for compliance with editor
    PSGP.skin1W				= xrSkin1W_x86;
    PSGP.skin2W				= xrSkin2W_x86;
#endif

	ReloadSettings			();
}

void CEngine::ReloadSettings()
{
	xr_delete				(pSettings);
    // game configure
    string_path 			si_name;
    FS.update_path			(si_name,"$game_config$","system.ltx");
	pSettings				= xr_new<CInifile>(si_name,TRUE);// FALSE,TRUE,TRUE);
}

void CEngine::Destroy()
{
    xr_delete				(pSettings);
	if (hPSGP)	{ FreeLibrary(hPSGP); hPSGP=0; }
}
