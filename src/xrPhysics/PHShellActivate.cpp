#include "StdAfx.h"
#include <cstdlib>
#include "PHDynamicData.h"
#include "Physics.h"
#include "PHShellSplitter.h"
#include "PHFracture.h"
#include "PHJointDestroyInfo.h"
#include "PHCollideValidator.h"

#include "IPhysicsShellHolder.h"
#include "PhysicsShellAnimator.h"
#include "Include/xrRender/Kinematics.h"

#include "ExtendedGeom.h"

#include "PHElement.h"
#include "PHElementInline.h"
#include "PHShell.h"
#include "xrPhysicsCore/IPhysicsCore.h"

void CPHShell::activate(bool disable)
{
    PresetActive();
    if (!CPHObject::is_active())
        vis_update_deactivate();
    if (!disable)
        EnableObject(0);
}

void CPHShell::Activate(const Fmatrix& m0, float dt01, const Fmatrix& m2, bool disable)
{
    if (isActive())
        return;

    activate(disable);
    mXFORM.set(m0);

    {
        auto i = elements.begin(), e = elements.end();
        for (; i != e; ++i)
            (*i)->Activate(mXFORM, disable);
    }

    {
        auto i = joints.begin(), e = joints.end();
        for (; i != e; ++i)
            (*i)->Activate();
    }

    Build();
    RunSimulation(false);

    m_flags.set(flActivating, TRUE);
    m_flags.set(flActive, TRUE);
    spatial_register();
}

void CPHShell::Activate(const Fmatrix& transform, const Fvector& velocity, const Fvector& angular_velocity, bool disable)
{
    if (isActive())
        return;

    activate(disable);
    mXFORM.set(transform);

    {
        auto i = elements.begin(), e = elements.end();
        for (; i != e; ++i)
            (*i)->Activate(transform, velocity, angular_velocity);
    }

    {
        auto i = joints.begin(), e = joints.end();
        for (; i != e; ++i)
            (*i)->Activate();
    }

    Build();
    RunSimulation(false);

    m_flags.set(flActivating, TRUE);
    m_flags.set(flActive, TRUE);
    spatial_register();
}

void CPHShell::Build(bool disable)
{
    if (isActive())
        return;

    if (PhysicsRefObject())
        m_pKinematics = PhysicsRefObject()->ObjectKinematics();

    {
        auto i = elements.begin(), e = elements.end();
        for (; i != e; ++i)
        {
            (*i)->build(disable);
        }
    }

    {
        auto i = joints.begin(), e = joints.end();
        for (; i != e; ++i)
            (*i)->Create();
    }
    // Living characters prepare death shells ahead of time. Their bodies and
    // joints must remain outside the simulation until RunSimulation is called.
    for (auto* element : elements)
        GetPhysicsCore()->SetBodyInWorld(element->get_body(), false);
}

void CPHShell::RunSimulation(bool place_current_forms)
{
    if (isActive())
        return;

    m_flags.set(flActive, TRUE);
    PresetActive();

    for (auto i = elements.begin(), e = elements.end(); i != e; ++i)
    {
        GetPhysicsCore()->SetBodyInWorld((*i)->get_body(), true);
        (*i)->RunSimulation();
    }

    for (auto i = joints.begin(), e = joints.end(); i != e; ++i)
        (*i)->RunSimulation();

    spatial_register();
}

void CPHShell::Deactivate()
{
    if (!isActive())
        return;

    R_ASSERT2(!ph_world->IsFreezed(), "can not deactivate physics shell when ph world is freezed!!!");
    R_ASSERT2(!CPHObject::IsFreezed(), "can not deactivate freezed !!!");
    ZeroCallbacks();
    VERIFY(ph_world && ph_world->Exist());

    if (isFullActive())
    {
        vis_update_deactivate();
        CPHObject::activate();
    }

    spatial_unregister();
    vis_update_activate();
    DisableObject();

    CPHObject::remove_from_recently_deactivated();

    for (auto i = elements.begin(); elements.end() != i; ++i)
        (*i)->Deactivate();

    for (auto j = joints.begin(); joints.end() != j; ++j)
        (*j)->Deactivate();

    m_flags.set(flActivating, FALSE);
    m_flags.set(flActive, FALSE);
}

void CPHShell::vis_update_activate()
{
    ++m_active_count;
    auto* holder = PhysicsRefObject();
    if (holder && m_active_count > 0)
    {
        m_active_count = 0;
        holder->ObjectProcessingActivate();
    }
}

void CPHShell::vis_update_deactivate()
{
    --m_active_count;
}

void CPHShell::PhDataUpdate(float dTime)
{
    bool disabled = true;
    bool wantsSleep = true;
    auto i = elements.begin(), e = elements.end();
    for (; i != e; ++i)
    {
        (*i)->PhDataUpdate(dTime);
        if (GetPhysicsCore()->IsBodyActive((*i)->get_body())) disabled = false;
        if (!(*i)->isFixed() && GetPhysicsCore()->IsBodyActive((*i)->get_body()) && !(*i)->WantsSleep()) wantsSleep = false;
    }
    static const bool profile = std::getenv("XRAY_JOLT_CONTACT_PROFILE") != nullptr;
    if (profile && ph_world->StepsNum() % 100 == 0) {
        unsigned active = 0, votes = 0, level1 = 0, level2 = 0;
        float linear = 0, angular = 0;
        for (auto* element : elements) {
            if (!GetPhysicsCore()->IsBodyActive(element->get_body())) continue;
            ++active;
            votes += element->WantsSleep();
            level1 += bool(element->SleepProfileFlags() & 1);
            level2 += bool(element->SleepProfileFlags() & 4);
            Fvector velocity;
            GetPhysicsCore()->GetBodyLinearVelocity(element->get_body(), velocity);
            linear = std::max(linear, velocity.magnitude());
            GetPhysicsCore()->GetBodyAngularVelocity(element->get_body(), velocity);
            angular = std::max(angular, velocity.magnitude());
        }
        Msg("NATIVE_SLEEP_PROFILE name=%s active=%u votes=%u l1=%u l2=%u linear=%g angular=%g",
            PhysicsRefObject() ? PhysicsRefObject()->ObjectName() : "fixture", active, votes, level1, level2, linear, angular);
    }
    if (wantsSleep && !disabled)
    {
        for (auto* element : elements) element->Disable();
        disabled = true;
    }
    if (disabled && isEnabled())
    {
        DisableObject();
        CPHObject::put_in_recently_deactivated();
    }

    if (m_flags.test(flActivating))
        m_flags.set(flActivating, FALSE);

    InterpolateGlobalTransform(&mXFORM);
}

void CPHShell::PhTune(float dTime)
{
    auto i = elements.begin(), e = elements.end();
    for (; i != e; ++i)
        (*i)->PhTune(dTime);
}

void CPHShell::Update()
{
    if (!isActive())
        return;

    if (m_flags.test(flActivating))
        InterpolateGlobalTransform(&mXFORM);
    else
        InterpolateGlobalPosition(&mXFORM.c);
}
