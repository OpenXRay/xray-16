#include "StdAfx.h"
#include "burer.h"
#include "xrPhysics/PhysicsShell.h"
#include "CharacterPhysicsSupport.h"
#include "Actor.h"
#include "burer_state_manager.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "sound_player.h"
#include "Level.h"
#include "ai_monster_space.h"
#include "level_debug.h"
#include "ai/monsters/monster_velocity_space.h"
#include "GamePersistent.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_movement_base.h"
#include "burer_fast_gravi.h"
#include "ai/monsters/anti_aim_ability.h"
#include "Inventory.h"
#include "ActorCondition.h"
#include "xr_level_controller.h"
#include "Weapon.h"
#include "xrCore/_vector3d_ext.h"
#include "ai/monsters/control_direction_base.h"

bool CBurer::can_scan = true;

CBurer::CBurer()
{
    StateMan = new CStateManagerBurer(this);

    m_fast_gravi = new CBurerFastGravi();

    control().add(m_fast_gravi, ControlCom::eComCustom1);
}

CBurer::~CBurer()
{
    xr_delete(StateMan);
    xr_delete(m_fast_gravi);
}

void CBurer::reinit()
{
    inherited::reinit();

    DeactivateShield();

    time_last_scan = 0;
}

void CBurer::net_Destroy() { inherited::net_Destroy(); }
void CBurer::reload(LPCSTR section)
{
    inherited::reload(section);

    // add specific sounds
    sound().add(pSettings->r_string(section, "sound_gravi_attack"), DEFAULT_SAMPLE_COUNT, SOUND_TYPE_MONSTER_ATTACKING,
        MonsterSound::eHighPriority + 2, u32(MonsterSound::eBaseChannel), eMonsterSoundGraviAttack, "head");

    sound().add(pSettings->r_string(section, "sound_tele_attack"), DEFAULT_SAMPLE_COUNT, SOUND_TYPE_MONSTER_ATTACKING,
        MonsterSound::eHighPriority + 3, u32(MonsterSound::eBaseChannel), eMonsterSoundTeleAttack, "head");
}

void CBurer::ActivateShield() { m_shield_active = true; }
void CBurer::DeactivateShield() { m_shield_active = false; }
void CBurer::Load(LPCSTR section)
{
    inherited::Load(section);

    // anim().AddReplacedAnim		(&m_bDamaged, eAnimStandIdle,	eAnimStandDamaged);
    // anim().AddReplacedAnim		(&m_bDamaged, eAnimRun,			eAnimRunDamaged);
    // anim().AddReplacedAnim		(&m_bDamaged, eAnimWalkFwd,		eAnimWalkDamaged);

    anim().accel_load(section);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRun);

    anim().AddReplacedAnim(&m_bRunTurnLeft, eAnimRun, eAnimRunTurnLeft);
    anim().AddReplacedAnim(&m_bRunTurnRight, eAnimRun, eAnimRunTurnRight);

    particle_gravi_wave = pSettings->r_string(section, "Particle_Gravi_Wave");
    particle_gravi_prepare = pSettings->r_string(section, "Particle_Gravi_Prepare");
    particle_tele_object = pSettings->r_string(section, "Particle_Tele_Object");

    GEnv.Sound->create(sound_gravi_wave, pSettings->r_string(section, "sound_gravi_wave"), st_Effect, SOUND_TYPE_WORLD);
    GEnv.Sound->create(sound_tele_hold, pSettings->r_string(section, "sound_tele_hold"), st_Effect, SOUND_TYPE_WORLD);
    GEnv.Sound->create(sound_tele_throw, pSettings->r_string(section, "sound_tele_throw"), st_Effect, SOUND_TYPE_WORLD);

    m_gravi.cooldown = pSettings->r_u32(section, "Gravi_Cooldown");
    m_gravi.min_dist = pSettings->r_float(section, "Gravi_MinDist");
    m_gravi.max_dist = pSettings->r_float(section, "Gravi_MaxDist");
    m_gravi.speed = pSettings->r_float(section, "Gravi_Speed");
    m_gravi.step = pSettings->r_float(section, "Gravi_Step");
    m_gravi.time_to_hold = pSettings->r_u32(section, "Gravi_Time_To_Hold");
    m_gravi.radius = pSettings->r_float(section, "Gravi_Radius");
    m_gravi.impulse_to_objects = pSettings->r_float(section, "Gravi_Impulse_To_Objects");
    m_gravi.impulse_to_enemy = pSettings->r_float(section, "Gravi_Impulse_To_Enemy");
    m_gravi.hit_power = pSettings->r_float(section, "Gravi_Hit_Power");

    m_weight_to_stamina_hit = READ_IF_EXISTS(pSettings, r_float, section, "weight_to_stamina_hit", 0.02f);
    m_weapon_drop_stamina_k = READ_IF_EXISTS(pSettings, r_float, section, "weapon_drop_stamina_k", 3.f);

    m_runaway_distance = READ_IF_EXISTS(pSettings, r_float, section, "runaway_distance", 6.f);
    m_normal_distance = READ_IF_EXISTS(pSettings, r_float, section, "normal_distance", 12.f);
    m_max_runaway_time = READ_IF_EXISTS(pSettings, r_u32, section, "max_runaway_time", 5000);

    m_weapon_drop_velocity = READ_IF_EXISTS(pSettings, r_float, section, "weapon_drop_velocity", 8);

    m_shield_cooldown = READ_IF_EXISTS(pSettings, r_u32, section, "shield_cooldown", 4000);
    m_shield_time = READ_IF_EXISTS(pSettings, r_u32, section, "shield_time", 3000);
    m_shield_keep_particle = READ_IF_EXISTS(pSettings, r_string, section, "shield_keep_particle", 0);
    m_shield_keep_particle_period = READ_IF_EXISTS(pSettings, r_u32, section, "shield_keep_particle_period", 1000);

    m_tele_max_handled_objects = pSettings->r_u32(section, "Tele_Max_Handled_Objects");
    m_tele_max_time = READ_IF_EXISTS(pSettings, r_u32, section, "Tele_Max_Time", 10000);
    m_tele_time_to_hold = pSettings->r_u32(section, "Tele_Time_To_Hold");
    m_tele_object_min_mass = pSettings->r_float(section, "Tele_Object_Min_Mass");
    m_tele_object_max_mass = pSettings->r_float(section, "Tele_Object_Max_Mass");
    m_tele_find_radius = pSettings->r_float(section, "Tele_Find_Radius");
    m_tele_min_distance = READ_IF_EXISTS(pSettings, r_float, section, "tele_min_distance", 8);
    m_tele_max_distance = READ_IF_EXISTS(pSettings, r_float, section, "tele_max_distance", 30);
    m_tele_raise_speed = READ_IF_EXISTS(pSettings, r_float, section, "tele_raise_speed", 5.f);
    m_tele_fly_velocity = READ_IF_EXISTS(pSettings, r_float, section, "tele_fly_velocity", 30.f);
    m_tele_object_height = READ_IF_EXISTS(pSettings, r_float, section, "tele_object_height", 2.f);

    particle_fire_shield = pSettings->r_string(section, "Particle_Shield");

    SVelocityParam& velocity_none = move().get_velocity(MonsterMovement::eVelocityParameterIdle);
    SVelocityParam& velocity_turn = move().get_velocity(MonsterMovement::eVelocityParameterStand);
    SVelocityParam& velocity_walk = move().get_velocity(MonsterMovement::eVelocityParameterWalkNormal);
    SVelocityParam& velocity_run = move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    // SVelocityParam &velocity_walk_dmg	= 	move().get_velocity(MonsterMovement::eVelocityParameterWalkDamaged);
    // SVelocityParam &velocity_run_dmg	= 	move().get_velocity(MonsterMovement::eVelocityParameterRunDamaged);
    // SVelocityParam &velocity_steal		= 	move().get_velocity(MonsterMovement::eVelocityParameterSteal);
    //		SVelocityParam &velocity_drag		= move().get_velocity(MonsterMovement::eVelocityParameterDrag);

    anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimStandTurnLeft, "stand_turn_ls_", -1, &velocity_turn,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimStandTurnRight, "stand_turn_rs_", -1, &velocity_turn,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    //	anim().AddAnim(eAnimStandDamaged,	"stand_idle_dmg_",		-1, &velocity_none,		PS_STAND); //, "fx_stand_f",
    //"fx_stand_b", "fx_stand_l", "fx_stand_r");

    anim().AddAnim(eAnimWalkFwd, "stand_walk_fwd_", -1, &velocity_walk,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimWalkDamaged,	"stand_walk_fwd_dmg_",	-1, &velocity_walk_dmg,	PS_STAND); //, 	"fx_stand_f",
    // "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimRun, "stand_run_fwd_", -1, &velocity_run,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimRunDamaged,		"stand_run_dmg_",		-1,	&velocity_run_dmg,	PS_STAND); //, "fx_stand_f",
    // "fx_stand_b", "fx_stand_l", "fx_stand_r");

    anim().AddAnim(eAnimAttack, "stand_attack_", -1, &velocity_turn,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");

    anim().AddAnim(eAnimDie, "stand_die_", -1, &velocity_none,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");

    anim().AddAnim(eAnimShieldStart, "stand_shield_", -1, &velocity_turn,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimShieldContinue, "stand_shield_idle_", -1, &velocity_turn,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");

    anim().AddAnim(eAnimTeleFire, "stand_power_attack_", -1, &velocity_turn,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimTelekinesis, "telekinesis_", -1, &velocity_turn,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimGraviFire, "stand_power_attack_", -1, &velocity_turn,
        PS_STAND); //, 	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");

    anim().AddAnim(eAnimRunTurnLeft, "stand_run_fwd_turn_left_", -1, &velocity_run, PS_STAND);
    anim().AddAnim(eAnimRunTurnRight, "stand_run_fwd_turn_right_", -1, &velocity_run, PS_STAND);

    // 	anim().AddAnim(eAnimScared,			"stand_scared_",		-1, &velocity_none,		PS_STAND); //, 	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // 	anim().AddAnim(eAnimSteal,			"stand_steal_",			-1, &velocity_steal,	PS_STAND); //, 	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // 	anim().AddAnim(eAnimEat,			"sit_eat_",				-1, &velocity_none,		PS_SIT); //, 	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    //
    // 	anim().AddAnim(eAnimSitIdle,		"sit_idle_",			-1, &velocity_none,		PS_SIT); //, 	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // 	anim().AddAnim(eAnimCheckCorpse,	"sit_check_corpse_",	-1, &velocity_none,		PS_SIT); //, "fx_stand_f",
    // "fx_stand_b", "fx_stand_l", "fx_stand_r");
    // 	anim().AddAnim(eAnimSitStandUp,		"sit_stand_up_",		-1, &velocity_none,		PS_SIT); //, "fx_stand_f",
    // "fx_stand_b", "fx_stand_l", "fx_stand_r");
    // 	anim().AddAnim(eAnimStandSitDown,	"stand_sit_down_",		-1, &velocity_none,		PS_STAND); //, "fx_stand_f",
    // "fx_stand_b", "fx_stand_l", "fx_stand_r");

    //	anim().AddTransition(PS_SIT,		PS_STAND,		eAnimSitStandUp,	false);
    //	anim().AddTransition(PS_STAND,		PS_SIT,			eAnimStandSitDown,	false);

    anim().LinkAction(ACT_STAND_IDLE, eAnimStandIdle);
    //	anim().LinkAction		(ACT_SIT_IDLE,		eAnimSitIdle);
    //	anim().LinkAction		(ACT_LIE_IDLE,		eAnimSitIdle);
    anim().LinkAction(ACT_WALK_FWD, eAnimWalkFwd);
    anim().LinkAction(ACT_WALK_BKWD, eAnimWalkFwd);
    anim().LinkAction(ACT_RUN, eAnimRun);
    // anim().LinkAction(ACT_EAT,			eAnimEat);
    anim().LinkAction(ACT_SLEEP, eAnimStandIdle); // eAnimSitIdle);
    anim().LinkAction(ACT_REST, eAnimStandIdle); // eAnimSitIdle);
    anim().LinkAction(ACT_DRAG, eAnimWalkFwd);
    anim().LinkAction(ACT_ATTACK, eAnimAttack);
// anim().LinkAction(ACT_STEAL,		eAnimSteal);
// anim().LinkAction(ACT_LOOK_AROUND,	eAnimScared);

#ifdef DEBUG
    anim().accel_chain_test();
#endif

    m_force_gravi_attack = false;

    PostLoad(section);
}

void CBurer::PostLoad(LPCSTR section)
{
    inherited::PostLoad(section);
    m_anti_aim->set_callback(anti_aim_ability::hit_callback(this, &CBurer::StaminaHit));
}

void CBurer::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    CTelekinesis::schedule_update();
}

void CBurer::CheckSpecParams(u32 spec_params) {}
void xr_stdcall CBurer::StaminaHit()
{
    if (GodMode())
    {
        return;
    }

    CWeapon* const active_weapon = smart_cast<CWeapon*>(Actor()->inventory().ActiveItem());
    if (!active_weapon)
    {
        return;
    }

    float const weight = active_weapon->Weight();
    float const stamina_hit = weight * m_weight_to_stamina_hit;

    bool const do_weapon_drop = Actor()->conditions().GetPower() < stamina_hit * m_weapon_drop_stamina_k;

    Actor()->conditions().PowerHit(stamina_hit, false);

    if (do_weapon_drop)
    {
        Fvector dir = Actor()->Direction();
        if (dir.y < 0.f)
        {
            dir.y = -dir.y;
        }
        active_weapon->SetActivationSpeedOverride(normalize(dir) * m_weapon_drop_velocity);

        if (!Actor()->inventory().Action((u16)kDROP, CMD_STOP))
        {
            Actor()->g_PerformDrop();
        }
    }
}

void CBurer::UpdateGraviObject()
{
    if (!m_gravi_object.active)
    {
        return;
    }

    if (!m_gravi_object.enemy || (m_gravi_object.enemy && m_gravi_object.enemy->getDestroy()))
    {
        m_gravi_object.deactivate();
        return;
    }

    if (m_gravi_object.from_pos.distance_to(m_gravi_object.cur_pos) >
        m_gravi_object.from_pos.distance_to(m_gravi_object.target_pos))
    {
        m_gravi_object.deactivate();
        return;
    }

    float dt = float(Device.dwTimeGlobal - m_gravi_object.time_last_update);
    float dist = dt * float(m_gravi.speed) / 1000.f;

    if (dist < m_gravi.step)
        return;

    Fvector new_pos;
    Fvector dir;
    dir.sub(m_gravi_object.target_pos, m_gravi_object.cur_pos);
    dir.normalize();

    new_pos.mad(m_gravi_object.cur_pos, dir, dist);

    // Trace to enemy
    Fvector enemy_center;
    m_gravi_object.enemy->Center(enemy_center);
    dir.sub(enemy_center, new_pos);
    dir.normalize();

    float trace_dist = float(m_gravi.step);

    collide::rq_result l_rq;
    if (Level().ObjectSpace.RayPick(new_pos, dir, trace_dist, collide::rqtBoth, l_rq, NULL))
    {
        const IGameObject* enemy = smart_cast<const IGameObject*>(m_gravi_object.enemy);
        if ((l_rq.O == enemy) && (l_rq.range < trace_dist))
        {
            // check for visibility
            bool b_enemy_visible = false;
            xr_vector<IGameObject*> visible_objects;
            feel_vision_get(visible_objects);

            // find object
            for (u32 i = 0; i < visible_objects.size(); i++)
            {
                if (visible_objects[i] == enemy)
                {
                    b_enemy_visible = true;
                    break;
                }
            }

            if (b_enemy_visible)
            {
                Fvector impulse_dir;

                impulse_dir.set(0.0f, 0.0f, 1.0f);
                impulse_dir.normalize();

                HitEntity(m_gravi_object.enemy, m_gravi.hit_power, m_gravi.impulse_to_enemy, impulse_dir,
                    ALife::eHitTypeStrike, false);
                m_gravi_object.deactivate();
                return;
            }
        }
    }

    m_gravi_object.cur_pos = new_pos;
    m_gravi_object.time_last_update = Device.dwTimeGlobal;

    // ---------------------------------------------------------------------
    // draw particle
    CParticlesObject* ps = CParticlesObject::Create(particle_gravi_wave, TRUE);

    // вычислить позицию и направленность партикла
    Fmatrix pos;
    pos.identity();
    pos.k.set(dir);
    Fvector::generate_orthonormal_basis_normalized(pos.k, pos.j, pos.i);
    // установить позицию
    pos.translate_over(m_gravi_object.cur_pos);

    ps->UpdateParent(pos, zero_vel);
    ps->Play(false);

    // hit objects
    m_nearest.clear();
    Level().ObjectSpace.GetNearest(m_nearest, m_gravi_object.cur_pos, m_gravi.radius, NULL);
    // xr_vector<IGameObject*> &m_nearest = Level().ObjectSpace.q_nearest;

    for (u32 i = 0; i < m_nearest.size(); i++)
    {
        CPhysicsShellHolder* obj = smart_cast<CPhysicsShellHolder*>(m_nearest[i]);
        if (!obj || !obj->m_pPhysicsShell)
            continue;

        Fvector dir;
        dir.sub(obj->Position(), m_gravi_object.cur_pos);
        dir.normalize();
        obj->m_pPhysicsShell->applyImpulse(dir, m_gravi.impulse_to_objects * obj->m_pPhysicsShell->getMass());
    }

    // играть звук
    Fvector snd_pos = m_gravi_object.cur_pos;
    snd_pos.y += 0.5f;
    if (sound_gravi_wave._feedback())
    {
        sound_gravi_wave.set_position(snd_pos);
    }
    else
        GEnv.Sound->play_at_pos(sound_gravi_wave, 0, snd_pos);
}

void CBurer::UpdateCL()
{
    inherited::UpdateCL();

    UpdateGraviObject();
    // if (m_fast_gravi->check_start_conditions())
    //	control().activate(ControlCom::eComCustom1);
}

void CBurer::StartGraviPrepare()
{
    const CEntityAlive* enemy = EnemyMan.get_enemy();
    if (!enemy)
        return;

    CActor* pA = const_cast<CActor*>(smart_cast<const CActor*>(enemy));
    if (!pA)
        return;

    pA->CParticlesPlayer::StartParticles(particle_gravi_prepare, Fvector().set(0.0f, 0.1f, 0.0f), pA->ID());
}
void CBurer::StopGraviPrepare()
{
    CActor* pA = Actor();
    if (!pA)
        return;
    pA->CParticlesPlayer::StopParticles(particle_gravi_prepare, BI_NONE, true);
}

void CBurer::StartTeleObjectParticle(CGameObject* pO)
{
    CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pO);
    if (!PP)
        return;
    PP->StartParticles(particle_tele_object, Fvector().set(0.0f, 0.1f, 0.0f), pO->ID());
}
void CBurer::StopTeleObjectParticle(CGameObject* pO)
{
    CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pO);
    if (!PP)
        return;
    PP->StopParticles(particle_tele_object, BI_NONE, true);
}

void CBurer::Hit(SHit* pHDS)
{
    if (m_shield_active && pHDS->hit_type == ALife::eHitTypeFireWound && Device.dwFrame != last_hit_frame)
    {
        // вычислить позицию и направленность партикла
        Fmatrix pos;
        // CParticlesPlayer::MakeXFORM(this,element,Fvector().set(0.f,0.f,1.f),p_in_object_space,pos);
        CParticlesPlayer::MakeXFORM(this, pHDS->bone(), pHDS->dir, pHDS->p_in_bone_space, pos);

        // установить particles
        CParticlesObject* ps = CParticlesObject::Create(particle_fire_shield, TRUE);

        ps->UpdateParent(pos, Fvector().set(0.f, 0.f, 0.f));
        GamePersistent().ps_needtoplay.push_back(ps);
    }
    else if (!m_shield_active)
    {
        inherited::Hit(pHDS);
    }

    last_hit_frame = Device.dwFrame;
}

void CBurer::Die(IGameObject* who)
{
    inherited::Die(who);

    if (com_man().ta_is_active())
    {
        com_man().ta_deactivate();
    }

    CTelekinesis::Deactivate();
}

void CBurer::net_Relcase(IGameObject* O)
{
    inherited::net_Relcase(O);

    TTelekinesis::remove_links(O);
}

#ifdef DEBUG
CBaseMonster::SDebugInfo CBurer::show_debug_info()
{
    CBaseMonster::SDebugInfo info = inherited::show_debug_info();
    if (!info.active)
        return CBaseMonster::SDebugInfo();

    string128 text;
    DBG().text(this).add_item(text, info.x, info.y += info.delta_y, info.color);
    DBG().text(this).add_item(
        "---------------------------------------", info.x, info.y += info.delta_y, info.delimiter_color);

    return CBaseMonster::SDebugInfo();
}
#endif

void CBurer::face_enemy()
{
    if (!EnemyMan.get_enemy())
    {
        return;
    }
    Fvector const enemy_pos = EnemyMan.get_enemy()->Position();
    Fvector const self_pos = Position();
    Fvector const self2enemy = enemy_pos - self_pos;
    bool const good_aiming = angle_between_vectors(self2enemy, Direction()) < deg2rad(20.f);

    if (!good_aiming)
    {
        dir().face_target(enemy_pos);
    }

    set_action(ACT_STAND_IDLE);
}

extern CActor* g_actor;

bool actor_is_reloading_weapon()
{
    if (!g_actor)
    {
        return false;
    }

    CWeapon* const active_weapon = smart_cast<CWeapon*>(Actor()->inventory().ActiveItem());
    if (active_weapon && active_weapon->GetState() == CWeapon::eReload)
    {
        return true;
    }

    return false;
}
