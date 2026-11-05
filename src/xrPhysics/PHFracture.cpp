#include "StdAfx.h"
#include "PHFracture.h"
#include "Physics.h"
#include "PHElement.h"
#include "PHShell.h"
#include "console_vars.h"
#include "PHJointDestroyInfo.h"
#include "PHJoint.h"
#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrPhysicsCore/IPhysicsCore.h"

extern class CPHWorld* ph_world;
static const float torque_factor = 10000000.f;

CPHFracturesHolder::CPHFracturesHolder() { m_has_breaks = false; }

CPHFracturesHolder::~CPHFracturesHolder()
{
    m_has_breaks = false;
    m_fractures.clear();
    m_impacts.clear();
    m_feedbacks.clear();
}

element_fracture CPHFracturesHolder::SplitFromEnd(CPHElement* element, u16 fracture)
{
    FRACTURE_I fract_i = m_fractures.begin() + fracture;
    const u16 geom_num = fract_i->m_start_geom_num;
    const u16 end_geom_num = fract_i->m_end_geom_num;
    const CShellSplitInfo splitInfo = *fract_i;
    auto* core = GetPhysicsCore();
    Fmatrix oldBody, oldPivot;
    core->GetBodyTransform(element->get_body(), oldBody);
    element->GetGlobalTransformDynamic(&oldPivot);
    Fvector firstCenter, secondCenter, angularVelocity;
    const auto firstMass = element->CalculateMassProperties(geom_num, end_geom_num, false, firstCenter);
    const auto secondMass = element->CalculateMassProperties(geom_num, end_geom_num, true, secondCenter);
    R_ASSERT(firstMass.mass > 0 && secondMass.mass > 0);
    Fvector firstPosition, secondPosition, firstVelocity, secondVelocity;
    oldBody.transform_tiny(firstPosition, firstCenter);
    oldBody.transform_tiny(secondPosition, secondCenter);
    core->GetBodyPointVelocity(element->get_body(), firstPosition, firstVelocity);
    core->GetBodyPointVelocity(element->get_body(), secondPosition, secondVelocity);
    core->GetBodyAngularVelocity(element->get_body(), angularVelocity);
    xr_vector<Fmatrix> worldGeometry;
    for (auto* geometry : element->m_geoms) {
        Fmatrix world;
        geometry->get_xform(world);
        worldGeometry.push_back(world);
    }
    SubFractureMass(fracture);

    CPHElement* new_element = cast_PHElement(P_create_Element());
    new_element->m_SelfID = fract_i->m_bone_id;
    new_element->mXFORM.set(element->mXFORM);
    new_element->m_mass = secondMass.mass;
    new_element->m_bone_masses = element->m_bone_masses;
    new_element->m_l_limit = element->m_l_limit;
    new_element->m_w_limit = element->m_w_limit;
    new_element->m_l_scale = element->m_l_scale;
    new_element->m_w_scale = element->m_w_scale;
    new_element->k_l = element->k_l;
    new_element->k_w = element->k_w;
    element->PassEndGeoms(geom_num, end_geom_num, new_element);

    Fmatrix shift_pivot = Fidentity;
    if (auto* kinematics = element->m_shell->PKinematics()) {
        shift_pivot.invert(kinematics->LL_GetBoneInstance(new_element->m_SelfID).mTransform);
        shift_pivot.mulB_43(kinematics->LL_GetBoneInstance(element->m_SelfID).mTransform);
    }
    new_element->m_mass_center.add(element->m_mass_center, secondCenter);
    shift_pivot.transform_tiny(new_element->m_mass_center);
    for (auto* geometry : new_element->m_geoms) geometry->move_local_basis(shift_pivot);
    new_element->SetShell(element->PHShell());
    new_element->CreateSimulBase();
    Fmatrix inverseShift, newBody;
    inverseShift.invert(shift_pivot);
    newBody.mul_43(oldPivot, inverseShift);
    newBody.c = secondPosition;
    Fmatrix firstBody = oldBody;
    firstBody.c = firstPosition;
    element->m_mass_center.add(firstCenter);
    element->m_mass = firstMass.mass;
    auto configure = [&](CPHElement* part, const Fmatrix& body, bool second) {
        Fmatrix inverseBody;
        inverseBody.invert(body);
        for (u16 index = 0; index < part->numberOfGeoms(); ++index) {
            auto* geometry = part->m_geoms[index];
            const u16 original = second ? geom_num + index : (index < geom_num ? index : index + end_geom_num - geom_num);
            geometry->runtime_local.mul_43(inverseBody, worldGeometry[original]);
            geometry->has_runtime_local = true;
            geometry->set_build_position(part->m_mass_center);
        }
        part->set_body(part->get_body());
        part->calc_volume_data();
        part->RebuildNativeShape();
        if (!second) core->RecenterBody(part->get_body(), firstCenter);
        core->SetBodyTransform(part->get_body(), body);
        Fvector center;
        part->SetNativeMassProperties(part->CalculateMassProperties(0, part->numberOfGeoms(), true, center));
        core->SetBodyGravityFactor(part->get_body(), core->GetBodyGravityFactor(element->get_body()));
        core->SetBodyDamping(part->get_body(), 0, 0);
        core->SetBodyLinearVelocity(part->get_body(), second ? secondVelocity : firstVelocity);
        core->SetBodyAngularVelocity(part->get_body(), angularVelocity);
        part->m_char_handle_interpolation.SetBody(part->get_body());
        part->FillInterpolation();
    };
    configure(element, firstBody, false);
    configure(new_element, newBody, true);
    // The impact has already changed the source body's velocities. Both pieces
    // inherit that motion at their own centers; replaying it would add it twice.
    element_fracture ret = std::make_pair(new_element, splitInfo);

    PassEndFractures(fracture, new_element);
    core->SetBodyContactFeedback(new_element->get_body(), new_element->isBreakable());
    if (element->isActive()) new_element->RunSimulation();

    return ret;
}

void CPHFracturesHolder::PassEndFractures(u16 from, CPHElement* dest)
{
    FRACTURE_I i = m_fractures.begin(), i_from = m_fractures.begin() + from, e = m_fractures.end();
    u16 end_geom = i_from->m_end_geom_num;
    u16 begin_geom_num = i_from->m_start_geom_num;
    u16 leaved_geoms = begin_geom_num;
    u16 passed_geoms = end_geom - begin_geom_num;
    if (i_from == e)
        return;

    for (; i != i_from; ++i)
    {
        u16& cur_end_geom = i->m_end_geom_num;
        if (cur_end_geom > begin_geom_num)
            cur_end_geom = cur_end_geom - passed_geoms;
    }

    ++i;
    for (; i != e; ++i)
    {
        u16& cur_end_geom = i->m_end_geom_num;
        u16& cur_geom = i->m_start_geom_num;
        if (cur_geom >= end_geom)
            break;
        cur_end_geom = cur_end_geom - leaved_geoms;
        cur_geom = cur_geom - leaved_geoms;
    }
    FRACTURE_I i_to = i;
    for (; i != e; ++i)
    {
        u16& cur_end_geom = i->m_end_geom_num;
        u16& cur_geom = i->m_start_geom_num;
        cur_end_geom = cur_end_geom - passed_geoms;
        cur_geom = cur_geom - passed_geoms;
    }

    if (i_from + 1 != i_to)
    {
        CPHFracturesHolder*& dest_fract_holder = dest->m_fratures_holder;
        if (!dest_fract_holder)
            dest_fract_holder = xr_new<CPHFracturesHolder>();
        dest_fract_holder->m_fractures.insert(dest_fract_holder->m_fractures.end(), i_from + 1, i_to);
    }
    m_fractures.erase(i_from, i_to);
}

void CPHFracturesHolder::SplitProcess(CPHElement* element, ELEMENT_PAIR_VECTOR& new_elements)
{
    u16 i = u16(m_fractures.size() - 1);

    for (; i != u16(-1); i--)
    {
        if (m_fractures[i].Breaked())
        {
            new_elements.push_back(SplitFromEnd(element, i));
        }
    }
    // These impacts were integrated before splitting and tested against every
    // fracture. Reusing them on the following step would fracture a piece twice.
    m_impacts.clear();
    m_has_breaks = false;
}

void CPHFracturesHolder::PhTune(CharacterVirtualHandle body)
{
    GetPhysicsCore()->SetBodyContactFeedback(body, true);
    auto* element = static_cast<CPHElement*>(GetPhysicsCore()->GetBodyUserData(body));
    if (!element || !element->PHShell()) return;
    auto* shell = element->PHShell();
    for (u16 index = 0; index < shell->get_JointsNumber(); ++index) {
        auto* joint = static_cast<CPHJoint*>(shell->get_JointByStoreOrder(index));
        if (joint->PFirstElement() == element || joint->PSecondElement() == element) joint->EnableFractureFeedback();
    }
}

bool CPHFracturesHolder::PhDataUpdate(CPHElement* element)
{
    FRACTURE_I i = m_fractures.begin(), e = m_fractures.end();
    for (; i != e; ++i)
    {
        m_has_breaks = i->Update(element) || m_has_breaks;
    }
    if (!m_has_breaks)
        m_impacts.clear();
    return m_has_breaks;
}

void CPHFracturesHolder::AddImpact(const Fvector& force, const Fvector& point, u16 id)
{
    m_impacts.push_back(SPHImpact(force, point, id));
}

u16 CPHFracturesHolder::AddFracture(const CPHFracture& fracture)
{
    m_fractures.push_back(fracture);
    return u16(m_fractures.size() - 1);
}

CPHFracture& CPHFracturesHolder::Fracture(u16 num)
{
    R_ASSERT2(num < m_fractures.size(), "out of range!");
    return m_fractures[num];
}

void CPHFracturesHolder::DistributeAdditionalMass(u16 geom_num, float m)
{
    FRACTURE_I f_i = m_fractures.begin(), f_e = m_fractures.end();
    for (; f_i != f_e; ++f_i)
    {
        R_ASSERT2(u16(-1) != f_i->m_start_geom_num, "fracture does not initialized!");

        if (geom_num >= f_i->m_start_geom_num && geom_num < f_i->m_end_geom_num)
            f_i->MassAddToSecond(m);
        else
            f_i->MassAddToFirst(m);
    }
}

void CPHFracturesHolder::SubFractureMass(u16 fracture_num)
{
    FRACTURE_I f_i = m_fractures.begin(), f_e = m_fractures.end();
    FRACTURE_I fracture = f_i + fracture_num;
    u16 start_geom = fracture->m_start_geom_num;
    u16 end_geom = fracture->m_end_geom_num;
    float second_mass = fracture->m_secondM;
    float first_mass = fracture->m_firstM;

    for (; f_i != f_e; ++f_i)
    {
        if (f_i == fracture)
            continue;
        R_ASSERT2(start_geom != f_i->m_start_geom_num, "Double fracture!!!");

        if (start_geom > f_i->m_start_geom_num)
        {
            if (end_geom <= f_i->m_end_geom_num)
                f_i->MassSubFromSecond(second_mass);
            else
            {
                R_ASSERT2(start_geom >= f_i->m_end_geom_num, "Odd fracture!!!");
                f_i->MassSubFromFirst(second_mass);
            }
        }
        else
        {
            if (end_geom >= f_i->m_end_geom_num)
                f_i->MassSubFromFirst(first_mass);
            else
            {
                R_ASSERT2(end_geom <= f_i->m_start_geom_num, "Odd fracture!!!");
                f_i->MassSubFromFirst(second_mass);
            }
        }
    }
}

CPHFracture::CPHFracture()
{
    m_start_geom_num = u16(-1);
    m_end_geom_num = u16(-1);
    m_breaked = false;
    m_firstM = 0.f;
    m_secondM = 0.f;
    m_break_force = 0.f;
    m_break_torque = 0.f;
    m_add_torque_z = 0.f;
}

bool CPHFracture::Update(CPHElement* element)
{
    if (m_breaked) return true;
    const auto body = element->get_body();
    if (body == INVALID_BODY_HANDLE) return false;
    if (m_cached_revision != element->NativeShapeRevision() || m_cached_geometry_count != element->numberOfGeoms() ||
        m_cached_mass != element->getMass() || !m_cached_center.similar(element->local_mass_Center(), EPS_S)) {
        m_first_properties = element->CalculateMassProperties(m_start_geom_num, m_end_geom_num, false, m_first_center);
        m_second_properties = element->CalculateMassProperties(m_start_geom_num, m_end_geom_num, true, m_second_center);
        m_firstM = m_first_properties.mass;
        m_secondM = m_second_properties.mass;
        m_cached_revision = element->NativeShapeRevision();
        m_cached_geometry_count = element->numberOfGeoms();
        m_cached_mass = element->getMass();
        m_cached_center = element->local_mass_Center();
    }
    if (m_firstM <= 0 || m_secondM <= 0) return false;
    Fmatrix transform;
    GetPhysicsCore()->GetBodyTransform(body, transform);
    Fvector center1, center2;
    transform.transform_tiny(center1, m_first_center);
    transform.transform_tiny(center2, m_second_center);
    Fvector first_part_force = {0, 0, 0}, second_part_force = {0, 0, 0};
    Fvector first_part_torque = {0, 0, 0}, second_part_torque = {0, 0, 0};
    auto inSecond = [this](u16 geometry) { return geometry >= m_start_geom_num && geometry < m_end_geom_num; };
    auto accumulate = [&](bool second, const Fvector& position, const Fvector& force, const Fvector& couple) {
        Fvector lever, torque;
        lever.sub(position, second ? center2 : center1);
        torque.crossproduct(lever, force);
        torque.add(couple);
        (second ? second_part_force : first_part_force).add(force);
        (second ? second_part_torque : first_part_torque).add(torque);
    };
    for (const auto& contact : GetPhysicsCore()->GetBodyContactForces(body))
        accumulate(inSecond(contact.geometry), contact.position, contact.force, contact.torque);
    auto* shell = element->PHShell();
    for (u16 index = 0; index < shell->get_JointsNumber(); ++index) {
        auto* joint = static_cast<CPHJoint*>(shell->get_JointByStoreOrder(index));
        const bool first = joint->PFirstElement() == element;
        if (!first && joint->PSecondElement() != element) continue;
        if (joint->GetJointHandle() == INVALID_JOINT_HANDLE) continue;
        const auto& feedback = joint->FractureFeedback();
        const auto& force = first ? feedback.f1 : feedback.f2;
        const auto& torque = first ? feedback.t1 : feedback.t2;
        Fvector position, lever, moment, couple;
        joint->GetAnchorDynamic(position);
        lever.sub(position, transform.c);
        moment.crossproduct(lever, force);
        couple.sub(torque, moment);
        const bool secondPart = first && joint->RootGeom() && inSecond(joint->RootGeom()->element_position());
        accumulate(secondPart, position, force, couple);
    }
    const Fvector zero = {0, 0, 0};
    for (const auto& impact : element->FracturesHolder()->Impacts()) {
        Fvector position, force = impact.force;
        transform.transform_tiny(position, impact.point);
        if (inSecond(impact.geom)) force.mul(ph_console::phRigidBreakWeaponFactor);
        accumulate(inSecond(impact.geom), position, force, zero);
    }
    const float gravity = element->get_ApplyByGravity() ? ph_world->Gravity() : 0;
    first_part_force.y -= gravity * m_firstM;
    second_part_force.y -= gravity * m_secondM;
    // Evaluate tensors and torques in body coordinates; the resulting magnitude
    // is invariant under the body's world rotation.
    Fmatrix inverseRotation;
    inverseRotation.transpose(transform);
    Fvector torque1, torque2, acceleration1, acceleration2, break_torque, temporary;
    inverseRotation.transform_dir(torque1, first_part_torque);
    inverseRotation.transform_dir(torque2, second_part_torque);
    Fmatrix inverseInertia;
    if (!inverseInertia.invert_b(element->NativeMassProperties().inertia)) return false;
    inverseInertia.transform_dir(acceleration1, torque1);
    inverseInertia.transform_dir(acceleration2, torque2);
    m_second_properties.inertia.transform_dir(break_torque, acceleration1);
    m_first_properties.inertia.transform_dir(temporary, acceleration2);
    break_torque.sub(temporary);
    if (break_torque.magnitude() * ph_console::phBreakCommonFactor > m_break_torque * torque_factor)
    {
        m_pos_in_element.set(second_part_force);
        m_break_force = second_part_torque.x;
        m_break_torque = second_part_torque.y;
        m_add_torque_z = second_part_torque.z;
        m_breaked = true;
        return m_breaked;
    }

    Fvector break_force;
    break_force.set(first_part_force);
    break_force.mul(m_secondM);
    Fvector vtemp;
    vtemp.set(second_part_force);
    vtemp.mul(m_firstM);
    break_force.sub(vtemp);

    break_force.mul(1.f / (m_firstM + m_secondM));

    float bfm = break_force.magnitude() * ph_console::phBreakCommonFactor;

    if (m_break_force < bfm && m_break_force > 0.f)
    {
        second_part_force.mul(bfm / m_break_force);
        m_pos_in_element.set(second_part_force);
        m_break_force = second_part_torque.x;
        m_break_torque = second_part_torque.y;
        m_add_torque_z = second_part_torque.z;
        m_breaked = true;
        return m_breaked;
    }

    return m_breaked;
}

void CPHFracture::SetMassParts(float first, float second)
{
    m_firstM = first;
    m_secondM = second;
}

void CPHFracture::MassAddToFirst(float m) { m_firstM += m; }
void CPHFracture::MassAddToSecond(float m) { m_secondM += m; }
void CPHFracture::MassSubFromFirst(float m) { m_firstM -= m; }
void CPHFracture::MassSubFromSecond(float m) { m_secondM -= m; }
void CPHFracture::MassSetFirst(float m) { m_firstM = m; }
void CPHFracture::MassSetSecond(float m) { m_secondM = m; }
void CPHFracture::MassUnsplitFromFirstToSecond(float m)
{
    m_firstM -= m;
    m_secondM += m;
}
void CPHFracture::MassSetZerro()
{
    m_firstM = 0.f;
    m_secondM = 0.f;
}
