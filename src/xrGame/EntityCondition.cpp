#include "StdAfx.h"
#include "EntityCondition.h"
#include "InventoryOwner.h"
#include "CustomOutfit.h"
#include "Inventory.h"
#include "Wound.h"
#include "Level.h"
#include "game_cl_base.h"
#include "entity_alive.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "Include/xrRender/Kinematics.h"
#include "Common/object_broker.h"
#include "ActorHelmet.h"

#define MAX_HEALTH 1.0f
#define MIN_HEALTH -0.01f

#define MAX_POWER 1.0f
#define MAX_RADIATION 1.0f
#define MAX_PSY_HEALTH 1.0f

CEntityConditionSimple::CEntityConditionSimple()
{
    max_health() = MAX_HEALTH;
    SetHealth(MAX_HEALTH);
}

CEntityConditionSimple::~CEntityConditionSimple() {}
CEntityCondition::CEntityCondition(CEntityAlive* object) : CEntityConditionSimple()
{
    VERIFY(object);

    m_object = object;

    m_use_limping_state = false;
    m_iLastTimeCalled = 0;
    m_bTimeValid = false;

    m_fPowerMax = MAX_POWER;
    m_fRadiationMax = MAX_RADIATION;
    m_fPsyHealthMax = MAX_PSY_HEALTH;
    m_fEntityMorale = m_fEntityMoraleMax = 1.f;

    m_fPower = MAX_POWER;
    m_fRadiation = 0;
    m_fPsyHealth = MAX_PSY_HEALTH;

    m_fMinWoundSize = 0.00001f;

    m_fHealthHitPart = 1.0f;
    m_fPowerHitPart = 0.5f;

    m_fBoostBurnImmunity = 0.f;
    m_fBoostShockImmunity = 0.f;
    m_fBoostRadiationImmunity = 0.f;
    m_fBoostTelepaticImmunity = 0.f;
    m_fBoostChemicalBurnImmunity = 0.f;
    m_fBoostExplImmunity = 0.f;
    m_fBoostStrikeImmunity = 0.f;
    m_fBoostFireWoundImmunity = 0.f;
    m_fBoostWoundImmunity = 0.f;
    m_fBoostRadiationProtection = 0.f;
    m_fBoostTelepaticProtection = 0.f;
    m_fBoostChemicalBurnProtection = 0.f;

    m_fDeltaHealth = 0;
    m_fDeltaPower = 0;
    m_fDeltaRadiation = 0;
    m_fDeltaPsyHealth = 0;

    m_fHealthLost = 0.f;
    m_pWho = NULL;
    m_iWhoID = 0;

    m_WoundVector.clear();

    m_fKillHitTreshold = 0;
    m_fLastChanceHealth = 0;
    m_fInvulnerableTime = 0;
    m_fInvulnerableTimeDelta = 0;

    m_fHitBoneScale = 1.f;
    m_fWoundBoneScale = 1.f;

    m_bIsBleeding = false;
    m_bCanBeHarmed = true;
}

CEntityCondition::~CEntityCondition(void) { ClearWounds(); }
void CEntityCondition::ClearWounds()
{
    for (auto it = m_WoundVector.begin(); m_WoundVector.end() != it; ++it)
        xr_delete(*it);
    m_WoundVector.clear();

    m_bIsBleeding = false;
}

void CEntityCondition::LoadCondition(LPCSTR entity_section)
{
    LPCSTR section = READ_IF_EXISTS(pSettings, r_string, entity_section, "condition_sect", entity_section);

    m_change_v.load(section, "");

    m_fMinWoundSize = pSettings->r_float(section, "min_wound_size");
    m_fHealthHitPart = pSettings->r_float(section, "health_hit_part");
    m_fPowerHitPart = pSettings->r_float(section, "power_hit_part");

    m_use_limping_state = !!(READ_IF_EXISTS(pSettings, r_bool, section, "use_limping_state", FALSE));
    m_limping_threshold = READ_IF_EXISTS(pSettings, r_float, section, "limping_threshold", .5f);

    m_fKillHitTreshold = READ_IF_EXISTS(pSettings, r_float, section, "killing_hit_treshold", 0.0f);
    m_fLastChanceHealth = READ_IF_EXISTS(pSettings, r_float, section, "last_chance_health", 0.0f);
    m_fInvulnerableTimeDelta = READ_IF_EXISTS(pSettings, r_float, section, "invulnerable_time", 0.0f) / 1000.f;
}

void CEntityCondition::LoadTwoHitsDeathParams(LPCSTR section)
{
    m_fKillHitTreshold = READ_IF_EXISTS(pSettings, r_float, section, "killing_hit_treshold", 0.0f);
    m_fLastChanceHealth = READ_IF_EXISTS(pSettings, r_float, section, "last_chance_health", 0.0f);
    m_fInvulnerableTimeDelta = READ_IF_EXISTS(pSettings, r_float, section, "invulnerable_time", 0.0f) / 1000.f;
}

void CEntityCondition::reinit()
{
    m_iLastTimeCalled = 0;
    m_bTimeValid = false;

    max_health() = MAX_HEALTH;
    m_fPowerMax = MAX_POWER;
    m_fRadiationMax = MAX_RADIATION;
    m_fPsyHealthMax = MAX_PSY_HEALTH;

    m_fEntityMorale = m_fEntityMoraleMax = 1.f;

    SetHealth(MAX_HEALTH);
    m_fPower = MAX_POWER;
    m_fRadiation = 0;
    m_fPsyHealth = MAX_PSY_HEALTH;

    m_fDeltaHealth = 0;
    m_fDeltaPower = 0;
    m_fDeltaRadiation = 0;

    m_fDeltaCircumspection = 0;
    m_fDeltaEntityMorale = 0;
    m_fDeltaPsyHealth = 0;

    m_fHealthLost = 0.f;
    m_pWho = NULL;
    m_iWhoID = 0;

    ClearWounds();
}

void CEntityCondition::ChangeHealth(const float value)
{
    VERIFY(_valid(value));
    m_fDeltaHealth += (CanBeHarmed() || (value > 0)) ? value : 0;
}

void CEntityCondition::ChangePower(const float value) { m_fDeltaPower += value; }
void CEntityCondition::ChangeRadiation(const float value) { m_fDeltaRadiation += value; }
void CEntityCondition::ChangePsyHealth(const float value) { m_fDeltaPsyHealth += value; }
void CEntityCondition::ChangeCircumspection(const float value) { m_fDeltaCircumspection += value; }
void CEntityCondition::ChangeEntityMorale(const float value) { m_fDeltaEntityMorale += value; }
void CEntityCondition::ChangeBleeding(const float percent)
{
    //затянуть раны
    for (auto it = m_WoundVector.begin(); m_WoundVector.end() != it; ++it)
    {
        (*it)->Incarnation(percent, m_fMinWoundSize);
        if (0 == (*it)->TotalSize())
            (*it)->SetDestroy(true);
    }
}

bool RemoveWoundPred(CWound* pWound)
{
    if (pWound->GetDestroy())
    {
        xr_delete(pWound);
        return true;
    }
    return false;
}

void CEntityCondition::UpdateWounds()
{
    //убрать все зашившие раны из списка
    m_WoundVector.erase(
        std::remove_if(m_WoundVector.begin(), m_WoundVector.end(), &RemoveWoundPred), m_WoundVector.end());
}

void CEntityCondition::UpdateConditionTime()
{
    u64 _cur_time = (GameID() == eGameIDSingle) ? Level().GetGameTime() : Level().timeServer();

    if (m_bTimeValid)
    {
        if (_cur_time > m_iLastTimeCalled)
        {
            float x = float(_cur_time - m_iLastTimeCalled) / 1000.0f;
            SetConditionDeltaTime(x);
        }
        else
            SetConditionDeltaTime(0.0f);
    }
    else
    {
        SetConditionDeltaTime(0.0f);
        m_bTimeValid = true;

        m_fDeltaHealth = 0;
        m_fDeltaPower = 0;
        m_fDeltaRadiation = 0;
        m_fDeltaCircumspection = 0;
        m_fDeltaEntityMorale = 0;
    }

    m_iLastTimeCalled = _cur_time;
}

//вычисление параметров с ходом игрового времени
void CEntityCondition::UpdateCondition()
{
    if (GetHealth() <= 0)
        return;
    //-----------------------------------------
    bool CriticalHealth = false;

    if (m_fDeltaHealth + GetHealth() <= 0)
    {
        CriticalHealth = true;
        m_object->OnCriticalHitHealthLoss();
    }
    else
    {
        if (m_fDeltaHealth < 0)
            m_object->OnHitHealthLoss(GetHealth() + m_fDeltaHealth);
    }
    //-----------------------------------------
    UpdateHealth();
    //-----------------------------------------
    if (!CriticalHealth && m_fDeltaHealth + GetHealth() <= 0)
    {
        CriticalHealth = true;
        m_object->OnCriticalWoundHealthLoss();
    };
    //-----------------------------------------
    UpdatePower();
    UpdateRadiation();
    //-----------------------------------------
    if (!CriticalHealth && m_fDeltaHealth + GetHealth() <= 0)
    {
        CriticalHealth = true;
        m_object->OnCriticalRadiationHealthLoss();
    };
    //-----------------------------------------
    UpdatePsyHealth();

    UpdateEntityMorale();

    if (Device.fTimeGlobal > m_fInvulnerableTime)
    {
        float curr_health = GetHealth();
        if (curr_health > m_fKillHitTreshold && curr_health + m_fDeltaHealth < 0)
        {
            SetHealth(m_fLastChanceHealth);
            m_fInvulnerableTime = Device.fTimeGlobal + m_fInvulnerableTimeDelta;
        }
        else
            SetHealth(curr_health + m_fDeltaHealth);
    }

    m_fPower += m_fDeltaPower;
    m_fPsyHealth += m_fDeltaPsyHealth;
    m_fEntityMorale += m_fDeltaEntityMorale;
    m_fRadiation += m_fDeltaRadiation;

    m_fDeltaHealth = 0;
    m_fDeltaPower = 0;
    m_fDeltaRadiation = 0;
    m_fDeltaPsyHealth = 0;
    m_fDeltaCircumspection = 0;
    m_fDeltaEntityMorale = 0;
    float l_health = GetHealth();
    clamp(l_health, MIN_HEALTH, max_health());
    SetHealth(l_health);
    clamp(m_fPower, 0.0f, m_fPowerMax);
    clamp(m_fRadiation, 0.0f, m_fRadiationMax);
    clamp(m_fEntityMorale, 0.0f, m_fEntityMoraleMax);
    clamp(m_fPsyHealth, 0.0f, m_fPsyHealthMax);
}

float CEntityCondition::HitOutfitEffect(
    float hit_power, ALife::EHitType hit_type, s16 element, float ap, bool& add_wound)
{
    CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(m_object);
    if (!pInvOwner)
        return hit_power;

    CCustomOutfit* pOutfit = (CCustomOutfit*)pInvOwner->inventory().ItemFromSlot(OUTFIT_SLOT);
    CHelmet* pHelmet = (CHelmet*)pInvOwner->inventory().ItemFromSlot(HELMET_SLOT);
    if (!pOutfit && !pHelmet)
        return hit_power;

    float new_hit_power = hit_power;
    if (pOutfit)
        new_hit_power = pOutfit->HitThroughArmor(hit_power, element, ap, add_wound, hit_type);

    if (pHelmet)
        new_hit_power = pHelmet->HitThroughArmor(new_hit_power, element, ap, add_wound, hit_type);

    if (bDebug)
        Msg("new_hit_power = %.3f  hit_type = %s  ap = %.3f", new_hit_power, ALife::g_cafHitType2String(hit_type), ap);

    return new_hit_power;
}

float CEntityCondition::HitPowerEffect(float power_loss)
{
    CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(m_object);
    if (!pInvOwner)
        return power_loss;

    CCustomOutfit* pOutfit = pInvOwner->GetOutfit();
    if (!pOutfit)
        return power_loss * 0.5f;

    float new_power_loss = power_loss * pOutfit->m_fPowerLoss;

    return new_power_loss;
}

CWound* CEntityCondition::AddWound(float hit_power, ALife::EHitType hit_type, u16 element)
{
    //максимальное число косточек 64
    VERIFY(element < 64 || BI_NONE == element);

    //запомнить кость по которой ударили и силу удара
    auto it = m_WoundVector.begin();
    for (; it != m_WoundVector.end(); it++)
    {
        if ((*it)->GetBoneNum() == element)
            break;
    }

    CWound* pWound = NULL;

    //новая рана
    if (it == m_WoundVector.end())
    {
        pWound = new CWound(element);
        pWound->AddHit(hit_power * ::Random.randF(0.5f, 1.5f), hit_type);
        m_WoundVector.push_back(pWound);
    }
    //старая
    else
    {
        pWound = *it;
        pWound->AddHit(hit_power * ::Random.randF(0.5f, 1.5f), hit_type);
    }

    VERIFY(pWound);
    return pWound;
}

CWound* CEntityCondition::ConditionHit(SHit* pHDS)
{
    //кто нанес последний хит
    m_pWho = pHDS->who;
    m_iWhoID = (NULL != pHDS->who) ? pHDS->who->ID() : 0;

    bool const is_special_hit_2_self = (pHDS->who == m_object) && (pHDS->boneID == BI_NONE);

    bool bAddWound = pHDS->add_wound;

    float hit_power_org = pHDS->damage();
    float hit_power = hit_power_org;
    hit_power = HitOutfitEffect(hit_power_org, pHDS->hit_type, pHDS->boneID, pHDS->armor_piercing, bAddWound);

    switch (pHDS->hit_type)
    {
    case ALife::eHitTypeTelepatic:
        hit_power -= m_fBoostTelepaticProtection;
        if (hit_power < 0.f)
            hit_power = 0.f;
        hit_power *= GetHitImmunity(pHDS->hit_type) - m_fBoostTelepaticImmunity;
        ChangePsyHealth(-hit_power);
        m_fHealthLost = hit_power * m_fHealthHitPart;
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        bAddWound = false;
        break;
    case ALife::eHitTypeLightBurn:
    case ALife::eHitTypeBurn:
        hit_power *= GetHitImmunity(ALife::eHitTypeBurn) - m_fBoostBurnImmunity;
        m_fHealthLost = hit_power * m_fHealthHitPart * m_fHitBoneScale;
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        //		bAddWound		=  is_special_hit_2_self;
        bAddWound = false;
        break;
    case ALife::eHitTypeChemicalBurn:
        hit_power -= m_fBoostChemicalBurnProtection;
        if (hit_power < 0.f)
            hit_power = 0.f;
        hit_power *= GetHitImmunity(pHDS->hit_type) - m_fBoostChemicalBurnImmunity;
        m_fHealthLost = hit_power * m_fHealthHitPart;
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        bAddWound = false;
        break;
    case ALife::eHitTypeShock:
        hit_power *= GetHitImmunity(pHDS->hit_type) - m_fBoostShockImmunity;
        m_fHealthLost = hit_power * m_fHealthHitPart;
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        bAddWound = false;
        break;
    case ALife::eHitTypeRadiation:
        hit_power -= m_fBoostRadiationProtection;
        if (hit_power < 0.f)
            hit_power = 0.f;
        hit_power *= GetHitImmunity(pHDS->hit_type) - m_fBoostRadiationImmunity;
        m_fDeltaRadiation += hit_power;
        bAddWound = false;
        return NULL;
        break;
    case ALife::eHitTypeExplosion:
        hit_power *= GetHitImmunity(pHDS->hit_type) - m_fBoostExplImmunity;
        m_fHealthLost = hit_power * m_fHealthHitPart;
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        break;
    case ALife::eHitTypeStrike:
        //	case ALife::eHitTypePhysicStrike:
        hit_power *= GetHitImmunity(pHDS->hit_type) - m_fBoostStrikeImmunity;
        m_fHealthLost = hit_power * m_fHealthHitPart;
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        bAddWound = false;
        break;
    case ALife::eHitTypeFireWound:
        hit_power *= GetHitImmunity(pHDS->hit_type) - m_fBoostFireWoundImmunity;
        m_fHealthLost = hit_power * m_fHealthHitPart * m_fHitBoneScale;
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        break;
    case ALife::eHitTypeWound:
        hit_power *= GetHitImmunity(pHDS->hit_type) - m_fBoostWoundImmunity;
        m_fHealthLost = hit_power * m_fHealthHitPart * m_fHitBoneScale;
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        break;
    default: { R_ASSERT2(0, "unknown hit type");
    }
    break;
    }

    if (bDebug && !is_special_hit_2_self)
    {
        Msg("%s hitted in %s with %f[%f]", m_object->Name(),
            smart_cast<IKinematics*>(m_object->Visual())->LL_BoneName_dbg(pHDS->boneID), m_fHealthLost * 100.0f,
            hit_power_org);
    }
    //раны добавляются только живому
    if (bAddWound && GetHealth() > 0)
    {
        return AddWound(hit_power * m_fWoundBoneScale, pHDS->hit_type, pHDS->boneID);
    }
    else
    {
        return NULL;
    }
}

float CEntityCondition::BleedingSpeed()
{
    float bleeding_speed = 0;

    for (auto it = m_WoundVector.begin(); m_WoundVector.end() != it; ++it)
        bleeding_speed += (*it)->TotalSize();

    return (m_WoundVector.empty() ? 0.f : bleeding_speed / m_WoundVector.size());
}

void CEntityCondition::UpdateHealth()
{
    float bleeding_speed = BleedingSpeed() * m_fDeltaTime * m_change_v.m_fV_Bleeding;
    m_bIsBleeding = fis_zero(bleeding_speed) ? false : true;
    m_fDeltaHealth -= CanBeHarmed() ? bleeding_speed : 0;
    m_fDeltaHealth += m_fDeltaTime * m_change_v.m_fV_HealthRestore;

    VERIFY(_valid(m_fDeltaHealth));
    ChangeBleeding(m_change_v.m_fV_WoundIncarnation * m_fDeltaTime);
}

void CEntityCondition::UpdatePower() {}
void CEntityCondition::UpdatePsyHealth() { m_fDeltaPsyHealth += m_change_v.m_fV_PsyHealth * m_fDeltaTime; }
void CEntityCondition::UpdateRadiation()
{
    if (m_fRadiation > 0)
    {
        m_fDeltaRadiation -= m_change_v.m_fV_Radiation * m_fDeltaTime;

        m_fDeltaHealth -= CanBeHarmed() ? m_change_v.m_fV_RadiationHealth * m_fRadiation * m_fDeltaTime : 0.0f;
    }
}

void CEntityCondition::UpdateEntityMorale()
{
    if (m_fEntityMorale < m_fEntityMoraleMax)
    {
        m_fDeltaEntityMorale += m_change_v.m_fV_EntityMorale * m_fDeltaTime;
    }
}

bool CEntityCondition::IsLimping() const
{
    if (!m_use_limping_state)
        return (false);
    return (m_fPower * GetHealth() <= m_limping_threshold);
}

void CEntityCondition::save(NET_Packet& output_packet)
{
    u8 is_alive = (GetHealth() > 0.f) ? 1 : 0;

    output_packet.w_u8(is_alive);
    if (is_alive)
    {
        save_data(m_fPower, output_packet);
        save_data(m_fRadiation, output_packet);
        save_data(m_fEntityMorale, output_packet);
        save_data(m_fPsyHealth, output_packet);

        output_packet.w_u8((u8)m_WoundVector.size());
        for (auto it = m_WoundVector.begin(); m_WoundVector.end() != it; it++)
            (*it)->save(output_packet);
    }
}

void CEntityCondition::load(IReader& input_packet)
{
    m_bTimeValid = false;

    u8 is_alive = input_packet.r_u8();
    if (is_alive)
    {
        load_data(m_fPower, input_packet);
        load_data(m_fRadiation, input_packet);
        load_data(m_fEntityMorale, input_packet);
        load_data(m_fPsyHealth, input_packet);

        ClearWounds();
        m_WoundVector.resize(input_packet.r_u8());
        if (!m_WoundVector.empty())
            for (u32 i = 0; i < m_WoundVector.size(); i++)
            {
                CWound* pWound = new CWound(BI_NONE);
                pWound->load(input_packet);
                m_WoundVector[i] = pWound;
            }
    }
}

void CEntityCondition::SConditionChangeV::load(LPCSTR sect, LPCSTR prefix)
{
    string256 str;
    m_fV_Circumspection = 0.01f;

    strconcat(sizeof(str), str, "radiation_v", prefix);
    m_fV_Radiation = pSettings->r_float(sect, str);
    strconcat(sizeof(str), str, "radiation_health_v", prefix);
    m_fV_RadiationHealth = pSettings->r_float(sect, str);
    strconcat(sizeof(str), str, "morale_v", prefix);
    m_fV_EntityMorale = pSettings->r_float(sect, str);
    strconcat(sizeof(str), str, "psy_health_v", prefix);
    m_fV_PsyHealth = pSettings->r_float(sect, str);
    strconcat(sizeof(str), str, "bleeding_v", prefix);
    m_fV_Bleeding = pSettings->r_float(sect, str);
    strconcat(sizeof(str), str, "wound_incarnation_v", prefix);
    m_fV_WoundIncarnation = pSettings->r_float(sect, str);
    strconcat(sizeof(str), str, "health_restore_v", prefix);
    m_fV_HealthRestore = READ_IF_EXISTS(pSettings, r_float, sect, str, 0.0f);
}

void CEntityCondition::remove_links(const IGameObject* object)
{
    if (m_pWho != object)
        return;

    m_pWho = m_object;
    m_iWhoID = m_object->ID();
}

bool CEntityCondition::ApplyInfluence(const SMedicineInfluenceValues& V, const shared_str& sect)
{
    ChangeHealth(V.fHealth);
    ChangePower(V.fPower);
    ChangeSatiety(V.fSatiety);
    ChangeRadiation(V.fRadiation);
    ChangeBleeding(V.fWoundsHeal);
    SetMaxPower(GetMaxPower() + V.fMaxPowerUp);
    ChangeAlcohol(V.fAlcohol);
    return true;
}

bool CEntityCondition::ApplyBooster(const SBooster& B, const shared_str& sect) { return true; }
void SMedicineInfluenceValues::Load(const shared_str& sect)
{
    fHealth = pSettings->r_float(sect.c_str(), "eat_health");
    fPower = pSettings->r_float(sect.c_str(), "eat_power");
    fSatiety = pSettings->r_float(sect.c_str(), "eat_satiety");
    fRadiation = pSettings->r_float(sect.c_str(), "eat_radiation");
    fWoundsHeal = pSettings->r_float(sect.c_str(), "wounds_heal_perc");
    clamp(fWoundsHeal, 0.f, 1.f);
    fMaxPowerUp = READ_IF_EXISTS(pSettings, r_float, sect.c_str(), "eat_max_power", 0.0f);
    fAlcohol = READ_IF_EXISTS(pSettings, r_float, sect.c_str(), "eat_alcohol", 0.0f);
    fTimeTotal = READ_IF_EXISTS(pSettings, r_float, sect.c_str(), "apply_time_sec", -1.0f);
}

void SBooster::Load(const shared_str& sect, EBoostParams type)
{
    fBoostTime = pSettings->r_float(sect.c_str(), "boost_time");
    m_type = type;
    switch (type)
    {
    case eBoostHpRestore: fBoostValue = pSettings->r_float(sect.c_str(), "boost_health_restore"); break;
    case eBoostPowerRestore: fBoostValue = pSettings->r_float(sect.c_str(), "boost_power_restore"); break;
    case eBoostRadiationRestore: fBoostValue = pSettings->r_float(sect.c_str(), "boost_radiation_restore"); break;
    case eBoostBleedingRestore: fBoostValue = pSettings->r_float(sect.c_str(), "boost_bleeding_restore"); break;
    case eBoostMaxWeight: fBoostValue = pSettings->r_float(sect.c_str(), "boost_max_weight"); break;
    case eBoostBurnImmunity: fBoostValue = pSettings->r_float(sect.c_str(), "boost_burn_immunity"); break;
    case eBoostShockImmunity: fBoostValue = pSettings->r_float(sect.c_str(), "boost_shock_immunity"); break;
    case eBoostRadiationImmunity: fBoostValue = pSettings->r_float(sect.c_str(), "boost_radiation_immunity"); break;
    case eBoostTelepaticImmunity: fBoostValue = pSettings->r_float(sect.c_str(), "boost_telepat_immunity"); break;
    case eBoostChemicalBurnImmunity: fBoostValue = pSettings->r_float(sect.c_str(), "boost_chemburn_immunity"); break;
    case eBoostExplImmunity: fBoostValue = pSettings->r_float(sect.c_str(), "boost_explosion_immunity"); break;
    case eBoostStrikeImmunity: fBoostValue = pSettings->r_float(sect.c_str(), "boost_strike_immunity"); break;
    case eBoostFireWoundImmunity: fBoostValue = pSettings->r_float(sect.c_str(), "boost_fire_wound_immunity"); break;
    case eBoostWoundImmunity: fBoostValue = pSettings->r_float(sect.c_str(), "boost_wound_immunity"); break;
    case eBoostRadiationProtection: fBoostValue = pSettings->r_float(sect.c_str(), "boost_radiation_protection"); break;
    case eBoostTelepaticProtection: fBoostValue = pSettings->r_float(sect.c_str(), "boost_telepat_protection"); break;
    case eBoostChemicalBurnProtection:
        fBoostValue = pSettings->r_float(sect.c_str(), "boost_chemburn_protection");
        break;
    default: NODEFAULT;
    }
}
