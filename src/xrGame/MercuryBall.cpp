///////////////////////////////////////////////////////////////
// MercuryBall.cpp
// MercuryBall - переливающийся и колыхающийся шар
// перекатывается с места на место
///////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "MercuryBall.h"
#include "xrPhysics/PhysicsShell.h"

CMercuryBall::CMercuryBall(void)
{
    m_timeLastUpdate = 0;
    m_timeToUpdate = 1000;

    m_fImpulseMin = 45.f;
    m_fImpulseMax = 90.f;
}

CMercuryBall::~CMercuryBall(void) {}
void CMercuryBall::Load(LPCSTR section)
{
    inherited::Load(section);

    m_timeToUpdate = pSettings->r_u32(section, "time_to_update");
    m_fImpulseMin = pSettings->r_float(section, "impulse_min");
    m_fImpulseMax = pSettings->r_float(section, "impulse_max");
}

void CMercuryBall::UpdateCLChild()
{
    if (getVisible() && m_pPhysicsShell)
    {
        if (Device.TimerAsync() - m_timeLastUpdate > m_timeToUpdate)
        {
            m_timeLastUpdate = Device.TimerAsync();

            if (::Random.randF(0.f, 1.0f) > 0.6f)
            {
                Fvector dir;
                dir.set(::Random.randF(-0.5f, 0.5f), 0.0f, ::Random.randF(-0.5f, 0.5f));
                m_pPhysicsShell->applyImpulse(
                    dir, ::Random.randF(m_fImpulseMin, m_fImpulseMax) * Device.fTimeDelta * m_pPhysicsShell->getMass());
            }
        }
    }
    else if (H_Parent())
        XFORM().set(H_Parent()->XFORM());
}
