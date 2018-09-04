#include "stdafx.h"

#include "MovementBoxDynamicActivate.h"

#include "ExtendedGeom.h"
#include "MathUtils.h"
#include "Physics.h"
#include "ph_valid_ode.h"
#include "MathUtilsOde.h"
#include "IPhysicsShellHolder.h"
#include "PHCharacter.h"
#include "xrEngine/GameMtlLib.h"

ObjectContactCallbackFun* saved_callback = 0;
static float max_depth = 0.f;

struct STestCallbackPars
{
    static float calback_friction_factor;
    static float depth_to_use_force;
    static float callback_force_factor;
    static float depth_to_change_softness_pars;
    static float callback_cfm_factor;
    static float callback_erp_factor;
    static float decrement_depth;
    static float max_real_depth;
};

float STestCallbackPars::calback_friction_factor = 0.0f;
float STestCallbackPars::depth_to_use_force = 0.3f;
float STestCallbackPars::callback_force_factor = 10.f;
float STestCallbackPars::depth_to_change_softness_pars = 0.00f;
float STestCallbackPars::callback_cfm_factor = world_cfm * 0.00001f;
float STestCallbackPars::callback_erp_factor = 1.f;
float STestCallbackPars::decrement_depth = 0.f;
float STestCallbackPars::max_real_depth = 0.2f;
struct STestFootCallbackPars
{
    static float calback_friction_factor;
    static float depth_to_use_force;
    static float callback_force_factor;
    static float depth_to_change_softness_pars;
    static float callback_cfm_factor;
    static float callback_erp_factor;
    static float decrement_depth;
    static float max_real_depth;
};

float STestFootCallbackPars::calback_friction_factor = 0.3f;
float STestFootCallbackPars::depth_to_use_force = 0.3f;
float STestFootCallbackPars::callback_force_factor = 10.f;
float STestFootCallbackPars::depth_to_change_softness_pars = 0.00f;
float STestFootCallbackPars::callback_cfm_factor = world_cfm * 0.00001f;
float STestFootCallbackPars::callback_erp_factor = 1.f;
float STestFootCallbackPars::decrement_depth = 0.05f;
float STestFootCallbackPars::max_real_depth = 0.2f;
template <class Pars>
void TTestDepthCallback(bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    if (saved_callback)
        saved_callback(do_colide, bo1, c, material_1, material_2);

    if (do_colide && !material_1->Flags.test(SGameMtl::flPassable) && !material_2->Flags.test(SGameMtl::flPassable))
    {
        float& depth = c.geom.depth;
        float test_depth = depth - Pars::decrement_depth;
        save_max(max_depth, test_depth);
        c.surface.mu *= Pars::calback_friction_factor;
        if (test_depth > Pars::depth_to_use_force)
        {
            float force = Pars::callback_force_factor * ph_world->Gravity();
            dBodyID b1 = dGeomGetBody(c.geom.g1);
            dBodyID b2 = dGeomGetBody(c.geom.g2);
            if (b1)
                dBodyAddForce(b1, c.geom.normal[0] * force, c.geom.normal[1] * force, c.geom.normal[2] * force);
            if (b2)
                dBodyAddForce(b2, -c.geom.normal[0] * force, -c.geom.normal[1] * force, -c.geom.normal[2] * force);
            dxGeomUserData* ud1 = retrieveGeomUserData(c.geom.g1);
            dxGeomUserData* ud2 = retrieveGeomUserData(c.geom.g2);

            if (ud1)
            {
                CPhysicsShell* phsl = ud1->ph_ref_object->ObjectPPhysicsShell();
                if (phsl)
                    phsl->Enable();
            }

            if (ud2)
            {
                CPhysicsShell* phsl = ud2->ph_ref_object->ObjectPPhysicsShell();
                if (phsl)
                    phsl->Enable();
            }

            do_colide = false;
        }
        else if (test_depth > Pars::depth_to_change_softness_pars)
        {
            c.surface.soft_cfm = Pars::callback_cfm_factor;
            c.surface.soft_erp = Pars::callback_erp_factor;
        }
        limit_above(depth, Pars::max_real_depth);
    }
}

ObjectContactCallbackFun* TestDepthCallback = &TTestDepthCallback<STestCallbackPars>;
ObjectContactCallbackFun* TestFootDepthCallback = &TTestDepthCallback<STestFootCallbackPars>;
///////////////////////////////////////////////////////////////////////////////////////
class CVelocityLimiter : public CPHUpdateObject
{
    dBodyID m_body;

public:
    float l_limit;
    float y_limit;

private:
    dVector3 m_safe_velocity;
    dVector3 m_safe_position;

public:
    CVelocityLimiter(dBodyID b, float l, float yl)
    {
        R_ASSERT(b);
        m_body = b;
        dVectorSet(m_safe_velocity, dBodyGetLinearVel(m_body));
        dVectorSet(m_safe_position, dBodyGetPosition(m_body));
        l_limit = l;
        y_limit = yl;
    }
    virtual ~CVelocityLimiter()
    {
        Deactivate();
        m_body = 0;
    }

    bool VelocityLimit()
    {
        const float* linear_velocity = dBodyGetLinearVel(m_body);
        // limit velocity
        bool ret = false;
        if (dV_valid(linear_velocity))
        {
            dReal mag;
            Fvector vlinear_velocity;
            vlinear_velocity.set(cast_fv(linear_velocity));
            mag = _sqrt(linear_velocity[0] * linear_velocity[0] + linear_velocity[2] * linear_velocity[2]); //
            if (mag > l_limit)
            {
                dReal f = mag / l_limit;
                // dBodySetLinearVel(m_body,linear_velocity[0]/f,linear_velocity[1],linear_velocity[2]/f);///f
                vlinear_velocity.x /= f;
                vlinear_velocity.z /= f;
                ret = true;
            }
            mag = _abs(linear_velocity[1]);
            if (mag > y_limit)
            {
                vlinear_velocity.y = linear_velocity[1] / mag * y_limit;
                ret = true;
            }
            dBodySetLinearVel(m_body, vlinear_velocity.x, vlinear_velocity.y, vlinear_velocity.z);
            return ret;
        }
        else
        {
            dBodySetLinearVel(m_body, m_safe_velocity[0], m_safe_velocity[1], m_safe_velocity[2]);
            return true;
        }
    }
    virtual void PhDataUpdate(dReal step)
    {
        const float* linear_velocity = dBodyGetLinearVel(m_body);

        if (VelocityLimit())
        {
            dBodySetPosition(m_body, m_safe_position[0] + linear_velocity[0] * fixed_step,
                m_safe_position[1] + linear_velocity[1] * fixed_step,
                m_safe_position[2] + linear_velocity[2] * fixed_step);
        }

        if (!dV_valid(dBodyGetPosition(m_body)))
            dBodySetPosition(m_body, m_safe_position[0] - m_safe_velocity[0] * fixed_step,
                m_safe_position[1] - m_safe_velocity[1] * fixed_step,
                m_safe_position[2] - m_safe_velocity[2] * fixed_step);

        dVectorSet(m_safe_position, dBodyGetPosition(m_body));
        dVectorSet(m_safe_velocity, linear_velocity);
    }

    virtual void PhTune(dReal step) { VelocityLimit(); }
};
////////////////////////////////////////////////////////////////////////////////////
class CGetContactForces : public CPHUpdateObject
{
    dBodyID m_body;
    float m_max_force_self;
    float m_max_torque_self;

    float m_max_force_self_y;
    float m_max_force_self_sd;

    float m_max_force_others;
    float m_max_torque_others;

public:
    CGetContactForces(dBodyID b)
    {
        R_ASSERT(b);
        m_body = b;
        InitValues();
    }
    float mf_slf() { return m_max_force_self; }
    float mf_othrs() { return m_max_force_others; }
    float mt_slf() { return m_max_torque_self; }
    float mt_othrs() { return m_max_torque_others; }
    float mf_slf_y() { return m_max_force_self_y; }
    float mf_slf_sd() { return m_max_force_self_sd; }
protected:
    virtual void PhTune(dReal step)
    {
        InitValues();
        int num = dBodyGetNumJoints(m_body);
        for (int i = 0; i < num; ++i)
        {
            dJointID joint = dBodyGetJoint(m_body, i);

            if (dJointGetType(joint) == dJointTypeContact)
            {
                dJointSetFeedback(joint, ContactFeedBacks.add());
            }
        }
    }

    virtual void PhDataUpdate(dReal step)
    {
        int num = dBodyGetNumJoints(m_body);
        for (int i = 0; i < num; i++)
        {
            dJointID joint = dBodyGetJoint(m_body, i);
            if (dJointGetType(joint) == dJointTypeContact)
            {
                dJointFeedback* feedback = dJointGetFeedback(joint);
                R_ASSERT2(feedback, "Feedback was not set!!!");
                dxJoint* b_joint = (dxJoint*)joint;
                dBodyID other_body = b_joint->node[1].body;
                bool b_body_second = (b_joint->node[1].body == m_body);
                dReal* self_force = feedback->f1;
                dReal* self_torque = feedback->t1;
                dReal* othrers_force = feedback->f2;
                dReal* othrers_torque = feedback->t2;
                if (b_body_second)
                {
                    other_body = b_joint->node[0].body;
                    self_force = feedback->f2;
                    self_torque = feedback->t2;
                    othrers_force = feedback->f1;
                    othrers_torque = feedback->t1;
                }

                save_max(m_max_force_self, _sqrt(dDOT(self_force, self_force)));
                save_max(m_max_torque_self, _sqrt(dDOT(self_torque, self_torque)));
                save_max(m_max_force_self_y, _abs(self_force[1]));
                save_max(m_max_force_self_sd, _sqrt(self_force[0] * self_force[0] + self_force[2] * self_force[2]));
                if (other_body)
                {
                    dVector3 shoulder;
                    dVectorSub(shoulder, dJointGetPositionContact(joint), dBodyGetPosition(other_body));
                    dReal shoulder_lenght = _sqrt(dDOT(shoulder, shoulder));

                    save_max(m_max_force_others, _sqrt(dDOT(othrers_force, othrers_force)));
                    if (!fis_zero(shoulder_lenght))
                        save_max(m_max_torque_others, _sqrt(dDOT(othrers_torque, othrers_torque)) / shoulder_lenght);
                }
            }
        }
    }

private:
    void InitValues()
    {
        m_max_force_self = 0.f;
        m_max_torque_self = 0.f;
        m_max_force_others = 0.f;
        m_max_torque_others = 0.f;
        m_max_force_self_y = 0.f;
        m_max_force_self_sd = 0.f;
    }
};
/////////////////////////////////////////////////////////////////////////////////////
bool ActivateBoxDynamic(IPHMovementControl* mov_control, bool character_exist, DWORD id, int num_it /*=8*/,
    int num_steps /*5*/, float resolve_depth /*=0.01f*/)
{
    /////////////////////////////////////////////////////////////////////////////
    // m_PhysicMovementControl->ActivateBox(id);
    VERIFY(mov_control);
    VERIFY(mov_control->character());

    mov_control->character()->CPHObject::activate();
    ph_world->Freeze();
    // UnFreeze(); //if(m_character) m_character->UnFreeze();
    mov_control->character()->UnFreeze();

    // saved_callback=ObjectContactCallback();

    /*	ObjectContactCallbackFun* CPHMovementControl::ObjectContactCallback()
    {
        if(m_character)
            return m_character->ObjectContactCallBack();
        else return NULL;
    }*/

    saved_callback = mov_control->character()->ObjectContactCallBack();

    //	SetOjectContactCallback(TestDepthCallback);

    //	void		CPHMovementControl::		SetOjectContactCallback (ObjectContactCallbackFun* callback)
    //{
    //	if(m_character)
    //		m_character->SetObjectContactCallback(callback);
    //}
    mov_control->character()->SetObjectContactCallback(TestDepthCallback);

    // SetFootCallBack(TestFootDepthCallback);
    // void		CPHMovementControl::		SetFootCallBack			(ObjectContactCallbackFun* callback)
    //{
    //	VERIFY(m_character);
    //	m_character->SetWheelContactCallback(callback);
    //}
    mov_control->character()->SetWheelContactCallback(TestFootDepthCallback);

    max_depth = 0.f;

    //////////////////////////////////pars///////////////////////////////////////////
    //	int		num_it=8;
    //	int		num_steps=5;
    //	float	resolve_depth=0.01f;

    if (!character_exist)
    {
        num_it = 20;
        num_steps = 1;
        resolve_depth = 0.1f;
    }
    ///////////////////////////////////////////////////////////////////////
    float fnum_it = float(num_it);
    float fnum_steps = float(num_steps);
    float fnum_steps_r = 1.f / fnum_steps;

    // const Fbox& box =Box();
    float pass = character_exist ? _abs(mov_control->Box().getradius() - mov_control->Boxes()[id].getradius()) :
                                   mov_control->Boxes()[id].getradius();
    float max_vel = pass / 2.f / fnum_it / fnum_steps / fixed_step;
    float max_a_vel = M_PI / 8.f / fnum_it / fnum_steps / fixed_step;
    VERIFY(mov_control->character());
    dBodySetForce(mov_control->character()->get_body(), 0.f, 0.f, 0.f);
    dBodySetLinearVel(mov_control->character()->get_body(), 0.f, 0.f, 0.f);

    // Calculate(Fvector().set(0,0,0),Fvector().set(1,0,0),0,0,0,0);
    mov_control->actor_calculate(Fvector().set(0, 0, 0), Fvector().set(1, 0, 0), 0, 0, 0, 0);

    CVelocityLimiter vl(mov_control->character()->get_body(), max_vel, max_vel);
    max_vel = 1.f / fnum_it / fnum_steps / fixed_step;

    bool ret = false;
    mov_control->character()->SwitchOFFInitContact();
    mov_control->character()->SetStaticContactCallBack(0);
    vl.Activate();
    vl.l_limit *= (fnum_it * fnum_steps / 5.f);
    vl.y_limit = vl.l_limit;
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int m = 0; 30 > m; ++m)
    {
        // Calculate(Fvector().set(0,0,0),Fvector().set(1,0,0),0,0,0,0);
        mov_control->actor_calculate(Fvector().set(0, 0, 0), Fvector().set(1, 0, 0), 0, 0, 0, 0);

        // EnableCharacter();
        // void		CPHMovementControl::EnableCharacter			()
        //{
        //	if( m_character && m_character->b_exist )
        //		m_character->Enable();
        //}
        VERIFY(mov_control->character()->b_exist);
        mov_control->character()->Enable();

        mov_control->character()->ApplyForce(0, ph_world->Gravity() * mov_control->character()->Mass(), 0);
        max_depth = 0.f;
        ph_world->Step();
        if (max_depth < resolve_depth)
        {
            break;
        }
        ph_world->CutVelocity(max_vel, max_a_vel);
    }
    vl.l_limit /= (fnum_it * fnum_steps / 5.f);
    vl.y_limit = vl.l_limit;
    /////////////////////////////////////

    for (int m = 0; num_steps > m; ++m)
    {
        float param = fnum_steps_r * (1 + m);
        mov_control->InterpolateBox(id, param);
        ret = false;
        for (int i = 0; num_it > i; ++i)
        {
            max_depth = 0.f;
            // Calculate(Fvector().set(0,0,0),Fvector().set(1,0,0),0,0,0,0);
            mov_control->actor_calculate(Fvector().set(0, 0, 0), Fvector().set(1, 0, 0), 0, 0, 0, 0);
            // EnableCharacter();
            mov_control->character()->Enable();
            mov_control->character()->ApplyForce(0, ph_world->Gravity() * mov_control->character()->Mass(), 0);
            ph_world->Step();
            ph_world->CutVelocity(max_vel, max_a_vel);
            if (max_depth < resolve_depth)
            {
                ret = true;
                break;
            }
        }
        if (!ret)
            break;
    }

    mov_control->character()->SwitchInInitContact();
    mov_control->character()->SetStaticContactCallBack(ph_world->default_character_contact_shotmark());
    vl.Deactivate();

    ph_world->UnFreeze();

    // SetOjectContactCallback(saved_callback);
    // void		CPHMovementControl::		SetOjectContactCallback (ObjectContactCallbackFun* callback)
    //{
    //	if(m_character)
    //		m_character->SetObjectContactCallback(callback);
    //}
    mov_control->character()->SetObjectContactCallback(saved_callback);
    saved_callback = 0;

    return ret;
    //////////////////////////////////////////////////////////////////////////////////////////
}
