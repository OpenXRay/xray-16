////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_object.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife object class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xrServer_Objects_ALife.h"
#include "alife_simulator.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrCommon/xr_vector.h"

void CSE_ALifeObject::spawn_supplies() { spawn_supplies(*m_ini_string); }

void CSE_ALifeObject::spawn_supplies(LPCSTR ini_string)
{
    if (!ini_string)
        return;

    if (!xr_strlen(ini_string))
        return;

#pragma warning(push)
#pragma warning(disable : 4238)
    IReader reader((void*)ini_string, xr_strlen(ini_string));
    CInifile ini(&reader, FS.get_path("$game_config$")->m_Path);
#pragma warning(pop)

    u8 loadout_index = 1;
    LPCSTR loadout_section = "spawn_loadout";

    while (ini.section_exist(loadout_section))
    {
        LPCSTR level_name = *ai().game_graph().header().level(ai().game_graph().vertex(m_tGraphID)->level_id()).name();
        LPCSTR item_section, item_line;
        xr_vector<u32> spawn_loadouts;

        for (u32 i = 0; ini.r_line(loadout_section, i, &item_section, &item_line); i++)
        {
            // If level=<level_name> then only spawn items if object on that level
            if (strstr(item_line, "level=") != nullptr)
            {
                if (strstr(item_line, level_name) != nullptr)
                {
                    spawn_loadouts.push_back(i);
                }
            }
            else
            {
                spawn_loadouts.push_back(i);
            }
        }

        if (spawn_loadouts.size())
        {
            s32 selected_loadout_index = ::Random.randI(0, spawn_loadouts.size() - 1);

            if (ini.r_line(loadout_section, spawn_loadouts.at(selected_loadout_index), &item_section, &item_line))
            {
                VERIFY(xr_strlen(item_section));

                if (pSettings->section_exist(item_section))
                {
                    u32 spawn_count = 1;
                    bool bScope = false;
                    bool bSilencer = false;
                    bool bLauncher = false;
                    float fCondition = 1.0f;
                    int iAmmoType = 0, n = 0;

                    if (item_line && xr_strlen(item_line))
                    {
                        n = _GetItemCount(item_line);
                        if (n > 0)
                        {
                            string64 tmp;
                            spawn_count = atoi(_GetItem(item_line, 0, tmp));
                        }

                        if (!spawn_count)
                            spawn_count = 1;
                        if (nullptr != strstr(item_line, "cond="))
                            fCondition = (float)atof(strstr(item_line, "cond=") + 5);
                        bScope = (nullptr != strstr(item_line, "scope"));
                        bSilencer = (nullptr != strstr(item_line, "silencer"));
                        bLauncher = (nullptr != strstr(item_line, "launcher"));
                        if (nullptr != strstr(item_line, "ammo_type="))
                            iAmmoType = atoi(strstr(item_line, "ammo_type=") + 10);
                    }

                    CSE_Abstract* E = alife().spawn_item(item_section, o_Position, m_tNodeID, m_tGraphID, ID);
                    CSE_ALifeItemWeapon* W = smart_cast<CSE_ALifeItemWeapon*>(E);

                    if (W)
                    {
                        if (W->m_scope_status == ALife::eAddonAttachable)
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonScope, bScope);
                        if (W->m_silencer_status == ALife::eAddonAttachable)
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSilencer, bSilencer);
                        if (W->m_grenade_launcher_status == ALife::eAddonAttachable)
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher, bLauncher);

                        // spawn count box(es) of the correct ammo for weapon
                        if (pSettings->line_exist(item_section, "ammo_class"))
                        {
                            LPCSTR ammo_class = pSettings->r_string(item_section, "ammo_class");
                            LPCSTR ammo_section = "";
                            for (int i = 0, n = _GetItemCount(ammo_class); i < n; ++i)
                            {
                                string128 tmp;
                                ammo_section = _GetItem(ammo_class, i, tmp);
                                if (i == iAmmoType)
                                    break;
                            }
                            if (xr_strlen(ammo_section) && pSettings->section_exist(ammo_section))
                            {
                                for (u32 i = 1; i <= spawn_count; ++i)
                                {
                                    alife().spawn_item(ammo_section, o_Position, m_tNodeID, m_tGraphID, ID);
                                }
                            }
                        }
                    }
                    CSE_ALifeInventoryItem* IItem = smart_cast<CSE_ALifeInventoryItem*>(E);

                    if (IItem)
                    {
                        IItem->m_fCondition = fCondition;
                    }
                }
            }
        }

        loadout_index += 1;
        string32 buf;
        loadout_section = strconcat(sizeof(buf), buf, "spawn_loadout", std::to_string(loadout_index).c_str());
    }
    // -Alundaio

    if (ini.section_exist("spawn"))
    {
        pcstr N, V;
        float p;
        for (u32 k = 0, j; ini.r_line("spawn", k, &N, &V); k++)
        {
            VERIFY(xr_strlen(N));

            if (pSettings->section_exist(N)) // Alundaio: verify item section exists!
            {
                float f_cond = 1.0f;
                bool bScope = false;
                bool bSilencer = false;
                bool bLauncher = false;

                j = 1;
                p = 1.f;

                if (V && xr_strlen(V))
                {
                    string64 buf;
                    j = atoi(_GetItem(V, 0, buf));
                    if (!j)
                        j = 1;

                    bScope = nullptr != strstr(V, "scope");
                    bSilencer = nullptr != strstr(V, "silencer");
                    bLauncher = nullptr != strstr(V, "launcher");
                    // probability
                    if (nullptr != strstr(V, "prob="))
                        p = static_cast<float>(atof(strstr(V, "prob=") + 5));
                    if (fis_zero(p))
                        p = 1.0f;
                    if (nullptr != strstr(V, "cond="))
                        f_cond = static_cast<float>(atof(strstr(V, "cond=") + 5));
                }
                for (u32 i = 0; i < j; ++i)
                {
                    if (randF(1.f) < p)
                    {
                        CSE_Abstract* E = alife().spawn_item(N, o_Position, m_tNodeID, m_tGraphID, ID);
                        // подсоединить аддоны к оружию, если включены соответствующие флажки
                        CSE_ALifeItemWeapon* W = smart_cast<CSE_ALifeItemWeapon*>(E);
                        if (W)
                        {
                            if (W->m_scope_status == ALife::eAddonAttachable)
                                W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonScope, bScope);
                            if (W->m_silencer_status == ALife::eAddonAttachable)
                                W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSilencer, bSilencer);
                            if (W->m_grenade_launcher_status == ALife::eAddonAttachable)
                                W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher, bLauncher);
                        }
                        CSE_ALifeInventoryItem* IItem = smart_cast<CSE_ALifeInventoryItem*>(E);
                        if (IItem)
                            IItem->m_fCondition = f_cond;
                    }
                }
            }
        }
    }
}

bool CSE_ALifeObject::keep_saved_data_anyway() const /* noexcept */ { return false; }
