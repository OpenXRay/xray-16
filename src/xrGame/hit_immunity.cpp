// hit_immunity.cpp:	класс для тех объектов, которые поддерживают
//						коэффициенты иммунитета для разных типов хитов
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "hit_immunity.h"

CHitImmunity::CHitImmunity()
{
    m_HitImmunityKoefs.resize(ALife::eHitTypeMax);
    for (int i = 0; i < ALife::eHitTypeMax; i++)
        m_HitImmunityKoefs[i] = 1.0f;
}

CHitImmunity::~CHitImmunity() {}
void CHitImmunity::LoadImmunities(const char* imm_sect, const CInifile* ini)
{
    R_ASSERT2(ini->section_exist(imm_sect), imm_sect);

    m_HitImmunityKoefs[ALife::eHitTypeBurn] = ini->r_float(imm_sect, "burn_immunity");
    m_HitImmunityKoefs[ALife::eHitTypeStrike] = ini->r_float(imm_sect, "strike_immunity");
    m_HitImmunityKoefs[ALife::eHitTypeShock] = ini->r_float(imm_sect, "shock_immunity");
    m_HitImmunityKoefs[ALife::eHitTypeWound] = ini->r_float(imm_sect, "wound_immunity");
    m_HitImmunityKoefs[ALife::eHitTypeRadiation] = ini->r_float(imm_sect, "radiation_immunity");
    m_HitImmunityKoefs[ALife::eHitTypeTelepatic] = ini->r_float(imm_sect, "telepatic_immunity");
    m_HitImmunityKoefs[ALife::eHitTypeChemicalBurn] = ini->r_float(imm_sect, "chemical_burn_immunity");
    m_HitImmunityKoefs[ALife::eHitTypeExplosion] = ini->r_float(imm_sect, "explosion_immunity");
    m_HitImmunityKoefs[ALife::eHitTypeFireWound] = ini->r_float(imm_sect, "fire_wound_immunity");
    //	m_HitImmunityKoefs[ALife::eHitTypePhysicStrike]	= READ_IF_EXISTS(ini, r_float,
    // imm_sect,"physic_strike_wound_immunity", 1.0f);
    m_HitImmunityKoefs[ALife::eHitTypeLightBurn] = m_HitImmunityKoefs[ALife::eHitTypeBurn];
}

void CHitImmunity::AddImmunities(const char* imm_sect, const CInifile* ini)
{
    R_ASSERT2(ini->section_exist(imm_sect), imm_sect);

    m_HitImmunityKoefs[ALife::eHitTypeBurn] += READ_IF_EXISTS(ini, r_float, imm_sect, "burn_immunity", 0.0f);
    m_HitImmunityKoefs[ALife::eHitTypeStrike] += READ_IF_EXISTS(ini, r_float, imm_sect, "strike_immunity", 0.0f);
    m_HitImmunityKoefs[ALife::eHitTypeShock] += READ_IF_EXISTS(ini, r_float, imm_sect, "shock_immunity", 0.0f);
    m_HitImmunityKoefs[ALife::eHitTypeWound] += READ_IF_EXISTS(ini, r_float, imm_sect, "wound_immunity", 0.0f);
    m_HitImmunityKoefs[ALife::eHitTypeRadiation] += READ_IF_EXISTS(ini, r_float, imm_sect, "radiation_immunity", 0.0f);
    m_HitImmunityKoefs[ALife::eHitTypeTelepatic] += READ_IF_EXISTS(ini, r_float, imm_sect, "telepatic_immunity", 0.0f);
    m_HitImmunityKoefs[ALife::eHitTypeChemicalBurn] +=
        READ_IF_EXISTS(ini, r_float, imm_sect, "chemical_burn_immunity", 0.0f);
    m_HitImmunityKoefs[ALife::eHitTypeExplosion] += READ_IF_EXISTS(ini, r_float, imm_sect, "explosion_immunity", 0.0f);
    m_HitImmunityKoefs[ALife::eHitTypeFireWound] += READ_IF_EXISTS(ini, r_float, imm_sect, "fire_wound_immunity", 0.0f);
    //	m_HitImmunityKoefs[ALife::eHitTypePhysicStrike]	+= READ_IF_EXISTS(ini, r_float,
    // imm_sect,"physic_strike_wound_immunity", 0.0f);
    m_HitImmunityKoefs[ALife::eHitTypeLightBurn] = m_HitImmunityKoefs[ALife::eHitTypeBurn];
}
