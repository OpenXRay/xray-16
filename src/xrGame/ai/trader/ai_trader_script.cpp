#include "pch_script.h"
#include "ai_trader.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CAI_Trader, (CGameObject),
{
	module(luaState)
	[
		class_<CAI_Trader,CGameObject>("CAI_Trader")
			.def(constructor<>())
	];
});
