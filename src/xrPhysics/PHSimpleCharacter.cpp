#include "StdAfx.h"

#include "PHDynamicData.h"
#include "ExtendedGeom.h"
#include "xrCDB/Intersect.hpp"
#include "xrEngine/xr_object_list.h"
#include "tri-colliderknoopc/__aabb_tri.h"
#include "PHSimpleCharacter.h"
#include "PHContactBodyEffector.h"

#include "SpaceUtils.h"

#include "params.h"
#include "MathUtils.h"

#include "xrEngine/GameMtlLib.h"

#include "IPhysicsShellHolder.h"
#include "Include/xrRender/Kinematics.h"
#include "PHSimpleCharacterInline.h"
#include "DamageSource.h"
#include "PHCollideValidator.h"
#include "CalculateTriangle.h"

#include "Geometry.h"

#include "xrCore/Animation/Bone.hpp"
#include "xrEngine/xr_object.h"
#include "ph_valid_ode.h"

IC bool PhOutOfBoundaries(const Fvector& v) { return v.y < phBoundaries.y1; }
//#ifdef DEBUG

//#endif

const float LOSE_CONTROL_DISTANCE = 0.5f; // fly distance to lose control
const float CLAMB_DISTANCE = 0.5f;
const float CLIMB_GETUP_HEIGHT = 0.3f;

float IC sgn(float v) { return v < 0.f ? -1.f : 1.f; }
bool test_sides(const Fvector& center, const Fvector& side_dir, const Fvector& fv_dir, const Fvector& box, int tri_id)
{
    Triangle tri;
    CalculateInitTriangle(
        inl_ph_world().ObjectSpace().GetStaticTris() + tri_id, tri, inl_ph_world().ObjectSpace().GetStaticVerts());
    Fvector* verts = inl_ph_world().ObjectSpace().GetStaticVerts();
    {
        float dist = cast_fv(tri.norm).dotproduct(center) - tri.dist;
        // if(dist<0.f)return false;
        //////////////////////////////////////////////tri norm
        float fvn = cast_fv(tri.norm).dotproduct(fv_dir);
        float sg_fvn = sgn(fvn);
        float sdn = cast_fv(tri.norm).dotproduct(side_dir);
        float sg_sdn = sgn(sdn);
        if (sg_fvn * fvn * box.z + sg_sdn * sdn * box.x + _abs(tri.norm[1]) * box.y > _abs(dist))
            return false;
    }
    {
        ///////////////////////////////////////////////side///////////////////////////////////////////////////////////////////////////////////////////////////
        float sdc = side_dir.dotproduct(center);
        float sd0 = cast_fv(tri.side0).dotproduct(side_dir);
        float sg_sd0 = sgn(sd0);
        float sd1 = cast_fv(tri.side1).dotproduct(side_dir);
        float sg_sd1 = sgn(sd1);
        float abs_sd0 = sg_sd0 * sd0;
        float abs_sd1 = sg_sd1 * sd1;

        float sd, abs_sd, sg_sd;
        u32 v;
        if (sg_sd0 == sg_sd1)
        {
            sd = -sd0 - sd1;
            abs_sd = abs_sd0 + abs_sd1;
            sg_sd = -sg_sd0;
            v = tri.T->verts[2];
        }
        else if (abs_sd0 > abs_sd1)
        {
            sd = sd0;
            abs_sd = abs_sd0;
            sg_sd = sg_sd0;
            v = tri.T->verts[0];
        }
        else
        {
            sd = sd1;
            abs_sd = abs_sd1;
            sg_sd = sg_sd1;
            v = tri.T->verts[1];
        }

        float vp = side_dir.dotproduct(verts[v]);
        float dist = vp - sdc;
        float sg_dist = sgn(dist);
        float abs_dist = sg_dist * dist;

        if (sg_dist != sg_sd)
        {
            if (abs_dist - abs_sd > box.x)
                return false;
        }
        else
        {
            if (abs_dist > box.x)
                return false;
        }
    }
    ////sides cross///////////////////////////////////////////////////////////////////////////////////////////////
    Fvector crses[3];
    crses[0].set(-tri.side0[2], 0, tri.side0[0]);
    crses[1].set(-tri.side1[2], 0, tri.side1[0]);
    const Fvector& v2 = verts[tri.T->verts[2]];
    const Fvector& v0 = verts[tri.T->verts[0]];
    crses[2].x = -(v0.z - v2.z);
    crses[2].y = 0.f;
    crses[2].z = v0.x - v2.x;
    for (u8 i = 0; 3 > i; ++i)
    {
        const Fvector& crs = crses[i];
        u32 sv = tri.T->verts[i % 3];
        u32 ov = tri.T->verts[(i + 2) % 3];

        float c_prg = crs.dotproduct(center);
        float sv_prg = crs.dotproduct(verts[sv]);
        float ov_prg = crs.dotproduct(verts[ov]);
        float dist = c_prg - sv_prg;
        float sg_dist = sgn(dist);
        if (sgn(ov_prg - c_prg) != sg_dist)
        {
            if (_abs(fv_dir.dotproduct(crs)) * box.z + _abs(side_dir.dotproduct(crs)) * box.x < sg_dist * dist)
                return false;
        }
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////class//CPHSimpleCharacter////////////////////
CPHSimpleCharacter::CPHSimpleCharacter()
    : m_last_environment_update(Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX)), m_last_picked_material(GAMEMTL_NONE_IDX)
{
    m_object_contact_callback = NULL;

    m_geom_shell = NULL;
    m_wheel = NULL;

    m_space = NULL;
    m_wheel_transform = NULL;
    m_shell_transform = NULL;

    m_hat = NULL;
    m_hat_transform = NULL;
    m_acceleration.set(0, 0, 0);
    b_external_impulse = false;
    m_ext_impuls_stop_step = u64(-1);
    m_ext_imulse.set(0, 0, 0);
    m_phys_ref_object = NULL;
    b_on_object = false;
    m_friction_factor = 1.f;
    dVectorSetZero(m_control_force);
    dVectorSetZero(m_depart_position);
    is_contact = false;
    was_contact = false;
    is_control = false;
    b_depart = false;
    b_meet = false;
    b_lose_control = true;
    b_lose_ground = true;
    b_depart_control = false;
    b_jump = false;
    b_side_contact = false;
    b_was_side_contact = false;
    b_clamb_jump = false;
    b_any_contacts = false;
    b_valide_ground_contact = false;
    b_valide_wall_contact = false;
    b_jump = false;
    b_exist = false;
    m_mass = 70.f;
    m_max_velocity = 5.f;
    // m_update_time=0.f;
    b_meet_control = false;
    b_jumping = false;
    b_death_pos = false;
    jump_up_velocity = 6.f;
    m_air_control_factor = 0;
    // m_capture_joint=NULL;
    m_cap = NULL;
    m_cap_transform = NULL;
    dVectorSetZero(m_safe_velocity);
    m_collision_damage_factor = 1.f;
    b_collision_restrictor_touch = false;
    b_foot_mtl_check = true;
    b_non_interactive = false;
}

void CPHSimpleCharacter::TestPathCallback(
    bool& do_colide, bool bo1, dContact& c, SGameMtl* /*material_1*/, SGameMtl* /*material_2*/)
{
    do_colide = false;
    CPHSimpleCharacter* ch = NULL;
    if (bo1)
    {
        ch = static_cast<CPHSimpleCharacter*>(retrieveGeomUserData(c.geom.g1)->ph_object);
    }
    else
    {
        ch = static_cast<CPHSimpleCharacter*>(retrieveGeomUserData(c.geom.g2)->ph_object);
    }
    VERIFY(ch);
    ch->b_side_contact = true;
}

void CPHSimpleCharacter::SetBox(const dVector3& sizes)
{
    m_radius = std::min(sizes[0], sizes[2]) / 2.f;
    m_cyl_hight = sizes[1] - 2.f * m_radius;
    if (m_cyl_hight < 0.f)
        m_cyl_hight = 0.01f;
    const dReal k = 1.20f;
    dReal doun = m_radius * _sqrt(1.f - 1.f / k / k) / 2.f;
    // m_geom_shell=dCreateCylinder(0,m_radius/k,m_cyl_hight+doun);
    dGeomCylinderSetParams(m_geom_shell, m_radius / k, m_cyl_hight + doun);
    // m_wheel=dCreateSphere(0,m_radius);
    dGeomSphereSetRadius(m_wheel, m_radius);
    // m_hat=dCreateSphere(0,m_radius/k);
    dGeomSphereSetRadius(m_hat, m_radius / k);

    dGeomSetPosition(m_hat, 0.f, m_cyl_hight, 0.f);
    dGeomSetPosition(m_geom_shell, 0.f, m_cyl_hight / 2.f - doun, 0.f);

    float test_radius = m_radius * 2.f;
    float test_height = test_radius + m_radius / 2.f;
    // m_cap=dCreateSphere(0,test_radius);
    dGeomSphereSetRadius(m_cap, test_radius);
    dGeomSetPosition(m_cap, 0.f, test_height, 0.f);
}
void CPHSimpleCharacter::get_Box(Fvector& sz, Fvector& c) const
{
    float r, h;
    dGeomCylinderGetParams(m_geom_shell, &r, &h);
    sz.set(2 * r, 2 * r + h, 2 * r);
    const dReal* rot = NULL;
    const dReal* pos = NULL;
    dMatrix3 rr;
    CODEGeom::get_final_tx(m_shell_transform, pos, rot, cast_fp(c), rr);
}

void CPHSimpleCharacter::Create(dVector3 sizes)
{
    if (b_exist)
        return;

    b_air_contact_state = false;
    lastMaterialIDX = GAMEMTL_NONE_IDX;
    injuriousMaterialIDX = GAMEMTL_NONE_IDX;
    m_creation_step = ph_world->m_steps_num;
    ////////////////////////////////////////////////////////

    m_radius = std::min(sizes[0], sizes[2]) / 2.f;
    m_current_object_radius = m_radius;
    m_cyl_hight = sizes[1] - 2.f * m_radius;
    if (m_cyl_hight < 0.f)
        m_cyl_hight = 0.01f;

    b_exist = true;
    const dReal k = 1.20f;
    dReal doun = m_radius * _sqrt(1.f - 1.f / k / k) / 2.f;

    m_geom_shell = dCreateCylinder(0, m_radius / k, m_cyl_hight + doun);

    m_wheel = dCreateSphere(0, m_radius);
    m_hat = dCreateSphere(0, m_radius / k);

    m_shell_transform = dCreateGeomTransform(0);
    dGeomTransformSetCleanup(m_shell_transform, 0);
    m_hat_transform = dCreateGeomTransform(0);
    dGeomTransformSetCleanup(m_hat_transform, 0);
    // m_wheel_transform=dCreateGeomTransform(0);

    dGeomTransformSetInfo(m_shell_transform, 1);
    dGeomTransformSetInfo(m_hat_transform, 1);
    // dGeomTransformSetInfo(m_wheel_transform,1);
    ////////////////////////////////////////////////////////////////////////

    dGeomSetPosition(m_hat, 0.f, m_cyl_hight, 0.f);
    ////////////////////////////////////////////////////////////////////////

    // dGeomSetPosition(chSphera2,0.f,-0.7f,0.f);

    ////////////////////////////////////////////////////////////////////////
    dGeomSetPosition(m_geom_shell, 0.f, m_cyl_hight / 2.f - doun, 0.f);

    dGeomTransformSetGeom(m_hat_transform, m_hat);

    dGeomTransformSetGeom(m_shell_transform, m_geom_shell);
    m_body = dBodyCreate(0);
    Island().AddBody(m_body);

    dGeomSetBody(m_shell_transform, m_body);
    dGeomSetBody(m_hat_transform, m_body);
    dGeomSetBody(m_wheel, m_body);

    dGeomCreateUserData(m_geom_shell);

    dGeomCreateUserData(m_wheel);
    dGeomCreateUserData(m_hat);

    dGeomUserDataSetPhObject(m_wheel, (CPHObject*)this);
    dGeomUserDataSetPhObject(m_geom_shell, (CPHObject*)this);
    dGeomUserDataSetPhObject(m_hat, (CPHObject*)this);

    /////////////////////////////////////////////////////////////////////////
    dMass m;
    dMassSetBox(&m, 1, 1000000.f, 1000000.f, 1000000.f);
    dMassAdjust(&m, m_mass);
    dBodySetMass(m_body, &m);

    m_space = dSimpleSpaceCreate(0);
    // dGeomGroupAdd(m_geom_group,m_wheel_transform);
    dSpaceAdd(m_space, m_wheel);

    dSpaceAdd(m_space, m_shell_transform);
    dSpaceAdd(m_space, m_hat_transform);
    // dGeomGroupAdd(chRGeomGroup,chRCylinder);
    m_body_interpolation.SetBody(m_body);

    float test_radius = m_radius * 2.f;
    float test_height = test_radius + m_radius / 2.f;
    m_cap = dCreateSphere(0, test_radius);
    dGeomSetPosition(m_cap, 0.f, test_height, 0.f);
    m_cap_transform = dCreateGeomTransform(0);
    dGeomTransformSetCleanup(m_cap_transform, 0);
    dGeomTransformSetInfo(m_cap_transform, 1);
    dGeomTransformSetGeom(m_cap_transform, m_cap);
    dGeomCreateUserData(m_cap);
    dGeomGetUserData(m_cap)->b_static_colide = false;

    dGeomUserDataSetPhObject(m_cap, (CPHObject*)this);
    dSpaceAdd(m_space, m_cap_transform);
    dGeomSetBody(m_cap_transform, m_body);
    dGeomUserDataSetObjectContactCallback(m_cap, TestPathCallback);
    dGeomGetUserData(m_cap)->b_static_colide = false;
    if (m_phys_ref_object)
    {
        SetPhysicsRefObject(m_phys_ref_object);
    }
    if (m_object_contact_callback)
    {
        SetObjectContactCallback(m_object_contact_callback);
    }
    VERIFY(ph_world);
    SetStaticContactCallBack(ph_world->default_character_contact_shotmark());
    // SetStaticContactCallBack(CharacterContactShotMark);
    m_elevator_state.SetCharacter(static_cast<CPHCharacter*>(this));
    CPHObject::activate();
    spatial_register();
    m_last_move.set(0, 0, 0);
    CPHCollideValidator::SetCharacterClass(*this);
    m_collision_damage_info.Construct();
    m_last_environment_update = Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    m_last_picked_material = GAMEMTL_NONE_IDX;
}
void CPHSimpleCharacter::SwitchOFFInitContact()
{
    VERIFY(b_exist);
    dGeomUserDataSetPhObject(m_wheel, 0);
    dGeomUserDataSetPhObject(m_geom_shell, 0);
    dGeomUserDataSetPhObject(m_hat, 0);
    b_lose_control = true;
    b_any_contacts = false;
    is_contact = false;
    b_foot_mtl_check = true;
    b_on_ground = b_valide_ground_contact = false;
}
void CPHSimpleCharacter::SwitchInInitContact()
{
    VERIFY(b_exist);
    dGeomUserDataSetPhObject(m_wheel, (CPHObject*)this);
    dGeomUserDataSetPhObject(m_geom_shell, (CPHObject*)this);
    dGeomUserDataSetPhObject(m_hat, (CPHObject*)this);
}
void CPHSimpleCharacter::Destroy()
{
    if (!b_exist)
        return;
    b_exist = false;
    R_ASSERT2(!ph_world->Processing(),
        "can not deactivate physics character shell during physics processing!!!"); // if(ph_world)
    R_ASSERT2(!ph_world->IsFreezed(), "can not deactivate physics character when ph world is freezed!!!");
    R_ASSERT2(!CPHObject::IsFreezed(), "can not deactivate freezed !!!");
    m_elevator_state.Deactivate();

    spatial_unregister();
    CPHObject::deactivate();

    if (m_cap)
    {
        dGeomDestroyUserData(m_cap);
        dGeomDestroy(m_cap);
        m_cap = NULL;
    }

    if (m_cap_transform)
    {
        dGeomDestroyUserData(m_cap_transform);
        dGeomDestroy(m_cap_transform);
        m_cap_transform = NULL;
    }

    if (m_geom_shell)
    {
        dGeomDestroyUserData(m_geom_shell);
        dGeomDestroy(m_geom_shell);
        m_geom_shell = NULL;
    }

    if (m_wheel)
    {
        dGeomDestroyUserData(m_wheel);
        dGeomDestroy(m_wheel);
        m_wheel = NULL;
    }

    if (m_shell_transform)
    {
        dGeomDestroyUserData(m_shell_transform);
        dGeomDestroy(m_shell_transform);
        m_shell_transform = NULL;
    }

    if (m_wheel_transform)
    {
        dGeomDestroyUserData(m_wheel_transform);
        dGeomDestroy(m_wheel_transform);
        m_wheel_transform = NULL;
    }

    if (m_hat)
    {
        dGeomDestroyUserData(m_hat);
        dGeomDestroy(m_hat);
        m_hat = NULL;
    }
    if (m_hat_transform)
    {
        dGeomDestroyUserData(m_hat_transform);
        dGeomDestroy(m_hat_transform);
        m_hat_transform = NULL;
    }

    if (m_space)
    {
        dSpaceDestroy(m_space);
        m_space = NULL;
    }

    if (m_body)
    {
        Island().RemoveBody(m_body);
        dBodyDestroy(m_body);
        m_body = NULL;
    }
}
const static u64 impulse_time_constant = 30;
void CPHSimpleCharacter::ApplyImpulse(const Fvector& dir, dReal P)
{
    if (!b_exist || b_external_impulse)
        return;
    // if(!dBodyIsEnabled(m_body)) dBodyEnable(m_body);
    m_ext_imulse.set(dir);
    if (b_lose_control || b_jumping || b_jump)
    {
        // m_ext_imulse.y-=3.f			;
        // m_ext_imulse.normalize_safe()	;
        m_ext_imulse.set(0, -1, 0);
        // P*=0.3f				;
    }
    Enable();
    b_lose_control = true;
    b_external_impulse = true;
    m_ext_impuls_stop_step = ph_world->m_steps_num + impulse_time_constant;
    // m_ext_imulse.set(Fvector().mul(dir,P/fixed_step/impulse_time_constant));
    dBodySetLinearVel(m_body, 0, 0, 0);
    dBodySetForce(
        m_body, m_ext_imulse.x * P / fixed_step, m_ext_imulse.y * P / fixed_step, m_ext_imulse.z * P / fixed_step);
}

void CPHSimpleCharacter::ApplyForce(const Fvector& force) { ApplyForce(force.x, force.y, force.z); }
void CPHSimpleCharacter::ApplyForce(float x, float y, float z)
{
    if (!b_exist)
        return;
    Enable();
    // if( !b_external_impulse )
    dBodyAddForce(m_body, x, y, z);
    // BodyCutForce(m_body,5.f,0.f);
}

void CPHSimpleCharacter::ApplyForce(const Fvector& dir, float force)
{
    ApplyForce(dir.x * force, dir.y * force, dir.z * force);
}

void CPHSimpleCharacter::PhDataUpdate(dReal /**step/**/)
{
    ///////////////////

    SafeAndLimitVelocity();

    if (!dBodyIsEnabled(m_body))
    {
        if (!ph_world->IsFreezed())
            b_lose_control = false;
        return;
    }
    if (is_contact && !is_control && !b_lose_ground)
        Disabling();

    ///////////////////////
    if (ph_world->m_steps_num > m_ext_impuls_stop_step)
    {
        b_external_impulse = false;
        m_ext_impuls_stop_step = u64(-1);
        m_ext_imulse.set(0, 0, 0);
        Fvector vel;
        GetVelocity(vel);
        dVectorLimit(cast_fp(vel), m_max_velocity, cast_fp(vel));
        SetVelocity(Fvector().set(0, 0, 0));
    }
    was_contact = is_contact;
    was_control = is_control;
    b_was_side_contact = b_side_contact;
    is_contact = false;
    b_side_contact = false;
    b_any_contacts = false;
    b_valide_ground_contact = false;
    b_valide_wall_contact = false;

    b_was_on_object = b_on_object;
    b_on_object = false;
    b_death_pos = false;
    m_contact_count = 0;
    m_friction_factor = 0.f;
    b_collision_restrictor_touch = false;
    b_foot_mtl_check = true;
    b_depart_control = false;
    dMatrix3 R;
    dRSetIdentity(R);
    dBodySetAngularVel(m_body, 0.f, 0.f, 0.f);
    dBodySetRotation(m_body, R);

    dMass mass;
    const float* linear_velocity = dBodyGetLinearVel(m_body);
    dReal linear_velocity_mag = _sqrt(dDOT(linear_velocity, linear_velocity));
    dBodyGetMass(m_body, &mass);
    dReal l_air = linear_velocity_mag * default_k_l; // force/velocity !!!
    if (l_air > mass.mass / fixed_step)
        l_air = mass.mass / fixed_step; // validate

    if (!fis_zero(l_air))
        dBodyAddForce(m_body, -linear_velocity[0] * l_air, -linear_velocity[1] * l_air, -linear_velocity[2] * l_air);
    if (b_non_interactive)
    {
        Disable();
        dBodySetPosition(m_body, m_last_move.x, m_last_move.y, m_last_move.z);
    }
    m_last_move.sub(cast_fv(dBodyGetPosition(m_body)), m_last_move);
    m_last_move.mul(1.f / fixed_step);
    VERIFY2(dBodyStateValide(m_body), "WRONG BODYSTATE IN PhDataUpdate");
    if (PhOutOfBoundaries(cast_fv(dBodyGetPosition(m_body))))
        Disable();
    VERIFY_BOUNDARIES(cast_fv(dBodyGetPosition(m_body)), phBoundaries, PhysicsRefObject());
    m_body_interpolation.UpdatePositions();
}

void CPHSimpleCharacter::PhTune(dReal step)
{
    // if(b_non_interactive)
    //{
    //	dGeomSetBody(m_shell_transform,m_body);
    //	dGeomSetBody(m_hat_transform,m_body);
    //	dGeomSetBody(m_wheel,m_body);
    //	dGeomSetBody(m_cap_transform,m_body);
    //}

    m_last_move.set(cast_fv(dBodyGetPosition(m_body)));
    m_elevator_state.PhTune(step);

    b_air_contact_state = !is_contact;

#ifdef DEBUG
    if (debug_output().ph_dbg_draw_mask().test(phDbgCharacterControl))
    {
        if (b_air_contact_state)
            debug_output().DBG_DrawPoint(cast_fv(dBodyGetPosition(m_body)), m_radius, color_xrgb(255, 0, 0));
    }
#endif
    bool b_good_graund = b_valide_ground_contact && m_ground_contact_normal[1] > M_SQRT1_2;

    dxGeomUserData* ud = dGeomGetUserData(m_wheel);
    if ((ud->pushing_neg || ud->pushing_b_neg) && !b_death_pos)
    {
        b_death_pos = true;
        //#ifdef DEBUG
        //		Msg("death pos %f2.2,%f2.2,%f2.2",ud->last_pos[0],ud->last_pos[1],ud->last_pos[2]);
        //#endif
        Fvector pos;
        pos.set(cast_fv(dBodyGetPosition(m_body)));
        Fvector d;
        d.set(cast_fv(dBodyGetLinearVel(m_body)));
        d.mul(fixed_step);
        pos.sub(d);
        if (!ud->pushing_b_neg)
        {
            // Fvector movement;movement.sub(cast_fv(dGeomGetPosition(m_wheel)),cast_fv(ud->last_pos));

            dVectorSet(m_death_position, cast_fp(pos));
        }
        else
        {
            dVectorSet(m_death_position, cast_fp(pos));
        }
    }

    if (b_death_pos && !(ud->pushing_neg || ud->pushing_b_neg))
    {
        b_death_pos = false;
    }

    CPHContactBodyEffector* contact_effector = (CPHContactBodyEffector*)dBodyGetData(m_body);
    if (contact_effector)
        contact_effector->Apply();

    if (!dBodyIsEnabled(m_body))
    {
        if (!ph_world->IsFreezed())
            b_lose_control = false;
        return;
    }

    if (m_acceleration.magnitude() > 0.1f)
        is_control = true;
    else
        is_control = false;

    b_depart = was_contact && (!is_contact);
    b_stop_control = was_control && (!is_control);
    b_meet = (!was_contact) && (is_contact);
    if (b_lose_control && (is_contact || m_elevator_state.ClimbingState()))
        b_meet_control = true;
    b_on_ground = b_valide_ground_contact || (b_meet && (!b_depart));

    if (m_elevator_state.ClimbingState())
    {
        b_side_contact = false;
        m_friction_factor = 1.f;
        if (b_stop_control)
            dBodySetLinearVel(m_body, 0.f, 0.f, 0.f);
    }

    // save depart position
    if (b_depart)
        dVectorSet(m_depart_position, dBodyGetPosition(m_body));

    const dReal* velocity = dBodyGetLinearVel(m_body);
    dReal linear_vel_smag = dDOT(velocity, velocity);
    if (b_lose_control && (b_on_ground && m_ground_contact_normal[1] > M_SQRT1_2 / 2.f
                              //&&
                              //			!b_external_impulse
                              /*&&
                dSqrt(velocity[0]*velocity[0]+velocity[2]*velocity[2])<5.*/
                              || fis_zero(linear_vel_smag) || m_elevator_state.ClimbingState()))
        b_lose_control = false;

    if (b_jumping && b_good_graund ||
        (m_elevator_state.ClimbingState() &&
            b_valide_wall_contact)) // b_good_graund=b_valide_ground_contact&&m_ground_contact_normal[1]>M_SQRT1_2
        b_jumping = false;

    // deside if control lost
    if (!b_on_ground && !m_elevator_state.ClimbingState())
    {
        const dReal* current_pos = dBodyGetPosition(m_body);
        dVector3 dif = {current_pos[0] - m_depart_position[0], current_pos[1] - m_depart_position[1],
            current_pos[2] - m_depart_position[2]};
        if (dDOT(dif, dif) > LOSE_CONTROL_DISTANCE * LOSE_CONTROL_DISTANCE && dFabs(dif[1]) > 0.1)
        {
            b_lose_control = true;
            b_depart_control = true;
        }
    }

    ValidateWalkOn();

    // jump
    if (b_jump)
    {
        b_lose_control = true;
        b_depart_control = true;
        dBodySetLinearVel(m_body, m_jump_accel.x, m_jump_accel.y, m_jump_accel.z); // vel[1]+
        // Log("jmp",m_jump_accel);
        dVectorSet(m_jump_depart_position, dBodyGetPosition(m_body));
        // m_jump_accel=m_acceleration;
        b_jump = false;
        b_jumping = true;
        m_elevator_state.Depart();
        Enable();
    }

    b_lose_ground = !(b_good_graund || m_elevator_state.ClimbingState()) || b_lose_control;

    ApplyAcceleration();

    dReal* chVel = const_cast<dReal*>(dBodyGetLinearVel(m_body));
    // if(b_jump)
    //	dBodyAddForce(m_body,0.f,m_control_force[1],0.f);//+2.f*9.8f*70.f
    // else
    dMass m;
    dBodyGetMass(m_body, &m);
    if (is_control)
    {
        dVector3 sidedir;
        dVector3 y = {0., 1., 0.};
        dCROSS(sidedir, =, m_control_force, y);
        accurate_normalize(sidedir);
        dReal vProj = dDOT(sidedir, chVel);

        dBodyAddForce(m_body, m_control_force[0], m_control_force[1], m_control_force[2]); //+2.f*9.8f*70.f
        if (!b_lose_control || b_clamb_jump) //)&&!b_external_impulse
            dBodyAddForce(m_body, -sidedir[0] * vProj * (500.f + 200.f * b_clamb_jump) * m_friction_factor,
                -m.mass * (50.f) * (!b_lose_control && !(is_contact || (b_any_contacts))), //&&!b_climb
                -sidedir[2] * vProj * (500.f + 200.f * b_clamb_jump) * m_friction_factor);
#ifdef DEBUG
        if (debug_output().ph_dbg_draw_mask().test(phDbgCharacterControl))
        {
            const Fvector dipsp = Fvector().set(0, 0.02f, 0);
            debug_output().DBG_DrawLine(cast_fv(dBodyGetPosition(m_body)),
                Fvector().add(cast_fv(dBodyGetPosition(m_body)), Fvector().mul(cast_fv(sidedir), 1.f)),
                color_xrgb(0, 0, 255));
            debug_output().DBG_DrawLine(cast_fv(dBodyGetPosition(m_body)),
                Fvector().add(cast_fv(dBodyGetPosition(m_body)), Fvector().mul(cast_fv(m_control_force), 1.f / 1000.f)),
                color_xrgb(0, 0, 255));
            debug_output().DBG_DrawLine(Fvector().add(cast_fv(dBodyGetPosition(m_body)), dipsp),
                Fvector().add(Fvector().add(cast_fv(dBodyGetPosition(m_body)), dipsp),
                    Fvector().mul(cast_fv(dBodyGetForce(m_body)), 1.f / 1000.f)),
                color_xrgb(255, 0, 0));
        }
#endif
        // if(b_clamb_jump){
        // accurate_normalize(m_control_force);
        // dReal proj=dDOT(m_control_force,chVel);
        // if(proj<0.f)
        //		dBodyAddForce(m_body,-chVel[0]*500.f,-chVel[1]*500.f,-chVel[2]*500.f);
        //		}
    }

    if (b_jumping)
    {
        float air_factor = 1.f;
        if (b_lose_control && CastActorCharacter()) //
            air_factor = 10.f * m_air_control_factor;

        dReal proj = m_acceleration.x * chVel[0] + m_acceleration.z * chVel[2];

        const dReal* current_pos = dBodyGetPosition(m_body);
        dVector3 dif = {current_pos[0] - m_jump_depart_position[0], current_pos[1] - m_jump_depart_position[1],
            current_pos[2] - m_jump_depart_position[2]};
        dReal amag = _sqrt(m_acceleration.x * m_acceleration.x + m_acceleration.z * m_acceleration.z);
        if (amag > 0.f)
            if (dif[0] * m_acceleration.x / amag + dif[2] * m_acceleration.z / amag < 0.3f)
            {
                Fvector jump_fv = m_acceleration; //{ m_acceleration.x/amag*1000.f,0,m_acceleration.z/amag*1000.f }
                jump_fv.mul(1000.f / amag * air_factor);
                dBodyAddForce(m_body, jump_fv.x, 0, jump_fv.z);
            }
        if (proj < 0.f)
        {
            dReal vmag = chVel[0] * chVel[0] + chVel[2] * chVel[2];

            Fvector jump_fv = cast_fv(chVel);
            jump_fv.mul(3000.f * air_factor / vmag / amag * proj);
            dBodyAddForce(m_body, jump_fv.x, 0, jump_fv.z);
        }
    }
    // else
    // dBodyAddForce(m_body,-chVel[0]*10.f,-20.f*70.f*(!is_contact),-chVel[2]*10.f);

    // if(b_depart&&!b_clamb_jump&&!b_jump&&!b_jumping&&is_control&&!b_external_impulse&&!m_elevator_state.Active()){
    //		dBodyAddForce(m_body,0,-m.mass*ph_world->Gravity()/fixed_step,0);
    //}

    BodyCutForce(m_body, 5.f, 0.f);
//

//
#ifdef DEBUG
    if (debug_output().ph_dbg_draw_mask().test(phDbgCharacterControl))
    {
        const Fvector dipsp = Fvector().set(0, 0.02f, 0);
        debug_output().DBG_DrawLine(Fvector().add(cast_fv(dBodyGetPosition(m_body)), dipsp),
            Fvector().add(Fvector().add(cast_fv(dBodyGetPosition(m_body)), dipsp),
                Fvector().mul(cast_fv(dBodyGetForce(m_body)), 1.f / 1000.f)),
            color_xrgb(255, 0, 0));
    }
#endif
}

const float CHWON_ACCLEL_SHIFT = 0.4f;
const float CHWON_AABB_FACTOR = 1.f;
const float CHWON_ANG_COS = M_SQRT1_2;
const float CHWON_CALL_UP_SHIFT = 0.05f;
const float CHWON_CALL_FB_HIGHT = 1.5f;
const float CHWON_AABB_FB_FACTOR = 1.f;

void CPHSimpleCharacter::ValidateWalkOn()
{
    if (b_on_object || b_was_on_object)
    {
        b_clamb_jump = ValidateWalkOnMesh();
        ValidateWalkOnObject();
    }
    else
        b_clamb_jump = ValidateWalkOnMesh() && !m_elevator_state.NearDown();
}
bool CPHSimpleCharacter::ValidateWalkOnObject()
{
    // deside to stop clamb
    if (b_clamb_jump)
    {
        const dReal* current_pos = dBodyGetPosition(m_body);
        dVector3 dif = {current_pos[0] - m_clamb_depart_position[0], current_pos[1] - m_clamb_depart_position[1],
            current_pos[2] - m_clamb_depart_position[2]};
        if ( //! b_valide_wall_contact||
            //(dDOT(dif,dif)>CLAMB_DISTANCE*CLAMB_DISTANCE))	//	(m_wall_contact_normal[1]> M_SQRT1_2) ||
            //]
            dFabs(dif[1]) > CLAMB_DISTANCE)
        {
            b_clamb_jump = false;
            //	dBodySetLinearVel(m_body,0.f,0.f,0.f);
        }
    }

    // decide to clamb
    if (!m_elevator_state.Active() && b_valide_wall_contact && (m_contact_count > 1) &&
        (m_wall_contact_normal[1] < M_SQRT1_2) &&
        !b_side_contact) //&& dDOT(m_wall_contact_normal,m_ground_contact_normal)<.9f
    {
        // if( dDOT(m_wall_contact_normal,m_ground_contact_normal)<.999999f)
        // dVector3
        // diff={m_wall_contact_normal[0]-m_ground_contact_normal[0],m_wall_contact_normal[1]-m_ground_contact_normal[1],m_wall_contact_normal[2]-m_ground_contact_normal[2]};
        // if( dDOT(diff,diff)>0.001f)
        if (((m_wall_contact_position[0] - m_ground_contact_position[0]) * m_control_force[0] +
                (m_wall_contact_position[2] - m_ground_contact_position[2]) * m_control_force[2]) > 0.05f &&
            m_wall_contact_position[1] - m_ground_contact_position[1] > 0.01f)
            b_clamb_jump = true;
    }

    if (b_valide_wall_contact && (m_contact_count > 1) && b_clamb_jump)
        if (dFabs((m_wall_contact_position[0] - m_ground_contact_position[0]) + //*m_control_force[0]
                (m_wall_contact_position[2] - m_ground_contact_position[2])) > 0.05f && // 0.01f//*m_control_force[2]
            m_wall_contact_position[1] - m_ground_contact_position[1] > 0.01f)
            dVectorSet(m_clamb_depart_position, dBodyGetPosition(m_body));
    return b_clamb_jump;
}
bool CPHSimpleCharacter::ValidateWalkOnMesh()
{
    Fvector AABB, AABB_forbid, center, center_forbid, accel_add, accel;

    AABB.x = m_radius;
    AABB.y = m_radius;
    AABB.z = m_radius;

    AABB_forbid.set(AABB);
    // AABB_forbid.x*=0.7f;
    // AABB_forbid.z*=0.7f;
    AABB_forbid.y += m_radius;
    AABB_forbid.mul(CHWON_AABB_FB_FACTOR);

    accel_add.set(m_acceleration);
    float mag = accel_add.magnitude();
    if (!(mag > 0.f))
        return true;
    accel_add.mul(CHWON_ACCLEL_SHIFT / mag);
    accel.set(accel_add);
    accel.div(CHWON_ACCLEL_SHIFT);
    AABB.mul(CHWON_AABB_FACTOR);
    GetPosition(center);
    center.add(accel_add);
    center_forbid.set(center);
    center_forbid.y += CHWON_CALL_FB_HIGHT;
    center.y += m_radius + CHWON_CALL_UP_SHIFT;

    // perform single query / two usages
    Fbox query, tmp;
    Fvector q_c, q_d;
    query.set(center_forbid, center_forbid);
    query.grow(AABB_forbid);
    tmp.set(center, center);
    tmp.grow(AABB);
    query.merge(tmp);
    query.get_CD(q_c, q_d);

    XRC.box_options(0);
    XRC.box_query(inl_ph_world().ObjectSpace().GetStaticModel(), q_c, q_d);
    // Fvector fv_dir;fv_dir.mul(accel,1.f/mag);
    Fvector sd_dir;
    sd_dir.set(-accel.z, 0, accel.x);
    Fvector obb_fb;
    obb_fb.set(m_radius * 0.5f, m_radius * 2.f, m_radius * 0.7f);
    Fvector obb;
    obb.set(m_radius * 0.5f, m_radius, m_radius * 0.7f);
#ifdef DEBUG
    if (debug_output().ph_dbg_draw_mask().test(phDbgCharacterControl))
    {
        Fmatrix m;
        m.identity();
        m.i.set(sd_dir);
        m.k.set(accel);
        m.c.set(center);
        debug_output().DBG_DrawOBB(m, obb, color_xrgb(0, 255, 0));
        m.c.set(center_forbid);
        debug_output().DBG_DrawOBB(m, obb_fb, color_xrgb(255, 0, 0));
    }
#endif

    // if(XRC.r_end()!=XRC.r_begin()) return false;
    for (auto &Res : *XRC.r_get())
    {
        SGameMtl* m = GMLibrary().GetMaterialByIdx(Res.material);
        if (m->Flags.test(SGameMtl::flPassable))
            continue;
        // CDB::TRI* T = T_array + Res->id;
        Point vertices[3] = {
            Point((dReal*)&Res.verts[0]), Point((dReal*)&Res.verts[1]), Point((dReal*)&Res.verts[2])};
        if (__aabb_tri(Point((float*)&center_forbid), Point((float*)&AABB_forbid), vertices))
        {
            if (test_sides(center_forbid, sd_dir, accel, obb_fb, Res.id))
            {
#ifdef DEBUG
                if (debug_output().ph_dbg_draw_mask().test(phDbgCharacterControl))
                {
                    debug_output().DBG_DrawTri(&Res, color_xrgb(255, 0, 0));
                }
#endif
                b_side_contact = true;
                return false;
            }
            // cast_fv(side0).sub(Res->verts[1],Res->verts[0]);
            // cast_fv(side1).sub(Res->verts[2],Res->verts[1]);
            // dCROSS(norm,=,side0,side1);//optimize it !!!
            // cast_fv(norm).normalize();

            // if(dDOT(norm,(float*)&accel)<-CHWON_ANG_COS)
        }
    }

    for (auto &Res : *XRC.r_get())
    {
        // CDB::TRI* T = T_array + Res->id;
        SGameMtl* m = GMLibrary().GetMaterialByIdx(Res.material);
        if (m->Flags.test(SGameMtl::flPassable))
            continue;
        Point vertices[3] = {
            Point((dReal*)&Res.verts[0]), Point((dReal*)&Res.verts[1]), Point((dReal*)&Res.verts[2])};
        if (__aabb_tri(Point((float*)&center), Point((float*)&AABB), vertices))
        {
            if (test_sides(center, sd_dir, accel, obb, Res.id))
            {
#ifdef DEBUG
                if (debug_output().ph_dbg_draw_mask().test(phDbgCharacterControl))
                {
                    debug_output().DBG_DrawTri(&Res, color_xrgb(0, 255, 0));
                }
#endif
                return true;
            }
        }
    }
    return false;
}
void CPHSimpleCharacter::SetAcceleration(Fvector accel)
{
    if (!b_exist)
        return;

    if (!dBodyIsEnabled(m_body))
        if (!fsimilar(0.f, accel.magnitude()))
            Enable();
    m_acceleration = accel;
}
void CPHSimpleCharacter::SetCamDir(const Fvector& cam_dir) { m_cam_dir.set(cam_dir); }
static const float pull_force = 25.f;
void CPHSimpleCharacter::ApplyAcceleration()
{
    dVectorSetZero(m_control_force);
    // if(m_max_velocity<EPS) return;
    dMass m;
    dBodyGetMass(m_body, &m);

    // if(b_jump)
    //	m_control_force[1]=60.f*m.mass*2.f;

    if (b_lose_control)
    {
        dVectorSet(m_control_force, cast_fp(m_acceleration));
        dVectorMul(m_control_force, m.mass * m_air_control_factor);
        return;
    }

    dVector3 accel = {m_acceleration.x, 0.f, m_acceleration.z};
    if (m_elevator_state.Active())
    {
        if (m_elevator_state.NearState())
            m_elevator_state.GetControlDir(*(Fvector*)accel);
        if (m_elevator_state.ClimbingState())
        {
            // dVectorSet(m_control_force,accel);
            if (m_elevator_state.GetControlDir(*(Fvector*)m_control_force))
            {
                dVectorMul(m_control_force, m_friction_factor * m.mass * pull_force * 2.f);
                return;
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////
    dVector3 fvdir, sidedir;
    ////////////////////////////////////////////////
    // deside which force direction use
    dVector3 y = {0.f, 1.f, 0.f};
    dCROSS(sidedir, =, y, accel);
    if (b_clamb_jump && b_valide_wall_contact)
    {
        dCROSS(fvdir, =, sidedir, m_wall_contact_normal);
        accurate_normalize(fvdir);
        dVectorAddMul(m_control_force, fvdir, m.mass * pull_force);
    }
    else
    {
        if (b_valide_ground_contact && (m_ground_contact_normal[1] > M_SQRT1_2))
        { // M_SQRT1_2//0.8660f
            dCROSS(fvdir, =, sidedir, m_ground_contact_normal);
            accurate_normalize(fvdir);
            dVectorAddMul(m_control_force, fvdir, m.mass * pull_force);
        }
        else
        {
            dVectorSet(fvdir, accel);
            accurate_normalize(fvdir);
            dVectorAddMul(m_control_force, fvdir, m.mass * pull_force * 1.5f);
        }
    }
    if (!m_elevator_state.ClimbingState() && b_clamb_jump)
    { //&&m_wall_contact_normal[1]<M_SQRT1_2
        dVectorMul(m_control_force, 4.f);
        m_control_force[1] = dFabs(m_control_force[1]);
        m_control_force[0] = m_control_force[0] * accel[0] >= 0.f ? m_control_force[0] : -m_control_force[0];
        m_control_force[2] = m_control_force[2] * accel[2] >= 0.f ? m_control_force[2] : -m_control_force[2];
    }
    dVectorMul(m_control_force, m_friction_factor);
}

void CPHSimpleCharacter::IPosition(Fvector& pos)
{
    if (!b_exist)
    {
        pos.set(cast_fv(m_safe_position));
    }
    else
    {
        m_body_interpolation.InterpolatePosition(pos);
        pos.y -= m_radius;
    }
    VERIFY_BOUNDARIES(pos, phBoundaries, PhysicsRefObject());
    return;
}

void CPHSimpleCharacter::SetPosition(const Fvector& pos)
{
    VERIFY_BOUNDARIES(pos, phBoundaries, PhysicsRefObject());
    if (!b_exist)
        return;
    m_death_position[0] = pos.x;
    m_death_position[1] = pos.y + m_radius;
    m_death_position[2] = pos.z;
    m_safe_position[0] = pos.x;
    m_safe_position[1] = pos.y + m_radius;
    m_safe_position[2] = pos.z;
    b_death_pos = false;

    dGeomGetUserData(m_wheel)->pushing_b_neg = false;
    dGeomGetUserData(m_hat)->pushing_b_neg = false;
    dGeomGetUserData(m_geom_shell)->pushing_b_neg = false;
    dGeomGetUserData(m_hat)->pushing_b_neg = false;
    dGeomGetUserData(m_wheel)->pushing_neg = false;
    dGeomGetUserData(m_hat)->pushing_neg = false;
    dGeomGetUserData(m_geom_shell)->pushing_neg = false;
    dGeomGetUserData(m_hat)->pushing_neg = false;

    dBodySetPosition(m_body, pos.x, pos.y + m_radius, pos.z);
    CPHDisablingTranslational::Reinit();
    m_body_interpolation.ResetPositions();
    CPHObject::spatial_move();
}

void CPHSimpleCharacter::GetPosition(Fvector& vpos)
{
    if (!b_exist)
    {
        vpos.set(m_safe_position[0], m_safe_position[1] - m_radius, m_safe_position[2]);
    }
    else
    {
        const dReal* pos = dBodyGetPosition(m_body);
        dVectorSet((dReal*)&vpos, pos);
        vpos.y -= m_radius;
    }

    VERIFY_BOUNDARIES(vpos, phBoundaries, PhysicsRefObject());
}
void CPHSimpleCharacter::GetPreviousPosition(Fvector& pos)
{
    VERIFY(b_exist);
    VERIFY(!ph_world->Processing());
    m_body_interpolation.GetPosition(pos, 0);
}
void CPHSimpleCharacter::GetVelocity(Fvector& vvel) const
{
    if (!b_exist)
    {
        vvel.set(m_safe_velocity[0], m_safe_velocity[1], m_safe_velocity[2]);
        return;
    }
    const dReal* vel = dBodyGetLinearVel(m_body);
    dVectorSet((dReal*)&vvel, vel);
    return;
}

void CPHSimpleCharacter::SetVelocity(Fvector vel)
{
    if (!b_exist)
        return;
    float sq_mag = vel.square_magnitude();
    if (sq_mag > default_l_limit * default_l_limit)
    {
        float mag = _sqrt(sq_mag);
        vel.mul(default_l_limit / mag);
#ifdef DEBUG
        Msg("set velocity magnitude is too large %f", mag);
#endif
    }
    dBodySetLinearVel(m_body, vel.x, vel.y, vel.z);
}

void CPHSimpleCharacter::SetMas(dReal mass)
{
    m_mass = mass;
    if (!b_exist)
        return;
    dMass m;
    // dMassSetBox(&m,1,1000000.f,1000000.f,1000000.f);
    dMassSetSphere(&m, 1, 1000000.f / 2.f);
    dMassAdjust(&m, mass);
    dBodySetMass(m_body, &m);
}
#ifdef DEBUG
void CPHSimpleCharacter::OnRender()
{
#if 0
	if(!b_exist) return;
	Fmatrix m;
	m.identity();
	Fvector n=*(Fvector*)m_ground_contact_normal;
	n.mul(100.f);
	Fvector pos;
	GetPosition(pos);
	pos.y+=m_radius;

	GLevel().debug_renderer().draw_line(m,pos,*(Fvector*)m_control_force, color_rgba(256,0,0,1));
	GLevel().debug_renderer().draw_line(m,pos,n, 0xefffffff);


	Fvector scale;
	scale.set(0.35f,0.35f,0.35f);
	Fmatrix M;
	M.identity();
	M.scale(scale);
	M.c.set(pos);


	GLevel().debug_renderer().draw_ellipse(M, 0xffffffff);

#ifdef DRAW_BOXES
	GLevel().debug_renderer().draw_aabb			(m_bcenter,m_AABB.x,m_AABB.y,m_AABB.z,color_xrgb(0,0,255));
	GLevel().debug_renderer().draw_aabb			(m_bcenter_forbid,m_AABB_forbid.x,m_AABB_forbid.y,m_AABB_forbid.z,color_xrgb(255,0,0));
#endif
	///M.c.set(0.f,1.f,0.f);
	//Level().debug_renderer().draw_ellipse(M, 0xffffffff);
#endif
}
#endif

EEnvironment CPHSimpleCharacter::CheckInvironment()
{
    // if(b_on_ground||(is_control&&!b_lose_control))
    if (b_lose_control)
        return peInAir;
    // else									 return peAtWall;
    else if (m_elevator_state.ClimbingState())
        return peAtWall;

    return peOnGround;
}

void CPHSimpleCharacter::SetPhysicsRefObject(IPhysicsShellHolder* ref_object)
{
    m_phys_ref_object = ref_object;
    if (b_exist)
    {
        dGeomUserDataSetPhysicsRefObject(m_geom_shell, ref_object);
        dGeomUserDataSetPhysicsRefObject(m_wheel, ref_object);
        dGeomUserDataSetPhysicsRefObject(m_hat, ref_object);
        dGeomUserDataSetPhysicsRefObject(m_cap, m_phys_ref_object);
    }
}

/*
void CPHSimpleCharacter::CaptureObject(dBodyID body,const dReal* anchor)
{
    m_capture_joint=dJointCreateBall(0,0);
    Island().AddJoint(m_capture_joint);
    dJointAttach(m_capture_joint,m_body,body);
    dJointSetBallAnchor(m_capture_joint,anchor[0],anchor[1],anchor[2]);
    dJointSetFeedback(m_capture_joint,&m_capture_joint_feedback);
}
*/
/*
void CPHSimpleCharacter::CapturedSetPosition(const dReal* position)
{
    //if(!m_capture_joint) return;
    //dJointSetBallAnchor(m_capture_joint,position[0],position[1],position[2]);
}

void CPHSimpleCharacter::CheckCaptureJoint()
{
    //m_capture_joint_feedback
}

void CPHSimpleCharacter::doCaptureExist(bool& do_exist)
{
    do_exist=!!m_capture_joint;
}
*/
// float max_hit_vel_limit = 3000.f;
void CPHSimpleCharacter::SafeAndLimitVelocity()
{
    const float* linear_velocity = dBodyGetLinearVel(m_body);
    if (dV_valid(linear_velocity))
    {
        dReal mag = _sqrt(linear_velocity[0] * linear_velocity[0] + linear_velocity[1] * linear_velocity[1] +
            linear_velocity[2] * linear_velocity[2]); //;
        // limit velocity
        dReal l_limit;
        if (is_control && !b_lose_control)
            l_limit = m_max_velocity / phTimefactor;
        else
            l_limit = default_l_limit;

        if (b_external_impulse)
        {
            float sq_mag = m_acceleration.square_magnitude();
            float ll_limit = m_ext_imulse.dotproduct(cast_fv(linear_velocity)) * 10.f / fixed_step;
            if (sq_mag > EPS_L)
            {
                Fvector acc;
                acc.set(Fvector().mul(m_acceleration, 1.f / _sqrt(sq_mag)));
                Fvector vll;
                vll.mul(cast_fv(linear_velocity), 1.f / mag);
                float mxa = vll.dotproduct(acc);
                if (mxa * ll_limit > l_limit && !fis_zero(mxa))
                {
                    ll_limit = l_limit / mxa;
                }
            }
            // clamp(ll_limit,0.f,max_hit_vel_limit);
            if (ll_limit > l_limit)
                l_limit = ll_limit;
        }
        ////////////////////////////////////////////////////////////////////////////////////////////
        m_mean_y = m_mean_y * 0.9999f + linear_velocity[1] * 0.0001f;
        if (mag > l_limit)
        { // CutVelocity(m_l_limit,m_w_limit);
            if (!fis_zero(l_limit))
            {
                dReal f = mag / l_limit;

                if (b_lose_ground && linear_velocity[1] < 0.f && linear_velocity[1] > -default_l_limit)
                    dBodySetLinearVel(m_body, linear_velocity[0] / f, linear_velocity[1], linear_velocity[2] / f); /// f
                else
                    CutVelocity(l_limit, 0.f);
                // dBodySetLinearVel(m_body,linear_velocity[0]/f,linear_velocity[1]/f,linear_velocity[2]/f);///f
                if (is_control && !b_lose_control)
                {
                    dBodySetPosition(m_body, m_safe_position[0] + linear_velocity[0] * fixed_step,
                        m_safe_position[1] + linear_velocity[1] * fixed_step,
                        m_safe_position[2] + linear_velocity[2] * fixed_step);
                    VERIFY_BOUNDARIES(cast_fv(dBodyGetPosition(m_body)), phBoundaries, PhysicsRefObject());
                }
            }
            else
                dBodySetLinearVel(m_body, 0, 0, 0);
        }
    }
    else
    {
        dBodySetLinearVel(m_body, m_safe_velocity[0], m_safe_velocity[1], m_safe_velocity[2]);
    }

    if (!dV_valid(dBodyGetPosition(m_body)))
        dBodySetPosition(m_body, m_safe_position[0] - m_safe_velocity[0] * fixed_step,
            m_safe_position[1] - m_safe_velocity[1] * fixed_step, m_safe_position[2] - m_safe_velocity[2] * fixed_step);

    dVectorSet(m_safe_position, dBodyGetPosition(m_body));
    dVectorSet(m_safe_velocity, linear_velocity);
}

void CPHSimpleCharacter::SetObjectContactCallback(ObjectContactCallbackFun* callback)
{
    m_object_contact_callback = callback;
    if (!b_exist)
        return;

    dGeomUserDataSetObjectContactCallback(m_hat, callback);
    dGeomUserDataSetObjectContactCallback(m_geom_shell, callback);
    dGeomUserDataSetObjectContactCallback(m_wheel, callback);
}
void CPHSimpleCharacter::SetObjectContactCallbackData(void* data)
{
    VERIFY(b_exist);
    dGeomUserDataSetCallbackData(m_hat, data);
    dGeomUserDataSetCallbackData(m_geom_shell, data);
    dGeomUserDataSetCallbackData(m_wheel, data);
}

void CPHSimpleCharacter::AddObjectContactCallback(ObjectContactCallbackFun* callback)
{
    VERIFY(b_exist);

    dGeomUserDataAddObjectContactCallback(m_hat, callback);
    dGeomUserDataAddObjectContactCallback(m_geom_shell, callback);
    dGeomUserDataAddObjectContactCallback(m_wheel, callback);
}
void CPHSimpleCharacter::RemoveObjectContactCallback(ObjectContactCallbackFun* callback)
{
    VERIFY(m_object_contact_callback != callback);
    if (!b_exist)
        return;

    dGeomUserDataRemoveObjectContactCallback(m_hat, callback);
    dGeomUserDataRemoveObjectContactCallback(m_geom_shell, callback);
    dGeomUserDataRemoveObjectContactCallback(m_wheel, callback);
}

void CPHSimpleCharacter::Disable()
{
    // if(is_contact&&!is_control&&!b_lose_ground)
    {
        dGeomGetUserData(m_wheel)->pushing_neg = false;
        dGeomGetUserData(m_wheel)->pushing_b_neg = false;
        dGeomGetUserData(m_geom_shell)->pushing_neg = false;
        dGeomGetUserData(m_geom_shell)->pushing_b_neg = false;
        dGeomGetUserData(m_hat)->pushing_neg = false;
        dGeomGetUserData(m_hat)->pushing_b_neg = false;
        dGeomGetUserData(m_cap)->pushing_neg = false;
        dGeomGetUserData(m_cap)->pushing_b_neg = false;
        CPHCharacter::Disable();
    }
}
void CPHSimpleCharacter::Enable()
{
    if (!b_exist)
        return;
    dGeomGetUserData(m_wheel)->pushing_neg = false;
    dGeomGetUserData(m_wheel)->pushing_b_neg = false;
    dGeomGetUserData(m_geom_shell)->pushing_neg = false;
    dGeomGetUserData(m_geom_shell)->pushing_b_neg = false;
    dGeomGetUserData(m_hat)->pushing_neg = false;
    dGeomGetUserData(m_hat)->pushing_b_neg = false;
    dGeomGetUserData(m_cap)->pushing_neg = false;
    dGeomGetUserData(m_cap)->pushing_b_neg = false;
    CPHCharacter::Enable();
}
void CPHSimpleCharacter::EnableObject(CPHObject* obj)
{
    dGeomGetUserData(m_wheel)->pushing_neg = false;
    dGeomGetUserData(m_wheel)->pushing_b_neg = false;
    dGeomGetUserData(m_geom_shell)->pushing_neg = false;
    dGeomGetUserData(m_geom_shell)->pushing_b_neg = false;
    dGeomGetUserData(m_hat)->pushing_neg = false;
    dGeomGetUserData(m_hat)->pushing_b_neg = false;
    dGeomGetUserData(m_cap)->pushing_neg = false;
    dGeomGetUserData(m_cap)->pushing_b_neg = false;
    CPHCharacter::EnableObject(obj);
}
void CPHSimpleCharacter::SetWheelContactCallback(ObjectContactCallbackFun* callback)
{
    VERIFY(b_exist);
    dGeomUserDataSetObjectContactCallback(m_wheel, callback);
}
void CPHSimpleCharacter::SetStaticContactCallBack(ContactCallbackFun* callback)
{
    if (!b_exist)
        return;

    dGeomUserDataSetContactCallback(m_hat, callback);
    dGeomUserDataSetContactCallback(m_geom_shell, callback);
    dGeomUserDataSetContactCallback(m_wheel, callback);
}

ObjectContactCallbackFun* CPHSimpleCharacter::ObjectContactCallBack() { return m_object_contact_callback; }
u16 CPHSimpleCharacter::RetriveContactBone()
{
    Fvector dir;
    m_collision_damage_info.HitDir(dir);
    collide::ray_defs Q(m_collision_damage_info.HitPos(), dir, m_radius, CDB::OPT_ONLYNEAREST | CDB::OPT_CULL,
        collide::rqtBoth); // CDB::OPT_ONLYFIRST CDB::OPT_ONLYNEAREST
    RQR.r_clear();
    u16 contact_bone = 0;
    //	IGameObject* object		=	smart_cast<IGameObject*>(m_phys_ref_object);
    // VERIFY	(object)	;
    VERIFY(!fis_zero(Q.dir.square_magnitude()));
    if (inl_ph_world().ObjectSpace().RayQuery(RQR, m_phys_ref_object->ObjectCollisionModel(), Q))
    {
        collide::rq_result* R = RQR.r_begin();
        contact_bone = (u16)R->element;
        // int y=result.r_count();
        // for (int k=0; k<y; ++k)
        //{
        //	ICollisionForm::RayQuery::Result* R = result.r_begin()+k;
        //	if(is_Door(R->element,i))
        //	{
        //		i->second.Use();
        //		return false;

        //	}
        //}
    }
    else
    {
        // IKinematics* K=smart_cast<IKinematics*>(m_phys_ref_object->ObjectVisual());
        IKinematics* K = m_phys_ref_object->ObjectKinematics();
        u16 count = K->LL_BoneCount();
        CBoneInstance* bone_instances = &K->LL_GetBoneInstance(0);
        Fvector pos_in_object;
        pos_in_object.sub(m_collision_damage_info.HitPos(),
            m_phys_ref_object
                ->ObjectPosition()); // vector from object center to contact position currently in global frame
        Fmatrix object_form;
        object_form.set(m_phys_ref_object->ObjectXFORM());
        object_form.transpose();
        object_form.transform_dir(
            pos_in_object); // project pos_in_object on object axes now it is position of contact in object frame
        float sq_dist = dInfinity;
        for (u16 i = 0; i < count; ++i)
        {
            Fvector c_to_bone;
            c_to_bone.sub(bone_instances[i].mTransform.c, pos_in_object);
            float temp_sq_dist = c_to_bone.square_magnitude();
            if (temp_sq_dist < sq_dist)
            {
                sq_dist = temp_sq_dist;
                contact_bone = i;
            }
        }
        VERIFY(dInfinity != sq_dist);
    }
    return contact_bone;
}

///////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
void CPHSimpleCharacter::InitContact(dContact* c, bool& do_collide, u16 material_idx_1, u16 material_idx_2)
{
    const dReal* normal = c->geom.normal;
    const dReal* pos = c->geom.pos;
    const dGeomID g1 = c->geom.g1;
    const dGeomID g2 = c->geom.g2;
    bool bo1 = (g1 == m_wheel) || g1 == m_cap_transform || g1 == m_shell_transform || g1 == m_hat_transform;

    // SGameMtl* tri_material=GMLibrary().GetMaterialByIdx((u16)c->surface.mode);

    u16 contact_material = bo1 ? material_idx_2 : material_idx_1;
    SGameMtl* tri_material = GMLibrary().GetMaterialByIdx(contact_material);

    bool bClimable = !!tri_material->Flags.test(SGameMtl::flClimable);
    if (is_control && m_elevator_state.ClimbingState())
    {
        c->surface.mu = 0.f;
        c->surface.soft_cfm = world_cfm * 2.f;
        c->surface.soft_erp = world_erp;
        b_any_contacts = true;
        is_contact = true;
    }
    u16 foot_material_idx = ((dxGeomUserData*)dGeomGetData(m_wheel))->tri_material;
    if (tri_material->Flags.test(SGameMtl::flPassable) && !do_collide)
    {
        UpdateStaticDamage(c, tri_material, bo1);
        foot_material_update(contact_material, foot_material_idx);
        return;
    }
    if (do_collide)
    {
        b_any_contacts = true;
        is_contact = true;
    }
    dReal spring_rate = def_spring_rate;
    dReal dumping_rate = def_dumping_rate;
    bool object = (dGeomGetBody(g1) && dGeomGetBody(g2));
    b_on_object = b_on_object || object;

    ////////////////////////it is necessary to shift the collision!!
    //////////////
    FootProcess(c, do_collide, bo1);
    if (!do_collide)
        return;
    if (g1 == m_hat_transform || g2 == m_hat_transform)
    {
        b_side_contact = true;
        MulSprDmp(c->surface.soft_cfm, c->surface.soft_erp, spring_rate, dumping_rate);
        c->surface.mu = 0.00f;
    }

    if (object)
    {
        spring_rate *= 10.f;
        dBodyID b;
        u16 obj_material_idx = u16(-1);
        if (bo1)
        {
            b = dGeomGetBody(c->geom.g2);
            obj_material_idx = material_idx_2;
        }
        else
        {
            b = dGeomGetBody(c->geom.g1);
            obj_material_idx = material_idx_1;
        }
        UpdateDynamicDamage(c, obj_material_idx, b, bo1);
        if (g1 == m_wheel || g2 == m_wheel)
        {
            dxGeomUserData* ud = bo1 ? retrieveGeomUserData(c->geom.g2) : retrieveGeomUserData(c->geom.g1);
            foot_material_idx = ud->material;
        }
        contact_material = obj_material_idx;
    }

    foot_material_update(contact_material, foot_material_idx);

    // if(!((g1==m_wheel) || (g2==m_wheel)||(m_elevator_state.ClimbingState())  ))//
    //	return;

    float friction = 1.f;
    if (!object && !b_side_contact)
    {
        friction = c->surface.mu;
    }
    if (m_friction_factor < friction)
        m_friction_factor = friction;
    ++m_contact_count;

    if (bo1)
    {
        if (normal[1] > m_ground_contact_normal[1] || !b_valide_ground_contact) //
        {
            dVectorSet(m_ground_contact_normal, normal);
            dVectorSet(m_ground_contact_position, pos);
            b_valide_ground_contact = true;
        }
        if (dXZDot(normal, cast_fp(m_acceleration)) < dXZDot(m_wall_contact_normal, cast_fp(m_acceleration)) ||
            !b_valide_wall_contact)
        {
            dVectorSet(m_wall_contact_normal, c->geom.normal);
            dVectorSet(m_wall_contact_position, c->geom.pos);
            b_valide_wall_contact = true;
        }
    }
    else
    {
        if (normal[1] < -m_ground_contact_normal[1] || !b_valide_ground_contact) //
        {
            dVectorSetInvert(m_ground_contact_normal, normal);
            dVectorSet(m_ground_contact_position, pos);
            b_valide_ground_contact = true;
        }
        if (dXZDot(normal, cast_fp(m_acceleration)) > -dXZDot(m_wall_contact_normal, cast_fp(m_acceleration)) ||
            !b_valide_wall_contact) //
        {
            dVectorSetInvert(m_wall_contact_normal, normal);
            dVectorSet(m_wall_contact_position, pos);
            b_valide_wall_contact = true;
        }
    }
    float soft_param = dumping_rate + normal[1] * (1.f - dumping_rate); //=(1.f-normal[1])*dumping_rate +normal[1]
    if (is_control)
    { //&&!b_lose_control||b_jumping
        if (g1 == m_wheel || g2 == m_wheel && !bClimable)
        {
            c->surface.mu = 0.f; // 0.00f;
        }
        else
        {
            c->surface.mu = 0.00f;
        }
        MulSprDmp(c->surface.soft_cfm, c->surface.soft_erp, spring_rate, soft_param);
    }
    else
    {
        MulSprDmp(c->surface.soft_cfm, c->surface.soft_erp, spring_rate, soft_param);
        c->surface.mu *= (1.f + b_clamb_jump * 3.f) * m_friction_factor;
    }
    UpdateStaticDamage(c, tri_material, bo1);
}

void CPHSimpleCharacter::FootProcess(dContact* c, bool& do_collide, bool bo)
{
    const dGeomID g1 = c->geom.g1;
    const dGeomID g2 = c->geom.g2;
    dGeomID g = g1;
    float sign = 1.f;
    dReal* normal = c->geom.normal;
    if (!bo)
    {
        g = g2;
        sign = -1.f;
    }
    if (m_elevator_state.ClimbingState() || !is_control || !b_clamb_jump || b_was_side_contact || b_side_contact ||
        b_jumping || b_jump)
    {
        if (g == m_wheel && sign * normal[1] < 0.f)
            do_collide = false;
        return;
    }

    ///////////////////////////////////////////////////////////////////////////////////

    dReal* pos = c->geom.pos;

    float c_pos = pos[1] - dBodyGetPosition(m_body)[1];

    /////////////////////////////////////////////////////////////////////////////////////

    if (dXZDot(m_acceleration, cast_fv(normal)) * sign > 0.f)
        return;
    if (g == m_wheel)
    {
        if (sign * normal[1] <= 0.f)
        {
            normal[0] = normal[2] = 0.f;
            normal[1] = sign;
            c->geom.depth = c_pos + m_radius;
        }
    }

    if (g == m_shell_transform)
    {
        if (c_pos < 0.f)
        {
            normal[0] = normal[2] = 0.f;
            normal[1] = sign;
            c->geom.depth = c_pos + m_radius;
        }
    }
}
void CPHSimpleCharacter::GroundNormal(Fvector& norm)
{
    if (m_elevator_state.ClimbingState())
    {
        m_elevator_state.GetLeaderNormal(norm);
    }
    else
    {
        norm.set(*((Fvector*)m_ground_contact_normal));
    }
}
u16 CPHSimpleCharacter::ContactBone() { return RetriveContactBone(); }
void CPHSimpleCharacter::SetMaterial(u16 material)
{
    if (!b_exist)
        return;
    dGeomGetUserData(m_geom_shell)->material = material;
    dGeomGetUserData(m_wheel)->material = material;
    dGeomGetUserData(m_cap)->material = material;
    dGeomGetUserData(m_hat)->material = material;
}
void CPHSimpleCharacter::get_State(SPHNetState& state)
{
    CPHCharacter::get_State(state);

    state.previous_position.y -= m_radius;
}
void CPHSimpleCharacter::set_State(const SPHNetState& state) { CPHCharacter::set_State(state); }
void CPHSimpleCharacter::get_spatial_params()
{
    spatialParsFromDGeom((dGeomID)m_space, spatial.sphere.P, AABB, spatial.sphere.R);
}

float CPHSimpleCharacter::FootRadius()
{
    if (b_exist)
        return m_radius;
    else
        return 0.f;
}
void CPHSimpleCharacter::DeathPosition(Fvector& deathPos)
{
    if (!b_exist)
        return;

    if (b_death_pos)
        deathPos.set(m_death_position);
    else
    {
        deathPos.set(cast_fv(dBodyGetPosition(m_body)));
        if (!_valid(deathPos))
            deathPos.set(m_safe_position);
    }
    deathPos.y -= m_radius;
}
void CPHSimpleCharacter::AddControlVel(const Fvector& vel)
{
    m_acceleration.add(vel);
    m_max_velocity += vel.magnitude();
}
void CPHSimpleCharacter::SetInitiated() { m_collision_damage_info.is_initiated = true; }
bool CPHSimpleCharacter::IsInitiated() const { return m_collision_damage_info.is_initiated; }
u16 CPHSimpleCharacter::DamageInitiatorID() const
{
    u16 ret = u16(-1); // m_collision_damage_info.DamageInitiatorID();

    IPhysicsShellHolder* object = 0;
    if (m_collision_damage_info.m_obj_id != u16(-1))
    {
        IGameObject* obj = inl_ph_world().LevelObjects().net_Find(m_collision_damage_info.m_obj_id);
        VERIFY(!obj || smart_cast<IPhysicsShellHolder*>(obj));
        object = smart_cast<IPhysicsShellHolder*>(obj);
    }
    if (object && !object->ObjectGetDestroy())
    {
        IDamageSource* ds = object->ObjectCastIDamageSource();
        if (ds)
            ret = ds->Initiator();
    }
    //	return u16(-1);

    if (ret == u16(-1))
        ret = m_phys_ref_object->ObjectID();
    return ret;
}

IGameObject* CPHSimpleCharacter::DamageInitiator() const
{
    VERIFY(m_phys_ref_object);
    if (m_collision_damage_info.m_dmc_type == SCollisionDamageInfo::ctStatic)
        return smart_cast<IGameObject*>(m_phys_ref_object);
    u16 initiator_id = DamageInitiatorID();
    VERIFY(initiator_id != u16(-1));
    if (initiator_id == m_phys_ref_object->ObjectID())
        return smart_cast<IGameObject*>(m_phys_ref_object);
    else
    {
        return inl_ph_world().LevelObjects().net_Find(initiator_id);
    }
}

CPHSimpleCharacter::SCollisionDamageInfo::SCollisionDamageInfo() { Construct(); }
void CPHSimpleCharacter::SCollisionDamageInfo::Construct()
{
    m_contact_velocity = 0.f;
    SCollisionDamageInfo::Reinit();
    m_hit_type = ALife::eHitTypeStrike;
    // m_damege_contact;

    // m_dmc_signum;
    // m_dmc_type;
}
float CPHSimpleCharacter::SCollisionDamageInfo::ContactVelocity() const
{
    dReal ret = m_contact_velocity;
    m_contact_velocity = 0;
    return ret;
}

void CPHSimpleCharacter::SCollisionDamageInfo::HitDir(Fvector& dir) const
{
    dir.set(m_damege_contact.geom.normal[0] * m_dmc_signum, m_damege_contact.geom.normal[1] * m_dmc_signum,
        m_damege_contact.geom.normal[2] * m_dmc_signum);
}

// u16 CPHSimpleCharacter::SCollisionDamageInfo::DamageInitiatorID() const
//{
//	//if(!m_object)
//				//return u16(-1);
//	IPhysicsShellHolder* object =static_cast<IPhysicsShellHolder*>(Level().Objects.net_Find(m_obj_id));
//	if(!object)return u16(-1);
//	IDamageSource* ds=m_object->cast_IDamageSource();
//	if(ds) return ds->Initiator();
//	return u16(-1);
//}
void CPHSimpleCharacter::SCollisionDamageInfo::Reinit()
{
    // m_damege_contact;

    m_obj_id = u16(-1);
    m_hit_callback = NULL;
    m_contact_velocity = 0;
    is_initiated = false;

    // float					m_dmc_signum;
    // enum{ctStatic,ctObject}	m_dmc_type;
}
bool CPHSimpleCharacter::GetAndResetInitiated()
{
    bool ret = m_collision_damage_info.is_initiated;
    m_collision_damage_info.is_initiated = false;
    return ret;
}
void CPHSimpleCharacter::GetSmothedVelocity(Fvector& vvel)
{
    if (!b_exist)
    {
        vvel.set(0, 0, 0);
        return;
    }
    vvel.set(m_last_move);

    // if(IsEnabled()&&m_count<m_frames)
    //{
    //	vvel.set(m_mean_velocity.sum);
    //	vvel.mul(1.f/(m_frames-m_count)/fixed_step);
    //}
    // else
    //{
    //	GetSavedVelocity(vvel);
    //}
}

CElevatorState* CPHSimpleCharacter::ElevatorState() { return &m_elevator_state; }
ICollisionHitCallback* CPHSimpleCharacter::HitCallback() const { return m_collision_damage_info.m_hit_callback; }
const float resolve_depth = 0.05f;
static float restrictor_depth = 0.f;
void CPHSimpleCharacter::TestRestrictorContactCallbackFun(
    bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    dGeomID g_this = NULL;
    dGeomID g_obj = NULL;
    if (bo1)
    {
        g_this = c.geom.g1;
        g_obj = c.geom.g2;
    }
    else
    {
        g_this = c.geom.g2;
        g_obj = c.geom.g1;
    }
    dxGeomUserData* obj_data = retrieveGeomUserData(g_obj);
    if (!obj_data)
        return;
    if (!obj_data->ph_object)
        return;
    if (obj_data->ph_object->CastType() != tpCharacter)
        return;
    CPHActorCharacter* actor_character = (static_cast<CPHCharacter*>(obj_data->ph_object))->CastActorCharacter();
    if (!actor_character)
        return;
    CPHSimpleCharacter* ch_this = static_cast<CPHSimpleCharacter*>(retrieveGeomUserData(g_this)->ph_object);
    VERIFY(ch_this);
    save_max(restrictor_depth, c.geom.depth);
    do_colide = true;
    c.surface.mu = 0.f;
}

bool CPHSimpleCharacter::UpdateRestrictionType(CPHCharacter* ach)
{
    VERIFY(ph_world);
    VERIFY(ph_world->Exist());
    if (m_restriction_type == m_new_restriction_type)
        return true;
    ach->Enable();
    Enable();
    restrictor_depth = 0.f;
    // bool state=IsEnabled();

    ph_world->Freeze();
    ERestrictionType old = m_restriction_type;
    m_restriction_type = m_new_restriction_type;
    AddObjectContactCallback(TestRestrictorContactCallbackFun);
    UnFreeze();
    ph_world->StepTouch();
    ach->SwitchOFFInitContact();
    if (restrictor_depth < resolve_depth)
    {
        RemoveObjectContactCallback(TestRestrictorContactCallbackFun);
        ph_world->UnFreeze();
        ach->SwitchInInitContact();
        // if(!state)Disable();
        return true;
    }
    u16 num_steps = 2 * (u16)iCeil(restrictor_depth / resolve_depth);
    for (u16 i = 0; num_steps > i; ++i)
    {
        // Calculate(Fvector().set(0,0,0),Fvector().set(1,0,0),0,0,0,0);
        restrictor_depth = 0.f;
        ach->Enable();
        Enable();
        // ach->ApplyForce(0,ph_world->Gravity()*ach->Mass(),0);
        ph_world->Step();

        if (restrictor_depth < resolve_depth)
        {
            RemoveObjectContactCallback(TestRestrictorContactCallbackFun);
            ph_world->UnFreeze();
            ach->SwitchInInitContact();
            // if(!state)Disable();
            return true;
        }
    }
    RemoveObjectContactCallback(TestRestrictorContactCallbackFun);
    ach->SwitchInInitContact();
    ph_world->UnFreeze();
    // if(!state)Disable();
    m_new_restriction_type = old;
#ifdef DEBUG
    if (debug_output().ph_dbg_draw_mask1().test(ph_m1_DbgActorRestriction))
        Msg("restriction can not change change small -> large");
#endif
    return false;
}
bool CPHSimpleCharacter::TouchRestrictor(ERestrictionType rttype)
{
    b_collision_restrictor_touch = true;
    return rttype == RestrictionType();
}

IC bool valide_res(u16& res_material_idx, const collide::rq_result& R)
{
    if (!R.O)
    {
        CDB::TRI* tri = inl_ph_world().ObjectSpace().GetStaticTris() + R.element;
        VERIFY(tri);
        res_material_idx = tri->material;
        return !ignore_material(res_material_idx);
    }
    IRenderVisual* V = R.O->Visual();
    if (!V)
        return false;
    IKinematics* K = V->dcast_PKinematics();
    CBoneData& bd = K->LL_GetData((u16)R.element);
    res_material_idx = bd.game_mtl_idx;
    return true;
}

bool PickMaterial(
    u16& res_material_idx, const Fvector& pos_, const Fvector& dir_, float range_, IGameObject* ignore_object)
{
    Fvector pos = pos_;
    pos.y += EPS_L;
    Fvector dir = dir_;
    float range = range_;
    collide::rq_result R;
    res_material_idx = GAMEMTL_NONE_IDX;
    while (inl_ph_world().ObjectSpace().RayPick(pos, dir, range, collide::rqtBoth, R, ignore_object))
    {
        float r_range = R.range + EPS_L;
        Fvector next_pos = pos.mad(dir, r_range);
        float next_range = range - r_range;
        if (valide_res(res_material_idx, R))
            return true;
        range = next_range;
        pos = next_pos;
        if (range < EPS_L)
            return false;
    }
    return false;
}
const float material_pick_dist = 0.5f;
const float material_pick_upset = 0.5f;
const float material_update_tolerance = 0.1f;
void CPHSimpleCharacter::update_last_material()
{
    // if( ignore_material( *p_lastMaterialIDX ) )
    //{
    Fvector pos;
    GetPosition(pos);
    pos.y += material_pick_upset;
    if (m_last_picked_material != GAMEMTL_NONE_IDX && pos.similar(m_last_environment_update, material_update_tolerance))
    {
        *p_lastMaterialIDX = m_last_picked_material;
        return;
    }
    u16 new_material;
    VERIFY(!PhysicsRefObject() || smart_cast<IGameObject*>(PhysicsRefObject()));
    if (PickMaterial(new_material, pos, Fvector().set(0, -1, 0), material_pick_dist + material_pick_upset,
            smart_cast<IGameObject*>(PhysicsRefObject())))
    {
        m_last_picked_material = new_material;
        *p_lastMaterialIDX = new_material;
        m_last_environment_update = pos;
    }
    //}
    // m->flActorObstacle
}

void CPHSimpleCharacter::SetNonInteractive(bool v) { b_non_interactive = v; }
void CPHSimpleCharacter::Collide()
{
    OnStartCollidePhase();

    inherited::Collide();
    if (injuriousMaterialIDX == GAMEMTL_NONE_IDX && (*p_lastMaterialIDX) != GAMEMTL_NONE_IDX &&
        GMLibrary().GetMaterialByIdx(*p_lastMaterialIDX)->Flags.test(SGameMtl::flInjurious))
        injuriousMaterialIDX = *p_lastMaterialIDX;
}
void CPHSimpleCharacter::OnStartCollidePhase() { injuriousMaterialIDX = GAMEMTL_NONE_IDX; }
void CPHSimpleCharacter::NetRelcase(IPhysicsShellHolder* O)
{
    inherited::NetRelcase(O);
    m_elevator_state.NetRelcase(O);
}
