////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_alife.cpp
//	Created 	: 15.10.2004
//  Modified 	: 15.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker ALife functions
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ai/stalker/ai_stalker.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_space.h"
#include "Inventory.h"
#include "PDA.h"
#include "eatable_item.h"
#include "medkit.h"
#include "Weapon.h"
#include "Grenade.h"
#include "CustomDetector.h"
#include "ef_storage.h"
#include "ef_primary.h"
#include "ef_pattern.h"
#include "trade_parameters.h"
#include "clsid_game.h"

extern u32 get_rank(const shared_str& section);

static const int MAX_AMMO_ATTACH_COUNT = 1;
static const int enough_ammo_box_count = 1;

IC bool CAI_Stalker::CTradeItem::operator<(const CTradeItem& trade_item) const
{
    return (m_item->object().ID() < trade_item.m_item->object().ID());
}

IC bool CAI_Stalker::CTradeItem::operator==(u16 id) const { return (m_item->object().ID() == id); }
bool CAI_Stalker::tradable_item(CInventoryItem* inventory_item, const u16& current_owner_id)
{
    if (!inventory_item->useful_for_NPC())
        return (false);

    if (CLSID_DEVICE_PDA == inventory_item->object().CLS_ID)
    {
        CPda* pda = smart_cast<CPda*>(inventory_item);
        VERIFY(pda);
        if (pda->GetOriginalOwnerID() == current_owner_id)
            return (false);
    }

    return (trade_parameters().enabled(CTradeParameters::action_sell(0), inventory_item->object().cNameSect()));
}

u32 CAI_Stalker::fill_items(CInventory& inventory, CGameObject* old_owner, ALife::_OBJECT_ID new_owner_id)
{
    u32 result = 0;
    TIItemContainer::iterator I = inventory.m_all.begin();
    TIItemContainer::iterator E = inventory.m_all.end();
    for (; I != E; ++I)
    {
        if (!tradable_item(*I, old_owner->ID()))
            continue;

        m_temp_items.push_back(CTradeItem(*I, old_owner->ID(), new_owner_id));
        result += (*I)->Cost();
    }

    return (result);
}

void CAI_Stalker::transfer_item(CInventoryItem* item, CGameObject* old_owner, CGameObject* new_owner)
{
    NET_Packet P;
    CGameObject* O = old_owner;
    O->u_EventGen(P, GE_TRADE_SELL, O->ID());
    P.w_u16(u16(item->object().ID()));
    O->u_EventSend(P);

    O = new_owner;
    O->u_EventGen(P, GE_TRADE_BUY, O->ID());
    P.w_u16(u16(item->object().ID()));
    O->u_EventSend(P);
}

IC void CAI_Stalker::buy_item_virtual(CTradeItem& item)
{
    item.m_new_owner_id = ID();
    m_total_money -= item.m_item->Cost();
    if (m_current_trader)
        m_current_trader->set_money(m_current_trader->get_money() + item.m_item->Cost(), true);
}

void CAI_Stalker::choose_food()
{
    // stalker cannot change food due to the game design :-(((
    return;
}

void CAI_Stalker::attach_available_ammo(CWeapon* weapon)
{
    if (!weapon || weapon->m_ammoTypes.empty())
        return;

    u32 count = 0;
    xr_vector<CTradeItem>::iterator I = m_temp_items.begin();
    xr_vector<CTradeItem>::iterator E = m_temp_items.end();
    for (; I != E; ++I)
    {
        if (m_total_money < (*I).m_item->Cost())
            continue;

        if (std::find(weapon->m_ammoTypes.begin(), weapon->m_ammoTypes.end(), (*I).m_item->object().cNameSect()) ==
            weapon->m_ammoTypes.end())
            continue;

        buy_item_virtual(*I);

        ++count;
        if (count >= MAX_AMMO_ATTACH_COUNT)
            break;
    }
}

void CAI_Stalker::choose_weapon(ALife::EWeaponPriorityType weapon_priority_type)
{
    CTradeItem* best_weapon = 0;
    float best_value = -1.f;
    ai().ef_storage().non_alife().member() = this;

    xr_vector<CTradeItem>::iterator I = m_temp_items.begin();
    xr_vector<CTradeItem>::iterator E = m_temp_items.end();
    for (; I != E; ++I)
    {
        if (m_total_money < (*I).m_item->Cost())
            continue;

        ai().ef_storage().non_alife().member_item() = &(*I).m_item->object();

        int j = ai().ef_storage().m_pfPersonalWeaponType->dwfGetWeaponType();
        float current_value = -1.f;
        switch (weapon_priority_type)
        {
        case ALife::eWeaponPriorityTypeKnife:
        {
            if (1 != j)
                continue;
            current_value = ai().ef_storage().m_pfItemValue->ffGetValue();
            if (I->m_item->m_ItemCurrPlace.type == eItemPlaceSlot)
                current_value += 10.0f;

            break;
        }
        case ALife::eWeaponPriorityTypeSecondary:
        {
            if (5 != j)
                continue;
            current_value = ai().ef_storage().m_pfSmallWeaponValue->ffGetValue();
            if (I->m_item->m_ItemCurrPlace.type == eItemPlaceSlot)
                current_value += 10.0f;
            break;
        }
        case ALife::eWeaponPriorityTypePrimary:
        {
            if ((6 != j) && (7 != j) && (8 != j) && (9 != j) && (11 != j) && (12 != j))
                continue;
            current_value = ai().ef_storage().m_pfMainWeaponValue->ffGetValue();
            if (I->m_item->m_ItemCurrPlace.type == eItemPlaceSlot)
                current_value += 10.0f;
            break;
        }
        case ALife::eWeaponPriorityTypeGrenade:
        {
            if (4 != j)
                continue;
            current_value = ai().ef_storage().m_pfItemValue->ffGetValue();
            if (I->m_item->m_ItemCurrPlace.type == eItemPlaceSlot)
                current_value += 10.0f;
            break;
        }
        default: NODEFAULT;
        }

        if ((current_value > best_value))
        {
            best_value = current_value;
            best_weapon = &*I;
        }
    }
    if (best_weapon)
    {
        buy_item_virtual(*best_weapon);
        attach_available_ammo(smart_cast<CWeapon*>(best_weapon->m_item));
    }
}

void CAI_Stalker::choose_medikit()
{
    // stalker cannot change medikit due to the game design :-(((
    return;
}

void CAI_Stalker::choose_detector()
{
    CTradeItem* best_detector = 0;
    float best_value = -1.f;
    ai().ef_storage().non_alife().member() = this;
    xr_vector<CTradeItem>::iterator I = m_temp_items.begin();
    xr_vector<CTradeItem>::iterator E = m_temp_items.end();
    for (; I != E; ++I)
    {
        if (m_total_money < (*I).m_item->Cost())
            continue;

        CCustomDetector* detector = smart_cast<CCustomDetector*>((*I).m_item);
        if (!detector)
            continue;

        // evaluating item
        ai().ef_storage().non_alife().member_item() = detector;
        float current_value = ai().ef_storage().m_pfDetectorType->ffGetValue();
        // choosing the best item
        if ((current_value > best_value))
        {
            best_detector = &*I;
            best_value = current_value;
        }
    }
    if (best_detector)
        buy_item_virtual(*best_detector);
}

void CAI_Stalker::choose_equipment()
{
    // stalker cannot change their equipment due to the game design :-(((
    return;
}

void CAI_Stalker::select_items()
{
    if (!m_can_select_items)
        return;

    choose_food();
    choose_weapon(ALife::eWeaponPriorityTypeKnife);
    choose_weapon(ALife::eWeaponPriorityTypeSecondary);
    choose_weapon(ALife::eWeaponPriorityTypePrimary);
    choose_weapon(ALife::eWeaponPriorityTypeGrenade);
    choose_medikit();
    choose_detector();
    choose_equipment();
}

void CAI_Stalker::update_sell_info()
{
    //	if (m_sell_info_actuality)
    //		return;

    m_sell_info_actuality = true;
    m_temp_items.clear();
    m_current_trader = 0;
    m_total_money = get_money();
    u32 money_delta = fill_items(inventory(), this, ALife::_OBJECT_ID(-1));
    m_total_money += money_delta;
    std::sort(m_temp_items.begin(), m_temp_items.end());
    select_items();

    TIItemContainer::iterator I = inventory().m_all.begin();
    TIItemContainer::iterator E = inventory().m_all.end();
    for (; I != E; ++I)
    {
        if (!tradable_item(*I, ID()))
            m_temp_items.push_back(CTradeItem(*I, ID(), ID()));
    }
}

bool CAI_Stalker::can_sell(CInventoryItem* item)
{
    if (READ_IF_EXISTS(pSettings, r_bool, cNameSect(), "is_trader", false))
        return (tradable_item(item, ID()));

    update_sell_info();
    xr_vector<CTradeItem>::const_iterator I = std::find(m_temp_items.begin(), m_temp_items.end(), item->object().ID());
    VERIFY(I != m_temp_items.end());
    return ((*I).m_new_owner_id != ID());
}

bool CAI_Stalker::AllowItemToTrade(CInventoryItem const* item, const SInvItemPlace& place) const
{
    if (!g_Alive())
        return (trade_parameters().enabled(CTradeParameters::action_show(0), item->object().cNameSect()));

    return (const_cast<CAI_Stalker*>(this)->can_sell(const_cast<CInventoryItem*>(item)));
}

bool CAI_Stalker::non_conflicted(const CInventoryItem* item, const CWeapon* new_weapon) const
{
    if (item == new_weapon)
        return (true);

    const CWeapon* weapon = smart_cast<const CWeapon*>(item);
    if (!weapon)
        return (true);

    return (weapon->ef_weapon_type() != new_weapon->ef_weapon_type());
}

bool CAI_Stalker::enough_ammo(const CWeapon* new_weapon) const
{
    int ammo_box_count = 0;

    TIItemContainer::const_iterator I = inventory().m_all.begin();
    TIItemContainer::const_iterator E = inventory().m_all.end();
    for (; I != E; ++I)
    {
        if (std::find(new_weapon->m_ammoTypes.begin(), new_weapon->m_ammoTypes.end(), (*I)->object().cNameSect()) ==
            new_weapon->m_ammoTypes.end())
            continue;

        ++ammo_box_count;
        if (ammo_box_count >= enough_ammo_box_count)
            return (true);
    }

    return (false);
}

bool CAI_Stalker::conflicted(
    const CInventoryItem* item, const CWeapon* new_weapon, bool new_wepon_enough_ammo, int new_weapon_rank) const
{
    if (non_conflicted(item, new_weapon))
        return (false);

    const CWeapon* weapon = smart_cast<const CWeapon*>(item);
    VERIFY(weapon);

    bool current_weapon_enough_ammo = enough_ammo(weapon);
    if (current_weapon_enough_ammo && !new_wepon_enough_ammo)
        return (true);

    if (!current_weapon_enough_ammo && new_wepon_enough_ammo)
        return (false);

    if (!fsimilar(weapon->GetCondition(), new_weapon->GetCondition(), .05f))
        return (weapon->GetCondition() >= new_weapon->GetCondition());

    if (weapon->ef_weapon_type() != new_weapon->ef_weapon_type())
        return (weapon->Cost() >= new_weapon->Cost());

    u32 weapon_rank = get_rank(weapon->cNameSect());

    if (weapon_rank != (u32)new_weapon_rank)
        return (weapon_rank >= (u32)new_weapon_rank);

    return (true);
}

bool CAI_Stalker::can_take(CInventoryItem const* item)
{
    const CWeapon* new_weapon = smart_cast<const CWeapon*>(item);
    if (!new_weapon)
        return (false);

    bool new_weapon_enough_ammo = enough_ammo(new_weapon);
    u32 new_weapon_rank = get_rank(new_weapon->cNameSect());

    TIItemContainer::iterator I = inventory().m_all.begin();
    TIItemContainer::iterator E = inventory().m_all.end();
    for (; I != E; ++I)
        if (conflicted(*I, new_weapon, new_weapon_enough_ammo, new_weapon_rank))
            return (false);

    return (true);
}

void CAI_Stalker::remove_personal_only_ammo(const CInventoryItem* item)
{
    const CWeapon* weapon = smart_cast<const CWeapon*>(item);
    VERIFY(weapon);

    xr_vector<shared_str>::const_iterator I = weapon->m_ammoTypes.begin();
    xr_vector<shared_str>::const_iterator E = weapon->m_ammoTypes.end();
    for (; I != E; ++I)
    {
        bool found = false;

        TIItemContainer::const_iterator i = inventory().m_all.begin();
        TIItemContainer::const_iterator e = inventory().m_all.end();
        for (; i != e; ++i)
        {
            if ((*i)->object().ID() == weapon->ID())
                continue;

            const CWeapon* temp = smart_cast<const CWeapon*>(*i);
            if (!temp)
                continue;

            if (std::find(temp->m_ammoTypes.begin(), temp->m_ammoTypes.end(), *I) == temp->m_ammoTypes.end())
                continue;

            found = true;
            break;
        }

        if (found)
            continue;

        for (i = inventory().m_all.begin(); i != e; ++i)
        {
            if (xr_strcmp(*I, (*i)->object().cNameSect()))
                continue;

            NET_Packet packet;
            u_EventGen(packet, GE_DESTROY, (*i)->object().ID());
            u_EventSend(packet);
        }
    }
}

void CAI_Stalker::update_conflicted(CInventoryItem* item, const CWeapon* new_weapon)
{
    if (non_conflicted(item, new_weapon))
        return;

    remove_personal_only_ammo(item);
    item->SetDropManual(TRUE);
}

void CAI_Stalker::on_after_take(const CGameObject* object)
{
    if (!g_Alive())
        return;

    if (!READ_IF_EXISTS(pSettings, r_bool, cNameSect(), "use_single_item_rule", true))
        return;

    const CWeapon* new_weapon = smart_cast<const CWeapon*>(object);
    if (!new_weapon)
        return;

    TIItemContainer::iterator I = inventory().m_all.begin();
    TIItemContainer::iterator E = inventory().m_all.end();
    for (; I != E; ++I)
        update_conflicted(*I, new_weapon);
}
