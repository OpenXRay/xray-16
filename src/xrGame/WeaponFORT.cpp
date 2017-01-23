#include "pch_script.h"
#include "WeaponFORT.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponFORT::CWeaponFORT()
{}

CWeaponFORT::~CWeaponFORT()
{}

using namespace luabind;

SCRIPT_EXPORT(CWeaponFORT, (CGameObject),
{
	module(luaState)
	[
		class_<CWeaponFORT,CGameObject>("CWeaponFORT")
			.def(constructor<>())
	];
});
