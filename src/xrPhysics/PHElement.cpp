#include "StdAfx.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "PHFracture.h"
#include "PHContactBodyEffector.h"
#include "MathUtils.h"
#include "matrix_utils.h"
#include "IPhysicsShellHolder.h"

#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"

#ifdef DEBUG
#include "debug_output.h"
#endif

#include "ExtendedGeom.h"
#include "PHShell.h"
#include "PHElement.h"
#include "PHElementInline.h"
#include "xrPhysicsCore/IPhysicsCore.h"

extern CPHWorld* ph_world;

static const float GLOBAL_IMPULSE_SCALE = 1.0f;

CPHElement::CPHElement()
{
    m_w_limit = default_w_limit;
    m_l_limit = default_l_limit;
    m_l_scale = default_l_scale;
    m_w_scale = default_w_scale;

    m_char_handle = INVALID_CHARACTER_VIRTUAL_HANDLE;
    m_flags.set(flActive, FALSE);
    m_flags.set(flActivating, FALSE);
    m_parent_element = NULL;
    m_shell = NULL;

    k_w = default_k_w;
    k_l = default_k_l;
    m_fratures_holder = NULL;
    m_flags.assign(0);
    mXFORM.identity();
    m_mass = 0.f;
    m_mass_center.set(0, 0, 0);
    m_volume = 0.f;
}

void CPHElement::add_Box(const Fobb& V) { CPHGeometryOwner::add_Box(V); }
void CPHElement::add_Sphere(const Fsphere& V) { CPHGeometryOwner::add_Sphere(V); }
void CPHElement::add_Cylinder(const Fcylinder& V) { CPHGeometryOwner::add_Cylinder(V); }

void CPHElement::build()
{
    if (m_geoms.empty())
    {
        // Для элементов без геометрии (косяки дверей и т.д.) создаем тело в слое NO_COLLISION.
        // Оно абсолютно призрачное (не коллизит ни с чем), но имеет валидный BodyID и трансформ для X-Ray/Jolt.
        Fvector half_extents = {0.01f, 0.01f, 0.01f};
        m_char_handle = GetPhysicsCore()->CreateBox(half_extents, m_mass_center, 1.f);
        Fix();
        if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
        {
            GetPhysicsCore()->SetBodyMotionType(m_char_handle, 0); // Static / Non-moving
            GetPhysicsCore()->SetBodyObjectLayer(m_char_handle, 4); // No collision geometry.
        }
    }
    else
    {
        VERIFY2(m_mass > 0.f, "Element has bad mass");

        xr_vector<PhysicsShapeHandle> shapes;
        xr_vector<Fmatrix> local_transforms;

        for (CPhysicsGeom* geom : m_geoms)
        {
            if (!geom->geometry())
            {
                geom->init();
            }

            if (geom->geometry())
            {
                shapes.push_back(geom->geometry());

                Fmatrix local_xform;
                geom->get_local_form(local_xform);

                // In ODE, dGeomSetBody automatically calculated the offset relative to the body.
                // In Jolt, we must pass the explicit offset relative to m_mass_center.
                local_xform.c.sub(m_mass_center);

                local_transforms.push_back(local_xform);
            }
        }

        if (shapes.empty())
        {
            // Защита от битых моделей без физики
            m_char_handle = GetPhysicsCore()->CreateBox({0.5f, 0.5f, 0.5f}, m_mass_center, m_mass);
        }
        else
        {
            PhysicsShapeHandle compound_shape = GetPhysicsCore()->CreateCompoundShape(
                shapes.data(),
                local_transforms.data(),
                shapes.size()
            );

            m_char_handle = GetPhysicsCore()->CreateBodyFromShape(compound_shape, m_mass_center, m_mass);
            GetPhysicsCore()->DestroyCDBModel(compound_shape);
        }
    }

    VERIFY_BOUNDARIES2(m_mass_center, phBoundaries, PhysicsRefObject(), "m_mass_center");

    // Применяем начальную трансформацию к собранному телу
    Fmatrix transform;
    transform.identity();
    transform.c = m_mass_center;
    GetPhysicsCore()->SetBodyTransform(m_char_handle, transform);

    CPHDisablingTranslational::Reinit();
    GetPhysicsCore()->SetBodyDamping(m_char_handle, 0.f, 0.f);
    CPHGeometryOwner::build();
    set_body(m_char_handle);

    if (isFixed() && m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        GetPhysicsCore()->SetBodyMotionType(m_char_handle, 0); // 0 = Static
    }

    if (m_geoms.empty()) GetPhysicsCore()->SetBodyObjectLayer(m_char_handle, 4);
    if (m_bone_masses && !m_custom_inertia) {
        Fvector center;
        SetNativeMassProperties(CalculateMassProperties(0, numberOfGeoms(), true, center));
    }
    if (m_custom_inertia) GetPhysicsCore()->SetBodyMassProperties(m_char_handle, m_native_mass);
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        GetPhysicsCore()->SetBodyUserData(m_char_handle, this);
        GetPhysicsCore()->SetBodyCollisionOwner(m_char_handle, m_shell);
        GetPhysicsCore()->SetBodyContactFeedback(m_char_handle, m_fratures_holder != nullptr);
    }
}

void CPHElement::RunSimulation()
{
    m_flags.set(flActive, TRUE);
    GetPhysicsCore()->ActivateBody(m_char_handle);
}

void CPHElement::destroy()
{
    CPHGeometryOwner::destroy();
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        GetPhysicsCore()->SetBodyUserData(m_char_handle, nullptr);
        GetPhysicsCore()->DestroyBody(m_char_handle);
        m_char_handle = INVALID_CHARACTER_VIRTUAL_HANDLE;
        set_body(INVALID_BODY_HANDLE);
    }
}

void CPHElement::calculate_it_data(const Fvector& mc, float mas)
{
    float density = mas / m_volume;
    calculate_it_data_use_density(mc, density);
}

void CPHElement::calc_it_fract_data_use_density(const Fvector& mc, float density)
{
    calc_volume_data();
    calculate_it_data_use_density(mc, density);
    for (auto& fracture : m_fratures_holder->m_fractures) {
        float secondMass = 0;
        for (u16 geometry = fracture.m_start_geom_num; geometry < fracture.m_end_geom_num; ++geometry)
            secondMass += m_geoms[geometry]->volume() * density;
        fracture.SetMassParts(m_mass - secondMass, secondMass);
    }
}

void CPHElement::set_local_mass_center(const Fvector& mc)
{
    m_mass_center.set(mc);
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
    VERIFY_BOUNDARIES2(mc, phBoundaries, PhysicsRefObject(), "mass center in set transform");

    Fmatrix transform = m0;
    transform.c = mc;
    GetPhysicsCore()->SetBodyTransform(m_char_handle, transform);
    if (isFixed())
    {
        GetPhysicsCore()->SetBodyMotionType(m_char_handle, 0); // Static
    }

    CPHDisablingFull::Reinit();

    Fmatrix check_tr;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, check_tr);
    VERIFY2(check_tr.c.magnitude() > 0.f || true, "not valide safe position");

    m_flags.set(flUpdate, TRUE);
    m_shell->spatial_move();
    if (history_state != mh_not_clear)
        CPHGeometryOwner::clear_motion_history(mh_unspecified == history_state);
}

void CPHElement::getQuaternion(Fquaternion& quaternion)
{
    if (!isActive())
        return;
    Fmatrix transform;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, transform);
    quaternion.set(transform);
    VERIFY(_valid(quaternion));
}

void CPHElement::setQuaternion(const Fquaternion& quaternion)
{
    VERIFY(_valid(quaternion));
    if (!isActive())
        return;

    Fmatrix transform;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, transform);
    transform.rotation(quaternion);
    GetPhysicsCore()->SetBodyTransform(m_char_handle, transform);

    CPHDisablingRotational::Reinit();
    m_flags.set(flUpdate, TRUE);
    m_shell->spatial_move();
}

void CPHElement::GetGlobalPositionDynamic(Fvector* v)
{
    if (!isActive())
        return;
    Fmatrix transform;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, transform);
    v->set(transform.c);
    VERIFY(_valid(*v));
}

void CPHElement::SetGlobalPositionDynamic(const Fvector& position)
{
    if (!isActive())
        return;
    VERIFY(_valid(position));
    VERIFY_BOUNDARIES2(position, phBoundaries, PhysicsRefObject(), "SetGlobalPosition argument ");

    Fmatrix transform;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, transform);
    transform.c = position;
    GetPhysicsCore()->SetBodyTransform(m_char_handle, transform);

    CPHDisablingTranslational::Reinit();
    m_flags.set(flUpdate, TRUE);
    m_shell->spatial_move();
}

void CPHElement::TransformPosition(const Fmatrix& form, motion_history_state history_state)
{
    if (!isActive())
        return;
    VERIFY(_valid(form));
    R_ASSERT2(m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE, "body is not created");

    Fmatrix bm;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, bm);
    Fmatrix new_bm;
    new_bm.mul(form, bm);

    GetPhysicsCore()->SetBodyTransform(m_char_handle, new_bm);
    VERIFY_BOUNDARIES2(new_bm.c, phBoundaries, PhysicsRefObject(), "TransformPosition dest pos");

    CPHDisablingFull::Reinit();
    m_char_handle_interpolation.ResetPositions();
    m_char_handle_interpolation.ResetRotations();
    m_flags.set(flUpdate, TRUE);
    if (history_state != mh_not_clear)
        clear_motion_history(mh_unspecified == history_state);
    m_shell->spatial_move();
}

CPHElement::~CPHElement()
{
    VERIFY(!isActive());
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        destroy();
    }
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

    GetPhysicsCore()->SetBodyLinearVelocity(m_char_handle, lin_vel);
    GetPhysicsCore()->SetBodyAngularVelocity(m_char_handle, ang_vel);

    m_char_handle_interpolation.SetBody(m_char_handle);

    if (disable)
        GetPhysicsCore()->DeactivateBody(m_char_handle);

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
    if (!GetPhysicsCore()->IsBodyActive(m_char_handle) && !m_flags.test(flUpdate))
        return;

    InterpolateGlobalTransform(&mXFORM);
    VERIFY2(_valid(mXFORM), "invalid position in update");
}

void CPHElement::PhTune(float step)
{
    if (!isActive())
        return;

    Fvector pos;
    Fmatrix tr;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, tr);
    pos = tr.c;
    VERIFY_BOUNDARIES2(pos, phBoundaries, PhysicsRefObject(), "PhTune body position");
}

void CPHElement::PhDataUpdate(float step)
{
    if (!isActive())
        return;

    if (isFixed())
    {
        GetPhysicsCore()->SetBodyLinearVelocity(m_char_handle, Fvector().set(0,0,0));
        GetPhysicsCore()->SetBodyAngularVelocity(m_char_handle, Fvector().set(0,0,0));
        GetPhysicsCore()->SetBodyForce(m_char_handle, Fvector().set(0,0,0));
        GetPhysicsCore()->SetBodyTorque(m_char_handle, Fvector().set(0,0,0));
        return;
    }

#ifdef DEBUG
    if (debug_output().ph_dbg_draw_mask().test(phDbgDrawMassCenters))
    {
        Fmatrix tr;
        GetPhysicsCore()->GetBodyTransform(m_char_handle, tr);
        debug_output().DBG_DrawPoint(tr.c, 0.03f, color_xrgb(255, 0, 0));
    }
#endif

    bool is_active_body = GetPhysicsCore()->IsBodyActive(m_char_handle);
    m_flags.set(flEnabledOnStep, is_active_body);
    if (!is_active_body)
        return;

    Fvector linear_velocity, angular_velocity;
    GetPhysicsCore()->GetBodyLinearVelocity(m_char_handle, linear_velocity);
    GetPhysicsCore()->GetBodyAngularVelocity(m_char_handle, angular_velocity);

    VERIFY(_valid(linear_velocity));
    VERIFY(_valid(angular_velocity));

    VERIFY(!fis_zero(m_l_scale) && !fis_zero(m_w_scale));
    // Avoid acquiring native write locks when velocity scaling is disabled.
    if (m_l_scale != 1.f) {
        linear_velocity.div(m_l_scale);
        GetPhysicsCore()->SetBodyLinearVelocity(m_char_handle, linear_velocity);
    }
    if (m_w_scale != 1.f) {
        angular_velocity.div(m_w_scale);
        GetPhysicsCore()->SetBodyAngularVelocity(m_char_handle, angular_velocity);
    }

    float linear_velocity_mag = linear_velocity.magnitude();
    float angular_velocity_mag = angular_velocity.magnitude();

    if (linear_velocity_mag > m_l_limit || angular_velocity_mag > m_w_limit)
    {
        CutVelocity(m_l_limit, m_w_limit);
    }

    Disabling();
    Fmatrix tr;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, tr);
    VERIFY_BOUNDARIES2(tr.c, phBoundaries, PhysicsRefObject(), "PhDataUpdate end, body position");

    UpdateInterpolation();
    if (!GetPhysicsCore()->IsBodyActive(m_char_handle)) return;
    GetPhysicsCore()->GetBodyLinearVelocity(m_char_handle, linear_velocity);
    GetPhysicsCore()->GetBodyAngularVelocity(m_char_handle, angular_velocity);
    // Air resistance acts on an already active body. Adding it through the
    // waking force APIs would reset Jolt's sleep timer on every frame.
    Fvector accumulated;
    if (!fis_zero(k_w))
    {
        GetPhysicsCore()->GetBodyTorque(m_char_handle, accumulated);
        accumulated.mad(angular_velocity, -k_w);
        GetPhysicsCore()->SetBodyTorque(m_char_handle, accumulated);
    }
    const float drag = std::min(linear_velocity.magnitude() * k_l,
        GetPhysicsCore()->GetBodyMass(m_char_handle) / step);
    if (!fis_zero(drag))
    {
        GetPhysicsCore()->GetBodyForce(m_char_handle, accumulated);
        accumulated.mad(linear_velocity, -drag);
        GetPhysicsCore()->SetBodyForce(m_char_handle, accumulated);
    }
}

void CPHElement::Enable()
{
    if (!isActive())
        return;
    m_shell->EnableObject(0);
    GetPhysicsCore()->ActivateBody(m_char_handle);
}

void CPHElement::Disable()
{
    if (!isActive() || !GetPhysicsCore()->IsBodyActive(m_char_handle))
        return;
    FillInterpolation();
    GetPhysicsCore()->DeactivateBody(m_char_handle);
}

void CPHElement::ReEnable() {}

void CPHElement::Freeze()
{
    if (m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE)
        return;

    m_flags.set(flWasEnabledBeforeFreeze, GetPhysicsCore()->IsBodyActive(m_char_handle));
    GetPhysicsCore()->DeactivateBody(m_char_handle);
}

void CPHElement::UnFreeze()
{
    if (m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE)
        return;
    if (m_flags.test(flWasEnabledBeforeFreeze))
        GetPhysicsCore()->ActivateBody(m_char_handle);
}

bool dbg_draw_ph_force_apply = false;

void CPHElement::applyImpulseVsMC(const Fvector& pos, const Fvector& dir, float val)
{
    if (!isActive() || m_flags.test(flFixed))
        return;
    if (!GetPhysicsCore()->IsBodyActive(m_char_handle))
        GetPhysicsCore()->ActivateBody(m_char_handle);

    Fvector impulse;
    impulse.set(dir);
    impulse.mul(val* GLOBAL_IMPULSE_SCALE);

    Fmatrix tr;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, tr);
    Fvector world_pos;
    tr.transform_tiny(world_pos, pos);

    GetPhysicsCore()->ApplyPointImpulse(m_char_handle, impulse, world_pos);
}

void CPHElement::applyImpulseVsGF(const Fvector& pos, const Fvector& dir, float val)
{
    VERIFY(_valid(pos) && _valid(dir) && _valid(val));
    if (!isActive() || m_flags.test(flFixed))
        return;
    if (!GetPhysicsCore()->IsBodyActive(m_char_handle))
        GetPhysicsCore()->ActivateBody(m_char_handle);

    Fvector impulse;
    impulse.set(dir);
    impulse.mul(val* GLOBAL_IMPULSE_SCALE);

    GetPhysicsCore()->ApplyPointImpulse(m_char_handle, impulse, pos);
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

    applyImpulseVsMC(body_pos, dir, val);

    if (m_fratures_holder)
    {
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
    if (m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE || !m_flags.test(flUpdate))
    {
        GetGlobalTransformDynamic(m);
        VERIFY(_valid(*m));
        return;
    }
    m_char_handle_interpolation.InterpolateRotation(*m);
    m_char_handle_interpolation.InterpolatePosition(m->c);
    MulB43InverceLocalForm(*m);
    m_flags.set(flUpdate, FALSE);
    VERIFY(_valid(*m));
}

void CPHElement::GetGlobalTransformDynamic(Fmatrix* m) const
{
    if (m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        m->set(mXFORM);
        VERIFY(_valid(*m));
        return;
    }

    GetPhysicsCore()->GetBodyTransform(m_char_handle, *m);
    MulB43InverceLocalForm(*m);
    VERIFY(_valid(*m));
}

void CPHElement::InterpolateGlobalPosition(Fvector* v)
{
    if (m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        v->set(mXFORM.c);
        VERIFY(_valid(*v));
        return;
    }

    m_char_handle_interpolation.InterpolatePosition(*v);
    VERIFY(_valid(*v));
}

void CPHElement::build(bool disable)
{
    if (isActive())
        return;
    m_flags.set(flActive, TRUE);
    m_flags.set(flActivating, TRUE);
    build();
    SetTransform(mXFORM, mh_unspecified);

    m_char_handle_interpolation.SetBody(m_char_handle);
    if (disable)
        GetPhysicsCore()->DeactivateBody(m_char_handle);
}

void CPHElement::RunSimulation(const Fmatrix& start_from)
{
    RunSimulation();
    Fmatrix globe;
    globe.mul(start_from, mXFORM);
    SetTransform(globe, mh_unspecified);
}

void CPHElement::StataticRootBonesCallBack(CBoneInstance* B)
{
    VERIFY(false);
}

void CPHElement::BoneGlPos(Fmatrix& m, const Fmatrix& BoneTransform) const
{
    VERIFY(m_shell);
    m.mul_43(m_shell->mXFORM, BoneTransform);
}

void CPHElement::GetAnimBonePos(Fmatrix& bp)
{
    VERIFY(m_shell->PKinematics());
    IKinematics* pK = m_shell->PKinematics();
    CBoneInstance* BI = &pK->LL_GetBoneInstance(m_SelfID);
    if (!BI->callback())
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

    IPhysicsShellHolder* ph = PhysicsRefObject();
    VERIFY(ph);
    Fmatrix bpl;
    GetAnimBonePos(bpl);
    Fmatrix bp;
    bp.mul_43(ph->ObjectXFORM(), bpl);

    Fmatrix cp;
    GetGlobalTransformDynamic(&cp);

    cp.invert();
    Fmatrix diff;
    diff.mul_43(cp, bp);
    if (dt < EPS_S)
        dt = EPS_S;

    Fvector mc1;
    CPHGeometryOwner::get_mc_vs_transform(mc1, bp);

    Fvector mc0;
    Fmatrix tr;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, tr);
    mc0 = tr.c;

    Fvector lv;
    linear_diff(lv, mc1, mc0, dt);
    Fvector aw;
    angular_diff(aw, diff, dt);

    float aw_sqm = aw.square_magnitude();
    float lv_sqm = lv.square_magnitude();
    bool ret = aw_sqm < a_limit * a_limit && lv_sqm < l_limit * l_limit;

    put_in_range(lv, l_limit);
    put_in_range(aw, a_limit);

    GetPhysicsCore()->SetBodyLinearVelocity(m_char_handle, lv);
    GetPhysicsCore()->SetBodyAngularVelocity(m_char_handle, aw);
    return ret;
}

void CPHElement::ToBonePos(const Fmatrix& BoneTransform, motion_history_state history_state)
{
    VERIFY2(!ph_world->Processing(), PhysicsRefObject()->ObjectNameSect());
    VERIFY(_valid(BoneTransform));

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
    VERIFY_RMATRIX(B->mTransform);
    VERIFY_BOUNDARIES2(B->mTransform.c, phBoundaries, PhysicsRefObject(), "BonesCallBack incoming bone position");

    if (m_flags.test(flActivating))
    {
        ActivatingPos(B->mTransform);
        B->set_callback_overwrite(TRUE);
    }

    CalculateBoneTransform(B->mTransform);
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
void CPHElement::set_ContactCallback(ObjectContactCallbackFun* callback)
{
    CPHGeometryOwner::set_ContactCallback(callback);
}

void CPHElement::SetMaterial(u16 m) { CPHGeometryOwner::SetMaterial(m); }

PhysicsMassProperties CPHElement::getMassTensor()
{
    return NativeMassProperties();
}

PhysicsMassProperties CPHElement::NativeMassProperties()
{
    if (m_custom_inertia) return m_native_mass;
    if (m_char_handle != INVALID_BODY_HANDLE && !isFixed())
        return GetPhysicsCore()->GetBodyMassProperties(m_char_handle);
    if (m_bone_masses) {
        Fvector center;
        return CalculateMassProperties(0, numberOfGeoms(), true, center);
    }
    xr_vector<PhysicsShapeHandle> shapes;
    xr_vector<Fmatrix> transforms;
    for (auto* geom : m_geoms) {
        geom->init();
        shapes.push_back(geom->geometry());
        Fmatrix transform;
        geom->get_local_form(transform);
        transform.c.sub(m_mass_center);
        transforms.push_back(transform);
    }
    const auto shape = GetPhysicsCore()->CreateCompoundShape(shapes.data(), transforms.data(), shapes.size());
    const auto properties = GetPhysicsCore()->GetShapeMassProperties(shape, m_mass);
    GetPhysicsCore()->DestroyCDBModel(shape);
    return properties;
}
PhysicsMassProperties CPHElement::CalculateMassProperties(u16 from, u16 to, bool inside, Fvector& center)
{
    PhysicsMassProperties properties;
    properties.inertia.i.set(0, 0, 0);
    properties.inertia.j.set(0, 0, 0);
    properties.inertia.k.set(0, 0, 0);
    center.set(0, 0, 0);
    float totalWeight = 0;
    for (auto* geom : m_geoms) totalWeight += m_bone_masses ? geom->native_mass : geom->volume();
    if (totalWeight <= 0) return properties;
    for (u16 index = 0; index < numberOfGeoms(); ++index) {
        if ((index >= from && index < to) != inside) continue;
        auto* geom = m_geoms[index];
        const float mass = m_mass * (m_bone_masses ? geom->native_mass : geom->volume()) / totalWeight;
        if (mass <= 0) continue;
        geom->init();
        Fmatrix transform;
        if (geom->has_runtime_local) geom->get_local_form_bt(transform);
        else { geom->get_local_form(transform); transform.c.sub(m_mass_center); }
        auto shape = geom->geometry();
        auto compound = GetPhysicsCore()->CreateCompoundShape(&shape, &transform, 1);
        const auto part = GetPhysicsCore()->GetShapeMassProperties(compound, mass);
        GetPhysicsCore()->DestroyCDBModel(compound);
        properties.mass += mass;
        properties.inertia.i.add(part.inertia.i);
        properties.inertia.j.add(part.inertia.j);
        properties.inertia.k.add(part.inertia.k);
        Fvector massCenter = transform.c;
        if (m_bone_masses) {
            Fmatrix original, inverse;
            geom->get_local_form(original);
            inverse.invert(original);
            inverse.transform_tiny(massCenter, geom->native_mass_center);
            transform.transform_tiny(massCenter);
        }
        center.mad(massCenter, mass);
    }
    if (properties.mass > 0) center.div(properties.mass);
    return properties;
}
void CPHElement::SetNativeMassProperties(const PhysicsMassProperties& properties)
{
    if (properties.mass <= 0) return;
    m_custom_inertia = true;
    m_native_mass = properties;
    m_mass = properties.mass;
    GetPhysicsCore()->SetBodyMassProperties(m_char_handle, properties);
}

void CPHElement::setInertia(float M)
{
    m_mass = M;
    GetPhysicsCore()->SetBodyMass(m_char_handle, M);
}

void CPHElement::addInertia(float M)
{
    m_mass += M;
    GetPhysicsCore()->SetBodyMass(m_char_handle, m_mass);
}

void CPHElement::get_LinearVel(Fvector& velocity) const
{
    if (!isActive() || (!m_flags.test(flAnimated) && !GetPhysicsCore()->IsBodyActive(m_char_handle)))
    {
        velocity.set(0, 0, 0);
        return;
    }
    GetPhysicsCore()->GetBodyLinearVelocity(m_char_handle, velocity);
}

void CPHElement::get_AngularVel(Fvector& velocity) const
{
    if (!isActive() || (!m_flags.test(flAnimated) && !GetPhysicsCore()->IsBodyActive(m_char_handle)))
    {
        velocity.set(0, 0, 0);
        return;
    }
    GetPhysicsCore()->GetBodyAngularVelocity(m_char_handle, velocity);
}

void CPHElement::set_LinearVel(const Fvector& velocity)
{
    if (!isActive() || m_flags.test(flFixed))
        return;
    VERIFY2(_valid(velocity), "not valid argument velocity");
    Fvector vel = velocity;
    put_in_range(vel, m_l_limit);
    GetPhysicsCore()->SetBodyLinearVelocity(m_char_handle, vel);
}

void CPHElement::set_AngularVel(const Fvector& velocity)
{
    VERIFY(_valid(velocity));
    if (!isActive() || m_flags.test(flFixed))
        return;

    Fvector vel = velocity;
    put_in_range(vel, m_w_limit);
    GetPhysicsCore()->SetBodyAngularVelocity(m_char_handle, vel);
}

void CPHElement::getForce(Fvector& force)
{
    GetPhysicsCore()->GetBodyForce(m_char_handle, force);
}
void CPHElement::getTorque(Fvector& torque)
{
    GetPhysicsCore()->GetBodyTorque(m_char_handle, torque);
}

void CPHElement::setForce(const Fvector& force) { GetPhysicsCore()->SetBodyForce(m_char_handle, force); }
void CPHElement::setTorque(const Fvector& torque) { GetPhysicsCore()->SetBodyTorque(m_char_handle, torque); }

void CPHElement::applyForce(const Fvector& dir, float val)
{
    applyForce(dir.x * val, dir.y * val, dir.z * val);
}

void CPHElement::applyForce(float x, float y, float z)
{
    VERIFY(_valid(x) && _valid(y) && _valid(z));
    if (!isActive() || m_flags.test(flFixed))
        return;
    if (!GetPhysicsCore()->IsBodyActive(m_char_handle))
        GetPhysicsCore()->ActivateBody(m_char_handle);

    GetPhysicsCore()->ApplyForce(m_char_handle, Fvector().set(x, y, z));
    m_shell->EnableObject(0);
}

void CPHElement::applyImpulse(const Fvector& dir, float val)
{
    if (!isActive() || m_flags.test(flFixed))
        return;
    if (!GetPhysicsCore()->IsBodyActive(m_char_handle))
        GetPhysicsCore()->ActivateBody(m_char_handle);

    Fvector impulse;
    impulse.set(dir);
    impulse.mul(val * GLOBAL_IMPULSE_SCALE);
    GetPhysicsCore()->ApplyLinearImpulse(m_char_handle, impulse);
}

void CPHElement::add_Shape(const SBoneShape& shape, const Fmatrix& offset)
{
    CPHGeometryOwner::add_Shape(shape, offset);
}

void CPHElement::add_Shape(const SBoneShape& shape) { CPHGeometryOwner::add_Shape(shape); }

void CPHElement::add_geom(CPhysicsGeom* g)
{
    g->set_body(m_char_handle);
    CPHGeometryOwner::add_geom(g);
}

void CPHElement::remove_geom(CPhysicsGeom* g)
{
    g->set_body(INVALID_CHARACTER_VIRTUAL_HANDLE);
    CPHGeometryOwner::remove_geom(g);
}

void CPHElement::add_Mass(
    const SBoneShape& shape, const Fmatrix& offset, const Fvector& mass_center, float mass, CPHFracture* fracture)
{
    if (mass <= 0 || m_geoms.empty()) return;
    if (!m_bone_masses && m_geoms.size() > 1) {
        float volume = 0;
        for (size_t index = 0; index + 1 < m_geoms.size(); ++index) volume += m_geoms[index]->volume();
        if (volume > 0)
            for (size_t index = 0; index + 1 < m_geoms.size(); ++index) {
                m_geoms[index]->native_mass = m_mass * m_geoms[index]->volume() / volume;
                m_geoms[index]->native_mass_center = m_geoms[index]->local_center();
            }
    }
    Fvector center;
    offset.transform_tiny(center, mass_center);
    m_mass_center.mul(m_mass);
    m_mass_center.mad(center, mass);
    m_mass += mass;
    m_mass_center.div(m_mass);
    auto* geometry = m_geoms.back();
    geometry->native_mass_center.mul(geometry->native_mass);
    geometry->native_mass_center.mad(center, mass);
    geometry->native_mass += mass;
    geometry->native_mass_center.div(geometry->native_mass);
    m_bone_masses = true;
    m_custom_inertia = false;
    ++m_native_shape_revision;
    if (m_fratures_holder)
    {
        m_fratures_holder->DistributeAdditionalMass(u16(m_geoms.size() - 1), mass);
    }
    if (fracture)
    {
        fracture->MassAddToSecond(mass);
    }
}

void CPHElement::set_BoxMass(const Fobb& box, float mass)
{
    m_mass = mass;
    GetPhysicsCore()->SetBodyMass(m_char_handle, mass);
    m_mass_center.set(box.m_translate);
}

void CPHElement::calculate_it_data_use_density(const Fvector& mc, float density)
{
    m_mass_center = mc;
    m_mass = m_volume * density;
    m_custom_inertia = false;
    m_bone_masses = false;
    ++m_native_shape_revision;
    GetPhysicsCore()->SetBodyMass(m_char_handle, m_mass);
}

float CPHElement::getRadius() { return CPHGeometryOwner::getRadius(); }

void CPHElement::set_DynamicLimits(float l_limit, float w_limit)
{
    m_l_limit = l_limit;
    m_w_limit = w_limit;
}

void CPHElement::set_DynamicScales(float l_scale, float w_scale)
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
    static Fvector mc;
    Fmatrix tr;
    GetPhysicsCore()->GetBodyTransform(m_char_handle, tr);
    mc = tr.c;
    return mc;
}

CPhysicsShell* CPHElement::PhysicsShell() { return smart_cast<CPhysicsShell*>(m_shell); }
CPHShell* CPHElement::PHShell() { return (m_shell); }

void CPHElement::SetShell(CPHShell* p)
{
    m_shell = p;
    if (m_char_handle != INVALID_BODY_HANDLE)
        GetPhysicsCore()->SetBodyCollisionOwner(m_char_handle, p);
}

void CPHElement::PassEndGeoms(u16 from, u16 to, CPHElement* dest)
{
    R_ASSERT(from < to && to <= numberOfGeoms());
    GEOM_I i_from = m_geoms.begin() + from, e = m_geoms.begin() + to;
    dest->m_geoms.insert(dest->m_geoms.end(), i_from, e);
    for (u16 index = 0; index < dest->numberOfGeoms(); ++index) {
        auto* geom = dest->m_geoms[index];
        geom->owner = dest;
        geom->set_body(INVALID_BODY_HANDLE);
        geom->element_position() = index;
    }
    dest->b_builded = true;
    m_geoms.erase(i_from, e);
    for (u16 index = 0; index < numberOfGeoms(); ++index) m_geoms[index]->element_position() = index;
    RebuildNativeShape();
}

void CPHElement::SplitProcess(ELEMENT_PAIR_VECTOR& new_elements)
{
    m_fratures_holder->SplitProcess(this, new_elements);
    if (!m_fratures_holder->m_fractures.size())
        DeleteFracturesHolder();
}

void CPHElement::DeleteFracturesHolder() {
    GetPhysicsCore()->SetBodyContactFeedback(m_char_handle, false);
    xr_delete(m_fratures_holder);
}

void CPHElement::CreateSimulBase()
{
    Fvector half_extents = {0.5f, 0.5f, 0.5f};
    m_char_handle = GetPhysicsCore()->CreateBox(half_extents, m_mass_center, m_mass);
    GetPhysicsCore()->SetBodyUserData(m_char_handle, this);
    GetPhysicsCore()->SetBodyCollisionOwner(m_char_handle, m_shell);
    GetPhysicsCore()->SetBodyContactFeedback(m_char_handle, m_fratures_holder != nullptr);
    set_body(m_char_handle);
    GetPhysicsCore()->DeactivateBody(m_char_handle);
}

void CPHElement::PresetActive()
{
    if (isActive())
        return;

    CBoneInstance& B = m_shell->PKinematics()->LL_GetBoneInstance(m_SelfID);
    mXFORM.set(B.mTransform);
    Fmatrix global_transform;
    global_transform.mul_43(m_shell->mXFORM, mXFORM);
    SetTransform(global_transform, mh_unspecified);

    if (!m_parent_element)
    {
        m_shell->m_object_in_root.set(mXFORM);
        m_shell->m_object_in_root.invert();
    }

    m_char_handle_interpolation.SetBody(m_char_handle);
    FillInterpolation();
    m_flags.set(flActive, TRUE);
    RunSimulation();
}

bool CPHElement::isBreakable() { return !!m_fratures_holder; }

u16 CPHElement::setGeomFracturable(CPHFracture& fracture)
{
    if (!m_fratures_holder)
        m_fratures_holder = xr_new<CPHFracturesHolder>();
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
    GetPhysicsCore()->SetBodyGravityFactor(m_char_handle, flag ? 1.0f : 0.0f);
}

bool CPHElement::get_ApplyByGravity()
{
    return m_char_handle != INVALID_BODY_HANDLE &&
        GetPhysicsCore()->GetBodyGravityFactor(m_char_handle) != 0.f;
}

void CPHElement::Fix()
{
    if (isFixed())
        return;
    m_flags.set(flFixed, TRUE);
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        GetPhysicsCore()->SetBodyMotionType(m_char_handle, 0); // 0 = Static
    }
}

void CPHElement::SetAnimated(bool v) { m_flags.set(flAnimated, BOOL(v)); }

void CPHElement::ReleaseFixed()
{
    if (!isFixed())
        return;
    m_flags.set(flFixed, FALSE);
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        GetPhysicsCore()->SetBodyMotionType(m_char_handle, 2); // 2 = Dynamic
    }
}

void CPHElement::SetCollideWithStatics(bool collide)
{
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
        GetPhysicsCore()->SetBodyCollideWithStatics(m_char_handle, collide);
}

void CPHElement::applyGravityAccel(const Fvector& accel)
{
    VERIFY(_valid(accel));
    if (!isActive() || m_flags.test(flFixed))
        return;

    if (!GetPhysicsCore()->IsBodyActive(m_char_handle))
        GetPhysicsCore()->ActivateBody(m_char_handle);

    Fvector force;
    force.set(accel);
    force.mul(m_mass);

    GetPhysicsCore()->ApplyForce(m_char_handle, force);
    m_shell->EnableObject(0);
}

void CPHElement::CutVelocity(float l_limit, float a_limit)
{
    if (!isActive())
        return;
    VERIFY(_valid(l_limit) && _valid(a_limit));

    Fvector lin_vel, ang_vel;
    GetPhysicsCore()->GetBodyLinearVelocity(m_char_handle, lin_vel);
    GetPhysicsCore()->GetBodyAngularVelocity(m_char_handle, ang_vel);

    if (lin_vel.magnitude() > l_limit)
    {
        lin_vel.normalize();
        lin_vel.mul(l_limit);
        GetPhysicsCore()->SetBodyLinearVelocity(m_char_handle, lin_vel);
    }
    if (ang_vel.magnitude() > a_limit)
    {
        ang_vel.normalize();
        ang_vel.mul(a_limit);
        GetPhysicsCore()->SetBodyAngularVelocity(m_char_handle, ang_vel);
    }
}

void CPHElement::ClearDestroyInfo() { DeleteFracturesHolder(); }

void CPHElement::GetPointVel(Fvector& res_vel, const Fvector& point) const
{
    if (!isActive() || m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        res_vel.set(0, 0, 0);
        return;
    }
    GetPhysicsCore()->GetBodyPointVelocity(m_char_handle, point, res_vel);
}

#ifdef DEBUG
void CPHElement::dbg_draw_velocity(float scale, u32 color)
{
    VERIFY(isActive());
    VERIFY(m_shell);
    VERIFY(m_shell->PKinematics());
    Fmatrix bone;
    GetGlobalTransformDynamic(&bone);
    VERIFY(m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE);

    Fvector vel;
    GetPhysicsCore()->GetBodyLinearVelocity(m_char_handle, vel);
    debug_output().DBG_DrawPoint(bone.c, 0.01f, color);
    debug_output().DBG_DrawLine(bone.c, Fvector().add(bone.c, vel.mul(scale)), color);
}

void CPHElement::dbg_draw_force(float scale, u32 color)
{
}

void CPHElement::dbg_draw_geometry(float scale, u32 color, Flags32 flags) const
{
    CPHGeometryOwner::dbg_draw(scale, color, flags);
}
#endif
