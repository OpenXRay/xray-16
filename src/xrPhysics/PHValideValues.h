#pragma once

#include "xrPhysicsCore/IPhysicsCore.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_quaternion.h"
#include "xrCore/_matrix.h"

class CSafeValue
{
    float m_safe_value;

public:
    CSafeValue(float val = 0.f)
    {
        R_ASSERT(_valid(val));
        m_safe_value = val;
    }

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
    IC void new_val(Fvector& val)
    {
        m_safe_values[0].new_val(val.x);
        m_safe_values[1].new_val(val.y);
        m_safe_values[2].new_val(val.z);
    }
};

class CSafeVector4
{
    CSafeValue m_safe_values[4];

public:
    IC void new_val(Fquaternion& val)
    {
        m_safe_values[0].new_val(val.x);
        m_safe_values[1].new_val(val.y);
        m_safe_values[2].new_val(val.z);
        m_safe_values[3].new_val(val.w);
    }
};

class CSafeBodyLinearState
{
    CSafeVector3 m_safe_position;
    CSafeVector3 m_safe_linear_vel;

public:
    IC void create(CharacterVirtualHandle b)
    {
        R_ASSERT(b != INVALID_CHARACTER_VIRTUAL_HANDLE);
        new_state(b);
    }

    IC void new_state(CharacterVirtualHandle b)
    {
        Fmatrix transform;
        GetPhysicsCore()->GetBodyTransform(b, transform);

        Fvector pos = transform.c;
        m_safe_position.new_val(pos);

        // Если позиция была исправлена, обновляем её в движке
        if (pos.x != transform.c.x || pos.y != transform.c.y || pos.z != transform.c.z)
        {
            transform.c = pos;
            GetPhysicsCore()->SetBodyTransform(b, transform);
        }

        Fvector vel;
        GetPhysicsCore()->GetBodyLinearVelocity(b, vel);
        m_safe_linear_vel.new_val(vel);
        GetPhysicsCore()->SetBodyLinearVelocity(b, vel);
    }
};

class CSafeFixedRotationState
{
    Fmatrix rotation;
    CSafeBodyLinearState m_safe_linear_state;

public:
    CSafeFixedRotationState()
    {
        rotation.identity();
    }

    IC void create(CharacterVirtualHandle b)
    {
        R_ASSERT(b != INVALID_CHARACTER_VIRTUAL_HANDLE);
        GetPhysicsCore()->GetBodyTransform(b, rotation);
        rotation.c.set(0, 0, 0); // Оставляем только матрицу поворота
        new_state(b);
    }

    IC void set_rotation(const Fmatrix& r)
    {
        rotation = r;
        rotation.c.set(0, 0, 0);
    }

    IC void new_state(CharacterVirtualHandle b)
    {
        Fmatrix transform;
        GetPhysicsCore()->GetBodyTransform(b, transform);

        // Принудительно устанавливаем фиксированное вращение, сохраняя позицию
        Fvector pos = transform.c;
        transform = rotation;
        transform.c = pos;
        GetPhysicsCore()->SetBodyTransform(b, transform);

        GetPhysicsCore()->SetBodyAngularVelocity(b, Fvector().set(0.f, 0.f, 0.f));
        m_safe_linear_state.new_state(b);
    }
};

class CSafeBodyAngularState
{
    CSafeVector3 m_safe_angular_vel;
    CSafeVector4 m_safe_quaternion;

public:
    IC void create(CharacterVirtualHandle b)
    {
        R_ASSERT(b != INVALID_CHARACTER_VIRTUAL_HANDLE);
        new_state(b);
    }

    IC void new_state(CharacterVirtualHandle b)
    {
        Fvector ang_vel;
        GetPhysicsCore()->GetBodyAngularVelocity(b, ang_vel);
        m_safe_angular_vel.new_val(ang_vel);
        GetPhysicsCore()->SetBodyAngularVelocity(b, ang_vel);

        Fmatrix transform;
        GetPhysicsCore()->GetBodyTransform(b, transform);

        Fquaternion q;
        q.set(transform);
        m_safe_quaternion.new_val(q);

        Fmatrix new_rot;
        new_rot.rotation(q);
        new_rot.c = transform.c;
        GetPhysicsCore()->SetBodyTransform(b, new_rot);
    }
};

class CSafeBodyState
{
    CSafeBodyLinearState m_safe_linear_state;
    CSafeBodyAngularState m_safe_angular_state;

public:
    IC void create(CharacterVirtualHandle b)
    {
        R_ASSERT(b != INVALID_CHARACTER_VIRTUAL_HANDLE);
        new_state(b);
    }

    IC void new_state(CharacterVirtualHandle b)
    {
        m_safe_linear_state.new_state(b);
        m_safe_angular_state.new_state(b);
    }
};
