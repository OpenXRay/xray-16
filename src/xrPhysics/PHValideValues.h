#ifndef PH_VALIDE_VALUES
#define PH_VALIDE_VALUES

#include "MathUtilsOde.h"
#include "ph_valid_ode.h"
class CSafeValue
{
    float m_safe_value;

public:
    CSafeValue(float val)
    {
        R_ASSERT(_valid(val));
        m_safe_value = val;
    }
    CSafeValue() { m_safe_value = 0.f; }
    IC void new_val(float& val)
    {
        if (_valid(val))
            m_safe_value = val;
        else
            val = m_safe_value;
    }
};

class CSafeVector3
{
    CSafeValue m_safe_values[3];

public:
    IC void new_val(float* val)
    {
        m_safe_values[0].new_val(val[0]);
        m_safe_values[1].new_val(val[1]);
        m_safe_values[2].new_val(val[2]);
    }
};

class CSafeVector4
{
    CSafeValue m_safe_values[4];

public:
    IC void new_val(float* val)
    {
        m_safe_values[0].new_val(val[0]);
        m_safe_values[1].new_val(val[1]);
        m_safe_values[2].new_val(val[2]);
        m_safe_values[3].new_val(val[3]);
    }
};

class CSafeBodyLinearState
{
    CSafeVector3 m_safe_position;
    CSafeVector3 m_safe_linear_vel;

public:
    IC void create(dBodyID b)
    {
        R_ASSERT(dBodyStateValide(b));
        new_state(b);
    }
    IC void new_state(dBodyID b)
    {
        dVector3 v;
        dVectorSet(v, dBodyGetPosition(b));
        m_safe_position.new_val(v);
        dBodySetPosition(b, v[0], v[1], v[2]);

        dVectorSet(v, dBodyGetLinearVel(b));
        m_safe_linear_vel.new_val(v);
        dBodySetLinearVel(b, v[0], v[1], v[2]);
    }
};

class CSafeFixedRotationState
{
    dMatrix3 rotation;
    CSafeBodyLinearState m_safe_linear_state;

public:
    CSafeFixedRotationState() { dRSetIdentity(rotation); }
    IC void create(dBodyID b)
    {
        R_ASSERT(dBodyStateValide(b));
        std::copy_n(dBodyGetRotation(b), sizeof(rotation) / sizeof(dReal), rotation);
        new_state(b);
    }
    IC void set_rotation(const dMatrix3 r)
    {
        VERIFY(sizeof(rotation) == sizeof(dMatrix3));
        std::copy_n(r, sizeof(rotation) / sizeof(dReal), rotation);
    }
    IC void new_state(dBodyID b)
    {
        dBodySetRotation(b, rotation);
        dBodySetAngularVel(b, 0.f, 0.f, 0.f);
        m_safe_linear_state.new_state(b);
    }
};
class CSafeBodyAngularState
{
    CSafeVector3 m_safe_angular_vel;
    CSafeVector4 m_safe_quaternion;

public:
    IC void create(dBodyID b)
    {
        R_ASSERT(dBodyStateValide(b));
        new_state(b);
    }
    IC void new_state(dBodyID b)
    {
        dVector3 v;

        dVectorSet(v, dBodyGetAngularVel(b));
        m_safe_angular_vel.new_val(v);
        dBodySetAngularVel(b, v[0], v[1], v[2]);

        dQuaternion q;
        dQuaternionSet(q, dBodyGetQuaternion(b));
        m_safe_quaternion.new_val(q);
        dBodySetQuaternion(b, q);
    }
};

class CSafeBodyState
{
    CSafeBodyLinearState m_safe_linear_state;
    CSafeBodyAngularState m_safe_angular_state;

public:
    IC void create(dBodyID b)
    {
        R_ASSERT(dBodyStateValide(b));
        new_state(b);
    }
    IC void new_state(dBodyID b)
    {
        m_safe_linear_state.new_state(b);
        m_safe_angular_state.new_state(b);
    }
};
#endif
