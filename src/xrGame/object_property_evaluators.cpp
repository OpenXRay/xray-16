////////////////////////////////////////////////////////////////////////////
//	Module 		: object_property_evaluators.cpp
//	Created 	: 12.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Object property evaluators
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "object_property_evaluators.h"
#include "Weapon.h"
#include "ai/stalker/ai_stalker.h"
#include "Inventory.h"
#include "Missile.h"
#include "FoodItem.h"
#include "WeaponMagazined.h"

//////////////////////////////////////////////////////////////////////////
// CObjectPropertyEvaluatorState
//////////////////////////////////////////////////////////////////////////

CObjectPropertyEvaluatorState::CObjectPropertyEvaluatorState(
    CWeapon* item, CAI_Stalker* owner, u32 state, bool equality)
    : inherited(item, owner), m_state(state), m_equality(equality)
{
}

CObjectPropertyEvaluatorState::_value_type CObjectPropertyEvaluatorState::evaluate()
{
    VERIFY(m_item);
    return (_value_type((m_item->GetState() == m_state) == m_equality));
}

CObjectPropertyEvaluatorWeaponHidden::CObjectPropertyEvaluatorWeaponHidden(CWeapon* item, CAI_Stalker* owner)
    : inherited(item, owner)
{
}

CObjectPropertyEvaluatorWeaponHidden::_value_type CObjectPropertyEvaluatorWeaponHidden::evaluate()
{
    VERIFY(m_item);

    return ((m_item != m_item->m_pInventory->ActiveItem()) || (m_item->GetState() == CWeapon::eShowing));
}
//////////////////////////////////////////////////////////////////////////
// CObjectPropertyEvaluatorAmmo
//////////////////////////////////////////////////////////////////////////

CObjectPropertyEvaluatorAmmo::CObjectPropertyEvaluatorAmmo(CWeapon* item, CAI_Stalker* owner, u32 ammo_type)
    : inherited(item, owner), m_ammo_type(ammo_type)
{
}

CObjectPropertyEvaluatorAmmo::_value_type CObjectPropertyEvaluatorAmmo::evaluate()
{
    if (!m_ammo_type)
        return (_value_type(!!m_item->GetSuitableAmmoTotal()));
    else
        return (_value_type(false));
}

//////////////////////////////////////////////////////////////////////////
// CObjectPropertyEvaluatorEmpty
//////////////////////////////////////////////////////////////////////////

CObjectPropertyEvaluatorEmpty::CObjectPropertyEvaluatorEmpty(CWeapon* item, CAI_Stalker* owner, u32 ammo_type)
    : inherited(item, owner), m_ammo_type(ammo_type)
{
}

CObjectPropertyEvaluatorEmpty::_value_type CObjectPropertyEvaluatorEmpty::evaluate()
{
    if (!m_ammo_type)
        return (_value_type(!m_item->GetAmmoElapsed()));
    else
        return (_value_type(false));
}

//////////////////////////////////////////////////////////////////////////
// CObjectPropertyEvaluatorFull
//////////////////////////////////////////////////////////////////////////

CObjectPropertyEvaluatorFull::CObjectPropertyEvaluatorFull(CWeapon* item, CAI_Stalker* owner, u32 ammo_type)
    : inherited(item, owner), m_ammo_type(ammo_type)
{
}

CObjectPropertyEvaluatorFull::_value_type CObjectPropertyEvaluatorFull::evaluate()
{
    if (!m_ammo_type)
        return (_value_type(m_item->GetAmmoElapsed() == m_item->GetAmmoMagSize()));
    else
        return (_value_type(false));
}

//////////////////////////////////////////////////////////////////////////
// CObjectPropertyEvaluatorReady
//////////////////////////////////////////////////////////////////////////

CObjectPropertyEvaluatorReady::CObjectPropertyEvaluatorReady(CWeapon* item, CAI_Stalker* owner, u32 ammo_type)
    : inherited(item, owner), m_ammo_type(ammo_type)
{
}

CObjectPropertyEvaluatorReady::_value_type CObjectPropertyEvaluatorReady::evaluate()
{
    if (!m_ammo_type)
        //		return		(_value_type(!m_item->IsMisfire() && m_item->GetAmmoElapsed()));
        return (_value_type(
            !m_item->IsMisfire() && (m_item->GetAmmoElapsed() && (m_item->GetState() != CWeapon::eReload))));
    else
        return (_value_type(false));
}

//////////////////////////////////////////////////////////////////////////
// CObjectPropertyEvaluatorQueue
//////////////////////////////////////////////////////////////////////////

CObjectPropertyEvaluatorQueue::CObjectPropertyEvaluatorQueue(CWeapon* item, CAI_Stalker* owner, u32 type)
    : inherited(item, owner), m_type(type)
{
    m_magazined = smart_cast<CWeaponMagazined*>(item);
}

CObjectPropertyEvaluatorQueue::_value_type CObjectPropertyEvaluatorQueue::evaluate()
{
    return (!m_magazined ? true : !m_magazined->StopedAfterQueueFired());
}

//////////////////////////////////////////////////////////////////////////
// CObjectPropertyEvaluatorNoItems
//////////////////////////////////////////////////////////////////////////

CObjectPropertyEvaluatorNoItems::CObjectPropertyEvaluatorNoItems(CAI_Stalker* owner) { m_object = owner; }
CObjectPropertyEvaluatorNoItems::_value_type CObjectPropertyEvaluatorNoItems::evaluate()
{
    PIItem I = object().inventory().ActiveItem();
    if (!I)
        return (true);

    if (!I->cast_hud_item() || I->cast_hud_item()->IsHidden())
        return (true);

    if (I->cast_hud_item() && I->cast_hud_item()->IsShowing())
        return (true);

    return (false);
}

//////////////////////////////////////////////////////////////////////////
// CObjectPropertyEvaluatorMissile
//////////////////////////////////////////////////////////////////////////

CObjectPropertyEvaluatorMissile::CObjectPropertyEvaluatorMissile(
    CMissile* item, CAI_Stalker* owner, u32 state, bool equality)
    : inherited(item, owner), m_state(state), m_equality(equality)
{
}

CObjectPropertyEvaluatorMissile::_value_type CObjectPropertyEvaluatorMissile::evaluate()
{
    VERIFY(m_item);
    return (_value_type((m_item->GetState() == m_state) == m_equality));
}

//////////////////////////////////////////////////////////////////////////
// CObjectPropertyEvaluatorMissileStarted
//////////////////////////////////////////////////////////////////////////

CObjectPropertyEvaluatorMissileStarted::CObjectPropertyEvaluatorMissileStarted(CMissile* item, CAI_Stalker* owner)
    : inherited(item, owner)
{
}

CObjectPropertyEvaluatorMissileStarted::_value_type CObjectPropertyEvaluatorMissileStarted::evaluate()
{
    VERIFY(m_item);
    if (m_item->GetState() != CMissile::eThrow)
        return (false);

    return (true);
}

//////////////////////////////////////////////////////////////////////////
// CObjectPropertyEvaluatorMissileHidden
//////////////////////////////////////////////////////////////////////////

CObjectPropertyEvaluatorMissileHidden::CObjectPropertyEvaluatorMissileHidden(CMissile* item, CAI_Stalker* owner)
    : inherited(item, owner)
{
}

CObjectPropertyEvaluatorMissileHidden::_value_type CObjectPropertyEvaluatorMissileHidden::evaluate()
{
    VERIFY(m_item);

    if (!object().inventory().ActiveItem())
        return (true);

    if (object().inventory().ActiveItem() != m_item)
        return (true);

    if (m_item->GetState() == CMissile::eHidden)
        return (true);

    if (m_item->GetState() == CMissile::eShowing)
        return (true);

    return (false);
}
