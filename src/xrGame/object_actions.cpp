////////////////////////////////////////////////////////////////////////////
//	Module 		: object_actions.h
//	Created 	: 12.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Object actions
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "object_actions.h"
#include "Inventory.h"
#include "ai/stalker/ai_stalker.h"
#include "xr_level_controller.h"
#include "xrMessages.h"
#include "FoodItem.h"
#include "Weapon.h"
#include "WeaponMagazined.h"
#include "object_handler_space.h"
#include "stalker_animation_manager.h"
#include "object_handler_planner.h"
#include "object_handler_planner_impl.h"

//////////////////////////////////////////////////////////////////////////
// CObjectActionCommand
//////////////////////////////////////////////////////////////////////////

CObjectActionCommand::CObjectActionCommand(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, u32 command, LPCSTR action_name)
    : inherited(item, owner, storage, action_name), m_command(command)
{
}

void CObjectActionCommand::initialize()
{
    inherited::initialize();
    object().inventory().Action((u16)m_command, CMD_START);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionShow
//////////////////////////////////////////////////////////////////////////

CObjectActionShow::CObjectActionShow(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, LPCSTR action_name)
    : inherited(item, owner, storage, action_name)
{
    m_weapon = smart_cast<CWeapon*>(item);
}

void CObjectActionShow::initialize()
{
    inherited::initialize();

    VERIFY(m_item);

    CInventoryItem* slot_item = object().inventory().ItemFromSlot(m_item->BaseSlot());
    if (slot_item == m_item)
        return;

    if (slot_item)
        object().inventory().Ruck(slot_item);

    object().inventory().Slot(m_item->BaseSlot(), m_item);
}

void CObjectActionShow::execute()
{
    inherited::execute();
    VERIFY(m_item);
    if (object().inventory().ActiveItem() && (object().inventory().ActiveItem() == m_item))
        return;

    CHudItem* hud_item = smart_cast<CHudItem*>(object().inventory().ActiveItem());
    if (!hud_item)
    {
        object().inventory().Slot(m_item->BaseSlot(), m_item);
        object().inventory().Activate(m_item->BaseSlot());
        return;
    }

    if (hud_item->IsPending())
        return;

    CInventoryItem* slot_item = object().inventory().ItemFromSlot(m_item->BaseSlot());
    if (slot_item == m_item)
        return;

    if (slot_item)
        object().inventory().Ruck(slot_item);

    object().inventory().Slot(m_item->BaseSlot(), m_item);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionHide
//////////////////////////////////////////////////////////////////////////

CObjectActionHide::CObjectActionHide(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, LPCSTR action_name)
    : inherited(item, owner, storage, action_name)
{
}

void CObjectActionHide::execute()
{
    inherited::execute();
    VERIFY(m_item);
    object().inventory().Activate(NO_ACTIVE_SLOT);
    set_property(ObjectHandlerSpace::eWorldPropertyUseEnough, false);
}

void CObjectActionHide::finalize()
{
    inherited::finalize();

    if (!object().inventory().ActiveItem())
        return;

    CHudItem* const hud_item = smart_cast<CHudItem*>(object().inventory().ActiveItem());
    VERIFY(hud_item);
    if (hud_item->IsHidden())
        return;

    hud_item->StopCurrentAnimWithoutCallback();
    hud_item->SetState(CHUDState::eIdle);
    hud_item->SetNextState(CHUDState::eIdle);
}

// to prevent several recharges
static bool try_advance_ammo(CWeapon const& weapon)
{
    VERIFY(weapon.m_pInventory);
    CInventory& inventory = *weapon.m_pInventory;
    for (u8 i = 0; i < u8(weapon.m_ammoTypes.size()); ++i)
    {
        LPCSTR l_ammoType = weapon.m_ammoTypes[i].c_str();

        for (TIItemContainer::iterator l_it = inventory.m_belt.begin(); inventory.m_belt.end() != l_it; ++l_it)
        {
            CWeaponAmmo* l_pAmmo = smart_cast<CWeaponAmmo*>(*l_it);

            if (l_pAmmo && !xr_strcmp(l_pAmmo->cNameSect(), l_ammoType))
            {
                if (l_pAmmo->m_boxCurr < l_pAmmo->m_boxSize)
                {
                    l_pAmmo->m_boxCurr = l_pAmmo->m_boxSize;
                    return (true);
                }
            }
        }

        for (TIItemContainer::iterator l_it = inventory.m_ruck.begin(); inventory.m_ruck.end() != l_it; ++l_it)
        {
            CWeaponAmmo* l_pAmmo = smart_cast<CWeaponAmmo*>(*l_it);
            if (l_pAmmo && !xr_strcmp(l_pAmmo->cNameSect(), l_ammoType))
            {
                if (l_pAmmo->m_boxCurr < l_pAmmo->m_boxSize)
                {
                    l_pAmmo->m_boxCurr = l_pAmmo->m_boxSize;
                    return (true);
                }
            }
        }
    }

    return (false);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionReload
//////////////////////////////////////////////////////////////////////////

CObjectActionReload::CObjectActionReload(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, _condition_type type, LPCSTR action_name)
    : inherited(item, owner, storage, action_name), m_type(type)
{
}

void CObjectActionReload::initialize()
{
    inherited::initialize();
    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());
    if (object().infinite_ammo())
    {
        CWeapon* weapon = smart_cast<CWeapon*>(&m_item->object());
        VERIFY(weapon);
        try_advance_ammo(*weapon);
    }

    object().inventory().Action(kWPN_RELOAD, CMD_START);
}

void CObjectActionReload::execute()
{
    inherited::execute();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    CWeapon* weapon = smart_cast<CWeapon*>(object().inventory().ActiveItem());
    VERIFY(weapon);
    if (weapon->IsPending())
        return;

    if (weapon->GetAmmoElapsed())
    {
        VERIFY(weapon->GetSuitableAmmoTotal() >= weapon->GetAmmoElapsed());
        if (weapon->GetSuitableAmmoTotal() == weapon->GetAmmoElapsed())
            return;

        VERIFY(weapon->GetAmmoMagSize() >= weapon->GetAmmoElapsed());
        if (weapon->GetAmmoMagSize() == weapon->GetAmmoElapsed())
            return;
    }

    object().inventory().Action(kWPN_RELOAD, CMD_START);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionFire
//////////////////////////////////////////////////////////////////////////

CObjectActionFire::CObjectActionFire(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, _condition_type type, LPCSTR action_name)
    : inherited(item, owner, storage, action_name), m_type(type)
{
}

void CObjectActionFire::initialize()
{
    inherited::inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if (!m_object->can_kill_member())
        object().inventory().Action(kWPN_FIRE, CMD_START);
    else
        object().inventory().Action(kWPN_FIRE, CMD_STOP);
}

void CObjectActionFire::execute()
{
    inherited::execute();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if (!m_object->can_kill_member())
    {
        CWeapon* weapon = smart_cast<CWeapon*>(object().inventory().ActiveItem());
        if (!weapon || (weapon->GetState() != CWeapon::eFire))
            object().inventory().Action(kWPN_FIRE, CMD_START);
    }
    else
        object().inventory().Action(kWPN_FIRE, CMD_STOP);
}

void CObjectActionFire::finalize()
{
    inherited::finalize();
    object().inventory().Action(kWPN_FIRE, CMD_STOP);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionFireNoReload
//////////////////////////////////////////////////////////////////////////

CObjectActionFireNoReload::CObjectActionFireNoReload(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, _condition_type type, LPCSTR action_name)
    : inherited(item, owner, storage, action_name), m_type(type), m_fired(false)
{
}

void CObjectActionFireNoReload::initialize()
{
    inherited::inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if (!m_object->can_kill_member())
        object().inventory().Action(kWPN_FIRE, CMD_START);
    else
        object().inventory().Action(kWPN_FIRE, CMD_STOP);

    CWeapon* weapon = smart_cast<CWeapon*>(object().inventory().ActiveItem());
    if (weapon && (weapon->GetState() == CWeapon::eFire))
        m_fired = true;
    else
        m_fired = false;
}

void CObjectActionFireNoReload::execute()
{
    inherited::execute();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if (m_fired)
    {
        if (!m_object->can_kill_member())
            return;

        object().inventory().Action(kWPN_FIRE, CMD_STOP);
        //		m_fired					= false;
        return;
    }

    if (m_object->can_kill_member())
        return;

    CWeapon* weapon = smart_cast<CWeapon*>(object().inventory().ActiveItem());
    if (!weapon || (weapon->GetState() != CWeapon::eFire))
        object().inventory().Action(kWPN_FIRE, CMD_START);

    if (weapon && (weapon->GetState() == CWeapon::eFire))
        m_fired = true;
}

void CObjectActionFireNoReload::finalize()
{
    inherited::finalize();

    object().inventory().Action(kWPN_FIRE, CMD_STOP);
    m_storage->set_property(m_type, false);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionStrapping
//////////////////////////////////////////////////////////////////////////

CObjectActionStrapping::CObjectActionStrapping(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, LPCSTR action_name)
    : inherited(item, owner, storage, action_name)
{
    m_callback_removed = true;
}

CObjectActionStrapping::~CObjectActionStrapping()
{
    if (m_callback_removed)
    {
        VERIFY(!object().animation().torso().callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrapping::on_animation_end)));
        return;
    }

    object().animation().torso().remove_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrapping::on_animation_end));
}

void CObjectActionStrapping::on_animation_end()
{
    VERIFY(!m_callback_removed);

    m_storage->set_property(ObjectHandlerSpace::eWorldPropertyStrapped, true);

    object().animation().torso().remove_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrapping::on_animation_end));

    m_callback_removed = true;
}

void CObjectActionStrapping::initialize()
{
    inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    m_callback_removed = false;

    if (object().inventory().ActiveItem())
        stop_hiding_operation_if_any();

    m_storage->set_property(ObjectHandlerSpace::eWorldPropertyStrapped2Idle, true);

    object().animation().torso().add_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrapping::on_animation_end));
}

void CObjectActionStrapping::execute()
{
    inherited::execute();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    prevent_weapon_state_switch_ugly();
}

void CObjectActionStrapping::finalize()
{
    inherited::finalize();

    if (!m_callback_removed)
    {
        object().animation().torso().remove_callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrapping::on_animation_end));

        m_callback_removed = true;
    }
    else
    {
        VERIFY(!object().animation().torso().callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrapping::on_animation_end)));
    }
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionStrappingToIdle
//////////////////////////////////////////////////////////////////////////

CObjectActionStrappingToIdle::CObjectActionStrappingToIdle(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, LPCSTR action_name)
    : inherited(item, owner, storage, action_name)
{
    m_callback_removed = true;
}

CObjectActionStrappingToIdle::~CObjectActionStrappingToIdle()
{
    if (m_callback_removed)
    {
        VERIFY(!object().animation().torso().callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrappingToIdle::on_animation_end)));
        return;
    }

    object().animation().torso().remove_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrappingToIdle::on_animation_end));
}

void CObjectActionStrappingToIdle::on_animation_end()
{
    VERIFY(!m_callback_removed);

    m_storage->set_property(ObjectHandlerSpace::eWorldPropertyStrapped2Idle, false);

    object().animation().torso().remove_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrappingToIdle::on_animation_end));

    m_callback_removed = true;
}

void CObjectActionStrappingToIdle::initialize()
{
    inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if (object().inventory().ActiveItem())
        stop_hiding_operation_if_any();

    m_callback_removed = false;

    object().animation().torso().add_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrappingToIdle::on_animation_end));
}

void CObjectActionStrappingToIdle::execute()
{
    inherited::execute();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    prevent_weapon_state_switch_ugly();
}

void CObjectActionStrappingToIdle::finalize()
{
    inherited::finalize();

    if (!m_callback_removed)
    {
        object().animation().torso().remove_callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrappingToIdle::on_animation_end));

        m_callback_removed = true;
    }
    else
    {
        VERIFY(!object().animation().torso().callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionStrappingToIdle::on_animation_end)));
    }
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionUnstrapping
//////////////////////////////////////////////////////////////////////////

CObjectActionUnstrapping::CObjectActionUnstrapping(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, LPCSTR action_name)
    : inherited(item, owner, storage, action_name)
{
    m_callback_removed = true;
}

CObjectActionUnstrapping::~CObjectActionUnstrapping()
{
    if (m_callback_removed)
    {
        VERIFY(!object().animation().torso().callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrapping::on_animation_end)));
        return;
    }

    object().animation().torso().remove_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrapping::on_animation_end));
}

void CObjectActionUnstrapping::on_animation_end()
{
    VERIFY(!m_callback_removed);

    m_storage->set_property(ObjectHandlerSpace::eWorldPropertyStrapped, false);

    object().animation().torso().remove_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrapping::on_animation_end));

    m_callback_removed = true;
}

void CObjectActionUnstrapping::initialize()
{
    inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if (object().inventory().ActiveItem())
        stop_hiding_operation_if_any();

    m_callback_removed = false;

    m_storage->set_property(ObjectHandlerSpace::eWorldPropertyStrapped2Idle, true);

    object().animation().torso().add_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrapping::on_animation_end));
}

void CObjectActionUnstrapping::execute()
{
    inherited::execute();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    prevent_weapon_state_switch_ugly();
}

void CObjectActionUnstrapping::finalize()
{
    inherited::finalize();

    if (!m_callback_removed)
    {
        object().animation().torso().remove_callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrapping::on_animation_end));

        m_callback_removed = true;
    }
    else
    {
        VERIFY(!object().animation().torso().callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrapping::on_animation_end)));
    }
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionUnstrappingToIdle
//////////////////////////////////////////////////////////////////////////

CObjectActionUnstrappingToIdle::CObjectActionUnstrappingToIdle(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, LPCSTR action_name)
    : inherited(item, owner, storage, action_name)
{
    m_callback_removed = true;
}

CObjectActionUnstrappingToIdle::~CObjectActionUnstrappingToIdle()
{
    if (m_callback_removed)
    {
        VERIFY(!object().animation().torso().callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrappingToIdle::on_animation_end)));
        return;
    }

    object().animation().torso().remove_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrappingToIdle::on_animation_end));
}

void CObjectActionUnstrappingToIdle::on_animation_end()
{
    VERIFY(!m_callback_removed);

    m_storage->set_property(ObjectHandlerSpace::eWorldPropertyStrapped2Idle, false);

    object().animation().torso().remove_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrappingToIdle::on_animation_end));

    m_callback_removed = true;
}

void CObjectActionUnstrappingToIdle::initialize()
{
    inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if (object().inventory().ActiveItem())
        stop_hiding_operation_if_any();

    m_callback_removed = false;

    object().animation().torso().add_callback(
        CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrappingToIdle::on_animation_end));
}

void CObjectActionUnstrappingToIdle::execute()
{
    inherited::execute();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    prevent_weapon_state_switch_ugly();
}

void CObjectActionUnstrappingToIdle::finalize()
{
    inherited::finalize();

    if (!m_callback_removed)
    {
        object().animation().torso().remove_callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrappingToIdle::on_animation_end));

        m_callback_removed = true;
    }
    else
    {
        VERIFY(!object().animation().torso().callback(
            CStalkerAnimationPair::CALLBACK_ID(this, &CObjectActionUnstrappingToIdle::on_animation_end)));
    }
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionQueueWait
//////////////////////////////////////////////////////////////////////////

CObjectActionQueueWait::CObjectActionQueueWait(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, _condition_type type, LPCSTR action_name)
    : inherited(item, owner, storage, action_name), m_type(type)
{
    m_magazined = smart_cast<CWeaponMagazined*>(item);
}

void CObjectActionQueueWait::initialize()
{
    inherited::inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());
}

void CObjectActionQueueWait::execute()
{
    inherited::execute();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if (completed())
        m_magazined->StopedAfterQueueFired(false);
}

void CObjectActionQueueWait::finalize()
{
    inherited::finalize();

    // VERIFY						(m_item);
    // VERIFY						(object().inventory().ActiveItem());
    // VERIFY						(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if ((object().inventory().ActiveItem() == m_magazined) && !completed())
        m_magazined->StopedAfterQueueFired(false);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionSwitch
//////////////////////////////////////////////////////////////////////////

CObjectActionSwitch::CObjectActionSwitch(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, _condition_type type, LPCSTR action_name)
    : inherited(item, owner, storage, action_name), m_type(type)
{
}

void CObjectActionSwitch::initialize()
{
    inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());
}

void CObjectActionSwitch::execute()
{
    inherited::execute();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());
}

void CObjectActionSwitch::finalize() { inherited::finalize(); }
//////////////////////////////////////////////////////////////////////////
// CObjectActionDrop
//////////////////////////////////////////////////////////////////////////

CObjectActionDrop::CObjectActionDrop(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, LPCSTR action_name)
    : inherited(item, owner, storage, action_name)
{
}

void CObjectActionDrop::initialize()
{
    inherited::initialize();
    if (!m_item || !m_item->object().H_Parent() || (m_object->ID() != m_item->object().H_Parent()->ID()))
        return;

    NET_Packet P;
    m_object->u_EventGen(P, GE_OWNERSHIP_REJECT, m_object->ID());
    P.w_u16(u16(m_item->object().ID()));
    m_object->u_EventSend(P);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionAim
//////////////////////////////////////////////////////////////////////////

CObjectActionAim::CObjectActionAim(CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage,
    _condition_type condition_id, _value_type value, LPCSTR action_name)
    : inherited(item, owner, storage, condition_id, value, action_name)
{
    m_weapon = smart_cast<CWeaponMagazined*>(m_item);
    //	VERIFY						(m_weapon);
}

void CObjectActionAim::initialize()
{
    inherited::inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());
}

void CObjectActionAim::execute()
{
    inherited::execute();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if (m_weapon && completed())
        m_weapon->StopedAfterQueueFired(false);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionIdle
//////////////////////////////////////////////////////////////////////////

CObjectActionIdle::CObjectActionIdle(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, LPCSTR action_name)
    : inherited(item, owner, storage, action_name)
{
}

void CObjectActionIdle::initialize()
{
    inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    if (m_storage->property(ObjectHandlerSpace::eWorldPropertyUseEnough))
        object().CObjectHandler::set_goal(MonsterSpace::eObjectActionActivate, object().inventory().ActiveItem());
    m_storage->set_property(ObjectHandlerSpace::eWorldPropertyUseEnough, false);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionIdleMissile
//////////////////////////////////////////////////////////////////////////

CObjectActionIdleMissile::CObjectActionIdleMissile(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, LPCSTR action_name)
    : inherited(item, owner, storage, action_name)
{
}

void CObjectActionIdleMissile::initialize()
{
    inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    m_storage->set_property(
        object().planner().uid(m_item->object().ID(), ObjectHandlerSpace::eWorldPropertyThrowStarted), false);
    m_storage->set_property(
        object().planner().uid(m_item->object().ID(), ObjectHandlerSpace::eWorldPropertyThrowIdle), false);
    m_storage->set_property(
        object().planner().uid(m_item->object().ID(), ObjectHandlerSpace::eWorldPropertyFiring1), false);
}

//////////////////////////////////////////////////////////////////////////
// CObjectActionThrowMissile
//////////////////////////////////////////////////////////////////////////

CObjectActionThrowMissile::CObjectActionThrowMissile(
    CInventoryItem* item, CAI_Stalker* owner, CPropertyStorage* storage, LPCSTR action_name)
    : inherited(item, owner, storage, action_name)
{
}

void CObjectActionThrowMissile::initialize()
{
    inherited::initialize();

    VERIFY(m_item);
    VERIFY(object().inventory().ActiveItem());
    VERIFY(object().inventory().ActiveItem()->object().ID() == m_item->object().ID());

    object().inventory().Action(kWPN_ZOOM, CMD_START);

    float distance = object().throw_target().distance_to(object().Position());
    if (distance > 45)
    {
        set_inertia_time(2500);
        return;
    }

    if (distance > 30)
    {
        set_inertia_time(2000);
        return;
    }

    if (distance > 15)
    {
        set_inertia_time(1500);
        return;
    }

    set_inertia_time(1000);
}

void CObjectActionThrowMissile::execute()
{
    inherited::execute();
    if (completed())
        object().inventory().Action(kWPN_ZOOM, CMD_STOP);
}
