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

#include <luabind/object.hpp>
#include <luabind/operator.hpp>
#include <luabind/raw_policy.hpp>

using namespace luabind;

template<class U>
static void convert_result(lua_State* L, U const& x)
{
    typename detail::default_policy::generate_converter<
        U const&
        , detail::cpp_to_lua
    >::type cv;

    cv.apply(L, x);
}

template<class Derived, class V>
struct index_adaptor 
{
    typedef typename V::value_type value_type;
    typedef typename V::iterator iterator_type;
    typedef typename V::reverse_iterator reverse_iterator_type;

    static void set(V& v, int index, value_type const& value)
    {
        v[index] = value;
    }

    static value_type const& get(V const& v, int index)
    {
        return v[index];
    }

    static void append(V& v, value_type const& value)
    {
        v.push_back(value);
    }

    static void remove(V& v, value_type const& value)
    {
        v.erase(std::remove(v.begin(), v.end(), value), v.end());
    }

    static void erase(V& v, int index)
    {
        v.erase(v.begin() + index);
    }

    static int len(V const& v)
    {
        return v.size();
    }

    struct slice
    {
        slice(V& v_, int begin_, int end_)
            : v(v_)
            , begin(begin_)
            , end(end_)
        {
        }

        void clear()
        {
            v.erase(v.begin() + begin, v.begin() + end);
        }

        void set(value_type const& value)
        {
            v.erase(v.begin() + begin + 1, v.end());
            v[begin] = value;
        }

        void set_slice(slice const& s)
        {
            v.insert(
                v.begin() + begin
              , s.v.begin() + s.begin
              , s.v.begin() + s.end
            );

            int len = s.end - s.begin;

            v.erase(
                v.begin() + begin + len
              , v.begin() + end + len
            );
        }

        void execute(lua_State* L, object, object)
        {
            if (begin == end)
            {
                lua_pushnil(L);
                return;
            }

            convert_result(L, v[begin++]);
        }

        template<class T>
        static void register_(T& x)
        {
            x.scope [

                class_<slice>("slice")
                    .def("clear", &slice::clear)
                    .def("set", &slice::set)
                    .def("set", &slice::set_slice)
                    .def("__call", &slice::execute, raw(_2))
           ];
        }

        V& v;
        int begin;
        int end;
    };

    static slice make_slice(V& v, int begin, int end)
    {
        return slice(v, begin, end);
    }

    template<class Base>
    struct iterator
    {
        iterator(Base begin, Base end)
            : m_iter(begin)
            , m_end(end)
        {
        }

        void operator()(lua_State* L, object, object)
        {
            if (m_iter == m_end)
            {
                lua_pushnil(L);
                return;
            }

            convert_result(L, *m_iter++);
        }

        template<class T>
        static void register_(T& x, char const* name)
        {
            x.scope [

                class_<iterator>(name)
                    .def(self(other<lua_State*>(), other<object>(), other<object>()), raw(_3))
    
            ];
        }

        Base m_iter;
        Base m_end;
    };

    static iterator<iterator_type> make_iterator(V& v)
    {
        return iterator<iterator_type>(v.begin(), v.end());
    }

    static iterator<reverse_iterator_type> make_reversed_iterator(V& v)
    {
        return iterator<reverse_iterator_type>(v.rbegin(), v.rend());
    }
    
    template<class T>
    T& visit(T const& x_)
    {   
        T& x = const_cast<T&>(x_);

        x.def(constructor<>());

        x.def("len", &len);
        x.def("set", &set);
        x.def("get", &get);
        x.def("append", &append);
        x.def("remove", &remove);
        x.def("erase", &erase);
        x.def("slice", &make_slice);
        x.property("elems", &make_iterator);
        x.property("relems", &make_reversed_iterator);

        slice::register_(x);
        iterator<iterator_type>::register_(x, "iterator");
        iterator<reverse_iterator_type>::register_(x, "reverse_iterator");

        return x;
    }
};

namespace {

    struct abstract
    {
        virtual ~abstract() {}
        virtual string_class hello() = 0;
    }; 

    struct abstract_wrap : abstract, wrap_base
    {
        string_class hello()
        {
            return call_member<string_class>(this, "hello");
        }

        static void default_hello(abstract const&)
        {
            throw std::runtime_error("abstract function");
        }
    };

    string_class call_hello(abstract& a)
    {
        return a.hello();
    }

} // namespace unnamed

#include <vector>

void test_abstract_base()
{
    COUNTER_GUARD(abstract);

    lua_state L;

    module(L)
    [
        class_<abstract, abstract_wrap>("abstract")
            .def(constructor<>())
            .def("hello", &abstract::hello/*, &abstract_wrap::default_hello*/),

        def("call_hello", &call_hello),

        index_adaptor<int, std::vector<int> >().visit(
            class_<std::vector<int> >("int_vec")
        ),

        index_adaptor<int, std::vector<float> >().visit(
            class_<std::vector<float> >("float_vec")
        )
    ];
   
    DOSTRING(L,
        "x = int_vec()\n"
        "x:append(20)\n"
        "x:append(10)\n"
        "x:append(5)\n"
        "x:append(2)\n"
        "x:append(1)\n"
        "x:append(0)\n"
        "print(x:len())\n"
    );

    DOSTRING(L,
        "for i in x.elems do\n"
        "    print(i)\n"
        "end\n"
    );

    DOSTRING(L,
        "for i in x.relems do\n"
        "    print(i)\n"
        "end\n"
    );

    DOSTRING(L,
        "for i in x.elems do\n"
        "    print(i)\n"
        "end\n"
    );

    DOSTRING(L,
        "y = float_vec()\n"
        "for i in x.elems do\n"
        "    y:append(i / 2)\n"
        "end\n"
        
        "for i in y.elems do\n"
        "    print(i)\n"
        "end\n"
    );

    DOSTRING(L, "print('slice test')\n");

    DOSTRING(L,
        "y:slice(1,5):set(y:slice(2,4))\n"
    );

    DOSTRING(L,
        "for i in y.elems do\n"
        "    print(i)\n"
        "end\n"
    );
    
    DOSTRING_EXPECTED(L,
        "x = abstract()\n"
        "x:hello()\n"
      , "pure virtual function called");

    DOSTRING_EXPECTED(L, 
        "call_hello(x)\n"
      , "pure virtual function called");
    
    DOSTRING(L,
        "class 'concrete' (abstract)\n"
        "  function concrete:__init()\n"
        "      super()\n"
        "  end\n"

        "  function concrete:hello()\n"
        "      return 'hello from lua'\n"
        "  end\n");

    DOSTRING(L,
        "y = concrete()\n"
        "y:hello()\n");

    DOSTRING(L, "call_hello(y)\n");
}

