#include "StdAfx.h"
#include "killer_victim_velocity_angle.h"
#include "game_state_accumulator.h"
#include "game_cl_base.h"
#include "Level.h"
#include "ammunition_groups.h"
#include "Actor.h"
#include "CharacterPhysicsSupport.h"
#include "PHMovementControl.h"
#include "xrEngine/CameraBase.h"

namespace award_system
{
killer_victim_angle::killer_victim_angle(game_state_accumulator* owner)
    : inherited(owner), m_killer_victim_angle_cos(1.f)
{
}

void killer_victim_angle::reset_game() { m_killer_victim_angle_cos = 1.f; }
u32 const killer_victim_angle::get_u32_param() { return u32(acosf(m_killer_victim_angle_cos) * (180.f / PI)); }
void killer_victim_angle::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (tmp_local_player->GameID != killer_id)
        return;

    CActor* killer_actor = m_owner->get_players_actor(killer_id);
    CActor* victim_actor = m_owner->get_players_actor(target_id);
    if (!victim_actor || !killer_actor)
        return;

    Fvector killer_dir;
    Fvector victim_velocity;

    CCharacterPhysicsSupport* tmp_cphys = victim_actor->character_physics_support();
    if (!tmp_cphys)
        return;

    CPHMovementControl* tmp_movc = tmp_cphys->movement();
    if (!tmp_movc)
        return;

    victim_velocity = tmp_movc->GetVelocity();
    if (victim_velocity.square_magnitude() < EPS)
        return;

    Fvector cam_pos, cam_norm;
    killer_actor->cam_Active()->Get(cam_pos, killer_dir, cam_norm);
    if (killer_dir.square_magnitude() < EPS)
        return;

    killer_dir.normalize();
    victim_velocity.normalize();

    m_killer_victim_angle_cos = _abs(killer_dir.dotproduct(victim_velocity));
    // Msg("* Killer victim angle cos: %1.3f", m_killer_victim_angle_cos);
    // Msg("* Killer victim angle: %d", get_u32_param());
}

} // namespace award_system
