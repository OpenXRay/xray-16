////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item_upgrade.cpp
//	Created 	: 27.11.2007
//  Modified 	: 27.11.2007
//	Author		: Sokolov Evgeniy
//	Description : Inventory item upgrades class impl
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "inventory_item.h"
#include "inventory_item_impl.h"

#include "xrAICore/Navigation/ai_object_location.h"
#include "Common/object_broker.h"

#include "ai_space.h"
#include "alife_simulator.h"
#include "inventory_upgrade_manager.h"
#include "inventory_upgrade.h"
#include "Level.h"
#include "WeaponMagazinedWGrenade.h"

extern int g_normalize_mouse_sens;
extern int g_normalize_upgrade_mouse_sens;

bool CInventoryItem::has_upgrade_group(const shared_str& upgrade_group_id)
{
    auto it = m_upgrades.cbegin();
    auto it_e = m_upgrades.cend();

    for (; it != it_e; ++it)
    {
        inventory::upgrade::Upgrade* upgrade = ai().alife().inventory_upgrade_manager().get_upgrade(*it);
        if (upgrade->parent_group_id() == upgrade_group_id)
            return true;
    }
    return false;
}

bool CInventoryItem::has_upgrade_group_by_upgrade_id(const shared_str& upgrade_id)
{
    inventory::upgrade::Upgrade* upgrade = ai().alife().inventory_upgrade_manager().get_upgrade(upgrade_id);

    if (!upgrade)
    {
        return false;
    }

    return has_upgrade_group(upgrade->parent_group_id());
}

bool CInventoryItem::has_upgrade(const shared_str& upgrade_id)
{
    if (m_section_id == upgrade_id)
    {
        return true;
    }
    return (std::find(m_upgrades.begin(), m_upgrades.end(), upgrade_id) != m_upgrades.end());
}

void CInventoryItem::add_upgrade(const shared_str& upgrade_id, bool loading)
{
    if (!has_upgrade(upgrade_id))
    {
        m_upgrades.push_back(upgrade_id);

        if (!loading)
        {
            NET_Packet P;
            CGameObject::u_EventGen(P, GE_INSTALL_UPGRADE, object_id());
            P.w_stringZ(upgrade_id);
            CGameObject::u_EventSend(P);
        }
    }
}

bool CInventoryItem::get_upgrades_str(string2048& res) const
{
    int prop_count = 0;
    res[0] = 0;
    auto ib = m_upgrades.cbegin();
    auto ie = m_upgrades.cend();
    inventory::upgrade::Upgrade* upgr;
    for (; ib != ie; ++ib)
    {
        upgr = ai().alife().inventory_upgrade_manager().get_upgrade(*ib);
        if (!upgr)
        {
            continue;
        }

        LPCSTR upgr_section = upgr->section();
        if (prop_count > 0)
        {
            xr_strcat(res, sizeof(res), ", ");
        }
        xr_strcat(res, sizeof(res), upgr_section);
        ++prop_count;
    }
    if (prop_count > 0)
    {
        return true;
    }
    return false;
}

bool CInventoryItem::equal_upgrades(Upgrades_type const& other_upgrades) const
{
    if (m_upgrades.size() != other_upgrades.size())
    {
        return false;
    }

    auto ib = m_upgrades.cbegin();
    auto ie = m_upgrades.cend();
    for (; ib != ie; ++ib)
    {
        shared_str const& name1 = (*ib);
        bool upg_equal = false;
        auto ib2 = other_upgrades.cbegin();
        auto ie2 = other_upgrades.cend();
        for (; ib2 != ie2; ++ib2)
        {
            if (name1.equal((*ib2)))
            {
                upg_equal = true;
                break; // from for2, in for1
            }
        }
        if (!upg_equal)
        {
            return false;
        }
    }
    return true;
}

#ifdef DEBUG
void CInventoryItem::log_upgrades()
{
    Msg("* all upgrades of item = %s", m_section_id.c_str());
    auto ib = m_upgrades.cbegin();
    auto ie = m_upgrades.cend();
    for (; ib != ie; ++ib)
    {
        Msg("    %s", (*ib).c_str());
    }
    Msg("* finish - upgrades of item = %s", m_section_id.c_str());
}
#endif // DEBUG

void CInventoryItem::net_Spawn_install_upgrades(CSE_Abstract* DC) // net_Spawn
{
    if (!IsGameTypeSingle() || !ai().get_alife())
    {
        return;
    }

    CSE_ALifeInventoryItem* pSE_InventoryItem = smart_cast<CSE_ALifeInventoryItem*>(DC);
    if (!pSE_InventoryItem)
    {
        return;
    }

    Upgrades_type saved_upgrades = pSE_InventoryItem->m_upgrades;

    m_upgrades.clear();

    ai().alife().inventory_upgrade_manager().init_install(*this); // from pSettings

    for (const auto& upgrade : saved_upgrades)
    {
        ai().alife().inventory_upgrade_manager().upgrade_install(*this, *upgrade, true);
    }
}

bool CInventoryItem::install_upgrade(LPCSTR section) { return install_upgrade_impl(section, false); }
bool CInventoryItem::verify_upgrade(LPCSTR section) { return install_upgrade_impl(section, true); }

bool CInventoryItem::install_upgrade_impl(LPCSTR section, bool test)
{
    bool result = process_if_exists(section, "cost", &CInifile::r_u32, m_cost, test);
    result |= process_if_exists(section, "inv_weight", &CInifile::r_float, m_weight, test);

    bool result2 = false;
    if (BaseSlot() != NO_ACTIVE_SLOT)
    {
        bool value = m_flags.test(FRuckDefault);
        result2 = process_if_exists_set(section, "default_to_ruck", &CInifile::r_bool, value, test);
        if (result2 && !test)
        {
            m_flags.set(FRuckDefault, value);
        }
        result |= result2;

        value = m_flags.test(FAllowSprint);
        result2 = process_if_exists_set(section, "sprint_allowed", &CInifile::r_bool, value, test);
        if (result2 && !test)
        {
            m_flags.set(FAllowSprint, value);
        }
        result |= result2;

        if (!g_normalize_upgrade_mouse_sens)
        {
            result |= process_if_exists(section, "control_inertion_factor", &CInifile::r_float, m_fControlInertionFactor, test);
        }

        const bool needsNormalizedUpgradeSens = g_normalize_mouse_sens && !g_normalize_upgrade_mouse_sens;
        if (needsNormalizedUpgradeSens)
        {
            if (m_fControlInertionFactor < 0.f)
            {
                float upgr_factor = pSettings->read_if_exists<float>(section, "control_inertion_factor", 1.0f);
                if (abs(upgr_factor) > 1)
                    upgr_factor /= 4;
                else if (abs(upgr_factor) >= 0.5)
                    upgr_factor /= 3;
                else if (abs(upgr_factor) > 0.1)
                    upgr_factor /= 2;

                m_fControlInertionFactor = 1.f + upgr_factor;
                clamp(m_fControlInertionFactor, 0.1f, 1.f);
            }
        }
    }

    pcstr str = nullptr;
    result2 = process_if_exists_set(section, "immunities_sect", &CInifile::r_string, str, test);
    if (result2 && !test)
        CHitImmunity::LoadImmunities(str, pSettings);

    result2 = process_if_exists_set(section, "immunities_sect_add", &CInifile::r_string, str, test);
    if (result2 && !test)
        CHitImmunity::AddImmunities(str, pSettings);

    return result;
}

void CInventoryItem::pre_install_upgrade()
{
    CWeaponMagazined* wm = smart_cast<CWeaponMagazined*>(this);
    if (wm)
    {
        wm->UnloadMagazine();

        CWeaponMagazinedWGrenade* wg = smart_cast<CWeaponMagazinedWGrenade*>(this);
        if (wg)
        {
            if (wg->IsGrenadeLauncherAttached())
            {
                wg->PerformSwitchGL();
                wg->UnloadMagazine();
                wg->PerformSwitchGL(); // restore state
            }
        }
    }

    CWeapon* weapon = smart_cast<CWeapon*>(this);
    if (weapon)
    {
        if (weapon->ScopeAttachable() && weapon->IsScopeAttached())
        {
            weapon->Detach(weapon->GetScopeName().c_str(), true);
        }
        if (weapon->SilencerAttachable() && weapon->IsSilencerAttached())
        {
            weapon->Detach(weapon->GetSilencerName().c_str(), true);
        }
        if (weapon->GrenadeLauncherAttachable() && weapon->IsGrenadeLauncherAttached())
        {
            weapon->Detach(weapon->GetGrenadeLauncherName().c_str(), true);
        }
    }
}
