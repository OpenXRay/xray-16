#include "StdAfx.h"
#include "PHShellCreator.h"
#include "xrPhysics/PhysicsShell.h"
#include "GameObject.h"
#include "PhysicsShellHolder.h"
#include "Include/xrRender/Kinematics.h"

void CPHShellSimpleCreator::CreatePhysicsShell()
{
    CPhysicsShellHolder* owner = smart_cast<CPhysicsShellHolder*>(this);
    VERIFY(owner);
    if (!owner->Visual())
        return;

    IKinematics* pKinematics = smart_cast<IKinematics*>(owner->Visual());
    VERIFY(pKinematics);

    if (owner->PPhysicsShell())
        return;

    phys_shell_verify_object_model(*owner);

    owner->PPhysicsShell() = P_create_Shell();
#ifdef DEBUG
    owner->PPhysicsShell()->dbg_obj = owner;
#endif
    owner->m_pPhysicsShell->build_FromKinematics(pKinematics, 0);

    owner->PPhysicsShell()->set_PhysicsRefObject(owner);
    // m_pPhysicsShell->SmoothElementsInertia(0.3f);
    owner->PPhysicsShell()->mXFORM.set(owner->XFORM());
    owner->PPhysicsShell()->SetAirResistance(0.001f, 0.02f);
}
