#include "StdAfx.h"
#include "PHIsland.h"
#include "Physics.h"
#include "ph_valid_ode.h"
void CPHIsland::Step(dReal step)
{
    if (!m_flags.is_active())
        return;
    // dWorldStepFast1	(DWorld(),	fixed_step,	phIterations/*+Random.randI(0,phIterationCycle)*/);
    if (m_flags.is_exact_integration_prefeared() && nj < max_joint_allowed_for_exeact_integration)
        dWorldStep(DWorld(), fixed_step);
    else
        dWorldQuickStep(DWorld(), fixed_step);
    // dWorldStep(DWorld(),fixed_step);
}

void CPHIsland::Enable()
{
    if (!m_flags.is_active())
        return;

    for (dxBody* body = DWorld()->firstbody; body; body = (dxBody*)body->next)
        body->flags &= ~dxBodyDisabled;
}
void CPHIsland::Repair()
{
    if (!m_flags.is_active())
        return;
    dBodyID body;
    for (body = firstbody; body; body = (dxBody*)body->next)
    {
        if (!dV_valid(dBodyGetAngularVel(body)))
            dBodySetAngularVel(body, 0.f, 0.f, 0.f);
        if (!dV_valid(dBodyGetLinearVel(body)))
            dBodySetLinearVel(body, 0.f, 0.f, 0.f);
        if (!dV_valid(dBodyGetPosition(body)))
            dBodySetPosition(body, 0.f, 0.f, 0.f);
        if (!dQ_valid(dBodyGetQuaternion(body)))
        {
            dQuaternion q = {1.f, 0.f, 0.f, 0.f}; // dQSetIdentity(q);
            dBodySetQuaternion(body, q);
        }
    }
}
