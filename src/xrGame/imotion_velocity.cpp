#include "StdAfx.h"
#include "imotion_velocity.h"

#include "xrPhysics/PhysicsShell.h"

void imotion_velocity::state_start()
{
    VERIFY(shell);
    inherited::state_start();
    if (!is_enabled())
        return;
    shell->set_ApplyByGravity(false);

    // s->set_DynamicLimits(default_l_limit,default_w_limit * 5.f);
    // s->set_DynamicScales(1,1);
    // s->SetAirResistance(0,0);
}

void imotion_velocity::state_end()
{
    VERIFY(shell);
    inherited::state_end();
    shell->AnimToVelocityState(Device.fTimeDelta, default_l_limit * 10, default_w_limit * 10);
    shell->set_ApplyByGravity(true);
}

void imotion_velocity::collide() {}
void imotion_velocity::move_update()
{
    VERIFY(shell);
    if (!shell->AnimToVelocityState(Device.fTimeDelta, 2 * default_l_limit, 10.f * default_w_limit))
        flags.set(fl_switch_dm_toragdoll, TRUE);
    Fmatrix sv;
    sv.set(shell->mXFORM);
    shell->InterpolateGlobalTransform(&shell->mXFORM);
    shell->mXFORM.set(sv);
}
