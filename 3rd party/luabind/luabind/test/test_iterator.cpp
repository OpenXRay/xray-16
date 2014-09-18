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

#include <vector>
#include <luabind/iterator_policy.hpp>

namespace
{
	struct IteratorTest: counted_type<IteratorTest>
	{
		IteratorTest()
		{
			names.push_back("first one");
			names.push_back("foobar");
			names.push_back("last one");
		}

		std::vector<string_class> names;
	};
	
} // anonymous namespace

void test_iterator()
{
    COUNTER_GUARD(IteratorTest);

	lua_state L;

	using namespace luabind;

	module(L)
	[
		class_<IteratorTest>("A")
			.def(constructor<>())
			.def_readonly("names", &IteratorTest::names, return_stl_iterator)
	];

	DOSTRING(L,
		"a = A()\n"
		"b = ''\n"
		"for name in a.names do\n"
		"	b = b .. ' ' .. name\n"
		"end\n"
		"assert(b == ' first one foobar last one')");
}
