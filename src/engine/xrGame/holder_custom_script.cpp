#include "pch_script.h"
#include "holder_custom.h"

using namespace luabind;

#pragma optimize("s",on)
void CHolderCustom::script_register(lua_State *L)
{
	module(L)
		[
			class_<CHolderCustom>("holder")
			.def("engaged",			&CHolderCustom::Engaged)
			.def("Action",			&CHolderCustom::Action)
//			.def("SetParam",		(void (CHolderCustom::*)(int,Fvector2)) &CHolderCustom::SetParam)
			.def("SetParam",		(void (CHolderCustom::*)(int,Fvector)) &CHolderCustom::SetParam)
		];
}