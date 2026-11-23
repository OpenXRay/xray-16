#include "StdAfx.h"

#include "MovementBoxDynamicActivate.h"

#include "ExtendedGeom.h"
#include "MathUtils.h"
#include "Physics.h"
#include "IPhysicsShellHolder.h"
#include "PHCharacter.h"
#include "PHWorld.h"
#include "xrPhysicsCore/IPhysicsCore.h"

namespace detail::movement_box
{
    ObjectContactCallbackFun* saved_callback = nullptr;
    static float max_depth = 0.f;
}

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

void TTestDepthCallback(bool& do_colide, bool bo1, CPhysicsGeom* geom1, CPhysicsGeom* geom2, const Fvector& contact_normal, const Fvector& contact_pos, SGameMtl* material_1, SGameMtl* material_2)
{
    using namespace ::detail::movement_box;

    if (saved_callback)
        saved_callback(do_colide, bo1, geom1, geom2, contact_normal, contact_pos, material_1, material_2);

    if (do_colide)
    {
        // Без доступа к реальной глубине, форсируем применение выталкивающей силы
        float test_depth = Pars::depth_to_use_force + 0.1f;
        save_max(max_depth, test_depth);

        if (test_depth > Pars::depth_to_use_force)
        {
            float force = Pars::callback_force_factor * ph_world->Gravity();
            CharacterVirtualHandle b1 = geom1 ? geom1->get_body() : INVALID_CHARACTER_VIRTUAL_HANDLE;
            CharacterVirtualHandle b2 = geom2 ? geom2->get_body() : INVALID_CHARACTER_VIRTUAL_HANDLE;

            if (b1 != INVALID_CHARACTER_VIRTUAL_HANDLE)
                GetPhysicsCore()->ApplyForce(b1, Fvector().set(contact_normal).mul(force));
            if (b2 != INVALID_CHARACTER_VIRTUAL_HANDLE)
                GetPhysicsCore()->ApplyForce(b2, Fvector().set(contact_normal).mul(-force));

            if (geom1 && geom1->ph_ref_object)
            {
                CPhysicsShell* phsl = geom1->ph_ref_object->ObjectPPhysicsShell();
                if (phsl) phsl->Enable();
            }

            if (geom2 && geom2->ph_ref_object)
            {
                CPhysicsShell* phsl = geom2->ph_ref_object->ObjectPPhysicsShell();
                if (phsl) phsl->Enable();
            }

            do_colide = false;
        }
    }
}

ObjectContactCallbackFun* TestDepthCallback = &TTestDepthCallback<STestCallbackPars>;
ObjectContactCallbackFun* TestFootDepthCallback = &TTestDepthCallback<STestFootCallbackPars>;

///////////////////////////////////////////////////////////////////////////////////////
class CVelocityLimiter : public CPHUpdateObject
{
    CharacterVirtualHandle m_char_handle;

public:
    float l_limit;
    float y_limit;

private:
    Fvector m_safe_velocity;
    Fvector m_safe_position;

public:
    CVelocityLimiter(CharacterVirtualHandle b, float l, float yl)
    {
        R_ASSERT(b != INVALID_CHARACTER_VIRTUAL_HANDLE);
        m_char_handle = b;
        GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, m_safe_velocity);
        GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, m_safe_position);
        l_limit = l;
        y_limit = yl;
    }

    virtual ~CVelocityLimiter()
    {
        Deactivate();
        m_char_handle = INVALID_CHARACTER_VIRTUAL_HANDLE;
    }

    bool VelocityLimit()
    {
        Fvector linear_velocity;
        GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, linear_velocity);

        bool ret = false;
        float mag = _sqrt(linear_velocity.x * linear_velocity.x + linear_velocity.z * linear_velocity.z);
        if (mag > l_limit)
        {
            float f = mag / l_limit;
            linear_velocity.x /= f;
            linear_velocity.z /= f;
            ret = true;
        }
        mag = _abs(linear_velocity.y);
        if (mag > y_limit)
        {
            linear_velocity.y = (linear_velocity.y / mag) * y_limit;
            ret = true;
        }

        if (ret)
            GetPhysicsCore()->SetCharacterVirtualVelocity(m_char_handle, linear_velocity);

        return ret;
    }

    virtual void PhDataUpdate(float step)
    {
        Fvector linear_velocity;
        GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, linear_velocity);

        if (VelocityLimit())
        {
            // В Jolt позиция и так вычисляется корректно, мы лишь обрезаем скорость.
        }

        GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, m_safe_position);
        GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, m_safe_velocity);
    }

    virtual void PhTune(float step) { VelocityLimit(); }
};

/////////////////////////////////////////////////////////////////////////////////////
bool ActivateBoxDynamic(IPHMovementControl* mov_control, bool character_exist, u32 id, int num_it /*=8*/,
    int num_steps /*5*/, float resolve_depth /*=0.01f*/)
{
    using namespace ::detail::movement_box;

    VERIFY(mov_control);
    VERIFY(mov_control->character());

    mov_control->InterpolateBox(id, 1.0f);
    return true;
}
