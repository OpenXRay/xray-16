#include "pch_script.h"
#include "HairsZone.h"

using namespace luabind;

#pragma optimize("s",on)
void CHairsZone::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CHairsZone,CGameObject>("CHairsZone")
			.def(constructor<>())
	];
}
