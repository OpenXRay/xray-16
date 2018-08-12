// WeaponDispersion.cpp: разбос при стрельбе
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "Weapon.h"
#include "InventoryOwner.h"
#include "Actor.h"
#include "inventory_item_impl.h"

#include "ActorEffector.h"
#include "EffectorShot.h"
#include "EffectorShotX.h"

//возвращает 1, если оружие в отличном состоянии и >1 если повреждено
float CWeapon::GetConditionDispersionFactor() const
{
    return (1.f + fireDispersionConditionFactor * (1.f - GetCondition()));
}

float CWeapon::GetFireDispersion(bool with_cartridge, bool for_crosshair)
{
    if (!with_cartridge)
        return GetFireDispersion(1.0f, for_crosshair);
    if (!m_magazine.empty())
        m_fCurrentCartirdgeDisp = m_magazine.back().param_s.kDisp;
    return GetFireDispersion(m_fCurrentCartirdgeDisp, for_crosshair);
}

float CWeapon::GetBaseDispersion(float cartridge_k)
{
    return fireDispersionBase * cur_silencer_koef.fire_dispersion * cartridge_k * GetConditionDispersionFactor();
}

//текущая дисперсия (в радианах) оружия с учетом используемого патрона
float CWeapon::GetFireDispersion(float cartridge_k, bool for_crosshair)
{
    //учет базовой дисперсии, состояние оружия и влияение патрона
    float fire_disp = GetBaseDispersion(cartridge_k);

    //вычислить дисперсию, вносимую самим стрелком
    if (H_Parent())
    {
        const CInventoryOwner* pOwner = smart_cast<const CInventoryOwner*>(H_Parent());
        float parent_disp = pOwner->GetWeaponAccuracy();
        fire_disp += parent_disp;
    }

    return fire_disp;
}

//////////////////////////////////////////////////////////////////////////
// Для эффекта отдачи оружия
void CWeapon::AddShotEffector() { inventory_owner().on_weapon_shot_start(this); }
void CWeapon::RemoveShotEffector()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
    if (pInventoryOwner)
        pInventoryOwner->on_weapon_shot_remove(this);
}

void CWeapon::ClearShotEffector()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
    if (pInventoryOwner)
        pInventoryOwner->on_weapon_hide(this);
}

void CWeapon::StopShotEffector()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
    if (pInventoryOwner)
        pInventoryOwner->on_weapon_shot_stop();
}
