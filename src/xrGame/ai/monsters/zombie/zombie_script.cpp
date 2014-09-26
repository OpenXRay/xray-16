#include "pch_script.h"
#include "zombie.h"

using namespace luabind;

#pragma optimize("s",on)
void CZombie::script_register(lua_State *L)
{
	module(L)
	[
		class_<CZombie,CGameObject>("CZombie")
			.def(constructor<>())
	];
}
