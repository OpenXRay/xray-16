#include "StdAfx.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "tri-colliderknoopc/dTriList.h"
#include "PHFracture.h"
#include "PHContactBodyEffector.h"
#include "MathUtils.h"
#include "matrix_utils.h"
#include "IPhysicsShellHolder.h"
#include "ph_valid_ode.h"

#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "ode/ode/src/util.h"

#ifdef DEBUG
#include "debug_output.h"

#endif // DEBUG

///////////////////////////////////////////////////////////////
#pragma warning(push)
#pragma warning(disable : 4995)
#pragma warning(disable : 4267)
#include "ode/ode/src/collision_kernel.h"
#pragma warning(pop)
///////////////////////////////////////////////////////////////////

#include "ExtendedGeom.h"

#include "PHShell.h"
#include "PHElement.h"
#include "PHElementInline.h"
extern CPHWorld* ph_world;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////Implementation for CPhysicsElement
CPHElement::CPHElement() // aux
{
    m_w_limit = default_w_limit;
    m_l_limit = default_l_limit;
    m_l_scale = default_l_scale;
    m_w_scale = default_w_scale;

    // push_untill=0;

    // temp_for_push_out=NULL;

    m_body = NULL;
    // bActive=false;
    // bActivating=false;
    m_flags.set(flActive, FALSE);
    m_flags.set(flActivating, FALSE);
    m_parent_element = NULL;
    m_shell = NULL;

    k_w = default_k_w;
    k_l = default_k_l; // 1.8f;
    m_fratures_holder = NULL;
    // b_enabled_onstep=false;
    // m_flags.set(flEnabledOnStep,FALSE);
    m_flags.assign(0);
    mXFORM.identity();
    m_mass.setZero();
    m_mass_center.set(0, 0, 0);
    m_volume = 0.f;
}

void CPHElement::add_Box(const Fobb& V) { CPHGeometryOwner::add_Box(V); }
void CPHElement::add_Sphere(const Fsphere& V) { CPHGeometryOwner::add_Sphere(V); }
void CPHElement::add_Cylinder(const Fcylinder& V) { CPHGeometryOwner::add_Cylinder(V); }
void CPHElement::build()
{
    m_body = dBodyCreate(0); // phWorld
    // m_saved_contacts=dJointGroupCreate (0);
    // b_contacts_saved=false;
    dBodyDisable(m_body);
    // dBodySetFiniteRotationMode(m_body,1);
    // dBodySetFiniteRotationAxis(m_body,0,0,0);
    VERIFY2(dMass_valide(&m_mass), "Element has bad mass");
    if (m_geoms.empty())
    {
        Fix();
    }
    else
    {
        VERIFY2(m_mass.mass > 0.f, "Element has bad mass");
        dBodySetMass(m_body, &m_mass);
    }

    VERIFY_BOUNDARIES2(m_mass_center, phBoundaries, PhysicsRefObject(), "m_mass_center");

    dBodySetPosition(m_body, m_mass_center.x, m_mass_center.y, m_mass_center.z);

    CPHDisablingTranslational::Reinit();
    ///////////////////////////////////////////////////////////////////////////////////////
    CPHGeometryOwner::build();
    set_body(m_body);
}

void CPHElement::RunSimulation()
{
    // if(push_untill)
    // push_untill+=Device.dwTimeGlobal;

    if (group_space())
        dSpaceAdd(m_shell->dSpace(), (dGeomID)group_space());

    // else
    //	if(!m_geoms.empty())(*m_geoms.begin())->add_to_space(m_shell->dSpace());
    if (!m_body->world)
    {
        // dWorldAddBody(phWorld, m_body);
        m_shell->Island().AddBody(m_body);
    }
    dBodyEnable(m_body);
}

void CPHElement::destroy()
{
    // dJointGroupDestroy(m_saved_contacts);
    CPHGeometryOwner::destroy();
    if (m_body) //&&m_body->world
    {
        if (m_body->world)
            m_shell->Island().RemoveBody(m_body);
        dBodyDestroy(m_body);
        m_body = NULL;
    }
    DestroyGroupSpace();
}

void CPHElement::calculate_it_data(const Fvector& mc, float mas)
{
    float density = mas / m_volume;
    calculate_it_data_use_density(mc, density);
}

static float static_dencity;
void CPHElement::calc_it_fract_data_use_density(const Fvector& mc, float density)
{
    m_mass_center.set(mc);
    dMassSetZero(&m_mass);
    static_dencity = density;
    recursive_mass_summ(0, m_fratures_holder->m_fractures.begin());
}
void CPHElement::set_local_mass_center(const Fvector& mc)
{
    m_mass_center.set(mc);
    dVectorSet(m_mass.c, cast_fp(mc));
}
dMass CPHElement::recursive_mass_summ(u16 start_geom, FRACTURE_I cur_fracture)
{
    // XXX: Review
    dMass end_mass;
    dMassSetZero(&end_mass);
    GEOM_I i_geom = m_geoms.begin() + start_geom, e = m_geoms.begin() + cur_fracture->m_start_geom_num;
    for (; i_geom != e; ++i_geom)
        (*i_geom)->add_self_mass(end_mass, m_mass_center, static_dencity);
    dMassAdd(&m_mass, &end_mass);
    start_geom = cur_fracture->m_start_geom_num;
    ++cur_fracture;
    if (m_fratures_holder->m_fractures.end() != cur_fracture)
        cur_fracture->SetMassParts(m_mass, recursive_mass_summ(start_geom, cur_fracture));
    return end_mass;
}
void CPHElement::setDensity(float M) { calculate_it_data_use_density(get_mc_data(), M); }
void CPHElement::setMass(float M) { calculate_it_data(get_mc_data(), M); }
void CPHElement::setDensityMC(float M, const Fvector& mass_center)
{
    m_mass_center.set(mass_center);
    calc_volume_data();
    calculate_it_data_use_density(mass_center, M);
}

void CPHElement::setMassMC(float M, const Fvector& mass_center)
{
    m_mass_center.set(mass_center);
    calc_volume_data();
    calculate_it_data(mass_center, M);
}

void CPHElement::Start()
{
    build();
    RunSimulation();
}

void CPHElement::Deactivate()
{
    VERIFY(isActive());

    destroy();
    m_flags.set(flActive, FALSE);
    m_flags.set(flActivating, FALSE);
    // bActive=false;
    // bActivating=false;
    IKinematics* K = m_shell->PKinematics();
    if (K)
    {
        if (K->LL_GetBoneInstance(m_SelfID).callback_type() == bctPhysics)
            ClearBoneCallback();
    }
}

void CPHElement::SetTransform(const Fmatrix& m0, motion_history_state history_state)
{
    VERIFY2(_valid(m0), make_string("invalid_form_in_set_transform") + (PhysicsRefObject()->dump(full_capped)));
    VERIFY2(valid_pos(m0.c), dbg_valide_pos_string(m0.c, PhysicsRefObject(), "invalid_form_in_set_transform"));
    Fvector mc;
    CPHGeometryOwner::get_mc_vs_transform(mc, m0);
    VERIFY_BOUNDARIES2(mc, phBoundaries, PhysicsRefObject(), "mass	center	in set transform");
    dBodySetPosition(m_body, mc.x, mc.y, mc.z);
    Fmatrix33 m33;
    m33.set(m0);
    dMatrix3 R;
    PHDynamicData::FMX33toDMX(m33, R);
    dBodySetRotation(m_body, R);
    CPHDisablingFull::Reinit();

    VERIFY2(dBodyGetPosition(m_body), "not valide safe position");
    VERIFY2(dBodyGetLinearVel(m_body), "not valide safe velocity");
    m_flags.set(flUpdate, TRUE);
    m_shell->spatial_move();
    if (history_state != mh_not_clear)
        CPHGeometryOwner::clear_motion_history(mh_unspecified == history_state);
}

void CPHElement::getQuaternion(Fquaternion& quaternion)
{
    if (!isActive())
        return;
    const float* q = dBodyGetQuaternion(m_body);
    quaternion.set(-q[0], q[1], q[2], q[3]);
    VERIFY(_valid(quaternion));
}
void CPHElement::setQuaternion(const Fquaternion& quaternion)
{
    VERIFY(_valid(quaternion));
    if (!isActive())
        return;
    dQuaternion q = {-quaternion.w, quaternion.x, quaternion.y, quaternion.z};
    dBodySetQuaternion(m_body, q);
    CPHDisablingRotational::Reinit();
    m_flags.set(flUpdate, TRUE);
    m_shell->spatial_move();
}
void CPHElement::GetGlobalPositionDynamic(Fvector* v)
{
    if (!isActive())
        return;
    v->set((*(Fvector*)dBodyGetPosition(m_body)));
    VERIFY(_valid(*v));
}

void CPHElement::SetGlobalPositionDynamic(const Fvector& position)
{
    if (!isActive())
        return;
    VERIFY(_valid(position));
    VERIFY_BOUNDARIES2(position, phBoundaries, PhysicsRefObject(), "SetGlobalPosition argument ");
    dBodySetPosition(m_body, position.x, position.y, position.z);
    CPHDisablingTranslational::Reinit();
    m_flags.set(flUpdate, TRUE);
    m_shell->spatial_move();
}

void CPHElement::TransformPosition(const Fmatrix& form, motion_history_state history_state)
{
    if (!isActive())
        return;
    VERIFY(_valid(form));
    R_ASSERT2(m_body, "body is not created");
    Fmatrix bm;
    PHDynamicData::DMXPStoFMX(dBodyGetRotation(m_body), dBodyGetPosition(m_body), bm);
    Fmatrix new_bm;
    new_bm.mul(form, bm);
    dMatrix3 dBM;
    PHDynamicData::FMXtoDMX(new_bm, dBM);
    dBodySetRotation(m_body, dBM);
    VERIFY_BOUNDARIES2(new_bm.c, phBoundaries, PhysicsRefObject(), "TransformPosition dest pos");
    dBodySetPosition(m_body, new_bm.c.x, new_bm.c.y, new_bm.c.z);
    CPHDisablingFull::Reinit();
    m_body_interpolation.ResetPositions();
    m_body_interpolation.ResetRotations();
    m_flags.set(flUpdate, TRUE);
    if (history_state != mh_not_clear)
        clear_motion_history(mh_unspecified == history_state);
    m_shell->spatial_move();
}
CPHElement::~CPHElement()
{
    VERIFY(!isActive());
    DeleteFracturesHolder();
}

void CPHElement::SetBoneCallback()
{
    IKinematics* K = m_shell->PKinematics();
    VERIFY(K);
    K->LL_GetBoneInstance(m_SelfID).set_callback(bctPhysics, m_shell->GetBonesCallback(), cast_PhysicsElement(this));
}
void CPHElement::ClearBoneCallback()
{
    IKinematics* K = m_shell->PKinematics();
    K->LL_GetBoneInstance(m_SelfID).reset_callback();
}

void CPHElement::Activate(const Fmatrix& transform, const Fvector& lin_vel, const Fvector& ang_vel, bool disable)
{
    VERIFY(!isActive());
    mXFORM.set(transform);
    Start();
    SetTransform(transform, mh_unspecified);

    dBodySetLinearVel(m_body, lin_vel.x, lin_vel.y, lin_vel.z);

    dBodySetAngularVel(m_body, ang_vel.x, ang_vel.y, ang_vel.z);
    VERIFY(dBodyStateValide(m_body));
    //	dVectorSet(m_safe_position,dBodyGetPosition(m_body));
    //	dQuaternionSet(m_safe_quaternion,dBodyGetQuaternion(m_body));
    //	dVectorSet(m_safe_velocity,dBodyGetLinearVel(m_body));

    m_body_interpolation.SetBody(m_body);

    if (disable)
        dBodyDisable(m_body);
    m_flags.set(flActive, TRUE);
    m_flags.set(flActivating, TRUE);
    if (m_shell->PKinematics())
        SetBoneCallback();
}
void CPHElement::Activate(const Fmatrix& m0, float dt01, const Fmatrix& m2, bool disable)
{
    Fvector lvel, avel;
    lvel.set(m2.c.x - m0.c.x, m2.c.y - m0.c.y, m2.c.z - m0.c.z);
    avel.set(0.f, 0.f, 0.f);
    Activate(m0, lvel, avel, disable);
}

void CPHElement::Activate(bool disable, bool /*not_set_bone_callbacks*/)
{
    Fvector lvel, avel;
    lvel.set(0.f, 0.f, 0.f);
    avel.set(0.f, 0.f, 0.f);
    Activate(mXFORM, lvel, avel, disable);
}

void CPHElement::Activate(const Fmatrix& start_from, bool disable)
{
    VERIFY(_valid(start_from));
    Fmatrix globe;
    globe.mul_43(start_from, mXFORM);

    Fvector lvel, avel;
    lvel.set(0.f, 0.f, 0.f);
    avel.set(0.f, 0.f, 0.f);
    Activate(globe, lvel, avel, disable);
}

void CPHElement::Update()
{
    if (!isActive())
        return;
    if (m_flags.test(flActivating))
        m_flags.set(flActivating, FALSE);
    if (!dBodyIsEnabled(m_body) && !m_flags.test(flUpdate) /*!bUpdate*/)
        return;

    InterpolateGlobalTransform(&mXFORM);
    VERIFY2(_valid(mXFORM), "invalid position in update");
}

void CPHElement::PhTune(dReal step)
{
    if (!isActive())
        return;
    CPHContactBodyEffector* contact_effector = (CPHContactBodyEffector*)dBodyGetData(m_body);
    if (contact_effector)
        contact_effector->Apply();
    VERIFY_BOUNDARIES2(cast_fv(dBodyGetPosition(m_body)), phBoundaries, PhysicsRefObject(), "PhTune body position");
}
void CPHElement::PhDataUpdate(dReal step)
{
    if (!isActive())
        return;

///////////////skip for disabled elements////////////////////////////////////////////////////////////
// b_enabled_onstep=!!dBodyIsEnabled(m_body);
// VERIFY_BOUNDARIES2(cast_fv(dBodyGetPosition(m_body)),phBoundaries,PhysicsRefObject(),"PhDataUpdate begin, body
// position");
#ifdef DEBUG
    if (debug_output().ph_dbg_draw_mask().test(phDbgDrawMassCenters))
    {
        debug_output().DBG_DrawPoint(cast_fv(dBodyGetPosition(m_body)), 0.03f, color_xrgb(255, 0, 0));
    }
#endif

    m_flags.set(flEnabledOnStep, !!dBodyIsEnabled(m_body));
    if (!m_flags.test(flEnabledOnStep) /*!b_enabled_onstep*/)
        return;

    //////////////////////////////////base pointers/////////////////////////////////////////////////
    const dReal* linear_velocity = dBodyGetLinearVel(m_body);
    const dReal* angular_velocity = dBodyGetAngularVel(m_body);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////scale velocity///////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VERIFY(dV_valid(linear_velocity));
#ifdef DEBUG
    if (!dV_valid(angular_velocity))
    {
        Msg("angular vel %f,%f,%f", angular_velocity[0], angular_velocity[1], angular_velocity[2]);
        Msg("linear vel %f,%f,%f", linear_velocity[0], linear_velocity[1], linear_velocity[2]);
        Msg("position  %f,%f,%f", dBodyGetPosition(m_body)[0], dBodyGetPosition(m_body)[1],
            dBodyGetPosition(m_body)[2]);
        Msg("quaternion  %f,%f,%f,%f", dBodyGetQuaternion(m_body)[0], dBodyGetQuaternion(m_body)[1],
            dBodyGetQuaternion(m_body)[2], dBodyGetQuaternion(m_body)[3]);
        Msg("matrix");
        Msg("x  %f,%f,%f", dBodyGetRotation(m_body)[0], dBodyGetRotation(m_body)[4], dBodyGetRotation(m_body)[8]);
        Msg("y  %f,%f,%f", dBodyGetRotation(m_body)[1], dBodyGetRotation(m_body)[5], dBodyGetRotation(m_body)[9]);
        Msg("z  %f,%f,%f", dBodyGetRotation(m_body)[2], dBodyGetRotation(m_body)[6], dBodyGetRotation(m_body)[10]);
        IPhysicsShellHolder* ph = PhysicsRefObject();
        Msg("name visual %s", ph->ObjectNameVisual());
        Msg("name obj %s", ph->ObjectName());
        Msg("name section %s", ph->ObjectNameSect());
        VERIFY2(0, "bad angular velocity");
    }
#endif
    VERIFY(!fis_zero(m_l_scale));
    VERIFY(!fis_zero(m_w_scale));
    dBodySetLinearVel(
        m_body, linear_velocity[0] / m_l_scale, linear_velocity[1] / m_l_scale, linear_velocity[2] / m_l_scale);
    dBodySetAngularVel(
        m_body, angular_velocity[0] / m_w_scale, angular_velocity[1] / m_w_scale, angular_velocity[2] / m_w_scale);

    ///////////////////scale changes values directly so get base values after it/////////////////////////
    /////////////////////////////base values////////////////////////////////////////////////////////////
    dReal linear_velocity_smag = dDOT(linear_velocity, linear_velocity);
    dReal linear_velocity_mag = _sqrt(linear_velocity_smag);

    dReal angular_velocity_smag = dDOT(angular_velocity, angular_velocity);
    dReal angular_velocity_mag = _sqrt(angular_velocity_smag);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////limit velocity & secure
    ////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////limit linear
    /// vel////////////////////////////////////////////////////////////////////////////////////////

    VERIFY(dV_valid(linear_velocity));
    if (linear_velocity_mag > m_l_limit)
    {
        CutVelocity(m_l_limit, m_w_limit);
        VERIFY_BOUNDARIES2(
            cast_fv(dBodyGetPosition(m_body)), phBoundaries, PhysicsRefObject(), "PhDataUpdate end, body position");
        linear_velocity_smag = dDOT(linear_velocity, linear_velocity);
        linear_velocity_mag = _sqrt(linear_velocity_smag);
        angular_velocity_smag = dDOT(angular_velocity, angular_velocity);
        angular_velocity_mag = _sqrt(angular_velocity_smag);
    }
    ////////////////secure position///////////////////////////////////////////////////////////////////////////////////
    const dReal* position = dBodyGetPosition(m_body);
    VERIFY(dV_valid(position));
    /////////////////limit & secure angular
    /// vel///////////////////////////////////////////////////////////////////////////////
    VERIFY(dV_valid(angular_velocity));

    if (angular_velocity_mag > m_w_limit)
    {
        CutVelocity(m_l_limit, m_w_limit);
        angular_velocity_smag = dDOT(angular_velocity, angular_velocity);
        angular_velocity_mag = _sqrt(angular_velocity_smag);
        linear_velocity_smag = dDOT(linear_velocity, linear_velocity);
        linear_velocity_mag = _sqrt(linear_velocity_smag);
    }

    ////////////////secure
    /// rotation////////////////////////////////////////////////////////////////////////////////////////
    {
        VERIFY(dQ_valid(dBodyGetQuaternion(m_body)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////disable///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (dBodyIsEnabled(m_body))
        Disabling();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////position
    /// update///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VERIFY(dBodyStateValide(m_body));
    VERIFY2(dV_valid(dBodyGetPosition(m_body)), "invalid body position");
    VERIFY2(dV_valid(dBodyGetQuaternion(m_body)), "invalid body rotation");
    /*
        if(!valid_pos(cast_fv(dBodyGetPosition(m_body)),phBoundaries)) //hack
        {															   //hack
            Fvector	pos;											   //hack
            m_body_interpolation.GetPosition(pos,0);				   //hack
            dBodySetPosition(m_body,pos.x,pos.y,pos.z);				   //hack
        }															   //hack
    */
    VERIFY_BOUNDARIES2(
        cast_fv(dBodyGetPosition(m_body)), phBoundaries, PhysicsRefObject(), "PhDataUpdate end, body position");
    UpdateInterpolation();

    if (!dBodyIsEnabled(m_body))
        return;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////air
    /// resistance/////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (!fis_zero(k_w))
        dBodyAddTorque(m_body, -angular_velocity[0] * k_w, -angular_velocity[1] * k_w, -angular_velocity[2] * k_w);

    dMass mass;
    dBodyGetMass(m_body, &mass);
    dReal l_air = linear_velocity_mag * k_l; // force/velocity !!!
    if (l_air > mass.mass / fixed_step)
        l_air = mass.mass / fixed_step; // validate

    if (!fis_zero(l_air))
        dBodyAddForce(m_body, -linear_velocity[0] * l_air, -linear_velocity[1] * l_air, -linear_velocity[2] * l_air);
}

void CPHElement::Enable()
{
    if (!isActive())
        return;
    m_shell->EnableObject(0);
    if (dBodyIsEnabled(m_body))
        return;
    dBodyEnable(m_body);
}

void CPHElement::Disable()
{
    //	return;
    if (!isActive() || !dBodyIsEnabled(m_body))
        return;
    FillInterpolation();

    dBodyDisable(m_body);
}

void CPHElement::ReEnable()
{
    // dJointGroupEmpty(m_saved_contacts);
    // b_contacts_saved=false;
}

void CPHElement::Freeze()
{
    if (!m_body)
        return;

    m_flags.set(flWasEnabledBeforeFreeze, !!dBodyIsEnabled(m_body));
    dBodyDisable(m_body);
}
void CPHElement::UnFreeze()
{
    if (!m_body)
        return;
    if (m_flags.test(flWasEnabledBeforeFreeze) /*was_enabled_before_freeze*/)
        dBodyEnable(m_body);
}

bool dbg_draw_ph_force_apply = false;

void CPHElement::applyImpulseVsMC(const Fvector& pos, const Fvector& dir, float val)
{
    if (!isActive() || m_flags.test(flFixed))
        return;
    if (!dBodyIsEnabled(m_body))
        dBodyEnable(m_body);
    /////////////////////////////////////////////////////////////////////////
    Fvector impulse;
    val /= fixed_step;
    impulse.set(dir);
    impulse.mul(val);
#ifdef DEBUG
    if (dbg_draw_ph_force_apply)
    {
        Fvector draw_pos;
        draw_pos.add(cast_fv(dBodyGetPosition(m_body)), pos);
        debug_output().DBG_OpenCashedDraw();
        debug_output().DBG_DrawLine(draw_pos, Fvector().add(draw_pos, dir), color_xrgb(255, 0, 0));
        debug_output().DBG_ClosedCashedDraw(50000);
    }
#endif
    dBodyAddForceAtRelPos(m_body, impulse.x, impulse.y, impulse.z, pos.x, pos.y, pos.z);
    BodyCutForce(m_body, m_l_limit, m_w_limit);
    ////////////////////////////////////////////////////////////////////////
}
void CPHElement::applyImpulseVsGF(const Fvector& pos, const Fvector& dir, float val)
{
    VERIFY(_valid(pos) && _valid(dir) && _valid(val));
    if (!isActive() || m_flags.test(flFixed))
        return;
    if (!dBodyIsEnabled(m_body))
        dBodyEnable(m_body);
/////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
    if (dbg_draw_ph_force_apply)
    {
        debug_output().DBG_OpenCashedDraw();
        debug_output().DBG_DrawLine(pos, Fvector().add(pos, dir), color_xrgb(255, 05, 0));
        debug_output().DBG_ClosedCashedDraw(50000);
    }
#endif
    /////////////////////////////////////////////////////////////////////////
    Fvector impulse;
    val /= fixed_step;
    impulse.set(dir);
    impulse.mul(val);
    dBodyAddForceAtPos(m_body, impulse.x, impulse.y, impulse.z, pos.x, pos.y, pos.z);
    BodyCutForce(m_body, m_l_limit, m_w_limit);
    VERIFY(dBodyStateValide(m_body));
    ////////////////////////////////////////////////////////////////////////
}
void CPHElement::applyImpulseTrace(const Fvector& pos, const Fvector& dir, float val, u16 id)
{
    VERIFY(_valid(pos) && _valid(dir) && _valid(val));
    if (!isActive() || m_flags.test(flFixed))
        return;
    Fvector body_pos;
    if (id != BI_NONE)
    {
        if (id == m_SelfID)
        {
            body_pos.sub(pos, m_mass_center);
        }
        else
        {
            IKinematics* K = m_shell->PKinematics();
            if (K)
            {
                Fmatrix()
                    .mul_43(Fmatrix().invert(K->LL_GetTransform(m_SelfID)), K->LL_GetTransform(id))
                    .transform(body_pos, pos);
                body_pos.sub(m_mass_center);
            }
            else
            {
                body_pos.set(0.f, 0.f, 0.f);
            }
        }
    }
    else
    {
        body_pos.set(0.f, 0.f, 0.f);
    }
#ifdef DEBUG
    if (debug_output().ph_dbg_draw_mask().test(phHitApplicationPoints))
    {
        debug_output().DBG_OpenCashedDraw();
        Fvector dbg_position;
        dbg_position.set(body_pos);
        dMULTIPLY0_331(cast_fp(dbg_position), dBodyGetRotation(m_body), cast_fp(body_pos));
        dbg_position.add(cast_fv(dBodyGetPosition(m_body)));
        debug_output().DBG_DrawPoint(dbg_position, 0.01f, color_xrgb(255, 255, 255));
        debug_output().DBG_DrawLine(cast_fv(dBodyGetPosition(m_body)), dbg_position, color_xrgb(255, 255, 255));
        debug_output().DBG_DrawLine(
            dbg_position, Fvector().add(dbg_position, Fvector().mul(dir, 0.4f)), color_xrgb(255, 0, 255));
        debug_output().DBG_ClosedCashedDraw(10000);
    }
#endif
    applyImpulseVsMC(body_pos, dir, val);
    if (m_fratures_holder)
    {
        /// impulse.add(*((Fvector*)dBodyGetPosition(m_body)));
        Fvector impulse;
        impulse.set(dir);
        impulse.mul(val / fixed_step);
        m_fratures_holder->AddImpact(impulse, body_pos, m_shell->BoneIdToRootGeom(id));
    }
}
void CPHElement::applyImpact(const SPHImpact& I)
{
    Fvector pos;
    pos.add(I.point, m_mass_center);
    Fvector dir;
    dir.set(I.force);
    float val = I.force.magnitude();

    if (!fis_zero(val) && GeomByBoneID(I.geom))
    {
        dir.mul(1.f / val);
        applyImpulseTrace(pos, dir, val, I.geom);
    }
}
void CPHElement::InterpolateGlobalTransform(Fmatrix* m)
{
    if (!m_flags.test(flUpdate))
    {
        GetGlobalTransformDynamic(m);
        VERIFY(_valid(*m));
        return;
    }
    m_body_interpolation.InterpolateRotation(*m);
    m_body_interpolation.InterpolatePosition(m->c);
    MulB43InverceLocalForm(*m);
    m_flags.set(flUpdate, FALSE);
    VERIFY(_valid(*m));
}
void CPHElement::GetGlobalTransformDynamic(Fmatrix* m) const
{
    PHDynamicData::DMXPStoFMX(dBodyGetRotation(m_body), dBodyGetPosition(m_body), *m);
    MulB43InverceLocalForm(*m);
    VERIFY(_valid(*m));
}

void CPHElement::InterpolateGlobalPosition(Fvector* v)
{
    m_body_interpolation.InterpolatePosition(*v);
    VERIFY(_valid(*v));
}

void CPHElement::build(bool disable)
{
    if (isActive())
        return;
    // bActive=true;
    // bActivating=true;
    m_flags.set(flActive, TRUE);
    m_flags.set(flActivating, TRUE);
    build();
    //	if(place_current_forms)
    {
        SetTransform(mXFORM, mh_unspecified);
    }

    m_body_interpolation.SetBody(m_body);
    // previous_f[0]=dInfinity;
    if (disable)
        dBodyDisable(m_body);
}

void CPHElement::RunSimulation(const Fmatrix& start_from)
{
    RunSimulation();
    //	if(place_current_forms)
    {
        Fmatrix globe;
        globe.mul(start_from, mXFORM);
        SetTransform(globe, mh_unspecified);
    }
    // dVectorSet(m_safe_position,dBodyGetPosition(m_body));
    // dQuaternionSet(m_safe_quaternion,dBodyGetQuaternion(m_body));
    // dVectorSet(m_safe_velocity,dBodyGetLinearVel(m_body));
}

void CPHElement::StataticRootBonesCallBack(CBoneInstance* B)
{
    VERIFY(false);
    Fmatrix parent;
    VERIFY2(isActive(), "the element is not active");
    VERIFY(_valid(m_shell->mXFORM));
    // VERIFY2(fsimilar(DET(B->mTransform),1.f,DET_CHECK_EPS),"Bones callback resive 0 matrix");
    VERIFY_RMATRIX(B->mTransform);
    VERIFY(valid_pos(B->mTransform.c, phBoundaries));
    if (m_flags.test(flActivating))
    {
        // if(!dBodyIsEnabled(m_body))
        //	dBodyEnable(m_body);
        VERIFY(!ph_world->Processing());
        VERIFY(_valid(B->mTransform));
        VERIFY(!m_shell->dSpace()->lock_count);
        mXFORM.set(B->mTransform);
        // m_start_time=Device.fTimeGlobal;
        Fmatrix global_transform;
        // if(m_parent_element)
        global_transform.mul_43(m_shell->mXFORM, mXFORM);
        SetTransform(global_transform, mh_unspecified);

        FillInterpolation();
        // bActivating=false;
        m_flags.set(flActivating, FALSE);
        if (!m_parent_element)
        {
            m_shell->m_object_in_root.set(mXFORM);
            m_shell->m_object_in_root.invert();
            m_shell->SetNotActivating();
        }
        B->set_callback_overwrite(TRUE);
        // VERIFY2(fsimilar(DET(B->mTransform),1.f,DET_CHECK_EPS),"Bones callback returns 0 matrix");
        VERIFY_RMATRIX(B->mTransform);
        VERIFY(valid_pos(B->mTransform.c, phBoundaries));
        // return;
    }

    // VERIFY2(fsimilar(DET(B->mTransform),1.f,DET_CHECK_EPS),"Bones callback returns 0 matrix");
    VERIFY_RMATRIX(B->mTransform);
    VERIFY(valid_pos(B->mTransform.c, phBoundaries));
    // if( !m_shell->is_active() && !m_flags.test(flUpdate)/*!bUpdate*/ ) return;

    {
        // InterpolateGlobalTransform(&mXFORM);
        parent.invert(m_shell->mXFORM);
        B->mTransform.mul_43(parent, mXFORM);
    }
    // VERIFY2(fsimilar(DET(B->mTransform),1.f,DET_CHECK_EPS),"Bones callback returns 0 matrix");
    VERIFY_RMATRIX(B->mTransform);
    VERIFY(valid_pos(B->mTransform.c, phBoundaries));
    VERIFY2(_valid(B->mTransform), "Bones callback returns bad matrix");
    // else
    //{

    //	InterpolateGlobalTransform(&m_shell->mXFORM);
    //	mXFORM.identity();
    //	B->mTransform.set(mXFORM);
    // parent.set(B->mTransform);
    // parent.invert();
    // m_shell->mXFORM.mulB(parent);

    //}
}

// void CPHElement::BoneGlPos(Fmatrix &m, const CBoneInstance* B) const
void CPHElement::BoneGlPos(Fmatrix& m, const Fmatrix& BoneTransform) const
{
    VERIFY(m_shell);
    m.mul_43(m_shell->mXFORM, BoneTransform);
}

void CPHElement::GetAnimBonePos(Fmatrix& bp)
{
    VERIFY(m_shell->PKinematics());
    IKinematics* pK = m_shell->PKinematics();
    // IKinematicsAnimated *ak = pK->dcast_PKinematicsAnimated();
    // VERIFY(ak);
    CBoneInstance* BI = &pK->LL_GetBoneInstance(m_SelfID);
    if (!BI->callback()) //.
    {
        bp.set(BI->mTransform);
        return;
    }

    pK->Bone_GetAnimPos(bp, m_SelfID, u8(-1), true);
}

IC bool put_in_range(Fvector& v, float range)
{
    VERIFY(range > EPS_S);
    float sq_mag = v.square_magnitude();
    if (sq_mag > range * range)
    {
        float mag = _sqrt(sq_mag);
        v.mul(range / mag);
        return true;
    }
    return false;
}

bool CPHElement::AnimToVel(float dt, float l_limit, float a_limit)
{
    VERIFY(m_shell);
    VERIFY(m_shell->PKinematics());
    // CBoneInstance *BI = &m_shell->PKinematics()->LL_GetBoneInstance(m_SelfID);
    //
    //	Fmatrix bp;BoneGlPos(bp,BI);
    //
    IPhysicsShellHolder* ph = PhysicsRefObject();
    VERIFY(ph);
    Fmatrix bpl;
    GetAnimBonePos(bpl);
    Fmatrix bp;
    bp.mul_43(ph->ObjectXFORM(), bpl);
    // BoneGlPos(bp,BI);

    Fmatrix cp;
    GetGlobalTransformDynamic(&cp);

    // Fquaternion q0; q0.set(cp);

    cp.invert();
    Fmatrix diff;
    diff.mul_43(cp, bp);
    if (dt < EPS_S)
        dt = EPS_S;
    Fvector mc1;
    CPHGeometryOwner::get_mc_vs_transform(mc1, bp);
    Fvector mc0 = cast_fv(dBodyGetPosition(m_body));
    // Fvector mc1;diff.transform_tiny(mc1,mc0);
    Fvector lv;
    linear_diff(lv, mc1, mc0, dt);
    Fvector aw;
    angular_diff(aw, diff, dt);

    // Fquaternion q1; q1.set(bp);
    // twoq_2w(q0,q1,dt,aw);
    float aw_sqm = aw.square_magnitude();
    float lv_sqm = lv.square_magnitude();
    bool ret = aw_sqm < a_limit * a_limit && lv_sqm < l_limit * l_limit;

    put_in_range(lv, l_limit);
    put_in_range(aw, a_limit);

    VERIFY(_valid(lv));
    VERIFY(_valid(aw));

    dBodySetLinearVel(m_body, lv.x, lv.y, lv.z);
    dBodySetAngularVel(m_body, aw.x, aw.y, aw.z);
    // set_LinearVel(lv);
    // set_AngularVel(aw);
    return ret;
}
void CPHElement::ToBonePos(const Fmatrix& BoneTransform, motion_history_state history_state)
{
    VERIFY2(!ph_world->Processing(), PhysicsRefObject()->ObjectNameSect());
    VERIFY(_valid(BoneTransform));
    VERIFY(!m_shell->dSpace()->lock_count);

    // mXFORM.set( B->mTransform );

    // Fmatrix global_transform;
    BoneGlPos(mXFORM, BoneTransform);
    SetTransform(mXFORM, history_state);
    FillInterpolation();
}
void CPHElement::ToBonePos(const CBoneInstance* B, motion_history_state history_state)
{
    VERIFY(B);
    ToBonePos(B->mTransform, history_state);
}

void CPHElement::SetBoneCallbackOverwrite(bool v)
{
    VERIFY(m_shell);
    VERIFY(m_shell->PKinematics());
    m_shell->PKinematics()->LL_GetBoneInstance(m_SelfID).set_callback_overwrite(v);
}

void CPHElement::BonesCallBack(CBoneInstance* B)
{
    VERIFY(isActive());
    VERIFY(_valid(m_shell->mXFORM));
    // VERIFY2(fsimilar(DET(B->mTransform),1.f,DET_CHECK_EPS),"Bones callback receive 0 matrix");
    VERIFY_RMATRIX(B->mTransform);
    VERIFY_BOUNDARIES2(B->mTransform.c, phBoundaries, PhysicsRefObject(), "BonesCallBack incoming bone position");

    if (m_flags.test(flActivating))
    {
        ActivatingPos(B->mTransform);
        B->set_callback_overwrite(TRUE);
    }

    VERIFY_RMATRIX(B->mTransform);
    VERIFY(valid_pos(B->mTransform.c, phBoundaries));

    CalculateBoneTransform(B->mTransform);

    //	Fmatrix parent;
    //	parent.invert		( m_shell->mXFORM );
    //	B->mTransform.mul_43( parent, mXFORM );

    VERIFY_RMATRIX(B->mTransform);
    VERIFY(valid_pos(B->mTransform.c, phBoundaries));
    VERIFY2(_valid(B->mTransform), "Bones callback returns bad matrix");
}

void CPHElement::set_PhysicsRefObject(IPhysicsShellHolder* ref_object)
{
    CPHGeometryOwner::set_PhysicsRefObject(ref_object);
}

void CPHElement::set_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
    CPHGeometryOwner::set_ObjectContactCallback(callback);
}
void CPHElement::add_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
    CPHGeometryOwner::add_ObjectContactCallback(callback);
}
void CPHElement::remove_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
    CPHGeometryOwner::remove_ObjectContactCallback(callback);
}
ObjectContactCallbackFun* CPHElement::get_ObjectContactCallback()
{
    return CPHGeometryOwner::get_ObjectContactCallback();
}
void CPHElement::set_CallbackData(void* cd) { CPHGeometryOwner::set_CallbackData(cd); }
void* CPHElement::get_CallbackData() { return CPHGeometryOwner::get_CallbackData(); }
void CPHElement::set_ContactCallback(ContactCallbackFun* callback)
{
    // push_untill=0;
    CPHGeometryOwner::set_ContactCallback(callback);
}

void CPHElement::SetMaterial(u16 m) { CPHGeometryOwner::SetMaterial(m); }
dMass* CPHElement::getMassTensor() // aux
{
    return &m_mass;
}

void CPHElement::setInertia(const dMass& M)
{
    m_mass = M;
    if (!isActive() || m_flags.test(flFixed))
        return;
    dBodySetMass(m_body, &M);
}

void CPHElement::addInertia(const dMass& M)
{
    dMassAdd(&m_mass, &M);
    if (!isActive())
        return;
    dBodySetMass(m_body, &m_mass);
}
void CPHElement::get_LinearVel(Fvector& velocity) const
{
    if (!isActive() || (!m_flags.test(flAnimated) && !dBodyIsEnabled(m_body)))
    {
        velocity.set(0, 0, 0);
        return;
    }
    dVectorSet((dReal*)&velocity, dBodyGetLinearVel(m_body));
}
void CPHElement::get_AngularVel(Fvector& velocity) const
{
    if (!isActive() || (!m_flags.test(flAnimated) && !dBodyIsEnabled(m_body)))
    {
        velocity.set(0, 0, 0);
        return;
    }
    dVectorSet((dReal*)&velocity, dBodyGetAngularVel(m_body));
}

void CPHElement::set_LinearVel(const Fvector& velocity)
{
    if (!isActive() || m_flags.test(flFixed))
        return;
    VERIFY2(_valid(velocity), "not valid arqument velocity");
    Fvector vel = velocity;
#ifdef DEBUG
    if (velocity.magnitude() > m_l_limit)
        Msg(" CPHElement::set_LinearVel set velocity magnitude is too large %f", velocity.magnitude());
#endif
    put_in_range(vel, m_l_limit);
    dBodySetLinearVel(m_body, vel.x, vel.y, vel.z);
    // dVectorSet(m_safe_velocity,dBodyGetLinearVel(m_body));
}

void CPHElement::set_AngularVel(const Fvector& velocity)
{
    VERIFY(_valid(velocity));
    if (!isActive() || m_flags.test(flFixed))
        return;

    Fvector vel = velocity;
#ifdef DEBUG
    if (velocity.magnitude() > m_w_limit)
        Msg("CPHElement::set_AngularVel set velocity magnitude is too large %f", velocity.magnitude());
#endif
    put_in_range(vel, m_w_limit);
    dBodySetAngularVel(m_body, vel.x, vel.y, vel.z);
}

void CPHElement::getForce(Fvector& force)
{
    if (!isActive())
        return;
    force.set(*(Fvector*)dBodyGetForce(m_body));
    VERIFY(dBodyStateValide(m_body));
}
void CPHElement::getTorque(Fvector& torque)
{
    if (!isActive())
        return;
    torque.set(*(Fvector*)dBodyGetTorque(m_body));
    VERIFY(dBodyStateValide(m_body));
}
void CPHElement::setForce(const Fvector& force)
{
    if (!isActive() || m_flags.test(flFixed))
        return;
    if (!dBodyIsEnabled(m_body))
        dBodyEnable(m_body);
    m_shell->EnableObject(0);
    dBodySetForce(m_body, force.x, force.y, force.z);
    BodyCutForce(m_body, m_l_limit, m_w_limit);
    VERIFY(dBodyStateValide(m_body));
}
void CPHElement::setTorque(const Fvector& torque)
{
    if (!isActive() || m_flags.test(flFixed))
        return;
    if (!dBodyIsEnabled(m_body))
        dBodyEnable(m_body);
    m_shell->EnableObject(0);
    dBodySetTorque(m_body, torque.x, torque.y, torque.z);
    BodyCutForce(m_body, m_l_limit, m_w_limit);
    VERIFY(dBodyStateValide(m_body));
}

void CPHElement::applyForce(const Fvector& dir, float val) // aux
{
    applyForce(dir.x * val, dir.y * val, dir.z * val);
}
void CPHElement::applyForce(float x, float y, float z) // called anywhere ph state influent
{
    VERIFY(_valid(x) && _valid(y) && _valid(z));
    if (!isActive())
        return; // hack??
    if (m_flags.test(flFixed))
        return;
    if (!dBodyIsEnabled(m_body))
        dBodyEnable(m_body);
    m_shell->EnableObject(0);
    dBodyAddForce(m_body, x, y, z);
    BodyCutForce(m_body, m_l_limit, m_w_limit);
    VERIFY(dBodyStateValide(m_body));
}

void CPHElement::applyImpulse(const Fvector& dir, float val) // aux
{
    applyForce(dir.x * val / fixed_step, dir.y * val / fixed_step, dir.z * val / fixed_step);
}

void CPHElement::add_Shape(const SBoneShape& shape, const Fmatrix& offset)
{
    CPHGeometryOwner::add_Shape(shape, offset);
}

void CPHElement::add_Shape(const SBoneShape& shape) { CPHGeometryOwner::add_Shape(shape); }
void CPHElement::add_geom(CODEGeom* g)
{
    Fmatrix gf;
    g->get_xform(gf);

    Fmatrix bf;
    PHDynamicData::DMXPStoFMX(dBodyGetRotation(m_body), dBodyGetPosition(m_body), bf);

    Fmatrix diff = Fmatrix().mul_43(Fmatrix().invert(bf), gf);

    dMatrix3 m;
    PHDynamicData::FMXtoDMX(diff, m);

    VERIFY(g->geom());
    dGeomSetPosition(g->geom(), diff.c.x, diff.c.y, diff.c.z);
    dGeomSetRotation(g->geom(), m);

    g->set_body(m_body);
    CPHGeometryOwner::add_geom(g);
}

void CPHElement::remove_geom(CODEGeom* g)
{
    g->set_body(0);
    CPHGeometryOwner::remove_geom(g);
}

#pragma todo(remake it using Geometry functions)

void CPHElement::add_Mass(
    const SBoneShape& shape, const Fmatrix& offset, const Fvector& mass_center, float mass, CPHFracture* fracture)
{
    dMass m;
    dMatrix3 DMatx;
    switch (shape.type)
    {
    case SBoneShape::stBox:
    {
        dMassSetBox(&m, 1.f, shape.box.m_halfsize.x * 2.f, shape.box.m_halfsize.y * 2.f, shape.box.m_halfsize.z * 2.f);
        dMassAdjust(&m, mass);
        Fmatrix box_transform;
        shape.box.xform_get(box_transform);
        PHDynamicData::FMX33toDMX(shape.box.m_rotate, DMatx);
        dMassRotate(&m, DMatx);
        dMassTranslate(&m, shape.box.m_translate.x - mass_center.x, shape.box.m_translate.y - mass_center.y,
            shape.box.m_translate.z - mass_center.z);
        break;
    }
    case SBoneShape::stSphere:
    {
        shape.sphere;
        dMassSetSphere(&m, 1.f, shape.sphere.R);
        dMassAdjust(&m, mass);
        dMassTranslate(
            &m, shape.sphere.P.x - mass_center.x, shape.sphere.P.y - mass_center.y, shape.sphere.P.z - mass_center.z);
        break;
    }

    case SBoneShape::stCylinder:
    {
        const Fvector& pos = shape.cylinder.m_center;
        Fvector l;
        l.sub(pos, mass_center);
        dMassSetCylinder(&m, 1.f, 2, shape.cylinder.m_radius, shape.cylinder.m_height);
        dMassAdjust(&m, mass);
        dMatrix3 DMatx2;
        Fmatrix33 m33;
        m33.j.set(shape.cylinder.m_direction);
        Fvector::generate_orthonormal_basis(m33.j, m33.k, m33.i);
        PHDynamicData::FMX33toDMX(m33, DMatx2);
        dMassRotate(&m, DMatx2);
        dMassTranslate(&m, l.x, l.y, l.z);
        break;
    }

    case SBoneShape::stNone: break;
    default: NODEFAULT;
    }
    PHDynamicData::FMXtoDMX(offset, DMatx);
    dMassRotate(&m, DMatx);

    Fvector mc;
    offset.transform_tiny(mc, mass_center);
    // calculate _new mass_center
    // new_mc=(m_mass_center*m_mass.mass+mc*mass)/(mass+m_mass.mass)
    Fvector tmp1;
    tmp1.set(m_mass_center);
    tmp1.mul(m_mass.mass);
    Fvector tmp2;
    tmp2.set(mc);
    tmp2.mul(mass);
    Fvector new_mc;
    new_mc.add(tmp1, tmp2);
    new_mc.mul(1.f / (mass + m_mass.mass));
    mc.sub(new_mc);
    dMassTranslate(&m, mc.x, mc.y, mc.z);
    m_mass_center.sub(new_mc);
    dMassTranslate(&m_mass, m_mass_center.x, m_mass_center.y, m_mass_center.z);
    if (m_fratures_holder)
    {
        m_fratures_holder->DistributeAdditionalMass(u16(m_geoms.size() - 1), m);
    }
    if (fracture)
    {
        fracture->MassAddToSecond(m);
    }
    R_ASSERT2(dMass_valide(&m), "bad bone mass params");
    dMassAdd(&m_mass, &m);
    R_ASSERT2(dMass_valide(&m), "bad result mass params");
    m_mass_center.set(new_mc);
}

void CPHElement::set_BoxMass(const Fobb& box, float mass)
{
    dMassSetZero(&m_mass);
    m_mass_center.set(box.m_translate);
    const Fvector& hside = box.m_halfsize;
    dMassSetBox(&m_mass, 1, hside.x * 2.f, hside.y * 2.f, hside.z * 2.f);
    dMassAdjust(&m_mass, mass);
    dMatrix3 DMatx;
    PHDynamicData::FMX33toDMX(box.m_rotate, DMatx);
    dMassRotate(&m_mass, DMatx);
}

void CPHElement::calculate_it_data_use_density(const Fvector& mc, float density)
{
    dMassSetZero(&m_mass);
    GEOM_I i_geom = m_geoms.begin(), e = m_geoms.end();
    for (; i_geom != e; ++i_geom)
        (*i_geom)->add_self_mass(m_mass, mc, density);
    VERIFY2(dMass_valide(&m_mass), "non valide mass obtained!");
}

float CPHElement::getRadius() { return CPHGeometryOwner::getRadius(); }
void CPHElement::set_DynamicLimits(float l_limit, float w_limit)
{
    m_l_limit = l_limit;
    m_w_limit = w_limit;
}

void CPHElement::set_DynamicScales(float l_scale /* =default_l_scale */, float w_scale /* =default_w_scale */)
{
    m_l_scale = l_scale;
    m_w_scale = w_scale;
}

void CPHElement::set_DisableParams(const SAllDDOParams& params) { CPHDisablingFull::set_DisableParams(params); }
void CPHElement::get_Extensions(const Fvector& axis, float center_prg, float& lo_ext, float& hi_ext) const
{
    CPHGeometryOwner::get_Extensions(axis, center_prg, lo_ext, hi_ext);
}

const Fvector& CPHElement::mass_Center() const
{
    VERIFY(dBodyStateValide(m_body));
    return *((const Fvector*)dBodyGetPosition(m_body));
}

CPhysicsShell* CPHElement::PhysicsShell() { return smart_cast<CPhysicsShell*>(m_shell); }
CPHShell* CPHElement::PHShell() { return (m_shell); }
void CPHElement::SetShell(CPHShell* p)
{
    if (!m_body || !m_shell)
    {
        m_shell = p;
        return;
    }
    if (m_shell != p)
    {
        m_shell->Island().RemoveBody(m_body);
        p->Island().AddBody(m_body);
        m_shell = p;
    }
}
void CPHElement::PassEndGeoms(u16 from, u16 to, CPHElement* dest)
{
    GEOM_I i_from = m_geoms.begin() + from, e = m_geoms.begin() + to;
    u16 shift = to - from;
    GEOM_I i = i_from;
    for (; i != e; ++i)
    {
        //(*i)->remove_from_space( group_space() );
        group_remove(*(*i));
        //(*i)->add_to_space(dest->m_group);
        //(*i)->set_body(dest->m_body);
        (*i)->set_body(0);
        u16& element_pos = (*i)->element_position();
        element_pos = element_pos - shift;
    }
    GEOM_I last = m_geoms.end();
    for (; i != last; ++i)
    {
        u16& element_pos = (*i)->element_position();
        element_pos = element_pos - shift;
    }

    dest->m_geoms.insert(dest->m_geoms.end(), i_from, e);
    dest->b_builded = true;
    m_geoms.erase(i_from, e);
}
void CPHElement::SplitProcess(ELEMENT_PAIR_VECTOR& new_elements)
{
    m_fratures_holder->SplitProcess(this, new_elements);
    if (!m_fratures_holder->m_fractures.size())
        xr_delete(m_fratures_holder);
}
void CPHElement::DeleteFracturesHolder() { xr_delete(m_fratures_holder); }
void CPHElement::CreateSimulBase()
{
    m_body = dBodyCreate(0);
    m_shell->Island().AddBody(m_body);
    // m_saved_contacts=dJointGroupCreate (0);
    // b_contacts_saved=false;
    dBodyDisable(m_body);
    // CPHGeometryOwner::CreateGroupSpace( );
}
void CPHElement::ReAdjustMassPositions(const Fmatrix& shift_pivot, float density)
{
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
    {
        (*i)->move_local_basis(shift_pivot);
    }
    if (m_shell->PKinematics())
    {
        float mass;
        get_mc_kinematics(m_shell->PKinematics(), m_mass_center, mass);
        calculate_it_data(m_mass_center, mass);
    }
    else
    {
        setDensity(density);
    }

    dBodySetMass(m_body, &m_mass);
    // m_inverse_local_transform.identity();
    // m_inverse_local_transform.c.set(m_mass_center);
    // m_inverse_local_transform.invert();
    // dBodySetPosition(m_body,m_mass_center.x,m_mass_center.y,m_mass_center.z);
}
void CPHElement::ResetMass(float density)
{
    Fvector tmp, shift_mc;

    tmp.set(m_mass_center);

    setDensity(density);
    dBodySetMass(m_body, &m_mass);

    shift_mc.sub(m_mass_center, tmp);
    tmp.set(*(Fvector*)dBodyGetPosition(m_body));
    tmp.add(shift_mc);

    // bActivating = true;
    m_flags.set(flActivating, TRUE);

    CPHGeometryOwner::setPosition(m_mass_center);
}
void CPHElement::ReInitDynamics(const Fmatrix& shift_pivot, float density)
{
    VERIFY(_valid(shift_pivot) && _valid(density));
    ReAdjustMassPositions(shift_pivot, density);
    GEOM_I i = m_geoms.begin(), e = m_geoms.end();
    for (; i != e; ++i)
    {
        (*i)->set_build_position(m_mass_center);
        (*i)->set_body(m_body);
        // if(object_contact_callback)geom.set_obj_contact_cb(object_contact_callback);
        // if(m_phys_ref_object) geom.set_ref_object(m_phys_ref_object);
        /*
                if(m_group)
                {
                    (*i)->add_to_space((dSpaceID)m_group);
                }
        */
        group_add(*(*i));
    }
}

void CPHElement::PresetActive()
{
    if (isActive())
        return;

    CBoneInstance& B = m_shell->PKinematics()->LL_GetBoneInstance(m_SelfID);
    mXFORM.set(B.mTransform);
    // m_start_time=Device.fTimeGlobal;
    Fmatrix global_transform;
    global_transform.mul_43(m_shell->mXFORM, mXFORM);
    SetTransform(global_transform, mh_unspecified);

    if (!m_parent_element)
    {
        m_shell->m_object_in_root.set(mXFORM);
        m_shell->m_object_in_root.invert();
    }
    // dVectorSet(m_safe_position,dBodyGetPosition(m_body));
    // dQuaternionSet(m_safe_quaternion,dBodyGetQuaternion(m_body));
    // dVectorSet(m_safe_velocity,dBodyGetLinearVel(m_body));

    //////////////////////////////////////////////////////////////
    // initializing values for disabling//////////////////////////
    //////////////////////////////////////////////////////////////
    VERIFY(dBodyStateValide(m_body));
    m_body_interpolation.SetBody(m_body);
    FillInterpolation();
    // bActive=true;
    m_flags.set(flActive, TRUE);
    RunSimulation();
    VERIFY(dBodyStateValide(m_body));
}

bool CPHElement::isBreakable() { return !!m_fratures_holder; }
u16 CPHElement::setGeomFracturable(CPHFracture& fracture)
{
    if (!m_fratures_holder)
        m_fratures_holder = new CPHFracturesHolder();
    return m_fratures_holder->AddFracture(fracture);
}

CPHFracture& CPHElement::Fracture(u16 num)
{
    R_ASSERT2(m_fratures_holder, "no fractures!");
    return m_fratures_holder->Fracture(num);
}
u16 CPHElement::numberOfGeoms() const { return CPHGeometryOwner::numberOfGeoms(); }
void CPHElement::cv2bone_Xfrom(const Fquaternion& q, const Fvector& pos, Fmatrix& xform)
{
    VERIFY2(_valid(q) && _valid(pos), "cv2bone_Xfrom receive wrong data");
    xform.rotation(q);
    xform.c.set(pos);
    // xform.mulB(m_inverse_local_transform);
    MulB43InverceLocalForm(xform);
    VERIFY2(_valid(xform), "cv2bone_Xfrom returns wrong data");
}
void CPHElement::cv2obj_Xfrom(const Fquaternion& q, const Fvector& pos, Fmatrix& xform)
{
    cv2bone_Xfrom(q, pos, xform);
    xform.mulB_43(m_shell->m_object_in_root);
    VERIFY2(_valid(xform), "cv2obj_Xfrom returns wrong data");
}

void CPHElement::set_ApplyByGravity(bool flag)
{
    if (!isActive() || m_flags.test(flFixed))
        return;
    dBodySetGravityMode(m_body, flag);
}
bool CPHElement::get_ApplyByGravity() { return (!!dBodyGetGravityMode(m_body)); }
void CPHElement::Fix()
{
    m_flags.set(flFixed, TRUE);
    FixBody(m_body);
}

void CPHElement::SetAnimated(bool v) { m_flags.set(flAnimated, BOOL(v)); }
void CPHElement::ReleaseFixed()
{
    if (!isFixed())
        return;
    m_flags.set(flFixed, FALSE);
    if (!isActive())
        return;
    dBodySetMass(m_body, &m_mass);
}
void CPHElement::applyGravityAccel(const Fvector& accel)
{
    VERIFY(_valid(accel));
    if (m_flags.test(flFixed))
        return;
    if (!dBodyIsEnabled(m_body))
        dBodyEnable(m_body);
    m_shell->EnableObject(0);
    Fvector val;
    val.set(accel);
    val.mul(m_mass.mass);
    // ApplyGravityAccel(m_body,(const dReal*)(&accel));
    applyForce(val.x, val.y, val.z);
}

void CPHElement::CutVelocity(float l_limit, float a_limit)
{
    if (!isActive())
        return;
    VERIFY(_valid(l_limit) && _valid(a_limit));
    dVector3 limitedl, limiteda, diffl, diffa;
    bool blimitl = dVectorLimit(dBodyGetLinearVel(m_body), l_limit, limitedl);
    bool blimita = dVectorLimit(dBodyGetAngularVel(m_body), a_limit, limiteda);

    if (blimitl || blimita)
    {
        dVectorSub(diffl, limitedl, dBodyGetLinearVel(m_body));
        dVectorSub(diffa, limiteda, dBodyGetAngularVel(m_body));
        dBodySetLinearVel(m_body, diffl[0], diffl[1], diffl[2]);
        dBodySetAngularVel(m_body, diffa[0], diffa[1], diffa[2]);
        dxStepBody(m_body, fixed_step);
        dBodySetLinearVel(m_body, limitedl[0], limitedl[1], limitedl[2]);
        dBodySetAngularVel(m_body, limiteda[0], limiteda[1], limiteda[2]);
    }
}
void CPHElement::ClearDestroyInfo() { xr_delete(m_fratures_holder); }
void CPHElement::GetPointVel(Fvector& res_vel, const Fvector& point) const
{
    dVector3 res;
    // Fvector	 res_vel;
    dBodyGetPointVel(get_bodyConst(), point.x, point.y, point.z, res);
    CopyMemory(&res_vel, res, sizeof(Fvector));
}

#ifdef DEBUG

void CPHElement::dbg_draw_velocity(float scale, u32 color)
{
    VERIFY(isActive());
    VERIFY(m_shell);
    VERIFY(m_shell->PKinematics());
    Fmatrix bone;
    GetGlobalTransformDynamic(&bone);
    VERIFY(m_body);
    dVector3 res;
    dBodyGetPointVel(m_body, bone.c.x, bone.c.y, bone.c.z, res);
    debug_output().DBG_DrawPoint(bone.c, 0.01f, color);
    debug_output().DBG_DrawLine(bone.c, Fvector().add(bone.c, cast_fv(res).mul(scale)), color);
    // m_shell->PKinematics()->LL_GetTransform()
}

static void dBodyGetPointForce(dBodyID b, dReal px, dReal py, dReal pz, dVector3 result)
{
    VERIFY(b);
    dVector3 p;
    p[0] = px - b->posr.pos[0];
    p[1] = py - b->posr.pos[1];
    p[2] = pz - b->posr.pos[2];
    p[3] = 0;
    result[0] = b->facc[0];
    result[1] = b->facc[1];
    result[2] = b->facc[2];
    dCROSS(result, +=, b->tacc, p);
}

void CPHElement::dbg_draw_force(float scale, u32 color)
{
    VERIFY(isActive());
    VERIFY(m_shell);
    VERIFY(m_shell->PKinematics());
    Fmatrix bone;
    GetGlobalTransformDynamic(&bone);
    VERIFY(m_body);
    dVector3 res;
    dBodyGetPointForce(m_body, bone.c.x, bone.c.y, bone.c.z, res);
    debug_output().DBG_DrawPoint(bone.c, 0.01f, color);
    debug_output().DBG_DrawLine(bone.c, Fvector().add(bone.c, cast_fv(res).mul(scale)), color);
}

void CPHElement::dbg_draw_geometry(float scale, u32 color, Flags32 flags /*= Flags32().assign( 0 )*/) const
{
    CPHGeometryOwner::dbg_draw(scale, color, flags);
}
#endif
// bool CPHElement::CheckBreakConsistent()
//{
//	if(!m_fratures_holder) return true;
//	m_fratures_holder->m_fractures
//	m_fratures_holder->Fracture()
//}) return true;
//	m_fratures_holder->m_fractures
//	m_fratures_holder->Fracture()
//}
