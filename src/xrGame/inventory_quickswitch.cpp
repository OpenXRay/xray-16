#include "StdAfx.h"
#include "Inventory.h"
#include "Weapon.h"
#include "Actor.h"
#include "xrCore/xr_ini.h"

static u32 const ammo_to_cost_map_koef = 3;
class next_weapon_searcher
{
public:
    typedef xr_set<PIItem> exception_items_t;
    next_weapon_searcher(priority_group& pg, PIItem& best_fit, exception_items_t& except_set, bool ignore_ammo)
        : m_prior_group(pg), m_best_fit(best_fit), m_best_fit_cost(0), m_best_fit_ammo_elapsed(0),
          m_except_set(except_set), m_ignore_ammo(ignore_ammo)
    {
        m_best_fit = NULL;
    };

    next_weapon_searcher(next_weapon_searcher const& copy)
        : m_prior_group(copy.m_prior_group), m_best_fit(copy.m_best_fit), m_best_fit_cost(copy.m_best_fit_cost),
          m_best_fit_ammo_elapsed(copy.m_best_fit_ammo_elapsed), m_except_set(copy.m_except_set),
          m_ignore_ammo(copy.m_ignore_ammo){};

    void operator()(PIItem const& item)
    {
        if (!item)
            return;

        CGameObject* tmp_obj = item->cast_game_object();
        if (!tmp_obj)
            return;

        if (m_except_set.find(item) != m_except_set.end())
            return;

        if (!m_prior_group.is_item_in_group(tmp_obj->cNameSect()))
            return;

        CWeapon* tmp_weapon = smart_cast<CWeapon*>(tmp_obj);
        if (!tmp_weapon)
            return;

        u32 tmp_ammo_count = tmp_weapon->GetAmmoElapsed();
        u32 tmp_cost = tmp_weapon->Cost();

        if (!tmp_ammo_count && !m_ignore_ammo)
            return;

        if (!m_best_fit)
        {
            m_best_fit = item;
            m_best_fit_cost = tmp_weapon->Cost();
            m_best_fit_ammo_elapsed = tmp_weapon->GetAmmoElapsed();
            return;
        }

        if ((tmp_cost + (tmp_ammo_count * ammo_to_cost_map_koef)) >
            (m_best_fit_cost + (m_best_fit_ammo_elapsed * ammo_to_cost_map_koef)))
        {
            m_best_fit = item;
            m_best_fit_cost = tmp_weapon->Cost();
            m_best_fit_ammo_elapsed = tmp_weapon->GetAmmoElapsed();
            return;
        }
    };

private:
    next_weapon_searcher& operator=(next_weapon_searcher const& copy) {}
    priority_group& m_prior_group;
    exception_items_t& m_except_set;
    PIItem& m_best_fit;
    u32 m_best_fit_cost;
    u32 m_best_fit_ammo_elapsed;
    bool m_ignore_ammo;
}; // class next_weapon_searcher

static u32 const exception_items_clear_time = 2000; // 2 seconds
PIItem CInventory::GetNextItemInActiveSlot(u8 const priority_value, bool ignore_ammo)
{
    if (m_next_item_iteration_time + exception_items_clear_time <= Device.dwTimeGlobal)
    {
        m_next_items_exceptions.clear();
        m_next_items_exceptions.insert(ActiveItem());
    }
    PIItem best_fit = NULL;
    priority_group& tmp_prior_group = GetPriorityGroup(priority_value, m_iActiveSlot);
    next_weapon_searcher tmp_predicate(tmp_prior_group, best_fit, m_next_items_exceptions, ignore_ammo);

    std::for_each(m_ruck.begin(), m_ruck.end(), tmp_predicate);
    if (best_fit)
    {
        m_next_items_exceptions.insert(best_fit);
        m_next_item_iteration_time = Device.dwTimeGlobal;
        return best_fit;
    }

    u8 next_priority_group = priority_value + 1;
    if (next_priority_group == qs_priorities_count)
    {
        if (ignore_ammo)
        {
            m_next_items_exceptions.clear();
            m_next_items_exceptions.insert(ActiveItem());
            return NULL;
        }
        return GetNextItemInActiveSlot(0, true);
    }
    return GetNextItemInActiveSlot(next_priority_group, ignore_ammo);
};

bool CInventory::ActivateNextItemInActiveSlot()
{
    if (m_iActiveSlot == NO_ACTIVE_SLOT)
    {
        return false;
    }

    IGameObject* pActor_owner = smart_cast<IGameObject*>(m_pOwner);
    if (Level().CurrentViewEntity() != pActor_owner)
    {
        return false;
    }

    PIItem new_item = GetNextItemInActiveSlot(0, false);

    if (new_item == NULL)
    {
        return false; // only 1 item for this slot
    }

    m_activ_last_items.push_back(new_item);
    PIItem current_item = ActiveItem();

    NET_Packet P;
    bool res;
    if (current_item)
    {
        res = Ruck(current_item);
        R_ASSERT(res);
        current_item->object().u_EventGen(P, GEG_PLAYER_ITEM2RUCK, current_item->object().H_Parent()->ID());
        P.w_u16(current_item->object().ID());
        current_item->object().u_EventSend(P);
    }

    res = Slot(m_iActiveSlot, new_item);
    R_ASSERT(res);
    new_item->object().u_EventGen(P, GEG_PLAYER_ITEM2SLOT, new_item->object().H_Parent()->ID());
    P.w_u16(new_item->object().ID());
    P.w_u16(m_iActiveSlot);
    new_item->object().u_EventSend(P);

    // activate
    new_item->object().u_EventGen(P, GEG_PLAYER_ACTIVATE_SLOT, new_item->object().H_Parent()->ID());
    P.w_u16(m_iActiveSlot);
    new_item->object().u_EventSend(P);

    //	Msg( "Weapon change" );
    return true;
}

priority_group& CInventory::GetPriorityGroup(u8 const priority_value, u16 slot)
{
    R_ASSERT(priority_value < qs_priorities_count);
    if (slot == INV_SLOT_2)
    {
        VERIFY(m_slot2_priorities[priority_value]);
        return *m_slot2_priorities[priority_value];
    }
    else if (slot == INV_SLOT_3)
    {
        VERIFY(m_slot3_priorities[priority_value]);
        return *m_slot3_priorities[priority_value];
    }
    return m_null_priority;
}

enum enum_priorities_groups
{
    epg_pistols = 0x00,
    epg_shotgun,
    epg_assault,
    epg_sniper_rifels,
    epg_heavy_weapons,
    epg_groups_count
};

char const* groups_names[CInventory::qs_priorities_count] = {
    "pistols", "shotgun", "assault", "sniper_rifles", "heavy_weapons"};

u32 g_slot2_pistol_switch_priority = 0;
u32 g_slot2_shotgun_switch_priority = 1;
u32 g_slot2_assault_switch_priority = 2;
u32 g_slot2_sniper_switch_priority = 4;
u32 g_slot2_heavy_switch_priority = 3;

u32 g_slot3_pistol_switch_priority = 4; // not switch
u32 g_slot3_shotgun_switch_priority = 2;
u32 g_slot3_assault_switch_priority = 0;
u32 g_slot3_sniper_switch_priority = 1;
u32 g_slot3_heavy_switch_priority = 3;

static char const* teamdata_section = "deathmatch_team0";

void CInventory::InitPriorityGroupsForQSwitch()
{
    static_assert(epg_groups_count == qs_priorities_count, "Groups count problem.");
    for (int i = epg_pistols; i < epg_groups_count; ++i)
    {
        m_groups[i].init_group(teamdata_section, groups_names[i]);
    }

    m_slot2_priorities[g_slot2_pistol_switch_priority] = &m_groups[epg_pistols];
    m_slot2_priorities[g_slot2_shotgun_switch_priority] = &m_groups[epg_shotgun];
    m_slot2_priorities[g_slot2_assault_switch_priority] = &m_groups[epg_assault];
    m_slot2_priorities[g_slot2_sniper_switch_priority] = &m_groups[epg_sniper_rifels];
    m_slot2_priorities[g_slot2_heavy_switch_priority] = &m_groups[epg_heavy_weapons];

    m_slot3_priorities[g_slot3_pistol_switch_priority] = &m_groups[epg_assault]; //&m_groups[epg_pistols];
    m_slot3_priorities[g_slot3_shotgun_switch_priority] = &m_groups[epg_shotgun];
    m_slot3_priorities[g_slot3_assault_switch_priority] = &m_groups[epg_assault];
    m_slot3_priorities[g_slot3_sniper_switch_priority] = &m_groups[epg_sniper_rifels];
    m_slot3_priorities[g_slot3_heavy_switch_priority] = &m_groups[epg_heavy_weapons];
}

priority_group::priority_group() {}
void priority_group::init_group(shared_str const& game_section, shared_str const& line)
{
    shared_str tmp_string = pSettings->r_string(game_section, line.c_str());
    string256 dststr;
    u32 count = _GetItemCount(tmp_string.c_str());
    for (u32 i = 0; i < count; ++i)
    {
        _GetItem(tmp_string.c_str(), i, dststr);
        m_sections.insert(shared_str(dststr));
    }
}

bool priority_group::is_item_in_group(shared_str const& section_name) const
{
    return m_sections.find(section_name) != m_sections.end();
}
