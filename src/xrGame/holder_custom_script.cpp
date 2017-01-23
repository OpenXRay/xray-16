#include "pch_script.h"
#include "holder_custom.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CHolderCustom, (),
{
	module(luaState)
	[
		class_<CHolderCustom>("holder")
		.def("engaged",			&CHolderCustom::Engaged)
		.def("Action",			&CHolderCustom::Action)
//			.def("SetParam",		(void (CHolderCustom::*)(int,Fvector2)) &CHolderCustom::SetParam)
		.def("SetParam",		(void (CHolderCustom::*)(int,Fvector)) &CHolderCustom::SetParam)
	];
});
