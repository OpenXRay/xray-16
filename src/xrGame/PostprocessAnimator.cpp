#include "StdAfx.h"
#pragma hdrstop

#include "PostprocessAnimator.h"
#include "ActorEffector.h"

CPostprocessAnimator::CPostprocessAnimator(int id, bool cyclic)
    : CEffectorPP((EEffectorPPType)id, 100000, true), BasicPostProcessAnimator(id, cyclic)
{
}

CPostprocessAnimator::CPostprocessAnimator() {}
void CPostprocessAnimator::Stop(float speed)
{
    // XXX nitrocaster: GSC implementation does not call CEffectorPP::Stop here. bug?
    BasicPostProcessAnimator::Stop(speed);
}

void CPostprocessAnimator::Load(LPCSTR name, bool internalFs /*= true*/)
{
    BasicPostProcessAnimator::Load(name, internalFs);
    if (!m_bCyclic)
        fLifeTime = f_length;
}

BOOL CPostprocessAnimator::Valid()
{
    if (m_bCyclic)
        return TRUE;

    return CEffectorPP::Valid();
}

BOOL CPostprocessAnimator::Process(SPPInfo& PPInfo)
{
    if (m_bCyclic)
        fLifeTime = 100000;

    CEffectorPP::Process(PPInfo);

    if (m_start_time < 0.0f)
        m_start_time = Device.fTimeGlobal;
    if (m_bCyclic && ((Device.fTimeGlobal - m_start_time) > f_length))
        m_start_time += f_length;

    Update(Device.fTimeGlobal - m_start_time);

    VERIFY(_valid(m_factor));
    VERIFY(_valid(m_factor_speed));
    VERIFY(_valid(m_dest_factor));
    if (m_bStop)
        m_factor -= Device.fTimeDelta * m_factor_speed;
    else
        m_factor += m_factor_speed * Device.fTimeDelta * (m_dest_factor - m_factor);

    clamp(m_factor, 0.0001f, 1.0f);

    VERIFY(_valid(m_factor));
    VERIFY(_valid(m_factor_speed));

    m_EffectorParams.color_base += pp_identity.color_base;
    m_EffectorParams.color_gray += pp_identity.color_gray;
    m_EffectorParams.color_add += pp_identity.color_add;

    if (0 == m_Params[pp_noise_i]->get_keys_count())
    {
        m_EffectorParams.noise.intensity = pp_identity.noise.intensity;
    }

    if (0 == m_Params[pp_noise_g]->get_keys_count())
    {
        m_EffectorParams.noise.grain = pp_identity.noise.grain;
    }

    if (0 == m_Params[pp_noise_f]->get_keys_count())
    {
        m_EffectorParams.noise.fps = pp_identity.noise.fps;
    }
    else
        m_EffectorParams.noise.fps *= 100.0f;

    PPInfo.lerp(pp_identity, m_EffectorParams, m_factor);

    if (PPInfo.noise.grain <= 0.0f)
    {
        R_ASSERT3(0, "noise.grain cant be zero! see postprocess", *m_Name);
    }

    if (fsimilar(m_factor, 0.0001f, EPS_S))
        return FALSE;

    return TRUE;
}

BOOL CPostprocessAnimatorLerp::Process(SPPInfo& PPInfo)
{
    if (!m_bStop)
        m_factor = m_get_factor_func();
    return CPostprocessAnimator::Process(PPInfo);
}

BOOL CPostprocessAnimatorLerpConst::Process(SPPInfo& PPInfo)
{
    if (!m_bStop)
        m_factor = m_power;
    return CPostprocessAnimator::Process(PPInfo);
}

CPostprocessAnimatorControlled::CPostprocessAnimatorControlled(CEffectorController* c) : m_controller(c)
{
    m_controller->SetPP(this);
    SetFactorFunc(fastdelegate::FastDelegate0<float>(m_controller, &CEffectorController::GetFactor));
}

CPostprocessAnimatorControlled::~CPostprocessAnimatorControlled() { m_controller->SetPP(NULL); }
BOOL CPostprocessAnimatorControlled::Valid() { return m_controller->Valid(); }
