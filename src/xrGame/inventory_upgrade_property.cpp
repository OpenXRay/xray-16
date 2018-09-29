////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade_property.cpp
//	Created 	: 22.11.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade property class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "string_table.h"

#include "inventory_upgrade_property.h"
#include "inventory_upgrade_manager.h"

namespace inventory
{
namespace upgrade
{
Property::Property() {}
Property::~Property() {}
void Property::construct(shared_str const& property_id, Manager& manager_r)
{
    m_id._set(property_id);
    VERIFY2(
        pSettings->section_exist(m_id), make_string("Section of upgrade property [%s] does not exist!", m_id.c_str()));

    m_name = StringTable().translate(pSettings->r_string(id(), "name"));
    m_icon._set(pSettings->r_string(id(), "icon"));

    // functor
    LPCSTR functor_str = pSettings->r_string(id(), "functor");
    m_desc.parameter = "";
    m_desc.parameter2 = id_str();
    R_ASSERT2(GEnv.ScriptEngine->functor(functor_str, m_desc.functr),
        make_string("Failed to get upgrade property functor in section[%s], functor[%s]", id_str(), functor_str));
    m_desc(); // test

    LPCSTR funct_params_str = pSettings->r_string(id(), "params");
    u32 const buffer_size = (xr_strlen(funct_params_str) + 1) * sizeof(char);
    PSTR temp = (PSTR)_alloca(buffer_size);
    for (int n = _GetItemCount(funct_params_str), i = 0; i < n; ++i)
    {
        LPCSTR i_param = (_GetItem(funct_params_str, i, temp, buffer_size));
        m_functor_params.push_back(i_param);
    }
}

bool Property::run_functor(LPCSTR parameter, string256& result)
{
    result[0] = 0;

    m_desc.parameter = parameter;
    LPCSTR functor_res = m_desc(); // execute !!!
    if (!functor_res || !xr_strcmp(functor_res, ""))
    {
        return false;
    }
    xr_strcpy(result, sizeof(result), functor_res);
    return true;
}

} // namespace upgrade
} // namespace inventory
