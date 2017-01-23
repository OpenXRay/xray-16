#include "pch_script.h"
#include "PHSimpleCalls.h"
#include "xrPhysics/PhysicsShell.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CPHCallOnStepCondition, (),
{
	module(luaState)
	[
		class_<CPHCallOnStepCondition>("phcondition_callonstep")
		.def("set_step",				&CPHCallOnStepCondition::set_step)
		.def("set_steps_interval",		&CPHCallOnStepCondition::set_steps_interval)
		.def("set_global_time_ms",		(void(CPHCallOnStepCondition::*)(u32))(&CPHCallOnStepCondition::set_global_time))
		.def("set_global_time_s",		(void(CPHCallOnStepCondition::*)(float))(&CPHCallOnStepCondition::set_global_time))
		.def("set_time_interval_ms",	(void(CPHCallOnStepCondition::*)(u32))(&CPHCallOnStepCondition::set_time_interval))
		.def("set_time_interval_s",		(void(CPHCallOnStepCondition::*)(float))(&CPHCallOnStepCondition::set_time_interval))
		.def(constructor<>())
	];
});

SCRIPT_EXPORT(CPHExpireOnStepCondition, (CPHCallOnStepCondition),
{
	module(luaState)
	[
		class_<CPHExpireOnStepCondition,CPHCallOnStepCondition>("phcondition_expireonstep")
		.def(constructor<>())
	];
});

SCRIPT_EXPORT(CPHConstForceAction, (),
{
	module(luaState)
	[
		class_<CPHConstForceAction>("phaction_constforce")
		.def(constructor<CPhysicsShell*,const Fvector&>())
	];
});

//(CPhysicsJoint*(CPhysicsShell::*)(u16))(&CPhysicsShell::get_Joint))
//.def("set_gravity",					&CPHWorld::SetGravity),
//.def("add_call",					&CPHWorld::AddCall)
