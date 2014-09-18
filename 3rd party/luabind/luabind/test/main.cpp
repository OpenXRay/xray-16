// Copyright (c) 2004 Daniel Wallin

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
#include <luabind/open.hpp>
#include <luabind/error.hpp>
#include <string>
#include <iostream>

extern "C"
{
    #include "lauxlib.h"
    #include "lualib.h"
}

int pcall_handler(lua_State* L)
{
 /*   lua_Debug dbg;
    string_class str;
    
    std::cout << "stacktrace..\n";
    
    try 
    {
        std::stringstream s;

        for (int i = 0; lua_getstack(L, i, &dbg); ++i)
        {
            lua_getinfo(L, "lS", &dbg);

//            std::cout << i <<    ": " << dbg.currentline << "\n";

            s << dbg.source << " *\n";
        }

        str = s.str();
    }
    catch (...)
    {
        return 1;
    }

//    lua_pushstring(L, str.c_str());
*/
    return 1;
}

lua_state::lua_state()
    : m_state(lua_open())
{
    luaopen_base(m_state);
    lua_baselibopen(m_state);
    m_top = lua_gettop(m_state);
    luabind::open(m_state);
}

lua_state::~lua_state()
{
    BOOST_CHECK(lua_gettop(m_state) == m_top);
    lua_close(m_state);
}

void lua_state::check() const
{
    BOOST_CHECK(lua_gettop(m_state) == m_top);
}

lua_state::operator lua_State*() const
{
    return m_state;
}

void dostring(lua_State* state, char const* str)
{
    lua_pushcclosure(state, &pcall_handler, 0);

    if (luaL_loadbuffer(state, str, std::strlen(str), str))
    {
        string_class err(lua_tostring(state, -1));
        lua_pop(state, 2);
		throw err;
    }

    if (lua_pcall(state, 0, 0, -2))
    {
        string_class err(lua_tostring(state, -1));
        lua_pop(state, 2);
		throw err;
    }

    lua_pop(state, 1);
}

/*
void translate_luabind_error(luabind::error const& e)
{
    BOOST_ERROR("luabind exception caught");
}
*/

// -- test cases ------------------------------------------------------------

void test_exceptions();
void test_lua_classes();
void test_attributes();
void test_held_type();
void test_separation();
void test_scope();
void test_yield();
void test_construction();
void test_type_traits();
void test_implicit_cast();
void test_operators();
void test_const();
void test_object();
void test_policies();
void test_free_functions();
void test_iterator();
void test_abstract_base();

// --------------------------------------------------------------------------

#include <boost/test/included/unit_test_framework.hpp>
#include <boost/test/unit_test.hpp>

using boost::unit_test_framework::test_suite;
//using boost::unit_test_framework::register_exception_translator;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE("luabind test suite");

//    register_exception_translator<luabind::error>(
  //      &translate_luabind_error);

//    test->add(BOOST_TEST_CASE(&test_exceptions));
    test->add(BOOST_TEST_CASE(&test_lua_classes));
    test->add(BOOST_TEST_CASE(&test_attributes));
    test->add(BOOST_TEST_CASE(&test_held_type));
    test->add(BOOST_TEST_CASE(&test_separation));
    test->add(BOOST_TEST_CASE(&test_scope));
    test->add(BOOST_TEST_CASE(&test_construction));
    test->add(BOOST_TEST_CASE(&test_yield));
    test->add(BOOST_TEST_CASE(&test_type_traits));
    test->add(BOOST_TEST_CASE(&test_implicit_cast));
    test->add(BOOST_TEST_CASE(&test_const));
    test->add(BOOST_TEST_CASE(&test_object));
    test->add(BOOST_TEST_CASE(&test_policies));
    test->add(BOOST_TEST_CASE(&test_free_functions));
    test->add(BOOST_TEST_CASE(&test_iterator));
    test->add(BOOST_TEST_CASE(&test_abstract_base));
    test->add(BOOST_TEST_CASE(&test_operators));

    return test;
}

