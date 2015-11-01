////////////////////////////////////////////////////////////////////////////
//	Module 		: script_rtoken_list_script.cpp
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script rtoken list class export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_rtoken_list.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CScriptRTokenList, (),
{
	module(luaState)
	[
		class_<CScriptRTokenList>("rtoken_list")
			.def(					constructor<>())
			.def("add",				&CScriptRTokenList::add)
			.def("remove",			&CScriptRTokenList::remove)
			.def("clear",			&CScriptRTokenList::clear)
			.def("count",			&CScriptRTokenList::size)
			.def("get",				&CScriptRTokenList::get)
	];
});
