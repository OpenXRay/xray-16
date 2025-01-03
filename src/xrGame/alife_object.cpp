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

    luabind::functor<bool> funct;
	if (GEnv.ScriptEngine->functor("ai_stalker.CSE_ALifeObject_spawn_supplies", funct))
	{
		if (funct(this, ID, ini_string))
			return;
	}

#pragma warning(push)
#pragma warning(disable : 4238)
    IReader reader((void*)ini_string, xr_strlen(ini_string));
    CInifile ini(&reader, FS.get_path("$game_config$")->m_Path);
#pragma warning(pop)
    u8 loadoutIndex = 0;
    string32 loadoutSection = "spawn_loadout";

    // Alundaio: This will spawn a single random section listed in [spawn_loadout].
    // No need to spawn ammo, this will automatically spawn 1 box for weapon and if ammo_type is specified it will spawn that type.
    // Count is used only for ammo boxes (ie wpn_pm = 3) will spawn 3 boxes, not 3 wpn_pm.
    // Supports few loadout options, iterates over `spawn_loadout`, `spawn_loadout2` ... `spawn_loadoutN`.
    while (ini.section_exist(loadoutSection))
    {
        pcstr itmSection, V;
        xr_vector<u32> spawnLoadouts;

        pcstr lname = ai().game_graph().header().level(ai().game_graph().vertex(m_tGraphID)->level_id()).name().c_str();

        for (u32 k = 0; ini.r_line(loadoutSection, k, &itmSection, &V); k++)
        {
            // If level=<lname> then only spawn items if object on that level
            if (strstr(V, "level=") != nullptr)
            {
                if (strstr(V, lname) != nullptr)
                    spawnLoadouts.push_back(k);
            }
            else
            {
                spawnLoadouts.push_back(k);
            }
        }

        if (!spawnLoadouts.empty())
        {
            s32 sel = Random.randI(0, spawnLoadouts.size());
            if (ini.r_line(loadoutSection, spawnLoadouts.at(sel), &itmSection, &V))
            {
                VERIFY(xr_strlen(itmSection));
                if (pSettings->section_exist(itmSection))
                {
                    u32 spawnCount = 1;
                    bool bScope = false;
                    bool bSilencer = false;
                    bool bLauncher = false;
                    float fCond = 1.0f;
                    int iAmmoType = 0, n = 0;

                    if (V && xr_strlen(V))
                    {
                        n = _GetItemCount(V);
                        if (n > 0)
                        {
                            string64 tmp;
                            spawnCount = atoi(_GetItem(V, 0, tmp)); //count
                        }

                        bScope = is_spawn_supplies_flag_set(V, "scope");
                        bSilencer = is_spawn_supplies_flag_set(V, "silencer");
                        bLauncher = is_spawn_supplies_flag_set(V, "launcher");

                        if (!spawnCount) spawnCount = 1;
                        if (strstr(V, "cond=") != nullptr)
                            fCond = static_cast<float>(atof(strstr(V, "cond=") + 5));
                        if (strstr(V, "ammo_type=") != nullptr)
                            iAmmoType = atoi(strstr(V, "ammo_type=") + 10);
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
                                if (i == iAmmoType)
                                    break;
                            }
                            if (xr_strlen(ammoSec) && pSettings->section_exist(ammoSec))
                                for (u32 i = 1; i <= spawnCount; ++i)
                                    alife().spawn_item(ammoSec, o_Position, m_tNodeID, m_tGraphID, ID);
                        }
                    }
                    // If not weapon item, handle count as literal count, not ammo (useful for grenades and consumables).
                    else
                    {
                        for (u32 i = 1; i <= spawnCount - 1; ++i)
                            alife().spawn_item(itmSection, o_Position, m_tNodeID, m_tGraphID, ID);
                    }

                    if (const auto IItem = smart_cast<CSE_ALifeInventoryItem*>(E))
                        IItem->m_fCondition = fCond;
                }
            }
        }

        loadoutIndex += 1;
        xr_sprintf(loadoutSection, "spawn_loadout%d", loadoutIndex);
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
                float fCond = 1.0f;
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

                    bScope = is_spawn_supplies_flag_set(V, "scope");
                    bSilencer = is_spawn_supplies_flag_set(V, "silencer");
                    bLauncher = is_spawn_supplies_flag_set(V, "launcher");

                    // probability
                    if (strstr(V, "prob=") != nullptr)
                        p = static_cast<float>(atof(strstr(V, "prob=") + 5));
                    if (fis_zero(p))
                        p = 1.0f;
                    if (strstr(V, "cond=") != nullptr)
                        fCond = static_cast<float>(atof(strstr(V, "cond=") + 5));
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
                            IItem->m_fCondition = fCond;
                    }
                }
            }
        }
    }
}

bool CSE_ALifeObject::keep_saved_data_anyway() const /* noexcept */ { return false; }

/// Check if spawn supplies section flag was set.
/// Comparing to original `section = value, scope, silencer, launcher, cond=0.5`,
/// also support extended variants like `section = value, scope=true, silencer=0.5, launcher, cond=0.5`.
bool CSE_ALifeObject::is_spawn_supplies_flag_set(pcstr value, pcstr flag)
{
    pcstr flagSubstring = strstr(value, flag);
    int flagLength = strlen(flag);

    if (flagSubstring != nullptr)
    {
        // Got full definition with '=' char, not simple shorthand like scope/silencer.
        if (*(flagSubstring + flagLength) == '=')
        {
            if (strncmp(flagSubstring + flagLength, "=true", 5) != -1)
            {
                return true;
            }

            float probability = static_cast<float>(atof(flagSubstring + flagLength + 1));

            // Assume 0 is 1 for cases like `flag=,flag=\n` and consistency with `prob` calculations.
            return fis_zero(probability) ? true : randF(1.f) <= probability;
        }
        // Short variant of flag without assigned value.
        else
        {
            return true;
        }
    }

    return false;
}
