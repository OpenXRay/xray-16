////////////////////////////////////////////////////////////////////////////
//	Module 		: xrGame.cpp
//	Created 	: 07.01.2001
//  Modified 	: 27.05.2004
//	Author		: Aleksandr Maksimchuk and Oles' Shyshkovtsov
//	Description : Defines the entry point for the DLL application.
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "object_factory.h"
#include "ui/xrUIXmlParser.h"
#include "xr_level_controller.h"
#include "profiler.h"

#include "lua/library_linkage.h"
#include "luabind/library_linkage.h"

//#pragma comment(lib,"ode.lib")
#pragma comment(lib,"xrEngine.lib")

extern "C" {
	DLL_API DLL_Pure*	__cdecl xrFactory_Create		(CLASS_ID clsid)
	{
		DLL_Pure			*object = object_factory().client_object(clsid);
#ifdef DEBUG
		if (!object)
			return			(0);
#endif
		object->CLS_ID		= clsid;
		return				(object);
	}

	DLL_API void		__cdecl	xrFactory_Destroy		(DLL_Pure* O)
	{
		xr_delete			(O);
	}
};

void CCC_RegisterCommands	();
void setup_luabind_allocator();

BOOL APIENTRY DllMain(HANDLE hModule, u32 ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH: {
			// register console commands
			CCC_RegisterCommands();
			// keyboard binding
			CCC_RegisterInput	();

			setup_luabind_allocator	();
#ifdef DEBUG
			g_profiler			= xr_new<CProfiler>();
#endif
			break;
		}

		case DLL_PROCESS_DETACH: {
			break;
		}
	}
    return								(TRUE);
}
