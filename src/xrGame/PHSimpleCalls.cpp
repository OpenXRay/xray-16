#include "StdAfx.h"
#include "xrPhysics/PhysicsShell.h"
#include "PHSimpleCalls.h"

// extern		CPHWorld			*ph_world;
#include "xrPhysics/IPHWorld.h"

CPHCallOnStepCondition::CPHCallOnStepCondition()
{
    if (physics_world())
        set_step(physics_world()->StepsNum());
    else
        set_step(0);
}

IC bool CPHCallOnStepCondition::time_out() const { return physics_world()->StepsNum() > m_step; }
bool CPHCallOnStepCondition::is_true() { return time_out(); }
bool CPHCallOnStepCondition::obsolete() const { return time_out(); }
void CPHCallOnStepCondition::set_steps_interval(u64 steps) { set_step(physics_world()->StepsNum() + steps); }
void CPHCallOnStepCondition::set_time_interval(float time) { set_steps_interval(iCeil(time / fixed_step)); }
void CPHCallOnStepCondition::set_time_interval(u32 time) { set_time_interval(float(time) / 1000.f); }
void CPHCallOnStepCondition::set_global_time(float time)
{
    float time_interval = Device.fTimeGlobal - time;
    if (time_interval < 0.f)
        set_step(physics_world()->StepsNum());
    set_time_interval(time_interval);
}
void CPHCallOnStepCondition::set_global_time(u32 time) { set_global_time(float(time) / 1000.f); }
CPHShellBasedAction::CPHShellBasedAction(CPhysicsShell* shell)
{
    VERIFY(shell && shell->isActive());
    m_shell = shell;
}
bool CPHShellBasedAction::obsolete() const { return !m_shell || !m_shell->isActive(); }
CPHConstForceAction::CPHConstForceAction(CPhysicsShell* shell, const Fvector& force) : CPHShellBasedAction(shell)
{
    m_force.set(force);
}

void CPHConstForceAction::run() { m_shell->applyForce(m_force.x, m_force.y, m_force.z); }
CPHReqComparerHasShell::CPHReqComparerHasShell(CPhysicsShell* shell)
{
    VERIFY(shell);
    m_shell = shell;
}
// CPHTimeCondition::CPHTimeCondition(u32 time)
//{
//	//m_step=u64(ph_world->CalcNumSteps(time))+ph_world->m_steps_num;
//}
//
// CPHTimeCondition::CPHTimeCondition(float time)
//{
//	///if (dTime < m_frame_time*1000) return 0;
//	u32 res = iCeil((float(dTime) - m_frame_time*1000) / (fixed_step*1000));
//	m_step=
//}
