// Copyright (c) 2004 Daniel Wallin and Arvid Norberg

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

/* ... */

void lua_engineDrawImage(float x, float y, float w, float h, float s1, float t1,
float s2, float t2, int shader) {}

void lua_engineDrawImage(int x, int y, int w, int h, int shader) {}

/* ... */

#include <luabind/luabind.hpp>
#include <luabind/functor.hpp>
#include <luabind/adopt_policy.hpp>

namespace {

    luabind::functor<int> functor_test;
    
    void set_functor(luabind::functor<int> f)
    {
        functor_test = f;
    }

    struct base: counted_type<base>
    {
        int f()
        {
            return 5;
        }
    };

    int f(int x)
    {
        return x + 1;
    }

    int f(int x, int y)
    {
        return x + y;
    }
    
    base* create_base()
    {
        return new base();
    }

    void test_value_converter(const string_class str)
    {
        BOOST_TEST(str == "converted string");
    }

    void test_pointer_converter(const char* const str)
    {
        BOOST_TEST(std::strcmp(str, "converted string") == 0);
    }

    struct copy_me
    {
    };

    void take_by_value(copy_me m)
    {
    }

    int function_should_never_be_called(lua_State* L)
    {
        lua_pushnumber(L, 10);
        return 1;
    }

} // anonymous namespace

namespace luabind { namespace converters
{
    yes_t is_user_defined(by_value<int>);

    int convert_lua_to_cpp(lua_State* L, by_value<int>, int index)
    {
        return static_cast<int>(lua_tonumber(L, index));
    }

    int match_lua_to_cpp(lua_State* L, by_value<int>, int index)
    {
        if (lua_isnumber(L, index)) return 0; else return -1;
    }

    void convert_cpp_to_lua(lua_State* L, const  int& v)
    {
        lua_pushnumber(L, v);
    }

}}

void test_free_functions()
{
    COUNTER_GUARD(base);

    lua_state L;

    using namespace luabind;

    lua_pushstring(L, "f");
    lua_pushcclosure(L, &function_should_never_be_called, 0);
    lua_settable(L, LUA_GLOBALSINDEX);

    DOSTRING(L, "assert(f() == 10)");

    module(L)
    [
        class_<copy_me>("copy_me")
            .def(constructor<>()),
    
        class_<base>("base")
            .def("f", &base::f),


        def("by_value", &take_by_value),

        def("f", (int(*)(int)) &f),
        def("f", (int(*)(int, int)) &f),
        def("create", &create_base, adopt(return_value)),
        def("set_functor", &set_functor)
            
#if !(BOOST_MSVC < 1300)
        ,
        def("test_value_converter", &test_value_converter),
        def("test_pointer_converter", &test_pointer_converter)
#endif
            
    ];

    module(L, "engine")
    [
            /* ... */
        def("drawImage", 
        (void(*)(float,float,float,float,float,float,float,float,int))
            &lua_engineDrawImage),
        def("drawImage", (void(*)(int,int,int,int,int)) &lua_engineDrawImage)
            /* ... */
    ];

    DOSTRING(L,
        "engine.drawImage(0, 0, 0, 0, 0, 0, 0, 0, 0)\n");

    DOSTRING(L,
        "engine.drawImage(0, 0, 0, 0, 0)\n");

    DOSTRING(L,
        "function actions(x) return 0 end\n");

    base c;
    
    call_function<int>(L, "actions", & c);
    
    DOSTRING(L,
        "e = create()\n"
        "assert(e:f() == 5)");

    DOSTRING(L, "assert(f(7) == 8)");

    DOSTRING(L, "assert(f(3, 9) == 12)");

    DOSTRING(L, "set_functor(function(x) return x * 10 end)");

    BOOST_CHECK(functor_test(20) == 200);

    DOSTRING(L, "set_functor(nil)");

    DOSTRING(L, "function lua_create() return create() end");
    base* ptr = call_function<base*>(L, "lua_create") [ adopt(result) ];
    delete ptr;

#if !(BOOST_MSVC < 1300)
    DOSTRING(L, "test_value_converter('converted string')");
    DOSTRING(L, "test_pointer_converter('converted string')");
#endif

    DOSTRING_EXPECTED(L, "f('incorrect', 'parameters')",
        "no match for function call 'f' with the parameters (string, string)\n"
        "candidates are:\n"
        "f(number)\n"
        "f(number, number)\n");

    DOSTRING(L,
        "function functor_test(a) glob = a\n"
        " return 'foobar'\n"
        "end");
    functor<string_class> functor_test = object_cast<functor<string_class> >(get_globals(L)["functor_test"]);
    
    BOOST_CHECK(functor_test(6)[detail::null_type()] == "foobar");
    BOOST_CHECK(object_cast<int>(get_globals(L)["glob"]) == 6);

    functor<string_class> functor_test2 = object_cast<functor<string_class> >(get_globals(L)["functor_test"]);

    BOOST_CHECK(functor_test == functor_test2);

    // this must be reset before the lua state is destructed!
    functor_test.reset();

}

