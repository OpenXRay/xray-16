#include "pch_script.h"
#include "fracture.h"

using namespace luabind;

#pragma optimize("s",on)
void CFracture::script_register(lua_State *L)
{
	module(L)
	[
		class_<CFracture,CGameObject>("CFracture")
			.def(constructor<>())
	];
}
