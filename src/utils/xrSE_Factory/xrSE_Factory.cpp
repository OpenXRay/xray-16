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
#include "xrCore/ModuleLookup.hpp"
#include "xrScriptEngine/script_engine.hpp"
#include "xrServerEntities/object_factory.h"
#include "xrServerEntities/character_info.h"
#include "xrServerEntities/specific_character.h"
#include "xrSE_Factory_import_export.h"
#include "script_properties_list_helper.h"

extern CSE_Abstract* F_entity_Create(LPCSTR section, bool no_assert);

extern CScriptPropertiesListHelper* g_property_list_helper;
extern XRay::Module prop_helper_module;

namespace xrSE_Factory
{
IServerEntity* create_entity(LPCSTR section) { return F_entity_Create(section, true); }

void destroy_entity(IServerEntity*& abstract)
{
    auto object = smart_cast<CSE_Abstract*>(abstract);
    F_entity_Destroy(object);
    abstract = 0;
}

void initialize()
{
    string_path SYSTEM_LTX;
    FS.update_path(SYSTEM_LTX, "$game_config$", "system.ltx");
    pSettings = xr_new<CInifile>(SYSTEM_LTX);

    CCharacterInfo::InitInternal();
    CSpecificCharacter::InitInternal();
}

void destroy()
{
    CCharacterInfo::DeleteSharedData();
    CCharacterInfo::DeleteIdToIndexData();
    CSpecificCharacter::DeleteSharedData();
    CSpecificCharacter::DeleteIdToIndexData();

    auto s = (CInifile**)&pSettings;
    xr_delete(*s);
    xr_delete(g_property_list_helper);
    xr_delete(g_ai_space);
    xr_delete(g_object_factory);
    prop_helper_module = nullptr;
}

}; // xrSE_Factory

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
