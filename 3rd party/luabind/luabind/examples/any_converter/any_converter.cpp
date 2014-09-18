#include <iostream>

extern "C"
{
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}

bool dostring(lua_State* L, const char* str)
{
	if (luaL_loadbuffer(L, str, std::strlen(str), str) || lua_pcall(L, 0, 0, 0))
	{
		const char* a = lua_tostring(L, -1);
		std::cout << a << "\n";
		lua_pop(L, 1);
		return true;
	}
	return false;
}

#define LUABIND_NO_HEADERS_ONLY

#include <luabind/luabind.hpp>
#include <boost/any.hpp>

template<class T>
struct convert_any
{
	static void convert(lua_State* L, const boost::any& a)
	{
		typename luabind::detail::default_policy::template generate_converter<T, luabind::detail::cpp_to_lua>::type conv;

		conv.apply(L, *boost::any_cast<T>(&a));
	}
};

std::map<const std::type_info*, void(*)(lua_State*, const boost::any&)> any_converters;

template<class T>
void register_any_converter()
{
	any_converters[&typeid(T)] = convert_any<T>::convert;
}

namespace luabind
{
	namespace converters
	{
		yes_t is_user_defined(by_value<boost::any>);
		yes_t is_user_defined(by_const_reference<boost::any>);

		void convert_cpp_to_lua(lua_State* L, const boost::any& a)
		{
			typedef void(*conv_t)(lua_State* L, const boost::any&);
			conv_t conv = any_converters[&a.type()];
			conv(L, a);
		}
	}
}

boost::any f(bool b)
{
	if (b) return "foobar";
	else return "3.5f";
}

int main()
{
	register_any_converter<int>();
	register_any_converter<float>();
	register_any_converter<const char*>();
	register_any_converter<string_class>();

	lua_State* L = lua_open();
	lua_baselibopen(L);

	using namespace luabind;
	
	open(L);
	module(L)
	[
		def("f", &f)
	];

	dostring(L, "print( f(true) )");
	dostring(L, "print( f(false) )");
	dostring(L, "function update(p) print(p) end");

	boost::any param = string_class("foo");

	luabind::call_function<void>(L, "update", param);

	lua_close(L);
}

