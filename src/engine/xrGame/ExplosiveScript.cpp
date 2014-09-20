#include "pch_script.h"
#include "Explosive.h"

using namespace luabind;

#pragma optimize("s",on)
void CExplosive::script_register(lua_State *L)
{
	module(L)
	[
		class_<CExplosive>("explosive")
			.def("explode",					(&CExplosive::Explode))
	];
}

