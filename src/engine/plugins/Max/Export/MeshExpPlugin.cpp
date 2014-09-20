// file: MeshExpPlugin.cpp

#include "stdafx.h"
#pragma hdrstop

#include "MeshExpUtility.h"


//-------------------------------------------------------------------
// Class Descriptor

class MeshExpUtilityClassDesc : public ClassDesc {
	public:
	int 			IsPublic()					{ return 1; }
	void *			Create( BOOL loading )		{ return &U; }
	const TCHAR *	ClassName()					{ return "S.T.A.L.K.E.R. Export"; }
	SClass_ID		SuperClassID()				{ return UTILITY_CLASS_ID; }
	Class_ID 		ClassID()					{ return Class_ID(EXP_UTILITY_CLASSID,0); }
	const TCHAR* 	Category()					{ return "S.T.A.L.K.E.R. Export";  }
};

MeshExpUtility U;
MeshExpUtilityClassDesc MeshExpUtilityClassDescCD;

//-------------------------------------------------------------------
// DLL interface
HINSTANCE hInstance;
int controlsInit = FALSE;


BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
{
	hInstance = hinstDLL;

	if ( !controlsInit ) {
		controlsInit = TRUE;
		Core._initialize("S.T.A.L.K.E.R.Plugin",ELogCallback,FALSE);
		FS._initialize	(CLocatorAPI::flScanAppRoot,NULL,"xray_path.ltx");
		FPU::m64r	(); // нужно чтобы макс не сбрасывал контрольки в 0
		InitCustomControls(hInstance);
		InitCommonControls();
		ELog.Msg(mtInformation,"S.T.A.L.K.E.R. Object Export (ver. %d.%02d)",EXPORTER_VERSION,EXPORTER_BUILD);
		ELog.Msg(mtInformation,"-------------------------------------------------------" );
	}

	switch(fdwReason) {

		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			Core._destroy();
			break;
		}
	return(TRUE);
}



__declspec( dllexport ) const TCHAR *
LibDescription() { return "S.T.A.L.K.E.R. Mesh Export utility"; }


__declspec( dllexport ) int LibNumberClasses() {
	return 1;
}


__declspec( dllexport ) ClassDesc* LibClassDesc(int i) {
	switch(i) {
		case 0: return &MeshExpUtilityClassDescCD;
		default: return 0;
	}
}


__declspec( dllexport ) ULONG LibVersion() 
{
	return VERSION_3DSMAX; 
}


