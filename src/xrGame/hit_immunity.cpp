// hit_immunity.cpp:	класс для тех объектов, которые поддерживают
//						коэффициенты иммунитета для разных типов хитов
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "hit_immunity.h"

CHitImmunity::CHitImmunity()
{
    m_HitImmunityKoefs.resize(ALife::eHitTypeMax);
    for (u32 i = 0; i < ALife::eHitTypeMax; i++)
        m_HitImmunityKoefs[i] = 1.0f;
}

constexpr std::tuple<ALife::EHitType, cpcstr, bool> hit_immunities[] =
{
    // { hit type,                 "immunity",                     optional  }
    { ALife::eHitTypeBurn,         "burn_immunity",                false },
    { ALife::eHitTypeStrike,       "strike_immunity",              false },
    { ALife::eHitTypeShock,        "shock_immunity",               false },
    { ALife::eHitTypeWound,        "wound_immunity",               false },
    { ALife::eHitTypeRadiation,    "radiation_immunity",           false },
    { ALife::eHitTypeTelepatic,    "telepatic_immunity",           false },
    { ALife::eHitTypeChemicalBurn, "chemical_burn_immunity",       false },
    { ALife::eHitTypeExplosion,    "explosion_immunity",           false },
    { ALife::eHitTypeFireWound,    "fire_wound_immunity",          false },
    { ALife::eHitTypeLightBurn,    "burn_immunity",                false },
    { ALife::eHitTypePhysicStrike, "physic_strike_wound_immunity", true  },
};

void CHitImmunity::LoadImmunities(const char* imm_sect, const CInifile* ini, bool invert /*= false*/)
{
    R_ASSERT2(ini->section_exist(imm_sect), imm_sect);

    for (const auto& [hit_type, immunity_name, optional] : hit_immunities)
    {
        if (optional && !ini->line_exist(imm_sect, immunity_name))
            continue;

        float immunity = ini->r_float(imm_sect, immunity_name);
        if (invert)
            immunity = 1.0f - immunity;

        m_HitImmunityKoefs[hit_type] = immunity;
    }
}

void CHitImmunity::AddImmunities(const char* imm_sect, const CInifile* ini, bool invert /*= false*/)
{
    R_ASSERT2(ini->section_exist(imm_sect), imm_sect);

    for (const auto& [hit_type, immunity_name, _] : hit_immunities)
    {
        if (!ini->line_exist(imm_sect, immunity_name))
            continue;
        float immunity = ini->r_float(imm_sect, immunity_name);
        if (invert)
            immunity = 1.0f - immunity;
        m_HitImmunityKoefs[hit_type] += immunity;
    }
}
