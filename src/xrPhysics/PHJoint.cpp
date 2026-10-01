#include "StdAfx.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "PHJointDestroyInfo.h"
#include "ExtendedGeom.h"
#include "PHElement.h"
#include "PHJoint.h"
#include "PHShell.h"
#include "xrPhysicsCore/IPhysicsCore.h"

const float hinge2_spring = 20000.f;
const float hinge2_damping = 1000.f;

IC BodyHandle body_for_joint(CPhysicsElement* ee)
{
    VERIFY(smart_cast<CPHElement*>(ee));
    CPHElement* e = static_cast<CPHElement*>(ee);
    // The legacy joint convention treats fixed elements as world anchors.
    return e->isFixed() ? INVALID_BODY_HANDLE : e->get_body();
}

IC void SwapLimits(float& lo, float& hi)
{
    float t = -lo;
    lo = -hi;
    hi = t;
}

CPHJoint::~CPHJoint()
{
    xr_delete(m_destroy_info);
    VERIFY(!bActive);
    axes.clear();
    if (m_back_ref)
        *m_back_ref = nullptr;
}

void CPHJoint::SetBackRef(CPhysicsJoint** j)
{
    R_ASSERT2(*j == static_cast<CPhysicsJoint*>(this), "wrong reference");
    m_back_ref = j;
}

void CPHJoint::CreateBall()
{
    Fvector pos;
    Fmatrix first_matrix, second_matrix;
    CPHElement* first = pFirst_element;
    CPHElement* second = pSecond_element;

    VERIFY(first && second);
    first->GetGlobalTransformDynamic(&first_matrix);
    second->GetGlobalTransformDynamic(&second_matrix);
    pos.set(0, 0, 0);
    switch (vs_anchor)
    {
    case vs_first: first_matrix.transform_tiny(pos, anchor); break;
    case vs_second: second_matrix.transform_tiny(pos, anchor); break;
    case vs_global: pShell->mXFORM.transform_tiny(pos, anchor); break;
    default: NODEFAULT;
    }

    m_joint = GetPhysicsCore()->CreateJoint(ball, body_for_joint(first), body_for_joint(second), pos,
                                            Fvector().set(0,0,0), Fvector().set(0,0,0), Fvector().set(0,0,0),
                                            Fvector().set(0,0,0), Fvector().set(0,0,0));
}

void CPHJoint::CreateHinge()
{
    Fvector pos, axis0;
    Fmatrix first_matrix, second_matrix;

    CPHElement* first = pFirst_element;
    CPHElement* second = pSecond_element;
    VERIFY(first && second);
    first->GetGlobalTransformDynamic(&first_matrix);
    second->GetGlobalTransformDynamic(&second_matrix);

    pos.set(0, 0, 0);
    switch (vs_anchor)
    {
    case vs_first: first_matrix.transform_tiny(pos, anchor); break;
    case vs_second: second_matrix.transform_tiny(pos, anchor); break;
    case vs_global: pShell->mXFORM.transform_tiny(pos, anchor); break;
    default: NODEFAULT;
    }

    float lo, hi;
    axis0.set(0, 0, 0);

    CharacterVirtualHandle b1 = body_for_joint(first);
    CharacterVirtualHandle b2 = body_for_joint(second);

    CalcAxis(0, axis0, lo, hi, first_matrix, second_matrix);

    Fvector ref_normal;
    if (b1 != INVALID_CHARACTER_VIRTUAL_HANDLE)
        first_matrix.transform_dir(ref_normal, Fvector().set(1.f, 0.f, 0.f));
    else
        pShell->mXFORM.transform_dir(ref_normal, Fvector().set(1.f, 0.f, 0.f));

    ref_normal.sub(Fvector(axis0).mul(axis0.dotproduct(ref_normal)));
    if (ref_normal.square_magnitude() < 0.001f)
    {
        if (b1 != INVALID_CHARACTER_VIRTUAL_HANDLE)
            first_matrix.transform_dir(ref_normal, Fvector().set(0.f, 0.f, 1.f));
        else
            pShell->mXFORM.transform_dir(ref_normal, Fvector().set(0.f, 0.f, 1.f));
    }
    ref_normal.normalize_safe();

    float jolt_lo = lo;
    float jolt_hi = hi;
    if (jolt_lo > jolt_hi) std::swap(jolt_lo, jolt_hi);

    m_joint = GetPhysicsCore()->CreateJoint(hinge, b1, b2, pos,
                                            axis0, ref_normal, Fvector().set(0,0,0),
                                            Fvector().set(jolt_lo, 0, 0), Fvector().set(jolt_hi, 0, 0));

    if (axes[0].force > 0.f)
        GetPhysicsCore()->SetJointMotor(m_joint, 0, axes[0].force, axes[0].velocity);

    GetPhysicsCore()->SetJointSpringDamping(m_joint, 0, axes[0].erp, axes[0].cfm);
}

void CPHJoint::CreateHinge2()
{
    Fvector pos, axis0, axis1;
    Fmatrix first_matrix, second_matrix;

    CPHElement* first = pFirst_element;
    CPHElement* second = pSecond_element;
    VERIFY(first && second);
    first->GetGlobalTransformDynamic(&first_matrix);
    second->GetGlobalTransformDynamic(&second_matrix);
    pos.set(0, 0, 0);
    switch (vs_anchor)
    {
    case vs_first: first_matrix.transform_tiny(pos, anchor); break;
    case vs_second: second_matrix.transform_tiny(pos, anchor); break;
    case vs_global: pShell->mXFORM.transform_tiny(pos, anchor); break;
    default: NODEFAULT;
    }

    CharacterVirtualHandle b1 = body_for_joint(first);
    CharacterVirtualHandle b2 = body_for_joint(second);

    Fmatrix first_matrix_inv;
    first_matrix_inv.set(first_matrix);
    first_matrix_inv.invert();
    Fmatrix rotate;
    rotate.mul(first_matrix_inv, second_matrix);

    float lo0, hi0, lo1, hi1;
    axis0.set(0, 0, 0);
    axis1.set(0, 0, 0);

    CalcAxis(0, axis0, lo0, hi0, first_matrix, second_matrix);
    if (b1 == INVALID_CHARACTER_VIRTUAL_HANDLE) axis0.invert();

    CalcAxis(1, axis1, lo1, hi1, first_matrix, second_matrix);

    m_joint = GetPhysicsCore()->CreateJoint(hinge2, b1, b2, pos,
                                            axis0, axis1, Fvector().set(0,0,0),
                                            Fvector().set(lo0, lo1, 0), Fvector().set(hi0, hi1, 0));

    if (!(axes[0].force < 0.f)) GetPhysicsCore()->SetJointMotor(m_joint, 0, axes[0].force, axes[0].velocity);
    if (!(axes[1].force < 0.f)) GetPhysicsCore()->SetJointMotor(m_joint, 1, axes[1].force, axes[1].velocity);

    GetPhysicsCore()->SetJointSpringDamping(m_joint, -1, m_erp, m_cfm);
    GetPhysicsCore()->SetJointSpringDamping(m_joint, 0, axes[0].erp, axes[0].cfm);
}

void CPHJoint::CreateSlider()
{
    Fvector pos, axis0, axis1;
    Fmatrix first_matrix, second_matrix;

    CPHElement* first = pFirst_element;
    CPHElement* second = pSecond_element;

    VERIFY(first && second);
    first->GetGlobalTransformDynamic(&first_matrix);
    second->GetGlobalTransformDynamic(&second_matrix);
    CharacterVirtualHandle body1 = body_for_joint(first);
    CharacterVirtualHandle body2 = body_for_joint(second);

    pos.set(0, 0, 0);
    switch (vs_anchor)
    {
    case vs_first: first_matrix.transform_tiny(pos, anchor); break;
    case vs_second: second_matrix.transform_tiny(pos, anchor); break;
    case vs_global: pShell->mXFORM.transform_tiny(pos, anchor); break;
    default: NODEFAULT;
    }

    if (body1 != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        axes[0].vs = vs_first;
        axes[1].vs = vs_first;
    }
    else if (body2 != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        axes[0].vs = vs_second;
        axes[1].vs = vs_second;
    }

    Fmatrix first_matrix_inv;
    first_matrix_inv.set(first_matrix);
    first_matrix_inv.invert();
    Fmatrix rotate;
    rotate.mul(first_matrix_inv, second_matrix);

    float lo0, hi0, lo1, hi1;
    axis0.set(0, 0, 0);
    axis1.set(0, 0, 0);

    CalcAxis(0, axis0, lo0, hi0, first_matrix, second_matrix);
    CalcAxis(1, axis1, lo1, hi1, first_matrix, second_matrix);
    if (body1 == INVALID_CHARACTER_VIRTUAL_HANDLE) axis1.invert();

    Fvector slider_axis = {-axis0.x, -axis0.y, -axis0.z};
    m_joint = GetPhysicsCore()->CreateJoint(slider, body1, body2, pos,
                                            slider_axis, axis1, Fvector().set(0,0,0),
                                            Fvector().set(axes[0].low, lo1, 0), Fvector().set(axes[0].high, hi1, 0));

    if (!(axes[0].force < 0.f)) GetPhysicsCore()->SetJointMotor(m_joint, 0, axes[0].force, axes[0].velocity);
    if (!(axes[1].force < 0.f)) GetPhysicsCore()->SetJointMotor(m_joint, 1, axes[1].force, axes[1].velocity);

    GetPhysicsCore()->SetJointSpringDamping(m_joint, 0, axes[0].erp, axes[0].cfm);
    GetPhysicsCore()->SetJointSpringDamping(m_joint, 1, axes[1].erp, axes[1].cfm);
    GetPhysicsCore()->SetJointSpringDamping(m_joint, -1, m_erp, m_cfm);
}

void CPHJoint::CreateFullControl()
{
    Fvector pos, axis0, axis1, axis2;
    Fmatrix first_matrix, second_matrix;

    CPHElement* first = pFirst_element;
    CPHElement* second = pSecond_element;
    VERIFY(first && second);
    first->GetGlobalTransformDynamic(&first_matrix);
    second->GetGlobalTransformDynamic(&second_matrix);
    CharacterVirtualHandle body1 = body_for_joint(first);
    CharacterVirtualHandle body2 = body_for_joint(second);

    pos.set(0, 0, 0);
    switch (vs_anchor)
    {
    case vs_first: first_matrix.transform_tiny(pos, anchor); break;
    case vs_second: second_matrix.transform_tiny(pos, anchor); break;
    case vs_global: pShell->mXFORM.transform_tiny(pos, anchor); break;
    default: NODEFAULT;
    }

    Fmatrix first_matrix_inv;
    first_matrix_inv.set(first_matrix);
    first_matrix_inv.invert();
    Fmatrix rotate;
    rotate.mul(first_matrix_inv, second_matrix);

    float lo0, hi0, lo1, hi1, lo2, hi2;
    axis0.set(0, 0, 0); axis1.set(0, 0, 0); axis2.set(0, 0, 0);

    CalcAxis(0, axis0, lo0, hi0, first_matrix, second_matrix);
    if (body1 == INVALID_CHARACTER_VIRTUAL_HANDLE) axis0.invert();

    CalcAxis(1, axis1, lo1, hi1, first_matrix, second_matrix);
    if (body1 == INVALID_CHARACTER_VIRTUAL_HANDLE) axis1.invert();

    CalcAxis(2, axis2, lo2, hi2, first_matrix, second_matrix);
    if (body1 == INVALID_CHARACTER_VIRTUAL_HANDLE) axis2.invert();

    m_joint = GetPhysicsCore()->CreateJoint(full_control, body1, body2, pos,
                                            axis0, axis1, axis2,
                                            Fvector().set(lo0, lo1, lo2), Fvector().set(hi0, hi1, hi2));

    if (!(axes[0].force < 0.f)) GetPhysicsCore()->SetJointMotor(m_joint, 0, axes[0].force, axes[0].velocity);
    if (!(axes[1].force < 0.f)) GetPhysicsCore()->SetJointMotor(m_joint, 1, axes[1].force, axes[1].velocity);
    if (!(axes[2].force < 0.f)) GetPhysicsCore()->SetJointMotor(m_joint, 2, axes[2].force, axes[2].velocity);

    GetPhysicsCore()->SetJointSpringDamping(m_joint, 0, axes[0].erp, axes[0].cfm);
    GetPhysicsCore()->SetJointSpringDamping(m_joint, 1, axes[1].erp, axes[1].cfm);
    GetPhysicsCore()->SetJointSpringDamping(m_joint, 2, axes[2].erp, axes[2].cfm);
    GetPhysicsCore()->SetJointSpringDamping(m_joint, -1, m_erp, m_cfm);
}

void CPHJoint::SetAnchor(const float x, const float y, const float z)
{
    vs_anchor = vs_global;
    anchor.set(x, y, z);
}

void CPHJoint::SetAnchorVsFirstElement(const float x, const float y, const float z)
{
    vs_anchor = vs_first;
    anchor.set(x, y, z);
}

void CPHJoint::SetAnchorVsSecondElement(const float x, const float y, const float z)
{
    vs_anchor = vs_second;
    anchor.set(x, y, z);
}

void CPHJoint::SetAxisDir(const float x, const float y, const float z, const int axis_num)
{
    int ax = axis_num;
    LimitAxisNum(ax);
    VERIFY(-1 != ax);
    axes[ax].vs = vs_global;
    axes[ax].direction.set(x, y, z);

    SetAxisDirDynamic(axes[ax].direction, axis_num);
}

void CPHJoint::SetAxisDirDynamic(const Fvector& orientation, const int axis_num)
{
    VERIFY(axis_num >= 0 && axis_num <= 2);
    if (m_joint != INVALID_JOINT_HANDLE)
    {
        GetPhysicsCore()->SetJointAxisDir(m_joint, axis_num, orientation);
    }
}

void CPHJoint::SetAxisDirVsFirstElement(const float x, const float y, const float z, const int axis_num)
{
    int ax = axis_num;
    LimitAxisNum(ax);
    if (-1 == ax) return;
    axes[ax].vs = vs_first;
    axes[ax].direction.set(x, y, z);
}

void CPHJoint::SetAxisDirVsSecondElement(const float x, const float y, const float z, const int axis_num)
{
    int ax = axis_num;
    LimitAxisNum(ax);
    if (-1 == ax) return;
    axes[ax].vs = vs_second;
    axes[ax].direction.set(x, y, z);
}

void CPHJoint::SetLimits(const float low, const float high, const int axis_num)
{
    if (!(pFirst_element && pSecond_element)) return;

    int ax = axis_num;
    LimitAxisNum(ax);
    if (-1 == ax) return;

    Fvector axis;
    switch (axes[ax].vs)
    {
    case vs_first: pFirst_element->mXFORM.transform_dir(axis, axes[ax].direction); break;
    case vs_second: pSecond_element->mXFORM.transform_dir(axis, axes[ax].direction); break;
    case vs_global:
    default: axis.set(axes[ax].direction);
    }

    axes[ax].low = low;
    axes[ax].high = high;
    Fmatrix m1, m2;
    m1.set(pFirst_element->mXFORM);
    m1.invert();
    m2.mul(m1, pSecond_element->mXFORM);

    float zer;
    axis_angleA(m2, axes[ax].direction, zer);

    axes[ax].zero = zer;
    if (bActive)
        SetLimitsActive(axis_num);
}

CPHJoint::CPHJoint(CPhysicsJoint::enumType type, CPhysicsElement* first, CPhysicsElement* second)
{
    pShell = nullptr;
    m_bone_id = u16(-1);
    m_back_ref = nullptr;
    m_destroy_info = nullptr;
    pFirstGeom = nullptr;
    pFirst_element = cast_PHElement(first);
    pSecond_element = cast_PHElement(second);
    m_joint = INVALID_JOINT_HANDLE;
    eType = type;
    bActive = false;

    m_erp = world_erp;
    m_cfm = world_cfm;

    SPHAxis axis, axis2, axis3;
    axis2.set_direction(1, 0, 0);
    axis3.direction.crossproduct(axis.direction, axis2.direction);
    vs_anchor = vs_first;

    switch (eType)
    {
    case ball: break;
    case hinge: axes.push_back(axis); break;
    case hinge2:
        axes.push_back(axis);
        axes.push_back(axis2);
        break;
    case full_control:
        axes.push_back(axis);
        axes.push_back(axis2);
        axes.push_back(axis3);
        break;
    case slider:
        axes.push_back(axis);
        axes.push_back(axis);
        break;
    }
}

void CPHJoint::SetLimitsVsFirstElement(const float low, const float high, const int axis_num) {}
void CPHJoint::SetLimitsVsSecondElement(const float low, const float high, const int axis_num) {}

void CPHJoint::Create()
{
    if (bActive) return;
    switch (eType)
    {
    case ball: CreateBall(); break;
    case hinge: CreateHinge(); break;
    case hinge2: CreateHinge2(); break;
    case full_control: CreateFullControl(); break;
    case slider: CreateSlider(); break;
    }

    if (m_joint != INVALID_JOINT_HANDLE && (m_destroy_info ||
        (pFirst_element && pFirst_element->isBreakable()) || (pSecond_element && pSecond_element->isBreakable())))
    {
        GetPhysicsCore()->SetJointFeedback(m_joint,
            m_destroy_info ? m_destroy_info->JointFeedback() : &m_fracture_feedback);
    }
    bActive = true;
}

void CPHJoint::RunSimulation()
{

}

void CPHJoint::Activate()
{
    Create();
    RunSimulation();
}

void CPHJoint::Deactivate()
{
    if (!bActive) return;

    if (m_joint != INVALID_JOINT_HANDLE)
    {
        GetPhysicsCore()->DestroyJoint(m_joint);
        m_joint = INVALID_JOINT_HANDLE;
    }

    bActive = false;
}

void CPHJoint::ReattachFirstElement(CPHElement* new_element)
{
    Deactivate();
    pFirst_element = new_element;
    Activate();
}

void CPHJoint::SetForceAndVelocity(const float force, const float velocity, const int axis_num)
{
    if (pShell && pShell->isActive())
        pShell->Enable();
    SetForce(force, axis_num);
    SetVelocity(velocity, axis_num);
}

void CPHJoint::GetMaxForceAndVelocity(float& force, float& velocity, int axis_num)
{
    force = axes[axis_num].force;
    velocity = axes[axis_num].velocity;
}

void CPHJoint::SetForce(const float force, const int axis_num)
{
    int ax = axis_num;
    LimitAxisNum(ax);

    if (ax == -1)
    {
        for (size_t i = 0; i < axes.size(); ++i)
            axes[i].force = force;
    }
    else
    {
        axes[ax].force = force;
    }

    if (bActive) SetForceActive(ax);
}

void CPHJoint::SetForceActive(const int axis_num)
{
    if (m_joint == INVALID_JOINT_HANDLE) return;

    if (axis_num == -1) {
        for (size_t i = 0; i < axes.size(); ++i) {
            GetPhysicsCore()->SetJointMotor(m_joint, i, axes[i].force, axes[i].velocity);
        }
    } else {
        GetPhysicsCore()->SetJointMotor(m_joint, axis_num, axes[axis_num].force, axes[axis_num].velocity);
    }
}

void CPHJoint::SetVelocity(const float velocity, const int axis_num)
{
    int ax = axis_num;
    LimitAxisNum(ax);

    if (ax == -1)
    {
        for (size_t i = 0; i < axes.size(); ++i)
            axes[i].velocity = velocity;
    }
    else
    {
        axes[ax].velocity = velocity;
    }

    if (bActive) SetVelocityActive(ax);
}

void CPHJoint::SetVelocityActive(const int axis_num)
{
    if (m_joint == INVALID_JOINT_HANDLE) return;

    if (axis_num == -1) {
        for (size_t i = 0; i < axes.size(); ++i) {
            GetPhysicsCore()->SetJointMotor(m_joint, i, axes[i].force, axes[i].velocity);
        }
    } else {
        GetPhysicsCore()->SetJointMotor(m_joint, axis_num, axes[axis_num].force, axes[axis_num].velocity);
    }
}

void CPHJoint::SetLoLimitDynamic(int axis_num, float limit)
{
    VERIFY(axis_num >= 0 && axis_num <= 2);
    VERIFY(bActive);
    axes[axis_num].low = limit;
    if (m_joint != INVALID_JOINT_HANDLE)
    {
        float lo, hi;
        Fvector axis;
        Fmatrix first_matrix, second_matrix;
        pFirst_element->GetGlobalTransformDynamic(&first_matrix);
        pSecond_element->GetGlobalTransformDynamic(&second_matrix);
        CalcAxis(axis_num, axis, lo, hi, first_matrix, second_matrix);
        GetPhysicsCore()->SetJointLimits(m_joint, axis_num, lo, hi);
    }
}

void CPHJoint::SetHiLimitDynamic(int axis_num, float limit)
{
    VERIFY(axis_num >= 0 && axis_num <= 2);
    VERIFY(bActive);
    axes[axis_num].high = limit;
    if (m_joint != INVALID_JOINT_HANDLE)
    {
        float lo, hi;
        Fvector axis;
        Fmatrix first_matrix, second_matrix;
        pFirst_element->GetGlobalTransformDynamic(&first_matrix);
        pSecond_element->GetGlobalTransformDynamic(&second_matrix);
        CalcAxis(axis_num, axis, lo, hi, first_matrix, second_matrix);
        GetPhysicsCore()->SetJointLimits(m_joint, axis_num, lo, hi);
    }
}

void CPHJoint::SetLimitsActive(int axis_num)
{
    if (m_joint == INVALID_JOINT_HANDLE) return;

    Fmatrix first_matrix, second_matrix;
    if (pFirst_element && pSecond_element)
    {
        pFirst_element->GetGlobalTransformDynamic(&first_matrix);
        pSecond_element->GetGlobalTransformDynamic(&second_matrix);
    }

    if (axis_num == -1) {
        for (size_t i = 0; i < axes.size(); ++i) {
            float lo = axes[i].low, hi = axes[i].high;
            Fvector axis;
            if (pFirst_element && pSecond_element)
                CalcAxis((int)i, axis, lo, hi, first_matrix, second_matrix);
            GetPhysicsCore()->SetJointLimits(m_joint, (int)i, lo, hi);
        }
    } else {
        float lo = axes[axis_num].low, hi = axes[axis_num].high;
        Fvector axis;
        if (pFirst_element && pSecond_element)
            CalcAxis(axis_num, axis, lo, hi, first_matrix, second_matrix);
        GetPhysicsCore()->SetJointLimits(m_joint, axis_num, lo, hi);
    }
}

float CPHJoint::GetAxisAngleRate(int axis_num)
{
    VERIFY(axis_num >= 0 && axis_num <= 2);
    VERIFY(bActive);
    if (m_joint == INVALID_JOINT_HANDLE) return 0.f;
    return GetPhysicsCore()->GetJointAxisAngleRate(m_joint, axis_num);
}

float CPHJoint::GetAxisAngle(int axis_num)
{
    if (m_joint == INVALID_JOINT_HANDLE) return FLT_MAX;
    float angle = GetPhysicsCore()->GetJointAxisAngle(m_joint, axis_num);
    if (!pFirst_element || pFirst_element->isFixed() || !pFirst_element->has_geoms())
        angle = -angle;
    return angle;
}

void CPHJoint::LimitAxisNum(int& axis_num)
{
    if (axis_num < -1)
    {
        axis_num = -1;
        return;
    }

    switch (eType)
    {
    case ball: axis_num = -1; break;
    case hinge: axis_num = 0; break;
    case slider:
    case hinge2: axis_num = axis_num > 1 ? 1 : axis_num; break;
    case full_control: axis_num = axis_num > 2 ? 2 : axis_num; break;
    }
}

void CPHJoint::SetAxis(const SPHAxis& axis, const int axis_num)
{
    int ax = axis_num;
    LimitAxisNum(ax);
    if (ax == -1)
    {
        for (size_t i = 0; i < axes.size(); ++i)
            axes[i] = axis;
    }
    else
    {
        axes[ax] = axis;
    }
}

void CPHJoint::SetAxisSDfactors(float spring_factor, float damping_factor, int axis_num)
{
    int ax = axis_num;
    LimitAxisNum(ax);

    if (ax == -1)
    {
        for (size_t i = 0; i < axes.size(); ++i)
            axes[i].set_sd_factors(spring_factor, damping_factor, eType);

        if (bActive) SetLimitsSDfactorsActive();
    }
    else
    {
        axes[ax].set_sd_factors(spring_factor, damping_factor, eType);
        if (bActive) SetAxisSDfactorsActive(ax);
    }
}

void CPHJoint::SetJointSDfactors(float spring_factor, float damping_factor)
{
    switch (eType)
    {
    case hinge2:
        m_cfm = CFM(hinge2_spring * spring_factor, hinge2_damping * damping_factor);
        m_erp = ERP(hinge2_spring * spring_factor, hinge2_damping * damping_factor);
        break;
    case ball:;
    case hinge:;
    case full_control:;
    case slider:;
        m_erp = ERP(world_spring * spring_factor, world_damping * damping_factor);
        m_cfm = CFM(world_spring * spring_factor, world_damping * damping_factor);
        break;
    }
    if (bActive) SetJointSDfactorsActive();
}

void CPHJoint::SetJointSDfactorsActive()
{
    if (m_joint != INVALID_JOINT_HANDLE)
        GetPhysicsCore()->SetJointSpringDamping(m_joint, -1, m_erp, m_cfm);
}

void CPHJoint::SetLimitsSDfactorsActive()
{
    if (m_joint == INVALID_JOINT_HANDLE) return;
    for (size_t i = 0; i < axes.size(); ++i)
        GetPhysicsCore()->SetJointSpringDamping(m_joint, i, axes[i].erp, axes[i].cfm);
}

void CPHJoint::SetAxisSDfactorsActive(int axis_num)
{
    LimitAxisNum(axis_num);
    if (m_joint != INVALID_JOINT_HANDLE && axis_num >= 0 && axis_num < (int)axes.size())
        GetPhysicsCore()->SetJointSpringDamping(m_joint, axis_num, axes[axis_num].erp, axes[axis_num].cfm);
}

void CPHJoint::SetJointFudgefactorActive(float factor)
{
    VERIFY(bActive);
    if (m_joint != INVALID_JOINT_HANDLE)
        GetPhysicsCore()->SetJointFudgeFactor(m_joint, factor);
}

void CPHJoint::GetJointSDfactors(float& spring_factor, float& damping_factor)
{
    spring_factor = SPRING(m_cfm, m_erp);
    damping_factor = DAMPING(m_cfm, m_erp);
    if (eType == hinge2)
    {
        spring_factor /= hinge2_spring;
        damping_factor /= hinge2_damping;
    }
    else
    {
        spring_factor /= world_spring;
        damping_factor /= world_damping;
    }
}

void CPHJoint::GetAxisSDfactors(float& spring_factor, float& damping_factor, int axis_num)
{
    LimitAxisNum(axis_num);
    spring_factor = SPRING(axes[axis_num].cfm, axes[axis_num].erp) / world_spring;
    damping_factor = DAMPING(axes[axis_num].cfm, axes[axis_num].erp) / world_damping;
}

u16 CPHJoint::GetAxesNumber() { return u16(axes.size()); }

void CPHJoint::CalcAxis(int ax_num, Fvector& axis, float& lo, float& hi, const Fmatrix& first_matrix, const Fmatrix& second_matrix, const Fmatrix& rotate)
{
    switch (axes[ax_num].vs)
    {
    case vs_first: first_matrix.transform_dir(axis, axes[ax_num].direction); break;
    case vs_second: second_matrix.transform_dir(axis, axes[ax_num].direction); break;
    case vs_global: pShell->mXFORM.transform_dir(axis, axes[ax_num].direction); break;
    default: NODEFAULT;
    }
    lo = axes[ax_num].low;
    hi = axes[ax_num].high;
    if (lo < -float(M_PI))
    {
        lo = -float(M_PI);
    }
    if (hi > float(M_PI))
    {
        hi = float(M_PI);
    }
}

void CPHJoint::CalcAxis(int ax_num, Fvector& axis, float& lo, float& hi, const Fmatrix& first_matrix, const Fmatrix& second_matrix)
{
    switch (axes[ax_num].vs)
    {
    case vs_first: first_matrix.transform_dir(axis, axes[ax_num].direction); break;
    case vs_second: second_matrix.transform_dir(axis, axes[ax_num].direction); break;
    case vs_global: pShell->mXFORM.transform_dir(axis, axes[ax_num].direction); break;
    default: NODEFAULT;
    }

    Fmatrix inv_first_matrix;
    inv_first_matrix.set(first_matrix);
    inv_first_matrix.invert();

    Fmatrix rotate;
    rotate.mul(inv_first_matrix, second_matrix);

    float shift_angle;
    axis_angleA(rotate, axes[ax_num].direction, shift_angle);

    shift_angle -= axes[ax_num].zero;

    if (shift_angle > float(M_PI))
        shift_angle -= 2.f * float(M_PI);
    if (shift_angle < -float(M_PI))
        shift_angle += 2.f * float(M_PI);

    lo = axes[ax_num].low;
    hi = axes[ax_num].high;
    if (lo < -float(M_PI))
    {
        lo = -float(M_PI);
    }
    if (hi > float(M_PI))
    {
        hi = float(M_PI);
    }
}

void CPHJoint::GetLimits(float& lo_limit, float& hi_limit, int axis_num)
{
    LimitAxisNum(axis_num);
    if (pFirst_element && !pFirst_element->isFixed() && pFirst_element->has_geoms())
    {
        lo_limit = axes[axis_num].low;
        hi_limit = axes[axis_num].high;
    }
    else
    {
        lo_limit = -axes[axis_num].high;
        hi_limit = -axes[axis_num].low;
    }
}

void CPHJoint::GetAxisDir(int num, Fvector& axis, eVs& vs)
{
    LimitAxisNum(num);
    vs = axes[num].vs;
    axis.set(axes[num].direction);
}

void CPHJoint::GetAxisDirDynamic(int num, Fvector& axis)
{
    LimitAxisNum(num);
    if (m_joint != INVALID_JOINT_HANDLE)
        GetPhysicsCore()->GetJointAxisDir(m_joint, num, axis);
    else
        axis.set(0,0,0);
}

void CPHJoint::GetAnchorDynamic(Fvector& anchor)
{
    if (m_joint != INVALID_JOINT_HANDLE)
        GetPhysicsCore()->GetJointAnchor(m_joint, anchor);
    else
        anchor.set(0,0,0);
}

CPHJoint::SPHAxis::SPHAxis()
{
    high = FLT_MAX;
    low = -FLT_MAX;
    zero = 0.f;
    erp = world_erp;
    cfm = world_cfm;
    direction.set(0, 0, 1);
    vs = vs_first;
    force = 0.f;
    velocity = 0.f;
}

void CPHJoint::SPHAxis::set_sd_factors(float sf, float df, enumType jt)
{
    switch (jt)
    {
    case hinge2:
        cfm = 0.f;
        erp = 1.f;
        break;
    case ball:;
    case hinge:;
    case full_control:;
    case slider:;
        erp = ERP(world_spring * sf, world_damping * df);
        cfm = CFM(world_spring * sf, world_damping * df);
        break;
    }
}

CPhysicsElement* CPHJoint::PFirst_element() { return cast_PhysicsElement(pFirst_element); }
CPhysicsElement* CPHJoint::PSecond_element() { return cast_PhysicsElement(pSecond_element); }

void CPHJoint::SetBreakable(float force, float torque)
{
    if (!m_destroy_info)
        m_destroy_info = xr_new<CPHJointDestroyInfo>(force, torque);
}

void CPHJoint::SetShell(CPHShell* p)
{
    if (!m_joint || !pShell)
    {
        pShell = p;
        return;
    }
    if (pShell != p)
    {
        pShell = p;
    }
}

void CPHJoint::ClearDestroyInfo() { xr_delete(m_destroy_info); }
bool CPHJoint::IsWheelJoint() { return eType == hinge2; }
bool CPHJoint::IsHingeJoint() { return eType == hinge; }
