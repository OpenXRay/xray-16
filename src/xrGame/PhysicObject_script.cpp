#include "pch_script.h"
#include "PhysicObject.h"
#include "PHCollisionDamageReceiver.h"
#include "PHDestroyable.h"
#include "hit_immunity.h"
#include "damage_manager.h"
#include "DestroyablePhysicsObject.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CPhysicObject, (CGameObject),
{
	module(luaState)
	[
		class_<CPhysicObject,CGameObject>("CPhysicObject")
			.def(constructor<>())
			.def("run_anim_forward",				&CPhysicObject::run_anim_forward)
			.def("run_anim_back",					&CPhysicObject::run_anim_back)
			.def("stop_anim",						&CPhysicObject::stop_anim)
			.def("anim_time_get",					&CPhysicObject::anim_time_get)
			.def("anim_time_set",					&CPhysicObject::anim_time_set)
			.def("play_bones_sound",				&CPhysicObject::play_bones_sound)
			.def("stop_bones_sound",				&CPhysicObject::stop_bones_sound)
			.def("set_door_ignore_dynamics",		&CPhysicObject::set_door_ignore_dynamics)
			.def("unset_door_ignore_dynamics",		&CPhysicObject::unset_door_ignore_dynamics),
        class_<CDestroyablePhysicsObject, CPhysicObject>("CDestroyablePhysicsObject")
            .def(constructor<>())
	];
});
