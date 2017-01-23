#include "pch_script.h"
#include "physics_element_scripted.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

Fmatrix	global_transform(cphysics_element_scripted* E)
{
	Fmatrix m;
	E->GetGlobalTransformDynamic(&m);
	return m;
}

SCRIPT_EXPORT(cphysics_element_scripted, (),
{
	module(luaState)
	[
		class_<cphysics_element_scripted>("physics_element")
		.def("apply_force",					(void (cphysics_element_scripted::*)(float,float,float))(&cphysics_element_scripted::applyForce))
		.def("is_breakable",				&cphysics_element_scripted::isBreakable)
		.def("get_linear_vel",				&cphysics_element_scripted::get_LinearVel)
		.def("get_angular_vel",				&cphysics_element_scripted::get_AngularVel)
		.def("get_mass",					&cphysics_element_scripted::getMass)
		.def("get_density",					&cphysics_element_scripted::getDensity)
		.def("get_volume",					&cphysics_element_scripted::getVolume)
		.def("fix",							&cphysics_element_scripted::Fix)
		.def("release_fixed",				&cphysics_element_scripted::ReleaseFixed)
		.def("is_fixed",					&cphysics_element_scripted::isFixed)
		.def("global_transform",			&global_transform)
	];
});
