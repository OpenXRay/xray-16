/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "tri-colliderknoopc/dTriList.h"
#include "PHShellSplitter.h"
#include "PHFracture.h"
#include "PHJointDestroyInfo.h"
#include "SpaceUtils.h"
#include "MathUtils.h"
#include "IPhysicsShellHolder.h"
#include "Include/xrRender/Kinematics.h"
#include "PHCollideValidator.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrEngine/GameMtlLib.h"

//#pragma warning(push)
//#pragma warning(disable:4995)
//#pragma warning(disable:4267)

//#pragma warning(pop)
///////////////////////////////////////////////////////////////
//#pragma warning(push)
///#pragma warning(disable:4995)

//#pragma warning(pop)
///////////////////////////////////////////////////////////////////

#include "ExtendedGeom.h"
#include "PHElement.h"
#include "PHShell.h"
#include "PHCollideValidator.h"
#include "PHElementInline.h"
#include "PhysicsShellAnimator.h"
#include "PHShellBuildJoint.h"
#include "Common/Noncopyable.hpp"
#ifdef DEBUG
#include "debug_output.h"
#endif

IC bool PhOutOfBoundaries(const Fvector& v) { return v.y < phBoundaries.y1; }
CPHShell::~CPHShell()
{
    m_pKinematics = nullptr;
    VERIFY(!isActive());

    for (auto& it : elements)
        xr_delete(it);
    elements.clear();

    for (auto& it : joints)
        xr_delete(it);
    joints.clear();

    if (m_spliter_holder)
        xr_delete(m_spliter_holder);
}
CPHShell::CPHShell()
{
    // bActive=false;
    // bActivating=false;
    m_flags.assign(0);
    m_flags.set(flActivating, false);
    m_flags.set(flActive, false);
    m_space = nullptr;
    m_pKinematics = nullptr;
    m_spliter_holder = nullptr;
    m_object_in_root.identity();
    m_active_count = 0;
    m_pPhysicsShellAnimatorC = nullptr;
}

void CPHShell::EnableObject(CPHObject* obj)
{
    CPHObject::activate();
    if (m_spliter_holder)
        m_spliter_holder->Activate();
}
void CPHShell::DisableObject()
{
    IPhysicsShellHolder* ref_object = elements.front()->PhysicsRefObject();
    //. if (!ref_object) return;

    if (ref_object)
        ref_object->on_physics_disable();

    // InterpolateGlobalTransform(&mXFORM);
    CPHObject::deactivate();
    if (m_spliter_holder)
        m_spliter_holder->Deactivate();
    if (m_flags.test(flRemoveCharacterCollisionAfterDisable))
        DisableCharacterCollision();
}
void CPHShell::DisableCharacterCollision() { CPHCollideValidator::SetCharacterClassNotCollide(*this); }
void CPHShell::Disable()
{
    DisableObject();
    for (auto& it : elements)
        it->Disable();
    ClearCashedTries();
}
void CPHShell::DisableCollision() { CPHObject::collision_disable(); }
void CPHShell::EnableCollision() { CPHObject::collision_enable(); }
void CPHShell::ReanableObject()
{
    // if(b_contacts_saved) dJointGroupEmpty(m_saved_contacts);
    // b_contacts_saved=false;
}

void CPHShell::vis_update_activate()
{
    ++m_active_count;
    IPhysicsShellHolder* ref_object = elements.front()->PhysicsRefObject();
    if (ref_object && m_active_count > 0)
    {
        m_active_count = 0;
        ref_object->ObjectProcessingActivate();
    }
}

void CPHShell::vis_update_deactivate()
{
    --m_active_count;
    // IPhysicsShellHolder* ref_object=elements.front()->PhysicsRefObject();
    // if(ref_object&&!m_flags.test(flProcessigDeactivated))
    //{
    //  //ref_object->processing_deactivate();
    //  m_flags.set(flProcessigDeactivate,TRUE);
    //}
}
void CPHShell::setDensity(float M)
{
    //float volume = 0.f;
    //for (auto& it : elements)
    //    volume += it->get_volume();

    for (auto& it : elements)
        it->setDensity(M);
}

void CPHShell::setMass(float M)
{
    float volume = 0.f;
    for (auto& it : elements)
        volume += it->get_volume();

    for (auto& it : elements)
        it->setMass(it->get_volume() / volume * M);
}

void CPHShell::setMass1(float M)
{
    for (auto& it : elements)
        it->setMass(M / elements.size());
}

void CPHShell::MassAddBox(float mass, const Fvector& full_size)
{
    dMass m;
    dMassSetBox(&m, mass, full_size.x, full_size.y, full_size.z); // mass = m_Shell->getMass()/100.f, full_size (1,1,1)
    addEquelInertiaToEls(m);
}

float CPHShell::getMass()
{
    float m = 0.f;

    for (auto& it : elements)
        m += it->getMass();

    return m;
}

void CPHShell::get_spatial_params()
{
    spatialParsFromDGeom((dGeomID)m_space, spatial.sphere.P, AABB, spatial.sphere.R);
}

float CPHShell::getVolume()
{
    float v = 0.f;

    for (auto& it : elements)
        v += it->getVolume();

    return v;
}

float CPHShell::getDensity() { return getMass() / getVolume(); }
void CPHShell::PhDataUpdate(dReal step)
{
    bool disable = true;
    for (auto& it : elements)
    {
        it->PhDataUpdate(step);
        const dBodyID body = it->get_body();
        if (body && disable && dBodyIsEnabled(body))
            disable = false;
    }
    if (disable)
    {
        DisableObject();
        CPHObject::put_in_recently_deactivated();
    }
    else
        ReanableObject();
#if 0
    DBG_OpenCashedDraw();
    dbg_draw_velocity   ( 0.1f, color_xrgb( 255, 0, 0 ) ); 
    dbg_draw_force      ( 0.1f, color_xrgb( 0, 0, 255 ) ); 
    DBG_ClosedCashedDraw( 10000 );
    //dbg_draw_geometry
#endif
    if (PhOutOfBoundaries(cast_fv(dBodyGetPosition(elements.front()->get_body()))))
        Disable();
}

void CPHShell::PhTune(dReal step)
{
    for (auto& it : elements)
        it->PhTune(step);
}

void CPHShell::Update()
{
    if (!isActive())
        return;
    if (m_flags.test(flActivating))
        m_flags.set(flActivating, false);

    for (auto& it : elements)
        it->Update();

    mXFORM.set(elements.front()->mXFORM);
    VERIFY2(_valid(mXFORM), "invalid position in update");
}

void CPHShell::Freeze() { CPHObject::Freeze(); }
void CPHShell::UnFreeze() { CPHObject::UnFreeze(); }
void CPHShell::FreezeContent()
{
    CPHObject::FreezeContent();
    for (auto& it : elements)
        it->Freeze();
}
void CPHShell::UnFreezeContent()
{
    CPHObject::UnFreezeContent();
    for (auto& it : elements)
        it->UnFreeze();
}
void CPHShell::applyForce(const Fvector& dir, float val)
{
    if (!isActive())
        return;
    val /= getMass();
    for (auto& it : elements)
        it->applyForce(dir, val * it->getMass());
    EnableObject(nullptr);
};
void CPHShell::applyForce(float x, float y, float z)
{
    Fvector dir;
    dir.set(x, y, z);
    float val = dir.magnitude();
    if (!fis_zero(val))
    {
        dir.mul(1.f / val);
        applyForce(dir, val);
    }
};
void CPHShell::applyImpulse(const Fvector& dir, float val)
{
    if (!isActive())
        return;
    elements.front()->applyImpulse(dir, val);
    EnableObject(nullptr);
};
void CPHShell::applyImpulseTrace(const Fvector& pos, const Fvector& dir, float val)
{
    if (!isActive())
        return;
    elements.front()->applyImpulseTrace(pos, dir, val, 0);
    EnableObject(nullptr);
}

void CPHShell::applyImpulseTrace(const Fvector& pos, const Fvector& dir, float val, const u16 id)
{
    if (!isActive())
        return;
    VERIFY(m_pKinematics);
    CBoneInstance& instance = m_pKinematics->LL_GetBoneInstance(id);
    if (instance.callback_type() != bctPhysics || !instance.callback_param())
        return;

    ((CPhysicsElement*)instance.callback_param())->applyImpulseTrace(pos, dir, val, id);
    EnableObject(nullptr);
}

CPhysicsElement* CPHShell::get_Element(const shared_str& bone_name)
{
    VERIFY(m_pKinematics);
    return get_Element(m_pKinematics->LL_BoneID(bone_name));
}
CPhysicsElement* CPHShell::get_Element(LPCSTR bone_name) { return get_Element((const shared_str&)(bone_name)); }
CPhysicsElement* CPHShell::get_ElementByStoreOrder(u16 num)
{
    R_ASSERT2(num < elements.size(), "argument is out of range");
    return cast_PhysicsElement(elements[num]);
}
const CPhysicsElement* CPHShell::get_ElementByStoreOrder(u16 num) const
{
    R_ASSERT2(num < elements.size(), "argument is out of range");
    return cast_PhysicsElement(elements[num]);
}
CPHSynchronize* CPHShell::get_ElementSync(u16 element) { return smart_cast<CPHSynchronize*>(elements[element]); }
CPhysicsElement* CPHShell::get_PhysicsParrentElement(u16 bone_id)
{
    VERIFY(PKinematics());
    CPhysicsElement* E = get_Element(bone_id);
    u16 bid = bone_id;
    while (!E && bid != PKinematics()->LL_GetBoneRoot())
    {
        CBoneData& bd = PKinematics()->LL_GetData(bid);
        bid = bd.GetParentID();
        E = get_Element(bid);
    }
    return E;
}

CPhysicsElement* CPHShell::get_Element(u16 bone_id)
{
    if (m_pKinematics && isActive())
    {
        CBoneInstance& instance = m_pKinematics->LL_GetBoneInstance(bone_id);
        if (instance.callback() == BonesCallback || instance.callback() == StataticRootBonesCallBack)
        {
            return (instance.callback_type() == bctPhysics) ? (CPhysicsElement*)instance.callback_param() : NULL;
        }
    }

    for (auto& it : elements)
        if (it->m_SelfID == bone_id)
            return static_cast<CPhysicsElement*>(it);
    return nullptr;
}

CPhysicsJoint* CPHShell::get_Joint(u16 bone_id)
{
    for (auto& it : joints)
        if (it->BoneID() == bone_id)
            return static_cast<CPhysicsJoint*>(it);
    return nullptr;
}
CPhysicsJoint* CPHShell::get_Joint(const shared_str& bone_name)
{
    VERIFY(m_pKinematics);
    return get_Joint(m_pKinematics->LL_BoneID(bone_name));
}

CPhysicsJoint* CPHShell::get_Joint(LPCSTR bone_name) { return get_Joint((const shared_str&)bone_name); }
CPhysicsJoint* CPHShell::get_JointByStoreOrder(u16 num)
{
    R_ASSERT(num < joints.size());
    return (CPhysicsJoint*)joints[num];
}

u16 CPHShell::get_JointsNumber() { return u16(joints.size()); }
void CPHShell::update_root_transforms()
{
    u16 anim_root = PKinematics()->LL_GetBoneRoot();
    u16 phys_root = root_element().m_SelfID;
    VERIFY(BI_NONE != anim_root);
    VERIFY(BI_NONE != phys_root);

    if (anim_root == phys_root)
    {
        mXFORM.set(root_element().mXFORM);
        return;
    }

    // Fmatrix anim_root_transform = PKinematics()->LL_GetBindTransform( anim_root );
}

void CPHShell::BonesCallback(CBoneInstance* B)
{
    /// CPHElement*  E           = smart_cast<CPHElement*>   (static_cast<CPhysicsBase*>(B->Callback_Param));

    CPHElement* E = cast_PHElement(B->callback_param());
    // if( E == &root_element() )
    //{

    //}
    E->BonesCallBack(B);
    VERIFY2(_valid(B->mTransform), "CPHShell:: BonesCallback");
}

void CPHShell::StataticRootBonesCallBack(CBoneInstance* B)
{
    /// CPHElement*  E           = smart_cast<CPHElement*>   (static_cast<CPhysicsBase*>(B->Callback_Param));

    CPHElement* E = cast_PHElement(B->callback_param());
    E->StataticRootBonesCallBack(B);
}

void CPHShell::SetTransform(const Fmatrix& m0, motion_history_state history_state)
{
    mXFORM.set(m0);
    for (auto& it : elements)
        it->SetTransform(m0, history_state);
    spatial_move();
}

void CPHShell::Enable()
{
    if (!isActive())
        return;

    
    for (auto& it : elements)
    {
        //if(dBodyIsEnabled(it->get_body()))
        //    return;
        it->Enable();
    }
    EnableObject(nullptr);
}

void CPHShell::set_PhysicsRefObject(IPhysicsShellHolder* ref_object)
{
    if (elements.empty())
        return;
    if (elements.front()->PhysicsRefObject() == ref_object)
        return;

    for (auto& it : elements)
        it->set_PhysicsRefObject(ref_object);
}

void CPHShell::set_ContactCallback(ContactCallbackFun* callback)
{
    for (auto& it : elements)
        it->set_ContactCallback(callback);
}

void CPHShell::set_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
    for (auto& it : elements)
        it->set_ObjectContactCallback(callback);
}
void CPHShell::add_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
    for (auto& it : elements)
        it->add_ObjectContactCallback(callback);
}
void CPHShell::remove_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
    for (auto& it : elements)
        it->remove_ObjectContactCallback(callback);
}
void CPHShell::set_CallbackData(void* cd)
{
    for (auto& it : elements)
        it->set_CallbackData(cd);
}
void CPHShell::SetPhObjectInElements()
{
    if (!isActive())
        return;

    for (auto& it : elements)
        it->SetPhObjectInGeomData((CPHObject*)this);
}

void CPHShell::SetMaterial(LPCSTR m)
{
    for (auto& it : elements)
        it->SetMaterial(m);
}

void CPHShell::SetMaterial(u16 m)
{
    for (auto& it : elements)
        it->SetMaterial(m);
}

void CPHShell::get_LinearVel(Fvector& velocity) const { elements.front()->get_LinearVel(velocity); }
void CPHShell::get_AngularVel(Fvector& velocity) const { elements.front()->get_AngularVel(velocity); }
void CPHShell::set_LinearVel(const Fvector& velocity)
{
    for (auto& it : elements)
        it->set_LinearVel(velocity);
}

void CPHShell::set_AngularVel(const Fvector& velocity)
{
    for (auto& it : elements)
        it->set_AngularVel(velocity);
}

void CPHShell::TransformPosition(const Fmatrix& form, motion_history_state history_state)
{
    for (auto& it : elements)
        it->TransformPosition(form, history_state);
}

void CPHShell::SetGlTransformDynamic(const Fmatrix& form)
{
    VERIFY(isActive());
    VERIFY(_valid(form));
    Fmatrix current, replace;
    GetGlobalTransformDynamic(&current);
    current.invert();
    replace.mul(form, current);
    TransformPosition(replace, mh_clear);
}
void CPHShell::SmoothElementsInertia(float k)
{
    dMass m_avrg;
    dReal krc = 1.f - k;
    dMassSetZero(&m_avrg);

    for (auto& it : elements)
        dMassAdd(&m_avrg, it->getMassTensor());

    int n = (int)elements.size();
    m_avrg.mass *= k / float(n);
    for (int j = 0; j < 4 * 3; ++j)
        m_avrg.I[j] *= k / float(n);

    for (auto& it : elements)
    {
        dVector3 tmp;
        dMass* m = it->getMassTensor();
        dVectorSet(tmp, m->c);

        m->mass *= krc;
        for (int j = 0; j < 4 * 3; ++j)
            m->I[j] *= krc;
        dMassAdd(m, &m_avrg);

        dVectorSet(m->c, tmp);
    }
}

void CPHShell::setEquelInertiaForEls(const dMass& M)
{
    for (auto& it : elements)
        it->setInertia(M);
}

void CPHShell::addEquelInertiaToEls(const dMass& M)
{
    for (auto& it : elements)
        it->addInertia(M);
}
static BONE_P_MAP* spGetingMap = nullptr;
void CPHShell::build_FromKinematics(IKinematics* K, BONE_P_MAP* p_geting_map)
{
    VERIFY(K);
    phys_shell_verify_model(*K);
    m_pKinematics = K;
    spGetingMap = p_geting_map;
    // CBoneData& bone_data  = m_pKinematics->LL_GetData(0);
    if (!m_spliter_holder)
        m_spliter_holder = new CPHShellSplitterHolder(this);
    bool vis_check = false;
    AddElementRecursive(nullptr, m_pKinematics->LL_GetBoneRoot(), Fidentity, 0, &vis_check);
    // R_ASSERT2(elements.front()->numberOfGeoms(),"No physics shapes was assigned for model or no shapes in main
    // root bone!!!");
    // SetCallbacks(BonesCallback);
    if (m_spliter_holder->isEmpty())
        ClearBreakInfo();
}

void CPHShell::preBuild_FromKinematics(IKinematics* K, BONE_P_MAP* p_geting_map)
{
    VERIFY(K);
    phys_shell_verify_model(*K);
    m_pKinematics = K;
    spGetingMap = p_geting_map;
    // CBoneData& bone_data  = m_pKinematics->LL_GetData(0);
    if (!m_spliter_holder)
        m_spliter_holder = new CPHShellSplitterHolder(this);
    bool vis_check = false;
    AddElementRecursive(nullptr, m_pKinematics->LL_GetBoneRoot(), Fidentity, 0, &vis_check);
    // R_ASSERT2(elements.front()->numberOfGeoms(),"No physics shapes was assigned for model or no shapes in main
    // root bone!!!");
    if (m_spliter_holder->isEmpty())
        ClearBreakInfo();
    m_pKinematics = nullptr;
}
void CPHShell::ClearBreakInfo()
{
    for (auto& it : elements)
        it->ClearDestroyInfo();

    for (auto& it : joints)
        it->ClearDestroyInfo();
    xr_delete(m_spliter_holder);
}

ICF bool no_physics_shape(const SBoneShape& shape)
{
    return shape.type == SBoneShape::stNone || shape.flags.test(SBoneShape::sfNoPhysics);
}

bool shape_is_physic(const SBoneShape& shape) { return !no_physics_shape(shape); }
void CPHShell::AddElementRecursive(
    CPhysicsElement* root_e, u16 id, Fmatrix global_parent, u16 element_number, bool* vis_check)
{
    // CBoneInstance& B  = m_pKinematics->LL_GetBoneInstance(u16(id));
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const IBoneData& bone_data = m_pKinematics->GetBoneData(u16(id));
    const SJointIKData& joint_data = bone_data.get_IK_data();
    Fmatrix fm_position;
    fm_position.set(bone_data.get_bind_transform());
    fm_position.mulA_43(global_parent);
    Flags64 mask;
    mask.assign(m_pKinematics->LL_GetBonesVisible());
	bool no_visible = !mask.is(UINT64_C(1) << (u64)id);
    bool lvis_check = false;
    if (no_visible)
    {
        // for (vecBonesIt it=bone_data.children.begin(); bone_data.children.end() != it; ++it)
        // AddElementRecursive       (root_e,(*it)->GetSelfID(),fm_position,element_number,&lvis_check);
        // IBoneData &ibone_data = bone_data;
        u16 num_children = bone_data.GetNumChildren();
        for (u16 i = 0; i < num_children; ++i)
            AddElementRecursive(root_e, bone_data.GetChild(i).GetSelfID(), fm_position, element_number, &lvis_check);

        return;
    }

    CPhysicsElement* E = nullptr;
    CPhysicsJoint* J = nullptr;
    bool breakable = joint_data.ik_flags.test(SJointIKData::flBreakable) && root_e &&
        !(no_physics_shape(bone_data.get_shape()) && joint_data.type == jtRigid);

    ///////////////////////////////////////////////////////////////
    lvis_check = (check_obb_sise(bone_data.get_obb()));

    bool* arg_check = vis_check;
    if (breakable || !root_e) //.
    {
        arg_check = &lvis_check;
    }
    else
    {
        *vis_check = *vis_check || lvis_check;
    }
    /////////////////////////////////////////////////////////////////////

    bool element_added = false; // set true when if elemen created and added by this call
    u16 splitter_position = 0;
    u16 fracture_num = u16(-1);

    if (!no_physics_shape(bone_data.get_shape()) || !root_e) //
    {
        if (joint_data.type == jtRigid && root_e) //
        {
            Fmatrix vs_root_position;
            vs_root_position.set(root_e->mXFORM);
            vs_root_position.invert();
            vs_root_position.mulB_43(fm_position);

            E = root_e;
            if (breakable)
            {
                CPHFracture fracture;
                fracture.m_bone_id = id;
                R_ASSERT2(id < 64, "ower 64 bones in breacable are not supported");
                fracture.m_start_geom_num = E->numberOfGeoms();
                fracture.m_end_geom_num = u16(-1);
                fracture.m_start_el_num = u16(elements.size());
                fracture.m_start_jt_num = u16(joints.size());
                fracture.MassSetFirst(*(E->getMassTensor()));
                fracture.m_pos_in_element.set(vs_root_position.c);
                VERIFY(u16(-1) != fracture.m_start_geom_num);
                fracture.m_break_force = joint_data.break_force;
                fracture.m_break_torque = joint_data.break_torque;
                root_e->add_Shape(bone_data.get_shape(), vs_root_position);
                root_e->add_Mass(bone_data.get_shape(), vs_root_position, bone_data.get_center_of_mass(),
                    bone_data.get_mass(), &fracture);

                fracture_num = E->setGeomFracturable(fracture);
            }
            else
            {
                root_e->add_Shape(bone_data.get_shape(), vs_root_position);
                root_e->add_Mass(
                    bone_data.get_shape(), vs_root_position, bone_data.get_center_of_mass(), bone_data.get_mass());
            }

            // B.Callback_Param=root_e;
            // B.Callback=NULL;
        }
        else //
        {
            E = P_create_Element();
            E->m_SelfID = id;
            E->mXFORM.set(fm_position);
            u16 mtlIndex = bone_data.get_game_mtl_idx();
            if (mtlIndex == u16(-1))
                mtlIndex = GMLibrary().GetMaterialIdx(bone_data.GetMaterialName().c_str());
            E->SetMaterial(mtlIndex);
            // Fvector mc;
            // fm_position.transform_tiny(mc,bone_data.center_of_mass);
            E->set_ParentElement(root_e);
            /// B.set_callback(BonesCallback1,E);
            if (!no_physics_shape(bone_data.get_shape()))
            {
                E->add_Shape(bone_data.get_shape());
                E->setMassMC(bone_data.get_mass(), bone_data.get_center_of_mass());
            }
            element_number = u16(elements.size());
            add_Element(E);
            element_added = true;

            if (root_e)
            {
                J = BuildJoint(bone_data, root_e, E);
                if (J)
                {
                    // J->SetForceAndVelocity(joint_data.friction);//joint_data.friction
                    SetJointRootGeom(root_e, J);
                    J->SetBoneID(id);
                    add_Joint(J);
                    if (breakable)
                    {
                        setEndJointSplitter();
                        J->SetBreakable(joint_data.break_force, joint_data.break_torque);
                    }
                }
            }
            if (m_spliter_holder)
            {
                splitter_position = u16(m_spliter_holder->m_splitters.size());
            }
        }
    }
    else
    {
        // B.set_callback(0,root_e);
        E = root_e;
    }

    if (!no_physics_shape(bone_data.get_shape()))
    {
        CODEGeom* added_geom = E->last_geom();
        if (added_geom)
        {
            added_geom->set_bone_id(id);
            added_geom->set_shape_flags(bone_data.get_shape().flags);
        }
    }
#ifdef DEBUG
    if (E->last_geom())
        VERIFY(E->last_geom()->bone_id() != u16(-1));
#endif
    if (m_spliter_holder && E->has_geoms())
    {
        m_spliter_holder->AddToGeomMap(std::make_pair(id, E->last_geom()));
    }

    if (spGetingMap)
    {
        const auto c_iter = spGetingMap->find(id);
        if (spGetingMap->end() != c_iter)
        {
            c_iter->second.joint = J;
            c_iter->second.element = E;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // for (vecBonesIt it=bone_data.children.begin(); bone_data.children.end() != it; ++it)
    //  AddElementRecursive     (E,(*it)->GetSelfID(),fm_position,element_number,arg_check);
    // IBoneData &ibone_data = bone_data;
    u16 num_children = bone_data.GetNumChildren();
    for (u16 i = 0; i < num_children; ++i)
        AddElementRecursive(E, bone_data.GetChild(i).GetSelfID(), fm_position, element_number, arg_check);
    /////////////////////////////////////////////////////////////////////////////////////
    if (breakable)
    {
        if (joint_data.type == jtRigid)
        {
            CPHFracture& fracture = E->Fracture(fracture_num);
            fracture.m_bone_id = id;
            fracture.m_end_geom_num = E->numberOfGeoms();
            fracture.m_end_el_num = u16(elements.size()); // just after this el = current+1
            fracture.m_end_jt_num = u16(joints.size()); // current+1
        }
        else
        {
            if (J)
            {
                J->JointDestroyInfo()->m_end_element = u16(elements.size());
                J->JointDestroyInfo()->m_end_joint = u16(joints.size());
            }
        }
    }

    if (element_added && E->isBreakable())
        setElementSplitter(element_number, splitter_position);
#ifdef DEBUG
    bool bbb = lvis_check || (!breakable && root_e);
    if (!bbb)
    {
        IKinematics* K = m_pKinematics;

        Msg("all bones transform:--------");

        for (u16 ii = 0; ii < K->LL_BoneCount(); ++ii)
        {
            Fmatrix tr;

            tr = K->LL_GetTransform(ii);
            Log("bone ", K->LL_BoneName_dbg(ii));
            Log("bone_matrix", tr);
        }
        Log("end-------");
    }

    VERIFY3(bbb, dbg_obj->ObjectNameVisual(), "has breaking parts with no vertexes or size less than 1mm"); //
#endif
}

void CPHShell::ResetCallbacks(u16 id, Flags64& mask) { ResetCallbacksRecursive(id, u16(-1), mask); }
void CPHShell::ResetCallbacksRecursive(u16 id, u16 element, Flags64& mask)
{
    // if(elements.size()==element)  return;
    CBoneInstance& B = m_pKinematics->LL_GetBoneInstance(u16(id));
    const IBoneData& bone_data = m_pKinematics->GetBoneData(u16(id));
    const SJointIKData& joint_data = bone_data.get_IK_data();

	if (mask.is(UINT64_C(1) << (u64)id))
    {
        if ((no_physics_shape(bone_data.get_shape()) || joint_data.type == jtRigid) && element != u16(-1))
        {
            B.set_callback(bctPhysics, nullptr, cast_PhysicsElement(elements[element]));
        }
        else
        {
            element++;
            R_ASSERT2(element < elements.size(), "Out of elements!!");
            // if(elements.size()==element)  return;
            B.set_callback(bctPhysics, BonesCallback, cast_PhysicsElement(elements[element]));
            B.set_callback_overwrite(TRUE);
        }
    }

    // for (vecBonesIt it=bone_data.children.begin(); it!=bone_data.children.end(); ++it)
    //  ResetCallbacksRecursive((*it)->GetSelfID(),element,mask);
    // IBoneData &ibone_data = bone_data;
    u16 num_children = bone_data.GetNumChildren();
    for (u16 i = 0; i < num_children; ++i)
        ResetCallbacksRecursive(bone_data.GetChild(i).GetSelfID(), element, mask);
}

void CPHShell::EnabledCallbacks(BOOL val)
{
    if (val)
    {
        SetCallbacks();
        // set callback overwrite in used bones

        for (auto& it : elements)
        {
            CBoneInstance& B = m_pKinematics->LL_GetBoneInstance(it->m_SelfID);
            B.set_callback_overwrite(true);
        }
    }
    else
        ZeroCallbacks();
}

template <typename T>
void for_each_bone_id(IKinematics& K, const T& op)
{
    u16 bn = K.LL_BoneCount();
    for (u16 i = 0; i < bn; ++i)
        op(i);
}

CPHElement* get_physics_parent(IKinematics& k, u16 id)
{
    VERIFY(BI_NONE != id);

    while (true)
    {
        CBoneInstance& B = k.LL_GetBoneInstance(u16(id));
        const IBoneData& bone_data = k.GetBoneData(u16(id));
        if (B.callback_type() == bctPhysics && B.callback_param())
            return cast_PHElement(B.callback_param());

        if (k.LL_GetBoneRoot() == id)
            return nullptr;

        id = bone_data.GetParentID();

        if (BI_NONE == id)
            return nullptr;
    }
}
static u16 element_position_in_set_calbacks = u16(-1);
void CPHShell::SetCallbacks()
{
    struct set_bone_callback
    {
        void operator()(CPHElement* e) { e->SetBoneCallback(); }
    };
    std::for_each(elements.begin(), elements.end(), set_bone_callback());

    struct set_bone_reference : private Noncopyable
    {
        IKinematics& K;
        set_bone_reference(IKinematics& K_) : K(K_) {}
        void operator()(u16 id) const
        {
            CBoneInstance& bi = K.LL_GetBoneInstance(id);
            if (!bi.callback() || bi.callback_type() != bctPhysics)
            {
                CPHElement* root_e = get_physics_parent(K, id);
                if (root_e && K.LL_GetBoneVisible(id))
                    bi.set_callback(bctPhysics, nullptr, cast_PhysicsElement(root_e));
            }
        }
    };
    for_each_bone_id(*PKinematics(), set_bone_reference(*PKinematics()));

    // element_position_in_set_calbacks=u16(-1);

    // SetCallbacksRecursive(m_pKinematics->LL_GetBoneRoot(),element_position_in_set_calbacks);
}

void CPHShell::SetCallbacksRecursive(u16 id, u16 element)
{
    VERIFY(false);
    // if(elements.size()==element)  return;
    CBoneInstance& B = m_pKinematics->LL_GetBoneInstance(u16(id));
    const IBoneData& bone_data = m_pKinematics->GetBoneData(u16(id));
    const SJointIKData& joint_data = bone_data.get_IK_data();
    Flags64 mask;
    mask.assign(m_pKinematics->LL_GetBonesVisible());
	if (mask.is(UINT64_C(1) << (u64)id))
    {
        if ((no_physics_shape(bone_data.get_shape()) || joint_data.type == jtRigid) && element != u16(-1))
        {
            B.set_callback(bctPhysics, nullptr, cast_PhysicsElement(elements[element]));
        }
        else
        {
            element_position_in_set_calbacks++;
            element = element_position_in_set_calbacks;
            R_ASSERT2(element < elements.size(), "Out of elements!!");
            // if(elements.size()==element)  return;
            B.set_callback(bctPhysics, BonesCallback, cast_PhysicsElement(elements[element]));
            // B.Callback_overwrite=TRUE;
        }
    }

    // for (vecBonesIt it=bone_data.children.begin(); it!=bone_data.children.end(); ++it)
    //  SetCallbacksRecursive((*it)->GetSelfID(),element);
    // IBoneData &ibone_data = bone_data;
    u16 num_children = bone_data.GetNumChildren();
    for (u16 i = 0; i < num_children; ++i)
        SetCallbacksRecursive(bone_data.GetChild(i).GetSelfID(), element);
}

void CPHShell::ZeroCallbacks()
{
    if (m_pKinematics)
        ZeroCallbacksRecursive(m_pKinematics->LL_GetBoneRoot());
}
void CPHShell::ZeroCallbacksRecursive(u16 id)
{
    CBoneInstance& B = m_pKinematics->LL_GetBoneInstance(u16(id));
    const IBoneData& bone_data = m_pKinematics->GetBoneData(u16(id));
    if (B.callback_type() == bctPhysics)
    {
        B.reset_callback();
    }
    // for (vecBonesIt it=bone_data.children.begin(); bone_data.children.end() != it; ++it)
    //  ZeroCallbacksRecursive      ((*it)->GetSelfID());
    // IBoneData &ibone_data = bone_data;
    u16 num_children = bone_data.GetNumChildren();
    for (u16 i = 0; i < num_children; ++i)
        ZeroCallbacksRecursive(bone_data.GetChild(i).GetSelfID());
}
void CPHShell::set_DynamicLimits(float l_limit, float w_limit)
{
    for (auto& it : elements)
        it->set_DynamicLimits(l_limit, w_limit);
}

void CPHShell::set_DynamicScales(float l_scale /* =default_l_scale */, float w_scale /* =default_w_scale */)
{
    for (auto& it : elements)
        it->set_DynamicScales(l_scale, w_scale);
}

void CPHShell::set_DisableParams(const SAllDDOParams& params)
{
    for (auto& it : elements)
        it->set_DisableParams(params);
}

void CPHShell::UpdateRoot()
{
    auto element = elements.front();
    if (!element->isFullActive())
        return;

    element->InterpolateGlobalTransform(&mXFORM);
}

Fmatrix& CPHShell::get_animation_root_matrix(Fmatrix& m) { return m; }
void CPHShell::InterpolateGlobalTransform(Fmatrix* m)
{
    // if(!CPHObject::is_active()&&!CPHObject::NetInterpolation()) return;

    for (auto& it : elements)
        it->InterpolateGlobalTransform(&it->mXFORM);

    m->set(root_element().mXFORM);

    m->mulB_43(m_object_in_root);

    mXFORM.set(*m);

    VERIFY2(_valid(*m), "not valide transform");

    IPhysicsShellHolder* ref_object = elements.front()->PhysicsRefObject();
    if (ref_object && m_active_count < 0)
    {
        ref_object->ObjectProcessingDeactivate();
        ref_object->ObjectSpatialMove();
        m_active_count = 0;
    }
}

void CPHShell::GetGlobalTransformDynamic(Fmatrix* m)
{
    for (const auto& it : elements)
        it->GetGlobalTransformDynamic(&it->mXFORM);
    m->set(elements.front()->mXFORM);

    m->mulB_43(m_object_in_root);
    mXFORM.set(*m);

    VERIFY2(_valid(*m), "not valide transform");
}
void CPHShell::InterpolateGlobalPosition(Fvector* v)
{
    elements.front()->InterpolateGlobalPosition(v);

    v->add(m_object_in_root.c);

    VERIFY2(_valid(*v), "not valide result position");
}

void CPHShell::GetGlobalPositionDynamic(Fvector* v)
{
    elements.front()->GetGlobalPositionDynamic(v);
    VERIFY2(_valid(*v), "not valide result position");
}

void CPHShell::ObjectToRootForm(const Fmatrix& form)
{
    Fmatrix M;
    Fmatrix ILF;
    elements.front()->InverceLocalForm(ILF);
    M.mul(m_object_in_root, ILF);
    M.invert();
    mXFORM.mul(form, M);
    VERIFY2(_valid(form), "not valide transform");
}

CPhysicsElement* CPHShell::NearestToPoint(const Fvector& point, NearestToPointCallback* cb /*=0*/)
{
    auto i = elements.begin();
    auto e = elements.end();
    float min_distance = dInfinity;
    CPHElement* nearest_element = nullptr;
    for (; i != e; ++i)
    {
        Fvector tmp;
        float distance;
        if (cb && !(*cb)(*i))
            continue;
        (*i)->GetGlobalPositionDynamic(&tmp);
        tmp.sub(point);
        distance = tmp.magnitude();
        if (distance < min_distance)
        {
            min_distance = distance;
            nearest_element = *i;
        }
    }
    return nearest_element;
}
void CPHShell::CreateSpace()
{
    if (!m_space)
    {
        m_space = dSimpleSpaceCreate(nullptr);
        dSpaceSetCleanup(m_space, 0);
    }
}
void CPHShell::PassEndElements(u16 from, u16 to, CPHShell* dest)
{
    auto i_from = elements.begin() + from, e = elements.begin() + to;
    if (from != to)
    {
        if (!dest->elements.empty())
            (*i_from)->set_ParentElement(dest->elements.back());
        else
            (*i_from)->set_ParentElement(nullptr);
    }
    for (auto i = i_from; i != e; ++i)
    {
        dGeomID spaced_geom = (*i)->dSpacedGeometry();
        if (spaced_geom) // for active elems
        {
            dSpaceRemove(m_space, spaced_geom);
            dSpaceAdd(dest->m_space, spaced_geom);
        }
        VERIFY(_valid(dest->mXFORM));
        (*i)->SetShell(dest);
    }
    dest->elements.insert(dest->elements.end(), i_from, e);
    elements.erase(i_from, e);
}

void CPHShell::PassEndJoints(u16 from, u16 to, CPHShell* dest)
{
    auto i_from = joints.begin() + from, e = joints.begin() + to;
    auto i = i_from;
    for (; i != e; i++)
    {
        (*i)->SetShell(dest);
    }
    dest->joints.insert(dest->joints.end(), i_from, e);
    joints.erase(i_from, e);
}

void CPHShell::DeleteElement(u16 element)
{
    elements[element]->Deactivate();
    xr_delete(elements[element]);
    elements.erase(elements.begin() + element);
}
void CPHShell::DeleteJoint(u16 joint)
{
    joints[joint]->Deactivate();
    xr_delete(joints[joint]);
    joints.erase(joints.begin() + joint);
}

void CPHShell::setEndElementSplitter()
{
    if (!elements.back()->FracturesHolder()) // adding fracture for element supposed before adding splitter. Need only
        // one splitter for an element
        AddSplitter(CPHShellSplitter::splElement, u16(elements.size() - 1), u16(joints.size() - 1));
}

void CPHShell::setElementSplitter(u16 element_number, u16 splitter_position)
{
    AddSplitter(CPHShellSplitter::splElement, element_number, element_number - 1, splitter_position);
}
void CPHShell::AddSplitter(CPHShellSplitter::EType type, u16 element, u16 joint)
{
    if (!m_spliter_holder)
        m_spliter_holder = new CPHShellSplitterHolder(this);
    m_spliter_holder->AddSplitter(type, element, joint);
}

void CPHShell::AddSplitter(CPHShellSplitter::EType type, u16 element, u16 joint, u16 position)
{
    if (!m_spliter_holder)
        m_spliter_holder = new CPHShellSplitterHolder(this);
    m_spliter_holder->AddSplitter(type, element, joint, position);
}
void CPHShell::setEndJointSplitter()
{
    if (!joints.back()->JointDestroyInfo()) // setting joint breacable supposed before adding splitter. Need only one
        // splitter for a joint
        AddSplitter(CPHShellSplitter::splJoint, u16(elements.size() - 1), u16(joints.size() - 1));
}

bool CPHShell::isBreakable() { return (m_spliter_holder && !m_spliter_holder->IsUnbreakable()); }
bool CPHShell::isFractured() { return (m_spliter_holder && m_spliter_holder->Breaked()); }
void CPHShell::SplitProcess(PHSHELL_PAIR_VECTOR& out_shels)
{
    if (!m_spliter_holder)
        return;
    m_spliter_holder->SplitProcess(out_shels);
    if (!m_spliter_holder->m_splitters.size())
        xr_delete(m_spliter_holder);
}
void CPHShell::SplitterHolderActivate()
{
    CPHShellSplitterHolder* sh = SplitterHolder();
    if (sh)
        sh->Activate();
}
void CPHShell::SplitterHolderDeactivate()
{
    CPHShellSplitterHolder* sh = SplitterHolder();
    if (sh)
        sh->Deactivate();
}

u16 CPHShell::BoneIdToRootGeom(u16 id)
{
    if (!m_spliter_holder)
        return u16(-1);
    return m_spliter_holder->FindRootGeom(id);
}

void CPHShell::SetJointRootGeom(CPhysicsElement* root_e, CPhysicsJoint* J)
{
    R_ASSERT(root_e);
    R_ASSERT(J);
    CPHElement* e = cast_PHElement(root_e);
    CPHJoint* j = static_cast<CPHJoint*>(J);

    CPHFracturesHolder* f_holder = e->FracturesHolder();
    if (!f_holder)
        return;
    j->RootGeom() = e->Geom(f_holder->LastFracture().m_start_geom_num);
}

void CPHShell::set_ApplyByGravity(bool flag)
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->set_ApplyByGravity(flag);
}

bool CPHShell::get_ApplyByGravity()
{
    if (elements.empty())
        return (false);

    VERIFY(elements.front());
    return (elements.front()->get_ApplyByGravity());
}

void CPHShell::applyGravityAccel(const Fvector& accel)
{
    if (!isActive())
        return;

    Fvector a;
    a.set(accel);
    a.mul((float)elements.size());
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->applyGravityAccel(a);
    EnableObject(nullptr);
}

void CPHShell::PlaceBindToElForms()
{
    Flags64 mask;
    mask.assign(m_pKinematics->LL_GetBonesVisible());
    PlaceBindToElFormsRecursive(Fidentity, m_pKinematics->LL_GetBoneRoot(), 0, mask);
}
void CPHShell::setTorque(const Fvector& torque)
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->setTorque(torque);
}
void CPHShell::setForce(const Fvector& force)
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->setForce(force);
}
void CPHShell::PlaceBindToElFormsRecursive(Fmatrix parent, u16 id, u16 element, Flags64& mask)
{
    CBoneData& bone_data = m_pKinematics->LL_GetData(u16(id));
    SJointIKData& joint_data = bone_data.IK_data;

	if (mask.is(UINT64_C(1) << (u64)id))
    {
        if (no_physics_shape(bone_data.shape) || joint_data.type == jtRigid && element != u16(-1))
        {
        }
        else
        {
            element++;
            R_ASSERT2(element < elements.size(), "Out of elements!!");
            // if(elements.size()==element)  return;
            CPHElement* E = (elements[element]);
            E->mXFORM.mul(parent, bone_data.bind_transform);
        }
    }
    // for (vecBonesIt it=bone_data.children.begin(); it!=bone_data.children.end(); ++it)
    //  PlaceBindToElFormsRecursive(mXFORM,(*it)->GetSelfID(),element,mask);
    IBoneData& ibone_data = bone_data;
    u16 num_children = ibone_data.GetNumChildren();
    for (u16 i = 0; i < num_children; ++i)
        PlaceBindToElFormsRecursive(mXFORM, ibone_data.GetChild(i).GetSelfID(), element, mask);
}

void CPHShell::BonesBindCalculate(u16 id_from) { BonesBindCalculateRecursive(Fidentity, 0); }
void CPHShell::BonesBindCalculateRecursive(Fmatrix parent, u16 id)
{
    CBoneInstance& bone_instance = m_pKinematics->LL_GetBoneInstance(id);
    CBoneData& bone_data = m_pKinematics->LL_GetData(u16(id));

    bone_instance.mTransform.mul(parent, bone_data.bind_transform);

    //  for (vecBonesIt it=bone_data.children.begin(); it!=bone_data.children.end(); ++it)
    //      BonesBindCalculateRecursive(bone_instance.mTransform,(*it)->GetSelfID());
    IBoneData& ibone_data = bone_data;
    u16 num_children = ibone_data.GetNumChildren();
    for (u16 i = 0; i < num_children; ++i)
        BonesBindCalculateRecursive(bone_instance.mTransform, ibone_data.GetChild(i).GetSelfID());
}

void CPHShell::AddTracedGeom(u16 element /*=0*/, u16 geom /*=0*/)
{
    CODEGeom* g = elements[element]->Geom(geom);
    g->set_ph_object(this);
    m_traced_geoms.add(g);
    EnableGeomTrace();
}
void CPHShell::SetAllGeomTraced()
{
    auto b = elements.begin();
    auto e = elements.end();
    for (auto i = b; i != e; ++i)
    {
        u16 gn = (*i)->numberOfGeoms();
        for (u16 j = 0; j < gn; ++j)
            AddTracedGeom(u16(i - b), j);
    }
}

void CPHShell::ClearTracedGeoms()
{
    m_traced_geoms.clear();
    DisableGeomTrace();
}

void CPHShell::DisableGeomTrace() { CPHObject::UnsetRayMotions(); }
void CPHShell::EnableGeomTrace()
{
    if (!m_traced_geoms.empty())
        CPHObject::SetRayMotions();
}

void CPHShell::SetPrefereExactIntegration() { CPHObject::SetPrefereExactIntegration(); }
void CPHShell::add_Element(CPhysicsElement* E)
{
    CPHElement* ph_element = cast_PHElement(E);
    ph_element->SetShell(this);
    elements.push_back(ph_element);
}

void CPHShell::add_Joint(CPhysicsJoint* J)
{
    if (!J)
        return;
    joints.push_back(static_cast<CPHJoint*>(J));
    joints.back()->SetShell(this);
}

CODEGeom* CPHShell::get_GeomByID(u16 bone_id)
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
    {
        CODEGeom* ret = (*i)->GeomByBoneID(bone_id);
        if (ret)
            return ret;
    }
    return nullptr;
}

void CPHShell::PureStep(float step)
{
    // CPHObject::Island().Step(step);
    // PhDataUpdate(step);
    CPHObject::step(step);
}
void CPHShell::CollideAll()
{
    CPHObject::Collide();
    CPHObject::reinit_single();
}

void CPHShell::RegisterToCLGroup(CGID g) { CPHCollideValidator::RegisterObjToGroup(g, *static_cast<CPHObject*>(this)); }
bool CPHShell::IsGroupObject() { return CPHCollideValidator::IsGroupObject(*this); };
void CPHShell::SetIgnoreStatic() { CPHCollideValidator::SetStaticNotCollide(*this); }
void CPHShell::SetIgnoreDynamic() { CPHCollideValidator::SetDynamicNotCollide(*this); }
void CPHShell::SetRagDoll() { CPHCollideValidator::SetRagDollClass(*this); }
void CPHShell::SetIgnoreRagDoll() { CPHCollideValidator::SetRagDollClassNotCollide(*this); }
//Makes this physical object animated
void CPHShell::CreateShellAnimator(CInifile const* ini, LPCSTR section)
{
    //For the collision filter, we refer this object to the class of animated objects
    CPHCollideValidator::SetAnimatedClass(*this);
    m_pPhysicsShellAnimatorC = new CPhysicsShellAnimator(this, ini, section);
    VERIFY(PhysicsRefObject());
    PhysicsRefObject()->ObjectProcessingActivate();
    // m_pPhysicsShellAnimatorC->ResetCallbacks();
}

//Configures the collision filter to ignore the collision
//of this physical object with an animated physical object
void CPHShell::SetIgnoreAnimated()
{
    //For the collision filter, we indicate that this
    //physical object ignores the animated physical objects (bodies)

    CPHCollideValidator::SetAnimatedClassNotCollide(*this);
}

//Displays information about whether the object is animated

void CPHShell::SetSmall() { CPHCollideValidator::SetClassSmall(*this); }
void CPHShell::SetIgnoreSmall() { CPHCollideValidator::SetClassSmallNotCollide(*this); }
void CPHShell::CutVelocity(float l_limit, float a_limit)
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->CutVelocity(l_limit, a_limit);
}

void CPHShell::ClearRecentlyDeactivated() { ClearCashedTries(); }
void CPHShell::ClearCashedTries()
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->clear_cashed_tries();
}

void CPHShell::get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const
{
    t_get_extensions(elements, axis, center_prg, lo_ext, hi_ext);
    /*
    lo_ext=dInfinity;hi_ext=-dInfinity;
    ELEMENT_CI i=elements.begin(),e=elements.end();
    for(;i!=e;++i)
    {
        float temp_lo_ext,temp_hi_ext;
        (*i)->get_Extensions(axis,center_prg,temp_lo_ext,temp_hi_ext);
        if(lo_ext>temp_lo_ext)lo_ext=temp_lo_ext;
        if(hi_ext<temp_hi_ext)hi_ext=temp_hi_ext;
    }
*/
}

const CGID& CPHShell::GetCLGroup() const { return CPHCollideValidator::GetGroup(*this); }
void* CPHShell::get_CallbackData()
{
    VERIFY(isActive());
    return elements.front()->get_CallbackData();
}

void CPHShell::SetBonesCallbacksOverwrite(bool v)
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->SetBoneCallbackOverwrite(v);
}

void CPHShell::ToAnimBonesPositions(motion_history_state history_state)
{
    VERIFY(PKinematics());

    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->ToBonePos(&PKinematics()->LL_GetBoneInstance((*i)->m_SelfID), history_state);
}

bool CPHShell::AnimToVelocityState(float dt, float l_limit, float a_limit)
{
    auto i = elements.begin();
    auto e = elements.end();
    bool ret = true;
    for (; i != e; ++i)
        ret = (*i)->AnimToVel(dt, l_limit, a_limit) && ret;
    return ret;
}

void CPHShell::SetAnimated(bool v)
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->SetAnimated(v);
}

void CPHShell::AnimatorOnFrame()
{
    VERIFY(PPhysicsShellAnimator());
    PPhysicsShellAnimator()->OnFrame();
}

#ifdef DEBUG

void CPHShell::dbg_draw_velocity(float scale, u32 color)
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->dbg_draw_velocity(scale, color);
}
void CPHShell::dbg_draw_force(float scale, u32 color)
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->dbg_draw_force(scale, color);
}

void CPHShell::dbg_draw_geometry(float scale, u32 color, Flags32 flags /*= Flags32().assign( 0 )*/) const
{
    auto i = elements.begin();
    auto e = elements.end();
    for (; i != e; ++i)
        (*i)->dbg_draw_geometry(scale, color, flags);
}

#endif
