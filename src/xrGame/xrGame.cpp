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

#ifdef WINDOWS
BOOL APIENTRY DllMain(HANDLE hModule, u32 ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
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
        break;
    }

    case DLL_PROCESS_DETACH:
    {
        CleanupUIStyleToken();
        xr_delete(gStringTable);
        break;
    }
    }
    return (TRUE);
}
#else
__attribute__((constructor))
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
}

__attribute__((destructor))
static void unload()
{
    CleanupUIStyleToken();
}
#endif
