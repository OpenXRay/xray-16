#include "pch_script.h"
#include "WeaponKnife.h"

using namespace luabind;

#pragma optimize("s",on)
void CWeaponKnife::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponKnife,CGameObject>("CWeaponKnife")
			.def(constructor<>())
	];
}
