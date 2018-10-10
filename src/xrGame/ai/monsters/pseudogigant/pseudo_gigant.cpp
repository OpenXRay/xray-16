#include "StdAfx.h"
#include "ai/monsters/pseudogigant/pseudo_gigant.h"
#include "ai/monsters/pseudogigant/pseudo_gigant_step_effector.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "Level.h"
#include "pseudogigant_state_manager.h"
#include "ai/monsters/monster_velocity_space.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_movement_base.h"
#include "ai/monsters/ai_monster_effector.h"
#include "xrEngine/CameraBase.h"
#include "xr_level_controller.h"
#include "detail_path_manager_space.h"
#include "detail_path_manager.h"
#include "CharacterPhysicsSupport.h"
#include "ai/monsters/control_path_builder_base.h"

CPseudoGigant::CPseudoGigant()
{
    CControlled::init_external(this);

    StateMan = new CStateManagerGigant(this);

    com_man().add_ability(ControlCom::eControlRunAttack);
    com_man().add_ability(ControlCom::eControlThreaten);
    // com_man().add_ability(ControlCom::eControlJump);
    com_man().add_ability(ControlCom::eControlRotationJump);
}

CPseudoGigant::~CPseudoGigant() { xr_delete(StateMan); }
void CPseudoGigant::Load(LPCSTR section)
{
    inherited::Load(section);

    anim().AddReplacedAnim(&m_bDamaged, eAnimRun, eAnimRunDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimWalkFwd, eAnimWalkDamaged);
    // anim().AddReplacedAnim(&m_bRunTurnLeft,		eAnimRun,		eAnimRunTurnLeft);
    // anim().AddReplacedAnim(&m_bRunTurnRight,	eAnimRun,		eAnimRunTurnRight);

    anim().accel_load(section);
    // anim().accel_chain_add		(eAnimWalkFwd,		eAnimRun);
    // anim().accel_chain_add		(eAnimWalkFwd,		eAnimRunTurnLeft);
    // anim().accel_chain_add		(eAnimWalkFwd,		eAnimRunTurnRight);
    // anim().accel_chain_add		(eAnimWalkDamaged,	eAnimRunDamaged);

    step_effector.time = pSettings->r_float(section, "step_effector_time");
    step_effector.amplitude = pSettings->r_float(section, "step_effector_amplitude");
    step_effector.period_number = pSettings->r_float(section, "step_effector_period_number");

    SVelocityParam& velocity_none = move().get_velocity(MonsterMovement::eVelocityParameterIdle);
    SVelocityParam& velocity_turn = move().get_velocity(MonsterMovement::eVelocityParameterStand);
    SVelocityParam& velocity_walk = move().get_velocity(MonsterMovement::eVelocityParameterWalkNormal);
    //	SVelocityParam &velocity_run		= move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    SVelocityParam& velocity_walk_dmg = move().get_velocity(MonsterMovement::eVelocityParameterWalkDamaged);
    //	SVelocityParam &velocity_run_dmg	= move().get_velocity(MonsterMovement::eVelocityParameterRunDamaged);
    SVelocityParam& velocity_steal = move().get_velocity(MonsterMovement::eVelocityParameterSteal);

    anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimStandTurnLeft, "stand_turn_ls_", -1, &velocity_turn, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimStandTurnRight, "stand_turn_rs_", -1, &velocity_turn, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimLieIdle, "stand_sleep_", -1, &velocity_none, PS_LIE, "fx_stand_f", "fx_stand_b", "fx_stand_l",
        "fx_stand_r");
    anim().AddAnim(
        eAnimSitIdle, "sit_idle_", -1, &velocity_none, PS_SIT, "fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(
        eAnimSleep, "stand_sleep_", -1, &velocity_none, PS_LIE, "fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimWalkFwd, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimWalkDamaged, "stand_walk_fwd_dmg_", -1, &velocity_walk_dmg, PS_STAND, "fx_stand_f",
        "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimRun, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l",
        "fx_stand_r");
    anim().AddAnim(eAnimRunDamaged, "stand_walk_fwd_dmg_", -1, &velocity_walk_dmg, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(
        eAnimEat, "stand_eat_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimAttack, "stand_attack_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l",
        "fx_stand_r");
    anim().AddAnim(eAnimLookAround, "stand_idle_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimSteal, "stand_steal_", -1, &velocity_steal, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l",
        "fx_stand_r");
    anim().AddAnim(
        eAnimDie, "stand_idle_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimStandLieDown, "stand_lie_down_", -1, &velocity_none, PS_STAND, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");
    anim().AddAnim(eAnimLieToSleep, "lie_to_sleep_", -1, &velocity_none, PS_LIE, "fx_stand_f", "fx_stand_b",
        "fx_stand_l", "fx_stand_r");

    // anim().AddAnim(eAnimStandIdle,		"stand_idle_",			-1, &velocity_none,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimStandTurnLeft,	"stand_turn_ls_",		-1, &velocity_turn,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimStandTurnRight,	"stand_turn_rs_",		-1, &velocity_turn,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimLieIdle,		"stand_sleep_",			-1, &velocity_none,		PS_LIE,		"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimSitIdle,		"sit_idle_",			-1, &velocity_none,		PS_SIT,		"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimSleep,			"stand_sleep_",			-1, &velocity_none,		PS_LIE,		"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimWalkFwd,		"stand_walk_fwd_",		-1, &velocity_walk,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimWalkDamaged,	"stand_walk_fwd_dmg_",	-1, &velocity_walk_dmg,	PS_STAND,	"fx_stand_f",
    // "fx_stand_b", "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimRun,			"stand_run_fwd_",		-1,	&velocity_run,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimRunDamaged,		"stand_run_dmg_",		-1,	&velocity_run_dmg,	PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimEat,			"stand_eat_",			-1, &velocity_none,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimAttack,			"stand_attack_",		-1, &velocity_turn,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimLookAround,		"stand_idle_",			-1, &velocity_none,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimSteal,			"stand_steal_",			-1, &velocity_steal,	PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimDie,			"stand_idle_",			-1, &velocity_none,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimStandLieDown,	"stand_lie_down_",		-1, &velocity_none,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimLieToSleep,		"lie_to_sleep_",		-1, &velocity_none,		PS_LIE,		"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");
    // anim().AddAnim(eAnimThreaten,		"stand_kick_",			-1, &velocity_none,		PS_STAND,	"fx_stand_f",
    // "fx_stand_b",
    // "fx_stand_l", "fx_stand_r");

    // 	anim().AddAnim(eAnimRunTurnLeft,	"stand_run_left_",		-1, &velocity_run,		PS_STAND);
    // 	anim().AddAnim(eAnimRunTurnRight,	"stand_run_right_",		-1, &velocity_run,		PS_STAND);

    anim().LinkAction(ACT_STAND_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_SIT_IDLE, eAnimSitIdle);
    anim().LinkAction(ACT_LIE_IDLE, eAnimLieIdle);
    anim().LinkAction(ACT_WALK_FWD, eAnimWalkFwd);
    anim().LinkAction(ACT_WALK_BKWD, eAnimWalkFwd);
    anim().LinkAction(ACT_RUN, eAnimRun);
    anim().LinkAction(ACT_EAT, eAnimEat);
    anim().LinkAction(ACT_SLEEP, eAnimSleep);
    anim().LinkAction(ACT_REST, eAnimSleep);
    anim().LinkAction(ACT_DRAG, eAnimWalkFwd);
    anim().LinkAction(ACT_ATTACK, eAnimAttack);
    anim().LinkAction(ACT_STEAL, eAnimSteal);
    anim().LinkAction(ACT_LOOK_AROUND, eAnimStandIdle);

    // define transitions
    anim().AddTransition(eAnimStandLieDown, eAnimSleep, eAnimLieToSleep, false);
    anim().AddTransition(PS_STAND, eAnimSleep, eAnimStandLieDown, true);
    anim().AddTransition(PS_STAND, PS_LIE, eAnimStandLieDown, false);

#ifdef DEBUG
    anim().accel_chain_test();
#endif

    // Load psi postprocess --------------------------------------------------------
    LPCSTR ppi_section = pSettings->r_string(section, "threaten_effector");
    m_threaten_effector.ppi.duality.h = pSettings->r_float(ppi_section, "duality_h");
    m_threaten_effector.ppi.duality.v = pSettings->r_float(ppi_section, "duality_v");
    m_threaten_effector.ppi.gray = pSettings->r_float(ppi_section, "gray");
    m_threaten_effector.ppi.blur = pSettings->r_float(ppi_section, "blur");
    m_threaten_effector.ppi.noise.intensity = pSettings->r_float(ppi_section, "noise_intensity");
    m_threaten_effector.ppi.noise.grain = pSettings->r_float(ppi_section, "noise_grain");
    m_threaten_effector.ppi.noise.fps = pSettings->r_float(ppi_section, "noise_fps");
    VERIFY(!fis_zero(m_threaten_effector.ppi.noise.fps));

    sscanf(pSettings->r_string(ppi_section, "color_base"), "%f,%f,%f", &m_threaten_effector.ppi.color_base.r,
        &m_threaten_effector.ppi.color_base.g, &m_threaten_effector.ppi.color_base.b);
    sscanf(pSettings->r_string(ppi_section, "color_gray"), "%f,%f,%f", &m_threaten_effector.ppi.color_gray.r,
        &m_threaten_effector.ppi.color_gray.g, &m_threaten_effector.ppi.color_gray.b);
    sscanf(pSettings->r_string(ppi_section, "color_add"), "%f,%f,%f", &m_threaten_effector.ppi.color_add.r,
        &m_threaten_effector.ppi.color_add.g, &m_threaten_effector.ppi.color_add.b);

    m_threaten_effector.time = pSettings->r_float(ppi_section, "time");
    m_threaten_effector.time_attack = pSettings->r_float(ppi_section, "time_attack");
    m_threaten_effector.time_release = pSettings->r_float(ppi_section, "time_release");

    m_threaten_effector.ce_time = pSettings->r_float(ppi_section, "ce_time");
    m_threaten_effector.ce_amplitude = pSettings->r_float(ppi_section, "ce_amplitude");
    m_threaten_effector.ce_period_number = pSettings->r_float(ppi_section, "ce_period_number");
    m_threaten_effector.ce_power = pSettings->r_float(ppi_section, "ce_power");

    // --------------------------------------------------------------------------------

    GEnv.Sound->create(
        m_sound_threaten_hit, pSettings->r_string(section, "sound_threaten_hit"), st_Effect, SOUND_TYPE_WORLD);
    GEnv.Sound->create(m_sound_start_threaten, pSettings->r_string(section, "sound_threaten_start"), st_Effect,
        SOUND_TYPE_MONSTER_ATTACKING);

    m_kick_damage = pSettings->r_float(section, "HugeKick_Damage");
    m_kick_particles = pSettings->r_string(section, "HugeKick_Particles");
    read_distance(section, "HugeKick_MinMaxDist", m_threaten_dist_min, m_threaten_dist_max);
    read_delay(section, "HugeKick_MinMaxDelay", m_threaten_delay_min, m_threaten_delay_max);

    m_time_kick_actor_slow_down = pSettings->r_u32(section, "HugeKick_Time_SlowDown");

    PostLoad(section);
}

void CPseudoGigant::reinit()
{
    inherited::reinit();

    m_time_next_threaten = 0;

    if (CCustomMonster::use_simplified_visual())
        return;

    move().load_velocity(*cNameSect(), "Velocity_JumpPrepare", MonsterMovement::eGiantVelocityParameterJumpPrepare);
    move().load_velocity(*cNameSect(), "Velocity_JumpGround", MonsterMovement::eGiantVelocityParameterJumpGround);

    // com_man().load_jump_data(0,"jump_attack_0", "jump_attack_1", "jump_attack_2",
    // MonsterMovement::eGiantVelocityParameterJumpPrepare, MonsterMovement::eGiantVelocityParameterJumpGround,0);
    com_man().add_rotation_jump_data("1", "2", "3", "4", PI_DIV_2);

    com_man().set_threaten_data("stand_kick_0", 0.43f);
}

#define MAX_STEP_RADIUS 60.f

void CPseudoGigant::event_on_step()
{
    //////////////////////////////////////////////////////////////////////////
    // Earthquake Effector	//////////////
    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (pActor)
    {
        float dist_to_actor = pActor->Position().distance_to(Position());
        float max_dist = MAX_STEP_RADIUS;
        if (dist_to_actor < max_dist)
            Actor()->Cameras().AddCamEffector(new CPseudogigantStepEffector(step_effector.time, step_effector.amplitude,
                step_effector.period_number, (max_dist - dist_to_actor) / (1.2f * max_dist)));
    }
    //////////////////////////////////
}

bool CPseudoGigant::check_start_conditions(ControlCom::EControlType type)
{
    if (!inherited::check_start_conditions(type))
        return false;

    if (type == ControlCom::eControlRunAttack)
        return true;

    if (type == ControlCom::eControlThreaten)
    {
        if (m_time_next_threaten > time())
            return false;

        if (!EnemyMan.get_enemy())
            return false;

        // check distance to enemy
        float dist = EnemyMan.get_enemy()->Position().distance_to(Position());

        if ((dist > m_threaten_dist_max) || (dist < m_threaten_dist_min))
            return false;
    }

    return true;
}

void CPseudoGigant::on_activate_control(ControlCom::EControlType type)
{
    if (type == ControlCom::eControlThreaten)
    {
        m_sound_start_threaten.play_at_pos(this, get_head_position(this));
        m_time_next_threaten = time() + Random.randI(m_threaten_delay_min, m_threaten_delay_max);
    }
}

void CPseudoGigant::on_threaten_execute()
{
    // разбросить объекты
    m_nearest.clear();
    Level().ObjectSpace.GetNearest(m_nearest, Position(), 15.f, NULL);
    for (u32 i = 0; i < m_nearest.size(); i++)
    {
        CPhysicsShellHolder* obj = smart_cast<CPhysicsShellHolder*>(m_nearest[i]);
        if (!obj || !obj->m_pPhysicsShell)
            continue;

        Fvector dir;
        Fvector pos;
        pos.set(obj->Position());
        pos.y += 2.f;
        dir.sub(pos, Position());
        dir.normalize();
        obj->m_pPhysicsShell->applyImpulse(dir, 20 * obj->m_pPhysicsShell->getMass());
    }

    // играть звук
    Fvector pos;
    pos.set(Position());
    pos.y += 0.1f;
    m_sound_threaten_hit.play_at_pos(this, pos);

    // играть партиклы
    PlayParticles(m_kick_particles, pos, Direction());

    CActor* pA = const_cast<CActor*>(smart_cast<const CActor*>(EnemyMan.get_enemy()));
    if (!pA)
        return;
    if ((pA->MovingState() & ACTOR_DEFS::mcJump) != 0)
        return;

    float dist_to_enemy = pA->Position().distance_to(Position());
    float hit_value;
    hit_value = m_kick_damage - m_kick_damage * dist_to_enemy / m_threaten_dist_max;
    clamp(hit_value, 0.f, 1.f);

    // запустить эффектор
    Actor()->Cameras().AddCamEffector(
        new CMonsterEffectorHit(m_threaten_effector.ce_time, m_threaten_effector.ce_amplitude * hit_value,
            m_threaten_effector.ce_period_number, m_threaten_effector.ce_power * hit_value));
    Actor()->Cameras().AddPPEffector(new CMonsterEffector(m_threaten_effector.ppi, m_threaten_effector.time,
        m_threaten_effector.time_attack, m_threaten_effector.time_release, hit_value));

    // развернуть камеру
    if (pA->cam_Active())
    {
        pA->cam_Active()->Move(Random.randI(2) ? kRIGHT : kLEFT, Random.randF(0.3f * hit_value));
        pA->cam_Active()->Move(Random.randI(2) ? kUP : kDOWN, Random.randF(0.3f * hit_value));
    }

    Actor()->lock_accel_for(m_time_kick_actor_slow_down);

    // Нанести хит
    NET_Packet l_P;
    SHit HS;

    HS.GenHeader(GE_HIT, pA->ID()); //	u_EventGen	(l_P,GE_HIT, pA->ID());
    HS.whoID = (ID()); //	l_P.w_u16	(ID());
    HS.weaponID = (ID()); //	l_P.w_u16	(ID());
    HS.dir = (Fvector().set(0.f, 1.f, 0.f)); //	l_P.w_dir	(Fvector().set(0.f,1.f,0.f));
    HS.power = (hit_value); //	l_P.w_float	(m_kick_damage);
    HS.boneID = (smart_cast<IKinematics*>(
        pA->Visual())->LL_GetBoneRoot()); //	l_P.w_s16	(smart_cast<IKinematics*>(pA->Visual())->LL_GetBoneRoot());
    HS.p_in_bone_space = (Fvector().set(0.f, 0.f, 0.f)); //	l_P.w_vec3	(Fvector().set(0.f,0.f,0.f));
    HS.impulse = (80 * pA->character_physics_support()->movement()->GetMass()); //	l_P.w_float	(20 *
                                                                                //pA->movement_control()->GetMass());
    HS.hit_type = (ALife::eHitTypeStrike); //	l_P.w_u16	( u16(ALife::eHitTypeWound) );
    HS.Write_Packet(l_P);
    u_EventSend(l_P);
}

void CPseudoGigant::HitEntityInJump(const CEntity* pEntity)
{
    SAAParam& params = anim().AA_GetParams("jump_attack_1");
    HitEntity(pEntity, params.hit_power, params.impulse, params.impulse_dir);
}

void CPseudoGigant::TranslateActionToPathParams()
{
    if ((anim().m_tAction != ACT_RUN) && (anim().m_tAction != ACT_WALK_FWD))
    {
        inherited::TranslateActionToPathParams();
        return;
    }

    u32 vel_mask = (m_bDamaged ? MonsterMovement::eVelocityParamsWalkDamaged : MonsterMovement::eVelocityParamsWalk);
    u32 des_mask =
        (m_bDamaged ? MonsterMovement::eVelocityParameterWalkDamaged : MonsterMovement::eVelocityParameterWalkNormal);

    if (m_force_real_speed)
        vel_mask = des_mask;

    path().set_velocity_mask(vel_mask);
    path().set_desirable_mask(des_mask);
    path().enable_path();
}
