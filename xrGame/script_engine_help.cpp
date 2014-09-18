////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine_help.cpp
//	Created 	: 01.04.2004
//  Modified 	: 01.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Script Engine help
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"

#ifdef DEBUG

#ifndef BOOST_NO_STRINGSTREAM
//#	include <sstream>
#else
//#	include <strstream>
#endif

xr_string to_string					(luabind::object const& o)
{
	using namespace luabind;
	if (o.type() == LUA_TSTRING) return object_cast<luabind::internal_string>(o).c_str();
	lua_State* L = o.lua_state();
	LUABIND_CHECK_STACK(L);

	if (o.type() == LUA_TNUMBER)
	{
		char buffer[_CVTBUFSIZE];
		_gcvt_s( buffer, object_cast<float>(o), 16);
		return buffer;
	}

	return xr_string("<") + lua_typename(L, o.type()) + ">";
}

void strreplaceall						(xr_string &str, LPCSTR S, LPCSTR N)
{
	LPCSTR	A;
	int		S_len = xr_strlen(S);
	while ((A = strstr(str.c_str(),S)) != 0)
		str.replace(A - str.c_str(),S_len,N);
}

xr_string &process_signature				(xr_string &str)
{
	strreplaceall	(str,"custom [","");
	strreplaceall	(str,"]","");
	strreplaceall	(str,"float","number");
	strreplaceall	(str,"lua_State*, ","");
	strreplaceall	(str," ,lua_State*","");
	return			(str);
}

xr_string member_to_string			(luabind::object const& e, LPCSTR function_signature)
{
#if 1 || !defined(LUABIND_NO_ERROR_CHECKING)
    using namespace luabind;
	lua_State* L = e.lua_state();
	LUABIND_CHECK_STACK(L);

	if (e.type() == LUA_TFUNCTION)
	{
		e.pushvalue();
		detail::stack_pop p(L, 1);

		{
			if (lua_getupvalue(L, -1, 3) == 0) return to_string(e);
			detail::stack_pop p2(L, 1);
			if (lua_touserdata(L, -1) != reinterpret_cast<void*>(0x1337)) return to_string(e);
		}

// #ifdef BOOST_NO_STRINGSTREAM
// 		std::strstream s;
// #else
// 		std::stringstream s;
// #endif
		xr_string s = "";

		{
			lua_getupvalue(L, -1, 2);
			detail::stack_pop p2(L, 1);
		}

		{
			lua_getupvalue(L, -1, 1);
			detail::stack_pop p2(L, 1);
			detail::method_rep* m = static_cast<detail::method_rep*>(lua_touserdata(L, -1));

			for (std::vector<detail::overload_rep>::const_iterator i = m->overloads().begin();
				i != m->overloads().end(); ++i)
			{
				luabind::internal_string str;
				i->get_signature(L, str);
				if (i != m->overloads().begin())
					s += "\n";

				xr_string xr_str(str.c_str());
				s += function_signature + process_signature(xr_str) + ";";
			}
		}
#ifdef BOOST_NO_STRINGSTREAM
		s += "\n";// std::ends;
#endif
		return s;
	}

    return to_string(e);
#else
    return "";
#endif
}

void print_class						(lua_State *L, luabind::detail::class_rep *crep)
{
	xr_string			S;
	// print class and bases
	{
		S				= (crep->get_class_type() != luabind::detail::class_rep::cpp_class) ? "LUA class " : "C++ class ";
		S.append		(crep->name());
		typedef luabind::internal_vector<luabind::detail::class_rep::base_info> BASES;
		const BASES &bases = crep->bases();
		BASES::const_iterator	I = bases.begin(), B = I;
		BASES::const_iterator	E = bases.end();
		if (B != E)
			S.append	(" : ");
		for ( ; I != E; ++I) {
			if (I != B)
				S.append(",");
			S.append	((*I).base->name());
		}
		Msg				("%s {",S.c_str());
	}
	// print class constants
	{
		const luabind::detail::class_rep::STATIC_CONSTANTS	&constants = crep->static_constants();
		luabind::detail::class_rep::STATIC_CONSTANTS::const_iterator	I = constants.begin();
		luabind::detail::class_rep::STATIC_CONSTANTS::const_iterator	E = constants.end();
		for ( ; I != E; ++I)
#ifndef USE_NATIVE_LUA_STRINGS
			Msg		("    const %s = %d;",(*I).first,(*I).second);
#else
			Msg		("    const %s = %d;",getstr((*I).first.m_object),(*I).second);
#endif
		if (!constants.empty())
			Msg		("    ");
	}
	// print class properties
	{
#ifndef USE_NATIVE_LUA_STRINGS
		typedef luabind::internal_map<const char*, luabind::detail::class_rep::callback, luabind::detail::ltstr> PROPERTIES;
#else
		typedef luabind::detail::class_rep::callback_map PROPERTIES;
#endif
		const PROPERTIES &properties = crep->properties();
		PROPERTIES::const_iterator	I = properties.begin();
		PROPERTIES::const_iterator	E = properties.end();
		for ( ; I != E; ++I)
#ifndef USE_NATIVE_LUA_STRINGS
			Msg	("    property %s;",(*I).first);
#else
			Msg	("    property %s;",getstr((*I).first.m_object));
#endif
		if (!properties.empty())
			Msg		("    ");
	}
	// print class constructors
	{
		typedef luabind::internal_vector<luabind::detail::construct_rep::overload_t> Constructors;
		const Constructors &constructors = crep->constructors().overloads;
		Constructors::const_iterator	I = constructors.begin();
		Constructors::const_iterator	E = constructors.end();
		for ( ; I != E; ++I) {
			luabind::internal_string luaS;
			(*I).get_signature(L,luaS);
			xr_string S(luaS.c_str());
			strreplaceall	(S,"custom [","");
			strreplaceall	(S,"]","");
			strreplaceall	(S,"float","number");
			strreplaceall	(S,"lua_State*, ","");
			strreplaceall	(S," ,lua_State*","");
			Msg		("    %s %s;",crep->name(),S.c_str());
		}
		if (!constructors.empty())
			Msg		("    ");
	}
	// print class methods
	{
		crep->get_table	(L);
		luabind::object	table(L);
		table.set		();
		for (luabind::object::iterator i = table.begin(); i != table.end(); ++i) {
			luabind::object	object = *i;
			xr_string	S;
			S			= "    function ";
			S.append	(to_string(i.key()).c_str());

			strreplaceall	(S,"function __add","operator +");
			strreplaceall	(S,"function __sub","operator -");
			strreplaceall	(S,"function __mul","operator *");
			strreplaceall	(S,"function __div","operator /");
			strreplaceall	(S,"function __pow","operator ^");
			strreplaceall	(S,"function __lt","operator <");
			strreplaceall	(S,"function __le","operator <=");
			strreplaceall	(S,"function __gt","operator >");
			strreplaceall	(S,"function __ge","operator >=");
			strreplaceall	(S,"function __eq","operator ==");
			Msg			("%s",member_to_string(object,S.c_str()).c_str());
		}
	}
	Msg			("};\n");
}

void print_free_functions				(lua_State *L, const luabind::object &object, LPCSTR header, const xr_string &indent)
{
	u32							count = 0;
	luabind::object::iterator	I = object.begin();
	luabind::object::iterator	E = object.end();
	for ( ; I != E; ++I) {
		if ((*I).type() != LUA_TFUNCTION)
			continue;
		(*I).pushvalue();
		luabind::detail::free_functions::function_rep* rep = 0;
		if (lua_iscfunction(L, -1))
		{
			if (lua_getupvalue(L, -1, 2) != 0)
			{
				// check the magic number that identifies luabind's functions
				if (lua_touserdata(L, -1) == (void*)0x1337)
				{
					if (lua_getupvalue(L, -2, 1) != 0)
					{
						if (!count)
							Msg("\n%snamespace %s {",indent.c_str(),header);
						++count;
						rep = static_cast<luabind::detail::free_functions::function_rep*>(lua_touserdata(L, -1));
						std::vector<luabind::detail::free_functions::overload_rep>::const_iterator	i = rep->overloads().begin();
						std::vector<luabind::detail::free_functions::overload_rep>::const_iterator	e = rep->overloads().end();
						for ( ; i != e; ++i) {
							luabind::internal_string luaS;
							(*i).get_signature(L,luaS);
							xr_string	S(luaS.c_str());
							Msg("    %sfunction %s%s;",indent.c_str(),rep->name(),process_signature(S).c_str());
						}
						lua_pop(L, 1);
					}
				}
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);
	}
	{
		xr_string				_indent = indent;
		_indent.append			("    ");
		object.pushvalue();
		lua_pushnil		(L);
		while (lua_next(L, -2) != 0) {
			if (lua_type(L, -1) == LUA_TTABLE) {
				LPCSTR			S = lua_tostring(L, -2);
				if (xr_strcmp("_G",S) && xr_strcmp("package",S)) {
					luabind::object		object(L);
					object.set			();
					if (!xr_strcmp("security",S)) {
						S = S;
					}
					print_free_functions(L,object,S,_indent);
				}
			}
#pragma todo("Dima to Dima : Remove this hack if find out why")
			if (lua_isnumber(L,-2)) {
				lua_pop(L,1);
				lua_pop(L,1);
				break;
			}
			lua_pop	(L, 1);
		}
	}
	if (count)
		Msg("%s};",indent.c_str());
}

void print_help							(lua_State *L)
{
	Msg					("\nList of the classes exported to LUA\n");
	luabind::detail::class_registry::get_registry(L)->iterate_classes(L,&print_class);
	Msg					("End of list of the classes exported to LUA\n");
	Msg					("\nList of the namespaces exported to LUA\n");
	print_free_functions(L,luabind::get_globals(L),"","");
	Msg					("End of list of the namespaces exported to LUA\n");
}
#else
void print_help							(lua_State *L)
{
	Msg					("! Release build doesn't support lua-help :(");
}
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
