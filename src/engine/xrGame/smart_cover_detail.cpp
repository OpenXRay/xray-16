////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_detail.cpp
//	Created 	: 22.08.2007
//	Author		: Alexander Dudin
//	Description : detail namespace functions
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_detail.h"

float smart_cover::detail::parse_float	(
		luabind::object const &table,
		LPCSTR identifier,
		float const &min_threshold,
		float const &max_threshold
	)
{
	VERIFY2			(table.type() == LUA_TTABLE, "invalid loophole description passed");
	luabind::object	lua_result = table[identifier];
	VERIFY2			(lua_result.type() != LUA_TNIL, make_string("cannot read number value %s", identifier));
	VERIFY2			(lua_result.type() == LUA_TNUMBER, make_string("cannot read number value %s", identifier));
	float			result = luabind::object_cast<float>(lua_result);
	VERIFY2			(result >= min_threshold, make_string("invalid read number value %s", identifier));
	VERIFY2			(result <= max_threshold, make_string("invalid number value %s", identifier));
	return			(result);
}

LPCSTR smart_cover::detail::parse_string(luabind::object const &table, LPCSTR identifier)
{
	VERIFY2			(table.type() == LUA_TTABLE, "invalid loophole description passed");
	luabind::object	result = table[identifier];
	VERIFY2			(result.type() != LUA_TNIL, make_string("cannot read string value %s", identifier));
	VERIFY2			(result.type() == LUA_TSTRING, make_string("cannot read string value %s", identifier));
	return			(luabind::object_cast<LPCSTR>(result));
}

void smart_cover::detail::parse_table	(luabind::object const &table, LPCSTR identifier, luabind::object &result)
{
	VERIFY2			(table.type() == LUA_TTABLE, "invalid loophole description passed");
	result			= table[identifier];
	VERIFY2			(result.type() != LUA_TNIL, make_string("cannot read table value %s", identifier));
	VERIFY2			(result.type() == LUA_TTABLE, make_string("cannot read table value %s", identifier));
}

bool smart_cover::detail::parse_bool	(luabind::object const &table, LPCSTR identifier)
{
	VERIFY2			(table.type() == LUA_TTABLE, "invalid loophole description passed");
	luabind::object	result = table[identifier];
	VERIFY2			(result.type() != LUA_TNIL, make_string("cannot read boolean value %s", identifier));
	VERIFY2			(result.type() == LUA_TBOOLEAN, make_string("cannot read boolean value %s", identifier));
	return			(luabind::object_cast<bool>(result));
}

int smart_cover::detail::parse_int		(luabind::object const &table, LPCSTR identifier)
{
	VERIFY2			(table.type() == LUA_TTABLE, "invalid loophole description passed");
	luabind::object	result = table[identifier];
	VERIFY2			(result.type() != LUA_TNIL, make_string("cannot read number value %s", identifier));
	VERIFY2			(result.type() == LUA_TNUMBER, make_string("cannot read number value %s", identifier));
	return			(luabind::object_cast<int>(result));
}

Fvector smart_cover::detail::parse_fvector (luabind::object const &table, LPCSTR identifier)
{
	VERIFY2			(table.type() == LUA_TTABLE, "invalid loophole description passed");
	luabind::object	result = table[identifier];
	VERIFY2			(result.type() != LUA_TNIL, make_string("cannot read vector value %s", identifier));
	return			(luabind::object_cast<Fvector>(result));
}