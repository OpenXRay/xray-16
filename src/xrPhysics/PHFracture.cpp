#include "stdafx.h"
#include "PHFracture.h"
#include "Physics.h"
#include "PHElement.h"
#include "PHShell.h"
#include "console_vars.h"

#include "Include/xrRender/Kinematics.h"
#include "ph_valid_ode.h"
#include "xrCore/Animation/Bone.hpp"

#pragma warning(push)
#pragma warning(disable : 4995)
#pragma warning(disable : 4267)
#include "ode/ode/src/joint.h"
#pragma warning(pop)

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
void CPHFracturesHolder::ApplyImpactsToElement(CPHElement* E)
{
    auto i = m_impacts.begin(), e = m_impacts.end();
    BOOL ac_state = E->isActive();
    // E->bActive=true;
    E->m_flags.set(CPHElement::flActive, TRUE);
    for (; e != i; ++i)
    {
        E->applyImpact(*i);
    }
    // E->bActive=ac_state;
    E->m_flags.set(CPHElement::flActive, ac_state);
}
element_fracture CPHFracturesHolder::SplitFromEnd(CPHElement* element, u16 fracture)
{
    FRACTURE_I fract_i = m_fractures.begin() + fracture;
    u16 geom_num = fract_i->m_start_geom_num;
    u16 end_geom_num = fract_i->m_end_geom_num;
    SubFractureMass(fracture);

    CPHElement* new_element = cast_PHElement(P_create_Element());
    new_element->m_SelfID = fract_i->m_bone_id;
    new_element->mXFORM.set(element->mXFORM);
    element->PassEndGeoms(geom_num, end_geom_num, new_element);
    /////////////////////////////////////////////
    IKinematics* pKinematics = element->m_shell->PKinematics();
    const CBoneInstance& new_bi = pKinematics->LL_GetBoneInstance(new_element->m_SelfID);
    const CBoneInstance& old_bi = pKinematics->LL_GetBoneInstance(element->m_SelfID);

    Fmatrix shift_pivot;
    shift_pivot.set(new_bi.mTransform);
    shift_pivot.invert();
    shift_pivot.mulB_43(old_bi.mTransform);
    /////////////////////////////////////////////
    float density = element->getDensity();
    new_element->SetShell(element->PHShell());
    Fmatrix current_transtform;
    element->GetGlobalTransformDynamic(&current_transtform);
    InitNewElement(new_element, shift_pivot, density);
    Fmatrix shell_form;
    element->PHShell()->GetGlobalTransformDynamic(&shell_form);
    current_transtform.mulA_43(shell_form);
    new_element->SetTransform(current_transtform, mh_unspecified);

    // dBodyID new_element_body=new_element->get_body();
    // dBodyAddForce(new_element_body,fract_i->m_pos_in_element[0],
    //									  fract_i->m_pos_in_element[1],
    //									  fract_i->m_pos_in_element[2]);
    ApplyImpactsToElement(new_element);

    // dBodyAddTorque(new_element->get_body(),fract_i->m_break_force,
    //									   fract_i->m_break_torque,
    //									   fract_i->m_add_torque_z);
    // BodyCutForce(new_element_body,default_l_limit,default_w_limit);
    element_fracture ret = std::make_pair(new_element, (CShellSplitInfo)(*fract_i));

    if (m_fractures.size() - fracture > 0)
    {
        if (new_element->m_fratures_holder == NULL) // create fractures holder if it was not created before
        {
            new_element->m_fratures_holder = new CPHFracturesHolder();
        }
        PassEndFractures(fracture, new_element);
    }

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

    for (; i != i_from; ++i) // correct end geoms for fractures leaved in source
    {
        u16& cur_end_geom = i->m_end_geom_num;
        if (cur_end_geom > begin_geom_num)
            cur_end_geom = cur_end_geom - passed_geoms;
    }

    i++; // omit used fracture;
    // these to be passed
    for (; i != e; i++) // itterate antil a fracture where geom num > end geom num
    {
        u16& cur_end_geom = i->m_end_geom_num;
        u16& cur_geom = i->m_start_geom_num;
        if (cur_geom >= end_geom)
            break;
        cur_end_geom = cur_end_geom - leaved_geoms;
        cur_geom = cur_geom - leaved_geoms;
    }
    FRACTURE_I i_to = i;
    for (; i != e; ++i) // correct data in the rest leaved fractures
    {
        u16& cur_end_geom = i->m_end_geom_num;
        u16& cur_geom = i->m_start_geom_num;
        cur_end_geom = cur_end_geom - passed_geoms;
        cur_geom = cur_geom - passed_geoms;
    }

    if (i_from + 1 != i_to) // insure it!!
    {
        CPHFracturesHolder*& dest_fract_holder = dest->m_fratures_holder;
        if (!dest_fract_holder)
            dest_fract_holder = new CPHFracturesHolder();
        // pass fractures not including end fracture
        dest_fract_holder->m_fractures.insert(dest_fract_holder->m_fractures.end(), i_from + 1, i_to);

        // u16 deb=u16(i_to-i_from-1);
        // deb++;deb--;
    }
    m_fractures.erase(i_from, i_to); // erase along whith used fracture
}
void CPHFracturesHolder::SplitProcess(CPHElement* element, ELEMENT_PAIR_VECTOR& new_elements)
{
    // FRACTURE_RI i=m_fractures.rbegin(),e=m_fractures.rend();//reversed
    u16 i = u16(m_fractures.size() - 1);

    for (; i != u16(-1); i--)
    {
        if (m_fractures[i].Breaked())
        {
            // float density = element->getDensity();
            new_elements.push_back(SplitFromEnd(element, i));
            // element->ResetMass(density);
        }
    }
}

void CPHFracturesHolder::InitNewElement(CPHElement* element, const Fmatrix& shift_pivot, float density)
{
    element->CreateSimulBase();
    element->ReInitDynamics(shift_pivot, density);
    VERIFY(dBodyStateValide(element->get_body()));
}

void CPHFracturesHolder::PhTune(dBodyID body)
{
    // iterate through all body's joints and set joints feedbacks where is not already set
    // contact feedbacks stored in global storage - ContactFeedBacks wich cleared on each step
    // breacable joints already has their feedbacks,
    // feedbacks for rest noncontact joints stored in m_feedbacks in runtime in this function and
    // and killed by destructor

    // int dBodyGetNumJoints (dBodyID b);
    // dJointID dBodyGetJoint (dBodyID, int index);
    // dJointGetType
    // dJointTypeContact

    int num = dBodyGetNumJoints(body);
    for (int i = 0; i < num; ++i)
    {
        dJointID joint = dBodyGetJoint(body, i);

        if (dJointGetType(joint) == dJointTypeContact)
        {
            dJointSetFeedback(joint, ContactFeedBacks.add());
        }
        else
        {
            CPHJoint* ph_joint = (CPHJoint*)dJointGetData(joint);
            if (!(ph_joint && ph_joint->JointDestroyInfo()))
                dJointSetFeedback(joint, ContactFeedBacks.add());
            // if(!dJointGetFeedback(joint))
            //{
            //	m_feedbacks.push_back(dJointFeedback());
            //	dJointSetFeedback(joint,&m_feedbacks.back());
            //}
        }
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
void CPHFracturesHolder::DistributeAdditionalMass(u16 geom_num, const dMass& m)
{
    FRACTURE_I f_i = m_fractures.begin(), f_e = m_fractures.end();
    for (; f_i != f_e; ++f_i)
    {
        R_ASSERT2(u16(-1) != f_i->m_start_geom_num, "fracture does not initialized!");

        if (f_i->m_end_geom_num == u16(-1))
            f_i->MassAddToSecond(m);
        else
            f_i->MassAddToFirst(m);

        // f_i->MassAddToFirst(m);
    }
}
void CPHFracturesHolder::SubFractureMass(u16 fracture_num)
{
    FRACTURE_I f_i = m_fractures.begin(), f_e = m_fractures.end();
    FRACTURE_I fracture = f_i + fracture_num;
    u16 start_geom = fracture->m_start_geom_num;
    u16 end_geom = fracture->m_end_geom_num;
    dMass& second_mass = fracture->m_secondM;
    dMass& first_mass = fracture->m_firstM;
    for (; f_i != f_e; ++f_i)
    {
        if (f_i == fracture)
            continue;
        R_ASSERT2(start_geom != f_i->m_start_geom_num, "Double fracture!!!");

        if (start_geom > f_i->m_start_geom_num)
        {
            if (end_geom <= f_i->m_end_geom_num)
                f_i->MassSubFromSecond(second_mass); // tag fracture is in current
            else
            {
                R_ASSERT2(start_geom >= f_i->m_end_geom_num, "Odd fracture!!!");
                f_i->MassSubFromFirst(second_mass); // tag fracture is ouside current
            }
        }
        else
        {
            if (end_geom >= f_i->m_end_geom_num)
                f_i->MassSubFromFirst(first_mass); // current fracture is in tag
            else
            {
                R_ASSERT2(end_geom <= f_i->m_start_geom_num, "Odd fracture!!!");
                f_i->MassSubFromFirst(second_mass); // tag fracture is ouside current
            }
        }
    }
}

CPHFracture::CPHFracture()
{
    // m_bone_id=bone_id;
    // m_position.set(position);
    // m_direction.set(direction);
    // m_break_force=break_force;
    // m_break_torque=break_torque;
    m_start_geom_num = u16(-1);
    m_end_geom_num = u16(-1);
    m_breaked = false;
}

//#define DBG_BREAK
bool CPHFracture::Update(CPHElement* element)
{
    ////itterate through impacts & calculate
    dBodyID body = element->get_body();
    // const Fvector& v_bodyvel=*((Fvector*)dBodyGetLinearVel(body));
    CPHFracturesHolder* holder = element->FracturesHolder();
    PH_IMPACT_STORAGE& impacts = holder->Impacts();

    Fvector second_part_force, first_part_force, second_part_torque, first_part_torque;
    second_part_force.set(0.f, 0.f, 0.f);
    first_part_force.set(0.f, 0.f, 0.f);
    second_part_torque.set(0.f, 0.f, 0.f);
    first_part_torque.set(0.f, 0.f, 0.f);

    // const Fvector& body_local_pos=element->local_mass_Center();
    const Fvector& body_global_pos = *(const Fvector*)dBodyGetPosition(body);
    Fvector body_to_first, body_to_second;
    body_to_first.set(*((const Fvector*)m_firstM.c)); //,body_local_pos
    body_to_second.set(*((const Fvector*)m_secondM.c)); //,body_local_pos
    // float body_to_first_smag=body_to_first.square_magnitude();
    // float body_to_second_smag=body_to_second.square_magnitude();
    int num = dBodyGetNumJoints(body);
    for (int i = 0; i < num; i++)
    {
        bool applied_to_second = false;
        dJointID joint = dBodyGetJoint(body, i);
        dJointFeedback* feedback = dJointGetFeedback(joint);
        VERIFY2(feedback, "Feedback was not set!!!");
        dxJoint* b_joint = (dxJoint*)joint;
        bool b_body_second = (b_joint->node[1].body == body);
        Fvector joint_position;
        if (dJointGetType(joint) == dJointTypeContact)
        {
            dxJointContact* c_joint = (dxJointContact*)joint;
            dGeomID first_geom = c_joint->contact.geom.g1;
            dGeomID second_geom = c_joint->contact.geom.g2;
            joint_position.set(*(Fvector*)c_joint->contact.geom.pos);
            if (dGeomGetClass(first_geom) == dGeomTransformClass)
            {
                first_geom = dGeomTransformGetGeom(first_geom);
            }
            if (dGeomGetClass(second_geom) == dGeomTransformClass)
            {
                second_geom = dGeomTransformGetGeom(second_geom);
            }
            dxGeomUserData* UserData;
            UserData = dGeomGetUserData(first_geom);
            if (UserData)
            {
                u16 el_position = UserData->element_position;
                // define if the contact applied to second part;
                if (el_position < element->numberOfGeoms() && el_position >= m_start_geom_num &&
                    el_position < m_end_geom_num && first_geom == element->Geom(el_position)->geometry())
                    applied_to_second = true;
            }
            UserData = dGeomGetUserData(second_geom);
            if (UserData)
            {
                u16 el_position = UserData->element_position;
                if (el_position < element->numberOfGeoms() && el_position >= m_start_geom_num &&
                    el_position < m_end_geom_num && second_geom == element->Geom(el_position)->geometry())
                    applied_to_second = true;
            }
        }
        else
        {
            CPHJoint* J = (CPHJoint*)dJointGetData(joint);
            if (!J)
                continue; // hack..
            J->PSecondElement()->InterpolateGlobalPosition(&joint_position);
            CODEGeom* root_geom = J->RootGeom();
            if (root_geom)
            {
                u16 el_position = root_geom->element_position();
                if (element == J->PFirst_element() && el_position < element->numberOfGeoms() &&
                    el_position >= m_start_geom_num && el_position < m_end_geom_num)
                    applied_to_second = true;
            }
        }
        // accomulate forces applied by joints to first and second parts
        Fvector body_to_joint;
        body_to_joint.sub(joint_position, body_global_pos);
        if (applied_to_second)
        {
            Fvector shoulder;
            shoulder.sub(body_to_joint, body_to_second);
            if (b_body_second)
            {
                Fvector joint_force;
                joint_force.set(*(const Fvector*)feedback->f2);
                second_part_force.add(joint_force);
                Fvector torque;
                torque.crossproduct(shoulder, joint_force);
                second_part_torque.add(torque);
            }
            else
            {
                Fvector joint_force;
                joint_force.set(*(const Fvector*)feedback->f1);
                second_part_force.add(joint_force);

                Fvector torque;
                torque.crossproduct(shoulder, joint_force);
                second_part_torque.add(torque);
            }
        }
        else
        {
            Fvector shoulder;
            shoulder.sub(body_to_joint, body_to_first);
            if (b_body_second)
            {
                Fvector joint_force;
                joint_force.set(*(const Fvector*)feedback->f2);
                first_part_force.add(joint_force);
                Fvector torque;
                torque.crossproduct(shoulder, joint_force);
                first_part_torque.add(torque);
            }
            else
            {
                Fvector joint_force;
                joint_force.set(*(const Fvector*)feedback->f1);
                first_part_force.add(joint_force);
                Fvector torque;
                torque.crossproduct(shoulder, joint_force);
                first_part_torque.add(torque);
            }
        }
    }

    auto i_i = impacts.begin(), i_e = impacts.end();
    for (; i_i != i_e; i_i++)
    {
        u16 geom = i_i->geom;

        if ((geom >= m_start_geom_num && geom < m_end_geom_num))
        {
            Fvector force;
            force.set(i_i->force);
            force.mul(ph_console::phRigidBreakWeaponFactor);
            Fvector second_to_point;
            second_to_point.sub(body_to_second, i_i->point);
            // force.mul(30.f);
            second_part_force.add(force);
            Fvector torque;
            torque.crossproduct(second_to_point, force);
            second_part_torque.add(torque);
        }
        else
        {
            Fvector force;
            force.set(i_i->force);
            Fvector first_to_point;
            first_to_point.sub(body_to_first, i_i->point);
            // force.mul(4.f);
            first_part_force.add(force);
            Fvector torque;
            torque.crossproduct(first_to_point, force);
            second_part_torque.add(torque);
        }
    }
    Fvector gravity_force;
    gravity_force.set(0.f, -ph_world->Gravity() * m_firstM.mass, 0.f);
    first_part_force.add(gravity_force);
    second_part_force.add(gravity_force);
    dMatrix3 glI1, glI2, glInvI, tmp;

    // compute inertia tensors in global frame
    dMULTIPLY2_333(tmp, body->invI, body->posr.R);
    dMULTIPLY0_333(glInvI, body->posr.R, tmp);

    dMULTIPLY2_333(tmp, m_firstM.I, body->posr.R);
    dMULTIPLY0_333(glI1, body->posr.R, tmp);

    dMULTIPLY2_333(tmp, m_secondM.I, body->posr.R);
    dMULTIPLY0_333(glI2, body->posr.R, tmp);
    // both parts have equal start angular vel same as have body so we ignore it

    // compute breaking torque
    /// break_torque=glI2*glInvI*first_part_torque-glI1*glInvI*second_part_torque+crossproduct(second_in_bone,second_part_force)-crossproduct(first_in_bone,first_part_force)
    Fvector break_torque, vtemp;

    dMULTIPLY0_331((float*)&break_torque, glInvI, (float*)&first_part_torque);
    dMULTIPLY0_331((float*)&break_torque, glI2, (float*)&break_torque);

    dMULTIPLY0_331((float*)&vtemp, glInvI, (float*)&second_part_torque);
    dMULTIPLY0_331((float*)&vtemp, glI1, (float*)&vtemp);
    break_torque.sub(vtemp);

// Fvector first_in_bone,second_in_bone;
// first_in_bone.sub(*((const Fvector*)m_firstM.c),m_pos_in_element);
// second_in_bone.sub(*((const Fvector*)m_secondM.c),m_pos_in_element);

// vtemp.crossproduct(second_in_bone,second_part_force);
// break_torque.add(vtemp);
// vtemp.crossproduct(first_in_bone,first_part_force);
// break_torque.sub(vtemp);
#ifdef DBG_BREAK
    float btm_dbg = break_torque.magnitude() * ph_console::phBreakCommonFactor / torque_factor;
#endif
    if (break_torque.magnitude() * ph_console::phBreakCommonFactor > m_break_torque * torque_factor)
    {
        // m_break_torque.set(second_part_torque);
        m_pos_in_element.set(second_part_force);
        m_break_force = second_part_torque.x;
        m_break_torque = second_part_torque.y;
        m_add_torque_z = second_part_torque.z;
        m_breaked = true;
#ifndef DBG_BREAK
        return m_breaked;
#endif
    }

    Fvector break_force; //=1/(m1+m2)*(F1*m2-F2*m1)+r2xT2/(r2^2)-r1xT1/(r1^2)
    break_force.set(first_part_force);
    break_force.mul(m_secondM.mass);
    vtemp.set(second_part_force);
    vtemp.mul(m_firstM.mass);
    break_force.sub(vtemp);
    break_force.mul(1.f / element->getMass()); // element->getMass()//body->mass.mass

    // vtemp.crossproduct(second_in_bone,second_part_torque);
    // break_force.add(vtemp);
    // vtemp.crossproduct(first_in_bone,first_part_torque);
    // break_force.sub(vtemp);

    float bfm = break_force.magnitude() * ph_console::phBreakCommonFactor;

    if (m_break_force < bfm)
    {
        second_part_force.mul(bfm / m_break_force);
        m_pos_in_element.set(second_part_force);

        // m_pos_in_element.add(break_force);
        m_break_force = second_part_torque.x;
        m_break_torque = second_part_torque.y;
        m_add_torque_z = second_part_torque.z;
        m_breaked = true;
#ifndef DBG_BREAK
        return m_breaked;
#endif
    }
#ifdef DBG_BREAK
    Msg("bone_id %d break_torque - %f(max %f) break_force %f (max %f) breaked %d", m_bone_id, btm_dbg, m_break_torque,
        bfm, m_break_force, m_breaked);
#endif
    return m_breaked;
}

void CPHFracture::SetMassParts(const dMass& first, const dMass& second)
{
    m_firstM = first;
    m_secondM = second;
}

void CPHFracture::MassAddToFirst(const dMass& m) { dMassAdd(&m_firstM, &m); }
void CPHFracture::MassAddToSecond(const dMass& m) { dMassAdd(&m_secondM, &m); }
void CPHFracture::MassSubFromFirst(const dMass& m) { dMassSub(&m_firstM, &m); }
void CPHFracture::MassSubFromSecond(const dMass& m) { dMassSub(&m_secondM, &m); }
void CPHFracture::MassSetFirst(const dMass& m) { m_firstM = m; }
void CPHFracture::MassSetSecond(const dMass& m) { m_secondM = m; }
void CPHFracture::MassUnsplitFromFirstToSecond(const dMass& m)
{
    dMassSub(&m_firstM, &m);
    dMassAdd(&m_secondM, &m);
}
void CPHFracture::MassSetZerro()
{
    dMassSetZero(&m_firstM);
    dMassSetZero(&m_secondM);
}
