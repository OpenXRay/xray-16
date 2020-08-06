////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_manager_update.cpp
//	Created 	: 25.02.2003
//  Modified 	: 13.12.2006
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation manager update cycle
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_animation_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "game_object_space.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "xrEngine/profiler.h"
#include "stalker_movement_manager_smart_cover.h"

/*IC*/ void CStalkerAnimationManager::play_delayed_callbacks() // XXX: can't compile Release because of "inline"
{
    if (m_call_script_callback)
    {
        m_call_script_callback = false;
        object().callback(GameObject::eScriptAnimation)();
        return;
    }

    if (m_call_global_callback)
    {
        m_call_global_callback = false;
        if (m_global_callback)
            m_global_callback();
        return;
    }
}

IC bool CStalkerAnimationManager::script_callback() const
{
    if (script_animations().empty())
        return (false);

    return (object().callback(GameObject::eScriptAnimation));
}

IC bool CStalkerAnimationManager::need_update() const
{
    if (script_callback())
        return (true);

    return (non_script_need_update());
}

IC void CStalkerAnimationManager::update_tracks()
{
    if (!need_update())
        return;

    m_skeleton_animated->UpdateTracks();
}

#ifdef USE_HEAD_BONE_PART_FAKE
IC void CStalkerAnimationManager::play_script_impl()
{
    clear_unsafe_callbacks();
    global().reset();
    torso().reset();
    legs().reset();

    const CStalkerAnimationScript& selected = assign_script_animation();
    script().animation(selected.animation());
    if (selected.use_movement_controller())
    {
        script().target_matrix(selected.transform(object()));
        if (m_start_new_script_animation)
        {
            m_start_new_script_animation = false;
            if (selected.has_transform() && object().animation_movement())
                object().destroy_anim_mov_ctrl();
        }
    }

    script().play(m_skeleton_animated, script_play_callback, selected.use_movement_controller(),
        selected.local_animation(), false, m_script_bone_part_mask);

    head().animation(assign_head_animation());
    head().play(m_skeleton_animated, head_play_callback, false, false);
}
#else // USE_HEAD_BONE_PART_FAKE
IC void CStalkerAnimationManager::play_script_impl()
{
    clear_unsafe_callbacks();
    global().reset();
    head().reset();
    torso().reset();
    legs().reset();

    const CStalkerAnimationScript& selected = assign_script_animation();
    script().animation(selected.animation());
    script().play(m_skeleton_animated, script_play_callback, selected.use_movement_controller(),
        selected.local_animation(), false, m_script_bone_part_mask);
}
#endif // USE_HEAD_BONE_PART_FAKE

bool CStalkerAnimationManager::play_script()
{
    if (script_animations().empty())
    {
        m_start_new_script_animation = false;
        script().reset();
        return (false);
    }

    play_script_impl();

    return (true);
}

#ifdef USE_HEAD_BONE_PART_FAKE
IC void CStalkerAnimationManager::play_global_impl(const MotionID& animation, bool const& animation_movement_controller)
{
    torso().reset();
    legs().reset();

    global().animation(animation);
    global().play(m_skeleton_animated, global_play_callback, animation_movement_controller, true, false,
        m_script_bone_part_mask, true);

    if (m_global_modifier)
        m_global_modifier(global().blend());

    head().animation(assign_head_animation());
    head().play(m_skeleton_animated, head_play_callback, false, false);
}
#else // USE_HEAD_BONE_PART_FAKE
IC void CStalkerAnimationManager::play_global_impl(const MotionID& animation, bool const& animation_movement_controller)
{
    head().reset();
    torso().reset();
    legs().reset();

    global().animation(animation);
    global().play(m_skeleton_animated, global_play_callback, false, false, false);
}
#endif // USE_HEAD_BONE_PART_FAKE

bool CStalkerAnimationManager::play_global()
{
    bool animation_movement_controller = false;
    const MotionID& global_animation = assign_global_animation(animation_movement_controller);
    if (!global_animation)
    {
        clear_unsafe_callbacks();
        global().reset();
        return (false);
    }

    play_global_impl(global_animation, animation_movement_controller);

    return (true);
}

IC void CStalkerAnimationManager::play_head()
{
    head().animation(assign_head_animation());
    head().play(m_skeleton_animated, head_play_callback, false, false);
}

IC void CStalkerAnimationManager::play_torso()
{
    torso().animation(assign_torso_animation());
    torso().play(m_skeleton_animated, torso_play_callback, false, false);
}

void CStalkerAnimationManager::play_legs()
{
    float speed = 0.f;
    bool first_time = !legs().animation();
    bool result = legs().animation(assign_legs_animation());

    if (!first_time && !result && legs().blend())
    {
        float amount = legs().blend()->blendAmount;
        m_previous_speed = (m_target_speed - m_previous_speed) * amount + m_previous_speed;
    }

    legs().play(m_skeleton_animated, legs_play_callback, false, false, !fis_zero(m_target_speed));

    if (result && legs().blend())
    {
        float amount = legs().blend()->blendAmount;
        speed = (m_target_speed - m_previous_speed) * amount + m_previous_speed;
    }

    if (fis_zero(speed))
        return;

    if (!legs().blend())
        return;

    object().movement().setup_speed_from_animation(speed);
}

void CStalkerAnimationManager::update_impl()
{
    if (!object().g_Alive())
        return;

    update_tracks();
    play_delayed_callbacks();

    if (play_script())
        return;

    if (play_global())
        return;

    play_head();
    play_torso();
    play_legs();

    torso().synchronize(m_skeleton_animated, m_legs);
}

void CStalkerAnimationManager::update()
{
    START_PROFILE("stalker/client_update/animations")
    try
    {
        update_impl();
    }
    catch (...)
    {
        Msg("! error in stalker with visual %s and ID %u", *object().cNameVisual(), object().ID());
        head().reset();
        torso().reset();
        legs().reset();
        global().reset();
        return;
        // throw;
    }
    STOP_PROFILE
}
