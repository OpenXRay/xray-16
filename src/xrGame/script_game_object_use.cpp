#include "pch_script.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "GameObject.h"
#include "xrScriptEngine/script_engine.hpp"
#include "stalker_planner.h"
#include "ai/stalker/ai_stalker.h"
#include "searchlight.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "game_object_space.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "movement_manager.h"
#include "patrol_path_manager.h"
#include "PHCommander.h"
#include "PHScriptCall.h"
#include "PHSimpleCalls.h"
#include "xrPhysics/IPHWorld.h"
#include "doors_manager.h"

void CScriptGameObject::SetTipText(LPCSTR tip_text) { object().set_tip_text(tip_text); }
void CScriptGameObject::SetTipTextDefault() { object().set_tip_text_default(); }
void CScriptGameObject::SetNonscriptUsable(bool nonscript_usable) { object().set_nonscript_usable(nonscript_usable); }
Fvector CScriptGameObject::GetCurrentDirection()
{
    CProjector* obj = smart_cast<CProjector*>(&object());
    if (!obj)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "Script Object : cannot access class member GetCurrentDirection!");
        return Fvector().set(0.f, 0.f, 0.f);
    }
    return obj->GetCurrentDirection();
}

CScriptGameObject::CScriptGameObject(CGameObject* game_object) : m_game_object(game_object), m_door(0)
{
    R_ASSERT2(m_game_object, "Null actual object passed!");
}

CScriptGameObject::~CScriptGameObject()
{
    if (!m_door)
        return;

    unregister_door();
}

CScriptGameObject* CScriptGameObject::Parent() const
{
    CGameObject* l_tpGameObject = smart_cast<CGameObject*>(object().H_Parent());
    if (l_tpGameObject)
        return (l_tpGameObject->lua_game_object());
    else
        return (0);
}

int CScriptGameObject::clsid() const { return (object().clsid()); }
LPCSTR CScriptGameObject::Name() const { return (*object().cName()); }
shared_str CScriptGameObject::cName() const { return (object().cName()); }
LPCSTR CScriptGameObject::Section() const { return (*object().cNameSect()); }
void CScriptGameObject::Kill(CScriptGameObject* who, bool bypass_actor_check /*AVO: added for actor before death callback*/)
{
    CEntity* l_tpEntity = smart_cast<CEntity*>(&object());
    if (!l_tpEntity)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "%s cannot access class member Kill!", *object().cName());
        return;
    }
    if (!l_tpEntity->AlreadyDie())
        l_tpEntity->KillEntity(who ? who->object().ID() : object().ID(), bypass_actor_check);
    else
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "attempt to kill dead object %s", *object().cName());
}

bool CScriptGameObject::Alive() const
{
    CEntity* entity = smart_cast<CEntity*>(&object());
    if (!entity)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSciptEntity : cannot access class member Alive!");
        return (false);
    }
    return (!!entity->g_Alive());
}

ALife::ERelationType CScriptGameObject::GetRelationType(CScriptGameObject* who)
{
    CEntityAlive* l_tpEntityAlive1 = smart_cast<CEntityAlive*>(&object());
    if (!l_tpEntityAlive1)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "%s cannot access class member GetRelationType!", *object().cName());
        return ALife::eRelationTypeDummy;
    }

    CEntityAlive* l_tpEntityAlive2 = smart_cast<CEntityAlive*>(&who->object());
    if (!l_tpEntityAlive2)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "%s cannot apply GetRelationType method for non-alive object!", *who->object().cName());
        return ALife::eRelationTypeDummy;
    }

    return l_tpEntityAlive1->tfGetRelationType(l_tpEntityAlive2);
}

template <typename T>
IC T* CScriptGameObject::action_planner()
{
    CAI_Stalker* manager = smart_cast<CAI_Stalker*>(&object());
    if (!manager)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member action_planner!");
        return (0);
    }
    return (&manager->brain());
}

CScriptActionPlanner* script_action_planner(CScriptGameObject* obj)
{
    return (obj->action_planner<CScriptActionPlanner>());
}

void CScriptGameObject::set_enemy_callback(const luabind::functor<bool>& functor)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member set_enemy_callback!");
        return;
    }
    monster->memory().enemy().useful_callback().set(functor);
}

void CScriptGameObject::set_enemy_callback(const luabind::functor<bool>& functor, const luabind::object& object)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member set_enemy_callback!");
        return;
    }
    monster->memory().enemy().useful_callback().set(functor, object);
}

void CScriptGameObject::set_enemy_callback()
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member set_enemy_callback!");
        return;
    }
    monster->memory().enemy().useful_callback().clear();
}

void CScriptGameObject::SetCallback(GameObject::ECallbackType type, const luabind::functor<void>& functor)
{
    object().callback(type).set(functor);
}

void CScriptGameObject::SetCallback(
    GameObject::ECallbackType type, const luabind::functor<void>& functor, const luabind::object& object)
{
    this->object().callback(type).set(functor, object);
}

void CScriptGameObject::SetCallback(GameObject::ECallbackType type) { object().callback(type).clear(); }
void CScriptGameObject::set_fastcall(const luabind::functor<bool>& functor, const luabind::object& object)
{
    CPHScriptGameObjectCondition* c = new CPHScriptGameObjectCondition(object, functor, m_game_object);
    CPHDummiAction* a = new CPHDummiAction();
    CPHSriptReqGObjComparer cmpr(m_game_object);
    Level().ph_commander_scripts().RemoveCallsDeferred(&cmpr);
    Level().ph_commander_scripts().AddCallDeferred(c, a);
}
void CScriptGameObject::set_const_force(const Fvector& dir, float value, u32 time_interval)
{
    CPhysicsShell* shell = object().cast_physics_shell_holder()->PPhysicsShell();
    // if( !shell->isEnabled() )
    //	shell->set_LinearVel( Fvector().set(0,0,0) );
    if (!physics_world())
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "set_const_force : ph_world do not exist!");
        return;
    }
    if (!shell)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "set_const_force : object %s has no physics shell!", *object().cName());
        return;
    }
    //#ifdef DEBUG
    //	Msg( "const force added: force: %f,  time: %d ,dir(%f,%f,%f)", value, time_interval, dir.x, dir.y, dir.z );
    //#endif
    Fvector force;
    force.set(dir);
    force.mul(value);
    CPHConstForceAction* a = new CPHConstForceAction(shell, force);
    CPHExpireOnStepCondition* cn = new CPHExpireOnStepCondition();
    cn->set_time_interval(time_interval);
    // ph_world->AddCall(cn,a);
    Level().ph_commander_physics_worldstep().add_call_threadsafety(cn, a);
}
