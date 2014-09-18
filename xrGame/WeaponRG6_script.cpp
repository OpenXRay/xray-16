#include "pch_script.h"
#include "WeaponRG6.h"

using namespace luabind;

#pragma optimize("s",on)
void CWeaponRG6::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponRG6,CGameObject>("CWeaponRG6")
			.def(constructor<>())
	];
}
