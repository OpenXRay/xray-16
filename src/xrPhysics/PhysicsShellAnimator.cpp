#include "StdAfx.h"
#include "PhysicsShellAnimator.h"
#include "PhysicsShellAnimatorBoneData.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "Include/xrRender/Kinematics.h"
#include "PHDynamicData.h"

#include "IPhysicsShellHolder.h"
#include "xrCore/Animation/Bone.hpp"

CPhysicsShellAnimator::CPhysicsShellAnimator(CPhysicsShell* _pPhysicsShell, CInifile const* ini, LPCSTR section)
    : m_pPhysicsShell(_pPhysicsShell)
{
    VERIFY(ini->section_exist(section));
    IPhysicsShellHolder* obj = (*(_pPhysicsShell->Elements().begin()))->PhysicsRefObject();
    m_StartXFORM.set(obj->ObjectXFORM());
    bool all_bones = true;
    if (ini->line_exist(section, "controled_bones"))
    {
        LPCSTR controled = ini->r_string(section, "controled_bones");
        all_bones = xr_strcmp(controled, "all") == 0;
        if (!all_bones)
            CreateJoints(controled);
    }

    if (all_bones)
        for (auto& it : m_pPhysicsShell->Elements())
            CreateJoint(it);

    if (ini->line_exist(section, "leave_joints") && xr_strcmp(ini->r_string(section, "leave_joints"), "all") == 0)
        return;

    for (u16 i = 0; i < m_pPhysicsShell->get_JointsNumber(); i++)
    {
        ((CPHShell*)(m_pPhysicsShell))->DeleteJoint(i);
    }
}

CPhysicsShellAnimator::~CPhysicsShellAnimator()
{
    for (auto& it : m_bones_data)
    {
        ((CPHShell*)(m_pPhysicsShell))->Island().DActiveIsland()->RemoveJoint(it.m_anim_fixed_dJointID);
        dJointDestroy(it.m_anim_fixed_dJointID);
    }
}
void CPhysicsShellAnimator::CreateJoints(LPCSTR controled)
{
    IPhysicsShellHolder* obj = (*(m_pPhysicsShell->Elements().begin()))->PhysicsRefObject();
    const u16 nb = (u16)_GetItemCount(controled);
    for (u16 i = 0; nb > i; ++i)
    {
        string64 n;
        _GetItem(controled, i, n);
        u16 bid = m_pPhysicsShell->PKinematics()->LL_BoneID(n);
        VERIFY2(bid != BI_NONE, make_string("shell_animation - controled bone %s not found! object: %s, model: %s", n,
                                    obj->ObjectName(), obj->ObjectNameVisual()));
        CPHElement* e = smart_cast<CPHElement*>(m_pPhysicsShell->get_Element(bid));
        VERIFY2(e, make_string("shell_animation - controled bone %s has no physics collision! object: %s, model: %s", n,
                       obj->ObjectName(), obj->ObjectNameVisual()));
        CreateJoint(e);
    }
}
void CPhysicsShellAnimator::CreateJoint(CPHElement* e)
{
    CPhysicsShellAnimatorBoneData PhysicsShellAnimatorBoneDataC;
    PhysicsShellAnimatorBoneDataC.m_element = e;
    PhysicsShellAnimatorBoneDataC.m_anim_fixed_dJointID = dJointCreateFixed(0, 0);
    ((CPHShell*)(m_pPhysicsShell))
        ->Island()
        .DActiveIsland()
        ->AddJoint(PhysicsShellAnimatorBoneDataC.m_anim_fixed_dJointID);
    dJointAttach(
        PhysicsShellAnimatorBoneDataC.m_anim_fixed_dJointID, PhysicsShellAnimatorBoneDataC.m_element->get_body(), 0);
    dJointSetFixed(PhysicsShellAnimatorBoneDataC.m_anim_fixed_dJointID);
    m_bones_data.push_back(PhysicsShellAnimatorBoneDataC);
}
void CPhysicsShellAnimator::OnFrame()
{
    m_pPhysicsShell->Enable();

    for (auto it : m_bones_data)
    {
        Fmatrix target_obj_posFmatrixS;
        CBoneInstance& B = m_pPhysicsShell->PKinematics()->LL_GetBoneInstance(it.m_element->m_SelfID);
// B.Callback_overwrite = FALSE;
// B.Callback = 0;
#pragma todo("reset callback?")
        B.set_callback(B.callback_type(), nullptr, B.callback_param(), false);

        m_pPhysicsShell->PKinematics()->CalculateBones_Invalidate();
        m_pPhysicsShell->PKinematics()->CalculateBones(true);

        target_obj_posFmatrixS.mul_43(m_StartXFORM, B.mTransform);
        dQuaternion target_obj_quat_dQuaternionS;
        dMatrix3 ph_mat;
        PHDynamicData::FMXtoDMX(target_obj_posFmatrixS, ph_mat);
        dQfromR(target_obj_quat_dQuaternionS, ph_mat);
        Fvector mc;
        it.m_element->CPHGeometryOwner::get_mc_vs_transform(mc, target_obj_posFmatrixS);
        dJointSetFixedQuaternionPos(it.m_anim_fixed_dJointID, target_obj_quat_dQuaternionS, &mc.x);
    }
    //(*(m_pPhysicsShell->Elements().begin()))->PhysicsRefObject()->XFORM().set(m_pPhysicsShell->mXFORM);
}
