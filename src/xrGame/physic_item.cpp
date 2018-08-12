////////////////////////////////////////////////////////////////////////////
//	Module 		: physic_item.cpp
//	Created 	: 11.02.2004
//  Modified 	: 11.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Physic item
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "physic_item.h"
#include "xrPhysics/PhysicsShell.h"
#include "xrServer_Objects.h"
#include "Include/xrRender/RenderVisual.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "Include/xrRender/Kinematics.h"
#define CHOOSE_MAX(x, inst_x, y, inst_y, z, inst_z) \
    if (x > y)                                      \
        if (x > z)                                  \
        {                                           \
            inst_x;                                 \
        }                                           \
        else                                        \
        {                                           \
            inst_z;                                 \
        }                                           \
    else if (y > z)                                 \
    {                                               \
        inst_y;                                     \
    }                                               \
    else                                            \
    {                                               \
        inst_z;                                     \
    }

CPhysicItem::CPhysicItem() { init(); }
CPhysicItem::~CPhysicItem() { xr_delete(m_pPhysicsShell); }
void CPhysicItem::init() { m_pPhysicsShell = 0; }
void CPhysicItem::reinit()
{
    inherited::reinit();
    m_ready_to_destroy = false;
}

void CPhysicItem::Load(LPCSTR section) { inherited::Load(section); }
void CPhysicItem::reload(LPCSTR section) { inherited::reload(section); }
void CPhysicItem::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);

    if (m_ready_to_destroy)
        return;

    setVisible(TRUE);
    setEnabled(TRUE);

    if (!just_before_destroy)
        activate_physic_shell();
}

void CPhysicItem::OnH_B_Chield()
{
    inherited::OnH_B_Chield();

    setVisible(FALSE);
    setEnabled(FALSE);

    inherited::deactivate_physics_shell();
}

BOOL CPhysicItem::net_Spawn(CSE_Abstract* DC)
{
    if (!inherited::net_Spawn(DC))
        return (FALSE);
    IKinematics* pK = smart_cast<IKinematics*>(Visual());
    pK->CalculateBones_Invalidate();
    pK->CalculateBones(TRUE);
    CSE_Abstract* abstract = (CSE_Abstract*)DC;
    if (0xffff == abstract->ID_Parent)
    {
        if (!PPhysicsShell())
            setup_physic_shell();
        // else processing_deactivate();//.
    }

    setVisible(TRUE);
    setEnabled(TRUE);

    return (TRUE);
}

void CPhysicItem::net_Destroy() { inherited::net_Destroy(); }
void CPhysicItem::UpdateCL()
{
    //	if (!xr_strcmp("bolt",cName()))
    //		Log					("--- B - CBolt",renderable.xform);
    if (!H_Parent() && m_pPhysicsShell && m_pPhysicsShell->isActive())
        m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());
    //	if (!xr_strcmp("bolt",cName()))
    //		Log						("--- C - CBolt",renderable.xform);
    inherited::UpdateCL();
    //	if (!xr_strcmp("bolt",cName()))
    //		Log						("--- D - CBolt",renderable.xform);
}

void CPhysicItem::activate_physic_shell()
{
    IGameObject* object = smart_cast<IGameObject*>(H_Parent());
    R_ASSERT(object);
    XFORM().set(object->XFORM());
    inherited::activate_physic_shell();
    IKinematics* K = smart_cast<IKinematics*>(Visual());
    if (K)
    {
        K->CalculateBones_Invalidate();
        K->CalculateBones(TRUE);
    }
    /// m_pPhysicsShell->Update		();
}

void CPhysicItem::setup_physic_shell()
{
    inherited::setup_physic_shell();
    IKinematics* K = smart_cast<IKinematics*>(Visual());
    if (K)
    {
        K->CalculateBones_Invalidate();
        K->CalculateBones(TRUE);
    }

    // m_pPhysicsShell->Update		();
}

void CPhysicItem::create_box_physic_shell()
{
    // Physics (Box)
    Fobb obb;
    Visual()->getVisData().box.get_CD(obb.m_translate, obb.m_halfsize);
    obb.m_rotate.identity();

    // Physics (Elements)
    CPhysicsElement* E = P_create_Element();
    R_ASSERT(E);
    E->add_Box(obb);
    // Physics (Shell)
    m_pPhysicsShell = P_create_Shell();
    R_ASSERT(m_pPhysicsShell);
    m_pPhysicsShell->add_Element(E);
    m_pPhysicsShell->setDensity(2000.f);
}

void CPhysicItem::create_box2sphere_physic_shell()
{
    // Physics (Box)
    Fobb obb;
    Visual()->getVisData().box.get_CD(obb.m_translate, obb.m_halfsize);
    obb.m_rotate.identity();

    // Physics (Elements)
    CPhysicsElement* E = P_create_Element();
    R_ASSERT(E);

    Fvector ax;
    float radius;
    CHOOSE_MAX(obb.m_halfsize.x, ax.set(obb.m_rotate.i); ax.mul(obb.m_halfsize.x);
               radius = _min(obb.m_halfsize.y, obb.m_halfsize.z); obb.m_halfsize.y /= 2.f;
               obb.m_halfsize.z /= 2.f, obb.m_halfsize.y, ax.set(obb.m_rotate.j); ax.mul(obb.m_halfsize.y);
               radius = _min(obb.m_halfsize.x, obb.m_halfsize.z); obb.m_halfsize.x /= 2.f;
               obb.m_halfsize.z /= 2.f, obb.m_halfsize.z, ax.set(obb.m_rotate.k); ax.mul(obb.m_halfsize.z);
               radius = _min(obb.m_halfsize.y, obb.m_halfsize.x); obb.m_halfsize.y /= 2.f; obb.m_halfsize.x /= 2.f)
    // radius*=1.4142f;
    Fsphere sphere1, sphere2;
    sphere1.P.add(obb.m_translate, ax);
    sphere1.R = radius * 1.4142f;

    sphere2.P.sub(obb.m_translate, ax);
    sphere2.R = radius / 2.f;

    E->add_Box(obb);
    E->add_Sphere(sphere1);
    E->add_Sphere(sphere2);

    // Physics (Shell)
    m_pPhysicsShell = P_create_Shell();
    R_ASSERT(m_pPhysicsShell);
    m_pPhysicsShell->add_Element(E);
    m_pPhysicsShell->setDensity(2000.f);
    m_pPhysicsShell->SetAirResistance();
}

void CPhysicItem::create_physic_shell()
{
    /// create_box_physic_shell();
    inherited::create_physic_shell();
}
