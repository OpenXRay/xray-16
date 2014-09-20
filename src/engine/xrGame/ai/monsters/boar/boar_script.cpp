#include "pch_script.h"
#include "boar.h"

using namespace luabind;

#pragma optimize("s",on)
void CAI_Boar::script_register(lua_State *L)
{
	module(L)
	[
		class_<CAI_Boar,CGameObject>("CAI_Boar")
			.def(constructor<>())
	];
}
