#include "StdAfx.h"
#include "monster_morale.h"

void CMonsterMorale::init_external(CBaseMonster* obj) { m_object = obj; }
void CMonsterMorale::load(LPCSTR section)
{
    m_hit_quant = pSettings->r_float(section, "Morale_Hit_Quant");
    m_attack_success_quant = pSettings->r_float(section, "Morale_Attack_Success_Quant");
    m_v_taking_heart = pSettings->r_float(section, "Morale_Take_Heart_Speed");
    m_v_despondent = pSettings->r_float(section, "Morale_Despondent_Speed");
    m_v_stable = pSettings->r_float(section, "Morale_Stable_Speed");
    m_despondent_threshold = pSettings->r_float(section, "Morale_Despondent_Threashold");
}

void CMonsterMorale::reinit()
{
    m_state = eStable;
    m_morale = 1.0f;
}

void CMonsterMorale::on_hit() { change(-m_hit_quant); }
void CMonsterMorale::on_attack_success() { change(m_attack_success_quant); }
void CMonsterMorale::update_schedule(u32 dt)
{
    float cur_v = 1.f;

    switch (m_state)
    {
    case eStable: cur_v = m_v_stable; break;
    case eTakeHeart: cur_v = m_v_taking_heart; break;
    case eDespondent: cur_v = -m_v_despondent; break;
    }

    m_morale += cur_v * dt / 1000;

    clamp(m_morale, 0.f, 1.f);
}
