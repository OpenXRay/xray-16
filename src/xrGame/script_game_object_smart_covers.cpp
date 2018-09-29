////////////////////////////////////////////////////////////////////////////
//	Module 		: script_game_object_smart_covers.cpp
//	Created 	: 14.02.2008
//  Modified 	: 14.02.2008
//	Author		: Dmitriy Iassenev
//	Description : script game object class smart covers stuff
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_movement_manager_smart_cover.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "smart_cover.h"

bool CScriptGameObject::use_smart_covers_only() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member use_smart_covers_only!");
        return (false);
    }

    return (stalker->use_smart_covers_only());
}

void CScriptGameObject::use_smart_covers_only(bool value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member use_smart_covers_only!");
        return;
    }

    stalker->use_smart_covers_only(value);
}

void CScriptGameObject::set_smart_cover_target_selector()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_smart_cover_target_selector!");
        return;
    }

    stalker->movement().target_selector(CScriptCallbackEx<void>());
}

void CScriptGameObject::set_smart_cover_target_selector(luabind::functor<void> functor)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_smart_cover_target_selector!");
        return;
    }

    CScriptCallbackEx<void> callback;
    callback.set(functor);
    stalker->movement().target_selector(callback);
}

void CScriptGameObject::set_smart_cover_target_selector(luabind::functor<void> functor, luabind::object object)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&this->object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_smart_cover_target_selector!");
        return;
    }

    CScriptCallbackEx<void> callback;
    callback.set(functor, object);
    stalker->movement().target_selector(callback);
}

void CScriptGameObject::set_smart_cover_target_idle()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member smart_cover_setup_idle_target!");
        return;
    }

    if (!stalker->g_Alive())
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : do not call smart_cover_setup_idle_target when stalker is dead!");
        return;
    }

    stalker->movement().target_idle();
}

void CScriptGameObject::set_smart_cover_target_lookout()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member smart_cover_setup_lookout_target!");
        return;
    }

    if (!stalker->g_Alive())
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : do not call smart_cover_setup_lookout_target when stalker is dead!");
        return;
    }

    stalker->movement().target_lookout();
}

void CScriptGameObject::set_smart_cover_target_fire()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member smart_cover_setup_fire_target!");
        return;
    }

    if (!stalker->g_Alive())
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : do not call smart_cover_setup_fire_target when stalker is dead!");
        return;
    }

    stalker->movement().target_fire();
}

void CScriptGameObject::set_smart_cover_target_fire_no_lookout()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "CAI_Stalker : cannot access class member smart_cover_setup_fire_no_lookout_target!");
        return;
    }

    if (!stalker->g_Alive())
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "CAI_Stalker : do not call set_smart_cover_target_fire_no_lookout when stalker is dead!");
        return;
    }

    stalker->movement().target_fire_no_lookout();
}

void CScriptGameObject::set_smart_cover_target_default(bool value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_smart_cover_target_default!");
        return;
    }

    if (!stalker->g_Alive())
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : do not call set_smart_cover_target_default when stalker is dead!");
        return;
    }

    stalker->movement().target_default(value);
}

bool CScriptGameObject::in_smart_cover() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member in_smart_cover_mode!");
        return ("");
    }

    return (stalker->movement().in_smart_cover());
}

void CScriptGameObject::set_dest_smart_cover(LPCSTR cover_id)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_dest_smart_cover!");
        return;
    }

    stalker->movement().target_params().cover_id(cover_id);
}

void CScriptGameObject::set_dest_smart_cover()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_dest_smart_cover!");
        return;
    }

    stalker->movement().target_params().cover_id("");
}

CCoverPoint const* CScriptGameObject::get_dest_smart_cover()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member get_dest_smart_cover!");
        return (0);
    }

    return (stalker->movement().target_params().cover());
}
LPCSTR CScriptGameObject::get_dest_smart_cover_name()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member get_dest_smart_cover!");
        return (0);
    }

    return (stalker->movement().target_params().cover_id().c_str());
}

void CScriptGameObject::set_dest_loophole(LPCSTR loophole_id)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_dest_loophole!");
        return;
    }

    stalker->movement().target_params().cover_loophole_id(loophole_id);
}

void CScriptGameObject::set_dest_loophole()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_dest_loophole!");
        return;
    }

    stalker->movement().target_params().cover_loophole_id("");
}

void CScriptGameObject::set_smart_cover_target(Fvector value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_smart_cover_target!");
        return;
    }

    stalker->movement().target_params().cover_fire_position(&value);
}

void CScriptGameObject::set_smart_cover_target()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_smart_cover_target!");
        return;
    }

    stalker->movement().target_params().cover_fire_position(0);
}

void CScriptGameObject::set_smart_cover_target(CScriptGameObject* enemy_object)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_smart_cover_target!");
        return;
    }

    stalker->movement().target_params().cover_fire_object(&enemy_object->object());
}

bool CScriptGameObject::in_loophole_fov(LPCSTR cover_id, LPCSTR loophole_id, Fvector object_position) const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member object_in_loophole_fov!");
        return (false);
    }

    return (stalker->movement().in_fov(cover_id, loophole_id, object_position));
}

bool CScriptGameObject::in_current_loophole_fov(Fvector object_position) const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member object_in_loophole_fov!");
        return (false);
    }

    return (stalker->movement().in_current_loophole_fov(object_position));
}

bool CScriptGameObject::in_loophole_range(LPCSTR cover_id, LPCSTR loophole_id, Fvector object_position) const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member object_in_loophole_range!");
        return (false);
    }

    return (stalker->movement().in_range(cover_id, loophole_id, object_position));
}

bool CScriptGameObject::in_current_loophole_range(Fvector object_position) const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member object_in_loophole_range!");
        return (false);
    }

    return (stalker->movement().in_current_loophole_range(object_position));
}

float const CScriptGameObject::idle_min_time() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member idle_min_time!");
        return (flt_max);
    }

    return (stalker->movement().idle_min_time());
}

void CScriptGameObject::idle_min_time(float value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member idle_min_time!");
        return;
    }

    stalker->movement().idle_min_time(value);
}

float const CScriptGameObject::idle_max_time() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member idle_max_time!");
        return (flt_max);
    }

    return (stalker->movement().idle_max_time());
}

void CScriptGameObject::idle_max_time(float value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member idle_max_time!");
        return;
    }

    stalker->movement().idle_max_time(value);
}

float const CScriptGameObject::lookout_min_time() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member lookout_min_time!");
        return (flt_max);
    }

    return (stalker->movement().lookout_min_time());
}

void CScriptGameObject::lookout_min_time(float value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member lookout_min_time!");
        return;
    }

    stalker->movement().lookout_min_time(value);
}

float const CScriptGameObject::lookout_max_time() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member lookout_max_time!");
        return (flt_max);
    }

    return (stalker->movement().lookout_max_time());
}

void CScriptGameObject::lookout_max_time(float value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member lookout_max_time!");
        return;
    }

    stalker->movement().lookout_max_time(value);
}

float CScriptGameObject::apply_loophole_direction_distance() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member smart_cover_enter_distance!");
        return (flt_max);
    }

    return (stalker->movement().apply_loophole_direction_distance());
}

void CScriptGameObject::apply_loophole_direction_distance(float value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member smart_cover_enter_distance!");
        return;
    }

    stalker->movement().apply_loophole_direction_distance(value);
}

bool CScriptGameObject::movement_target_reached()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member movement_target_reached!");
        return (false);
    }

    return (stalker->movement().current_params().equal_to_target(stalker->movement().target_params()));
}
