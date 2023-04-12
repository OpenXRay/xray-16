#include "StdAfx.h"
#include "dog.h"
#include "dog_state_manager.h"
#include "ai/monsters/monster_velocity_space.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_movement_base.h"
#include "date_time.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "ai/monsters/monster_home.h"
#include "ai/monsters/ai_monster_squad.h"
#include "ai/monsters/ai_monster_squad_manager.h"
#include "CharacterPhysicsSupport.h"

#include "xrAICore/Navigation/level_graph.h"
#include "ai_space.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "Actor.h"

CAI_Dog::CAI_Dog()
{
    StateMan = xr_new<CStateManagerDog>(this);

    min_move_dist = u32(5);
    max_move_dist = u32(7);
    m_start_smelling = u32(-1);
    m_smelling_count = Random.randI(3);
    CControlled::init_external(this);

    if (!ShadowOfChernobylMode)
        com_man().add_ability(ControlCom::eControlJump);
    else
        com_man().add_ability(ControlCom::eControlMeleeJump);

    com_man().add_ability(ControlCom::eControlRotationJump);
}

CAI_Dog::~CAI_Dog() { xr_delete(StateMan); }
void CAI_Dog::Load(LPCSTR section)
{
    inherited::Load(section);
    m_anim_factor = pSettings->read_if_exists<u32>(section, "anim_factor", 50);

    m_corpse_use_timeout = 1000 * pSettings->read_if_exists<u32>(section, "corpse_use_timeout", 5); // default is 5000 (5 * 1000)
    m_min_sleep_time = 1000 * pSettings->read_if_exists<u32>(section, "min_sleep_time", 5); // default is 5000 (5 * 1000)
    m_min_life_time = 1000 * pSettings->read_if_exists<u32>(section, "min_life_time", 10); // default is 10000 (10 * 1000)
    m_drive_out_time = 1000 * pSettings->read_if_exists<u32>(section, "drive_out_time", 10); // default is 10000 (10 * 1000)

    min_move_dist = pSettings->read_if_exists<u32>(section, "min_move_dist", 5);
    max_move_dist = pSettings->read_if_exists<u32>(section, "max_move_dist", 7);

    if (max_move_dist < min_move_dist)
    {
        min_move_dist = u32(5);
        max_move_dist = u32(7);
    }

    anim().AddReplacedAnim(&m_bDamaged, eAnimRun, eAnimRunDamaged);
    anim().AddReplacedAnim(&m_bDamaged, eAnimWalkFwd, eAnimWalkDamaged);
    anim().AddReplacedAnim(&m_bRunTurnLeft, eAnimRun, eAnimRunTurnLeft);
    anim().AddReplacedAnim(&m_bRunTurnRight, eAnimRun, eAnimRunTurnRight);

    anim().accel_load(section);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRun);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRunTurnLeft);
    anim().accel_chain_add(eAnimWalkFwd, eAnimRunTurnRight);
    anim().accel_chain_add(eAnimWalkDamaged, eAnimRunDamaged);

    SVelocityParam& velocity_none = move().get_velocity(MonsterMovement::eVelocityParameterIdle);
    SVelocityParam& velocity_turn = move().get_velocity(MonsterMovement::eVelocityParameterStand);
    SVelocityParam& velocity_walk = move().get_velocity(MonsterMovement::eVelocityParameterWalkNormal);
    SVelocityParam& velocity_run = move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    SVelocityParam& velocity_walk_dmg = move().get_velocity(MonsterMovement::eVelocityParameterWalkDamaged);
    SVelocityParam& velocity_run_dmg = move().get_velocity(MonsterMovement::eVelocityParameterRunDamaged);
    SVelocityParam& velocity_steal = move().get_velocity(MonsterMovement::eVelocityParameterSteal);
    SVelocityParam& velocity_drag = move().get_velocity(MonsterMovement::eVelocityParameterDrag);
    SVelocityParam& velocity_walk_smell = move().get_velocity(MonsterMovement::eVelocityParameterWalkSmelling);
    SVelocityParam& velocity_walk_growl = move().get_velocity(MonsterMovement::eVelocityParameterWalkGrowl);

    // define animation set
    anim().AddAnim(eAnimStandIdle, "stand_idle_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimStandTurnLeft, "stand_turn_ls_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimStandTurnRight, "stand_turn_rs_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimEat, "stand_eat_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimSleep, "lie_sleep_", -1, &velocity_none, PS_LIE);
    anim().AddAnim(eAnimLieIdle, "lie_idle_", -1, &velocity_none, PS_LIE);
    anim().AddAnim(eAnimSitIdle, "sit_idle_", -1, &velocity_none, PS_SIT);
    anim().AddAnim(eAnimAttack, "stand_attack_", -1, &velocity_turn, PS_STAND);
    anim().AddAnim(eAnimWalkFwd, "stand_walk_fwd_", -1, &velocity_walk, PS_STAND);
    anim().AddAnim(eAnimWalkDamaged, "stand_walk_dmg_", -1, &velocity_walk_dmg, PS_STAND);
    anim().AddAnim(eAnimRun, "stand_run_", -1, &velocity_run, PS_STAND);
    anim().AddAnim(eAnimRunDamaged, "stand_run_dmg_", -1, &velocity_run_dmg, PS_STAND);

    anim().AddAnim(eAnimRunTurnLeft, "stand_run_turn_left_", -1, &velocity_run, PS_STAND);
    anim().AddAnim(eAnimRunTurnRight, "stand_run_turn_right_", -1, &velocity_run, PS_STAND);

    anim().AddAnim(eAnimCheckCorpse, "stand_check_corpse_", -1, &velocity_none, PS_STAND);
    anim().AddAnim2(eAnimDragCorpse, { "stand_drage_", "stand_drag_" }, -1, &velocity_drag, PS_STAND);
    // anim().AddAnim(eAnimSniff,		"stand_sniff_",			-1, &velocity_none,		PS_STAND);
    // anim().AddAnim(eAnimHowling,		"stand_howling_",		-1,	&velocity_none,		PS_STAND);

    // anim().AddAnim(eAnimJumpGlide,   	"jump_glide_",			-1, &velocity_none,		PS_STAND);
    anim().AddAnim(eAnimJumpGlide, "stand_jump_left_", 0, &velocity_none, PS_STAND);

    if (ShadowOfChernobylMode)
        anim().AddAnim(eAnimSteal, "stand_steal_", -1, &velocity_steal, PS_STAND);
    else
        anim().AddAnim(eAnimSteal, "stand_walk_fwd_", -1, &velocity_steal, PS_STAND);

    anim().AddAnim(eAnimThreaten, "stand_threaten_", -1, &velocity_none, PS_STAND);

    anim().AddAnim(eAnimSitLieDown, "sit_lie_down_", -1, &velocity_none, PS_SIT);
    anim().AddAnim(eAnimStandSitDown, "stand_sit_down_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimSitStandUp, "sit_stand_up_", -1, &velocity_none, PS_SIT);
    // anim().AddAnim(eAnimLieToSleep,	"lie_to_sleep_",		-1,	&velocity_none,		PS_LIE);
    anim().AddAnim(eAnimLieSitUp, "lie_to_sit_", -1, &velocity_none, PS_LIE);

    anim().AddAnim(eAnimJumpLeft, "stand_jump_left_", -1, &velocity_none, PS_STAND);
    anim().AddAnim(eAnimJumpRight, "stand_jump_right_", -1, &velocity_none, PS_STAND);

    /////////////mob home

    anim().AddAnim(eAnimHomeWalkSmelling, "stand_walk_fwd_", -1, &velocity_walk_smell, PS_STAND);
    anim().AddAnim(eAnimHomeWalkGrowl, "stand_walk_fwd_", -1, &velocity_walk_growl, PS_STAND);

    /////////////end mob home

    // define transitions
    // order : 1. [anim -> anim]	2. [anim->state]	3. [state -> anim]		4. [state -> state]
    anim().AddTransition(PS_SIT, PS_LIE, eAnimSitLieDown, false);
    anim().AddTransition(PS_STAND, PS_SIT, eAnimStandSitDown, false);
    anim().AddTransition(PS_SIT, PS_STAND, eAnimSitStandUp, false, SKIP_IF_AGGRESSIVE);
    anim().AddTransition(PS_LIE, PS_SIT, eAnimLieSitUp, false, SKIP_IF_AGGRESSIVE);

    // todo: stand -> lie

    // define links from Action to animations
    anim().LinkAction(ACT_STAND_IDLE, eAnimStandIdle);
    anim().LinkAction(ACT_SIT_IDLE, eAnimSitIdle);
    anim().LinkAction(ACT_LIE_IDLE, eAnimLieIdle);
    anim().LinkAction(ACT_WALK_FWD, eAnimWalkFwd);
    anim().LinkAction(ACT_WALK_BKWD, eAnimWalkBkwd);
    anim().LinkAction(ACT_RUN, eAnimRun);
    anim().LinkAction(ACT_EAT, eAnimEat);
    anim().LinkAction(ACT_SLEEP, eAnimSleep);
    anim().LinkAction(ACT_REST, eAnimSitIdle);
    anim().LinkAction(ACT_DRAG, eAnimDragCorpse);
    anim().LinkAction(ACT_ATTACK, eAnimAttack);
    anim().LinkAction(ACT_STEAL, eAnimSteal);
    anim().LinkAction(ACT_LOOK_AROUND, eAnimStandIdle);

    /////////////mob home

    anim().LinkAction(ACT_HOME_WALK_SMELLING, eAnimHomeWalkSmelling);
    anim().LinkAction(ACT_HOME_WALK_GROWL, eAnimHomeWalkGrowl);

/////////////end mob home

#ifdef DEBUG
    anim().accel_chain_test();
#endif
    PostLoad(section);
}

void CAI_Dog::reinit()
{
    inherited::reinit();

    if (CCustomMonster::use_simplified_visual())
        return;

    com_man().add_rotation_jump_data("1", "2", "3", "4", PI_DIV_2);
    com_man().add_rotation_jump_data("5", "6", "7", "8", deg(179));
    if (ShadowOfChernobylMode)
        com_man().add_melee_jump_data("5","jump_right_0");
    // com_man().add_rotation_jump_data("stand_jump_left_0","stand_jump_left_0",
    //	                             "stand_jump_right_0","stand_jump_right_0", deg(179));
    // com_man().add_melee_jump_data("stand_jump_left_0", "stand_jump_right_0");

    b_anim_end = false;
    b_state_anim = false;
    b_state_end = false;
    b_state_check = false;
    b_end_state_eat = false;
    saved_state = u32(-1);
    Home->set_move_dists(min_move_dist, max_move_dist);
}

void CAI_Dog::UpdateCL()
{
    inherited::UpdateCL();

    if (!::detail::object_exists_in_alife_registry(ID()))
    {
        return;
    }

    if (b_anim_end)
    {
        b_anim_end = false;
        StateMan->update();
    }
}

bool CAI_Dog::is_night()
{
    u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
    split_time(Level().GetGameTime(), year, month, day, hours, mins, secs, milisecs);
    if (hours <= 6 || hours >= 21)
    {
        return true;
    }
    return false;
}

void CAI_Dog::CheckSpecParams(u32 spec_params)
{
    if ((spec_params & ASP_CHECK_CORPSE) == ASP_CHECK_CORPSE)
    {
        com_man().seq_run(anim().get_motion_id(eAnimCheckCorpse));
    }

    if ((spec_params & ASP_THREATEN) == ASP_THREATEN)
    {
        anim().SetCurAnim(eAnimThreaten);
    }
    if ((spec_params & ASP_MOVE_SMELLING) == ASP_MOVE_SMELLING)
    {
        anim().SetCurAnim(eAnimHomeWalkGrowl);
    }
}

u32 CAI_Dog::get_number_animation() { return current_anim; }
u32 CAI_Dog::random_anim()
{
    if (m_anim_factor > u32(Random.randI(100)))
    {
        if (is_night())
            return 5;
        return 4;
    }
    return Random.randI(4);
}

void CAI_Dog::set_current_animation(u32 curr_anim)
{
    b_state_check = true;
    b_state_end = false;
    current_anim = curr_anim;
}

bool CAI_Dog::check_start_conditions(ControlCom::EControlType type)
{
    if (type == ControlCom::eControlJump)
    {
        // Lain: if leader or enemy is higher - can jump
        if (const CEntityAlive* enemy = EnemyMan.get_enemy())
        {
            if (can_use_agressive_jump(enemy))
            {
                // true, probably...
                return inherited::check_start_conditions(type);
            }
        }

        if (CMonsterSquad* squad = monster_squad().get_squad(this))
        {
            if (squad->GetLeader() != this)
            {
                return false;
            }
        }
    }
    return inherited::check_start_conditions(type);
}

void CAI_Dog::start_animation()
{
    // Lain: check if animation is captured
    CControl_Com* capturer = control().get_capturer(ControlCom::eControlAnimation);
    if (capturer && capturer->ced() != NULL)
    {
        return;
    }

    b_state_anim = true;
    com_man().script_capture(ControlCom::eControlAnimation);
    smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(get_current_animation(), TRUE, animation_end, this);
    b_state_end = true;
}

void CAI_Dog::animation_end(CBlend* B)
{
    ((CAI_Dog*)B->CallbackParam)->b_state_anim = false;
    ((CAI_Dog*)B->CallbackParam)->b_anim_end = true;
    ((CAI_Dog*)B->CallbackParam)->com_man().script_release(ControlCom::eControlAnimation);
    ((CAI_Dog*)B->CallbackParam)->b_state_end = false;
}

void CAI_Dog::anim_end_reinit()
{
    b_state_anim = false;
    b_anim_end = true;
    com_man().script_release(ControlCom::eControlAnimation);
}

bool CAI_Dog::get_custom_anim_state() { return b_state_anim; }
void CAI_Dog::set_custom_anim_state(bool b_state_animation) { b_state_anim = b_state_animation; }
LPCSTR CAI_Dog::get_current_animation()
{
    switch (current_anim)
    {
    case 1:
        return "stand_check_corpse_0"; //Нюхает вверх
    case 2:
        return "stand_check_corpse_0"; //Нюхает вниз
    case 3:
        return "stand_check_corpse_0"; //Нюхает по кругу
    case 4:
        return "stand_check_corpse_0"; //Обнюховает и роет землю
    case 5:
        return "stand_threaten_0"; //Воет
    case 6:
        return "stand_threaten_0"; //Рычит стоя
    case 7:
        return "stand_idle_1"; //Отряхивается !!!!!
    case 8:
        return "stand_sit_down_0"; //Садиться
    case 9:
        return "sit_idle_0"; // Cидит
    case 10:
        return "sit_idle_0"; //Чухается сидя
    case 11:
        return "sit_idle_0"; //Оглядывается сидя
    case 12:
        return "sit_stand_up_0"; //Встает
    case 13:
        return "sit_lie_down_0"; //Ложится
    case 14:
        return "lie_to_sit_0"; //Подымается
    case 15:
        return "stand_eat_0"; //Отрывает куски
    case 16:
        return "stand_threaten_0"; //Лает
    default:
        return "stand_idle_1"; //Нюхает вперед
    }
}

void CAI_Dog::reload(LPCSTR section)
{
    inherited::reload(section);
    if (!ShadowOfChernobylMode)
    {
        com_man().load_jump_data(0, "jump_ataka_01", "jump_ataka_02", "jump_ataka_03",
            MonsterMovement::eVelocityParameterRunNormal, MonsterMovement::eVelocityParameterRunNormal, 0);
    }
}

void CAI_Dog::HitEntityInJump(const CEntity* pEntity)
{
    SAAParam& params = anim().AA_GetParams("jump_ataka_02");

    HitEntity(pEntity, params.hit_power, params.impulse, params.impulse_dir);
}

// Lain: added
u32 CAI_Dog::get_attack_rebuild_time()
{
    float dist = EnemyMan.get_enemy()->Position().distance_to(Position());
    return 100 + u32(25 * dist);
}

bool CAI_Dog::can_use_agressive_jump(const IGameObject* enemy)
{
    float delta_y = 0.8f;
    if (enemy == Actor())
    {
        if (Actor()->is_jump())
        {
            delta_y += 0.8f;
        }
    }

    return enemy->Position().y - Position().y > delta_y;
}

#ifdef _DEBUG
void CAI_Dog::debug_on_key(int key)
{
    IKinematicsAnimated* skel = smart_cast<IKinematicsAnimated*>(Visual());

    switch (key)
    {
    case SDL_SCANCODE_1:
        Msg("Ohhhhhhhhhhhhhhh! Here it is!");
        // strafe left
        // com_man().seq_run(skel->ID_Cycle_Safe("stand_turn_ls_0"));
        break;
    case SDL_SCANCODE_2:
        // strafe right
        com_man().seq_run(skel->ID_Cycle_Safe("stand_turn_ls_0"));
        break;
    case SDL_SCANCODE_3:
        // threaten
        com_man().seq_run(skel->ID_Cycle_Safe("stand_threaten_0"));
        break;
    case SDL_SCANCODE_0: Msg("Ohhhhhhhhhhhhhhh! Here it is!"); break;
    }
}
#endif
