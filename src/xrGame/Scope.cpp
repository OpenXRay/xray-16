#include "pch_script.h"
#include "Scope.h"
#include "Silencer.h"
#include "GrenadeLauncher.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CScope::CScope	()
{
}

CScope::~CScope	() 
{
}

using namespace luabind;

SCRIPT_EXPORT(CScope, (CGameObject),
{
	module(luaState)
	[
		class_<CScope,CGameObject>("CScope")
			.def(constructor<>()),
		
		class_<CSilencer,CGameObject>("CSilencer")
			.def(constructor<>()),

		class_<CGrenadeLauncher,CGameObject>("CGrenadeLauncher")
			.def(constructor<>())
	];
});
