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
#include <luabind/luabind.hpp>
#include <luabind/wrapper_base.hpp>
#include <luabind/adopt_policy.hpp>
#include <boost/shared_ptr.hpp>

namespace luabind {

    template<class T>
    T* get_pointer(boost::shared_ptr<T>& p) { return p.get(); }

    template<class A>
    boost::shared_ptr<const A>* get_const_holder(boost::shared_ptr<A>*)
    {
        return 0;
    }
}

using namespace luabind;

namespace {

struct A : counted_type<A>
{
    virtual ~A() {}

    virtual string_class f()
    { return "A:f()"; }

    virtual string_class g() const
    { return "A:g()"; }
};

struct A_wrap : A, wrap_base
{
    string_class f()
    { 
        return call_member<string_class>(this, "f"); 
    }

    static string_class default_f(A* p)
    { return p->A::f(); }

    string_class g() const
    {
        return call_member<string_class>(this, "g");
    }

    static string_class default_g(A const* p)
    { return p->A::g(); }
};

struct B : A
{
    virtual string_class f()
    { return "B:f()"; }
};

struct B_wrap : B, wrap_base
{
    virtual string_class f()
    { return call_member<string_class>(this, "f"); }

    static string_class default_f(B* p)
    { return p->B::f(); }

    virtual string_class g() const
    { return call_member<string_class>(this, "g"); }

    static string_class default_g(B const* p)
    { return p->B::g(); }
};


struct base : counted_type<base>
{
    virtual ~base() {}

    virtual string_class f()
    {
        return "base:f()";
    }

    virtual string_class g() const { return ""; }
};

base* filter(base* p) { return p; }

struct base_wrap : base, wrap_base
{
    virtual string_class f()
    {
		return call_member<string_class>(this, "f");
    }

    static string_class default_f(base* p)
    {
        return p->base::f();
    }

	virtual string_class g() const
	{
		return call_member<string_class>(this, "g");
	}
};

struct simple_class : counted_type<simple_class>
{
    static int feedback;

    void f()
    {
        feedback = 1;
    }

    void f(int, int) {}
    void f(string_class a)
    {
        const char str[] = "foo\0bar";
        if (a == string_class(str, sizeof(str)-1))
            feedback = 2;
    }

    string_class g()
    {
        const char str[] = "foo\0bar";
        return string_class(str, sizeof(str)-1);
    }

};

struct T_ // vc6.5, don't name your types T!
{
	int f(int) { return 1; }
};

struct U : T_
{
	int g() { return 3; }
	int f(int, int) { return 2; }
};

int simple_class::feedback = 0;

struct dHinge2Joint
{
	void setParam( int param, float value )
	{
	}
};

} // namespace unnamed

#include <iostream>

void test_lua_classes()
{
    COUNTER_GUARD(A);
    COUNTER_GUARD(base);
	COUNTER_GUARD(simple_class);

	lua_state L;

    module(L)
    [
		class_<A, A_wrap, boost::shared_ptr<A> >("A")
            .def(constructor<>())
            .def("f", &A::f, &A_wrap::default_f)
			.def("g", &A::g, &A_wrap::default_g),

		class_<B, A, B_wrap, boost::shared_ptr<A> >("B")
            .def(constructor<>())
            .def("f", &B::f, &B_wrap::default_f)
			.def("g", &B::g, &B_wrap::default_g),

        def("filter", &filter),

        class_<base, base_wrap>("base")
            .def(constructor<>())
            .def("f", &base::f, &base_wrap::default_f)
			.def("g", &base::g),

		class_<T_>("T")
			.def("f", &T_::f),

		class_<U, T_>("U")
            .def(constructor<>())
			.def("f", &U::f)
			.def("g", &U::g),

		class_<dHinge2Joint>("dHinge2Joint")
			.def(constructor<>())
			.def("setParam", &dHinge2Joint::setParam)
	];

	DOSTRING(L,
		"a = dHinge2Joint()\n"
		"a:setParam(0, 0)\n");
	
	DOSTRING(L,
		"u = U()\n"
		"assert(u:f(0) == 1)\n"
		"assert(u:f(0,0) == 2)\n"
		"assert(u:g() == 3)\n");

	DOSTRING(L,
		"function base:fun()\n"
		"  return 4\n"
		"end\n"
		"ba = base()\n"
		"assert(ba:fun() == 4)");

    DOSTRING(L, 
        "class 'derived' (base)\n"
        "  function derived:__init() super() end\n"
        "  function derived:f()\n"
        "    return 'derived:f() : ' .. base.f(self)\n"
        "  end\n"

		"class 'empty_derived' (base)\n"
		"  function empty_derived:__init() super() end\n"

		"class 'C' (B)\n"
        "  function C:__init() super() end\n"
		"  function C:f() return 'C:f()' end\n"

        "function make_derived()\n"
        "  return derived()\n"
        "end\n"
		
        "function make_empty_derived()\n"
        "  return empty_derived()\n"
        "end\n"
		
		"function adopt_ptr(x)\n"
		"  a = x\n"
		"end\n");
		
	DOSTRING(L,
		"function gen_error()\n"
		"  assert(0 == 1)\n"
		"end\n");

	DOSTRING(L,
		"a = A()\n"
		"b = B()\n"
		"c = C()\n"

		"assert(c:f() == 'C:f()')\n"
		"assert(b:f() == 'B:f()')\n"
		"assert(a:f() == 'A:f()')\n"
		"assert(b:g() == 'A:g()')\n"
		"assert(c:g() == 'A:g()')\n"

		"assert(C.f(c) == 'C:f()')\n"
		"assert(B.f(c) == 'B:f()')\n"
		"assert(A.f(c) == 'A:f()')\n"
		"assert(A.g(c) == 'A:g()')\n");

#ifndef LUABIND_NO_EXCEPTONS
	{
		LUABIND_CHECK_STACK(L);

		try { call_function<int>(L, "gen_error"); }
		catch (luabind::error&)
		{
            bool result(
                lua_tostring(L, -1) == string_class("[string \"function "
                    "gen_error()...\"]:2: assertion failed!"));
			BOOST_CHECK(result);
			lua_pop(L, 1);
		}
	}

	{
		A a;

		DOSTRING(L, "function test_ref(x) end");
		call_function<void>(L, "test_ref", boost::ref(a));
	}

	{
		LUABIND_CHECK_STACK(L);

		try { call_function<void>(L, "gen_error"); }
		catch (luabind::error&)
		{
            bool result(
                lua_tostring(L, -1) == string_class("[string \"function "
                    "gen_error()...\"]:2: assertion failed!"));
			BOOST_CHECK(result);
			lua_pop(L, 1);
		}
	}

	{
		LUABIND_CHECK_STACK(L);

		try { call_function<void*>(L, "gen_error") [ adopt(result) ]; }
		catch (luabind::error&)
		{
            bool result(
                lua_tostring(L, -1) == string_class("[string \"function "
                    "gen_error()...\"]:2: assertion failed!"));
			BOOST_CHECK(result);
			lua_pop(L, 1);
		}
	}

#endif
	
	base* ptr;
	{
		LUABIND_CHECK_STACK(L);

		BOOST_CHECK_NO_THROW(
			object a = get_globals(L)["ba"];
			BOOST_CHECK(call_member<int>(a, "fun") == 4);
		);
	}

	{
		LUABIND_CHECK_STACK(L);

		object make_derived = get_globals(L)["make_derived"];
		BOOST_CHECK_NO_THROW(
			call_function<void>(make_derived)
			);
	}

	luabind::auto_ptr<base> own_ptr;
	{
		LUABIND_CHECK_STACK(L);

		BOOST_CHECK_NO_THROW(
		    own_ptr = luabind::auto_ptr<base>(
                call_function<base*>(L, "make_derived") [ adopt(result) ])
			);
	}

	// make sure the derived lua-part is still referenced by
	// the adopted c++ pointer
	DOSTRING(L,
		"collectgarbage()\n"
		"collectgarbage()\n"
		"collectgarbage()\n"
		"collectgarbage()\n"
		"collectgarbage()\n");

    BOOST_CHECK_NO_THROW(
        BOOST_CHECK(own_ptr->f() == "derived:f() : base:f()")
    );
	own_ptr = luabind::auto_ptr<base>();

	// test virtual functions that are not overridden by lua
    BOOST_CHECK_NO_THROW(
        own_ptr = luabind::auto_ptr<base>(
            call_function<base*>(L, "make_empty_derived") [ adopt(result) ])
        );
    BOOST_CHECK_NO_THROW(
        BOOST_CHECK(own_ptr->f() == "base:f()")
	);
    BOOST_CHECK_NO_THROW(
        call_function<void>(L, "adopt_ptr", own_ptr.get()) [ adopt(_1) ]
    );
	own_ptr.release();

	// test virtual functions that are overridden by lua
    BOOST_CHECK_NO_THROW(
        ptr = call_function<base*>(L, "derived")
    );

    BOOST_CHECK_NO_THROW(
        BOOST_CHECK(ptr->f() == "derived:f() : base:f()")
    );

	// test virtual function dispatch from within lua
	DOSTRING(L,
		"a = derived()\n"
        "b = filter(a)\n"
        "assert(b:f() == 'derived:f() : base:f()')\n");

	// test back references
	DOSTRING(L,
		"a = derived()\n"
		"assert(a == filter(a))\n");

	typedef void(simple_class::*f_overload1)();
	typedef void(simple_class::*f_overload2)(int, int);
	typedef void(simple_class::*f_overload3)(string_class);

    module(L)
    [
        class_<simple_class>("simple")
            .def(constructor<>())
			.def("f", (f_overload1)&simple_class::f)
			.def("f", (f_overload2)&simple_class::f)
			.def("f", (f_overload3)&simple_class::f)
			.def("g", &simple_class::g)
    ];

    DOSTRING(L,
        "class 'simple_derived' (simple)\n"
        "  function simple_derived:__init() super() end\n"
        "a = simple_derived()\n"
        "a:f()\n");
    BOOST_CHECK(simple_class::feedback == 1);

    DOSTRING(L, "a:f('foo\\0bar')");
    BOOST_CHECK(simple_class::feedback == 2);

	DOSTRING(L,
		"b = simple_derived()\n"
		"a.foo = 'yo'\n"
		"assert(b.foo == nil)");

	DOSTRING(L,
		"x = base()\n"
		"y = base()\n"
		"x.foo = 'yo'\n"
		"assert(x.foo == 'yo')"
		"assert(y.foo == nil)");

	DOSTRING(L,
		"simple_derived.foobar = 'yi'\n"
		"assert(b.foobar == 'yi')\n"
		"assert(a.foobar == 'yi')\n");

    simple_class::feedback = 0;

    DOSTRING_EXPECTED(L, "a:f('incorrect', 'parameters')",
        "no overload of  'simple:f' matched the arguments "
        "(simple_derived, string, string)\ncandidates are:\n"
        "simple:f()\nsimple:f(number, number)\nsimple:f(string)\n");

    DOSTRING(L, "if a:g() == \"foo\\0bar\" then a:f() end");
    BOOST_CHECK(simple_class::feedback == 1);
}

