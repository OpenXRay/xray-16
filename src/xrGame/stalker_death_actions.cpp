////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_death_actions.cpp
//	Created 	: 25.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker death action classes
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_death_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_decision_space.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"
#include "stalker_movement_manager_smart_cover.h"
#include "Inventory.h"
#include "Weapon.h"
#include "xr_level_controller.h"
#include "clsid_game.h"
#include "CharacterPhysicsSupport.h"
#include "WeaponMagazined.h"

using namespace StalkerDecisionSpace;

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDead
//////////////////////////////////////////////////////////////////////////

CStalkerActionDead::CStalkerActionDead(CAI_Stalker* object, LPCSTR action_name) : inherited(object, action_name) {}
bool CStalkerActionDead::fire() const
{
    if (object().inventory().TotalWeight() <= 0)
        return (false);

    CWeapon* weapon = smart_cast<CWeapon*>(object().inventory().ActiveItem());
    if (!weapon)
        return (false);

    if (!weapon->GetAmmoElapsed())
        return (false);

    if (!object().hammer_is_clutched())
        return (false);

    if (!object().character_physics_support()->can_drop_active_weapon())
        return (true);

    if (Device.dwTimeGlobal - object().GetLevelDeathTime() > 500)
        return (false);

    return (true);
}

void CStalkerActionDead::initialize()
{
    inherited::initialize();

    if (object().getDestroy())
        return;

    if (!fire())
        return;

    object().inventory().Action(kWPN_FIRE, CMD_START);

    u16 active_slot = object().inventory().GetActiveSlot();
    if (active_slot == INV_SLOT_3)
    {
        CInventoryItem* item = object().inventory().ItemFromSlot(active_slot);
        if (item)
        {
            CWeaponMagazined* weapon = smart_cast<CWeaponMagazined*>(item);
            VERIFY(weapon);
            weapon->SetQueueSize(weapon->GetAmmoElapsed());
        }
    }

    u16 I = object().inventory().FirstSlot();
    u16 E = object().inventory().LastSlot();
    for (; I <= E; ++I)
    {
        if (I == BOLT_SLOT)
            continue;

        if (I == object().inventory().GetActiveSlot())
            continue;

        PIItem item = object().inventory().ItemFromSlot(I);
        if (item)
            object().inventory().Ruck(item);
    }
}

void CStalkerActionDead::execute()
{
    inherited::execute();

    if (object().getDestroy())
        return;

    object().movement().enable_movement(false);

    if (fire())
        return;

    if (!object().character_physics_support()->can_drop_active_weapon())
        return;

    u16 I = object().inventory().FirstSlot();
    u16 E = object().inventory().LastSlot();
    for (; I <= E; ++I)
    {
        if (I == BOLT_SLOT)
            continue;

        PIItem item = object().inventory().ItemFromSlot(I);
        if (!item)
            continue;

        if (I == object().inventory().GetActiveSlot())
        {
            item->SetDropManual(TRUE);
            continue;
        }
        object().inventory().Ruck(item);
    }

    set_property(eWorldPropertyDead, true);
}
