#include "pch_script.h"
#include "ActorCondition.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "Inventory.h"
#include "Level.h"
#include "SleepEffector.h"
#include "game_base_space.h"
#include "autosave_manager.h"
#include "xrServer.h"
#include "ai_space.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"
#include "game_object_space.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "Common/object_broker.h"
#include "Weapon.h"

#include "PDA.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "UIGameCustom.h"
#include "ui/UIMainIngameWnd.h"
#include "xrUICore/Static/UIStatic.h"

#define MAX_SATIETY 1.0f
#define START_SATIETY 0.5f

BOOL GodMode()
{
    if (GameID() == eGameIDSingle)
        return psActorFlags.test(AF_GODMODE | AF_GODMODE_RT);
    return FALSE;
}

CActorCondition::CActorCondition(CActor* object) : inherited(object)
{
    m_fJumpPower = 0.f;
    m_fStandPower = 0.f;
    m_fWalkPower = 0.f;
    m_fJumpWeightPower = 0.f;
    m_fWalkWeightPower = 0.f;
    m_fOverweightWalkK = 0.f;
    m_fOverweightJumpK = 0.f;
    m_fAccelK = 0.f;
    m_fSprintK = 0.f;
    m_fAlcohol = 0.f;
    m_fSatiety = 1.0f;

    //	m_vecBoosts.clear();

    VERIFY(object);
    m_object = object;
    m_condition_flags.zero();
    m_death_effector = NULL;

    m_zone_max_power[ALife::infl_rad] = 1.0f;
    m_zone_max_power[ALife::infl_fire] = 1.0f;
    m_zone_max_power[ALife::infl_acid] = 1.0f;
    m_zone_max_power[ALife::infl_psi] = 1.0f;
    m_zone_max_power[ALife::infl_electra] = 1.0f;

    m_zone_danger[ALife::infl_rad] = 0.0f;
    m_zone_danger[ALife::infl_fire] = 0.0f;
    m_zone_danger[ALife::infl_acid] = 0.0f;
    m_zone_danger[ALife::infl_psi] = 0.0f;
    m_zone_danger[ALife::infl_electra] = 0.0f;
    m_f_time_affected = Device.fTimeGlobal;

    m_max_power_restore_speed = 0.0f;
    m_max_wound_protection = 0.0f;
    m_max_fire_wound_protection = 0.0f;
}

CActorCondition::~CActorCondition() { xr_delete(m_death_effector); }
void CActorCondition::LoadCondition(LPCSTR entity_section)
{
    inherited::LoadCondition(entity_section);

    LPCSTR section = READ_IF_EXISTS(pSettings, r_string, entity_section, "condition_sect", entity_section);

    m_fJumpPower = pSettings->r_float(section, "jump_power");
    m_fStandPower = pSettings->r_float(section, "stand_power");
    m_fWalkPower = pSettings->r_float(section, "walk_power");
    m_fJumpWeightPower = pSettings->r_float(section, "jump_weight_power");
    m_fWalkWeightPower = pSettings->r_float(section, "walk_weight_power");
    m_fOverweightWalkK = pSettings->r_float(section, "overweight_walk_k");
    m_fOverweightJumpK = pSettings->r_float(section, "overweight_jump_k");
    m_fAccelK = pSettings->r_float(section, "accel_k");
    m_fSprintK = pSettings->r_float(section, "sprint_k");

    //порог силы и здоровья меньше которого актер начинает хромать
    m_fLimpingHealthBegin = pSettings->r_float(section, "limping_health_begin");
    m_fLimpingHealthEnd = pSettings->r_float(section, "limping_health_end");
    R_ASSERT(m_fLimpingHealthBegin <= m_fLimpingHealthEnd);

    m_fLimpingPowerBegin = pSettings->r_float(section, "limping_power_begin");
    m_fLimpingPowerEnd = pSettings->r_float(section, "limping_power_end");
    R_ASSERT(m_fLimpingPowerBegin <= m_fLimpingPowerEnd);

    m_fCantWalkPowerBegin = pSettings->r_float(section, "cant_walk_power_begin");
    m_fCantWalkPowerEnd = pSettings->r_float(section, "cant_walk_power_end");
    R_ASSERT(m_fCantWalkPowerBegin <= m_fCantWalkPowerEnd);

    m_fCantSprintPowerBegin = pSettings->r_float(section, "cant_sprint_power_begin");
    m_fCantSprintPowerEnd = pSettings->r_float(section, "cant_sprint_power_end");
    R_ASSERT(m_fCantSprintPowerBegin <= m_fCantSprintPowerEnd);

    m_fPowerLeakSpeed = pSettings->r_float(section, "max_power_leak_speed");

    m_fV_Alcohol = pSettings->r_float(section, "alcohol_v");

    m_fSatietyCritical = pSettings->r_float(section, "satiety_critical");
    clamp(m_fSatietyCritical, 0.0f, 1.0f);
    m_fV_Satiety = pSettings->r_float(section, "satiety_v");
    m_fV_SatietyPower = pSettings->r_float(section, "satiety_power_v");
    m_fV_SatietyHealth = pSettings->r_float(section, "satiety_health_v");

    m_MaxWalkWeight = pSettings->r_float(section, "max_walk_weight");

    m_zone_max_power[ALife::infl_rad] = pSettings->r_float(section, "radio_zone_max_power");
    m_zone_max_power[ALife::infl_fire] = pSettings->r_float(section, "fire_zone_max_power");
    m_zone_max_power[ALife::infl_acid] = pSettings->r_float(section, "acid_zone_max_power");
    m_zone_max_power[ALife::infl_psi] = pSettings->r_float(section, "psi_zone_max_power");
    m_zone_max_power[ALife::infl_electra] = pSettings->r_float(section, "electra_zone_max_power");

    m_max_power_restore_speed = pSettings->r_float(section, "max_power_restore_speed");
    m_max_wound_protection = READ_IF_EXISTS(pSettings, r_float, section, "max_wound_protection", 1.0f);
    m_max_fire_wound_protection = READ_IF_EXISTS(pSettings, r_float, section, "max_fire_wound_protection", 1.0f);

    VERIFY(!fis_zero(m_zone_max_power[ALife::infl_rad]));
    VERIFY(!fis_zero(m_zone_max_power[ALife::infl_fire]));
    VERIFY(!fis_zero(m_zone_max_power[ALife::infl_acid]));
    VERIFY(!fis_zero(m_zone_max_power[ALife::infl_psi]));
    VERIFY(!fis_zero(m_zone_max_power[ALife::infl_electra]));
    VERIFY(!fis_zero(m_max_power_restore_speed));
}

float CActorCondition::GetZoneMaxPower(ALife::EInfluenceType type) const
{
    if (type < ALife::infl_rad || ALife::infl_electra < type)
    {
        return 1.0f;
    }
    return m_zone_max_power[type];
}

float CActorCondition::GetZoneMaxPower(ALife::EHitType hit_type) const
{
    ALife::EInfluenceType iz_type = ALife::infl_max_count;
    switch (hit_type)
    {
    case ALife::eHitTypeRadiation: iz_type = ALife::infl_rad; break;
    case ALife::eHitTypeLightBurn: iz_type = ALife::infl_fire; break;
    case ALife::eHitTypeBurn: iz_type = ALife::infl_fire; break;
    case ALife::eHitTypeChemicalBurn: iz_type = ALife::infl_acid; break;
    case ALife::eHitTypeTelepatic: iz_type = ALife::infl_psi; break;
    case ALife::eHitTypeShock: iz_type = ALife::infl_electra; break;

    case ALife::eHitTypeStrike:
    case ALife::eHitTypeExplosion:
    case ALife::eHitTypeFireWound:
    case ALife::eHitTypeWound_2:
        //	case ALife::eHitTypePhysicStrike:
        return 1.0f;
    case ALife::eHitTypeWound: return m_max_wound_protection;
    default: NODEFAULT;
    }

    return GetZoneMaxPower(iz_type);
}

void CActorCondition::UpdateCondition()
{
    if (psActorFlags.test(AF_GODMODE_RT))
    {
        UpdateSatiety();
        UpdateBoosters();

        m_fAlcohol += m_fV_Alcohol * m_fDeltaTime;
        clamp(m_fAlcohol, 0.0f, 1.0f);
        if (IsGameTypeSingle())
        {
            CEffectorCam* ce = Actor()->Cameras().GetCamEffector((ECamEffectorType)effAlcohol);
            if (ce)
                RemoveEffector(m_object, effAlcohol);
        }
    }

    if (GodMode())
        return;
    if (!object().g_Alive())
        return;
    if (!object().Local() && m_object != Level().CurrentViewEntity())
        return;

    float base_weight = object().MaxCarryWeight();
    float cur_weight = object().inventory().TotalWeight();

    if ((object().mstate_real & mcAnyMove))
    {
        ConditionWalk(cur_weight / base_weight, isActorAccelerated(object().mstate_real, object().IsZoomAimingMode()),
            (object().mstate_real & mcSprint) != 0);
    }
    else
    {
        ConditionStand(cur_weight / base_weight);
    }

    if (IsGameTypeSingle())
    {
        float k_max_power = 1.0f;
        if (true)
        {
            k_max_power =
                1.0f + std::min(cur_weight, base_weight) / base_weight + std::max(0.0f, (cur_weight - base_weight) / 10.0f);
        }
        else
        {
            k_max_power = 1.0f;
        }
        SetMaxPower(GetMaxPower() - m_fPowerLeakSpeed * m_fDeltaTime * k_max_power);
    }

    m_fAlcohol += m_fV_Alcohol * m_fDeltaTime;
    clamp(m_fAlcohol, 0.0f, 1.0f);

    if (IsGameTypeSingle())
    {
        CEffectorCam* ce = Actor()->Cameras().GetCamEffector((ECamEffectorType)effAlcohol);
        if ((m_fAlcohol > 0.0001f))
        {
            if (!ce)
            {
                AddEffector(
                    m_object, effAlcohol, "effector_alcohol", GET_KOEFF_FUNC(this, &CActorCondition::GetAlcohol));
            }
        }
        else
        {
            if (ce)
                RemoveEffector(m_object, effAlcohol);
        }

        string512 pp_sect_name;
        shared_str ln = Level().name();
        if (ln.size())
        {
            CEffectorPP* ppe = object().Cameras().GetPPEffector((EEffectorPPType)effPsyHealth);

            strconcat(sizeof(pp_sect_name), pp_sect_name, "effector_psy_health", "_", *ln);
            if (!pSettings->section_exist(pp_sect_name))
                xr_strcpy(pp_sect_name, "effector_psy_health");

            if (!fsimilar(GetPsyHealth(), 1.0f, 0.05f))
            {
                if (!ppe)
                {
                    AddEffector(m_object, effPsyHealth, pp_sect_name, GET_KOEFF_FUNC(this, &CActorCondition::GetPsy));
                }
            }
            else
            {
                if (ppe)
                    RemoveEffector(m_object, effPsyHealth);
            }
        }
        //-		if(fis_zero(GetPsyHealth()))
        //-			SetHealth( 0.0f );
    };

    UpdateSatiety();
    UpdateBoosters();

    inherited::UpdateCondition();

    if (IsGameTypeSingle())
        UpdateTutorialThresholds();

    if (GetHealth() < 0.05f && m_death_effector == NULL && IsGameTypeSingle())
    {
        if (pSettings->section_exist("actor_death_effector"))
            m_death_effector = new CActorDeathEffector(this, "actor_death_effector");
    }
    if (m_death_effector && m_death_effector->IsActual())
    {
        m_death_effector->UpdateCL();

        if (!m_death_effector->IsActual())
            m_death_effector->Stop();
    }

    AffectDamage_InjuriousMaterialAndMonstersInfluence();
}

void CActorCondition::UpdateBoosters()
{
    for (u8 i = 0; i < eBoostMaxCount; i++)
    {
        BOOSTER_MAP::iterator it = m_booster_influences.find((EBoostParams)i);
        if (it != m_booster_influences.end())
        {
            it->second.fBoostTime -= m_fDeltaTime / (IsGameTypeSingle() ? Level().GetGameTimeFactor() : 1.0f);
            if (it->second.fBoostTime <= 0.0f)
            {
                DisableBoostParameters(it->second);
                m_booster_influences.erase(it);
            }
        }
    }

    if (m_object == Level().CurrentViewEntity())
        CurrentGameUI()->UIMainIngameWnd->UpdateBoosterIndicators(m_booster_influences);
}

void CActorCondition::AffectDamage_InjuriousMaterialAndMonstersInfluence()
{
    float one = 0.1f;
    float tg = Device.fTimeGlobal;
    if (m_f_time_affected + one > tg)
    {
        return;
    }

    clamp(m_f_time_affected, tg - (one * 3), tg);

    float psy_influence = 0;
    float fire_influence = 0;
    float radiation_influence = GetInjuriousMaterialDamage(); // Get Radiation from Material

    // Add Radiation and Psy Level from Monsters
    CPda* const pda = m_object->GetPDA();

    if (pda)
    {
        typedef xr_vector<IGameObject*> monsters;

        for (monsters::const_iterator it = pda->feel_touch.begin(); it != pda->feel_touch.end(); ++it)
        {
            CBaseMonster* const monster = smart_cast<CBaseMonster*>(*it);
            if (!monster || !monster->g_Alive())
                continue;

            psy_influence += monster->get_psy_influence();
            radiation_influence += monster->get_radiation_influence();
            fire_influence += monster->get_fire_influence();
        }
    }

    struct
    {
        ALife::EHitType type;
        float value;

    } hits[] = {{ALife::eHitTypeRadiation, radiation_influence * one}, {ALife::eHitTypeTelepatic, psy_influence * one},
        {ALife::eHitTypeBurn, fire_influence * one}};

    NET_Packet np;

    while (m_f_time_affected + one < tg)
    {
        m_f_time_affected += one;

        for (int i = 0; i < sizeof(hits) / sizeof(hits[0]); ++i)
        {
            float damage = hits[i].value;
            ALife::EHitType type = hits[i].type;

            if (damage > EPS)
            {
                SHit HDS = SHit(damage,
                    //.								0.0f,
                    Fvector().set(0, 1, 0), NULL, BI_NONE, Fvector().set(0, 0, 0), 0.0f, type, 0.0f, false);

                HDS.GenHeader(GE_HIT, m_object->ID());
                HDS.Write_Packet(np);
                CGameObject::u_EventSend(np);
            }

        } // for

    } // while
}

#include "CharacterPhysicsSupport.h"
float CActorCondition::GetInjuriousMaterialDamage()
{
    u16 mat_injurios = m_object->character_physics_support()->movement()->injurious_material_idx();

    if (mat_injurios != GAMEMTL_NONE_IDX)
    {
        const SGameMtl* mtl = GMLib.GetMaterialByIdx(mat_injurios);
        return mtl->fInjuriousSpeed;
    }
    else
        return 0.0f;
}

void CActorCondition::SetZoneDanger(float danger, ALife::EInfluenceType type)
{
    VERIFY(type != ALife::infl_max_count);
    m_zone_danger[type] = danger;
    clamp(m_zone_danger[type], 0.0f, 1.0f);
}

float CActorCondition::GetZoneDanger() const
{
    float sum = 0.0f;
    for (u8 i = 1; i < ALife::infl_max_count; ++i)
    {
        sum += m_zone_danger[i];
    }

    clamp(sum, 0.0f, 1.5f);
    return sum;
}

void CActorCondition::UpdateRadiation() { inherited::UpdateRadiation(); }
void CActorCondition::UpdateSatiety()
{
    if (!IsGameTypeSingle())
    {
        m_fDeltaPower += m_fV_SatietyPower * m_fDeltaTime;
        return;
    }

    if (m_fSatiety > 0)
    {
        m_fSatiety -= m_fV_Satiety * m_fDeltaTime;
        clamp(m_fSatiety, 0.0f, 1.0f);
    }

    float satiety_health_koef = (m_fSatiety - m_fSatietyCritical) /
        (m_fSatiety >= m_fSatietyCritical ? 1 - m_fSatietyCritical : m_fSatietyCritical);
    if (CanBeHarmed() && !psActorFlags.test(AF_GODMODE_RT))
    {
        m_fDeltaHealth += m_fV_SatietyHealth * satiety_health_koef * m_fDeltaTime;
        m_fDeltaPower += m_fV_SatietyPower * m_fSatiety * m_fDeltaTime;
    }
}

CWound* CActorCondition::ConditionHit(SHit* pHDS)
{
    if (GodMode())
        return NULL;
    return inherited::ConditionHit(pHDS);
}

void CActorCondition::PowerHit(float power, bool apply_outfit)
{
    m_fPower -= apply_outfit ? HitPowerEffect(power) : power;
    clamp(m_fPower, 0.f, 1.f);
}
// weight - "удельный" вес от 0..1
void CActorCondition::ConditionJump(float weight)
{
    float power = m_fJumpPower;
    power += m_fJumpWeightPower * weight * (weight > 1.f ? m_fOverweightJumpK : 1.f);
    m_fPower -= HitPowerEffect(power);
}

void CActorCondition::ConditionWalk(float weight, bool accel, bool sprint)
{
    float power = m_fWalkPower;
    power += m_fWalkWeightPower * weight * (weight > 1.f ? m_fOverweightWalkK : 1.f);
    power *= m_fDeltaTime * (accel ? (sprint ? m_fSprintK : m_fAccelK) : 1.f);
    m_fPower -= HitPowerEffect(power);
}

void CActorCondition::ConditionStand(float weight)
{
    float power = m_fStandPower;
    power *= m_fDeltaTime;
    m_fPower -= power;
}

bool CActorCondition::IsCantWalk() const
{
    if (m_fPower < m_fCantWalkPowerBegin)
        m_bCantWalk = true;
    else if (m_fPower > m_fCantWalkPowerEnd)
        m_bCantWalk = false;
    return m_bCantWalk;
}

bool CActorCondition::IsCantWalkWeight()
{
    if (IsGameTypeSingle() && !GodMode())
    {
        float max_w = m_object->MaxWalkWeight();

        if (object().inventory().TotalWeight() > max_w)
        {
            m_condition_flags.set(eCantWalkWeight, TRUE);
            return true;
        }
    }
    m_condition_flags.set(eCantWalkWeight, FALSE);
    return false;
}

bool CActorCondition::IsCantSprint() const
{
    if (m_fPower < m_fCantSprintPowerBegin)
        m_bCantSprint = true;
    else if (m_fPower > m_fCantSprintPowerEnd)
        m_bCantSprint = false;
    return m_bCantSprint;
}

bool CActorCondition::IsLimping() const
{
    if (m_fPower < m_fLimpingPowerBegin || GetHealth() < m_fLimpingHealthBegin)
        m_bLimping = true;
    else if (m_fPower > m_fLimpingPowerEnd && GetHealth() > m_fLimpingHealthEnd)
        m_bLimping = false;
    return m_bLimping;
}
extern bool g_bShowHudInfo;

void CActorCondition::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    save_data(m_fAlcohol, output_packet);
    save_data(m_condition_flags, output_packet);
    save_data(m_fSatiety, output_packet);

    save_data(m_curr_medicine_influence.fHealth, output_packet);
    save_data(m_curr_medicine_influence.fPower, output_packet);
    save_data(m_curr_medicine_influence.fSatiety, output_packet);
    save_data(m_curr_medicine_influence.fRadiation, output_packet);
    save_data(m_curr_medicine_influence.fWoundsHeal, output_packet);
    save_data(m_curr_medicine_influence.fMaxPowerUp, output_packet);
    save_data(m_curr_medicine_influence.fAlcohol, output_packet);
    save_data(m_curr_medicine_influence.fTimeTotal, output_packet);
    save_data(m_curr_medicine_influence.fTimeCurrent, output_packet);

    output_packet.w_u8((u8)m_booster_influences.size());
    BOOSTER_MAP::iterator b = m_booster_influences.begin(), e = m_booster_influences.end();
    for (; b != e; b++)
    {
        output_packet.w_u8((u8)b->second.m_type);
        output_packet.w_float(b->second.fBoostValue);
        output_packet.w_float(b->second.fBoostTime);
    }
}

void CActorCondition::load(IReader& input_packet)
{
    inherited::load(input_packet);
    load_data(m_fAlcohol, input_packet);
    load_data(m_condition_flags, input_packet);
    load_data(m_fSatiety, input_packet);

    load_data(m_curr_medicine_influence.fHealth, input_packet);
    load_data(m_curr_medicine_influence.fPower, input_packet);
    load_data(m_curr_medicine_influence.fSatiety, input_packet);
    load_data(m_curr_medicine_influence.fRadiation, input_packet);
    load_data(m_curr_medicine_influence.fWoundsHeal, input_packet);
    load_data(m_curr_medicine_influence.fMaxPowerUp, input_packet);
    load_data(m_curr_medicine_influence.fAlcohol, input_packet);
    load_data(m_curr_medicine_influence.fTimeTotal, input_packet);
    load_data(m_curr_medicine_influence.fTimeCurrent, input_packet);

    u8 cntr = input_packet.r_u8();
    for (; cntr > 0; cntr--)
    {
        SBooster B;
        B.m_type = (EBoostParams)input_packet.r_u8();
        B.fBoostValue = input_packet.r_float();
        B.fBoostTime = input_packet.r_float();
        m_booster_influences[B.m_type] = B;
        BoostParameters(B);
    }
}

void CActorCondition::reinit()
{
    inherited::reinit();
    m_bLimping = false;
    m_fSatiety = 1.f;
}

void CActorCondition::ChangeAlcohol(float value) { m_fAlcohol += value; }
void CActorCondition::ChangeSatiety(float value)
{
    m_fSatiety += value;
    clamp(m_fSatiety, 0.0f, 1.0f);
}

void CActorCondition::BoostParameters(const SBooster& B)
{
    if (OnServer())
    {
        switch (B.m_type)
        {
        case eBoostHpRestore: BoostHpRestore(B.fBoostValue); break;
        case eBoostPowerRestore: BoostPowerRestore(B.fBoostValue); break;
        case eBoostRadiationRestore: BoostRadiationRestore(B.fBoostValue); break;
        case eBoostBleedingRestore: BoostBleedingRestore(B.fBoostValue); break;
        case eBoostMaxWeight: BoostMaxWeight(B.fBoostValue); break;
        case eBoostBurnImmunity: BoostBurnImmunity(B.fBoostValue); break;
        case eBoostShockImmunity: BoostShockImmunity(B.fBoostValue); break;
        case eBoostRadiationImmunity: BoostRadiationImmunity(B.fBoostValue); break;
        case eBoostTelepaticImmunity: BoostTelepaticImmunity(B.fBoostValue); break;
        case eBoostChemicalBurnImmunity: BoostChemicalBurnImmunity(B.fBoostValue); break;
        case eBoostExplImmunity: BoostExplImmunity(B.fBoostValue); break;
        case eBoostStrikeImmunity: BoostStrikeImmunity(B.fBoostValue); break;
        case eBoostFireWoundImmunity: BoostFireWoundImmunity(B.fBoostValue); break;
        case eBoostWoundImmunity: BoostWoundImmunity(B.fBoostValue); break;
        case eBoostRadiationProtection: BoostRadiationProtection(B.fBoostValue); break;
        case eBoostTelepaticProtection: BoostTelepaticProtection(B.fBoostValue); break;
        case eBoostChemicalBurnProtection: BoostChemicalBurnProtection(B.fBoostValue); break;
        default: NODEFAULT;
        }
    }
}
void CActorCondition::DisableBoostParameters(const SBooster& B)
{
    if (!OnServer())
        return;

    switch (B.m_type)
    {
    case eBoostHpRestore: BoostHpRestore(-B.fBoostValue); break;
    case eBoostPowerRestore: BoostPowerRestore(-B.fBoostValue); break;
    case eBoostRadiationRestore: BoostRadiationRestore(-B.fBoostValue); break;
    case eBoostBleedingRestore: BoostBleedingRestore(-B.fBoostValue); break;
    case eBoostMaxWeight: BoostMaxWeight(-B.fBoostValue); break;
    case eBoostBurnImmunity: BoostBurnImmunity(-B.fBoostValue); break;
    case eBoostShockImmunity: BoostShockImmunity(-B.fBoostValue); break;
    case eBoostRadiationImmunity: BoostRadiationImmunity(-B.fBoostValue); break;
    case eBoostTelepaticImmunity: BoostTelepaticImmunity(-B.fBoostValue); break;
    case eBoostChemicalBurnImmunity: BoostChemicalBurnImmunity(-B.fBoostValue); break;
    case eBoostExplImmunity: BoostExplImmunity(-B.fBoostValue); break;
    case eBoostStrikeImmunity: BoostStrikeImmunity(-B.fBoostValue); break;
    case eBoostFireWoundImmunity: BoostFireWoundImmunity(-B.fBoostValue); break;
    case eBoostWoundImmunity: BoostWoundImmunity(-B.fBoostValue); break;
    case eBoostRadiationProtection: BoostRadiationProtection(-B.fBoostValue); break;
    case eBoostTelepaticProtection: BoostTelepaticProtection(-B.fBoostValue); break;
    case eBoostChemicalBurnProtection: BoostChemicalBurnProtection(-B.fBoostValue); break;
    default: NODEFAULT;
    }
}
void CActorCondition::BoostHpRestore(const float value) { m_change_v.m_fV_HealthRestore += value; }
void CActorCondition::BoostPowerRestore(const float value) { m_fV_SatietyPower += value; }
void CActorCondition::BoostRadiationRestore(const float value) { m_change_v.m_fV_Radiation += value; }
void CActorCondition::BoostBleedingRestore(const float value) { m_change_v.m_fV_WoundIncarnation += value; }
void CActorCondition::BoostMaxWeight(const float value)
{
    m_object->inventory().SetMaxWeight(object().inventory().GetMaxWeight() + value);
    m_MaxWalkWeight += value;
}
void CActorCondition::BoostBurnImmunity(const float value) { m_fBoostBurnImmunity += value; }
void CActorCondition::BoostShockImmunity(const float value) { m_fBoostShockImmunity += value; }
void CActorCondition::BoostRadiationImmunity(const float value) { m_fBoostRadiationImmunity += value; }
void CActorCondition::BoostTelepaticImmunity(const float value) { m_fBoostTelepaticImmunity += value; }
void CActorCondition::BoostChemicalBurnImmunity(const float value) { m_fBoostChemicalBurnImmunity += value; }
void CActorCondition::BoostExplImmunity(const float value) { m_fBoostExplImmunity += value; }
void CActorCondition::BoostStrikeImmunity(const float value) { m_fBoostStrikeImmunity += value; }
void CActorCondition::BoostFireWoundImmunity(const float value) { m_fBoostFireWoundImmunity += value; }
void CActorCondition::BoostWoundImmunity(const float value) { m_fBoostWoundImmunity += value; }
void CActorCondition::BoostRadiationProtection(const float value) { m_fBoostRadiationProtection += value; }
void CActorCondition::BoostTelepaticProtection(const float value) { m_fBoostTelepaticProtection += value; }
void CActorCondition::BoostChemicalBurnProtection(const float value) { m_fBoostChemicalBurnProtection += value; }
void CActorCondition::UpdateTutorialThresholds()
{
    string256 cb_name;
    static float _cPowerThr = pSettings->r_float("tutorial_conditions_thresholds", "power");
    static float _cPowerMaxThr = pSettings->r_float("tutorial_conditions_thresholds", "max_power");
    static float _cBleeding = pSettings->r_float("tutorial_conditions_thresholds", "bleeding");
    static float _cSatiety = pSettings->r_float("tutorial_conditions_thresholds", "satiety");
    static float _cRadiation = pSettings->r_float("tutorial_conditions_thresholds", "radiation");
    static float _cWpnCondition = pSettings->r_float("tutorial_conditions_thresholds", "weapon_jammed");
    static float _cPsyHealthThr = pSettings->r_float("tutorial_conditions_thresholds", "psy_health");

    bool b = true;
    if (b && !m_condition_flags.test(eCriticalPowerReached) && GetPower() < _cPowerThr)
    {
        m_condition_flags.set(eCriticalPowerReached, TRUE);
        b = false;
        xr_strcpy(cb_name, "_G.on_actor_critical_power");
    }

    if (b && !m_condition_flags.test(eCriticalMaxPowerReached) && GetMaxPower() < _cPowerMaxThr)
    {
        m_condition_flags.set(eCriticalMaxPowerReached, TRUE);
        b = false;
        xr_strcpy(cb_name, "_G.on_actor_critical_max_power");
    }

    if (b && !m_condition_flags.test(eCriticalBleedingSpeed) && BleedingSpeed() > _cBleeding)
    {
        m_condition_flags.set(eCriticalBleedingSpeed, TRUE);
        b = false;
        xr_strcpy(cb_name, "_G.on_actor_bleeding");
    }

    if (b && !m_condition_flags.test(eCriticalSatietyReached) && GetSatiety() < _cSatiety)
    {
        m_condition_flags.set(eCriticalSatietyReached, TRUE);
        b = false;
        xr_strcpy(cb_name, "_G.on_actor_satiety");
    }

    if (b && !m_condition_flags.test(eCriticalRadiationReached) && GetRadiation() > _cRadiation)
    {
        m_condition_flags.set(eCriticalRadiationReached, TRUE);
        b = false;
        xr_strcpy(cb_name, "_G.on_actor_radiation");
    }

    if (b && !m_condition_flags.test(ePhyHealthMinReached) && GetPsyHealth() < _cPsyHealthThr)
    {
        m_condition_flags.set(ePhyHealthMinReached, TRUE);
        b = false;
        xr_strcpy(cb_name, "_G.on_actor_psy");
    }

    if (b && m_condition_flags.test(eCantWalkWeight) && !m_condition_flags.test(eCantWalkWeightReached))
    {
        m_condition_flags.set(eCantWalkWeightReached, TRUE);
        b = false;
        xr_strcpy(cb_name, "_G.on_actor_cant_walk_weight");
    }

    if (b && !m_condition_flags.test(eWeaponJammedReached) && m_object->inventory().GetActiveSlot() != NO_ACTIVE_SLOT)
    {
        PIItem item = m_object->inventory().ItemFromSlot(m_object->inventory().GetActiveSlot());
        CWeapon* pWeapon = smart_cast<CWeapon*>(item);
        if (pWeapon && pWeapon->GetCondition() < _cWpnCondition)
        {
            m_condition_flags.set(eWeaponJammedReached, TRUE);
            b = false;
            xr_strcpy(cb_name, "_G.on_actor_weapon_jammed");
        }
    }

    if (!b)
    {
        luabind::functor<void> fl;
        R_ASSERT(GEnv.ScriptEngine->functor<void>(cb_name, fl));
        fl();
    }
}

bool CActorCondition::DisableSprint(SHit* pHDS)
{
    return (pHDS->hit_type != ALife::eHitTypeTelepatic) && (pHDS->hit_type != ALife::eHitTypeChemicalBurn) &&
        (pHDS->hit_type != ALife::eHitTypeBurn) && (pHDS->hit_type != ALife::eHitTypeLightBurn) &&
        (pHDS->hit_type != ALife::eHitTypeRadiation);
}

bool CActorCondition::PlayHitSound(SHit* pHDS)
{
    switch (pHDS->hit_type)
    {
    case ALife::eHitTypeTelepatic: return false; break;
    case ALife::eHitTypeShock:
    case ALife::eHitTypeStrike:
    case ALife::eHitTypeWound:
    case ALife::eHitTypeExplosion:
    case ALife::eHitTypeFireWound:
    case ALife::eHitTypeWound_2:
        //		case ALife::eHitTypePhysicStrike:
        return true;
        break;

    case ALife::eHitTypeRadiation:
    case ALife::eHitTypeBurn:
    case ALife::eHitTypeLightBurn:
    case ALife::eHitTypeChemicalBurn:
        return (pHDS->damage() > 0.017f); // field zone threshold
        break;
    default: return true;
    }
}

float CActorCondition::HitSlowmo(SHit* pHDS)
{
    float ret;
    if (pHDS->hit_type == ALife::eHitTypeWound || pHDS->hit_type == ALife::eHitTypeStrike)
    {
        ret = pHDS->damage();
        clamp(ret, 0.0f, 1.f);
    }
    else
        ret = 0.0f;

    return ret;
}

bool CActorCondition::ApplyInfluence(const SMedicineInfluenceValues& V, const shared_str& sect)
{
    if (m_curr_medicine_influence.InProcess())
        return false;

    if (m_object->Local() && m_object == Level().CurrentViewEntity())
    {
        if (pSettings->line_exist(sect, "use_sound"))
        {
            if (m_use_sound._feedback())
                m_use_sound.stop();

            shared_str snd_name = pSettings->r_string(sect, "use_sound");
            m_use_sound.create(snd_name.c_str(), st_Effect, sg_SourceType);
            m_use_sound.play(NULL, sm_2D);
        }
    }

    if (V.fTimeTotal < 0.0f)
        return inherited::ApplyInfluence(V, sect);

    m_curr_medicine_influence = V;
    m_curr_medicine_influence.fTimeCurrent = m_curr_medicine_influence.fTimeTotal;
    return true;
}
bool CActorCondition::ApplyBooster(const SBooster& B, const shared_str& sect)
{
    if (B.fBoostValue > 0.0f)
    {
        if (m_object->Local() && m_object == Level().CurrentViewEntity())
        {
            if (pSettings->line_exist(sect, "use_sound"))
            {
                if (m_use_sound._feedback())
                    m_use_sound.stop();

                shared_str snd_name = pSettings->r_string(sect, "use_sound");
                m_use_sound.create(snd_name.c_str(), st_Effect, sg_SourceType);
                m_use_sound.play(NULL, sm_2D);
            }
        }

        BOOSTER_MAP::iterator it = m_booster_influences.find(B.m_type);
        if (it != m_booster_influences.end())
            DisableBoostParameters((*it).second);

        m_booster_influences[B.m_type] = B;
        BoostParameters(B);
    }
    return true;
}

void disable_input();
void enable_input();
void hide_indicators();
void show_indicators();

CActorDeathEffector::CActorDeathEffector(CActorCondition* parent, LPCSTR sect) // -((
    : m_pParent(parent)
{
    Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, true);
    hide_indicators();

    AddEffector(Actor(), effActorDeath, sect);
    disable_input();
    LPCSTR snd = pSettings->r_string(sect, "snd");
    m_death_sound.create(snd, st_Effect, 0);
    m_death_sound.play_at_pos(0, Fvector().set(0, 0, 0), sm_2D);

    SBaseEffector* pe = Actor()->Cameras().GetPPEffector((EEffectorPPType)effActorDeath);
    pe->m_on_b_remove_callback = SBaseEffector::CB_ON_B_REMOVE(this, &CActorDeathEffector::OnPPEffectorReleased);
    m_b_actual = true;
    m_start_health = m_pParent->health();
}

CActorDeathEffector::~CActorDeathEffector() {}
void CActorDeathEffector::UpdateCL() { m_pParent->SetHealth(m_start_health); }
void CActorDeathEffector::OnPPEffectorReleased()
{
    m_b_actual = false;
    Msg("111");
    // m_pParent->health()		= -1.0f;
    m_pParent->SetHealth(-1.0f);
}

void CActorDeathEffector::Stop()
{
    RemoveEffector(Actor(), effActorDeath);
    m_death_sound.destroy();
    enable_input();
    show_indicators();
}
