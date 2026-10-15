#include "StdAfx.h"
#include "PhysicsShellAnimator.h"
#include "PhysicsShellAnimatorBoneData.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "Include/xrRender/Kinematics.h"
#include "PHElement.h"
#include "PHShell.h"
#include "IPhysicsShellHolder.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrPhysicsCore/IPhysicsCore.h"

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
    {
        for (auto& it : m_pPhysicsShell->Elements())
            CreateJoint(smart_cast<CPHElement*>(it));
    }

    if (ini->line_exist(section, "leave_joints") && xr_strcmp(ini->r_string(section, "leave_joints"), "all") == 0)
        return;

    while (m_pPhysicsShell->get_JointsNumber())
        static_cast<CPHShell*>(m_pPhysicsShell)->DeleteJoint(0);
}

CPhysicsShellAnimator::~CPhysicsShellAnimator()
{
    for (auto& it : m_bones_data)
    {
        if (it.m_anim_fixed_joint != INVALID_JOINT_HANDLE)
        {
            GetPhysicsCore()->DestroyJoint(it.m_anim_fixed_joint);
            it.m_anim_fixed_joint = INVALID_JOINT_HANDLE;
        }
        if (it.m_anim_target != INVALID_BODY_HANDLE)
            GetPhysicsCore()->DestroyBody(it.m_anim_target);
    }
    m_bones_data.clear();
}

void CPhysicsShellAnimator::CreateJoints(LPCSTR controled)
{
    [[maybe_unused]] IPhysicsShellHolder* obj = (*(m_pPhysicsShell->Elements().begin()))->PhysicsRefObject();

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
    if (!e) return;

    CPhysicsShellAnimatorBoneData data;
    data.m_element = e;
    if (!e->isFixed()) {
        auto* core = GetPhysicsCore();
        Fmatrix transform;
        core->GetBodyTransform(e->get_body(), transform);
        data.m_anim_target = core->CreateBox(Fvector().set(.01f, .01f, .01f), transform.c, 1);
        core->SetBodyMotionType(data.m_anim_target, 1);
        core->SetBodyObjectLayer(data.m_anim_target, 4);
        core->SetBodyTransform(data.m_anim_target, transform);
        const Fvector zero = Fvector().set(0, 0, 0);
        data.m_anim_fixed_joint = core->CreateJoint(3, data.m_anim_target, e->get_body(), transform.c,
            transform.i, transform.j, transform.k, zero, zero);
        R_ASSERT(data.m_anim_fixed_joint != INVALID_JOINT_HANDLE);
    }
    m_bones_data.push_back(data);
}

void CPhysicsShellAnimator::OnFrame()
{
    m_pPhysicsShell->Enable();

    for (auto& it : m_bones_data)
    {
        if (!it.m_element) continue;

        Fmatrix target_obj_posFmatrixS;
        CBoneInstance& B = m_pPhysicsShell->PKinematics()->LL_GetBoneInstance(it.m_element->m_SelfID);

        B.set_callback(B.callback_type(), nullptr, B.callback_param(), false);

        m_pPhysicsShell->PKinematics()->CalculateBones_Invalidate();
        m_pPhysicsShell->PKinematics()->CalculateBones(true);

        target_obj_posFmatrixS.mul_43(m_StartXFORM, B.mTransform);

        Fvector mc;
        it.m_element->CPHGeometryOwner::get_mc_vs_transform(mc, target_obj_posFmatrixS);
        if (it.m_anim_target != INVALID_BODY_HANDLE) {
            target_obj_posFmatrixS.c = mc;
            GetPhysicsCore()->SetBodyTransform(it.m_anim_target, target_obj_posFmatrixS);
        } else {
            it.m_element->SetTransform(target_obj_posFmatrixS, mh_unspecified);
        }
    }
}
