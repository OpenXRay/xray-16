////////////////////////////////////////////////////////////////////////////
//	Module 		: script_entity.cpp
//	Created 	: 06.10.2003
//  Modified 	: 14.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Script entity class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_entity.h"
#include "CustomMonster.h"
#include "xrEngine/Feel_Vision.h"
#include "xrCore/Animation/Motion.hpp"
#include "Include/xrRender/Kinematics.h"
#include "script_entity_action.h"
#include "Weapon.h"
#include "ParticlesObject.h"
#include "script_game_object.h"
#include "xrScriptEngine/script_engine.hpp"
#include "movement_manager_space.h"
#include "detail_path_manager.h"
#include "patrol_path_manager.h"
#include "level_path_manager.h"
#include "level_location_selector.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "movement_manager.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "game_object_space.h"

void __stdcall ActionCallback(IKinematics* tpKinematics);

CScriptEntity::CScriptEntity()
{
    m_initialized = false;
    m_use_animation_movement_controller = false;
}

CScriptEntity::~CScriptEntity() { ResetScriptData(); }
void CScriptEntity::init()
{
    m_current_sound = 0;
    ResetScriptData();
}

IFactoryObject* CScriptEntity::_construct()
{
    m_object = smart_cast<CGameObject*>(this);
    VERIFY(m_object);

    m_monster = smart_cast<CCustomMonster*>(this);

    init();

    return (m_object);
}

void CScriptEntity::ResetScriptData(void* pointer)
{
    ClearActionQueue();

    m_caScriptName = "";
    m_bScriptControl = false;
    m_use_animation_movement_controller = false;
}

void CScriptEntity::ClearActionQueue()
{
    if (!m_tpActionQueue.empty())
        vfFinishAction(m_tpActionQueue.front());

    while (!m_tpActionQueue.empty())
    {
        xr_delete(m_tpActionQueue.front());
        m_tpActionQueue.erase(m_tpActionQueue.begin());
    }

    m_tpScriptAnimation.invalidate();
    m_tpCurrentEntityAction = 0;
    m_tpNextAnimation.invalidate();
    m_use_animation_movement_controller = false;
}

void CScriptEntity::reinit()
{
    ResetScriptData();

    set_script_capture();
}

void CScriptEntity::SetScriptControl(const bool bScriptControl, shared_str caSciptName)
{
    if (!(((m_bScriptControl && !bScriptControl) || (!m_bScriptControl && bScriptControl)) &&
            (bScriptControl || (xr_strlen(*m_caScriptName) && !xr_strcmp(caSciptName, m_caScriptName)))))
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "Invalid sequence of taking an entity under script control");
        return;
    }

    if (bScriptControl && !can_script_capture())
        return;

    if (bScriptControl && !m_bScriptControl)
        object().add_visual_callback(&ActionCallback);
    else if (!bScriptControl && m_bScriptControl)
        object().remove_visual_callback(&ActionCallback);

    m_bScriptControl = bScriptControl;
    m_caScriptName = caSciptName;
    /*
    #ifdef DEBUG
        if (bScriptControl)
            GEnv.ScriptEngine->script_log			(ScriptStorage::eLuaMessageTypeInfo,"Script %s set object %s
    under its control",*caSciptName,*object().cName());
        else
            GEnv.ScriptEngine->script_log			(ScriptStorage::eLuaMessageTypeInfo,"Script %s freed object %s
    from its control",*caSciptName,*object().cName());
    #endif
    */
    if (!bScriptControl)
        ResetScriptData(this);
}

bool CScriptEntity::GetScriptControl() const { return (m_bScriptControl); }
LPCSTR CScriptEntity::GetScriptControlName() const { return (*m_caScriptName); }
bool CScriptEntity::CheckObjectVisibility(const CGameObject* tpObject)
{
    if (!m_monster)
        return (false);

    return (m_monster->memory().visual().visible_now(tpObject));
}

//определяет видимость определенного типа объектов,
//заданного через section_name
bool CScriptEntity::CheckTypeVisibility(const char* section_name)
{
    if (!m_monster)
        return (false);

    CVisualMemoryManager::VISIBLES::const_iterator I = m_monster->memory().visual().objects().begin();
    CVisualMemoryManager::VISIBLES::const_iterator E = m_monster->memory().visual().objects().end();
    for (; I != E; ++I)
    {
        VERIFY((*I).m_object);
        if (!xr_strcmp(section_name, *(*I).m_object->cNameSect()))
            return (true);
    }
    return (false);
}

void CScriptEntity::AddAction(const CScriptEntityAction* tpEntityAction, bool bHighPriority)
{
    bool empty = m_tpActionQueue.empty();
    if (!bHighPriority || m_tpActionQueue.empty())
        m_tpActionQueue.push_back(new CScriptEntityAction(*tpEntityAction));
    else
    {
        VERIFY(m_tpActionQueue.front());
        CScriptEntityAction* l_tpEntityAction = new CScriptEntityAction(*m_tpActionQueue.front());
        vfFinishAction(m_tpActionQueue.front());
        xr_delete(m_tpActionQueue.front());
        m_tpActionQueue.front() = l_tpEntityAction;
        m_tpActionQueue.insert(m_tpActionQueue.begin(), new CScriptEntityAction(*tpEntityAction));
    }

    if (empty && m_initialized)
        ProcessScripts();
}

CScriptEntityAction* CScriptEntity::GetCurrentAction()
{
    if (m_tpActionQueue.empty())
        return (0);
    else
        return (m_tpActionQueue.front());
}

void __stdcall ActionCallback(IKinematics* tpKinematics)
{
    // sounds
    CScriptEntity* l_tpScriptMonster =
        smart_cast<CScriptEntity*>((CGameObject*)(tpKinematics->GetUpdateCallbackParam()));
    VERIFY(l_tpScriptMonster);
    if (!l_tpScriptMonster->GetCurrentAction())
        return;
    l_tpScriptMonster->vfUpdateSounds();
    l_tpScriptMonster->vfUpdateParticles();
}

void CScriptEntity::vfUpdateParticles()
{
    CScriptParticleAction& l_tParticleAction = GetCurrentAction()->m_tParticleAction;
    if (xr_strlen(l_tParticleAction.m_caBoneName))
    {
        CParticlesObject* l_tpParticlesObject = l_tParticleAction.m_tpParticleSystem;
        l_tpParticlesObject->UpdateParent(
            GetUpdatedMatrix(l_tParticleAction.m_caBoneName, l_tParticleAction.m_tParticlePosition,
                l_tParticleAction.m_tParticleAngles),
            l_tParticleAction.m_tParticleVelocity);
    }
}

void CScriptEntity::vfUpdateSounds()
{
    CScriptSoundAction& l_tSoundAction = GetCurrentAction()->m_tSoundAction;
    if (xr_strlen(l_tSoundAction.m_caBoneName) && m_current_sound && m_current_sound->_feedback())
        m_current_sound->_feedback()->set_position(
            GetUpdatedMatrix(l_tSoundAction.m_caBoneName, l_tSoundAction.m_tSoundPosition, Fvector().set(0, 0, 0)).c);
}

void CScriptEntity::vfFinishAction(CScriptEntityAction* tpEntityAction)
{
    if (m_current_sound)
    {
        m_current_sound->destroy();
        xr_delete(m_current_sound);
    }
    if (!tpEntityAction->m_tParticleAction.m_bAutoRemove)
        CParticlesObject::Destroy(tpEntityAction->m_tParticleAction.m_tpParticleSystem);
}

void CScriptEntity::ProcessScripts()
{
    CScriptEntityAction* l_tpEntityAction = 0;
#ifdef DEBUG
    bool empty_queue = m_tpActionQueue.empty();
#endif
    while (!m_tpActionQueue.empty())
    {
        l_tpEntityAction = m_tpActionQueue.front();
        VERIFY(l_tpEntityAction);
#ifdef _DEBUG
//		if (!xr_strcmp("m_stalker_wounded",*object().cName()))
//			Msg			("%6d Processing action :
//%s",Device.dwTimeGlobal,*l_tpEntityAction->m_tAnimationAction.m_caAnimationToPlay);
#endif

        if (m_tpCurrentEntityAction != l_tpEntityAction)
            l_tpEntityAction->initialize();

        m_tpCurrentEntityAction = l_tpEntityAction;

        if (!l_tpEntityAction->CheckIfActionCompleted())
            break;

#ifdef _DEBUG
//		if (!xr_strcmp("m_stalker_wounded",*object().cName()))
//			Msg			("%6d Action completed :
//%s",Device.dwTimeGlobal,*l_tpEntityAction->m_tAnimationAction.m_caAnimationToPlay);
#endif

        vfFinishAction(l_tpEntityAction);

#ifdef DEBUG
        if (g_LuaDebug.test(1))
            Msg("Entity Action removed!!!");
#endif
        if (true /*g_LuaDebug.test(1)*/)
        {
            object().callback(GameObject::eActionTypeRemoved)(object().lua_game_object(), u32(eActionTypeRemoved));
        }

        xr_delete(l_tpEntityAction);
        m_tpActionQueue.erase(m_tpActionQueue.begin());
    }

    if (m_tpActionQueue.empty())
    {
#ifdef DEBUG
        if (empty_queue)
            GEnv.ScriptEngine->script_log(
                LuaMessageType::Info, "Object %s has an empty script queue!", *object().cName());
#endif
        return;
    }

    try
    {
        bool l_bCompleted;
        l_bCompleted = l_tpEntityAction->m_tWatchAction.m_bCompleted;
        bfAssignWatch(l_tpEntityAction);
        if (l_tpEntityAction->m_tWatchAction.m_bCompleted && !l_bCompleted)
            object().callback(GameObject::eActionTypeWatch)(object().lua_game_object(), u32(eActionTypeWatch));

        l_bCompleted = l_tpEntityAction->m_tAnimationAction.m_bCompleted;
        bfAssignAnimation(l_tpEntityAction);

        l_bCompleted = l_tpEntityAction->m_tSoundAction.m_bCompleted;
        bfAssignSound(l_tpEntityAction);
        if (l_tpEntityAction->m_tSoundAction.m_bCompleted && !l_bCompleted)
            object().callback(GameObject::eActionTypeSound)(object().lua_game_object(), u32(eActionTypeSound));

        l_bCompleted = l_tpEntityAction->m_tParticleAction.m_bCompleted;
        bfAssignParticles(l_tpEntityAction);
        if (l_tpEntityAction->m_tParticleAction.m_bCompleted && !l_bCompleted)
            object().callback(GameObject::eActionTypeParticle)(object().lua_game_object(), u32(eActionTypeParticle));

        l_bCompleted = l_tpEntityAction->m_tObjectAction.m_bCompleted;
        bfAssignObject(l_tpEntityAction);
        if (l_tpEntityAction->m_tObjectAction.m_bCompleted && !l_bCompleted)
            object().callback(GameObject::eActionTypeObject)(object().lua_game_object(), u32(eActionTypeObject));

        l_bCompleted = l_tpEntityAction->m_tMovementAction.m_bCompleted;
        bfAssignMovement(l_tpEntityAction);
        if (l_tpEntityAction->m_tMovementAction.m_bCompleted && !l_bCompleted)
            object().callback(GameObject::eActionTypeMovement)(
                object().lua_game_object(), u32(eActionTypeMovement), -1);

        // Установить выбранную анимацию
        if (!l_tpEntityAction->m_tAnimationAction.m_bCompleted)
            bfScriptAnimation();

        bfAssignMonsterAction(l_tpEntityAction);
    }
    catch (...)
    {
        ResetScriptData();
    }
}

bool CScriptEntity::bfAssignWatch(CScriptEntityAction* tpEntityAction)
{
    return (GetCurrentAction() && !GetCurrentAction()->m_tWatchAction.m_bCompleted);
}

bool CScriptEntity::bfAssignMonsterAction(CScriptEntityAction* tpEntityAction)
{
    if (GetCurrentAction() && GetCurrentAction()->m_tMonsterAction.m_bCompleted)
        return (false);

    return (true);
}

bool CScriptEntity::bfAssignAnimation(CScriptEntityAction* tpEntityAction)
{
    m_tpNextAnimation.invalidate();

    if (GetCurrentAction() && GetCurrentAction()->m_tAnimationAction.m_bCompleted)
        return (false);

    if (!xr_strlen(GetCurrentAction()->m_tAnimationAction.m_caAnimationToPlay))
        return (true);

    IKinematicsAnimated& tVisualObject = *(smart_cast<IKinematicsAnimated*>(object().Visual()));
    m_tpNextAnimation = tVisualObject.ID_Cycle_Safe(*GetCurrentAction()->m_tAnimationAction.m_caAnimationToPlay);
    m_use_animation_movement_controller = GetCurrentAction()->m_tAnimationAction.m_use_animation_movement_controller;
    return (true);
}

const Fmatrix CScriptEntity::GetUpdatedMatrix(
    shared_str caBoneName, const Fvector& tPositionOffset, const Fvector& tAngleOffset)
{
    Fmatrix l_tMatrix;

    l_tMatrix.setHPB(VPUSH(tAngleOffset));
    l_tMatrix.c = tPositionOffset;

    if (xr_strlen(caBoneName))
    {
        CBoneInstance& l_tBoneInstance = smart_cast<IKinematics*>(
            object().Visual())->LL_GetBoneInstance(smart_cast<IKinematics*>(object().Visual())->LL_BoneID(caBoneName));
        l_tMatrix.mulA_43(l_tBoneInstance.mTransform);
        l_tMatrix.mulA_43(object().XFORM());
    }

    return (l_tMatrix);
}

bool CScriptEntity::bfAssignSound(CScriptEntityAction* tpEntityAction)
{
    CScriptSoundAction& l_tSoundAction = tpEntityAction->m_tSoundAction;
    if (l_tSoundAction.m_bCompleted)
        return (false);

    if (m_current_sound)
    {
        if (!m_current_sound->_feedback())
            if (!l_tSoundAction.m_bStartedToPlay)
            {
#ifdef _DEBUG
//				Msg									("%6d Starting sound
//%s",Device.dwTimeGlobal,*l_tSoundAction.m_caSoundToPlay);
#endif
                const Fmatrix& l_tMatrix = GetUpdatedMatrix(
                    l_tSoundAction.m_caBoneName, l_tSoundAction.m_tSoundPosition, l_tSoundAction.m_tSoundAngles);
                m_current_sound->play_at_pos(m_object, l_tMatrix.c, l_tSoundAction.m_bLooped ? sm_Looped : 0);
                l_tSoundAction.m_bStartedToPlay = true;
            }
            else
            {
                l_tSoundAction.m_bCompleted = true;
            }
    }
    else
    {
        if (xr_strlen(l_tSoundAction.m_caSoundToPlay))
        {
            m_current_sound = new ref_sound();
            m_current_sound->create(*l_tSoundAction.m_caSoundToPlay, st_Effect, l_tSoundAction.m_sound_type);
        }
        else
            l_tSoundAction.m_bCompleted = true;
    }
    return (!l_tSoundAction.m_bCompleted);
}

bool CScriptEntity::bfAssignParticles(CScriptEntityAction* tpEntityAction)
{
    CScriptParticleAction& l_tParticleAction = tpEntityAction->m_tParticleAction;
    if (l_tParticleAction.m_bCompleted)
        return (false);
    if (l_tParticleAction.m_tpParticleSystem)
    {
        if (true /** !l_tParticleAction.m_tpParticleSystem/**/)
            if (!l_tParticleAction.m_bStartedToPlay)
            {
                const Fmatrix& l_tMatrix = GetUpdatedMatrix(*l_tParticleAction.m_caBoneName,
                    l_tParticleAction.m_tParticlePosition, l_tParticleAction.m_tParticleAngles);
                Fvector zero_vel = {0.f, 0.f, 0.f};
                l_tParticleAction.m_tpParticleSystem->UpdateParent(l_tMatrix, zero_vel);
                l_tParticleAction.m_tpParticleSystem->play_at_pos(l_tMatrix.c);
                l_tParticleAction.m_bStartedToPlay = true;
            }
            else
            {
                l_tParticleAction.m_bCompleted = true;
            }
    }
    else
        l_tParticleAction.m_bCompleted = true;

    return (!l_tParticleAction.m_bCompleted);
}

bool CScriptEntity::bfAssignObject(CScriptEntityAction* tpEntityAction)
{
    return (GetCurrentAction() && !GetCurrentAction()->m_tObjectAction.m_bCompleted);
}

bool CScriptEntity::bfAssignMovement(CScriptEntityAction* tpEntityAction)
{
    CScriptMovementAction& l_tMovementAction = tpEntityAction->m_tMovementAction;

    if (l_tMovementAction.m_bCompleted)
        return (false);

    CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(this);
    if (entity_alive && !entity_alive->g_Alive())
    {
        l_tMovementAction.m_bCompleted = true;
        return (false);
    }

    if (!m_monster)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Cannot assign a movement action not to a monster!");
        return (true);
    }

    switch (l_tMovementAction.m_tGoalType)
    {
    case CScriptMovementAction::eGoalTypeObject:
    {
        CGameObject* l_tpGameObject = smart_cast<CGameObject*>(l_tMovementAction.m_tpObjectToGo);
#ifdef DEBUG
        THROW2(l_tpGameObject, "eGoalTypeObject specified, but no object passed!");
#else
        R_ASSERT(l_tpGameObject);
#endif
        m_monster->movement().set_path_type(MovementManager::ePathTypeLevelPath);
        //			Msg			("%6d Object %s, position
        //[%f][%f][%f]",Device.dwTimeGlobal,*l_tpGameObject->cName(),VPUSH(l_tpGameObject->Position()));
        m_monster->movement().detail().set_dest_position(l_tpGameObject->Position());
        m_monster->movement().set_level_dest_vertex(l_tpGameObject->ai_location().level_vertex_id());
        break;
    }
    case CScriptMovementAction::eGoalTypePatrolPath:
    {
        m_monster->movement().set_path_type(MovementManager::ePathTypePatrolPath);
        m_monster->movement().patrol().set_path(l_tMovementAction.m_path, l_tMovementAction.m_path_name);
        m_monster->movement().patrol().set_start_type(l_tMovementAction.m_tPatrolPathStart);
        m_monster->movement().patrol().set_route_type(l_tMovementAction.m_tPatrolPathStop);
        m_monster->movement().patrol().set_random(l_tMovementAction.m_bRandom);
        if (l_tMovementAction.m_previous_patrol_point != u32(-1))
        {
            m_monster->movement().patrol().set_previous_point(l_tMovementAction.m_previous_patrol_point);
        }
        break;
    }
    case CScriptMovementAction::eGoalTypeFollowLeader:
    case CScriptMovementAction::eGoalTypePathPosition:
    {
        m_monster->movement().set_path_type(MovementManager::ePathTypeLevelPath);
        m_monster->movement().detail().set_dest_position(l_tMovementAction.m_tDestinationPosition);

        u32 vertex_id;
        vertex_id = ai().level_graph().vertex(
            object().ai_location().level_vertex_id(), l_tMovementAction.m_tDestinationPosition);
        if (!ai().level_graph().valid_vertex_id(vertex_id))
            vertex_id = ai().level_graph().check_position_in_direction(object().ai_location().level_vertex_id(),
                object().Position(), l_tMovementAction.m_tDestinationPosition);

#ifdef DEBUG
        if (!ai().level_graph().valid_vertex_id(vertex_id))
        {
            string256 S;
            xr_sprintf(S,
                "Cannot find corresponding level vertex for the specified position [%f][%f][%f] for monster %s",
                VPUSH(l_tMovementAction.m_tDestinationPosition), *m_monster->cName());
            THROW2(ai().level_graph().valid_vertex_id(vertex_id), S);
        }
#endif
        m_monster->movement().level_path().set_dest_vertex(vertex_id);
        break;
    }
    case CScriptMovementAction::eGoalTypePathNodePosition:
    {
        VERIFY(ai().level_graph().valid_vertex_id(l_tMovementAction.m_tNodeID));
        m_monster->movement().set_path_type(MovementManager::ePathTypeLevelPath);
        m_monster->movement().detail().set_dest_position(l_tMovementAction.m_tDestinationPosition);
        m_monster->movement().level_path().set_dest_vertex(l_tMovementAction.m_tNodeID);
        break;
    }
    case CScriptMovementAction::eGoalTypeNoPathPosition:
    {
        m_monster->movement().set_path_type(MovementManager::ePathTypeLevelPath);
        if (m_monster->movement().detail().path().empty() ||
            (m_monster->movement()
                    .detail()
                    .path()[m_monster->movement().detail().path().size() - 1]
                    .position.distance_to(l_tMovementAction.m_tDestinationPosition) > .1f))
        {
            m_monster->movement().detail().m_path.resize(2);
            m_monster->movement().detail().m_path[0].position = object().Position();
            m_monster->movement().detail().m_path[1].position = l_tMovementAction.m_tDestinationPosition;
            m_monster->movement().detail().m_current_travel_point = 0;
        }

        if (m_monster->movement().detail().m_path[1].position.similar(object().Position(), .2f))
            l_tMovementAction.m_bCompleted = true;

        break;
    }
    default:
    {
        m_monster->movement().set_desirable_speed(0.f);
        return (l_tMovementAction.m_bCompleted = true);
    }
    }

    if (m_monster->movement().actual_all() && m_monster->movement().path_completed())
        l_tMovementAction.m_bCompleted = true;

    return (!l_tMovementAction.m_bCompleted);
}

void CScriptEntity::net_Destroy() { m_initialized = false; }
LPCSTR CScriptEntity::GetPatrolPathName()
{
#ifdef DEBUG
    if (!GetScriptControl())
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "Object %s is not under script control while you are trying to get patrol path name!", *m_object->cName());
        return "";
    }
#endif
    if (m_tpActionQueue.empty())
        return ("");
    return (*m_tpActionQueue.back()->m_tMovementAction.m_path_name);
}

BOOL CScriptEntity::net_Spawn(CSE_Abstract* DC)
{
    m_initialized = true;
    object().setVisible(TRUE);
    object().setEnabled(TRUE);

    return (TRUE);
}

void CScriptEntity::shedule_Update(u32 DT)
{
    if (m_bScriptControl)
        ProcessScripts();
}

void ScriptCallBack(CBlend* B)
{
    CScriptEntity* l_tpScriptMonster = static_cast<CScriptEntity*>(B->CallbackParam);
    VERIFY(l_tpScriptMonster);
    if (l_tpScriptMonster->GetCurrentAction() && !B->bone_or_part)
    {
        if (!l_tpScriptMonster->GetCurrentAction()->m_tAnimationAction.m_bCompleted)
            l_tpScriptMonster->object().callback(GameObject::eActionTypeAnimation)(
                l_tpScriptMonster->object().lua_game_object(), u32(eActionTypeAnimation));

        l_tpScriptMonster->m_tpScriptAnimation.invalidate();
        l_tpScriptMonster->GetCurrentAction()->m_tAnimationAction.m_bCompleted = true;
        if (l_tpScriptMonster->GetActionCount())
            l_tpScriptMonster->ProcessScripts();
    }
}

bool CScriptEntity::bfScriptAnimation()
{
    if (GetScriptControl() && !GetCurrentAction() && GetActionCount())
        ProcessScripts();

    if (GetScriptControl() && GetCurrentAction() && !GetCurrentAction()->m_tAnimationAction.m_bCompleted &&
        xr_strlen(GetCurrentAction()->m_tAnimationAction.m_caAnimationToPlay))
    {
        if (m_tpScriptAnimation == m_tpNextAnimation)
            return (true);

#ifdef DEBUG
// if (!xr_strcmp("m_stalker_wounded",*object().cName()))
//	Msg				("%6d Playing animation : %s , Object
//%s",Device.dwTimeGlobal,*GetCurrentAction()->m_tAnimationAction.m_caAnimationToPlay, *object().cName());
#endif
        m_tpScriptAnimation = m_tpNextAnimation;
        IKinematicsAnimated* skeleton_animated = smart_cast<IKinematicsAnimated*>(object().Visual());
        LPCSTR animation_id = *GetCurrentAction()->m_tAnimationAction.m_caAnimationToPlay;
        MotionID animation = skeleton_animated->ID_Cycle(animation_id);
        CBlend* result = 0;
        for (u16 i = 0; i < MAX_PARTS; ++i)
        {
            CBlend* blend = 0;
            if (result)
            {
                skeleton_animated->LL_PlayCycle(i, animation, TRUE, 0, 0);
                continue;
            }

            blend = skeleton_animated->LL_PlayCycle(i, animation, TRUE, ScriptCallBack, this);
            if (!blend)
                continue;
            result = blend;
            CMotionDef* MD = skeleton_animated->LL_GetMotionDef(animation);
            VERIFY(MD);
            if (m_use_animation_movement_controller)
                m_object->create_anim_mov_ctrl(blend, 0, true);
        }

        return (true);
    }
    else
    {
        m_tpScriptAnimation.invalidate();
        return (false);
    }
}

void CScriptEntity::UpdateCL() { bfScriptAnimation(); }
u32 CScriptEntity::GetActionCount() const { return (m_tpActionQueue.size()); }
const CScriptEntityAction* CScriptEntity::GetActionByIndex(u32 action_index) const
{
    return (m_tpActionQueue[action_index]);
}

void CScriptEntity::sound_callback(
    const IGameObject* object, int sound_type, const Fvector& position, float sound_power)
{
    if (!smart_cast<const CGameObject*>(object))
        return;

    if (!this->object().callback(GameObject::eSound))
        return;

    m_saved_sounds.push_back(CSavedSound(object->ID(), sound_type, position, sound_power));
}

CEntity* CScriptEntity::GetCurrentEnemy() { return (0); }
CEntity* CScriptEntity::GetCurrentCorpse() { return (0); }
int CScriptEntity::get_enemy_strength() { return (0); }
void CScriptEntity::process_sound_callbacks()
{
    xr_vector<CSavedSound>::const_iterator I = m_saved_sounds.begin();
    xr_vector<CSavedSound>::const_iterator E = m_saved_sounds.end();
    for (; I != E; ++I)
    {
        object().callback(GameObject::eSound)(
            object().lua_game_object(), (*I).m_game_object_id, (*I).m_sound_type, (*I).m_position, (*I).m_sound_power);
    }

    m_saved_sounds.clear();
}
