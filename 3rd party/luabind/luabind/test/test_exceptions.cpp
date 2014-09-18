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
#include <luabind/luabind.hpp>

namespace {

struct ex : public std::exception
{
    ex(const char* m): msg(m) {}
    virtual const char* what() const throw() { return msg; }
    const char* msg;
};

struct exception_thrower : counted_type<exception_thrower>
{
    exception_thrower() {}
    exception_thrower(int) { throw ex("exception description"); }
    exception_thrower(int, int) { throw "a string exception"; }
    exception_thrower(int, int, int) { throw 10; }
    int f() { throw ex("exception from a member function"); }
    int g() { throw "a string exception"; }
    int h() { throw 10; }
};

} // namespace unnamed

void test_exceptions()
{
	COUNTER_GUARD(exception_thrower);

	lua_state L;

    using namespace luabind;

#ifndef LUABIND_NO_EXCEPTIONS

    module(L)
    [
        class_<exception_thrower>("throw")
            .def(constructor<>())
            .def(constructor<int>())
            .def(constructor<int, int>())
            .def(constructor<int, int, int>())
            .def("f", &exception_thrower::f)
            .def("g", &exception_thrower::g)
            .def("h", &exception_thrower::h)
    ];

    DOSTRING_EXPECTED(L, "a = throw(1)", "exception description");
    DOSTRING_EXPECTED(L, "a = throw(1,1)", "a string exception");
    DOSTRING_EXPECTED(L, "a = throw(1,1,1)", "throw() threw an exception");
    DOSTRING(L, "a = throw()");
    DOSTRING_EXPECTED(L, "a:f()", "exception from a member function");
    DOSTRING_EXPECTED(L, "a:g()", "a string exception");

    DOSTRING_EXPECTED(L, "a:h()", "throw:h() threw an exception");
    DOSTRING_EXPECTED(L, 
        "obj = throw('incorrect', 'parameters', 'constructor')",
        "no constructor of 'throw' matched the arguments "
        "(string, string, string)\n candidates are:\n"
        "throw()\nthrow(number)\nthrow(number, number)\n"
        "throw(number, number, number)\n");

#endif
}

