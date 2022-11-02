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
#include "xrEngine/xr_level_controller.h"
#include "xrEngine/profiler.h"

extern void FillUIStyleToken();
extern void CleanupUIStyleToken();

void CCC_RegisterCommands();

extern float g_fTimeFactor;

extern "C"
{
    XR_EXPORT IFactoryObject* __cdecl xrFactory_Create(CLASS_ID clsid)
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

    XR_EXPORT void __cdecl xrFactory_Destroy(IFactoryObject* O) { xr_delete(O); }

    XR_EXPORT void initialize_library()
    {
        g_fTimeFactor = pSettings->r_float("alife", "time_factor"); // XXX: find a better place

        // Fill ui style token
        FillUIStyleToken();
        // register console commands
        CCC_RegisterCommands();
        // register localization
        StringTable().Init();
        // keyboard binding
        CCC_RegisterInput(); // XXX: Move to xrEngine
#ifdef DEBUG
        g_profiler = xr_new<CProfiler>();
#endif
    }

    XR_EXPORT void finalize_library()
    {
        CleanupUIStyleToken();
        StringTable().Destroy();
        CCC_DeregisterInput(); // XXX: Remove if possible
 }
}
