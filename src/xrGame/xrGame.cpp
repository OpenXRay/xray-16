////////////////////////////////////////////////////////////////////////////
//	Module 		: xrGame.cpp
//	Created 	: 07.01.2001
//  Modified 	: 27.05.2004
//	Author		: Aleksandr Maksimchuk and Oles' Shyshkovtsov
//	Description : Defines the entry point for the DLL application.
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "object_factory.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "xr_level_controller.h"
#include "xrEngine/profiler.h"

extern void FillUIStyleToken();
extern void CleanupUIStyleToken();

extern "C" {
DLL_API IFactoryObject* __cdecl xrFactory_Create(CLASS_ID clsid)
{
    IFactoryObject* object = object_factory().client_object(clsid);
#ifdef DEBUG
    if (!object)
        return (0);
#endif
    // XXX nitrocaster XRFACTORY: set clsid during factory initialization
    object->GetClassId() = clsid;
    return (object);
}

DLL_API void __cdecl xrFactory_Destroy(IFactoryObject* O) { xr_delete(O); }
};

void CCC_RegisterCommands();

#ifdef LINUX
__attribute__((constructor))
#endif
static void load(int argc, char** argv, char** envp)
{
    // Fill ui style token
    FillUIStyleToken();
    // register console commands
    CCC_RegisterCommands();
    // keyboard binding
    CCC_RegisterInput();
#ifdef DEBUG
// XXX nitrocaster PROFILER: temporarily disabled due to linkage issues
// g_profiler			= new CProfiler();
#endif
    gStringTable = new CStringTable();
    StringTable().Init();
}

#ifdef LINUX
__attribute__((destructor))
#endif
static void unload()
{
    CleanupUIStyleToken();
    xr_delete(gStringTable);
}

#ifdef WINDOWS
BOOL APIENTRY DllMain(HANDLE hModule, u32 ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        load(0, nullptr, nullptr);
        break;
    }

    case DLL_PROCESS_DETACH:
    {
        unload();
        break;
    }
    }
    return (TRUE);
}
#endif
