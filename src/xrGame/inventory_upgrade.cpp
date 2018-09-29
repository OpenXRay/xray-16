////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade.cpp
//	Created 	: 01.11.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "string_table.h"

#include "inventory_upgrade.h"
#include "inventory_upgrade_manager.h"
#include "inventory_upgrade_group.h"
#include "inventory_upgrade_root.h"
#include "inventory_upgrade_property.h"

namespace inventory
{
namespace upgrade
{
Upgrade::Upgrade() {}
Upgrade::~Upgrade() {}
void Upgrade::construct(const shared_str& upgrade_id, Group& parental_group, Manager& manager_r)
{
    inherited::construct(upgrade_id, manager_r);
    m_parent_group = &parental_group;

    // name : StringTable(); icon; description;
    m_name = StringTable().translate(pSettings->r_string(id(), "name"));
    m_description = StringTable().translate(pSettings->r_string(id(), "description"));
    m_icon._set(pSettings->r_string(id(), "icon"));

    // section --------------------------------------------------------------------------
    LPCSTR section_str = pSettings->r_string(id(), "section");
    VERIFY2(pSettings->section_exist(section_str),
        make_string("Upgrade <%s> : settings section [%s] not exist!", id_str(), section_str));
    VERIFY2(pSettings->line_count(section_str),
        make_string("Upgrade <%s> : settings section [%s] is empty !", id_str(), section_str));

    m_section._set(section_str);

    // precondition_functor
    LPCSTR precondition_functor_str = pSettings->r_string(id(), "precondition_functor");
    m_preconditions.parameter = pSettings->r_string(id(), "precondition_parameter");
    m_preconditions.parameter2 = m_section.c_str();
    R_ASSERT2(GEnv.ScriptEngine->functor(precondition_functor_str, m_preconditions.functr),
        make_string(
            "Failed to get precondition functor in section[%s], functor[%s]", id_str(), precondition_functor_str));
    m_preconditions();

    // effect_functor
    LPCSTR effect_functor_str = pSettings->r_string(id(), "effect_functor");
    m_effects.parameter = pSettings->r_string(id(), "effect_parameter");
    m_effects.parameter2 = m_section.c_str();
    m_effects.parameter3 = 1;
    R_ASSERT2(GEnv.ScriptEngine->functor(effect_functor_str, m_effects.functr),
        make_string("Failed to get effect functor in section[%s], functor[%s]", id_str(), effect_functor_str));
    m_effects();

    // prereq_functor (1,2) : m_prerequisites, m_tooltip
    LPCSTR prereq_functor_str = pSettings->r_string(id(), "prereq_functor"); // prerequisites_functor
    //	LPCSTR tooltip_functor_str	= pSettings->r_string( id(), "prereq_tooltip_functor" );
    m_prerequisites.parameter = pSettings->r_string(id(), "prereq_params"); // prerequisites_params
    m_prerequisites.parameter2 = m_section.c_str();
    //	m_tooltip.parameter			= pSettings->r_string( id(), "prereq_params" );
    R_ASSERT2(GEnv.ScriptEngine->functor(prereq_functor_str, m_prerequisites.functr),
        make_string("Failed to get prerequisites functor in section[%s], functor[%s]", id_str(), prereq_functor_str));
    m_prerequisites();

    /*R_ASSERT2(
        GEnv.ScriptEngine->functor( tooltip_functor_str, m_tooltip.functr ),
        make_string( "Failed to get tooltip functor in section[%s], functor[%s]",
        id_str(), tooltip_functor_str
        )
    );
    m_tooltip();*/

    // effects = groups
    LPCSTR groups_str = pSettings->r_string(id(), "effects");
    if (groups_str)
    {
        add_dependent_groups(groups_str, manager_r);
    }

    m_known = !!READ_IF_EXISTS(pSettings, r_bool, id(), "known", false);

    shared_str properties = pSettings->r_string(id(), "property");
    VERIFY2(properties.size(), make_string("Upgrade <%s> : property is empty !", id_str()));

    string256 buffer;
    for (u8 i = 0; i < max_properties_count; i++)
    {
        shared_str prop = _GetItem(properties.c_str(), i, buffer);
        if (prop.size())
        {
            m_properties[i] = prop;
            VERIFY2(manager_r.get_property(prop),
                make_string("Upgrade <%s> : property [%s] is unknown (not found in upgrade manager) !", id_str(),
                    prop.c_str()));
        }
    }

    m_scheme_index.set(-1, -1);
    m_scheme_index = pSettings->r_ivector2(id(), "scheme_index");

    m_highlight = false;
} // Upgrade()

#ifdef DEBUG

void Upgrade::log_hierarchy(LPCSTR nest)
{
    u32 sz = (xr_strlen(nest) + 4) * sizeof(char);
    PSTR nest2 = (PSTR)_alloca(sz);
    xr_strcpy(nest2, sz, nest);
    xr_strcat(nest2, sz, "   ");
    Msg("%s<u> %s", nest2, id_str());

    inherited::log_hierarchy(nest2);
}

#endif // DEBUG

void Upgrade::fill_root_container(Root* root)
{
    R_ASSERT(root);
    root->add_upgrade(this);
    inherited::fill_root_container(root);
}

UpgradeStateResult Upgrade::can_install(CInventoryItem& item, bool loading)
{
    UpgradeStateResult res = inherited::can_install(item, loading);
    if (res != result_ok)
    {
        return res;
    }

    res = m_parent_group->can_install(item, *this, loading);
    if (res != result_ok)
    {
        return res;
    }

    if (loading)
    {
        return result_ok; // later script check
    }

    int res_prec = m_preconditions();
    if (res_prec == 0)
    {
        return result_ok;
    }
    else if (res_prec == 1)
    {
        return result_e_precondition_money;
    }
    else if (res_prec == 2)
    {
        return result_e_precondition_quest;
    }

    return result_ok;
}

bool Upgrade::check_scheme_index(Ivector2 const& scheme_index)
{
    return (m_scheme_index.x == scheme_index.x && m_scheme_index.y == scheme_index.y);
}

LPCSTR Upgrade::get_prerequisites() { return m_prerequisites(); }
UpgradeStateResult Upgrade::get_preconditions()
{
    int res_prec = m_preconditions();
    if (res_prec == 0)
    {
        return result_ok;
    }
    else if (res_prec == 1)
    {
        return result_e_precondition_money;
    }
    else if (res_prec == 2)
    {
        return result_e_precondition_quest;
    }

    return result_ok;
}

void Upgrade::run_effects(bool loading)
{
    m_effects.parameter3 = loading ? 1 : 0;
    m_effects();
}

void Upgrade::set_highlight(bool value) { m_highlight = value; }
void Upgrade::highlight_up()
{
    set_highlight(true);
    Groups_type::iterator ib = m_depended_groups.begin();
    Groups_type::iterator ie = m_depended_groups.end();
    for (; ib != ie; ++ib)
    {
        (*ib)->highlight_up();
    }
}

void Upgrade::highlight_down()
{
    set_highlight(true);
    m_parent_group->highlight_down();
}

} // namespace upgrade
} // namespace inventory
