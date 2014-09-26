#include "pch_script.h"
#include "tushkano.h"

using namespace luabind;

#pragma optimize("s",on)
void CTushkano::script_register(lua_State *L)
{
	module(L)
		[
			class_<CTushkano,CGameObject>("CTushkano")
			.def(constructor<>())
		];
}
