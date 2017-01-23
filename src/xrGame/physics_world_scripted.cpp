#include "pch_script.h"
#include "physics_world_scripted.h"
#include "Level.h"
#include "PHCommander.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

void	cphysics_world_scripted::	AddCall			(	CPHCondition*c, CPHAction*a )	
{
	Level().ph_commander_physics_worldstep().add_call_threadsafety(c,a);
}

SCRIPT_EXPORT(cphysics_world_scripted, (),
{
	module(luaState)
	[
		class_<cphysics_world_scripted>("physics_world")
		.def("set_gravity",					&cphysics_world_scripted::SetGravity)
		.def("gravity",						&cphysics_world_scripted::Gravity)
		.def("add_call",					&cphysics_world_scripted::AddCall)
	];
});
