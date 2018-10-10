#include "StdAfx.h"
#include "actor_mp_state.h"

#define USE_LOGIC_ACCELERATION

enum
{
    physics_linear_velocity_x_flag = u32(1) << 0,
    physics_linear_velocity_y_flag = u32(1) << 1,
    physics_linear_velocity_z_flag = u32(1) << 2,
    physics_position_x_flag = u32(1) << 3,
    physics_position_y_flag = u32(1) << 4,
    physics_position_z_flag = u32(1) << 5,
    model_yaw_flag = u32(1) << 6,
    camera_yaw_flag = u32(1) << 7,
    camera_pitch_flag = u32(1) << 8,
    camera_roll_flag = u32(1) << 9,
    inventory_active_slot_flag = u32(1) << 10,
    body_state_flags_flag = u32(1) << 11,
    health_flag = u32(1) << 12,
    radiation_flag = u32(1) << 13,
    physics_state_enabled_flag = u32(1) << 14,
#ifdef USE_LOGIC_ACCELERATION
    logic_acceleration_flag = u32(1) << 15,
#endif // USE_LOGIC_ACCELERATION
};

#if 0
enum {
	is_acceleration_null				= u32(1) << 0,
	is_roll_null						= u32(1) << 1,
	is_camera_yaw_the_same_as_model_yaw	= u32(1) << 2,
	is_physics_state_enabled			= u32(1) << 3,
};
#endif // 0

enum
{
    inventory_active_slot_bits = u32(4),
    body_state_flags_bits = u32(15),
    health_bits = u32(8),
    radiation_bits = u32(4),
    physics_state_enabled_bits = u32(1),
};

static const float min_linear_velocity_component = -32.f;
static const float max_linear_velocity_component = 32.f;

#ifdef USE_DIFFERENCES
IC bool is_similar(const Fvector& _0, const Fvector& _1)
{
    return (fsimilar(_0.x, _1.x, EPS) && fsimilar(_0.y, _1.y, EPS) && fsimilar(_0.z, _1.z, EPS));
}

IC bool is_similar(const float& _0, const float& _1) { return (!!fsimilar(_0, _1, EPS)); }
IC bool is_similar(const u32& _0, const u32& _1) { return (_0 == _1); }
IC bool is_similar(const u16& _0, const u16& _1) { return (_0 == _1); }
#endif // USE_DIFFERENCES

IC bool actor_mp_state_holder::check(const int& flag) const
{
#ifdef USE_DIFFERENCES
    return (!!(m_mask & flag));
#else // USE_DIFFERENCES
    return (true);
#endif // USE_DIFFERENCES
}

IC void write(const u32& bit_count, const u32& value, u32& current, u32& output)
{
    output |= ((value & ((u32(1) << bit_count) - 1)) << current);
    current += bit_count;
    VERIFY(current <= 32);
}

IC u32 read(const u32& bit_count, u32& current, const u32& output)
{
    u32 result = (output >> current) & ((u32(1) << bit_count) - 1);
    current += bit_count;
    VERIFY(current <= 32);
    return (result);
}

IC float unpack(const u32& packed_value, const u32 bit_count)
{
    u32 max_value = (u32(1) << bit_count) - 1;
    float result = float(packed_value) / (float(max_value) + .0001f);
    return (result);
}

IC u32 pack(const float& unpacked_value, const u32 bit_count)
{
    float inValue = unpacked_value;
    clamp(inValue, 0.f, 1.f);
    u32 max_value = (u32(1) << bit_count) - 1;
    u32 result = iFloor(float(max_value) * inValue + .5f);

    if (bit_count > 1)
        if (result == 0 && unpacked_value != 0)
            result += 1;

    clamp(result, u32(0), max_value);
    return (result);
}

bool actor_mp_state_holder::relevant(const actor_mp_state& state)
{
#ifdef USE_DIFFERENCES
    m_mask = 0;

    m_mask |= is_similar(m_state.physics_linear_velocity.x, state.physics_linear_velocity.x) ?
        physics_linear_velocity_x_flag :
        0;
    m_mask |= is_similar(m_state.physics_linear_velocity.y, state.physics_linear_velocity.y) ?
        physics_linear_velocity_y_flag :
        0;
    m_mask |= is_similar(m_state.physics_linear_velocity.z, state.physics_linear_velocity.z) ?
        physics_linear_velocity_z_flag :
        0;
    m_mask |= is_similar(m_state.physics_position.x, state.physics_position.x) ? physics_position_x_flag : 0;
    m_mask |= is_similar(m_state.physics_position.y, state.physics_position.y) ? physics_position_y_flag : 0;
    m_mask |= is_similar(m_state.physics_position.z, state.physics_position.z) ? physics_position_z_flag : 0;
    m_mask |= is_similar(m_state.model_yaw, state.model_yaw) ? model_yaw_flag : 0;
    m_mask |= is_similar(m_state.camera_yaw, state.camera_yaw) ? camera_yaw_flag : 0;
    m_mask |= is_similar(m_state.camera_pitch, state.camera_pitch) ? camera_pitch_flag : 0;
    m_mask |= is_similar(m_state.camera_roll, state.camera_roll) ? camera_roll_flag : 0;
    m_mask |= is_similar(m_state.inventory_active_slot, state.inventory_active_slot) ? inventory_active_slot_flag : 0;
    m_mask |= is_similar(m_state.body_state_flags, state.body_state_flags) ? body_state_flags_flag : 0;
    m_mask |= is_similar(m_state.health, state.health) ? health_flag : 0;
    m_mask |= is_similar(m_state.radiation, state.radiation) ? radiation_flag : 0;
    m_mask |= is_similar(m_state.physics_state_enabled, state.physics_state_enabled) ? physics_state_enabled_flag : 0;
#ifdef USE_LOGIC_ACCELERATION
    m_mask |= is_similar(m_state.logic_acceleration, state.logic_acceleration) ? logic_acceleration_flag : 0;
#endif // USE_LOGIC_ACCELERATION
#endif // USE_DIFFERENCES
    m_state = state;

#ifdef USE_DIFFERENCES
    return (!!m_mask);
#else // USE_DIFFERENCES
    return (true);
#endif // USE_DIFFERENCES
}

void actor_mp_state_holder::write(NET_Packet& packet)
{
#ifdef USE_DIFFERENCES
    R_ASSERT(m_mask);
    packet.w_u32(m_mask);
#endif // USE_DIFFERENCES

    packet.w_u32(m_state.time);

    clamp(m_state.physics_linear_velocity.x, min_linear_velocity_component, max_linear_velocity_component);
    clamp(m_state.physics_linear_velocity.y, min_linear_velocity_component, max_linear_velocity_component);
    clamp(m_state.physics_linear_velocity.z, min_linear_velocity_component, max_linear_velocity_component);

    if (check(physics_linear_velocity_x_flag))
        packet.w_float_q8(
            m_state.physics_linear_velocity.x, min_linear_velocity_component, max_linear_velocity_component);
    if (check(physics_linear_velocity_y_flag))
        packet.w_float_q8(
            m_state.physics_linear_velocity.y, min_linear_velocity_component, max_linear_velocity_component);
    if (check(physics_linear_velocity_z_flag))
        packet.w_float_q8(
            m_state.physics_linear_velocity.z, min_linear_velocity_component, max_linear_velocity_component);
    if (check(physics_position_x_flag))
        packet.w_float(m_state.physics_position.x);
    if (check(physics_position_y_flag))
        packet.w_float(m_state.physics_position.y);
    if (check(physics_position_z_flag))
        packet.w_float(m_state.physics_position.z);
    if (check(model_yaw_flag))
        packet.w_float(m_state.model_yaw); // packet.w_float_q8	(m_state.model_yaw		,0.f	,PI_MUL_2);
    if (check(camera_yaw_flag))
        packet.w_float_q8(
            m_state.camera_yaw, 0.f, PI_MUL_2); // packet.w_float		(m_state.camera_yaw						);
    if (check(camera_pitch_flag))
        packet.w_float_q8(
            m_state.camera_pitch, 0.f, PI_MUL_2); // packet.w_float		(m_state.camera_pitch					);
    if (check(camera_roll_flag))
        packet.w_float_q8(
            m_state.camera_roll, 0.f, PI_MUL_2); // packet.w_float		(m_state.camera_roll					);

#ifdef USE_LOGIC_ACCELERATION
    if (check(logic_acceleration_flag))
        packet.w_sdir(m_state.logic_acceleration);
#else // USE_LOGIC_ACCELERATION
    m_state.logic_acceleration.set(0.f, 0.f, 0.f);
#endif // USE_LOGIC_ACCELERATION

    u32 current = 0;
    u32 output = 0;

    if (check(inventory_active_slot_flag))
        ::write(inventory_active_slot_bits, m_state.inventory_active_slot, current, output);
    if (check(body_state_flags_flag))
        ::write(body_state_flags_bits, m_state.body_state_flags, current, output);
    if (check(health_flag))
        ::write(health_bits, pack(m_state.health, health_bits), current, output);
    if (check(radiation_flag))
        ::write(radiation_bits, pack(m_state.radiation, radiation_bits), current, output);
    if (check(physics_state_enabled_flag))
        ::write(physics_state_enabled_bits, m_state.physics_state_enabled, current, output);

    packet.w_u8(u8((output & 0x000000ff) >> 0));
    if (current <= 8)
        return;

    packet.w_u8(u8((output & 0x0000ff00) >> 8));
    if (current <= 16)
        return;

    packet.w_u8(u8((output & 0x00ff0000) >> 16));
    if (current <= 24)
        return;

    packet.w_u8(u8((output & 0xff000000) >> 24));
}

void actor_mp_state_holder::read(NET_Packet& packet)
{
#ifdef USE_DIFFERENCES
    packet.r_u32(m_mask);
#endif // USE_DIFFERENCES

    packet.r_u32(m_state.time);

    if (check(physics_linear_velocity_x_flag))
        packet.r_float_q8(
            m_state.physics_linear_velocity.x, min_linear_velocity_component, max_linear_velocity_component);
    if (check(physics_linear_velocity_y_flag))
        packet.r_float_q8(
            m_state.physics_linear_velocity.y, min_linear_velocity_component, max_linear_velocity_component);
    if (check(physics_linear_velocity_z_flag))
        packet.r_float_q8(
            m_state.physics_linear_velocity.z, min_linear_velocity_component, max_linear_velocity_component);
    if (check(physics_position_x_flag))
        packet.r_float(m_state.physics_position.x);
    if (check(physics_position_y_flag))
        packet.r_float(m_state.physics_position.y);
    if (check(physics_position_z_flag))
        packet.r_float(m_state.physics_position.z);
    if (check(model_yaw_flag))
        packet.r_float(m_state.model_yaw); // packet.r_float_q8	(m_state.model_yaw,0.f,PI_MUL_2			);
    if (check(camera_yaw_flag))
        packet.r_float_q8(
            m_state.camera_yaw, 0.f, PI_MUL_2); // packet.r_float		(m_state.camera_yaw						);
    if (check(camera_pitch_flag))
        packet.r_float_q8(
            m_state.camera_pitch, 0.f, PI_MUL_2); // packet.r_float		(m_state.camera_pitch					);
    if (check(camera_roll_flag))
        packet.r_float_q8(
            m_state.camera_roll, 0.f, PI_MUL_2); // packet.r_float		(m_state.camera_roll					);

#ifdef USE_LOGIC_ACCELERATION
    if (check(logic_acceleration_flag))
        packet.r_sdir(m_state.logic_acceleration);
#else // USE_LOGIC_ACCELERATION
    m_state.logic_acceleration.set(0.f, 0.f, 0.f);
#endif // USE_LOGIC_ACCELERATION

    m_state.position = m_state.physics_position;

    u32 current = 0;
    u32 total_bit_count = 0;
    u32 output = 0;

    if (check(inventory_active_slot_flag))
        total_bit_count += inventory_active_slot_bits;
    if (check(body_state_flags_flag))
        total_bit_count += body_state_flags_bits;
    if (check(health_flag))
        total_bit_count += health_bits;
    if (check(radiation_flag))
        total_bit_count += radiation_bits;
    if (check(physics_state_enabled_flag))
        total_bit_count += physics_state_enabled_bits;

    if (!total_bit_count)
        return;

    output |= u32(packet.r_u8()) << 0;

    if (total_bit_count > 8)
        output |= u32(packet.r_u8()) << 8;

    if (total_bit_count > 16)
        output |= u32(packet.r_u8()) << 16;

    if (total_bit_count > 24)
        output |= u32(packet.r_u8()) << 24;

    if (check(inventory_active_slot_flag))
        m_state.inventory_active_slot = ::read(inventory_active_slot_bits, current, output);
    if (check(body_state_flags_flag))
        m_state.body_state_flags = ::read(body_state_flags_bits, current, output);
    if (check(health_flag))
        m_state.health = unpack(::read(health_bits, current, output), health_bits);
    if (check(radiation_flag))
        m_state.radiation = unpack(::read(radiation_bits, current, output), radiation_bits);
    if (check(physics_state_enabled_flag))
        m_state.physics_state_enabled = ::read(physics_state_enabled_bits, current, output);
}
