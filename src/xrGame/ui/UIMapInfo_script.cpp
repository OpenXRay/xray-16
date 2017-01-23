#include "pch_script.h"
#include "UIMapInfo.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CUIMapInfo, (CUIWindow),
{
	module(luaState)
	[
		class_<CUIMapInfo, CUIWindow>("CUIMapInfo")
		.def(				constructor<>())
		.def("Init",		&CUIMapInfo::InitMapInfo)
		.def("InitMap",		&CUIMapInfo::InitMap)
	];
});
