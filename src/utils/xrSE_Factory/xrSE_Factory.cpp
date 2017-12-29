////////////////////////////////////////////////////////////////////////////
//  Module      : xrSE_Factory.cpp
//  Created     : 18.06.2004
//  Modified    : 18.06.2004
//  Author      : Dmitriy Iassenev
//  Description : Precompiled header creatore
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrSE_Factory.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "object_factory.h"
#include "xrSE_Factory_import_export.h"
#include "script_properties_list_helper.h"
#include "xrCore/ModuleLookup.hpp"

#include "character_info.h"
#include "specific_character.h"

extern CSE_Abstract* F_entity_Create(LPCSTR section);

extern CScriptPropertiesListHelper* g_property_list_helper;
extern std::unique_ptr<XRay::Module> prop_helper_module;

extern "C" {
FACTORY_API IServerEntity* __stdcall create_entity(LPCSTR section) { return F_entity_Create(section); }
FACTORY_API void __stdcall destroy_entity(IServerEntity*& abstract)
{
    auto object = smart_cast<CSE_Abstract*>(abstract);
    F_entity_Destroy(object);
    abstract = 0;
}
};

// !!!!!!! Very ugly fix !!!!!!!
XRay::ScriptExporter::Node* XRay::ScriptExporter::Node::firstNode;
XRay::ScriptExporter::Node* XRay::ScriptExporter::Node::lastNode;
size_t XRay::ScriptExporter::Node::nodeCount;

// typedef void DUMMY_STUFF (const void*,const u32&,void*);
// XRCORE_API DUMMY_STUFF    *g_temporary_stuff;

//#define TRIVIAL_ENCRYPTOR_DECODER

BOOL APIENTRY DllMain(HANDLE module_handle, DWORD call_reason, LPVOID reserved)
{
    switch (call_reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        //g_temporary_stuff = &trivial_encryptor::decode;
        xrDebug::Initialize(false);
        Core.Initialize("xrSE_Factory", nullptr, true, "fsfactory.ltx");
        string_path SYSTEM_LTX;
        FS.update_path(SYSTEM_LTX, "$game_config$", "system.ltx");
        pSettings = new CInifile(SYSTEM_LTX);

        CCharacterInfo::InitInternal();
        CSpecificCharacter::InitInternal();

        break;
    }
    case DLL_PROCESS_DETACH:
    {
        CCharacterInfo::DeleteSharedData();
        CCharacterInfo::DeleteIdToIndexData();
        CSpecificCharacter::DeleteSharedData();
        CSpecificCharacter::DeleteIdToIndexData();

        xr_delete(g_object_factory);
        auto s = (CInifile**)&pSettings;
        xr_delete(*s);
        xr_delete(g_property_list_helper);
        xr_delete(g_ai_space);
        xr_delete(g_object_factory);
        if (prop_helper_module)
            prop_helper_module->close();
        Core._destroy();
        break;
    }
    }
    return (TRUE);
}

void _destroy_item_data_vector_cont(T_VECTOR* vec)
{
    auto it = vec->begin();
    auto it_e = vec->end();

    xr_vector<CUIXml*> _tmp;
    for (; it != it_e; ++it)
    {
        auto it_f = std::find(_tmp.begin(), _tmp.end(), (*it)._xml);
        if (it_f == _tmp.end())
            _tmp.push_back((*it)._xml);
    }
    delete_data(_tmp);
}
