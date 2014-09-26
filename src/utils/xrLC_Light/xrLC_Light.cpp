// xrLC_Light.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "xrLc_globaldata.h"
#pragma comment(lib,"xrCore.lib")
#pragma comment(lib,"xrCDB.lib")
#pragma comment(lib,"FreeImage.lib")
#ifdef _MANAGED
#pragma managed(push, off)
#endif

i_lc_log *lc_log	= 0;

void __cdecl	clLog( const char *format, ...)
{
	va_list		mark;
	char buf	[4*256];
	va_start	( mark, format );
	vsprintf	( buf, format, mark );

	if(!lc_log)
	{
		Msg( "clMsg: %s", buf );
		return;
	}

	VERIFY			( lc_log );
	lc_log->clLog	( buf );
}
void __cdecl clMsg( const char *format, ...)
{
	va_list		mark;
	char buf	[4*256];
	va_start	( mark, format );
	vsprintf	( buf, format, mark );

	if(!lc_log)
	{
		Msg( "clMsg: %s", buf );
		return;
	}

	VERIFY			( lc_log );
	lc_log->clMsg	( buf );
}

void __cdecl Status	(const char *format, ...)
{
	va_list				mark;
	va_start			( mark, format );

	char				status	[1024	]	="";
	vsprintf			( status, format, mark );

	if(!lc_log)
	{
		Msg( "Status: %s", status );
		return;
	}

	//strconcat			( sizeof(status), status, "    | %s", status ); 
	VERIFY				( lc_log );
	lc_log->Status		(status);
	
}
void Phase		( LPCSTR phase_name )
{
	if(!lc_log)
	{
		Msg( "Phase: %s", phase_name );
		return;
	}
	
	VERIFY				( lc_log );
	lc_log->Phase		( phase_name );
}
void Progress	( const float F )
{
	if(!lc_log)
	{
		Msg( "Progress: %f", F );
		return;
	}

	VERIFY				( lc_log );
	lc_log->Progress		( F );
}

b_params	&g_params()
{
	VERIFY(inlc_global_data());
	return inlc_global_data()->g_params();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    
		switch(ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			{
				Debug._initialize	(false);
				bool init_log	=  (0 != xr_strcmp( Core.ApplicationName, "XRayEditorTools" ));
				Core._initialize	("xrLC_Light",0,FALSE);
				if( init_log )
					CreateLog( );
				
				//FPU::m64r	();
				break;
			}
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			if(inlc_global_data())
				destroy_global_data();
			Core._destroy();
			break;
	}
	return TRUE;
	

}

#ifdef _MANAGED
#pragma managed(pop)
#endif

