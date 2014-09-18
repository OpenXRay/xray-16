#include "pch_script.h"
#include "snork.h"

using namespace luabind;

#pragma optimize("s",on)
void CSnork::script_register(lua_State *L)
{
	module(L)
	[
		class_<CSnork,CGameObject>("CSnork")
			.def(constructor<>())
	];
}
