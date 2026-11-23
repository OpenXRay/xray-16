#include "StdAfx.h"

#include "PHDynamicData.h"
#include "ExtendedGeom.h"
#include "xrCDB/Intersect.hpp"
#include "xrEngine/xr_object_list.h"
#include "PHSimpleCharacter.h"
#include "PHContactBodyEffector.h"

#include "SpaceUtils.h"

#include "params.h"
#include "MathUtils.h"

#include "IPhysicsShellHolder.h"
#include "Include/xrRender/Kinematics.h"
#include "PHSimpleCharacterInline.h"
#include "DamageSource.h"
#include "PHCollideValidator.h"

#include "Geometry.h"
#include "PHElement.h"

#include "xrCore/Animation/Bone.hpp"
#include "xrEngine/xr_object.h"
#include "xrEngine/IGame_Level.h"

const float LOSE_CONTROL_DISTANCE = 0.5f; // fly distance to lose control
const float CLAMB_DISTANCE = 0.5f;

float IC sgn(float v) { return v < 0.f ? -1.f : 1.f; }

CPHSimpleCharacter::CPHSimpleCharacter()
    : m_last_environment_update(Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX)), m_last_picked_material(GAMEMTL_NONE_IDX)
{
    m_object_contact_callback = nullptr;

    m_geom_shell = nullptr;
    m_wheel = nullptr;
    m_hat = nullptr;
    m_cap = nullptr;
    m_acceleration.set(0, 0, 0);
    b_external_impulse = false;
    m_ext_imulse.set(0, 0, 0);
    m_is_active = false;
    m_ext_impuls_stop_step = u64(-1);
    m_phys_ref_object = nullptr;
    b_on_object = false;
    b_was_on_object = false;
    m_friction_factor = 1.f;
    m_control_force.set(0, 0, 0);
    m_depart_position.set(0, 0, 0);
    is_contact = false;
    was_contact = false;
    is_control = false;
    was_control = false;
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
    b_exist = false;
    m_mass = 70.f;
    m_max_velocity = 5.f;
    b_meet_control = false;
    b_jumping = false;
    b_death_pos = false;
    jump_up_velocity = 6.f;
    m_air_control_factor = 0;
    m_safe_velocity.set(0,0,0);
    m_collision_damage_factor = 1.f;
    b_on_ground = false;
    b_lose_ground = true;
    b_collision_restrictor_touch = false;
    m_air_frames = 0;
    b_foot_mtl_check = true;
    b_non_interactive = false;
}

void CPHSimpleCharacter::TestPathCallback(bool& do_colide, bool bo1, CPhysicsGeom* geom1, CPhysicsGeom* geom2, const Fvector& contact_normal, const Fvector& contact_pos, SGameMtl* material_1, SGameMtl* material_2)
{
    do_colide = false;
    CPHSimpleCharacter* ch = nullptr;
    if (bo1 && geom1)
        ch = static_cast<CPHSimpleCharacter*>(geom1->ph_object);
    else if (!bo1 && geom2)
        ch = static_cast<CPHSimpleCharacter*>(geom2->ph_object);

    if (ch)
        ch->b_side_contact = true;
}

void CPHSimpleCharacter::SetBox(const Fvector& sizes)
{
    m_radius = std::min(sizes.x, sizes.z) / 2.f;
    m_cyl_hight = sizes.y - 2.f * m_radius;
    if (m_cyl_hight < 0.f)
        m_cyl_hight = 0.01f;

    // В Jolt размеры и геометрии обновляются через ядро
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        auto* core = GetPhysicsCore();
        const auto capsule = core->CreateCapsuleShape(m_radius, m_cyl_hight / 2.f);
        const auto translated = core->CreateRotatedTranslatedShape(capsule,
            Fvector().set(0.f, m_cyl_hight / 2.f, 0.f), Fquaternion().identity());
        core->SetCharacterVirtualShape(m_char_handle, translated);
        core->DestroyCDBModel(translated);
        core->DestroyCDBModel(capsule);
    }
}

void CPHSimpleCharacter::get_Box(Fvector& sz, Fvector& c) const
{
    sz.set(2 * m_radius, 2 * m_radius + m_cyl_hight, 2 * m_radius);
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
        GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, c);
    else
        c.set(0, 0, 0);
}

void CPHSimpleCharacter::Create(Fvector sizes)
{
    if (b_exist)
        return;

    b_air_contact_state = false;
    lastMaterialIDX = GAMEMTL_NONE_IDX;
    injuriousMaterialIDX = GAMEMTL_NONE_IDX;
    m_creation_step = ph_world->m_steps_num;

    m_radius = std::min(sizes.x, sizes.z) / 2.f;
    m_current_object_radius = m_radius;
    m_cyl_hight = sizes.y - 2.f * m_radius;
    if (m_cyl_hight < 0.f)
        m_cyl_hight = 0.01f;

    b_exist = true;

    PhysicsShapeHandle shape = GetPhysicsCore()->CreateCapsuleShape(m_radius, m_cyl_hight / 2.f);
    PhysicsShapeHandle translated = GetPhysicsCore()->CreateRotatedTranslatedShape(shape, Fvector().set(0.f, m_cyl_hight / 2.f, 0.f), Fquaternion().identity());
    GetPhysicsCore()->DestroyCDBModel(shape);
    shape = translated;
    m_char_handle = GetPhysicsCore()->CreateCharacterVirtual(shape, Fvector().set(0.f, 0.f, 0.f), m_mass);
    GetPhysicsCore()->DestroyCDBModel(shape);

    m_char_handle_interpolation.SetCharacter(m_char_handle);

    GetPhysicsCore()->SetCharacterVirtualUserData(m_char_handle, this);
    if (m_phys_ref_object)
    {
        SetPhysicsRefObject(m_phys_ref_object);
    }

    GetPhysicsCore()->SetCharacterVirtualContactCallback(m_char_handle, &CPHSimpleCharacter::JoltCharacterContactCallback, this);

    if (m_object_contact_callback)
    {
        SetObjectContactCallback(m_object_contact_callback);
    }

    VERIFY(ph_world);
    SetStaticContactCallBack(ph_world->default_character_contact_shotmark());
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
    b_lose_control = true;
    b_any_contacts = false;
    is_contact = false;
    b_foot_mtl_check = true;
    b_on_ground = b_valide_ground_contact = false;
}

void CPHSimpleCharacter::SwitchInInitContact()
{
    VERIFY(b_exist);
}

void CPHSimpleCharacter::Destroy()
{
    if (!b_exist)
        return;
    b_exist = false;
    R_ASSERT2(!ph_world->Processing(), "can not deactivate physics character shell during physics processing!!!");
    R_ASSERT2(!ph_world->IsFreezed(), "can not deactivate physics character when ph world is freezed!!!");
    R_ASSERT2(!CPHObject::IsFreezed(), "can not deactivate freezed !!!");
    m_elevator_state.Deactivate();

    spatial_unregister();
    CPHObject::deactivate();

    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        GetPhysicsCore()->DestroyCharacterVirtual(m_char_handle);
        m_char_handle = INVALID_CHARACTER_VIRTUAL_HANDLE;
    }
}

const static u64 impulse_time_constant = 30;

void CPHSimpleCharacter::ApplyImpulse(const Fvector& dir, const float P)
{
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        Fvector v;
        GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, v);

        // Standard realistic velocity delta from impulse (P = m * delta_v)
        float effective_mass = m_mass > 1.0f ? m_mass : 80.0f;
        float speed_delta = std::clamp(P / effective_mass, 0.0f, 25.0f);

        Fvector impulse_dir = dir;
        impulse_dir.normalize_safe();

        v.x += impulse_dir.x * speed_delta;
        v.y += impulse_dir.y * speed_delta;
        v.z += impulse_dir.z * speed_delta;

        // Sensible vertical speed limit (-30 to +30 m/s)
        v.y = std::clamp(v.y, -30.0f, 30.0f);

        GetPhysicsCore()->SetCharacterVirtualVelocity(m_char_handle, v);

        if (v.y > 0.5f)
        {
            b_jumping = true;
            b_on_ground = false;
            m_air_frames = 10;
        }
    }
}

void CPHSimpleCharacter::ApplyForce(const Fvector& force)
{
    m_force.add(force);
}

void CPHSimpleCharacter::ApplyForce(float x, float y, float z)
{
    ApplyForce(Fvector().set(x, y, z));
}

void CPHSimpleCharacter::ApplyForce(const Fvector& dir, float force)
{
    Fvector f; f.set(dir); f.mul(force); ApplyForce(f);
}

void CPHSimpleCharacter::PhDataUpdate(float step)
{
    SafeAndLimitVelocity(); // Enable velocity limiting for Jolt, but only clamp horizontal velocity
    if (!IsEnabled()) return;

    Fvector vel_before;
    GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, vel_before);
    vel_before.mad(m_force, step / m_mass);
    GetPhysicsCore()->SetCharacterVirtualVelocity(m_char_handle, vel_before);
    m_force.set(0, 0, 0);
    was_contact = is_contact;
    was_control = is_control;
    b_was_side_contact = b_side_contact;
    b_was_on_object = b_on_object;
    is_contact = b_side_contact = b_any_contacts = false;
    b_valide_ground_contact = b_valide_wall_contact = false;
    b_on_object = b_death_pos = b_collision_restrictor_touch = false;
    b_foot_mtl_check = true;
    m_contact_count = 0;

    GetPhysicsCore()->UpdateCharacterVirtual(m_char_handle, step, Fvector().set(0.f, -ph_world->Gravity(), 0.f));

    Fvector vel_after;
    GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, vel_after);

    Fvector pos;
    GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, pos);

    m_char_handle_interpolation.UpdatePositions();
}

void CPHSimpleCharacter::PhTune(float step)
{
    Fvector current_pos;
    GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, current_pos);
    m_last_move.set(current_pos);
    m_elevator_state.PhTune(step);

    if (!IsEnabled()) return;

    IPhysicsCore::SJoltCharacterGroundState ground_state;
    GetPhysicsCore()->GetCharacterVirtualGroundState(m_char_handle, ground_state);

    Fvector current_vel;
    GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, current_vel);

    if (ground_state.on_ground) {
        if ((b_jumping || current_vel.y > 0.5f) && current_vel.y > 0.1f) {
            // We are "on the ground" but moving UP fast (just jumped or launched by impulse).
            // Jolt's ground state is stale because it hasn't updated yet.
            m_air_frames++;
        } else {
            // Normal ground contact or we landed.
            if (m_air_frames > 0) {
                b_meet_control = true;
                is_contact = true;
            }
            b_jumping = false;
            m_air_frames = 0;
        }
    } else {
        m_air_frames++;
    }

    // 4 frames of "coyote time" to hide micro-bounces from the game logic
    b_on_ground = (m_air_frames < 4);

    bool stick_to_floor = !(b_jump || b_jumping) && (current_vel.y <= 0.1f);
    GetPhysicsCore()->SetCharacterVirtualStickToFloor(m_char_handle, stick_to_floor);

    if (b_on_ground) {
        b_valide_ground_contact = true;
        m_ground_contact_normal = ground_state.ground_normal;
        b_lose_control = false;
        b_lose_ground = false;
        m_ground_contact_position = current_pos;

        if (ground_state.ground_triangle_user_data != u32(-1))
        {
            CDB::TRI* tri = inl_ph_world().ObjectSpace().GetStaticTris() + ground_state.ground_triangle_user_data;
            if (tri && p_lastMaterialIDX)
            {
                *p_lastMaterialIDX = tri->material;
            }
        }

        if (m_air_frames > 0 && p_lastMaterialIDX && *p_lastMaterialIDX != GAMEMTL_NONE_IDX)
        {
            SGameMtl* tri_material = GMLib.GetMaterialByIdx(*p_lastMaterialIDX);
            if (tri_material)
            {
                UpdateStaticDamage(m_ground_contact_normal, current_pos, tri_material, true);
            }
        }
    } else {
        b_lose_control = true;
        b_lose_ground = true;
        b_valide_ground_contact = false;
        m_ground_contact_normal.set(0, 1, 0);
    }

    if (m_acceleration.magnitude() > 0.1f) is_control = true;
    else is_control = false;

    Fvector velocity;
    GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, velocity);

    if (b_jump)
    {
        velocity.set(m_jump_accel);
        b_jump = false;
        b_jumping = true;
    }

    if (b_on_ground || m_elevator_state.ClimbingState())
    {
        if (is_control)
        {
            if (m_elevator_state.ClimbingState())
            {
                velocity = m_acceleration;
            }
            else
            {
                // When jumping on this frame, we shouldn't overwrite the jump momentum immediately!
                // But wait, if we are on the ground, and we JUST jumped, we want to keep the jump momentum.
                if (!b_jumping)
                {
                    velocity.x = m_acceleration.x;
                    velocity.z = m_acceleration.z;
                }
            }
        }
        else
        {
            if (m_elevator_state.ClimbingState())
            {
                if (!b_jumping) velocity.set(0,0,0);
            }
            else
            {
                if (!b_jumping)
                {
                    // Smoothly damp velocity using ground friction instead of abrupt hard-zeroing
                    float damp = expf(-18.0f * step);
                    velocity.x *= damp;
                    velocity.z *= damp;
                    if (_abs(velocity.x) < 0.05f) velocity.x = 0;
                    if (_abs(velocity.z) < 0.05f) velocity.z = 0;
                }
            }
        }
    }
    else
    {
        if (is_control)
        {
            // Steer direction but preserve jump momentum
            float horiz_mag = _sqrt(velocity.x * velocity.x + velocity.z * velocity.z);

            velocity.x += (m_acceleration.x - velocity.x) * 0.05f;
            velocity.z += (m_acceleration.z - velocity.z) * 0.05f;

            float new_horiz_mag = _sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
            if (new_horiz_mag > 0.001f)
            {
                velocity.x *= horiz_mag / new_horiz_mag;
                velocity.z *= horiz_mag / new_horiz_mag;
            }
        }
    }

    if (b_on_ground && velocity.y < 0.0f)
    {
        velocity.y = ground_state.ground_velocity.y; // reset downward velocity accumulation
    }

    GetPhysicsCore()->SetCharacterVirtualVelocity(m_char_handle, velocity);
}

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
    if (b_clamb_jump)
    {
        Fvector current_pos;
        GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, current_pos);
        Fvector dif;
        dif.sub(current_pos, m_clamb_depart_position);
        if (_abs(dif.y) > CLAMB_DISTANCE)
        {
            b_clamb_jump = false;
        }
    }

    if (!m_elevator_state.Active() && b_valide_wall_contact && (m_contact_count > 1) &&
        (m_wall_contact_normal.y < M_SQRT1_2) && !b_side_contact)
    {
        if (((m_wall_contact_position.x - m_ground_contact_position.x) * m_control_force.x +
                (m_wall_contact_position.z - m_ground_contact_position.z) * m_control_force.z) > 0.05f &&
            m_wall_contact_position.y - m_ground_contact_position.y > 0.01f)
            b_clamb_jump = true;
    }

    if (b_valide_wall_contact && (m_contact_count > 1) && b_clamb_jump)
        if (_abs((m_wall_contact_position.x - m_ground_contact_position.x) +
                (m_wall_contact_position.z - m_ground_contact_position.z)) > 0.05f &&
            m_wall_contact_position.y - m_ground_contact_position.y > 0.01f)
        {
            GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, m_clamb_depart_position);
        }

    return b_clamb_jump;
}

bool CPHSimpleCharacter::ValidateWalkOnMesh()
{
    if (!(m_acceleration.magnitude() > 0.f))
        return true;

    return true;
}

void CPHSimpleCharacter::SetAcceleration(Fvector accel)
{
    if (!b_exist)
        return;

    if (!IsEnabled())
        if (!fsimilar(0.f, accel.magnitude()))
            Enable();
    m_acceleration = accel;
}

void CPHSimpleCharacter::SetCamDir(const Fvector& cam_dir) { m_cam_dir.set(cam_dir); }

static const float pull_force = 25.f;

void CPHSimpleCharacter::ApplyAcceleration() {}

void CPHSimpleCharacter::IPosition(Fvector& pos)
{
    if (!b_exist)
    {
        pos.set(m_safe_position);
    }
    else
    {
        GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, pos);
        pos.y -= m_radius;
    }
    VERIFY_BOUNDARIES(pos, phBoundaries, PhysicsRefObject());
    return;
}

void CPHSimpleCharacter::SetPosition(const Fvector& pos)
{
    if (!_valid(pos))
        return;
    VERIFY_BOUNDARIES(pos, phBoundaries, PhysicsRefObject());
    if (!b_exist)
        return;

    float full_height = m_cyl_hight + 2.f * m_radius;
    float center_y = pos.y + m_radius;

    m_death_position.set(pos.x, center_y, pos.z);
    m_safe_position.set(pos.x, center_y, pos.z);
    b_death_pos = false;

    GetPhysicsCore()->SetCharacterVirtualPosition(m_char_handle, Fvector().set(pos.x, center_y, pos.z));

    CPHDisablingTranslational::Reinit();
    m_char_handle_interpolation.ResetPositions();
    CPHObject::spatial_move();
}

void CPHSimpleCharacter::GetPosition(Fvector& vpos)
{
    if (!b_exist)
    {
        vpos.set(m_safe_position);
    }
    else
    {
        GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, vpos);
        vpos.y -= m_radius;
    }

    VERIFY_BOUNDARIES(vpos, phBoundaries, PhysicsRefObject());
}

void CPHSimpleCharacter::GetPreviousPosition(Fvector& pos)
{
    VERIFY(b_exist);
    VERIFY(!ph_world->Processing());
    m_char_handle_interpolation.GetPosition(pos, 0);
}

void CPHSimpleCharacter::GetVelocity(Fvector& vvel) const
{
    if (!b_exist)
    {
        vvel.set(m_safe_velocity);
        return;
    }
    GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, vvel);
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
    }
    GetPhysicsCore()->SetCharacterVirtualVelocity(m_char_handle, vel);
}

void CPHSimpleCharacter::SetMas(float mass)
{
    m_mass = mass;
    if (!b_exist || m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE)
        return;

    // В Jolt масса настраивается через свойства тела
    // GetPhysicsCore()->SetBodyMass(m_char_handle, mass);
}

EEnvironment CPHSimpleCharacter::CheckInvironment()
{
    if (m_elevator_state.ClimbingState())
        return peAtWall;
    if (b_on_ground)
        return peOnGround;

    return peInAir;
}

void CPHSimpleCharacter::SetPhysicsRefObject(IPhysicsShellHolder* ref_object)
{
    m_phys_ref_object = ref_object;
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        GetPhysicsCore()->SetCharacterVirtualUserData(m_char_handle, ref_object);
    }
}

void CPHSimpleCharacter::SafeAndLimitVelocity()
{
    Fvector linear_velocity;
    GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, linear_velocity);

    Fvector horiz_vel;
    horiz_vel.set(linear_velocity.x, 0, linear_velocity.z);
    float mag = horiz_vel.magnitude();

    float l_limit;
    if (is_control && !b_lose_control)
        l_limit = m_max_velocity / phTimefactor;
    else
        l_limit = default_l_limit;

    if (b_external_impulse)
    {
        float sq_mag = m_acceleration.square_magnitude();
        float ll_limit = m_ext_imulse.dotproduct(linear_velocity) * 10.f / fixed_step;
        if (sq_mag > EPS_L)
        {
            Fvector acc;
            acc.set(Fvector().mul(m_acceleration, 1.f / _sqrt(sq_mag)));
            Fvector vll;
            vll.mul(linear_velocity, 1.f / mag);
            float mxa = vll.dotproduct(acc);
            if (mxa * ll_limit > l_limit && !fis_zero(mxa))
            {
                ll_limit = l_limit / mxa;
            }
        }
        if (ll_limit > l_limit)
            l_limit = ll_limit;
    }

    m_mean_y = m_mean_y * 0.9999f + linear_velocity.y * 0.0001f;

    if (mag > l_limit)
    {
        if (!fis_zero(l_limit) && !fis_zero(mag))
        {
            float f = l_limit / mag;
            linear_velocity.x *= f;
            linear_velocity.z *= f;
            GetPhysicsCore()->SetCharacterVirtualVelocity(m_char_handle, linear_velocity);
        }
        else
        {
            linear_velocity.x = 0;
            linear_velocity.z = 0;
            GetPhysicsCore()->SetCharacterVirtualVelocity(m_char_handle, linear_velocity);
        }
    }

    if (linear_velocity.y > 35.0f || linear_velocity.y < -35.0f)
    {
        linear_velocity.y = std::clamp(linear_velocity.y, -35.0f, 35.0f);
        GetPhysicsCore()->SetCharacterVirtualVelocity(m_char_handle, linear_velocity);
    }

    Fvector body_pos;
    GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, body_pos);
    if (!_valid(body_pos))
    {
        Fvector fallback;
        fallback.x = m_safe_position.x - m_safe_velocity.x * fixed_step;
        fallback.y = m_safe_position.y - m_safe_velocity.y * fixed_step;
        fallback.z = m_safe_position.z - m_safe_velocity.z * fixed_step;
        GetPhysicsCore()->SetCharacterVirtualPosition(m_char_handle, fallback);
        body_pos = fallback;
    }

    m_safe_position = body_pos;
    m_safe_velocity = linear_velocity;
}

void CPHSimpleCharacter::SetObjectContactCallback(ObjectContactCallbackFun* callback)
{
    m_object_contact_callback = callback;
}

void CPHSimpleCharacter::SetObjectContactCallbackData(void* data) { m_native_callback_data = data; }
void CPHSimpleCharacter::AddObjectContactCallback(ObjectContactCallbackFun* callback) {
    if (callback && std::find(m_native_callbacks.begin(), m_native_callbacks.end(), callback) == m_native_callbacks.end())
        m_native_callbacks.push_back(callback);
}
void CPHSimpleCharacter::RemoveObjectContactCallback(ObjectContactCallbackFun* callback) {
    m_native_callbacks.erase(std::remove(m_native_callbacks.begin(), m_native_callbacks.end(), callback), m_native_callbacks.end());
}

void CPHSimpleCharacter::Disable()
{
    if (!b_exist)
        return;
    m_is_active = false;
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
        GetPhysicsCore()->DeactivateCharacterVirtual(m_char_handle);
    CPHCharacter::Disable();
}

void CPHSimpleCharacter::Enable()
{
    if (!b_exist)
        return;
    m_is_active = true;
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
        GetPhysicsCore()->ActivateCharacterVirtual(m_char_handle);
    CPHCharacter::Enable();
}

void CPHSimpleCharacter::EnableObject(CPHObject* obj)
{
    CPHCharacter::EnableObject(obj);
}

void CPHSimpleCharacter::SetWheelContactCallback(ObjectContactCallbackFun* callback) { VERIFY(b_exist); m_wheel_contact_callback = callback; }
void CPHSimpleCharacter::SetStaticContactCallBack(ObjectContactCallbackFun* callback) { VERIFY(b_exist); m_static_contact_callback = callback; }
ObjectContactCallbackFun* CPHSimpleCharacter::ObjectContactCallBack() { return m_object_contact_callback; }

void CPHSimpleCharacter::DispatchNativeContacts(bool& collide, bool first, CPhysicsGeom* geometry1, CPhysicsGeom* geometry2,
    const Fvector& normal, const Fvector& position, SGameMtl* material1, SGameMtl* material2)
{
    (first ? geometry1 : geometry2)->callback_data = m_native_callback_data;
    const auto callbacks = m_native_callbacks;
    if (m_object_contact_callback) m_object_contact_callback(collide, first, geometry1, geometry2, normal, position, material1, material2);
    for (auto* callback : callbacks) callback(collide, first, geometry1, geometry2, normal, position, material1, material2);
    const auto* other = first ? geometry2 : geometry1;
    if (m_static_contact_callback && !other->ph_ref_object && !other->ph_object)
        m_static_contact_callback(collide, first, geometry1, geometry2, normal, position, material1, material2);
    if (m_wheel_contact_callback && (first ? normal.y : -normal.y) > .5f)
        m_wheel_contact_callback(collide, first, geometry1, geometry2, normal, position, material1, material2);
}
void CPHSimpleCharacter::ProcessNativeContact(const NativePhysicsContact& contact)
{
    if (contact.depth < 0) return;
    if (contact.material2 == GAMEMTL_NONE_IDX || contact.material2 >= GMLib.CountMaterial()) return;
    auto* material = GMLib.GetMaterialByIdx(contact.material2);
    const bool passable = material->Flags.test(SGameMtl::flPassable) &&
        !material->Flags.test(SGameMtl::flActorObstacle);
    if (p_lastMaterialIDX && (passable || contact.normal.y > .5f))
        foot_material_update(contact.material2, contact.normal.y > .5f ? contact.material2 : GAMEMTL_NONE_IDX);
    if (!passable) {
        is_contact = b_any_contacts = true;
        ++m_contact_count;
        b_on_object = b_on_object || contact.kind2 == 1 || contact.kind2 == 3;
        if (!b_valide_ground_contact || contact.normal.y > m_ground_contact_normal.y) {
            m_ground_contact_normal = contact.normal;
            m_ground_contact_position = contact.position;
            b_valide_ground_contact = true;
        }
        if (!b_valide_wall_contact || dXZDot(contact.normal, m_acceleration) < dXZDot(m_wall_contact_normal, m_acceleration)) {
            m_wall_contact_normal = contact.normal;
            m_wall_contact_position = contact.position;
            b_valide_wall_contact = true;
        }
    }
    if (contact.kind2 == 0 && !contact.object2)
        UpdateStaticDamage(contact.normal, contact.position, material, true);
    else if (contact.kind2 == 1 || contact.kind2 == 3)
        UpdateDynamicDamage(contact.normal, contact.position, contact.body2, contact.material2, true);
}

u16 CPHSimpleCharacter::RetriveContactBone()
{
    if (!m_phys_ref_object) return 0;
    Fvector direction;
    m_collision_damage_info.HitDir(direction);
    if (direction.square_magnitude() > EPS_S) {
        collide::ray_defs query(m_collision_damage_info.HitPos(), direction, m_radius,
            CDB::OPT_ONLYNEAREST | CDB::OPT_CULL, collide::rqtBoth);
        RQR.r_clear();
        if (inl_ph_world().ObjectSpace().RayQuery(RQR, m_phys_ref_object->ObjectCollisionModel(), query))
            return static_cast<u16>(RQR.r_begin()->element);
    }
    auto* kinematics = m_phys_ref_object->ObjectKinematics();
    if (!kinematics || !kinematics->LL_BoneCount()) return 0;
    Fvector localPosition;
    localPosition.sub(m_collision_damage_info.HitPos(), m_phys_ref_object->ObjectPosition());
    Fmatrix orientation = m_phys_ref_object->ObjectXFORM();
    orientation.transpose();
    orientation.transform_dir(localPosition);
    float nearest = FLT_MAX;
    u16 bone = 0;
    for (u16 index = 0; index < kinematics->LL_BoneCount(); ++index) {
        Fvector delta;
        delta.sub(kinematics->LL_GetBoneInstance(index).mTransform.c, localPosition);
        const float distance = delta.square_magnitude();
        if (distance < nearest) { nearest = distance; bone = index; }
    }
    return bone;
}

void CPHSimpleCharacter::InitContact(bool& do_collide, bool bo1, float depth, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, u16 material_idx_1, u16 material_idx_2)
{
    // В Jolt Physics детали настройки контакта лучше перенести внутрь ContactListener.
    // Однако, чтобы сохранить логику X-Ray, мы эмулируем вызовы.

    u16 contact_material = bo1 ? material_idx_2 : material_idx_1;
    SGameMtl* tri_material = GMLib.GetMaterialByIdx(contact_material);

    bool bClimable = !!tri_material->Flags.test(SGameMtl::flClimable);
    if (is_control && m_elevator_state.ClimbingState())
    {
        b_any_contacts = true;
        is_contact = true;
    }

    u16 foot_material_idx = GAMEMTL_NONE_IDX;

    if (tri_material->Flags.test(SGameMtl::flPassable) && !do_collide)
    {
        UpdateStaticDamage(Fvector().set(0,1,0), Fvector().set(0,0,0), tri_material, bo1);
        foot_material_update(contact_material, foot_material_idx);
        return;
    }
    if (do_collide)
    {
        b_any_contacts = true;
        is_contact = true;
    }

    bool object = (my_geom && my_geom->get_body() != INVALID_CHARACTER_VIRTUAL_HANDLE) &&
                  (oposite_geom && oposite_geom->get_body() != INVALID_CHARACTER_VIRTUAL_HANDLE);

    b_on_object = b_on_object || object;

    // FootProcess(c, do_collide, bo1); // Упрощено для Jolt

    if (object)
    {
        CharacterVirtualHandle b = bo1 ? oposite_geom->get_body() : my_geom->get_body();
        u16 obj_material_idx = bo1 ? material_idx_2 : material_idx_1;
        UpdateDynamicDamage(Fvector().set(0,1,0), Fvector().set(0,0,0), b, obj_material_idx, bo1);
        contact_material = obj_material_idx;
    }

    foot_material_update(contact_material, foot_material_idx);

    ++m_contact_count;

    Fvector normal = {0, 1, 0}; // Нормаль должна приходить из коллбека (в Jolt это берется из ContactManifold)
    Fvector pos = {0, 0, 0};

    if (bo1)
    {
        if (normal.y > m_ground_contact_normal.y || !b_valide_ground_contact)
        {
            m_ground_contact_normal = normal;
            m_ground_contact_position = pos;
            b_valide_ground_contact = true;
        }
        if (dXZDot(normal, m_acceleration) < dXZDot(m_wall_contact_normal, m_acceleration) ||
            !b_valide_wall_contact)
        {
            m_wall_contact_normal = normal;
            m_wall_contact_position = pos;
            b_valide_wall_contact = true;
        }
    }
    else
    {
        if (normal.y < -m_ground_contact_normal.y || !b_valide_ground_contact)
        {
            m_ground_contact_normal.invert(normal);
            m_ground_contact_position = pos;
            b_valide_ground_contact = true;
        }
        if (dXZDot(normal, m_acceleration) > -dXZDot(m_wall_contact_normal, m_acceleration) ||
            !b_valide_wall_contact)
        {
            m_wall_contact_normal.invert(normal);
            m_wall_contact_position = pos;
            b_valide_wall_contact = true;
        }
    }

    UpdateStaticDamage(normal, pos, tri_material, bo1);
}

void CPHSimpleCharacter::JoltCharacterContactCallback(void* char_user_data,
                                                    const Fvector& contact_pos,
                                                    const Fvector& contact_normal,
                                                    const Fvector& contact_vel,
                                                    u32 tri_user_data,
                                                    BodyHandle other_body_handle,
                                                    void* other_body_user_data,
                                                    bool is_sensor)
{
    CPHSimpleCharacter* self = reinterpret_cast<CPHSimpleCharacter*>(char_user_data);
    if (!self || !self->b_exist) return;

    u16 mat_idx = GAMEMTL_NONE_IDX;
    if (tri_user_data != u32(-1))
    {
        CDB::TRI* tri = inl_ph_world().ObjectSpace().GetStaticTris() + tri_user_data;
        if (tri)
        {
            mat_idx = tri->material;
            SGameMtl* m = GMLib.GetMaterialByIdx(mat_idx);
            if (m && !m->Flags.test(SGameMtl::flPassable) && !m->Flags.test(SGameMtl::flLiquid))
            {
                if (self->p_lastMaterialIDX)
                {
                    *self->p_lastMaterialIDX = mat_idx;
                }
            }
        }
    }

    SGameMtl* tri_mat = (mat_idx != GAMEMTL_NONE_IDX) ? GMLib.GetMaterialByIdx(mat_idx) : nullptr;
    if (tri_mat)
    {
        // 1. Static impact damage / fall damage

        // 2. Passable / Liquid material sound and particle effects (bushes, water)
        if (tri_mat->Flags.test(SGameMtl::flPassable) || tri_mat->Flags.test(SGameMtl::flLiquid))
        {
            u16 self_mat = self->GetMaterial();
            if (self_mat == GAMEMTL_NONE_IDX || self_mat >= GMLib.CountMaterial())
            {
                self_mat = GMLib.GetMaterialIdx("creatures\\actor");
                if (self_mat == GAMEMTL_NONE_IDX)
                    self_mat = GMLib.GetMaterialIdx("creatures\\human");
                if (self_mat == GAMEMTL_NONE_IDX)
                    self_mat = GMLib.GetMaterialIdx("creature");
            }
            SGameMtl* self_game_mat = (self_mat != GAMEMTL_NONE_IDX) ? GMLib.GetMaterialByIdx(self_mat) : nullptr;

            if (self->m_static_contact_callback && self_game_mat)
            {
                bool do_collide = false;

                // Use CSphereGeom representing this character so get_callback_data() returns m_phys_ref_object
                Fsphere sph;
                sph.set(contact_pos, 0.5f);
                CSphereGeom dummy_geom(sph);
                dummy_geom.ph_ref_object = self->m_phys_ref_object;
                dummy_geom.ph_object = self;

                self->m_static_contact_callback(do_collide, true, &dummy_geom, nullptr, contact_normal, contact_pos, self_game_mat, tri_mat);
            }
        }
    }

    // 3. Dynamic object collision (Poltergeist props, flying items)
    if (other_body_handle != INVALID_BODY_HANDLE && other_body_user_data)
    {
        CPHElement* elem = reinterpret_cast<CPHElement*>(other_body_user_data);
        IPhysicsShellHolder* other_obj = elem->PhysicsRefObject();
        if (other_obj && !other_obj->ObjectGetDestroy() && !other_obj->IsActor() && !other_obj->IsStalker())
        {
            u16 dyn_mat_idx = GAMEMTL_NONE_IDX;
            if (elem->has_geoms() && elem->last_geom())
            {
                dyn_mat_idx = elem->last_geom()->material;
            }

            if (dyn_mat_idx == GAMEMTL_NONE_IDX || dyn_mat_idx >= GMLib.CountMaterial())
            {
                dyn_mat_idx = GMLib.GetMaterialIdx("default_object");
            }

        }
    }
}

void CPHSimpleCharacter::GroundNormal(Fvector& norm)
{
    if (m_elevator_state.ClimbingState())
        m_elevator_state.GetLeaderNormal(norm);
    else
        norm.set(m_ground_contact_normal);
}

u16 CPHSimpleCharacter::ContactBone() { return RetriveContactBone(); }

void CPHSimpleCharacter::SetMaterial(u16 material)
{
    m_material = material;
}

void CPHSimpleCharacter::get_State(SPHNetState& state)
{
    CPHCharacter::get_State(state);
    state.previous_position.y -= m_radius;
}

void CPHSimpleCharacter::set_State(const SPHNetState& state) { CPHCharacter::set_State(state); }

void CPHSimpleCharacter::get_spatial_params()
{
    // В Jolt пространственные параметры можно получить через AABB тела
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        Fvector c, d;
        GetPhysicsCore()->GetCharacterVirtualAABB(m_char_handle, c, d);
        spatial.sphere.P = c;
        spatial.sphere.R = d.magnitude();
        AABB.set(d);
    }
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
        GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, deathPos);
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
    u16 ret = u16(-1);

    IPhysicsShellHolder* object = nullptr;
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
}

float CPHSimpleCharacter::SCollisionDamageInfo::ContactVelocity() const
{
    float ret = m_contact_velocity;
    m_contact_velocity = 0;
    return ret;
}

void CPHSimpleCharacter::SCollisionDamageInfo::HitDir(Fvector& dir) const
{
    dir.set(m_damage_normal);
    dir.mul(m_dmc_signum);
}

void CPHSimpleCharacter::SCollisionDamageInfo::Reinit()
{
    m_obj_id = u16(-1);
    m_hit_callback = nullptr;
    m_contact_velocity = 0;
    is_initiated = false;
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
}

CElevatorState* CPHSimpleCharacter::ElevatorState() { return &m_elevator_state; }
ICollisionHitCallback* CPHSimpleCharacter::HitCallback() const { return m_collision_damage_info.m_hit_callback; }

const float resolve_depth = 0.05f;
static float restrictor_depth = 0.f;

void CPHSimpleCharacter::TestRestrictorContactCallbackFun(bool& do_colide, bool bo1, CPhysicsGeom* geom1, CPhysicsGeom* geom2, const Fvector& contact_normal, const Fvector& contact_pos, SGameMtl* material_1, SGameMtl* material_2)
{
    CPhysicsGeom* g_obj = bo1 ? geom2 : geom1;
    if (!g_obj || !g_obj->ph_object)
        return;
    if (g_obj->ph_object->CastType() != tpCharacter)
        return;
    CPHActorCharacter* actor_character = (static_cast<CPHCharacter*>(g_obj->ph_object))->CastActorCharacter();
    if (!actor_character)
        return;

    save_max(restrictor_depth, resolve_depth + 0.1f);
    do_colide = true;
}

bool CPHSimpleCharacter::UpdateRestrictionType(CPHCharacter* ach)
{
    VERIFY(ph_world && ph_world->Exist());
    if (m_restriction_type == m_new_restriction_type)
        return true;

    m_restriction_type = m_new_restriction_type;
    return true;
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

bool PickMaterial(u16& res_material_idx, const Fvector& pos_, const Fvector& dir_, float range_, IGameObject* ignore_object)
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
    Fvector pos;
    GetPosition(pos);
    pos.y += m_radius + material_pick_upset;
    if (m_last_picked_material != GAMEMTL_NONE_IDX && pos.similar(m_last_environment_update, material_update_tolerance))
    {
        *p_lastMaterialIDX = m_last_picked_material;
        return;
    }
    u16 new_material;
    VERIFY(!PhysicsRefObject() || smart_cast<IGameObject*>(PhysicsRefObject()));

    if (PickMaterial(new_material, pos, Fvector().set(0, -1, 0), m_radius + material_pick_dist + material_pick_upset,
            smart_cast<IGameObject*>(PhysicsRefObject())))
    {
        m_last_picked_material = new_material;
        *p_lastMaterialIDX = new_material;
        m_last_environment_update = pos;
    }
}

void CPHSimpleCharacter::SetNonInteractive(bool v) { b_non_interactive = v; }

void CPHSimpleCharacter::Collide()
{
    OnStartCollidePhase();

    inherited::Collide();
    if (injuriousMaterialIDX == GAMEMTL_NONE_IDX && (*p_lastMaterialIDX) != GAMEMTL_NONE_IDX &&
        GMLib.GetMaterialByIdx(*p_lastMaterialIDX)->Flags.test(SGameMtl::flInjurious))
        injuriousMaterialIDX = *p_lastMaterialIDX;
}

void CPHSimpleCharacter::OnStartCollidePhase() { injuriousMaterialIDX = GAMEMTL_NONE_IDX; }

void CPHSimpleCharacter::NetRelcase(IPhysicsShellHolder* O)
{
    inherited::NetRelcase(O);
    m_elevator_state.NetRelcase(O);
}

#ifdef DEBUG
void CPHSimpleCharacter::OnRender()
{
}
#endif
