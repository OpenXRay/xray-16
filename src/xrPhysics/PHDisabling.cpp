#include "StdAfx.h"
#include "PHDisabling.h"
#include "PhysicsCommon.h"
#include "Physics.h"
#ifdef DEBUG
#include "debug_output.h"
#endif

extern CPHWorld* ph_world;

SDisableVector::SDisableVector() { Init(); }
void SDisableVector::Reset() { sum.set(0.f, 0.f, 0.f); }
void SDisableVector::Init()
{
    previous.set(0.f, 0.f, 0.f);
    Reset();
}

float SDisableVector::Update(const Fvector& new_vector)
{
    Fvector dif;
    dif.sub(new_vector, previous);
    previous.set(new_vector);
    sum.add(dif);
    return dif.magnitude();
}

float SDisableVector::UpdatePrevious(const Fvector& new_vector)
{
    Fvector dif;
    dif.sub(new_vector, previous);
    previous.set(new_vector);
    return dif.magnitude();
}

float SDisableVector::SumMagnitude() { return sum.magnitude(); }

SDisableUpdateState::SDisableUpdateState() { Reset(); }
void SDisableUpdateState::Reset()
{
    disable = false;
    enable = false;
}

SDisableUpdateState& SDisableUpdateState::operator&=(SDisableUpdateState& lstate)
{
    disable = disable && lstate.disable;
    enable = enable || lstate.enable;
    return *this;
}

CBaseDisableData::CBaseDisableData()
{
    m_disabled = false;
    m_count = 0;
    m_frames = 0;
    m_last_frame_updated = u16(-1);
}

void CBaseDisableData::Disabling()
{
    if (!ph_world || ph_world->IsFreezed() || m_last_frame_updated == ph_world->StepsShortCnt())
        return;
    m_last_frame_updated = ph_world->StepsShortCnt();
    VERIFY(m_frames > 0);
    if (m_count == 0) m_count = m_frames;
    --m_count;
    UpdateL1();
    CheckState(m_stateL1);
    if (m_count == 0)
    {
        UpdateL2();
        CheckState(m_stateL2);
        m_count = m_frames;
    }
    Fvector force, torque;
    GetPhysicsCore()->GetBodyForce(get_body(), force);
    GetPhysicsCore()->GetBodyTorque(get_body(), torque);
    if (force.square_magnitude() > 0 || torque.square_magnitude() > 0) m_disabled = false;
    // Jolt wakes connected bodies together. CPHShell applies this decision to
    // all of its elements at once instead of cycling individual bodies asleep.
}

void CBaseDisableData::Reinit()
{
    m_stateL1.Reset();
    m_stateL2.Reset();
    m_count = m_frames;
    if (ph_world) m_count += ph_world->disable_count;
    m_disabled = false;
    m_last_frame_updated = u16(-1);
}

void CPHDisablingBase::UpdateValues(const Fvector& new_pos, const Fvector& new_vel)
{
    if (m_count < m_frames)
    {
        CheckState(m_stateL1, m_mean_velocity.Update(new_pos) * m_frames,
            m_mean_acceleration.Update(new_vel) * m_frames);
    }
    else
    {
        CheckState(m_stateL1, m_mean_velocity.UpdatePrevious(new_pos) * m_frames,
            m_mean_acceleration.UpdatePrevious(new_vel) * m_frames);
    }
}

void CPHDisablingBase::UpdateL2()
{
    m_stateL2.Reset();
    float velocity_param = m_mean_velocity.SumMagnitude() / m_frames;
    float acceleration_param = m_mean_acceleration.SumMagnitude() / m_frames;

    CheckState(m_stateL2, velocity_param, acceleration_param);

    m_mean_velocity.Reset();
    m_mean_acceleration.Reset();
}

void CPHDisablingBase::set_DisableParams(const SOneDDOParams& params)
{
    m_params = params;
    m_mean_velocity.Reset();
    m_mean_acceleration.Reset();
}

void CPHDisablingBase::Reinit()
{
    CBaseDisableData::Reinit();
    m_mean_velocity.Init();
    m_mean_acceleration.Init();
}

CPHDisablingTranslational::CPHDisablingTranslational()
{
    m_params = worldDisablingParams.objects_params.translational;
    m_frames = worldDisablingParams.objects_params.L2frames;
}

void CPHDisablingTranslational::Reinit() { CPHDisablingBase::Reinit(); }

void CPHDisablingTranslational::UpdateL1()
{
    m_stateL1.Reset();
    CharacterVirtualHandle body = get_body();
    if (body == INVALID_CHARACTER_VIRTUAL_HANDLE) return;

    Fmatrix transform;
    GetPhysicsCore()->GetBodyTransform(body, transform);

    Fvector velocity;
    GetPhysicsCore()->GetBodyLinearVelocity(body, velocity);

    CPHDisablingBase::UpdateValues(transform.c, velocity);
}

void CPHDisablingTranslational::set_DisableParams(const SAllDDOParams& params)
{
    CPHDisablingBase::set_DisableParams(params.translational);
    m_frames = params.L2frames;
}

CPHDisablingRotational::CPHDisablingRotational()
{
    m_params = worldDisablingParams.objects_params.rotational;
    m_frames = worldDisablingParams.objects_params.L2frames;
}

void CPHDisablingRotational::Reinit() { CPHDisablingBase::Reinit(); }

void CPHDisablingRotational::UpdateL1()
{
    m_stateL1.Reset();
    CharacterVirtualHandle body = get_body();
    if (body == INVALID_CHARACTER_VIRTUAL_HANDLE) return;

    Fmatrix transform;
    GetPhysicsCore()->GetBodyTransform(body, transform);

    Fvector angular_velocity;
    GetPhysicsCore()->GetBodyAngularVelocity(body, angular_velocity);

    // Магия X-Ray: замена ODEшных индексов вращения (9, 2, 4) на компоненты Fmatrix (k.y, i.z, j.x)
    Fvector vrotation;
    vrotation.set(transform.k.y, transform.i.z, transform.j.x);

    CPHDisablingBase::UpdateValues(vrotation, angular_velocity);
}

void CPHDisablingRotational::set_DisableParams(const SAllDDOParams& params)
{
    CPHDisablingBase::set_DisableParams(params.rotational);
    m_frames = params.L2frames;
}

void CPHDisablingFull::Reinit()
{
    CPHDisablingRotational::Reinit();
    CPHDisablingTranslational::Reinit();
}

void CPHDisablingFull::UpdateL1()
{
    SDisableUpdateState state;
    CPHDisablingRotational::UpdateL1();
    state = m_stateL1;
    CPHDisablingTranslational::UpdateL1();
    m_stateL1 &= state;
}

void CPHDisablingFull::UpdateL2()
{
    SDisableUpdateState state;
    CPHDisablingRotational::UpdateL2();
    state = m_stateL2;
    CPHDisablingTranslational::UpdateL2();
    m_stateL2 &= state;
}

void CPHDisablingFull::set_DisableParams(const SAllDDOParams& params)
{
    CPHDisablingRotational::set_DisableParams(params);
    CPHDisablingTranslational::set_DisableParams(params);
}
