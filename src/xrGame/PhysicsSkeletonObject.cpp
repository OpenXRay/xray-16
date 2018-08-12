#include "StdAfx.h"
#include "PhysicsSkeletonObject.h"

#include "xrPhysics/PhysicsShell.h"
#include "PHSynchronize.h"
#include "xrServer_Objects_ALife.h"
#include "Include/xrRender/Kinematics.h"
#include "xrEngine/xr_collide_form.h"

CPhysicsSkeletonObject::CPhysicsSkeletonObject() {}
CPhysicsSkeletonObject::~CPhysicsSkeletonObject() {}
BOOL CPhysicsSkeletonObject::net_Spawn(CSE_Abstract* DC)
{
    CSE_Abstract* e = (CSE_Abstract*)(DC);

    inherited::net_Spawn(DC);
    xr_delete(CForm);
    CForm = new CCF_Skeleton(this);
    CPHSkeleton::Spawn(e);
    setVisible(TRUE);
    setEnabled(TRUE);
    if (!PPhysicsShell()->isBreakable())
        SheduleUnregister();
    return TRUE;
}

void CPhysicsSkeletonObject::SpawnInitPhysics(CSE_Abstract* D)
{
    CreatePhysicsShell(D);
    IKinematics* K = smart_cast<IKinematics*>(Visual());
    if (K)
    {
        K->CalculateBones_Invalidate();
        K->CalculateBones(TRUE);
    }
}

void CPhysicsSkeletonObject::net_Destroy()
{
    inherited::net_Destroy();
    CPHSkeleton::RespawnInit();
}

void CPhysicsSkeletonObject::Load(LPCSTR section)
{
    inherited::Load(section);
    CPHSkeleton::Load(section);
}

void CPhysicsSkeletonObject::CreatePhysicsShell(CSE_Abstract* e)
{
    CSE_PHSkeleton* po = smart_cast<CSE_PHSkeleton*>(e);
    if (m_pPhysicsShell)
        return;
    if (!Visual())
        return;
    m_pPhysicsShell = P_build_Shell(this, !po->_flags.test(CSE_PHSkeleton::flActive));
}

void CPhysicsSkeletonObject::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    CPHSkeleton::Update(dt);
}

void CPhysicsSkeletonObject::net_Save(NET_Packet& P)
{
    inherited::net_Save(P);
    CPHSkeleton::SaveNetState(P);
}

BOOL CPhysicsSkeletonObject::net_SaveRelevant()
{
    return TRUE; //! m_flags.test(CSE_ALifeObjectPhysic::flSpawnCopy);
}

BOOL CPhysicsSkeletonObject::UsedAI_Locations() { return (FALSE); }
void CPhysicsSkeletonObject::UpdateCL()
{
    inherited::UpdateCL();
    PHObjectPositionUpdate();
}

void CPhysicsSkeletonObject::PHObjectPositionUpdate()
{
    if (m_pPhysicsShell)
    {
        m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());
    }
}
