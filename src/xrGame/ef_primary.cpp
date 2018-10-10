////////////////////////////////////////////////////////////////////////////
//	Module 		: ef_primary.cpp
//	Created 	: 13.06.2003
//  Modified 	: 13.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Primary evaluation function classes
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Weapon.h"
#include "entity_alive.h"
#include "InventoryOwner.h"
#include "alife_simulator.h"
#include "ef_storage.h"
#include "ai_space.h"
#include "xrAICore/Navigation/game_graph.h"
#include "Inventory.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "ef_primary.h"
#include "alife_human_brain.h"
#include "alife_human_object_handler.h"

//#define NO_HUMAN_BRAIN

IC CLASS_ID CBaseFunction::clsid_member_item() const
{
    CLASS_ID result;
    if (ef_storage().non_alife().member_item())
        result = ef_storage().non_alife().member_item()->CLS_ID;
    else
    {
        VERIFY2(ef_storage().alife().member_item(), "No object specified for evaluation function");
        result = ef_storage().alife().member_item()->m_tClassID;
    }
    return (result);
}

IC CLASS_ID CBaseFunction::clsid_enemy_item() const
{
    CLASS_ID result;
    if (ef_storage().non_alife().enemy_item())
        result = ef_storage().non_alife().enemy_item()->CLS_ID;
    else
    {
        VERIFY2(ef_storage().alife().enemy_item(), "No object specified for evaluation function");
        result = ef_storage().alife().enemy_item()->m_tClassID;
    }
    return (result);
}

IC CLASS_ID CBaseFunction::clsid_member() const
{
    CLASS_ID result;
    if (ef_storage().non_alife().member())
        result = ef_storage().non_alife().member()->CLS_ID;
    else
    {
        VERIFY2(ef_storage().alife().member(), "No object specified for evaluation function");
        const CSE_ALifeDynamicObject* l_tpALifeDynamicObject =
            smart_cast<const CSE_ALifeDynamicObject*>(ef_storage().alife().member());
        VERIFY2(l_tpALifeDynamicObject, "Invalid object passed to the evaluation function");
        result = l_tpALifeDynamicObject->m_tClassID;
    }
    return (result);
}

IC CLASS_ID CBaseFunction::clsid_enemy() const
{
    CLASS_ID result;
    if (ef_storage().non_alife().enemy())
        result = ef_storage().non_alife().enemy()->CLS_ID;
    else
    {
        VERIFY2(ef_storage().alife().enemy(), "No object specified for evaluation function");
        const CSE_ALifeDynamicObject* l_tpALifeDynamicObject =
            smart_cast<const CSE_ALifeDynamicObject*>(ef_storage().alife().enemy());
        VERIFY2(l_tpALifeDynamicObject, "Invalid object passed to the evaluation function");
        result = l_tpALifeDynamicObject->m_tClassID;
    }
    return (result);
}

float CDistanceFunction::ffGetValue()
{
    if (ef_storage().non_alife().member())
        return (
            ef_storage().non_alife().member()->Position().distance_to(ef_storage().non_alife().enemy()->Position()));
    else
        return (ef_storage().alife().member()->base()->Position().distance_to(
            ef_storage().alife().enemy()->base()->Position()));
}

float CPersonalHealthFunction::ffGetValue()
{
    if (ef_storage().non_alife().member())
    {
        m_fMaxResultValue = ef_storage().non_alife().member()->GetMaxHealth();
        return (ef_storage().non_alife().member()->GetfHealth());
    }
    else
    {
        const CSE_ALifeMonsterAbstract* l_tpALifeMonsterAbstract =
            smart_cast<const CSE_ALifeMonsterAbstract*>(ef_storage().alife().member());
        VERIFY3(l_tpALifeMonsterAbstract, "Invalid object passed to the evaluation function ", m_caName);
        m_fMaxResultValue = l_tpALifeMonsterAbstract->g_MaxHealth();
        return (l_tpALifeMonsterAbstract->get_health());
    }
}

float CPersonalMoraleFunction::ffGetValue()
{
    if (ef_storage().non_alife().member())
        return (ef_storage().non_alife().member()->m_fMorale);
    else
    {
        const CSE_ALifeMonsterAbstract* l_tpALifeMonsterAbstract =
            smart_cast<const CSE_ALifeMonsterAbstract*>(ef_storage().alife().member());
        VERIFY3(l_tpALifeMonsterAbstract, "Invalid object passed to the evaluation function ", m_caName);
        return (l_tpALifeMonsterAbstract->m_fMorale);
    }
}

float CPersonalCreatureTypeFunction::ffGetValue()
{
    u32 result;
    if (ef_storage().non_alife().member())
        result = ef_storage().non_alife().member()->ef_creature_type();
    else
    {
        VERIFY2(ef_storage().alife().member(), "No object specified for evaluation function");
        result = ef_storage().alife().member()->ef_creature_type();
    }

    VERIFY(float(result) < m_fMaxResultValue + 1);
    return (float(result));
}

u32 CPersonalWeaponTypeFunction::dwfGetWeaponType()
{
    u32 result;
    if (ef_storage().non_alife().member_item())
        result = ef_storage().non_alife().member_item()->ef_weapon_type();
    else
    {
        VERIFY2(ef_storage().alife().member_item(), "No object specified for evaluation function");
        result = ef_storage().alife().member_item()->ef_weapon_type();
    }
    return (result);
}

float CPersonalWeaponTypeFunction::ffGetTheBestWeapon()
{
    u32 dwBestWeapon = 0;

    if (ef_storage().non_alife().member() && ef_storage().non_alife().member_item())
        return (float(dwfGetWeaponType()));

    if (ef_storage().non_alife().member())
    {
        const CInventoryOwner* tpInventoryOwner = smart_cast<const CInventoryOwner*>(ef_storage().non_alife().member());
        if (tpInventoryOwner)
        {
            u16 I = tpInventoryOwner->inventory().FirstSlot();
            u16 E = tpInventoryOwner->inventory().LastSlot();
            for (; I <= E; ++I)
            {
                PIItem iitem = tpInventoryOwner->inventory().ItemFromSlot(I);
                if (iitem)
                {
                    CWeapon* tpCustomWeapon = smart_cast<CWeapon*>(iitem);
                    if (tpCustomWeapon &&
                        (tpCustomWeapon->GetSuitableAmmoTotal(true) > tpCustomWeapon->GetAmmoMagSize() / 10))
                    {
                        ef_storage().non_alife().member_item() = tpCustomWeapon;
                        u32 dwCurrentBestWeapon = dwfGetWeaponType();
                        if (dwCurrentBestWeapon > dwBestWeapon)
                            dwBestWeapon = dwCurrentBestWeapon;
                        ef_storage().non_alife().member_item() = 0;
                    }
                }
            }
        }
    }
    else
    {
        if (!ef_storage().alife().member() || !ef_storage().alife().member()->m_tpCurrentBestWeapon)
            return (0);
        ef_storage().alife().member_item() = ef_storage().alife().member()->m_tpCurrentBestWeapon;
        dwBestWeapon = dwfGetWeaponType();
    }
    return (float(dwBestWeapon));
}

float CPersonalWeaponTypeFunction::ffGetValue()
{
    float result;
    if (ef_storage().non_alife().member())
        if (ef_storage().non_alife().member()->natural_weapon())
            result = (float)ef_storage().non_alife().member()->ef_weapon_type();
        else
            result = ffGetTheBestWeapon();
    else
    {
        VERIFY2(ef_storage().alife().member(), "No object specified for evaluation function");
        if (ef_storage().alife().member()->natural_weapon())
            result = (float)ef_storage().alife().member()->ef_weapon_type();
        else
            result = ffGetTheBestWeapon();
    }
    VERIFY(result < m_fMaxResultValue + 1.f);
    return (result);
}

float CPersonalAccuracyFunction::ffGetValue()
{
    if (ef_storage().non_alife().member())
        return (ef_storage().non_alife().member()->m_fAccuracy);
    else
    {
        const CSE_ALifeMonsterAbstract* l_tpALifeMonsterAbstract =
            smart_cast<const CSE_ALifeMonsterAbstract*>(ef_storage().alife().member());
        VERIFY3(l_tpALifeMonsterAbstract, "Invalid object passed to the evaluation function ", m_caName);
        return (l_tpALifeMonsterAbstract->m_fAccuracy);
    }
}

float CPersonalIntelligenceFunction::ffGetValue()
{
    if (ef_storage().non_alife().member())
        return (ef_storage().non_alife().member()->m_fIntelligence);
    else
    {
        const CSE_ALifeMonsterAbstract* l_tpALifeMonsterAbstract =
            smart_cast<const CSE_ALifeMonsterAbstract*>(ef_storage().alife().member());
        VERIFY3(l_tpALifeMonsterAbstract, "Invalid object passed to the evaluation function ", m_caName);
        return (l_tpALifeMonsterAbstract->m_fIntelligence);
    }
}

float CPersonalRelationFunction::ffGetValue()
{
#pragma todo("Dima to Dima : Implement relation function")
    return (0);
}

float CPersonalGreedFunction::ffGetValue()
{
#pragma todo("Dima to Dima : Implement greed function")
    return (0);
}

float CPersonalAggressivenessFunction::ffGetValue()
{
#pragma todo("Dima to Dima : Implement aggressiveness function")
    return (0);
}

float CEnemyEquipmentCostFunction::ffGetValue()
{
#pragma todo("Dima to Dima : Implement enemy equipment cost function")
    return (0);
}

float CEnemyRukzakWeightFunction::ffGetValue()
{
    float m_fLastValue;
    if (ef_storage().non_alife().member())
    {
        const CInventoryOwner* tpInventoryOwner = smart_cast<const CInventoryOwner*>(ef_storage().non_alife().member());
        if (tpInventoryOwner)
            m_fLastValue = tpInventoryOwner->inventory().TotalWeight();
        else
            m_fLastValue = 0;
    }
    else
    {
        //		CSE_ALifeHumanAbstract *l_tpALifeHumanAbstract =
        // smart_cast<CSE_ALifeHumanAbstract*>(ef_storage().alife().member());
        //		if (l_tpALifeHumanAbstract)
        //			m_fLastValue	= l_tpALifeHumanAbstract->m_fCumulativeItemMass;
        //		else
        m_fLastValue = 0;
    }
    return (m_fLastValue);
}

float CEnemyAnomalityFunction::ffGetValue()
{
#pragma todo("Dima to Dima : Implement enemy anomality function")
    return (0);
}

float CGraphPointType0::ffGetValue()
{
    return (ai().game_graph().vertex(ef_storage().alife().member_item()->m_tGraphID)->vertex_type()[0]);
}

float CPersonalEyeRange::ffGetValue()
{
    const CSE_ALifeMonsterAbstract* l_tpALifeMonsterAbstract =
        smart_cast<const CSE_ALifeMonsterAbstract*>(ef_storage().alife().member());
    VERIFY3(l_tpALifeMonsterAbstract, "Invalid object passed to the evaluation function ", m_caName);
    return (l_tpALifeMonsterAbstract->m_fEyeRange);
}

float CPersonalMaxHealth::ffGetValue()
{
    CSE_ALifeMonsterAbstract* l_tpALifeMonsterAbstract =
        smart_cast<CSE_ALifeMonsterAbstract*>(ef_storage().alife().member());
    VERIFY3(l_tpALifeMonsterAbstract, "Invalid object passed to the evaluation function ", m_caName);
    const CSE_ALifeGroupAbstract* l_tpALifeGroupAbstract =
        smart_cast<const CSE_ALifeGroupAbstract*>(ef_storage().alife().member());
    if (!l_tpALifeGroupAbstract)
        return (l_tpALifeMonsterAbstract->m_fMaxHealthValue);
    else
        return (l_tpALifeMonsterAbstract->m_fMaxHealthValue * l_tpALifeGroupAbstract->m_wCount);
}

u32 CPersonalMaxHealth::dwfGetDiscreteValue(u32 dwDiscretizationValue)
{
    float fTemp = ffGetValue();
    if (fTemp <= m_fMinResultValue)
        return (0);
    else if (fTemp >= m_fMaxResultValue)
        return (dwDiscretizationValue - 1);
    else
    {
        if (fTemp <= 30)
            return (iFloor(1 * float(dwDiscretizationValue) / 10 + .5f));
        if (fTemp <= 50)
            return (iFloor(2 * float(dwDiscretizationValue) / 10 + .5f));
        if (fTemp <= 80)
            return (iFloor(3 * float(dwDiscretizationValue) / 10 + .5f));
        if (fTemp <= 100)
            return (iFloor(4 * float(dwDiscretizationValue) / 10 + .5f));
        if (fTemp <= 150)
            return (iFloor(5 * float(dwDiscretizationValue) / 10 + .5f));
        if (fTemp <= 250)
            return (iFloor(6 * float(dwDiscretizationValue) / 10 + .5f));
        if (fTemp <= 500)
            return (iFloor(7 * float(dwDiscretizationValue) / 10 + .5f));
        if (fTemp <= 750)
            return (iFloor(8 * float(dwDiscretizationValue) / 10 + .5f));
        return (iFloor(9 * float(dwDiscretizationValue) / 10 + .5f));
    }
}

float CEquipmentType::ffGetValue()
{
    u32 result;
    if (ef_storage().non_alife().member_item())
        result = ef_storage().non_alife().member_item()->ef_equipment_type();
    else
    {
        VERIFY2(ef_storage().alife().member_item(), "No object specified for evaluation function");
        result = ef_storage().alife().member_item()->ef_equipment_type();
    }
    VERIFY(float(result) < m_fMaxResultValue + 1.f);
    return (float(result));
}

float CItemDeterioration::ffGetValue()
{
    if (ef_storage().non_alife().member_item())
    {
        const CWeapon* weapon = smart_cast<const CWeapon*>(ef_storage().non_alife().member_item());
        if (weapon)
            return (1.f - weapon->GetCondition());

#pragma todo("Dima to Dima : Append ItemDeterioration with non-ALife non-weapon branch")
        return (0.f);
    }
    else
    {
        const CSE_ALifeInventoryItem* l_tpALifeInventoryItem =
            smart_cast<const CSE_ALifeInventoryItem*>(ef_storage().alife().member_item());
        R_ASSERT2(l_tpALifeInventoryItem, "Non-item object specified for the ItemDeterioration evaluation function");
        return (l_tpALifeInventoryItem->m_fDeteriorationValue);
    }
}

#ifndef NO_HUMAN_BRAIN
float CEquipmentPreference::ffGetValue()
{
    if (ef_storage().non_alife().member())
    {
#pragma todo("Dima to Dima : Append EquipmentPreference with non-ALife branch")
        return (0);
    }
    else
    {
        CSE_ALifeHumanAbstract* l_tpALifeHumanAbstract =
            smart_cast<CSE_ALifeHumanAbstract*>(ef_storage().alife().member());
        R_ASSERT2(l_tpALifeHumanAbstract, "Non-human object in EquipmentPreference evaluation function");
        return (l_tpALifeHumanAbstract->brain()
                    .m_cpEquipmentPreferences[ef_storage().m_pfEquipmentType->dwfGetDiscreteValue()]);
    }
}

float CMainWeaponType::ffGetValue()
{
    u32 result;
    if (ef_storage().non_alife().member_item())
        result = ef_storage().non_alife().member_item()->ef_main_weapon_type();
    else
    {
        VERIFY2(ef_storage().alife().member_item(), "No object specified for evaluation function");
        result = ef_storage().alife().member_item()->ef_main_weapon_type();
    }
    VERIFY(float(result) < m_fMaxResultValue + 1.f);
    return (float(result));
}

float CMainWeaponPreference::ffGetValue()
{
    if (ef_storage().non_alife().member())
    {
#pragma todo("Dima to Dima : Append MainWeaponPreference with non-ALife branch")
        return (0);
    }
    else
    {
        CSE_ALifeHumanAbstract* l_tpALifeHumanAbstract =
            smart_cast<CSE_ALifeHumanAbstract*>(ef_storage().alife().member());
        R_ASSERT2(l_tpALifeHumanAbstract, "Non-human object in EquipmentPreference evaluation function");
        return (l_tpALifeHumanAbstract->brain()
                    .m_cpMainWeaponPreferences[ef_storage().m_pfMainWeaponType->dwfGetDiscreteValue(
                        iFloor(ef_storage().m_pfMainWeaponType->ffGetMaxResultValue() + .5f))]);
    }
}
#else
float CEquipmentPreference::ffGetValue()
{
    if (ef_storage().non_alife().member())
    {
#pragma todo("Dima to Dima : Append EquipmentPreference with non-ALife branch")
        return (0);
    }
    else
    {
        CSE_ALifeHumanAbstract* l_tpALifeHumanAbstract =
            smart_cast<CSE_ALifeHumanAbstract*>(ef_storage().alife().member());
        R_ASSERT2(l_tpALifeHumanAbstract, "Non-human object in EquipmentPreference evaluation function");
        return (
            l_tpALifeHumanAbstract->m_cpEquipmentPreferences[ef_storage().m_pfEquipmentType->dwfGetDiscreteValue()]);
    }
}

float CMainWeaponType::ffGetValue()
{
    u32 result;
    if (ef_storage().non_alife().member_item())
        result = ef_storage().non_alife().member_item()->ef_main_weapon_type();
    else
    {
        VERIFY2(ef_storage().alife().member_item(), "No object specified for evaluation function");
        result = ef_storage().alife().member_item()->ef_main_weapon_type();
    }
    VERIFY(float(result) < m_fMaxResultValue + 1.f);
    return (float(result));
}

float CMainWeaponPreference::ffGetValue()
{
    if (ef_storage().non_alife().member())
    {
#pragma todo("Dima to Dima : Append MainWeaponPreference with non-ALife branch")
        return (0);
    }
    else
    {
        CSE_ALifeHumanAbstract* l_tpALifeHumanAbstract =
            smart_cast<CSE_ALifeHumanAbstract*>(ef_storage().alife().member());
        R_ASSERT2(l_tpALifeHumanAbstract, "Non-human object in EquipmentPreference evaluation function");
        return (l_tpALifeHumanAbstract->m_cpMainWeaponPreferences[ef_storage().m_pfMainWeaponType->dwfGetDiscreteValue(
            iFloor(ef_storage().m_pfMainWeaponType->ffGetMaxResultValue() + .5f))]);
    }
}
#endif

float CItemValue::ffGetValue()
{
    if (ef_storage().non_alife().member())
    {
#pragma todo("Dima to Dima : Append ItemValue with non-ALife branch")
        return (0);
    }
    else
    {
        const CSE_ALifeInventoryItem* l_tpALifeInventoryItem =
            smart_cast<const CSE_ALifeInventoryItem*>(ef_storage().alife().member_item());
        R_ASSERT2(l_tpALifeInventoryItem, "Non-item object specified for the ItemDeterioration evaluation function");
        return (float(l_tpALifeInventoryItem->m_dwCost));
    }
}

#ifndef NO_HUMAN_BRAIN
float CWeaponAmmoCount::ffGetValue()
{
    if (ef_storage().non_alife().member())
    {
#pragma todo("Dima to Dima : Append WeaponAmmoCount with non-ALife branch")
        return (0);
    }
    else
    {
        CSE_ALifeHumanAbstract* l_tpALifeHumanAbstract =
            smart_cast<CSE_ALifeHumanAbstract*>(ef_storage().alife().member());
        R_ASSERT2(l_tpALifeHumanAbstract, "Non-human object in WeaponAmmoCount evaluation function");
        return (l_tpALifeHumanAbstract->brain().objects().get_available_ammo_count(
            smart_cast<const CSE_ALifeItemWeapon*>(ef_storage().alife().member_item()),
            l_tpALifeHumanAbstract->alife().m_temp_item_vector));
    }
}
#else
float CWeaponAmmoCount::ffGetValue()
{
    if (ef_storage().non_alife().member())
    {
#pragma todo("Dima to Dima : Append WeaponAmmoCount with non-ALife branch")
        return (0);
    }
    else
    {
        CSE_ALifeHumanAbstract* l_tpALifeHumanAbstract =
            smart_cast<CSE_ALifeHumanAbstract*>(ef_storage().alife().member());
        R_ASSERT2(l_tpALifeHumanAbstract, "Non-human object in WeaponAmmoCount evaluation function");
        return (l_tpALifeHumanAbstract->get_available_ammo_count(
            smart_cast<const CSE_ALifeItemWeapon*>(ef_storage().alife().member_item()),
            l_tpALifeHumanAbstract->alife().m_temp_item_vector));
    }
}
#endif

u32 CWeaponAmmoCount::dwfGetDiscreteValue(u32 dwDiscretizationValue)
{
    float fTemp = ffGetValue();
    if (fTemp <= m_fMinResultValue)
        return (0);
    else if (fTemp >= m_fMaxResultValue)
        return (dwDiscretizationValue - 1);
    else
    {
        const CSE_ALifeItemWeapon* l_tpALifeItemWeapon =
            smart_cast<const CSE_ALifeItemWeapon*>(ef_storage().alife().member_item());
        if (l_tpALifeItemWeapon && l_tpALifeItemWeapon->m_caAmmoSections)
        {
            string32 S;
            _GetItem(l_tpALifeItemWeapon->m_caAmmoSections, 0, S);
            u32 l_dwBoxSize = pSettings->r_s32(S, "box_size");
            if (fTemp <= 3 * l_dwBoxSize)
                return (iFloor(1 * float(dwDiscretizationValue) / 10 + .5f));
            return (iFloor(2 * float(dwDiscretizationValue) / 10 + .5f));
        }
        else
            return (dwDiscretizationValue - 1);
    }
}

float CEnemyAnomalyType::ffGetValue()
{
    u32 result;
    if (ef_storage().non_alife().enemy())
        result = ef_storage().non_alife().enemy()->ef_anomaly_type();
    else
    {
        VERIFY2(ef_storage().alife().enemy(), "No object specified for evaluation function");
        result = ef_storage().alife().enemy()->ef_anomaly_type();
    }
    VERIFY(float(result) < m_fMaxResultValue + 1.f);
    return (float(result));
}

float CDetectorType::ffGetValue()
{
    if (ef_storage().non_alife().member())
    {
        if (!ef_storage().non_alife().member_item())
            return (0);
    }
    else
    {
        if (!ef_storage().alife().member_item())
            return (0);
    }

    u32 result;
    if (ef_storage().non_alife().member())
        if (ef_storage().non_alife().member()->natural_detector())
            result = ef_storage().non_alife().member()->ef_detector_type();
        else
            result = ef_storage().non_alife().member_item()->ef_detector_type();
    else
    {
        VERIFY2(ef_storage().alife().member(), "No object specified for evaluation function");
        if (ef_storage().alife().member()->natural_detector())
            result = ef_storage().alife().member()->ef_detector_type();
        else
            result = ef_storage().alife().member_item()->ef_detector_type();
    }
    VERIFY(float(result) < m_fMaxResultValue + 1.f);
    return (float(result));
}

float CEnemyDistanceToGraphPoint::ffGetValue()
{
    CSE_ALifeDynamicObject* l_tpALifeDynamicObject = smart_cast<CSE_ALifeDynamicObject*>(ef_storage().alife().enemy());
    R_ASSERT3(l_tpALifeDynamicObject, "Invalid object passed to the evaluation function ", m_caName);
    if (l_tpALifeDynamicObject->m_fDistance < 5.f)
        return (0);
    if (l_tpALifeDynamicObject->m_fDistance < 10.f)
        return (1);
    if (l_tpALifeDynamicObject->m_fDistance < 15.f)
        return (2);
    if (l_tpALifeDynamicObject->m_fDistance < 20.f)
        return (3);
    return (4);
}
