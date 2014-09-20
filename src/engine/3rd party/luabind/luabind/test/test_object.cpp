// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#include "test.hpp"
#include <luabind/luabind.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/detail/debug.hpp>
#include <luabind/error.hpp>

#include <utility>

namespace
{
	using namespace luabind;

	int test_object_param(const object& table)
	{
		LUABIND_CHECK_STACK(table.lua_state());

		object current_object;
		current_object = table;
		
		if (table.type() == LUA_TTABLE)
		{

			int sum = object_cast<int>(table["oh"]);
			for (object::array_iterator i = table.abegin(); i != table.aend(); ++i)
			{
				assert(i->type() == LUA_TNUMBER);
				sum += object_cast<int>(*i);
			}

			int sum2 = 0;
			for (object::iterator i = table.begin(); i != table.end(); ++i)
			{
				assert(i->type() == LUA_TNUMBER);
				sum2 += object_cast<int>(*i);
			}

			int sum3 = 0;
			for (object::raw_iterator i = table.raw_begin(); i != table.raw_end(); ++i)
			{
				assert(i->type() == LUA_TNUMBER);
				sum3 += object_cast<int>(*i);
			}

			table["sum"] = sum;
			table["sum2"] = sum2;
			table["sum3"] = sum3;
			table["blurp"] = 5;
			return 0;
		}
		else
		{
			if (table.type() != LUA_TNIL)
			{
				return 1;
			}
			else
			{
				return 2;
			}
		}
	}

	int test_fun()
	{
		return 42;
	}

	struct test_param : counted_type<test_param>
	{
		luabind::object obj;
		luabind::object obj2;
	};

	int test_match(const luabind::object& o)
	{
		return 0;
	}

	int test_match(int i)
	{
		return 1;
	}

	void test_match_object(
		luabind::object p1
		, luabind::object p2
		, luabind::object p3)
	{
		p1["ret"] = 1;
		p2["ret"] = 2;
		p3["ret"] = 3;
	}

} // anonymous namespace

void test_object()
{
    COUNTER_GUARD(test_param);

	lua_state L;

	using namespace luabind;

	module(L)
	[
		def("test_object_param", &test_object_param),
		def("test_fun", &test_fun),
		def("test_match", (int(*)(const luabind::object&))&test_match),
		def("test_match", (int(*)(int))&test_match),
		def("test_match_object", &test_match_object),
	
		class_<test_param>("test_param")
			.def(constructor<>())
			.def_readwrite("obj", &test_param::obj)
			.def_readonly("obj2", &test_param::obj2)
	];

	DOSTRING(L,
		"t = 2\n"
		"assert(test_object_param(t) == 1)");

	DOSTRING(L, "assert(test_object_param(nil) == 2)");
	DOSTRING(L, "t = { ['oh'] = 4, 3, 5, 7, 13 }");
	DOSTRING(L, "assert(test_object_param(t) == 0)");
	DOSTRING(L, "assert(t.sum == 4 + 3 + 5 + 7 + 13)");
	DOSTRING(L, "assert(t.sum2 == 4 + 3 + 5 + 7 + 13)");
	DOSTRING(L, "assert(t.sum3 == 4 + 3 + 5 + 7 + 13)");
	DOSTRING(L, "assert(t.blurp == 5)");

	object g = get_globals(L);
	object fun = g["test_fun"];
	object ret = fun();
	BOOST_CHECK(object_cast<int>(ret) == 42);

	DOSTRING(L, "function test_param_policies(x, y) end");
	object test_param_policies = g["test_param_policies"];
	int a = test_param_policies.type();
	BOOST_CHECK(a == LUA_TFUNCTION);

	// call the function and tell lua to adopt the pointer passed as first argument
	test_param_policies(5, new test_param())[adopt(_2)];

	DOSTRING(L, "assert(test_match(7) == 1)");
	DOSTRING(L, "assert(test_match('oo') == 0)");

	DOSTRING(L,
		"t = test_param()\n"
		"t.obj = 'foo'\n"
		"assert(t.obj == 'foo')\n"
		"assert(t.obj2 == nil)");

	DOSTRING(L,
		"function test_object_policies(a) glob = a\n"
		"return 6\n"
		"end");
	object test_object_policies = g["test_object_policies"];
	object ret_val = test_object_policies("teststring")[detail::null_type()];
	BOOST_CHECK(object_cast<int>(ret_val) == 6);
	BOOST_CHECK(object_cast<string_class>(g["glob"]) == "teststring");
	BOOST_CHECK(object_cast<string_class>(g.at("glob")) == "teststring");
	BOOST_CHECK(object_cast<string_class>(g.raw_at("glob")) == "teststring");

	object t = newtable(L);
	BOOST_CHECK(t.begin() == t.end());
	BOOST_CHECK(t.raw_begin() == t.raw_end());

	DOSTRING(L,
		"p1 = {}\n"
		"p2 = {}\n"
		"p3 = {}\n"
		"test_match_object(p1, p2, p3)\n"
		"assert(p1.ret == 1)\n"
		"assert(p2.ret == 2)\n"
		"assert(p3.ret == 3)\n");

#ifndef LUABIND_NO_EXCEPTIONS

	try
	{
		object not_initialized;
		int i = object_cast<int>(not_initialized);
		(void)i;
		BOOST_ERROR("invalid cast succeeded");
	}
	catch(luabind::cast_failed&) {}

#endif

	object not_initialized;
	BOOST_CHECK(!object_cast_nothrow<int>(not_initialized));
}
