#include "StdAfx.h"

#include "PHCharacter.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "ExtendedGeom.h"
#include "IPhysicsShellHolder.h"

#include "xrCDB/Intersect.hpp"
#include "xrEngine/GameMtlLib.h"

#include "tri-colliderknoopc/__aabb_tri.h"
#include "ode/ode/src/util.h"
#include "ph_valid_ode.h"
#include "PHAICharacter.h"
#include "PHActorCharacter.h"

CPHCharacter::CPHCharacter(void) : CPHDisablingTranslational()
{
    m_params.acceleration = 0.001f;
    m_params.velocity = 0.0001f;
    m_body = NULL;
    m_safe_velocity[0] = 0.f;
    m_safe_velocity[1] = 0.f;
    m_safe_velocity[2] = 0.f;
    m_mean_y = 0.f;
    m_new_restriction_type = m_restriction_type = rtNone;
    b_actor_movable = true;
    p_lastMaterialIDX = &lastMaterialIDX;
    lastMaterialIDX = GAMEMTL_NONE_IDX;
    injuriousMaterialIDX = GAMEMTL_NONE_IDX;
    m_creation_step = u64(-1);
    b_in_touch_resrtrictor = false;
    m_current_object_radius = -1.f;
}

CPHCharacter::~CPHCharacter(void) {}
void CPHCharacter::FreezeContent()
{
    dBodyDisable(m_body);
    CPHObject::FreezeContent();
}
void CPHCharacter::UnFreezeContent()
{
    dBodyEnable(m_body);
    CPHObject::UnFreezeContent();
}
void CPHCharacter::getForce(Fvector& force)
{
    if (!b_exist)
        return;
    force.set(*(Fvector*)dBodyGetForce(m_body));
}
void CPHCharacter::setForce(const Fvector& force)
{
    if (!b_exist)
        return;
    dBodySetForce(m_body, force.x, force.y, force.z);
}

void CPHCharacter::get_State(SPHNetState& state)
{
    GetPosition(state.position);
    m_body_interpolation.GetPosition(state.previous_position, 0);
    GetVelocity(state.linear_vel);
    getForce(state.force);

    state.angular_vel.set(0.f, 0.f, 0.f);
    state.quaternion.identity();
    state.previous_quaternion.identity();
    state.torque.set(0.f, 0.f, 0.f);
    //	state.accel = GetAcceleration();
    //	state.max_velocity = GetMaximumVelocity();

    if (!b_exist)
    {
        state.enabled = false;
        return;
    }
    state.enabled = CPHObject::is_active(); //!!dBodyIsEnabled(m_body);
}
void CPHCharacter::set_State(const SPHNetState& state)
{
    m_body_interpolation.SetPosition(state.previous_position, 0);
    m_body_interpolation.SetPosition(state.position, 1);
    SetPosition(state.position);
    SetVelocity(state.linear_vel);
    setForce(state.force);

    //	SetAcceleration(state.accel);
    //	SetMaximumVelocity(state.max_velocity);

    if (!b_exist)
        return;
    if (state.enabled)
    {
        Enable();
    };
    if (!state.enabled)
    {
        Disable();
    };
    VERIFY2(dBodyStateValide(m_body), "WRONG BODYSTATE WAS SET");
}

void CPHCharacter::Disable()
{
    CPHObject::deactivate();
    dBodyDisable(m_body);
    m_body_interpolation.ResetPositions();
}

void CPHCharacter::Enable()
{
    if (!b_exist)
        return;
    CPHObject::activate();
    dBodyEnable(m_body);
}

void CarHitCallback(bool& /**do_colide/**/, dContact& /**c/**/) {}
void CPHCharacter::GetSavedVelocity(Fvector& vvel)
{
    if (IsEnabled())
        vvel.set(m_safe_velocity);
    else
        GetVelocity(vvel);
}

void CPHCharacter::CutVelocity(float l_limit, float /*a_limit*/)
{
    dVector3 limitedl, diffl;
    if (dVectorLimit(dBodyGetLinearVel(m_body), l_limit, limitedl))
    {
        dVectorSub(diffl, limitedl, dBodyGetLinearVel(m_body));
        dBodySetLinearVel(m_body, diffl[0], diffl[1], diffl[2]);
        dBodySetAngularVel(m_body, 0.f, 0.f, 0.f);
        dxStepBody(m_body, fixed_step);
        dBodySetLinearVel(m_body, limitedl[0], limitedl[1], limitedl[2]);
    }
}

const Fmatrix& CPHCharacter::XFORM() const
{
    return m_phys_ref_object->ObjectXFORM(); //>renderable.xform;
}
void CPHCharacter::get_LinearVel(Fvector& velocity) const { GetVelocity(velocity); }
void CPHCharacter::get_AngularVel(Fvector& velocity) const { velocity.set(0, 0, 0); }
const Fvector& CPHCharacter::mass_Center() const { return cast_fv(dBodyGetLinearVel(m_body)); }
void CPHCharacter::get_body_position(Fvector& p)
{
    VERIFY(b_exist);
    VERIFY(get_body());
    p.set(cast_fv(dBodyGetPosition(get_body())));
}

void virtual_move_collide_callback(bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    if (!do_collide)
        return;
    do_collide = false;
    SGameMtl* oposite_matrial = bo1 ? material_1 : material_2;
    if (oposite_matrial->Flags.test(SGameMtl::flPassable))
        return;

    dxGeomUserData* my_data = retrieveGeomUserData(bo1 ? c.geom.g1 : c.geom.g2);
    dxGeomUserData* oposite_data = retrieveGeomUserData(bo1 ? c.geom.g2 : c.geom.g1);
    VERIFY(my_data);
    if (oposite_data && oposite_data->ph_ref_object == my_data->ph_ref_object)
        return;

    // if( c.geom.depth > camera_collision_sckin_depth/2.f )
    // cam_collided = true;
    // if( !cam_step )
    // return;

    c.surface.mu = 0;
    c.surface.soft_cfm = 0.01f;
    dJointID contact_joint =
        dJointCreateContact(0, ContactGroup, &c); // dJointCreateContactSpecial(0, ContactGroup, &c);
    CPHObject* obj = (CPHObject*)my_data->callback_data;
    VERIFY(obj);

    obj->Island().DActiveIsland()->ConnectJoint(contact_joint);

    if (bo1)
        dJointAttach(contact_joint, dGeomGetBody(c.geom.g1), 0);
    else
        dJointAttach(contact_joint, 0, dGeomGetBody(c.geom.g2));
}

void CPHCharacter::fix_body_rotation()
{
    dBodyID b = get_body(); // GetBody();
    if (b)
    {
        dMatrix3 R;
        dRSetIdentity(R);
        dBodySetAngularVel(b, 0.f, 0.f, 0.f);
        dBodySetRotation(b, R);
    }
}

CPHCharacter* create_ai_character() { return new CPHAICharacter(); }
CPHCharacter* create_actor_character(bool single_game) { return new CPHActorCharacter(single_game); }
