#include "stdafx.h"
#include "LW_Shader.h"
#include "..\..\Shared\BlenderListLoader.h"

extern "C" { EShaderList ENShaders; EShaderList LCShaders; EShaderList GameMtls;}

extern "C" {
	void __cdecl LoadShaders()
	{
		Core._initialize("XRayPlugin",0,FALSE);
		FS._initialize	(CLocatorAPI::flScanAppRoot,NULL,"xray_path.ltx");
		LPSTRVec lst;
		ENShaders.count=LoadBlenderList(lst);
		for (LPSTRIt b_it=lst.begin(); b_it!=lst.end(); b_it++)
			strcpy(ENShaders.Names[b_it-lst.begin()],*b_it);
		ClearList(lst);

		LCShaders.count=LoadShaderLCList(lst);
		for (LPSTRIt c_it=lst.begin(); c_it!=lst.end(); c_it++)
			strcpy(LCShaders.Names[c_it-lst.begin()],*c_it);
		ClearList(lst);

		GameMtls.count=LoadGameMtlList(lst);
		for (LPSTRIt g_it=lst.begin(); g_it!=lst.end(); g_it++)
			strcpy(GameMtls.Names[g_it-lst.begin()],*g_it);
		ClearList(lst);
	}
};