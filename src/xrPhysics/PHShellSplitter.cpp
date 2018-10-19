#include "StdAfx.h"
#include "Physics.h"
#include "PHShell.h"
#include "PHShellSplitter.h"
#include "PHFracture.h"
#include "PHJointDestroyInfo.h"
#include "Geometry.h"
#include "MathUtils.h"
#include "Include/xrRender/Kinematics.h"
#include "PHCollideValidator.h"
#include "ph_valid_ode.h"

CPHShellSplitterHolder::CPHShellSplitterHolder(CPHShell* shell)
{
    m_pShell = shell;
    m_has_breaks = false;
    m_unbreakable = false;
}

CPHShellSplitterHolder::~CPHShellSplitterHolder()
{
    Deactivate();
    m_splitters.clear();
    m_geom_root_map.clear();
}
// the simpliest case - a joint to be destroied
shell_root CPHShellSplitterHolder::SplitJoint(u16 aspl)
{
    // create _new physics shell

    CPhysicsShell* new_shell = P_create_Shell();
    CPHShell* new_shell_desc = smart_cast<CPHShell*>(new_shell);
    new_shell_desc->mXFORM.set(m_pShell->mXFORM);
    new_shell_desc->m_object_in_root.set(m_pShell->m_object_in_root);
    auto splitter = m_splitters.begin() + aspl;
    u16 start_element = splitter->m_element;
    u16 start_joint = splitter->m_joint;

    u16 end_element = m_pShell->joints[start_joint]->JointDestroyInfo()->m_end_element;
    u16 end_joint = m_pShell->joints[start_joint]->JointDestroyInfo()->m_end_joint;

    shell_root ret = std::make_pair(new_shell, (m_pShell->joints[start_joint])->BoneID());

    CShellSplitInfo split_inf;
    split_inf.m_bone_id = m_pShell->joints[start_joint]->BoneID();
    split_inf.m_start_el_num = start_element;
    split_inf.m_end_el_num = end_element;
    split_inf.m_start_jt_num = start_joint;
    split_inf.m_end_jt_num = end_joint;

    m_splitters.erase(splitter);
    PassEndSplitters(split_inf, new_shell_desc, 1, 0);

    InitNewShell(new_shell_desc);
    m_pShell->PassEndElements(start_element, end_element, new_shell_desc);
    m_pShell->PassEndJoints(start_joint + 1, end_joint, new_shell_desc);
    new_shell_desc->set_PhysicsRefObject(0);
    new_shell_desc->PureActivate();
    // new_shell_desc->ObjectInRoot().identity();
    m_pShell->DeleteJoint(start_joint);
    new_shell->set_ObjectContactCallback(NULL);
    new_shell->set_PhysicsRefObject(NULL);
    return ret;
}

void CPHShellSplitterHolder::PassEndSplitters(
    const CShellSplitInfo& spl_inf, CPHShell* dest, u16 jt_add_shift, u16 el_add_shift)
{
    CPHShellSplitterHolder*& dest_holder = dest->m_spliter_holder;
    if (!dest_holder)
        dest_holder = new CPHShellSplitterHolder(dest);

    ELEMENT_STORAGE& source_elements = m_pShell->elements;
    ELEMENT_STORAGE& dest_elements = dest->elements;
    auto i_elem = source_elements.begin(), e_elem = source_elements.begin() + spl_inf.m_start_el_num;
    u16 shift_e = spl_inf.m_end_el_num - spl_inf.m_start_el_num;
    u16 shift_j = spl_inf.m_end_jt_num - spl_inf.m_start_jt_num;

    R_ASSERT2(source_elements.size() >= spl_inf.m_start_el_num && source_elements.size() >= spl_inf.m_end_el_num,
        "wrong spl_inf");

    for (; i_elem != e_elem; ++i_elem) // until start elem in both joint or elem split fractures
    // end elems have to be corrected
    // if grater then end elem in moving diapason
    {
        CPHFracturesHolder* fracturesHolder = (*i_elem)->FracturesHolder();
        if (!fracturesHolder)
            continue;
        FRACTURE_I f_i = fracturesHolder->m_fractures.begin(), f_e = fracturesHolder->m_fractures.end();
        for (; f_i != f_e; ++f_i)
        {
            u16& end_el_num = f_i->m_end_el_num;
            u16& start_el_num = f_i->m_start_el_num;
            if (end_el_num >= spl_inf.m_end_el_num)
                end_el_num = end_el_num - shift_e;
            if (start_el_num >= spl_inf.m_end_el_num)
                start_el_num = start_el_num - shift_e;

            u16& end_jt_num = f_i->m_end_jt_num;
            u16& start_jt_num = f_i->m_start_jt_num;
            if (end_jt_num >= spl_inf.m_end_jt_num)
                end_jt_num = end_jt_num - shift_j;
            if (start_jt_num >= spl_inf.m_end_jt_num)
                start_jt_num = start_jt_num - shift_j;
        }
    }

    // same for joints
    JOINT_STORAGE& source_joints = m_pShell->joints;
    auto i_joint = source_joints.begin();
    if (u16(-1) != spl_inf.m_start_jt_num)
    {
        R_ASSERT2(source_joints.size() >= spl_inf.m_start_jt_num && source_joints.size() >= spl_inf.m_end_jt_num,
            "wrong spl_inf");
        auto e_joint = source_joints.begin() + spl_inf.m_start_jt_num;
        for (; i_joint != e_joint; i_joint++)
        {
            CPHJointDestroyInfo* jointDestroyInfo = (*i_joint)->JointDestroyInfo();
            if (!jointDestroyInfo)
                continue;
            u16& end_element = jointDestroyInfo->m_end_element;
            if (end_element >= spl_inf.m_end_el_num)
                end_element = end_element - shift_e;
            u16& end_joint = jointDestroyInfo->m_end_joint;
            if (end_joint >= spl_inf.m_end_jt_num)
                end_joint = end_joint - shift_j;
        }
    }

    // now process diapason that tobe unsplited

    e_elem = source_elements.begin() + spl_inf.m_end_el_num;
    u16 passed_shift_e = spl_inf.m_start_el_num - u16(dest_elements.size());
    u16 passed_shift_j = u16(-1) & (spl_inf.m_start_jt_num + jt_add_shift);
    for (; i_elem != e_elem; ++i_elem)

    {
        CPHFracturesHolder* fracturesHolder = (*i_elem)->FracturesHolder();
        if (!fracturesHolder)
            continue;
        FRACTURE_I f_i = fracturesHolder->m_fractures.begin(), f_e = fracturesHolder->m_fractures.end();
        for (; f_i != f_e; ++f_i)
        {
            u16& end_el_num = f_i->m_end_el_num;
            u16& start_el_num = f_i->m_start_el_num;
            end_el_num = end_el_num - passed_shift_e;
            start_el_num = start_el_num - passed_shift_e;

            u16& end_jt_num = f_i->m_end_jt_num;
            u16& start_jt_num = f_i->m_start_jt_num;
            end_jt_num = end_jt_num - passed_shift_j;
            start_jt_num = start_jt_num - passed_shift_j;
        }
    }

    //////correct data in fractures for elements allready added to dest with fractures from source///////
    auto i_dest_elem = dest_elements.begin(), e_dest_elem = dest_elements.end();
    for (; i_dest_elem != e_dest_elem; ++i_dest_elem)
    {
        CPHFracturesHolder* fracturesHolder = (*i_dest_elem)->FracturesHolder();
        if (!fracturesHolder)
            continue;
        FRACTURE_I f_i = fracturesHolder->m_fractures.begin(), f_e = fracturesHolder->m_fractures.end();
        for (; f_i != f_e; f_i++)
        {
            u16& end_el_num = f_i->m_end_el_num;
            u16& start_el_num = f_i->m_start_el_num;
            end_el_num = end_el_num - passed_shift_e;
            start_el_num = start_el_num - passed_shift_e;

            u16& end_jt_num = f_i->m_end_jt_num;
            u16& start_jt_num = f_i->m_start_jt_num;
            end_jt_num = end_jt_num - passed_shift_j;
            start_jt_num = start_jt_num - passed_shift_j;
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    if (spl_inf.m_end_jt_num != u16(-1))
    {
        auto e_joint = source_joints.begin() + spl_inf.m_end_jt_num;
        for (; i_joint != e_joint; ++i_joint)
        {
            CPHJointDestroyInfo* jointDestroyInfo = (*i_joint)->JointDestroyInfo();
            if (!jointDestroyInfo)
                continue;
            u16& end_element = jointDestroyInfo->m_end_element;
            u16& end_joint = jointDestroyInfo->m_end_joint;
            end_element = end_element - passed_shift_e;
            end_joint = end_joint - passed_shift_j;
        }
    }
    // the rest unconditionaly shift end & begin
    e_elem = source_elements.end();
    for (; i_elem != e_elem; ++i_elem)

    {
        CPHFracturesHolder* fracturesHolder = (*i_elem)->FracturesHolder();
        if (!fracturesHolder)
            continue;
        FRACTURE_I f_i = fracturesHolder->m_fractures.begin(), f_e = fracturesHolder->m_fractures.end();
        for (; f_i != f_e; ++f_i)
        {
            u16& end_el_num = f_i->m_end_el_num;
            u16& start_el_num = f_i->m_start_el_num;
            end_el_num = end_el_num - shift_e;
            start_el_num = start_el_num - shift_e;

            u16& end_jt_num = f_i->m_end_jt_num;
            u16& start_jt_num = f_i->m_start_jt_num;
            end_jt_num = end_jt_num - shift_j;
            start_jt_num = start_jt_num - shift_j;
        }
    }

    auto e_joint = source_joints.end();
    for (; i_joint != e_joint; ++i_joint)
    {
        CPHJointDestroyInfo* jointDestroyInfo = (*i_joint)->JointDestroyInfo();
        if (!jointDestroyInfo)
            continue;
        u16& end_element = jointDestroyInfo->m_end_element;
        u16& end_joint = jointDestroyInfo->m_end_joint;
        if (end_element > spl_inf.m_end_el_num)
            end_element = end_element - shift_e;
        if (end_joint > spl_inf.m_end_jt_num)
            end_joint = end_joint - shift_j;
    }
    // at rest!! pass splitters it is very similar passing fractures
    // correct data for splitters before passed and find start splitter to pass
    auto spl_e = m_splitters.end(), spl_i = m_splitters.begin();
    for (; spl_i != spl_e; ++spl_i)
    {
        u16& elem = spl_i->m_element;
        u16& joint = spl_i->m_joint;
        if (spl_i->m_type == CPHShellSplitter::splElement)
        {
            if (elem != u16(-1) && elem >= spl_inf.m_start_el_num)
                break; // we at beginning
        }
        else
        {
            if (joint != u16(-1) && joint >= spl_inf.m_start_jt_num)
                break; // we at beginning
        }
        if (elem != u16(-1) && elem > spl_inf.m_end_el_num)
            elem = elem - shift_e;
        if (joint != u16(-1) && joint > spl_inf.m_end_jt_num)
            joint = joint - shift_j;
    }
    auto i_from = spl_i;
    // correct data for passing splitters and find last splitter to pass
    for (; spl_i != spl_e; ++spl_i)
    {
        u16& elem = spl_i->m_element;
        u16& joint = spl_i->m_joint;
        if (spl_i->m_type == CPHShellSplitter::splElement)
        {
            if (elem != u16(-1) && elem >= spl_inf.m_end_el_num)
                break; // we after beginning
        }
        else
        {
            if (joint != u16(-1) && joint >= spl_inf.m_end_jt_num)
                break; // we after beginning
        }
        if (elem != u16(-1))
            elem = elem - passed_shift_e;
        if (joint != u16(-1))
            joint = joint - passed_shift_j;
    }

    auto i_to = spl_i;

    // corect data for all rest splitters
    for (; spl_i != spl_e; ++spl_i)
    {
        u16& elem = spl_i->m_element;
        u16& joint = spl_i->m_joint;
        if (elem != u16(-1))
            elem = elem - shift_e;
        if (joint != u16(-1))
            joint = joint - shift_j;
    }

    dest_holder->m_splitters.insert(dest_holder->m_splitters.end(), i_from, i_to);
    m_splitters.erase(i_from, i_to);
}

static ELEMENT_PAIR_VECTOR new_elements;
static xr_vector<Fmatrix> bones_bind_forms;
shell_root CPHShellSplitterHolder::ElementSingleSplit(
    const element_fracture& split_elem, const CPHElement* source_element)
{
    // const CPHShellSplitter& splitter=m_splitters[aspl];
    // CPHElement* element=m_pShell->elements[splitter.m_element];
    CPhysicsShell* new_shell_last = P_create_Shell();
    CPHShell* new_shell_last_desc = smart_cast<CPHShell*>(new_shell_last);
    new_shell_last->mXFORM.set(m_pShell->mXFORM);
    const u16 start_joint = split_elem.second.m_start_jt_num;
    R_ASSERT(_valid(new_shell_last->mXFORM));
    const u16 end_joint = split_elem.second.m_end_jt_num;
    // it is not right for multiple joints attached to the unsplitted part because all these need to be reattached
    if (start_joint != end_joint)
    {
        JOINT_STORAGE& joints = m_pShell->joints;
        auto i = joints.begin() + start_joint, e = joints.begin() + end_joint;
        for (; i != e; ++i)
        {
            CPHJoint* joint = (*i);
            if (joint->PFirst_element() == source_element)
            {
                IKinematics* K = m_pShell->PKinematics();
                dVector3 safe_pos1, safe_pos2;
                dQuaternion safe_q1, safe_q2;
                // CPhysicsElement* el1=cast_PhysicsElement(split_elem.first),*el2=joint->PSecond_element();
                VERIFY(smart_cast<CPHElement*>(joint->PSecond_element()));

                CPHElement *el1 = (split_elem.first), *el2 = static_cast<CPHElement *>(joint->PSecond_element());

                dBodyID body1 = el1->get_body(), body2 = el2->get_body();
                dVectorSet(safe_pos1, dBodyGetPosition(body1));
                dVectorSet(safe_pos2, dBodyGetPosition(body2));

                dQuaternionSet(safe_q1, dBodyGetQuaternion(body1));
                dQuaternionSet(safe_q2, dBodyGetQuaternion(body2));

                // m_pShell->PlaceBindToElForms();

                K->LL_GetBindTransform(bones_bind_forms);
                el1->SetTransform(bones_bind_forms[el1->m_SelfID], mh_unspecified);
                el2->SetTransform(bones_bind_forms[el2->m_SelfID], mh_unspecified);
                joint->ReattachFirstElement(split_elem.first);

                dVectorSet(const_cast<dReal*>(dBodyGetPosition(body1)), safe_pos1);
                dVectorSet(const_cast<dReal*>(dBodyGetPosition(body2)), safe_pos2);

                dQuaternionSet(const_cast<dReal*>(dBodyGetQuaternion(body1)), safe_q1);
                dQuaternionSet(const_cast<dReal*>(dBodyGetQuaternion(body2)), safe_q2);

                dBodySetPosition(body1, safe_pos1[0], safe_pos1[1], safe_pos1[2]);
                dBodySetPosition(body2, safe_pos2[0], safe_pos2[1], safe_pos2[2]);
                dBodySetQuaternion(body1, safe_q1);
                dBodySetQuaternion(body2, safe_q2);
            }
        }
        //	m_pShell->joints[split_elem.second.m_start_jt_num]->ReattachFirstElement(split_elem.first);
    }

    // the last new shell will have all splitted old elements end joints and one new element reattached to old joint
    // m_splitters.erase(m_splitters.begin()+aspl);
    // now aspl points to the next splitter
    if ((split_elem.first)->FracturesHolder()) // if this element can be splitted add a splitter for it
        new_shell_last_desc->AddSplitter(CPHShellSplitter::splElement, 0, u16(-1)); //

    new_shell_last_desc->add_Element(split_elem.first);
    // pass splitters taking into account that one element was olready added
    PassEndSplitters(split_elem.second, new_shell_last_desc, 0, 0);

    InitNewShell(new_shell_last_desc);
    m_pShell->PassEndElements(split_elem.second.m_start_el_num, split_elem.second.m_end_el_num, new_shell_last_desc);

    m_pShell->PassEndJoints(split_elem.second.m_start_jt_num, split_elem.second.m_end_jt_num, new_shell_last_desc);
    new_shell_last_desc->set_PhysicsRefObject(0);
    ///////////////////temporary for initialization set old Kinematics in new shell/////////////////
    new_shell_last->set_Kinematics(m_pShell->PKinematics());
    new_shell_last_desc->AfterSetActive();
    new_shell_last->set_Kinematics(NULL);
    VERIFY2(split_elem.second.m_bone_id < 64, "strange root");
    VERIFY(_valid(new_shell_last->mXFORM));
    VERIFY(dBodyStateValide(source_element->get_bodyConst()));
    VERIFY(dBodyStateValide(split_elem.first->get_body()));
    new_shell_last->set_ObjectContactCallback(NULL);
    new_shell_last->set_PhysicsRefObject(NULL);
    return std::make_pair(new_shell_last, split_elem.second.m_bone_id);
}

IC void correct_diapasones(ELEMENT_PAIR_VECTOR& element_pairs)
{
    auto b = element_pairs.begin(), e = element_pairs.end();

    for (auto i = b; i != e; ++i)
    {
        auto j = i + 1;
        for (; j != e; ++j)
            j->second.sub_diapasone(CShellSplitInfo(i->second));
    }
}

void CPHShellSplitterHolder::SplitElement(u16 aspl, PHSHELL_PAIR_VECTOR& out_shels)
{
    new_elements.clear();
    auto spl_i = (m_splitters.begin() + aspl);
    CPHElement* E = m_pShell->elements[spl_i->m_element];
    E->SplitProcess(new_elements);

    correct_diapasones(new_elements);

    auto i = new_elements.begin(), e = new_elements.end();

    for (; i != e; ++i)
    {
        out_shels.push_back(ElementSingleSplit(*i, E));
        CPhysicsElement* ee = out_shels.back().first->get_ElementByStoreOrder(0);
        VERIFY(ee);
        VERIFY(smart_cast<CPHElement*>(ee));
        CPHElement* e2 = static_cast<CPHElement*>(ee);
        VERIFY(dBodyStateValide(e2->get_body()));
    }

    if (!E->FracturesHolder())
        m_splitters.erase(spl_i); // delete splitter if the element no longer have fractures
    else
        spl_i->m_breaked = false; // it is no longer breaked
}

void CPHShellSplitterHolder::SplitProcess(PHSHELL_PAIR_VECTOR& out_shels)
{
    // any split process must start from the end of the elment storage
    // this based on that all childs in the bone hierarchy was added after their parrent

    u16 i = u16(m_splitters.size() - 1);
    for (; u16(-1) != i; --i)
    {
        if (m_splitters[i].m_breaked)
            switch (m_splitters[i].m_type)
            {
            case CPHShellSplitter::splJoint: out_shels.push_back(SplitJoint(i)); break;
            case CPHShellSplitter::splElement: SplitElement(i, out_shels); break;
            default: NODEFAULT;
            }
    }
    // VERIFY(dBodyStateValide(out_shels.back().first->get_ElementByStoreOrder(0)->get_body()));
    m_has_breaks = false;
}
void CPHShellSplitterHolder::InitNewShell(CPHShell* shell)
{
    shell->PresetActive();
    if (m_pShell->IsGroupObject())
        CPHCollideValidator::RegisterObjToGroup(m_pShell->collide_bits(), *static_cast<CPHObject*>(shell));
}

void CPHShellSplitterHolder::PhTune(dReal step)
{
    auto i = m_splitters.begin(), e = m_splitters.end();
    for (; i != e; ++i)
    {
        switch (i->m_type)
        {
        case CPHShellSplitter::splElement:
        {
            CPHElement* element = m_pShell->elements[i->m_element];
            element->FracturesHolder()->PhTune(element->get_body());
            break;
        }
        case CPHShellSplitter::splJoint: { break;
        }
        default: NODEFAULT;
        }
    }
}
void CPHShellSplitterHolder::PhDataUpdate(dReal step)
{
    auto i = m_splitters.begin(), e = m_splitters.end();
    for (; i != e; ++i)
    {
        switch (i->m_type)
        {
        case CPHShellSplitter::splElement:
        {
            CPHElement* element = m_pShell->elements[i->m_element];
            dBodyID body = element->get_body(); //! element->EnabledStateOnStep()
            if (!dBodyIsEnabled(body))
                return; //
            i->m_breaked = (element->FracturesHolder()->PhDataUpdate(element)) || i->m_breaked;
            break;
        }
        case CPHShellSplitter::splJoint:
        {
            CPHJoint* j = m_pShell->joints[i->m_joint];
            // if(j->bActive)
            i->m_breaked = j->JointDestroyInfo()->Update() || i->m_breaked;
            break;
        }
        default: NODEFAULT;
        }
        m_has_breaks = m_has_breaks || i->m_breaked;
    }
}
void CPHShellSplitterHolder::Activate()
{
    if (m_unbreakable)
        return;
    CPHUpdateObject::Activate();
    if (m_pShell->isActive())
        PhTune(fixed_step);
}

void CPHShellSplitterHolder::Deactivate() { CPHUpdateObject::Deactivate(); }
void CPHShellSplitterHolder::AddSplitter(CPHShellSplitter::EType type, u16 element, u16 joint)
{
    m_splitters.push_back(CPHShellSplitter(type, element, joint));
}
void CPHShellSplitterHolder::AddSplitter(CPHShellSplitter::EType type, u16 element, u16 joint, u16 position)
{
    m_splitters.insert(m_splitters.begin() + position, CPHShellSplitter(type, element, joint));
}
CPHShellSplitter::CPHShellSplitter(CPHShellSplitter::EType type, u16 element, u16 joint)
{
    m_breaked = false;
    m_type = type;
    m_element = element;
    m_joint = joint;
}

void CPHShellSplitterHolder::AddToGeomMap(const id_geom& id_rootgeom) { m_geom_root_map.insert(id_rootgeom); }
u16 CPHShellSplitterHolder::FindRootGeom(u16 bone_id)
{
    auto iter = m_geom_root_map.find(bone_id);
    if (iter == m_geom_root_map.end())
        return u16(-1);

    return iter->second->element_position();
}
void CPHShellSplitterHolder::SetUnbreakable()
{
    Deactivate();
    m_unbreakable = true;
}
void CPHShellSplitterHolder::SetBreakable()
{
    m_unbreakable = false;
    if (m_pShell->isEnabled())
        Activate();
}
CPHShellSplitter::CPHShellSplitter() { m_breaked = false; }
