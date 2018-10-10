// Wound.cpp: класс описания раны
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Wound.h"
#include "xrCore/Animation/Bone.hpp"

CWound::CWound(u16 bone_num)
{
    m_bToBeDestroy = false;

    m_Wounds.resize(ALife::eHitTypeMax);
    for (int i = 0; i < ALife::eHitTypeMax; i++)
    {
        m_Wounds[i] = 0.f;
    }

    m_iBoneNum = bone_num;
    m_iParticleBoneNum = BI_NONE;

    m_fDropTime = 0.f;
}

CWound::~CWound(void) {}
#define WOUND_MAX 10.f

// serialization
void CWound::save(NET_Packet& output_packet)
{
    output_packet.w_u8((u8)m_iBoneNum);
    for (int i = 0; i < ALife::eHitTypeMax; i++)
        output_packet.w_float_q8(m_Wounds[i], 0.f, WOUND_MAX);
}
void CWound::load(IReader& input_packet)
{
    m_iBoneNum = (u8)input_packet.r_u8();
    for (int i = 0; i < ALife::eHitTypeMax; i++)
    {
        m_Wounds[i] = input_packet.r_float_q8(0.f, WOUND_MAX);
        VERIFY(m_Wounds[i] >= 0.0f && m_Wounds[i] <= WOUND_MAX);
    }
}

float CWound::TotalSize()
{
    float total_size = 0.f;
    for (int i = 0; i < ALife::eHitTypeMax; i++)
    {
        VERIFY(_valid(m_Wounds[i]));
        total_size += m_Wounds[i];
    }
    VERIFY(_valid(total_size));
    return total_size;
}

float CWound::TypeSize(ALife::EHitType hit_type) { return m_Wounds[hit_type]; }
//кол-во кровавых ран
float CWound::BloodSize() { return m_Wounds[ALife::eHitTypeWound] + m_Wounds[ALife::eHitTypeFireWound]; }
void CWound::AddHit(float hit_power, ALife::EHitType hit_type)
{
    m_Wounds[hit_type] += hit_power;
    clamp(m_Wounds[hit_type], 0.0f, WOUND_MAX);
}

void CWound::Incarnation(float percent, float min_wound_size)
{
    float total_size = TotalSize();

    if (fis_zero(total_size))
    {
        for (int i = 0; i < ALife::eHitTypeMax; i++)
            m_Wounds[i] = 0.f;
        return;
    }

    //заживить все раны пропорционально их размеру
    for (int i = 0; i < ALife::eHitTypeMax; i++)
    {
        m_Wounds[i] -= percent /* *m_Wounds[i]*/;
        if (m_Wounds[i] < min_wound_size)
            m_Wounds[i] = 0;
    }
}
