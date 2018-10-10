////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_script.cpp
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation manager : script animations
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_animation_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "xrScriptEngine/script_engine.hpp"
#include "game_object_space.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "ai_space.h"

void CStalkerAnimationManager::script_play_callback(CBlend* blend)
{
    CAI_Stalker* object = (CAI_Stalker*)blend->CallbackParam;
    VERIFY(object);

    CStalkerAnimationManager& animation_manager = object->animation();
    CStalkerAnimationPair& pair = animation_manager.script();
    const SCRIPT_ANIMATIONS& animations = animation_manager.script_animations();

#if 0
    Msg							(
        "%6d Script callback [%s]",
        Device.dwTimeGlobal,
        animations.empty()
        ?
        "unknown"
        :
        animation_manager.m_skeleton_animated->LL_MotionDefName_dbg(animations.front().animation())
    );
#endif

    if (pair.animation() && !animations.empty() && (pair.animation() == animations.front().animation()))
        animation_manager.pop_script_animation();

    animation_manager.m_call_script_callback = true;
    animation_manager.m_start_new_script_animation = true;

    pair.on_animation_end();
}

void CStalkerAnimationManager::add_script_animation(
    LPCSTR animation, bool hand_usage, Fvector position, Fvector rotation, bool local_animation)
{
    const MotionID& motion = m_skeleton_animated->ID_Cycle_Safe(animation);
    if (!motion)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "There is no animation %s (object %s)!", animation, *object().cName());
        return;
    }

    //	Msg("add_script_animation %f,%f,%f %f,%f,%f local=%s [%s]",
    //		position.x,position.y,position.z,
    //		rotation.x,rotation.y,rotation.z,
    //		local_animation ? "true" : "false",
    //		m_object->animation_movement() ? "true" : "false"
    //	);

    Fmatrix transform;
    rotation.mul(PI / 180.f);
    transform.setXYZ(rotation);
    transform.c = position;
    m_script_animations.push_back(CStalkerAnimationScript(motion, hand_usage, true, &transform, local_animation));
}

void CStalkerAnimationManager::add_script_animation(LPCSTR animation, bool hand_usage, bool use_movement_controller)
{
    const MotionID& motion = m_skeleton_animated->ID_Cycle_Safe(animation);
    if (!motion)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "There is no animation %s (object %s)!", animation, *object().cName());
        return;
    }

    m_script_animations.push_back(CStalkerAnimationScript(motion, hand_usage, use_movement_controller));
}

const CStalkerAnimationScript& CStalkerAnimationManager::assign_script_animation()
{
    VERIFY(!script_animations().empty());

    const CStalkerAnimationScript& animation = script_animations().front();
    if (animation.use_movement_controller() ||
        script().use_animation_movement_control(m_skeleton_animated, animation.animation()))
        script().target_matrix(object().XFORM());

    return (script_animations().front());
}
