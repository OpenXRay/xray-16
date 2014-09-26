////////////////////////////////////////////////////////////////////////////
//	Module 		: script_token_list_script.cpp
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script token list class export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_token_list.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptTokenList::script_register(lua_State *L)
{
	module(L)
	[
		class_<xr_token>("token")
			.def(					constructor<>())
			.def_readwrite("name",	&xr_token::name)
			.def_readwrite("id",	&xr_token::id),

		class_<CScriptTokenList>("token_list")
			.def(					constructor<>())
			.def("add",				&CScriptTokenList::add)
			.def("remove",			&CScriptTokenList::remove)
			.def("clear",			&CScriptTokenList::clear)
			.def("id",				&CScriptTokenList::id)
			.def("name",			&CScriptTokenList::name)
	];
}
