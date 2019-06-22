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

    // Alundaio: This will spawn a single random section listed in [spawn_loadout]
    // No need to spawn ammo, this will automatically spawn 1 box for weapon and if ammo_type is specified it will spawn that type
    // count is used only for ammo boxes (ie wpn_pm = 3) will spawn 3 boxes, not 3 wpn_pm
    // Usage: to create random weapon loadouts
    if (ini.section_exist("spawn_loadout"))
    {
        pcstr itmSection, V;
        xr_vector<u32> OnlyOne;

        pcstr lname = *ai().game_graph().header().level(ai().game_graph().vertex(m_tGraphID)->level_id()).name();

        for (u32 k = 0; ini.r_line("spawn_loadout", k, &itmSection, &V); k++)
        {
            // If level=<lname> then only spawn items if object on that level
            if (strstr(V, "level=") != nullptr)
                if (strstr(V, lname) != nullptr)
                    OnlyOne.push_back(k);
                else
                    OnlyOne.push_back(k);
        }

        if (!OnlyOne.empty())
        {
            s32 sel = Random.randI(0, OnlyOne.size());
            if (ini.r_line("spawn_loadout", OnlyOne.at(sel), &itmSection, &V))
            {
                VERIFY(xr_strlen(itmSection));
                if (pSettings->section_exist(itmSection))
                {
                    u32 spawn_count = 1;
                    bool bScope = false;
                    bool bSilencer = false;
                    bool bLauncher = false;
                    float f_cond = 1.0f;
                    int i_ammo_type = 0, n = 0;

                    if (V && xr_strlen(V))
                    {
                        n = _GetItemCount(V);
                        if (n > 0)
                        {
                            string64 tmp;
                            _GetItem(V, 0, tmp);
                            std::from_chars(tmp, tmp + xr_strlen(tmp), spawn_count); // count
                        }

                        if (!spawn_count)
                            spawn_count = 1;
                        {
                            const char* cond_c = strstr(V, "cond=");
                            if (nullptr != cond_c)
                                std::from_chars(cond_c + 5, cond_c + 5 + xr_strlen(cond_c + 5), f_cond);
                        }
                        bScope = nullptr != strstr(V, "scope");
                        bSilencer = nullptr != strstr(V, "silencer");
                        bLauncher = nullptr != strstr(V, "launcher");
                        {
                            const char* ammo_type_c = strstr(V, "cond=");
                            if (nullptr != ammo_type_c)
                                std::from_chars(
                                    ammo_type_c + 10, ammo_type_c + 10 + xr_strlen(ammo_type_c + 10), i_ammo_type);
                        }
                    }

                    CSE_Abstract* E = alife().spawn_item(itmSection, o_Position, m_tNodeID, m_tGraphID, ID);
                    CSE_ALifeItemWeapon* W = smart_cast<CSE_ALifeItemWeapon*>(E);
                    if (W)
                    {
                        if (W->m_scope_status == ALife::eAddonAttachable)
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonScope, bScope);
                        if (W->m_silencer_status == ALife::eAddonAttachable)
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSilencer, bSilencer);
                        if (W->m_grenade_launcher_status == ALife::eAddonAttachable)
                            W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher, bLauncher);

                        //spawn count box(es) of the correct ammo for weapon
                        if (pSettings->line_exist(itmSection, "ammo_class"))
                        {
                            pcstr ammo_class = pSettings->r_string(itmSection, "ammo_class");
                            pcstr ammoSec = "";
                            for (int i = 0, n = _GetItemCount(ammo_class); i < n; ++i)
                            {
                                string128 tmp;
                                ammoSec = _GetItem(ammo_class, i, tmp);
                                if (i == i_ammo_type)
                                    break;
                            }
                            if (xr_strlen(ammoSec) && pSettings->section_exist(ammoSec))
                                for (u32 i = 1; i <= spawn_count; ++i)
                                    alife().spawn_item(ammoSec, o_Position, m_tNodeID, m_tGraphID, ID);
                        }
                    }
                    CSE_ALifeInventoryItem* IItem = smart_cast<CSE_ALifeInventoryItem*>(E);
                    if (IItem)
                        IItem->m_fCondition = f_cond;
                }
            }
        }
    }
    //-Alundaio

    if (ini.section_exist("spawn"))
    {
        pcstr N, V;
        float p;
        for (u32 k = 0, j; ini.r_line("spawn", k, &N, &V); k++)
        {
            VERIFY(xr_strlen(N));

            if (pSettings->section_exist(N)) //Alundaio: verify item section exists!
            {
                float f_cond = 1.0f;
                bool bScope = false;
                bool bSilencer = false;
                bool bLauncher = false;

                j = 1;
                p = 1.f;

                if (V && xr_strlen(V))
                {
                    {
                        string64 buf;
                        _GetItem(V, 0, buf);
                        std::from_chars(buf, buf + xr_strlen(buf), j);
                    }
                    if (!j)
                        j = 1;

                    bScope = nullptr != strstr(V, "scope");
                    bSilencer = nullptr != strstr(V, "silencer");
                    bLauncher = nullptr != strstr(V, "launcher");
                    // probability
                    {
                        const char* prob_c = strstr(V, "prob=");
                        if (prob_c != nullptr)
                            std::from_chars(prob_c + 5, prob_c + 5 + xr_strlen(prob_c + 5), p);
                    }
                    if (fis_zero(p))
                        p = 1.0f;
                    {
                        const char* cond_c = strstr(V, "cond=");
                        if (nullptr != cond_c)
                            std::from_chars(cond_c + 5, cond_c + 5 + xr_strlen(cond_c + 5), f_cond);
                    }
                }
                for (u32 i = 0; i < j; ++i)
                {
                    if (randF(1.f) < p)
                    {
                        CSE_Abstract* E = alife().spawn_item(N, o_Position, m_tNodeID, m_tGraphID, ID);
                        //подсоединить аддоны к оружию, если включены соответствующие флажки
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
