#include "pch_script.h"
#include "base_monster.h"
#include "Level.h"
#include "level_debug.h"
#include "EntityCondition.h"
#include "ai_debug.h"
#include "ai/monsters/state_defs.h"
#include "ai/monsters/state_manager.h"
#include "PHMovementControl.h"
#include "CharacterPhysicsSupport.h"
#include "Actor.h"
// Lain: added
#include "debug_text_tree.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "sound_memory_manager.h"
#include "hit_memory_manager.h"
#include "actor_memory.h"
#include "Inventory.h"
#include "Weapon.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "movement_manager_space.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/monster_state_manager.h"
#include "sound_player.h"
#include "game_path_manager.h"
#include "detail_path_manager.h"
#include "patrol_path_manager.h"
#include "script_entity_action.h"
#include "ai/monsters/ai_monster_squad.h"
#include "ai/monsters/ai_monster_squad_manager.h"
#include "ai/monsters/monster_home.h"

#ifdef DEBUG
CBaseMonster::SDebugInfo CBaseMonster::show_debug_info()
{
    if (!g_Alive())
        return SDebugInfo();

    if (m_show_debug_info == 0)
    {
        DBG().text(this).clear();
        return SDebugInfo();
    }

    float y = 200;
    float x = (m_show_debug_info == 1) ? 40.f : float(GEnv.Render->getTarget()->get_width() / 2) + 40.f;
    const float delta_y = 12;

    string256 text;

    u32 color = color_xrgb(0, 255, 0);
    u32 delimiter_color = color_xrgb(0, 0, 255);

    DBG().text(this).clear();
    DBG().text(this).add_item("---------------------------------------", x, y += delta_y, delimiter_color);

    xr_sprintf(text, "-- Monster : [%s]  Current Time = [%u]", *cName(), Device.dwTimeGlobal);
    DBG().text(this).add_item(text, x, y += delta_y, color);
    DBG().text(this).add_item("-----------   PROPERTIES   ------------", x, y += delta_y, delimiter_color);

    xr_sprintf(text, "Health = [%f]", conditions().GetHealth());
    DBG().text(this).add_item(text, x, y += delta_y, color);

    xr_sprintf(text, "Morale = [%f]", Morale.get_morale());
    DBG().text(this).add_item(text, x, y += delta_y, color);

    DBG().text(this).add_item("-----------   MEMORY   ----------------", x, y += delta_y, delimiter_color);

    if (EnemyMan.get_enemy())
    {
        xr_sprintf(text, "Current Enemy = [%s]", *EnemyMan.get_enemy()->cName());
    }
    else
        xr_sprintf(text, "Current Enemy = [NONE]");
    DBG().text(this).add_item(text, x, y += delta_y, color);

    if (EnemyMan.get_enemy())
    {
        xr_sprintf(text, "SeeEnemy[%u] EnemySeeMe[%u] TimeLastSeen[%u]", EnemyMan.see_enemy_now(),
            EnemyMan.enemy_see_me_now(), EnemyMan.get_enemy_time_last_seen());
        DBG().text(this).add_item(text, x, y += delta_y, color);
    }

    if (CorpseMan.get_corpse())
    {
        xr_sprintf(text, "Current Corpse = [%s] Satiety = [%.2f]", *CorpseMan.get_corpse()->cName(), GetSatiety());
    }
    else
        xr_sprintf(text, "Current Corpse = [NONE] Satiety = [%.2f]", GetSatiety());

    DBG().text(this).add_item(text, x, y += delta_y, color);

    // Sound
    if (SoundMemory.IsRememberSound())
    {
        SoundElem sound_elem;
        bool dangerous_sound;
        SoundMemory.GetSound(sound_elem, dangerous_sound);

        string128 s_type;

        switch (sound_elem.type)
        {
        case WEAPON_SHOOTING: xr_strcpy(s_type, "WEAPON_SHOOTING"); break;
        case MONSTER_ATTACKING: xr_strcpy(s_type, "MONSTER_ATTACKING"); break;
        case WEAPON_BULLET_RICOCHET: xr_strcpy(s_type, "WEAPON_BULLET_RICOCHET"); break;
        case WEAPON_RECHARGING: xr_strcpy(s_type, "WEAPON_RECHARGING"); break;

        case WEAPON_TAKING: xr_strcpy(s_type, "WEAPON_TAKING"); break;
        case WEAPON_HIDING: xr_strcpy(s_type, "WEAPON_HIDING"); break;
        case WEAPON_CHANGING: xr_strcpy(s_type, "WEAPON_CHANGING"); break;
        case WEAPON_EMPTY_CLICKING: xr_strcpy(s_type, "WEAPON_EMPTY_CLICKING"); break;

        case MONSTER_DYING: xr_strcpy(s_type, "MONSTER_DYING"); break;
        case MONSTER_INJURING: xr_strcpy(s_type, "MONSTER_INJURING"); break;
        case MONSTER_WALKING: xr_strcpy(s_type, "MONSTER_WALKING"); break;
        case MONSTER_JUMPING: xr_strcpy(s_type, "MONSTER_JUMPING"); break;
        case MONSTER_FALLING: xr_strcpy(s_type, "MONSTER_FALLING"); break;
        case MONSTER_TALKING: xr_strcpy(s_type, "MONSTER_TALKING"); break;

        case DOOR_OPENING: xr_strcpy(s_type, "DOOR_OPENING"); break;
        case DOOR_CLOSING: xr_strcpy(s_type, "DOOR_CLOSING"); break;
        case OBJECT_BREAKING: xr_strcpy(s_type, "OBJECT_BREAKING"); break;
        case OBJECT_FALLING: xr_strcpy(s_type, "OBJECT_FALLING"); break;
        case NONE_DANGEROUS_SOUND: xr_strcpy(s_type, "NONE_DANGEROUS_SOUND"); break;
        }

        if (sound_elem.who)
            xr_sprintf(text, "Sound: type[%s] time[%u] power[%.3f] val[%i] src[+]", s_type, sound_elem.time,
                sound_elem.power, sound_elem.value);
        else
            xr_sprintf(text, "Sound: type[%s] time[%u] power[%.3f] val[%i] src[?]", s_type, sound_elem.time,
                sound_elem.power, sound_elem.value);
    }
    else
        xr_sprintf(text, "Sound: NONE");

    DBG().text(this).add_item(text, x, y += delta_y, color);

    // Hit
    if (HitMemory.is_hit())
    {
        if (HitMemory.get_last_hit_object())
        {
            xr_sprintf(text, "Hit Info: object=[%s] time=[%u]", *(HitMemory.get_last_hit_object()->cName()),
                HitMemory.get_last_hit_time());
        }
        else
        {
            xr_sprintf(text, "Hit Info: object=[NONE] time=[%u]", HitMemory.get_last_hit_time());
        }
    }
    else
        xr_sprintf(text, "Hit Info: NONE");

    DBG().text(this).add_item(text, x, y += delta_y, color);

    DBG().text(this).add_item("-----------   MOVEMENT   ------------", x, y += delta_y, delimiter_color);

    xr_sprintf(
        text, "Actual = [%u] Enabled = [%u]", control().path_builder().actual(), control().path_builder().enabled());
    DBG().text(this).add_item(text, x, y += delta_y, color);

    xr_sprintf(text, "Speed: Linear = [%.3f] Angular = [%.3f]", control().movement().velocity_current(), 0.f);
    DBG().text(this).add_item(text, x, y += delta_y, color);

    DBG().text(this).add_item("------- Attack Distances -------------", x, y += delta_y, delimiter_color);
    xr_sprintf(text, "MinDist[%.3f] MaxDist[%.3f] As_Step[%.3f] As_MinDist[%.3f]", MeleeChecker.get_min_distance(),
        MeleeChecker.get_max_distance(), MeleeChecker.dbg_as_step(), MeleeChecker.dbg_as_min_dist());
    DBG().text(this).add_item(text, x, y += delta_y, color);

    if (EnemyMan.get_enemy())
    {
        xr_sprintf(text, "Current Enemy = [%s]", *EnemyMan.get_enemy()->cName());
    }
    else
        xr_sprintf(text, "Current Enemy = [NONE]");
    DBG().text(this).add_item(text, x, y += delta_y, color);

    return SDebugInfo(x, y, delta_y, color, delimiter_color);
}

void CBaseMonster::debug_fsm()
{
    if (!g_Alive())
        return;

    if (!psAI_Flags.test(aiMonsterDebug))
    {
        DBG().object_info(this, this).clear();
        return;
    }

    EMonsterState state = StateMan->get_state_type();

    string128 st;

    switch (state)
    {
    case eStateRest_WalkGraphPoint: xr_sprintf(st, "Rest :: Walk Graph"); break;
    case eStateRest_Idle: xr_sprintf(st, "Rest :: Idle"); break;
    case eStateRest_Fun: xr_sprintf(st, "Rest :: Fun"); break;
    case eStateRest_Sleep: xr_sprintf(st, "Rest :: Sleep"); break;
    case eStateRest_MoveToHomePoint: xr_sprintf(st, "Rest :: MoveToHomePoint"); break;
    case eStateRest_WalkToCover: xr_sprintf(st, "Rest :: WalkToCover"); break;
    case eStateRest_LookOpenPlace: xr_sprintf(st, "Rest :: LookOpenPlace"); break;

    case eStateEat_CorpseApproachRun: xr_sprintf(st, "Eat :: Corpse Approach Run"); break;
    case eStateEat_CorpseApproachWalk: xr_sprintf(st, "Eat :: Corpse Approach Walk"); break;
    case eStateEat_CheckCorpse: xr_sprintf(st, "Eat :: Check Corpse"); break;
    case eStateEat_Eat: xr_sprintf(st, "Eat :: Eating"); break;
    case eStateEat_WalkAway: xr_sprintf(st, "Eat :: Walk Away"); break;
    case eStateEat_Rest: xr_sprintf(st, "Eat :: Rest After Meal"); break;
    case eStateEat_Drag: xr_sprintf(st, "Eat :: Drag"); break;

    case eStateAttack_Run: xr_sprintf(st, "Attack :: Run"); break;
    case eStateAttack_Melee: xr_sprintf(st, "Attack :: Melee"); break;
    case eStateAttack_RunAttack: xr_sprintf(st, "Attack :: Run Attack"); break;
    case eStateAttack_RunAway: xr_sprintf(st, "Attack :: Run Away"); break;
    case eStateAttack_FindEnemy: xr_sprintf(st, "Attack :: Find Enemy"); break;
    case eStateAttack_Steal: xr_sprintf(st, "Attack :: Steal"); break;
    case eStateAttack_AttackHidden: xr_sprintf(st, "Attack :: Attack Hidden"); break;

    case eStateAttackCamp_Hide: xr_sprintf(st, "Attack Camp:: Hide"); break;
    case eStateAttackCamp_Camp: xr_sprintf(st, "Attack Camp:: Camp"); break;
    case eStateAttackCamp_StealOut: xr_sprintf(st, "Attack Camp:: Steal Out"); break;

    case eStateAttack_HideInCover: xr_sprintf(st, "Attack :: Hide In Cover"); break;
    case eStateAttack_MoveOut: xr_sprintf(st, "Attack :: Move Out From Cover"); break;
    case eStateAttack_CampInCover: xr_sprintf(st, "Attack :: Camp In Cover"); break;

    case eStateAttack_Psy: xr_sprintf(st, "Attack :: Psy"); break;
    case eStateAttack_MoveToHomePoint: xr_sprintf(st, "Attack :: Move To Home Point"); break;
    case eStateAttack_HomePoint_Hide: xr_sprintf(st, "Attack :: Home Point :: Hide"); break;
    case eStateAttack_HomePoint_Camp: xr_sprintf(st, "Attack :: Home Point :: Camp"); break;
    case eStateAttack_HomePoint_LookOpenPlace: xr_sprintf(st, "Attack :: Home Point :: Look Open Place"); break;

    case eStatePanic_Run: xr_sprintf(st, "Panic :: Run Away"); break;
    case eStatePanic_FaceUnprotectedArea: xr_sprintf(st, "Panic :: Face Unprotected Area"); break;
    case eStatePanic_HomePoint_Hide: xr_sprintf(st, "Panic :: Home Point :: Hide"); break;
    case eStatePanic_HomePoint_LookOpenPlace: xr_sprintf(st, "Panic :: Home Point :: Look Open Place"); break;
    case eStatePanic_HomePoint_Camp: xr_sprintf(st, "Panic :: Home Point :: Camp"); break;

    case eStateHitted_Hide: xr_sprintf(st, "Hitted :: Hide"); break;
    case eStateHitted_MoveOut: xr_sprintf(st, "Hitted :: MoveOut"); break;
    case eStateHitted_Home: xr_sprintf(st, "Hitted :: Home"); break;

    case eStateHearDangerousSound_Hide: xr_sprintf(st, "Dangerous Snd :: Hide"); break;
    case eStateHearDangerousSound_FaceOpenPlace: xr_sprintf(st, "Dangerous Snd :: FaceOpenPlace"); break;
    case eStateHearDangerousSound_StandScared: xr_sprintf(st, "Dangerous Snd :: StandScared"); break;
    case eStateHearDangerousSound_Home: xr_sprintf(st, "Dangerous Snd :: Home"); break;

    case eStateHearInterestingSound_MoveToDest: xr_sprintf(st, "Interesting Snd :: MoveToDest"); break;
    case eStateHearInterestingSound_LookAround: xr_sprintf(st, "Interesting Snd :: LookAround"); break;

    case eStateHearHelpSound: xr_sprintf(st, "Hear Help Sound"); break;
    case eStateHearHelpSound_MoveToDest: xr_sprintf(st, "Hear Help Sound :: MoveToDest"); break;
    case eStateHearHelpSound_LookAround: xr_sprintf(st, "Hear Help Sound :: LookAround"); break;

    case eStateControlled_Follow_Wait: xr_sprintf(st, "Controlled :: Follow : Wait"); break;
    case eStateControlled_Follow_WalkToObject: xr_sprintf(st, "Controlled :: Follow : WalkToObject"); break;
    case eStateControlled_Attack: xr_sprintf(st, "Controlled :: Attack"); break;
    case eStateThreaten: xr_sprintf(st, "Threaten :: "); break;
    case eStateFindEnemy_Run: xr_sprintf(st, "Find Enemy :: Run"); break;
    case eStateFindEnemy_LookAround_MoveToPoint: xr_sprintf(st, "Find Enemy :: Look Around : Move To Point"); break;
    case eStateFindEnemy_LookAround_LookAround: xr_sprintf(st, "Find Enemy :: Look Around : Look Around"); break;
    case eStateFindEnemy_LookAround_TurnToPoint: xr_sprintf(st, "Find Enemy :: Look Around : Turn To Point"); break;
    case eStateFindEnemy_Angry: xr_sprintf(st, "Find Enemy :: Angry"); break;
    case eStateFindEnemy_WalkAround: xr_sprintf(st, "Find Enemy :: Walk Around"); break;
    case eStateSquad_Rest_Idle: xr_sprintf(st, "Squad :: Rest : Idle"); break;
    case eStateSquad_Rest_WalkAroundLeader: xr_sprintf(st, "Squad :: Rest : WalkAroundLeader"); break;
    case eStateSquad_RestFollow_Idle: xr_sprintf(st, "Squad :: Follow Leader : Idle"); break;
    case eStateSquad_RestFollow_WalkToPoint: xr_sprintf(st, "Squad :: Follow Leader : WalkToPoint"); break;
    case eStateCustom_Vampire: xr_sprintf(st, "Attack :: Vampire"); break;
    case eStateVampire_ApproachEnemy: xr_sprintf(st, "Vampire :: Approach to enemy"); break;
    case eStateVampire_Execute: xr_sprintf(st, "Vampire :: Hit"); break;
    case eStateVampire_RunAway: xr_sprintf(st, "Vampire :: Run Away"); break;
    case eStateVampire_Hide: xr_sprintf(st, "Vampire :: Hide"); break;
    case eStatePredator: xr_sprintf(st, "Predator"); break;
    case eStatePredator_MoveToCover: xr_sprintf(st, "Predator :: MoveToCover"); break;
    case eStatePredator_LookOpenPlace: xr_sprintf(st, "Predator :: Look Open Place"); break;
    case eStatePredator_Camp: xr_sprintf(st, "Predator :: Camp"); break;
    case eStateBurerAttack_Tele: xr_sprintf(st, "Attack :: Telekinesis"); break;
    case eStateBurerAttack_Gravi: xr_sprintf(st, "Attack :: Gravi Wave"); break;
    case eStateBurerAttack_RunAround: xr_sprintf(st, "Attack :: Run Around"); break;
    case eStateBurerAttack_FaceEnemy: xr_sprintf(st, "Attack :: Face Enemy"); break;
    case eStateBurerAttack_Melee: xr_sprintf(st, "Attack :: Melee"); break;
    case eStateBurerAttack_AntiAim: xr_sprintf(st, "Attack :: AntiAim"); break;
    case eStateBurerAttack_Shield: xr_sprintf(st, "Attack :: Shield"); break;
    case eStateBurerScanning: xr_sprintf(st, "Attack :: Scanning"); break;
    case eStateCustomMoveToRestrictor: xr_sprintf(st, "Moving To Restrictor :: Position not accessible"); break;
    case eStateSmartTerrainTask: xr_sprintf(st, "ALIFE"); break;
    case eStateSmartTerrainTaskGamePathWalk: xr_sprintf(st, "ALIFE :: Game Path Walk"); break;
    case eStateSmartTerrainTaskLevelPathWalk: xr_sprintf(st, "ALIFE :: Level Path Walk"); break;
    case eStateSmartTerrainTaskWaitCapture: xr_sprintf(st, "ALIFE :: Wait till smart terrain will capture me"); break;
    case eStateUnknown: xr_sprintf(st, "Unknown State :: "); break;
    default: xr_sprintf(st, "Undefined State ::"); break;
    }

    DBG().object_info(this, this).remove_item(u32(0));
    DBG().object_info(this, this).remove_item(u32(1));
    DBG().object_info(this, this).remove_item(u32(2));

    DBG().object_info(this, this).add_item(*cName(), color_xrgb(255, 0, 0), 0);
    DBG().object_info(this, this).add_item(st, color_xrgb(255, 0, 0), 1);

    xr_sprintf(st, "Team[%u]Squad[%u]Group[%u]", g_Team(), g_Squad(), g_Group());
    DBG().object_info(this, this).add_item(st, color_xrgb(255, 0, 0), 2);

    CEntityAlive* entity = smart_cast<CEntityAlive*>(Level().CurrentEntity());
    if (entity && entity->character_physics_support()->movement())
    {
        xr_sprintf(st, "VELOCITY [%f,%f,%f] Value[%f]",
            VPUSH(entity->character_physics_support()->movement()->GetVelocity()),
            entity->character_physics_support()->movement()->GetVelocityActual());
        DBG().text(this).clear();
        DBG().text(this).add_item(st, 200, 100, COLOR_GREEN, 100);
    }
}

// Lain: added
// defined in stalker_debug.cpp
extern CActor* g_debug_actor;

xr_string make_xrstr(TSoundDangerValue value)
{
    switch (value)
    {
    case WEAPON_SHOOTING: return "WEAPON_SHOOTING";
    case MONSTER_ATTACKING: return "MONSTER_ATTACKING";
    case WEAPON_BULLET_RICOCHET: return "WEAPON_BULLET_RICOCHET";
    case WEAPON_RECHARGING: return "WEAPON_RECHARGING";

    case WEAPON_TAKING: return "WEAPON_TAKING";
    case WEAPON_HIDING: return "WEAPON_HIDING";
    case WEAPON_CHANGING: return "WEAPON_CHANGING";
    case WEAPON_EMPTY_CLICKING: return "WEAPON_EMPTY_CLICKING";

    case MONSTER_DYING: return "MONSTER_DYING";
    case MONSTER_INJURING: return "MONSTER_INJURING";
    case MONSTER_WALKING: return "MONSTER_WALKING";
    case MONSTER_JUMPING: return "MONSTER_JUMPING";
    case MONSTER_FALLING: return "MONSTER_FALLING";
    case MONSTER_TALKING: return "MONSTER_TALKING";

    case DOOR_OPENING: return "DOOR_OPENING";
    case DOOR_CLOSING: return "DOOR_CLOSING";
    case OBJECT_BREAKING: return "OBJECT_BREAKING";
    case OBJECT_FALLING: return "OBJECT_FALLING";
    case NONE_DANGEROUS_SOUND: return "NONE_DANGEROUS_SOUND";
    default: return "";
    }
}

xr_string make_xrstr(EMemberGoalType value)
{
    switch (value)
    {
    case MG_AttackEnemy: return "MG_Attack_Enemy";
    case MG_PanicFromEnemy: return "MG_Panic_FromEnemy";
    case MG_InterestingSound: return "MG_Interesting_Sound";
    case MG_DangerousSound: return "MG_Dangerous_Sound";
    case MG_WalkGraph: return "MG_Walk_Graph";
    case MG_Rest: return "MG_Rest";
    case MG_None: return "MG_None";
    default: return "unknown";
    }
}

xr_string make_xrstr(ESquadCommandType value)
{
    switch (value)
    {
    case SC_EXPLORE: return "SC_EXPLORE";
    case SC_ATTACK: return "SC_ATTACK";
    case SC_THREATEN: return "SC_THREATEN";
    case SC_COVER: return "SC_COVER";
    case SC_FOLLOW: return "SC_FOLLOW";
    case SC_FEEL_DANGER: return "SC_FEEL_DANGER";
    case SC_EXPLICIT_ACTION: return "SC_EXPLICIT_ACTION";
    case SC_REST: return "SC_REST";
    case SC_NONE: return "SC_NONE";
    default: return "Unknown";
    }
}

namespace detail
{
void add_debug_info(debug::text_tree& root_s, const CEntity* pE)
{
    if (pE)
    {
        root_s.add_line(*pE->cName());
        root_s.add_line("ID", pE->ID());
    }
    else
    {
        root_s.add_text("-");
    }
}

void add_debug_info(debug::text_tree& root_s, SoundElem& sound_elem, bool dangerous)
{
    root_s.add_line("Type", sound_elem.type);
    root_s.add_line("Time", sound_elem.time);
    root_s.add_line("Power", make_xrstr("%.3f", sound_elem.power));
    root_s.add_line("Val", sound_elem.value);

    debug::text_tree& src_s = root_s.add_line("Src");
    add_debug_info(src_s, smart_cast<const CEntity*>(sound_elem.who));

    root_s.add_line("Dangerous", dangerous);
}

void add_debug_info_restrictions(debug::text_tree& root_s, const xr_string& restr)
{
    size_t cur_i = 0;

    do
    {
        size_t pos = restr.find(',', cur_i);
        if (pos == xr_string::npos)
        {
            pos = restr.size() - 1;
        }

        if (cur_i < pos)
        {
            root_s.add_line(restr.substr(cur_i, pos - cur_i));
        }
        cur_i = pos + 1;

    } while (cur_i < restr.size());
}

void add_enemy_debug_info(debug::text_tree& root_s, const CCustomMonster* pThis, const CEntityAlive* pEnemy)
{
    add_debug_info(root_s, pEnemy);

    root_s.add_line("I_See_Enemy", pThis->memory().visual().visible_right_now(pEnemy));

    bool seen_now = false;
    if (Actor() == pEnemy)
    {
        seen_now = Actor()->memory().visual().visible_right_now(pThis);
    }
    else if (CCustomMonster* cm = const_cast<CEntityAlive*>(pEnemy)->cast_custom_monster())
    {
        seen_now = cm->memory().visual().visible_right_now(pThis);
    }

    root_s.add_line("Enemy_See_Me", seen_now);
}

void add_debug_info(debug::text_tree& root_s, CScriptEntityAction* p_action)
{
    if (!p_action)
    {
        root_s.add_text("-");
        return;
    }

    typedef debug::text_tree TextTree;

    root_s.add_line("Time_Over", p_action->CheckIfTimeOver());
    root_s.add_line("Action_Completed", p_action->CheckIfActionCompleted());
    root_s.add_line("Watch_Completed", p_action->CheckIfWatchCompleted());
    root_s.add_line("Animation_Completed", p_action->CheckIfAnimationCompleted());
    root_s.add_line("Sound_Completed", p_action->CheckIfSoundCompleted());
    root_s.add_line("Particle_Completed", p_action->CheckIfParticleCompleted());
    root_s.add_line("Sound_Completed", p_action->CheckIfSoundCompleted());
    root_s.add_line("Object_Completed", p_action->CheckIfObjectCompleted());
    root_s.add_line("Monster_Action_Completed", p_action->CheckIfMonsterActionCompleted());
    root_s.add_line("Object_Completed", p_action->CheckIfObjectCompleted());

    TextTree& movement_action_s = root_s.add_line("Movement_Completed", p_action->CheckIfMovementCompleted());

    CScriptMovementAction& move_action = const_cast<CScriptMovementAction&>(p_action->move());
    CScriptActionCondition const& action_condition = p_action->cond();

    if (action_condition.m_dwFlags & CScriptActionCondition::MOVEMENT_FLAG)
    {
        pcstr const path_name = move_action.m_path_name.c_str();

        movement_action_s.add_line("Path_Name", path_name ? path_name : "-");
        movement_action_s.add_line("Move_Action", (int)move_action.m_tMoveAction);
        movement_action_s.add_line("Dist_To_End", move_action.m_fDistToEnd);
        movement_action_s.add_line("Speed_Param", (int)move_action.m_tSpeedParam);
        movement_action_s.add_line("Prev_Patrol_Point", move_action.m_previous_patrol_point);
        movement_action_s.add_line("Speed", move_action.m_fSpeed);
        movement_action_s.add_line("Goal_Type", move_action.m_tGoalType);
        movement_action_s.add_line("Node_ID", move_action.m_tNodeID);
        movement_action_s.add_line("Patrol_Path_Start", (int)move_action.m_tPatrolPathStart);
        movement_action_s.add_line("Patrol_Path_Stop", (int)move_action.m_tPatrolPathStop);
        movement_action_s.add_line("Path_Type", (int)move_action.m_tPathType);
        movement_action_s.add_line("Body_State", (int)move_action.m_tBodyState);
        movement_action_s.add_line("Movement_Type", (int)move_action.m_tMovementType);
    }
}

void add_debug_info(debug::text_tree& root_s, CBlend* p_blend)
{
    root_s.add_line("Blend_Amount", p_blend->blendAmount);
    root_s.add_line("Time_Current", p_blend->timeCurrent);
    root_s.add_line("Time_Total", p_blend->timeTotal);
    root_s.add_line("Blend_Type", p_blend->blend_state());
    root_s.add_line("Blend_Accrue", p_blend->blendAccrue);
    root_s.add_line("Blend_Falloff", p_blend->blendFalloff);
    root_s.add_line("Blend_Power", p_blend->blendPower);
    root_s.add_line("Speed", p_blend->speed);
}

void add_debug_info(debug::text_tree& root_s, const SRotation& rot)
{
    root_s.add_line("yaw", rot.yaw);
    root_s.add_line("pitch", rot.pitch);
    root_s.add_line("roll", rot.roll);
}

} // namespace detail

void CBaseMonster::add_debug_info(debug::text_tree& root_s)
{
    if (!g_Alive())
    {
        return;
    }

    typedef debug::text_tree TextTree;

    //-----------------------------------------------
    // General
    //-----------------------------------------------
    TextTree& general_s = root_s.find_or_add("General");

    detail::add_debug_info(general_s, this);
    TextTree& current_visual_s = general_s.add_line("Current_Visual");
    current_visual_s.add_line(*cNameVisual());

    general_s.add_line("Health", conditions().GetHealth());
    general_s.add_line("Morale", Morale.get_morale());
    general_s.add_line("Angry", m_bAngry);
    general_s.add_line("Growling", m_bGrowling);
    general_s.add_line("Aggressive", m_bAggressive);
    general_s.add_line("Sleep", m_bSleep);

    TextTree& perceptors_s = general_s.find_or_add("Perceptors");
    TextTree& visuals_s = perceptors_s.find_or_add("Visual");

    float object_range, object_fov;
    update_range_fov(object_range, object_fov, eye_range, deg2rad(eye_fov));
    visuals_s.add_line("Eye_Range", object_range);
    visuals_s.add_line("FOV", rad2deg(object_fov));

    CActor* actor = smart_cast<CActor*>(Level().Objects.net_Find(0));
    if (!actor)
    {
        actor = g_debug_actor;
    }

    if (actor)
    {
        visuals_s.add_line("Actor_Visible", memory().visual().visible_now(actor));
    }

    //-----------------------------------------------
    // Sounds
    //-----------------------------------------------
    TextTree& sounds_s = perceptors_s.find_or_add("Sounds");
    sounds_s.add_line("Num_Sounds", SoundMemory.GetNumSounds());

    if (SoundMemory.IsRememberSound())
    {
        TextTree& last_s = sounds_s.add_line("Last");

        SoundElem last_sound;
        bool last_dangerous;
        SoundMemory.GetSound(last_sound, last_dangerous);
        detail::add_debug_info(last_s, last_sound, last_dangerous);

        if (SoundMemory.GetNumSounds() > 1)
        {
            SoundElem first_sound;
            bool first_dangerous;
            SoundMemory.GetFirstSound(first_sound, first_dangerous);

            TextTree& first_s = sounds_s.add_line("First");
            detail::add_debug_info(first_s, first_sound, first_dangerous);
        }
    }
    else
    {
        sounds_s.add_text("no");
    }

    //-----------------------------------------------
    // Hits
    //-----------------------------------------------
    TextTree& hit_s = perceptors_s.add_line("Hits", HitMemory.get_num_hits());

    // Hit
    if (HitMemory.is_hit())
    {
        TextTree& last_hit_object_s = hit_s.add_line("Object");
        detail::add_debug_info(last_hit_object_s, smart_cast<CEntity*>(HitMemory.get_last_hit_object()));
        hit_s.add_line("Time", HitMemory.get_last_hit_time());
        hit_s.add_line("Pos", HitMemory.get_last_hit_position());
        hit_s.add_line("Dir", HitMemory.get_last_hit_dir());
    }

    //-----------------------------------------------
    // Corpses
    //-----------------------------------------------
    TextTree& corpse_s = general_s.find_or_add("Corpse_Man");

    corpse_s.add_line("Current_Corpse", CorpseMan.get_corpse() ? *CorpseMan.get_corpse()->cName() : "none");
    corpse_s.add_line("Satiety", make_xrstr("%.2f", GetSatiety()));

    //-----------------------------------------------
    // Group behavious
    //-----------------------------------------------
    TextTree& group_s = general_s.find_or_add("Group_Behaviour");
    group_s.add_line("Team", g_Team());

    TextTree& squad_s = group_s.add_line("Squad", g_Squad());

    CMonsterSquad* squad = monster_squad().get_squad(this);
    if (squad)
    {
        squad_s.add_line("SquadActive", squad->SquadActive());
        squad_s.add_line("Im_Leader", squad->GetLeader() == this);
        detail::add_debug_info(squad_s.add_line("Leader"), squad->GetLeader());

        int num_alive = squad->squad_alife_count();
        if (!num_alive && g_Alive())
        {
            num_alive++;
        }
        squad_s.add_line("Alive_Count", num_alive);

        TextTree& squad_command_s = squad_s.add_line("My_Squad_Command");
        squad_command_s.add_line("Command_Type", squad->GetCommand(this).type);

        TextTree& squad_goal_s = squad_s.add_line("My_Squad_Goal");
        squad_goal_s.add_line("Goal_Type", squad->GetGoal(this).type);
        detail::add_debug_info(squad_goal_s.add_line("Goal_Entity"), squad->GetGoal(this).entity);
    }

    group_s.add_line("Group", g_Group());

    //-----------------------------------------------
    // Brain (Fsm & Script)
    //-----------------------------------------------
    TextTree& brain_s = root_s.find_or_add("Brain");

    TextTree& fsm_s = brain_s.find_or_add("Fsm");
    StateMan->add_debug_info(fsm_s);

    TextTree& script_control_s = brain_s.add_line("Script_Control_Name");
    if (!m_bScriptControl)
    {
        script_control_s.add_text("-");
    }
    else
    {
        script_control_s.add_text(GetScriptControlName());

        TextTree& cur_script_action_s = brain_s.add_line("Current_Script_Action");
        if (m_tpCurrentEntityAction)
        {
            detail::add_debug_info(cur_script_action_s, m_tpCurrentEntityAction);
        }
        else
        {
            cur_script_action_s.add_text("-");
        }

        TextTree& next_script_action_s = brain_s.add_line("Next_Script_Action");
        if (m_tpActionQueue.size())
        {
            detail::add_debug_info(next_script_action_s, m_tpActionQueue.front());
        }
        else
        {
            next_script_action_s.add_text("-");
        }
    }

    //-----------------------------------------------
    // Control Manager
    //-----------------------------------------------
    control().add_debug_info(brain_s.add_line("Control_Manager"));

    TextTree& map_home_s = brain_s.add_line("Map_Home");
    map_home_s.add_line("min", Home->get_min_radius());
    map_home_s.add_line("mid", Home->get_mid_radius());
    map_home_s.add_line("max", Home->get_max_radius());

    if (EnemyMan.get_enemy())
    {
        map_home_s.add_line("Enemy_At_Min", Home->at_min_home(EnemyMan.get_enemy()->Position()));
        map_home_s.add_line("Enemy_At_Mid", Home->at_mid_home(EnemyMan.get_enemy()->Position()));
        map_home_s.add_line("Enemy_At_Max", Home->at_home(EnemyMan.get_enemy()->Position()));

        map_home_s.add_line("Dist_To_Enemy", Position().distance_to(EnemyMan.get_enemy()->Position()));
    }

    //-----------------------------------------------
    // Enemies
    //-----------------------------------------------
    TextTree& enemies_s = general_s.find_or_add("Enemies");
    enemies_s.add_text(EnemyMemory.get_enemies_count());

    if (actor)
    {
        enemies_s.add_line("Actor_Is_Enemy", EnemyMan.is_enemy(actor));
    }

    TextTree& current_enemy_s = enemies_s.find_or_add("Current_Enemy");
    if (EnemyMan.get_enemy())
    {
        detail::add_enemy_debug_info(current_enemy_s, this, EnemyMan.get_enemy());
        current_enemy_s.add_line("Time_Last_Seen", EnemyMan.get_enemy_time_last_seen());
        current_enemy_s.add_line("See_Duration", EnemyMan.see_enemy_duration());
    }
    else
    {
        current_enemy_s.add_text("0");
    }

    int index = 1;
    for (ENEMIES_MAP::const_iterator i = EnemyMemory.get_memory().begin(), e = EnemyMemory.get_memory().end(); i != e;
         ++i)
    {
        const CEntityAlive* p_enemy = (*i).first;
        if (p_enemy != EnemyMan.get_enemy())
        {
            TextTree& enemy_s = enemies_s.add_line(make_xrstr("Enemy %i", index++));
            detail::add_enemy_debug_info(enemy_s, this, p_enemy);
        }
    }

    //-----------------------------------------------
    // Animations
    //-----------------------------------------------
    TextTree& controller_s = root_s.find_or_add("Controllers");
    TextTree& animation_s = controller_s.find_or_add("Animations");

    TextTree& current_animation_s = animation_s.add_line(*anim().cur_anim_info().name);

    CBlend* p_blend = control().animation().current_blend();
    if (!p_blend)
    {
        p_blend = anim().cur_anim_info().blend;
    }

    if (p_blend)
    {
        detail::add_debug_info(current_animation_s, p_blend);
        current_animation_s.add_line("Script_Animation?", p_blend->motionID == m_tpScriptAnimation);
    }
    else
    {
        current_animation_s.add_text("0");
    }

    //-----------------------------------------------
    // Movement
    //-----------------------------------------------
    TextTree& movement_s = controller_s.find_or_add("Movement");
    movement_s.add_line("Actual", control().path_builder().actual());
    movement_s.add_line("Enabled", control().path_builder().enabled());

    CEntityAlive* entity = smart_cast<CEntityAlive*>(Level().CurrentEntity());
    if (entity && entity->character_physics_support()->movement())
    {
        movement_s.add_line("Velocity", entity->character_physics_support()->movement()->GetVelocityActual());
    }
    movement_s.add_line("Position").add_line(Position());

    movement_s.add_line("Level_Vertex_ID", ai_location().level_vertex_id());
    movement_s.add_line("Game_Vertex_ID", ai_location().game_vertex_id());

    detail::add_debug_info(movement_s.add_line("Orientation_Current"), movement().body_orientation().current);
    detail::add_debug_info(movement_s.add_line("Orientation_Target"), movement().body_orientation().target);
    movement_s.add_line("Rotation_Speed", movement().body_orientation().speed);

    const char* pc_path_type = "undefined";
    switch (movement().path_type())
    {
    case MovementManager::ePathTypePatrolPath: pc_path_type = "Patrol_Path"; break;
    case MovementManager::ePathTypeGamePath: pc_path_type = "Game_Path"; break;
    case MovementManager::ePathTypeLevelPath: pc_path_type = "Level_Path"; break;
    }

    movement_s.add_line("Path_Type", pc_path_type);
    if (movement().path_type() == MovementManager::ePathTypePatrolPath)
    {
        movement_s.add_line("Path_Name", *movement().patrol().path_name());
        movement_s.add_line("Completed", movement().patrol().completed());

        movement_s.add_line("Current_Point", movement().patrol().get_current_point_index());
        if (movement().patrol().get_path() &&
            movement().patrol().get_path()->vertex(movement().patrol().get_current_point_index()))
        {
            movement_s.add_line("Extrapolate", movement().patrol().extrapolate_path());
        }
        else
        {
            movement_s.add_line("Extrapolate", "unknown");
        }
    }

    if (movement().path_type() == MovementManager::ePathTypeGamePath)
    {
        movement_s.add_line("Completed", movement().game_path().completed());
        movement_s.add_line("Path_Size", movement().game_path().path().size());
        movement_s.add_line("Current_Point", movement().game_path().intermediate_index());
    }

    TextTree& level_s = movement_s.add_line("Level");

    level_s.add_line("Path_Size", movement().level_path().path().size());
    level_s.add_line(
        "Start_Vertex", movement().level_path().path().empty() ? -1 : movement().level_path().path().front());
    level_s.add_line("End_Vertex", movement().level_path().path().empty() ? -1 : movement().level_path().path().back());

    if (!movement().detail().path().empty())
    {
        TextTree& detail_s = movement_s.add_line("Detail");

        detail_s.add_line("Velocities", movement().detail().velocities().size());
        detail_s.add_line("Extrapolate", movement().detail().extrapolate_length());
        detail_s.add_line("Path_Size", movement().detail().path().size());

        detail_s.add_line("Start_Point").add_line(movement().detail().path().front().position);
        detail_s.add_line("Dest_Point").add_line(movement().detail().path().back().position);
        TextTree& current_point_s = detail_s.add_line("Current_Point");
        current_point_s.add_line("Index", movement().detail().curr_travel_point_index());
        current_point_s.add_line("Position")
            .add_line(movement().detail().path()[movement().detail().curr_travel_point_index()].position);

        CDetailPathManager::STravelParams current_velocity = movement().detail().velocity(
            movement().detail().path()[movement().detail().curr_travel_point_index()].velocity);
        detail_s.add_line("linear", current_velocity.linear_velocity);
        detail_s.add_line("angular", rad2deg(current_velocity.real_angular_velocity));
        detail_s.add_line("speed(calc)", movement().speed());
        detail_s.add_line("speed(physics)", movement().speed(character_physics_support()->movement()));
    }

    if (movement().detail().use_dest_orientation())
    {
        movement_s.add_line("Orientation", movement().detail().dest_direction());
    }
    else
    {
        movement_s.add_line("Orientation", "no");
    }

    TextTree& atackdist_s = controller_s.find_or_add("Attack_Distance");
    atackdist_s.add_line("Mind_Dist", make_xrstr("%.3f", MeleeChecker.get_min_distance()));
    atackdist_s.add_line("Max_Dist", make_xrstr("%.3f", MeleeChecker.get_max_distance()));
    atackdist_s.add_line("As_Step", make_xrstr("%.3f", MeleeChecker.dbg_as_step()));
    atackdist_s.add_line("As_MinDist", make_xrstr("%.3f", MeleeChecker.dbg_as_min_dist()));

    TextTree& restrictions_s = movement_s.add_line("Restrictions");

    if (movement().restrictions().out_restrictions().size() || movement().restrictions().in_restrictions().size() ||
        movement().restrictions().base_out_restrictions().size() ||
        movement().restrictions().base_in_restrictions().size())
    {
        detail::add_debug_info_restrictions(
            restrictions_s.add_line("out"), *movement().restrictions().out_restrictions());
        detail::add_debug_info_restrictions(
            restrictions_s.add_line("in"), *movement().restrictions().in_restrictions());
        detail::add_debug_info_restrictions(
            restrictions_s.add_line("base_out"), *movement().restrictions().base_out_restrictions());
        detail::add_debug_info_restrictions(
            restrictions_s.add_line("base_in"), *movement().restrictions().base_in_restrictions());

        restrictions_s.add_line(
            "Actor_Accessible?", actor ? movement().restrictions().accessible(actor->Position()) : false);
    }
    else
    {
        restrictions_s.add_text("-");
    }

    //-----------------------------------------------
    // Sound Player
    //-----------------------------------------------
    TextTree& sound_player_s = controller_s.find_or_add("Sound_Player");
    sound_player_s.add_line("Num_Sounds", sound().objects().size());

    typedef CSoundPlayer::SOUND_COLLECTIONS::const_iterator SoundIterator;

    u32 object_count = 0;
    for (SoundIterator i = sound().objects().begin(), e = sound().objects().end(); i != e; ++i)
    {
        object_count += (*i).second.second->m_sounds.size();
    }

    TextTree& now_playing_s = sound_player_s.add_line("Objects", object_count);

    typedef xr_vector<CSoundPlayer::CSoundSingle>::const_iterator SoundSingleIterator;

    index = 1;
    for (SoundSingleIterator i = sound().playing_sounds().begin(), e = sound().playing_sounds().end(); i != e; ++i)
    {
        xr_string source = (*i).m_sound->_handle() ? (*i).m_sound->_handle()->file_name() : "no source";

        xr_string status = "not yet started";
        if (Device.dwTimeGlobal >= (*i).m_start_time)
        {
            status = (*i).m_sound->_feedback() ? "playing" : "already played";
        }

        TextTree& current_sound_s = now_playing_s.add_line(make_xrstr("Sound %i", index++));
        current_sound_s.add_line(source);
        current_sound_s.add_line(status);
    }
}

#endif
