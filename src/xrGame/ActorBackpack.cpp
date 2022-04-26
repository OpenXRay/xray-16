#include "StdAfx.h"
#include "ActorBackpack.h"
#include "Actor.h"
#include "Inventory.h"

CBackpack::CBackpack()
{
    m_flags.set(FUsingCondition, FALSE);
}

CBackpack::~CBackpack() {}

void CBackpack::Load(LPCSTR section)
{
    inherited::Load(section);

    m_additional_weight = pSettings->r_float(section, "additional_inventory_weight");
    m_additional_weight2 = pSettings->r_float(section, "additional_inventory_weight2");
    m_fPowerRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "power_restore_speed", 0.0f);
    m_fPowerLoss = READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 1.0f);
    clamp(m_fPowerLoss, EPS, 1.0f);

    m_fJumpSpeed = READ_IF_EXISTS(pSettings, r_float, section, "jump_speed", 1.f);
    m_fWalkAccel = READ_IF_EXISTS(pSettings, r_float, section, "walk_accel", 1.f);
    m_fOverweightWalkK = READ_IF_EXISTS(pSettings, r_float, section, "overweight_walk_accel", 1.f);

    m_flags.set(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", TRUE));
}

bool CBackpack::net_Spawn(CSE_Abstract* DC)
{
    return inherited::net_Spawn(DC);
}

void CBackpack::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);
}

void CBackpack::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);
}

void CBackpack::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
}

void CBackpack::OnMoveToSlot(const SInvItemPlace& previous_place)
{
    inherited::OnMoveToSlot(previous_place);
}

void CBackpack::OnMoveToRuck(const SInvItemPlace& previous_place)
{
    inherited::OnMoveToRuck(previous_place);
}

void CBackpack::Hit(float hit_power, ALife::EHitType hit_type)
{
    if (IsUsingCondition() == false) return;
    hit_power *= GetHitImmunity(hit_type);
    ChangeCondition(-hit_power);
}

bool CBackpack::install_upgrade_impl(LPCSTR section, bool test)
{
    bool result = inherited::install_upgrade_impl(section, test);

    result |= process_if_exists(section, "power_restore_speed", &CInifile::r_float, m_fPowerRestoreSpeed, test);
    result |= process_if_exists(section, "power_loss", &CInifile::r_float, m_fPowerLoss, test);
    clamp(m_fPowerLoss, 0.0f, 1.0f);

    result |= process_if_exists(section, "additional_inventory_weight", &CInifile::r_float, m_additional_weight, test);
    result |= process_if_exists(section, "additional_inventory_weight2", &CInifile::r_float, m_additional_weight2, test);

    return result;
}
