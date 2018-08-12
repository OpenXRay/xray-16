#include "StdAfx.h"
#ifdef DEBUG

#include "PHDebug.h"
#endif
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "Include/xrRender/Kinematics.h"
#include "xrPhysics/ExtendedGeom.h"

CCar::SWheel::SWheelCollisionParams::SWheelCollisionParams()
{
    spring_factor = 1;
    damping_factor = 1;
    mu_factor = 1;
}
IC void CCar::SWheel::applywheelCollisionParams(
    const dxGeomUserData* ud, bool& do_colide, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    if (ud && ud->object_callbacks && ud->object_callbacks->HasCallback(WheellCollisionCallback))
    {
        SWheelCollisionParams& cp = *((SWheelCollisionParams*)(ud->callback_data));
        dSurfaceParameters& sp = c.surface;
        sp.mu *= cp.mu_factor;
        MulSprDmp(sp.soft_cfm, sp.soft_cfm, cp.spring_factor, cp.damping_factor);
    }
}

void CCar::SWheel::WheellCollisionCallback(
    bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    dxGeomUserData* ud1 = PHRetrieveGeomUserData(c.geom.g1);
    dxGeomUserData* ud2 = PHRetrieveGeomUserData(c.geom.g2);
    applywheelCollisionParams(ud1, do_colide, c, material_1, material_2);
    applywheelCollisionParams(ud2, do_colide, c, material_1, material_2);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CCar::WheelHit(float P, s16 element, ALife::EHitType hit_type)
{
    xr_map<u16, SWheel>::iterator i = m_wheels_map.find(element);
    if (i != m_wheels_map.end())
    {
        i->second.Hit(P);
        return true;
    }
    else
        return false;
}
void CCar::SWheel::Init()
{
    if (inited)
        return;
    BONE_P_PAIR_CIT bone = bone_map.find(bone_id);
    R_ASSERT2(bone->second.element, "No Element was created for wheel. Check collision is set");
    bone->second.element->set_DynamicLimits(default_l_limit, default_w_limit * 100.f);
    CPhysicsElement* e = bone->second.element;
    CPhysicsJoint* j = bone->second.joint;
    radius = e->getRadius();
    R_ASSERT2(j, "No wheel joint was set for a wheel");
    joint = j;
    joint->SetBackRef(&joint);

    // R_ASSERT2(dJointGetType(joint->GetDJoint())==dJointTypeHinge2,"No wheel join was set for a wheel, only
    // wheel-joint valid!!!");
    R_ASSERT2(joint->IsWheelJoint(), "No wheel join was set for a wheel, only wheel-joint valid!!!");

    ApplyDriveAxisVelTorque(0.f, 0.f);
    e->add_ObjectContactCallback(WheellCollisionCallback);
    e->set_CallbackData((void*)&collision_params);
    e->SetAirResistance(0, 0);
    inited = true;
}
void CCar::SWheel::Load(LPCSTR section)
{
    IKinematics* K = PKinematics(car->Visual());
    CInifile* ini = K->LL_UserData();
    VERIFY(ini);
    if (ini->section_exist(section))
    {
        collision_params.damping_factor =
            READ_IF_EXISTS(ini, r_float, section, "damping_factor", collision_params.damping_factor);
        collision_params.spring_factor =
            READ_IF_EXISTS(ini, r_float, section, "spring_factor", collision_params.spring_factor);
        collision_params.mu_factor =
            READ_IF_EXISTS(ini, r_float, section, "friction_factor", collision_params.mu_factor);
    }
    else if (ini->section_exist("wheels_params"))
    {
        collision_params.damping_factor = ini->r_float("wheels_params", "damping_factor");
        collision_params.spring_factor = ini->r_float("wheels_params", "spring_factor");
        collision_params.mu_factor = ini->r_float("wheels_params", "friction_factor");
    }
}
void CCar::SWheel::ApplyDriveAxisTorque(float torque)
{
    if (!joint)
        return;
    // dJointSetHinge2Param(joint->GetDJoint(), dParamFMax2,torque);//car->m_axle_friction
    joint->SetForce(torque, 1);
}
void CCar::SWheel::ApplyDriveAxisVel(float vel)
{
    if (!joint)
        return;
    // dJointSetHinge2Param(joint->GetDJoint(), dParamVel2, vel);
    joint->SetVelocity(vel, 1);
}

void CCar::SWheel::ApplyDriveAxisVelTorque(float vel, float torque)
{
    ApplyDriveAxisVel(vel);
    ApplyDriveAxisTorque(torque);
}
void CCar::SWheel::ApplySteerAxisVel(float vel)
{
    if (!joint)
        return;
    // dJointSetHinge2Param(joint->GetDJoint(), dParamVel, vel);
    joint->SetVelocity(vel, 0);
}

void CCar::SWheel::ApplySteerAxisTorque(float torque)
{
    if (!joint)
        return;
    // dJointSetHinge2Param(joint->GetDJoint(), dParamFMax, torque);
    joint->SetForce(torque, 0);
}

void CCar::SWheel::ApplySteerAxisVelTorque(float vel, float torque)
{
    ApplySteerAxisVel(vel);
    ApplySteerAxisTorque(torque);
}

void CCar::SWheel::SetSteerHiLimit(float hi)
{
    if (!joint)
        return;
    // dJointSetHinge2Param(joint->GetDJoint(), dParamHiStop, hi);
    joint->SetHiLimitDynamic(0, hi);
}
void CCar::SWheel::SetSteerLoLimit(float lo)
{
    if (!joint)
        return;
    // dJointSetHinge2Param(joint->GetDJoint(), dParamLoStop, lo);
    joint->SetLoLimitDynamic(0, lo);
}
void CCar::SWheel::SetSteerLimits(float hi, float lo)
{
    SetSteerHiLimit(hi);
    SetSteerLoLimit(lo);
}

void CCar::SWheel::ApplyDamage(u16 level)
{
    inherited::ApplyDamage(level);
    if (!joint)
        return;
    if (level == 0)
        return;
    float sf, df;
    // dJointID dj=joint->GetDJoint();
    switch (level)
    {
    case 1:
        joint->GetJointSDfactors(sf, df);
        sf /= 20.f;
        df *= 4.f;
        joint->SetJointSDfactors(sf, df);
        car->m_damage_particles.PlayWheel1(car, bone_id);
        break;
    case 2:

        // dVector3 v;
        Fvector v;

        // dJointGetHinge2Axis2(dj,v);
        joint->GetAxisDirDynamic(1, v);

        v[0] += 0.1f;
        v[1] += 0.1f;
        v[2] += 0.1f;
        VERIFY(v.magnitude() > EPS_S);
        // accurate_normalize(v);
        v.normalize();

        // dJointSetHinge2Axis2(dj,v[0],v[1],v[2]);
        joint->SetAxisDir(v, 1);
        joint->GetJointSDfactors(sf, df);
        sf /= 30.f;
        df *= 8.f;
        joint->SetJointSDfactors(sf, df);
        car->m_damage_particles.PlayWheel2(car, bone_id);
        break;
    default: NODEFAULT;
    }
}

void CCar::SWheel::SaveNetState(NET_Packet& P)
{
    CSE_ALifeCar::SWheelState ws;
    ws.health = Health();
    ws.write(P);
}

void CCar::SWheel::RestoreNetState(const CSE_ALifeCar::SWheelState& a_state)
{
    SetHealth(a_state.health);
    RestoreEffect();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCar::SWheelDrive::Init()
{
    pwheel->Init();
    gear_factor = pwheel->radius / pwheel->car->m_ref_radius;
    CBoneData& bone_data = smart_cast<IKinematics*>(pwheel->car->Visual())->LL_GetData(u16(pwheel->bone_id));
    switch (bone_data.IK_data.type)
    {
    case jtWheel: pos_fvd = bone_map.find(pwheel->bone_id)->second.element->mXFORM.k.x; break;

    default: NODEFAULT;
    }

    pos_fvd = pos_fvd > 0.f ? -1.f : 1.f;
}
void CCar::SWheelDrive::Drive()
{
    float cur_speed = pwheel->car->RefWheelMaxSpeed() / gear_factor;
    pwheel->ApplyDriveAxisVel(pos_fvd * cur_speed);
}
void CCar::SWheelDrive::UpdatePower() { pwheel->ApplyDriveAxisTorque(pwheel->car->RefWheelCurTorque() / gear_factor); }
void CCar::SWheelDrive::Neutral() { pwheel->ApplyDriveAxisVelTorque(0.f, pwheel->car->m_axle_friction); }
float CCar::SWheelDrive::ASpeed()
{
    CPhysicsJoint* J = pwheel->joint;
    if (!J)
        return 0.f;
    // return (dJointGetHinge2Angle2Rate(J->GetDJoint()))*pos_fvd;//dFabs
    return (J->GetAxisAngleRate(1)) * pos_fvd; // dFabs
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCar::SWheelSteer::Init()
{
    IKinematics* pKinematics = smart_cast<IKinematics*>(pwheel->car->Visual());
    pwheel->Init();
    (bone_map.find(pwheel->bone_id))->second.joint->GetLimits(lo_limit, hi_limit, 0);
    CBoneData& bone_data = pKinematics->LL_GetData(u16(pwheel->bone_id));
    switch (bone_data.IK_data.type)
    {
    case jtWheel:
        pos_right =
            bone_map.find(pwheel->bone_id)->second.element->mXFORM.i.y; //.dotproduct(pwheel->car->m_root_transform.j);
        break;

    default: NODEFAULT;
    }

    pos_right = pos_right > 0.f ? -1.f : 1.f;
    float steering_torque = pKinematics->LL_UserData()->r_float("car_definition", "steering_torque");
    VERIFY(pwheel);
    pwheel->ApplySteerAxisTorque(steering_torque);
    VERIFY(pwheel->joint);
    // dJointSetHinge2Param(pwheel->joint->GetDJoint(), dParamFudgeFactor, 0.005f/steering_torque);
    pwheel->joint->SetJointFudgefactorActive(0.005f / steering_torque);
    pwheel->ApplySteerAxisVel(0.f);
    limited = false;
}

void CCar::SWheelSteer::SteerRight()
{
    limited = true; // no need to limit wheels when steering
    if (pos_right > 0)
    {
        pwheel->SetSteerHiLimit(hi_limit);
        pwheel->ApplySteerAxisVel(pwheel->car->m_steering_speed);
    }
    else
    {
        pwheel->SetSteerLoLimit(lo_limit);
        pwheel->ApplySteerAxisVel(-pwheel->car->m_steering_speed);
    }
}
void CCar::SWheelSteer::SteerLeft()
{
    limited = true; // no need to limit wheels when steering
    if (pos_right < 0)
    {
        pwheel->SetSteerHiLimit(hi_limit);
        pwheel->ApplySteerAxisVel(pwheel->car->m_steering_speed);
    }
    else
    {
        pwheel->SetSteerLoLimit(lo_limit);
        pwheel->ApplySteerAxisVel(-pwheel->car->m_steering_speed);
    }
}
void CCar::SWheelSteer::SteerIdle()
{
    limited = false;
    if (pwheel->car->e_state_steer == right)
    {
        if (pos_right < 0)
        {
            pwheel->SetSteerHiLimit(0.f);
            pwheel->ApplySteerAxisVel(pwheel->car->m_steering_speed);
        }
        else
        {
            pwheel->SetSteerLoLimit(0.f);
            pwheel->ApplySteerAxisVel(-pwheel->car->m_steering_speed);
        }
    }
    else
    {
        if (pos_right > 0)
        {
            pwheel->SetSteerHiLimit(0.f);
            pwheel->ApplySteerAxisVel(pwheel->car->m_steering_speed);
        }
        else
        {
            pwheel->SetSteerLoLimit(0.f);
            pwheel->ApplySteerAxisVel(-pwheel->car->m_steering_speed);
        }
    }
}

void CCar::SWheelSteer::Limit()
{
    CPhysicsJoint* J = pwheel->joint;
    if (!J)
        return;
    // dJointID joint=J->GetDJoint();
    if (!limited)
    {
        // dReal angle = dJointGetHinge2Angle1(joint);
        float angle = J->GetAxisAngle(0);
        if (_abs(angle) < M_PI / 180.f)
        {
            pwheel->SetSteerLimits(0.f, 0.f);
            pwheel->ApplySteerAxisVel(0.f);
            limited = true;
        }
    }
    pwheel->car->b_wheels_limited = pwheel->car->b_wheels_limited && limited;
}

float CCar::SWheelSteer::GetSteerAngle()
{
    VERIFY(pwheel);
    VERIFY(pwheel->joint);
    return -pos_right * pwheel->joint->GetAxisAngle(0); // dJointGetHinge2Angle1 (pwheel->joint->GetDJoint());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCar::SWheelBreak::Init()
{
    pwheel->Init();
    float k = pwheel->radius / pwheel->car->m_ref_radius;

    break_torque *= k;
    hand_break_torque *= k;
}
void CCar::SWheelBreak::Load(LPCSTR section)
{
    IKinematics* K = PKinematics(pwheel->car->Visual());
    CInifile* ini = K->LL_UserData();
    VERIFY(ini);
    break_torque = ini->r_float("car_definition", "break_torque");
    hand_break_torque = READ_IF_EXISTS(ini, r_float, "car_definition", "hand_break_torque", break_torque);
    if (ini->section_exist(section))
    {
        break_torque = READ_IF_EXISTS(ini, r_float, section, "break_torque", break_torque);
        hand_break_torque = READ_IF_EXISTS(ini, r_float, section, "hand_break_torque", hand_break_torque);
    }
}
void CCar::SWheelBreak::Break(float k) { pwheel->ApplyDriveAxisVelTorque(0.f, 100000.f * break_torque * k); }
void CCar::SWheelBreak::HandBreak() { pwheel->ApplyDriveAxisVelTorque(0.f, 100000.f * hand_break_torque); }
void CCar::SWheelBreak::Neutral() { pwheel->ApplyDriveAxisVelTorque(0.f, pwheel->car->m_axle_friction); }
