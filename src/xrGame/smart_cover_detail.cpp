////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_detail.cpp
//	Created 	: 22.08.2007
//	Author		: Alexander Dudin
//	Description : detail namespace functions
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_detail.h"

namespace smart_cover
{
constexpr pcstr s_enter_loophole_id = "<__ENTER__>";
constexpr pcstr s_exit_loophole_id  = "<__EXIT__>";

float detail::parse_float(
    luabind::object const& table, LPCSTR identifier, float const& min_threshold, float const& max_threshold)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    luabind::object lua_result = table[identifier];
    VERIFY2(luabind::type(lua_result) != LUA_TNIL, make_string("cannot read number value %s", identifier));
    VERIFY2(luabind::type(lua_result) == LUA_TNUMBER, make_string("cannot read number value %s", identifier));
    float result = luabind::object_cast<float>(lua_result);
    VERIFY2(result >= min_threshold, make_string("invalid read number value %s", identifier));
    VERIFY2(result <= max_threshold, make_string("invalid number value %s", identifier));
    return (result);
}

bool detail::parse_float(float& output,
    luabind::object const& table, LPCSTR identifier, float const& min_threshold, float const& max_threshold)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    const luabind::object lua_result = table[identifier];
    const auto type = luabind::type(lua_result);
    if (type == LUA_TNUMBER)
    {
        output = luabind::object_cast<float>(lua_result);
        VERIFY2(output >= min_threshold, make_string("invalid read number value %s", identifier));
        VERIFY2(output <= max_threshold, make_string("invalid number value %s", identifier));
        return true;
    }
    return false;
}

LPCSTR detail::parse_string(luabind::object const& table, LPCSTR identifier)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    luabind::object result = table[identifier];
    VERIFY2(luabind::type(result) != LUA_TNIL, make_string("cannot read string value %s", identifier));
    VERIFY2(luabind::type(result) == LUA_TSTRING, make_string("cannot read string value %s", identifier));
    return (luabind::object_cast<LPCSTR>(result));
}

void detail::parse_table(luabind::object const& table, LPCSTR identifier, luabind::object& result)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    result = table[identifier];
    VERIFY2(luabind::type(result) != LUA_TNIL, make_string("cannot read table value %s", identifier));
    VERIFY2(luabind::type(result) == LUA_TTABLE, make_string("cannot read table value %s", identifier));
}

bool detail::parse_bool(luabind::object const& table, LPCSTR identifier)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    luabind::object result = table[identifier];
    VERIFY2(luabind::type(result) != LUA_TNIL, make_string("cannot read boolean value %s", identifier));
    VERIFY2(luabind::type(result) == LUA_TBOOLEAN, make_string("cannot read boolean value %s", identifier));
    return (luabind::object_cast<bool>(result));
}

int detail::parse_int(luabind::object const& table, LPCSTR identifier)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    luabind::object result = table[identifier];
    VERIFY2(luabind::type(result) != LUA_TNIL, make_string("cannot read number value %s", identifier));
    VERIFY2(luabind::type(result) == LUA_TNUMBER, make_string("cannot read number value %s", identifier));
    return (luabind::object_cast<int>(result));
}

Fvector detail::parse_fvector(luabind::object const& table, LPCSTR identifier)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    luabind::object result = table[identifier];
    VERIFY2(luabind::type(result) != LUA_TNIL, make_string("cannot read vector value %s", identifier));
    return (luabind::object_cast<Fvector>(result));
}

bool detail::parse_fvector(luabind::adl::object const& table, LPCSTR identifier, Fvector& output)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    luabind::object result = table[identifier];
    if (luabind::type(result) != LUA_TNIL)
    {
        output = luabind::object_cast<Fvector>(result);
        return true;
    }
    return false;
}

shared_str transform_vertex(shared_str const& vertex_id, bool const& in)
{
    if (*vertex_id.c_str())
        return (vertex_id);

    if (in)
        return (s_enter_loophole_id);

    return (s_exit_loophole_id);
}

shared_str parse_vertex(luabind::object const& table, LPCSTR identifier, bool const& in)
{
    return (transform_vertex(detail::parse_string(table, identifier), in));
}
} // namespace smart_cover
