#include "StdAfx.h"
#include "NoGravityZone.h"
#include "xrPhysics/PhysicsShell.h"
#include "entity_alive.h"
#include "PHMovementControl.h"

#include "CharacterPhysicsSupport.h"
// extern CPHWorld	*ph_world;
#include "xrPhysics/IPHWorld.h"
void CNoGravityZone::enter_Zone(SZoneObjectInfo& io)
{
    inherited::enter_Zone(io);
    switchGravity(io, false);
}
void CNoGravityZone::exit_Zone(SZoneObjectInfo& io)
{
    switchGravity(io, true);
    inherited::exit_Zone(io);
}
void CNoGravityZone::UpdateWorkload(u32 dt)
{
    auto i = m_ObjectInfoMap.begin(), e = m_ObjectInfoMap.end();
    for (; e != i; i++)
        switchGravity(*i, false);
}
void CNoGravityZone::switchGravity(SZoneObjectInfo& io, bool val)
{
    if (io.object->getDestroy())
        return;
    CPhysicsShellHolder* sh = smart_cast<CPhysicsShellHolder*>(io.object);
    if (!sh)
        return;
    CPhysicsShell* shell = sh->PPhysicsShell();
    if (shell && shell->isActive())
    {
        shell->set_ApplyByGravity(val);
        if (!val && shell->get_ApplyByGravity())
        {
            CPhysicsElement* e = shell->get_ElementByStoreOrder(u16(Random.randI(0, shell->get_ElementsNumber())));
            if (e->isActive())
            {
                e->applyImpulseTrace(Fvector().random_point(e->getRadius()), Fvector().random_dir(),
                    shell->getMass() * physics_world()->Gravity() * fixed_step, e->m_SelfID);
            }
        }
        // shell->SetAirResistance(0.f,0.f);
        // shell->set_DynamicScales(1.f);
        return;
    }
    if (!io.nonalive_object)
    {
        CEntityAlive* ea = smart_cast<CEntityAlive*>(io.object);
        CPHMovementControl* mc = ea->character_physics_support()->movement();
        mc->SetApplyGravity(BOOL(val));
        mc->SetForcedPhysicsControl(!val);
        if (!val && mc->Environment() == CPHMovementControl::peOnGround)
        {
            Fvector gn;
            mc->GroundNormal(gn);
            mc->ApplyImpulse(gn, mc->GetMass() * physics_world()->Gravity() * fixed_step);
        }
    }
}
