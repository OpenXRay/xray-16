+++++++++
 luabind
+++++++++

:Author: Daniel Wallin, Arvid Norberg
:Copyright: Copyright Daniel Wallin, Arvid Norberg 2003.
:Date: $Date: 2004/08/07 10:18:10 $
:Revision: $Revision: 1.19.2.22 $
:License: Permission is hereby granted, free of charge, to any person obtaining a
          copy of this software and associated documentation files (the "Software"),
          to deal in the Software without restriction, including without limitation
          the rights to use, copy, modify, merge, publish, distribute, sublicense,
          and/or sell copies of the Software, and to permit persons to whom the
          Software is furnished to do so, subject to the following conditions:

          The above copyright notice and this permission notice shall be included
          in all copies or substantial portions of the Software.

          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
          ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
          TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
          PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
          SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
          ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
          ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
          OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
          OR OTHER DEALINGS IN THE SOFTWARE.


.. _MIT license: http://www.opensource.org/licenses/mit-license.php
.. _Boost: http://www.boost.org

Note: This library is currently in public beta phase. This documentation
should be considered beta as well. Please report any grammatical 
corrections/spelling corrections.

.. contents::
    :depth: 2
.. section-numbering::

Introduction
============

Luabind is a library that helps you create bindings between C++ and Lua. It has
the ability to expose functions and classes, written in C++, to Lua. It will
also supply the functionality to define classes in Lua and let them derive from
other Lua classes or C++ classes. Lua classes can override virtual functions
from their C++ base classes. It is written towards Lua 5.0, and does not work
with Lua 4.

It is implemented utilizing template meta programming. That means that you
don't need an extra preprocess pass to compile your project (it is done by the
compiler). It also means you don't (usually) have to know the exact signatureof
each function you register, since the library will generate code depending on
the compile-time type of the function (which includes the signature). The main
drawback of this approach is that the compilation time will increase for the
file that does the registration, it is therefore recommended that you register
everything in the same cpp-file.

luabind is released under the terms of the `MIT license`_.

We are very interested in hearing about projects that use luabind, please let
us know about your project.


Features
========

Luabind supports:

 - Overloaded free functions 
 - C++ classes in Lua 
 - Overloaded member functions 
 - Operators 
 - Properties 
 - Enums 
 - Lua functions in C++ 
 - Lua classes in C++ 
 - Lua classes (single inheritance) 
 - Derives from Lua or C++ classes 
 - Override virtual functions from C++ classes 
 - Implicit casts between registered types 
 - Best match signature matching 
 - Return value policies and parameter policies 


Portability
===========

Luabind has been tested to work on the following compilers:

 - Visual Studio 7.1 
 - Visual Studio 7.0 
 - Visual Studio 6.0 (sp 5) 
 - Intel C++ 6.0 (Windows) 
 - GCC 2.95.3 (cygwin) 
 - GCC 3.0.4 (Debian/Linux) 
 - GCC 3.1 (SunOS 5.8) 
 - GCC 3.2 (cygwin) 
 - GCC 3.3.1 (cygwin) 

It has been confirmed not to work with:

 - GCC 2.95.2 (SunOS 5.8) 

Metrowerks 8.3 (Windows) compiles but fails the const-test. This 
means that const member functions are treated as non-const member 
functions.

If you have tried luabind with a compiler not listed here, let us know 
your result with it.


Building luabind
================

To keep down the compilation-time luabind is built as a library. This means you
have to either build it and lika against it, or include its source files in
your project. You also have to make sure the luabind directory is somewhere in
your compiler's include path. It requires `Boost`_ 1.31.0 to be installed (only
boost headers). It also requires that Lua is installed.

The official way of building luabind is with `Boost.Build V2`_. To properly build
luabind with Boost.Build you need to set two environment variables:

BOOST_ROOT
    Point this to your Boost installation.

LUA_PATH
    Point this to your Lua directory. The build system will assume that the
    include and library files are located in ``$(LUA_PATH)/include/`` and
    ``$(LUA_PATH)/lib/.``

For backward compatibility, there is also a makefile in the root-directory that
will build the library and the test program. If you are using a UNIX-system (or
cygwin) they will make it easy to build luabind as a static library. If you are
using Visual Studio it may be easier to include the files in the src directory
in your project.

When building luabind you have several options that may streamline the library
to better suit your needs. It is extremely important that your application has
the same settings as the library was built with. The available options are
found in the `Configuration`_ section.

If you want to change the settings to differ from the default, it's recommended
that you define the settings on the commandline of all your files (in the
project settings in visual studio).

.. _`Boost.Build V2`: http://www.boost.org/tools/build/v2/index_v2.html


Basic usage
===========

To use luabind, you must include ``lua.h`` and luabind's main header file::

    extern "C"
    {
        #include "lua.h"
    }

    #include <luabind/luabind.hpp>

This includes support for both registering classes and functions. If you just
want to have support for functions or classes you can include
``luabind/function.hpp`` and ``luabind/class.hpp`` separately::

    #include <luabind/function.hpp>
    #include <luabind/class.hpp>

The first thing you need to do is to call ``luabind::open(lua_State*)`` which
will register the functions to create classes from Lua, and initialize some
state-global structures used by luabind. If you don't call this function you
will hit asserts later in the library. There is no corresponding close function
because once a class has been registered in Lua, there really isn't any good
way to remove it. Partly because any remaining instances of that class relies
on the class being there. Everything will be cleaned up when the state is
closed though.

.. Isn't this wrong? Don't we include lua.h using lua_include.hpp ?

Luabind's headers will never include ``lua.h`` directly, but through
``<luabind/lua_include.hpp>``. If you for some reason need to include another
Lua header, you can modify this file.


Hello world
-----------

::

    #include <iostream>
    #include <luabind/luabind.hpp>

    void greet()
    {
        std::cout << "hello world!\n";
    }

    extern "C" int init(lua_State* L)
    {
        using namespace luabind;

        open(L);

        module(L)
        [
            def("greet", &greet)
        ];

        return 0;
    }

::

    Lua 5.0  Copyright (C) 1994-2003 Tecgraf, PUC-Rio
    > loadlib('hello_world.dll', 'init')()
    > greet()
    Hello world!
    >

Scopes
======

Everything that gets registered in Lua is registered in a namespace (Lua
tables) or in the global scope (called module). All registrations must be
surrounded by its scope. To define a module, the ``luabind::module`` class is
used. It is used like this::

    module(L)
    [
        // declarations
    ];

This will register all declared functions or classes in the global namespace in
Lua. If you want to have a namespace for your module (like the standard
libraries) you can give a name to the constructor, like this::

    module(L, "my_library")
    [
        // declarations
    ];

Here all declarations will be put in the my_library table.

If you want nested namespaces you can use the ``luabind::namespace_`` class. It
works exactly as ``luabind::module`` except that it doesn't take a lua_State*
in it's constructor. An example of its usage could look like this::

    module(L, "my_library")
    [
        // declarations

        namespace_("detail")
        [
            // library-private declarations
        ]
    ];

As you might have figured out, the following declarations are equivalent::

    module(L)
    [
        namespace_("my_library")
        [
            // declarations
        ]

    ];

::
    
    module(L, "my_library")
    [
        // declarations
    ];

Each declaration must be separated by a comma, like this::

    module(L)
    [
        def("f", &f),
        def("g", &g),
        class_<A>("A")
            .def(constructor<int, int>),
        def("h", &h)
    ];


More about the actual declarations in the `Binding functions to Lua`_ and
`Binding classes to Lua`_ sections.

A word of caution, if you are in really bad need for performance, putting your
functions in tables will increase the lookup time.


Binding functions to Lua
========================

To bind functions to Lua you use the function ``luabind::def()``. It has the
following synopsis::

    template<class F, class policies>
    void def(const char* name, F f, const Policies&);

- name is the name the function will have within Lua. 
- F is the function pointer you want to register. 
- The Policies parameter is used to describe how parameters and return values 
  are treated by the function, this is an optional parameter. More on this in 
  the `policies`_ section.

An example usage could be if you want to register the function ``float
std::sin(float)``::

    module(L)
    [
        def("sin", &std::sin)
    ];

Overloaded functions
--------------------

If you have more than one function with the same name, and want to register
them in Lua, you have to explicitly give the signature. This is to let C++ know
which function you refer to. For example, if you have two functions, ``int
f(const char*)`` and ``void f(int)``. ::

    module(L)
    [
        def("f", (int(*)(const char*)) &f),
        def("f", (void(*)(int)) &f)
    ];

Signature matching
------------------

luabind will generate code that checks the Lua stack to see if the values there
can match your functions' signatures. It will handle implicit typecasts between
derived classes, and it will prefer matches with the least number of implicit
casts. In a function call, if the function is overloaded and there's no
overload that match the parameters better than the other, you have an
ambiguity. This will spawn a run-time error, stating that the function call is
ambiguous. A simple example of this is to register one function that takes an
int and one that takes a float. Since Lua don't distinguish between floats and
integers, both will always match.

Since all overloads are tested, it will always find the best match (not the
first match). This also means that it can handle situations where the only
difference in the signature is that one member function is const and the other
isn't. 

.. sidebar:: Ownership transfer

   To correctly handle ownership transfer, create_a() would need an adopt
   return value policy. More on this in the `Policies`_ section.

For example, if the following function and class is registered:

::
   
    struct A
    {
        void f();
        void f() const;
    };

    const A* create_a();

    struct B: A {};
    struct C: B {};

    void g(A*);
    void g(B*);

And the following Lua code is executed::

    a1 = create_a()
    a1:f() -- the const version is called

    a2 = A()
    a2:f() -- the non-const version is called

    a = A()
    b = B()
    c = C()

    g(a) -- calls g(A*)
    g(b) -- calls g(B*)
    g(c) -- calls g(B*)


Calling Lua functions
---------------------

To call a Lua function, you can either use ``call_function()``,
an ``object`` or ``functor``.

::

    template<class Ret>
    Ret call_function(lua_State* L, const char* name, ...)
    template<class Ret>
    Ret call_function(object const& obj, ...)

There are two overloads of the ``call_function`` function, one that calls
a function given its name, and one that takes an object that should be a Lua
value that can be called as a function.

The overload that takes a name can only call global Lua functions. The ...
represents a variable number of parameters that are sent to the Lua
function. This function call may throw ``luabind::error`` if the function
call fails.

The return value isn't actually Ret (the template parameter), but a proxy
object that will do the function call. This enables you to give policies to the
call. You do this with the operator[]. You give the policies within the
brackets, like this::

    int ret = call_function<int>(
        L 
      , "a_lua_function"
      , new complex_class()
    )[ adopt(_1) ];

If you want to pass a parameter as a reference, you have to wrap it with the
`Boost.Ref`__.

__ http://www.boost.org/doc/html/ref.html

Like this::

	int ret = call_function(L, "fun", boost::ref(val));


Using Lua threads
-----------------

To start a Lua thread, you have to call ``lua_resume()``, this means that you
cannot use the previous function ``call_function()`` to start a thread. You have
to use

::

    template<class Ret>
    Ret resume_function(lua_State* L, const char* name, ...)
    template<class Ret>
    Ret resume_function(object const& obj, ...)

and

::

    template<class Ret>
    Ret resume(lua_State* L, ...)

The first time you start the thread, you have to give it a function to execute. i.e. you
have to use ``resume_function``, when the Lua function yeilds, it will return the first
value passed in to ``lua_yield()``. When you want to continue the execution, you just call
``resume()`` on your ``lua_State``, since it's already executing a function, you don't pass
it one. The parameters to ``resume()`` will be returned by ``yield()`` on the Lua side.

For yielding C++-functions (without the support of passing data back and forth between the
Lua side and the c++ side), you can use the yield_ policy.

With the overload of ``resume_function`` that takes an object_, it is important that the
object was constructed with the thread as its ``lua_State*``. Like this:

.. parsed-literal::

	lua_State* thread = lua_newthread(L);
	object fun = get_global(**thread**)["my_thread_fun"];
	resume_function(fun);


Binding classes to Lua
======================

To register classes you use a class called ``class_``. Its name is supposed to
resemble the C++ keyword, to make it look more intuitive. It has an overloaded
member function ``def()`` that is used to register member functions, operators,
constructors, enums and properties on the class. It will return its
this-pointer, to let you register more members directly.

Let's start with a simple example. Consider the following C++ class::

    class testclass
    {
    public:
        testclass(const std::string& s): m_string(s) {}
        void print_string() { std::cout << m_string << "\n"; }

    private:
        std::string m_string;
    };

To register it with a Lua environment, write as follows (assuming you are using
namespace luabind)::

    module(L)
    [
        class_<testclass>("testclass")
            .def(constructor<const std::string&>())
            .def("print_string", &testclass::print_string)
    ];

This will register the class with the name testclass and constructor that takes
a string as argument and one member function with the name ``print_string``.

::

    Lua 5.0  Copyright (C) 1994-2003 Tecgraf, PUC-Rio
    > a = testclass('a string')
    > a:print_string()
    a string

It is also possible to register free functions as member functions. The
requirement on the function is that it takes a pointer, const pointer,
reference or const reference to the class type as the first parameter. The rest
of the parameters are the ones that are visible in Lua, while the object
pointer is given as the first parameter. If we have the following C++ code::

    struct A
    {
        int a;
    };

    int plus(A* o, int v) { return o->a + v; }

You can register ``plus()`` as if it was a member function of A like this::

    class_<A>("A")
        .def("plus", &plus)

``plus()`` can now be called as a member function on A with one parameter, int.
If the object pointer parameter is const, the function will act as if it was a
const member function (it can be called on const objects).


Properties
----------

To register a global data member with a class is easily done. Consider the
following class::

    struct A
    {
        int a;
    };

This class is registered like this::

    module(L)
    [
        class_<A>("A")
            .def_readwrite("a", &A::a)
    ];

This gives read and write access to the member variable ``A::a``. It is also
possible to register attributes with read-only access::

    module(L)
    [
        class_<A>("A")
        .def_readonly("a", &A::a)
    ];

You can also register getter and setter functions and make them look as if they
were a public data member. Consider the following class::

    class A
    {
    public:
        void set_a(int x) { a = x; }
        int get_a() const { return a; }

    private:
        int a;
    };

It can be registered as if it had a public data member a like this::

    class_<A>("A")
        .property("a", &A::get_a, &A::set_a)

This way the ``get_a()`` and ``set_a()`` functions will be called instead of
just writing  to the data member. If you want to make it read only you can just
omit the last parameter.


Enums
-----

If your class contains enumerated constants (enums), you can register them as
well to make them available in Lua. Note that they will not be type safe, all
enums are integers in Lua, and all functions that takes an enum, will accept
any integer. You register them like this::

    module(L)
    [
        class_<A>("A")
            .enum_("constants")
            [
                value("my_enum", 4),
                value("my_2nd_enum", 7),
                value("another_enum", 6)
            ]
    ];

In Lua they are accessed like any data member, except that they are read-only
and reached on the class itself rather than on an instance of the class.

::

    Lua 5.0  Copyright (C) 1994-2003 Tecgraf, PUC-Rio
    > print(A.my_enum)
    4
    > print(A.another_enum)
    6


Operators
---------

The mechanism for registering operators on your class is pretty simple. You use
a global name ``luabind::self`` to refer to the class itself and then you just
write the operator expression inside the ``def()`` call. This class::

    struct vec
    {
        vec operator+(int s);
    };

Is registered like this:

.. parsed-literal::

    module(L)
    [
        class_<vec>("vec")
            .def(**self + int()**)
    ];

This will work regardless if your plus operator is defined inside your class or
as a free function.

If you operator is const (or, when defined as a free function, takes a const
reference to the class itself) you have to use ``const_self`` instead of
``self``. Like this:

.. parsed-literal::

    module(L)
    [
        class_<vec>("vec")
            .def(**const_self** + int())
    ];

The operators supported are those available in Lua:

.. parsed-literal::

    +    -    \*    /    ==    !=    <    <=    >    >=

This means, no in-place operators. The equality operator (``==``) has a little
hitch; it will not be called if the references are equal. This means that the
``==`` operator has to do pretty much what's it's expected to do.

In the above example the other operand type is instantiated by writing
``int()``. If the operand type is a complex type that cannot easily be
instantiated you can wrap the type in a class called ``other<>``. For example:

To register this class, we don't want to instantiate a string just to register
the operator.

::

    struct vec
    {
        vec operator+(std::string);
    };

Instead we use the ``other<>`` wrapper like this:

.. parsed-literal::

    module(L)
    [
        class_<vec>("vec")
            .def(self + **other<std::string>()**)
    ];

To register an application operator:

.. parsed-literal::

    module(L)
    [
        class_<vec>("vec")
            .def( **self(int())** )
    ];

There's one special operator. In Lua it's called ``__tostring``, it's not
really an operator. It is used for converting objects to strings in a standard
way in Lua. If you register this functionality, you will be able to use the lua
standard function ``tostring()`` for converting you object to a string.

To implement this operator in C++ you should supply an ``operator<<`` for
ostream. Like this example:

.. parsed-literal::

    class number {};
    std::ostream& operator<<(std::ostream&, number&);

    ...
    
    module(L)
    [
        class_<number>("number")
            .def(**tostring(self)**)
    ];


Nested scopes and static functions
----------------------------------

It is possible to add nested scopes to a class. This is useful when you need 
to wrap a nested class, or a static function.

.. parsed-literal::

    class_<foo>("foo")
        .def(constructor<>()
        **.scope
        [
            class_<inner>("nested"),
            def("f", &f)
        ]**;

It's also possible to add namespaces to classes using the same syntax.


Derived classes
---------------
  
If you want to register classes that derives from other classes, you can
specify a template parameter ``bases<>`` to the ``class_`` instantiation. The
following hierarchy::
   
    struct A {};
    struct B : A {};

Would be registered like this::

    module(L)
    [
        class_<A>("A"),
        class_<B, A>("B")
    ];

If you have multiple inheritance you can specify more than one base. If B would
also derive from a class C, it would be registered like this::

    module(L)
    [
        class_<B, bases<A, C> >("B")
    ];

Note that you can omit ``bases<>`` when using single inheritance.

.. note::
   If you don't specify that classes derive from each other, luabind will not
   be able to implicitly cast pointers between the types.


Smart pointers
--------------

When you register a class you can tell luabind that all instances of that class
should be held by some kind of smart pointer (boost::shared_ptr for instance).
You do this by giving the holder type as an extra template parameter to
the ``class_`` you are constructing, like this::

    module(L)
    [
        class_<A, boost::shared_ptr<A> >("A")
    ];

You also have to supply two functions for your smart pointer. One that returns
the type of const version of the smart pointer type (boost::shared_ptr<const A>
in this case). And one function that extracts the raw pointer from the smart
pointer. The first function is needed because luabind has to allow the
non-const -> conversion when passing values from Lua to C++. The second
function is needed when Lua calls member functions on held types, the this
pointer must be a raw pointer, it is also needed to allow the smart_pointer ->
raw_pointer conversion from Lua to C++. They look like this::

    namespace luabind {

        template<class T>
        T* get_pointer(boost::shared_ptr<T> const& p) 
        {
            return p.get(); 
        }

        template<class A>
        boost::shared_ptr<const A>* 
        get_const_holder(boost::shared_ptr<A>*)
        {
            return 0;
        }
    }

The conversion that works are (given that B is a base class of A):

.. topic:: From Lua to C++

    ========================= ========================
    Source                    Target
    ========================= ========================
    ``holder_type<A>``        ``A*``
    ``holder_type<A>``        ``B*``
    ``holder_type<A>``        ``A const*``
    ``holder_type<A>``        ``B const*``
    ``holder_type<A>``        ``holder_type<A>``
    ``holder_type<A>``        ``holder_type<A const>``
    ``holder_type<A const>``  ``A const*``
    ``holder_type<A const>``  ``B const*``
    ``holder_type<A const>``  ``holder_type<A const>``
    ========================= ========================

.. topic:: From C++ to Lua

    =============================== ========================
    Source                          Target
    =============================== ========================
    ``holder_type<A>``              ``holder_type<A>``
    ``holder_type<A const>``        ``holder_type<A const>``
    ``holder_type<A> const&``       ``holder_type<A>``
    ``holder_type<A const> const&`` ``holder_type<A const>``
    =============================== ========================

When using a holder type, it can be useful to know if the pointer is valid. For
example when using std::auto_ptr, the holder will be invalidated when passed as
a parameter to a function. For this purpose there is a member of all object
instances in luabind: ``__ok``. ::

    struct X {};
    void f(std::auto_ptr<X>);

    module(L)
    [
        class_<X, std::auto_ptr<X> >("X")
            .def(constructor<>()),

        def("f", &f)
    ];

::
    
    Lua 5.0  Copyright (C) 1994-2003 Tecgraf, PUC-Rio
    > a = X()
    > f(a)
    > print a.__ok
    false


Object
======

Since functions have to be able to take Lua values (of variable type) we need a
wrapper around them. This wrapper is called ``luabind::object``. If the
function you register takes an object, it will match any Lua value. To use it,
you need to include ``luabind/object.hpp``.

.. topic:: Synopsis

    .. parsed-literal::

        class object
        {
        public:
            class iterator;
            class raw_iterator;
            class array_iterator;

            template<class T>
            object(lua_State\*, T const& value);
            object(object const&);
            object(lua_State\*);
            object();

            ~object();
            
            iterator begin() const;
            iterator end() const;
            raw_iterator raw_begin() const;
            raw_iterator raw_end() const;
            array_iterator abegin() const;
            array_iterator aend() const;

            void set();
            lua_State\* lua_state() const;
            void pushvalue() const;
            bool is_valid() const;
            operator safe_bool_type() const;

            template<class Key>
            *implementation-defined* operator[](Key const&);

            template<class Key>
            object at(Key const&) const;

            template<class Key>
            object raw_at(Key const&) const;

            template<class T>
            object& operator=(T const&);
            object& operator=(object const&);

            template<class T>
            bool operator==(T const&) const;
            bool operator==(object const&) const;
            bool operator<(object const&) const;
            bool operator<=(object const&) const;
            bool operator>(object const&) const;
            bool operator>=(object const&) const;
            bool operator!=(object const&) const;

            void swap(object&);
            int type() const;

            *implementation-defined* operator()();

            template<class A0>
            *implementation-defined* operator()(A0 const& a0);

            template<class A0, class A1>
            *implementation-defined* operator()(A0 const& a0, A1 const& a1);

            /\* ... \*/
        };

When you have a Lua object, you can assign it a new value with the assignment
operator (=). When you do this, the ``default_policy`` will be used to make the
conversion from C++ value to Lua. If your ``luabind::object`` is a table you
can access its members through the operator[] or the iterators. The value
returned from the operator[] is a proxy object that can be used both for
reading and writing values into the table (using operator=). Note that it is
impossible to know if a Lua value is indexable or not (lua_gettable doesn't
fail, it succeeds or crashes). This means that if you're trying to index
something that cannot be indexed, you're on your own. Lua will call its
``panic()`` function (you can define your own panic function using
``lua_setpanicf``). The ``at()`` and ``raw_at()`` functions returns the value at
the given table position (like operator[] but only for reading).

The ordinary ``object::iterator`` uses lua_gettable to extract the values from
the table, the standard way that will invoke metamethods if any. The
``object::raw_iterator`` uses lua_rawget and ``object::array_iterator`` uses
lua_rawgeti. The latter will only iterate over numberical keys starting at 1
and continue until the first nil value.

The ``lua_state()`` function returns the Lua state where this object is stored.
If you want to manipulate the object with Lua functions directly you can push
it onto the Lua stack by calling ``pushvalue()``. And set the object's value by
calling ``set()``, which will pop the top value from the Lua stack and assign
it to the object.

The operator== will call lua_equal() on the operands and return its result.

The ``type()`` member function will return the Lua type of the object. It will
return the same values as lua_type().

The ``is_valid()`` function tells you whether the object has been initialized
or not. When created with its default constructor, objects are invalid. To make
an object valid, you can assign it a value. If you want to invalidate an object
you can simply assign it an invalid object.

.. So what? implementation detail, leave out of docs
  isn't really an implicit cast to bool, but an implicit cast
  to a member pointer, since member pointers don't have any arithmetic operators
  on them (which can cause hard to find errors). The functionality of the cast
  operator

The ``operator safe_bool_type()`` is equivalent to ``is_valid()``. This means
that these snippets are equivalent::

    object o;
    // ...
    if (o)
    {
        // ...
    }

    ...

    object o;
    // ...
    if (o.is_valid())
    {
        // ...
    }

The application operator will call the value as if it was a function. You can
give it any number of parameters (currently the ``default_policy`` will be used
for the conversion). The returned object refers to the return value (currently
only one return value is supported). This operator may throw ``luabind::error``
if the function call fails. If you want to specify policies to your function
call, you can use index-operator (operator[]) on the function call, and give
the policies within the [ and ]. Like this::

    my_function_object(
        2
      , 8
      , new my_complex_structure(6)
    ) [ adopt(_3) ];

This tells luabind to make Lua adopt the ownership and responsibility for the
pointer passed in to the lua-function.

It's important that all instances of object have been destructed by the time
the Lua state is closed. The object will keep a pointer to the lua state and
release its Lua object in its destructor.

Here's an example of how a function can use a table::

    void my_function(const object& table)
    {
        if (table.type() == LUA_TTABLE)
        {
            table["time"] = std::clock();
            table["name"] = std::rand() < 500 ? "unusual" : "usual";

            std::cout << object_cast<std::string>(table[5]) << "\n";
        }
    }

If you take a ``luabind::object`` as a parameter to a function, any Lua value
will match that parameter. That's why we have to make sure it's a table before
we index into it.


Iterators
---------

The iterators, that are returned by ``begin()`` and ``end()`` (and their
variants) are (almost) models of the ForwardIterator concept. The exception
is that post increment doesn't exist on them.

They look like this

.. parsed-literal::

    class object::iterator
    {
        iterator();
        iterator(const iterator&);

        iterator& operator++();
        bool operator!=(const iterator&) const;
        iterator& operator=(const iterator&);

        object key() const;

        *implementation-defined* operator*();
    };

The implementation defined return value from the dereference operator is a
proxy object that can be used as if it was an object, it can also be used to
assign the specific table entry with a new value. If you want to assign a value
to an entry pointed to by an iterator, just use the assignment operator on the
dereferenced iterator::

    *iter = 5;

The ``key()`` member returns the key used by the iterator when indexing the
associated Lua table.


Related functions
-----------------

There are a couple of functions related to objects and tables. ::

    T object_cast<T>(object const&);
    T object_cast<T>(object const&, Policies);

    boost::optional<T> object_cast_nothrow<T>(object const&);
    boost::optional<T> object_cast_nothrow<T>(object const&, Policies);


Functor
-------

The ``functor`` class is similar to object, with the exception that it can only
be used to store functions. If you take it as a parameter, it will only match
functions.

To use it you need to include its header::

    #include <luabind/functor.hpp>

It takes one template parameter, the return value of the Lua function it
represents. Currently the functor can have at most one return value (unlike Lua
functions).

.. topic:: Synopsis

    .. parsed-literal::

        template<class Ret>
        class functor
        {
        public:

            functor(lua_State\*, char const* name);
            functor(functor const&);
            ~functor();

            bool is_valid() const;
            operator safe_bool_type() const;
            void reset();

            lua_State\* lua_state() const;
            void pushvalue() const;
            
            bool operator==(functor<Ret> const&);
            bool operator!=(functor<Ret> const&);
            
            *implementation-defined* operator()() const;
            
            template<class A0>
            *implementation-defined* operator()(A0 const&) const;

            template<class A0, class A1>
            *implementation-defined* operator()(A0 const&, A1 const&) const;

            /\* ... \*/
        };

The application operator takes any parameters. The parameters are converted
into Lua and the function is called. The return value will act as if it was the
type Ret, with the exception that you can use the return value to give policies
to the call. You do this the same way as you do with objects, using the
operator[], and giving the policies inside the brackets.

The ``is_valid()`` function works just like the one on object, it tells you if
the functor has been assigned with a valid Lua function. The ``operator
safe_bool_type()`` is an alias for this member function and also works just as
the one found in object.

For example, if you have the following Lua function::

    function f(a, b)
        return a + b
    end

You can expose it to C++ like this::

    functor<int> f(L, "f");

    std::cout << f(3, 5) << "\n";

This will print out the sum of 3 and 5. Note that you can pass any parameters
to the application operator of ``luabind::functor``, this is because lua
doesn't have signatures for its functions. All Lua functions take any number of
parameters of any type.

If we have a C++ function that takes a ``luabind::functor`` and registers it,
it will accept Lua functions passed to it. This enables us to expose APIs that
requires you to register callbacks. For example, if your C++ API looks like
this::

    void set_callback(void(*)(int, int));

And you want to expose it to Lua, you have to wrap the call to the Lua 
function inside a real C++ function, like this::

    functor<void> lua_callback;

    void callback_wrapper(int a, int b)
    {
        lua_callback(a, b);
    }

    void set_callback_wrapper(const functor<void>& f)
    {
        lua_callback = f;
        set_callback(&callback_wrapper);
    }

And then register ``set_callback_wrapper`` instead of registering
``set_callback``. This will have the effect that when one tries to register the
callback from Lua, your ``set_callback_wrapper`` will be called instead and
first set the Lua functor to the given function. It will then call the real
``set_callback`` with the ``callback_wrapper``. The ``callback_wrapper`` will
be called whenever the callback should be called, and it will simply call the
Lua function that we registered.

You can also use ``object_cast`` to cast an object to a functor.

``reset`` on ``functor`` will invalidate the functor (and remove any references
to its Lua value). If the functor object has longer lifetime than the lua state
(e.g. if it's a global).


Defining classes in Lua
=======================

In addition to binding C++ functions and classes with Lua, luabind also provide
an OO-system in Lua. ::

    class 'lua_testclass'

    function lua_testclass:__init(name)
        self.name = name
    end

    function lua_testclass:print()
        print(self.name)
    end

    a = lua_testclass('example')
    a:print()


Inheritance can be used between lua-classes::

    class 'derived' (lua_testclass)

    function derived:__init() super('derived name')
    end

    function derived:print()
        print('Derived:print() -> ')
        lua_testclass.print(self)
    end

Here the ``super`` keyword is used in the constructor to initialize the base
class. The user is required to call ``super`` first in the constructor.

As you can see in this example, you can call the base class member functions.
You can find all member functions in the base class, but you will have to give
the this-pointer (``self``) as first argument.


Deriving in lua
---------------

It is also possible to derive Lua classes from C++ classes, and override
virtual functions with Lua functions. To do this we have to create a wrapper
class for our C++ base class. This is the class that will hold the Lua object
when we instantiate a Lua class.

::

    class base
    {
    public:
        base(const char* s)
        { std::cout << s << "\n"; }

        virtual void f(int a) 
        { std::cout << "f(" << a << ")\n"; }
    };

    struct base_wrapper : base, luabind::wrap_base
    {
        base_wrapper(const char* s)
            : base(s) 
        {}

        virtual void f(int a) 
        { 
            call<void>("f", a); 
        }

        static void default_f(base_wraper* ptr, int a)
        {
            return ptr->base::f(a);
        }
    };

    ...

    module(L)
    [
        class_<base, base_wrapper>("base")
            .def(constructor<const char*>())
            .def("f", &base_wrapper::f, &base_wrapper::default_f)
    ];

.. Important::
    Since visual studio 6.5 doesn't support explicit template parameters
    to member functions, instead of using the member function ``call()``
    you call a free function ``call_member()`` and pass the this-pointer
    as first parameter.

Note that if you have both base classes and a base class wrapper, you must give
both bases and the base class wrapper type as template parameter to 
``class_`` (as done in the example above). The order in which you specify
them is not important. You must also register both the static version and the
virtual version of the function from the wrapper, this is necessary in order
to allow luabind to use both dynamic and static dispatch when calling the function.

.. Important::
    It is extremely important that the signatures of the static (default) function
    is identical to the virtual function. The fact that onw of them is a free
    function and the other a member function doesn't matter, but the parameters
    as seen from lua must match. It would not have worked if the static function
    took a ``base*`` as its first argument, since the virtual function takes a
    ``base_wrapper*`` as its first argument (its this pointer). There's currently
    no check in luabind to make sure the signatures match.

If we didn't have a class wrapper, it would not be possible to pass a Lua class
back to C++. Since the entry points of the virtual functions would still point
to the C++ base class, and not to the functions defined in Lua. That's why we
need one function that calls the base class' real function (used if the lua
class doesn't redefine it) and one virtual function that dispatches the call
into luabind, to allow it to select if a Lua function should be called, or if
the original function should be called. If you don't intend to derive from a
C++ class, or if it doesn't have any virtual member functions, you can register
it without a class wrapper.

You don't need to have a class wrapper in order to derive from a class, but if
it has virtual functions you may have silent errors. 

.. Unnecessary? The rule of thumb is: 
  If your class has virtual functions, create a wrapper type, if it doesn't
  don't create a wrapper type.

The wrappers must derive from ``luabind::wrap_base``, it contains a Lua reference
that will hold the Lua instance of the object to make it possible to dispatch
virtual function calls into Lua. This is done through an overloaded member function::

    template<class Ret>
    Ret call(char const* name, ...)

Its used in a similar way as ``call_function``, with the exception that it doesn't
take a ``lua_State`` pointer, and the name is a member function in the Lua class.

.. warning::

	The current implementation of ``call_member`` is not able to distinguish const
	member functions from non-const. If you have a situation where you have an overloaded
	virtual function where the only difference in their signatures is their constness, the
	wrong overload will be called by ``call_member``. This is rarely the case though.

Object identity
~~~~~~~~~~~~~~~

When a pointer or reference to a registered class with a wrapper is passed
to Lua, luabind will query for it's dynamic type. If the dynamic type
inherits from ``wrap_base``, object identity is preserved.

::

    struct A { .. };
    struct A_wrap : A, wrap_base { .. };

    A* f(A* ptr) { return ptr; }

    module(L)
    [
        class_<A, A_wrap>("A"),
        def("f", &f)
    ];

::

    > class 'B' (A)
    > x = B()
    > assert(x == f(x)) -- object identity is preserved when object is
                        -- passed through C++

This functionality relies on RTTI being enabled (that ``LUABIND_NO_RTTI`` is
not defined).

Overloading operators
---------------------

You can overload most operators in Lua for your classes. You do this by simply
declaring a member function with the same name as an operator (the name of the
metamethods in Lua). The operators you can overload are:

 - ``__add``
 - ``__sub`` 
 - ``__mul`` 
 - ``__div`` 
 - ``__pow`` 
 - ``__lt`` 
 - ``__le`` 
 - ``__eq`` 
 - ``__call`` 
 - ``__unm`` 
 - ``__tostring``

``__tostring`` isn't really an operator, but it's the metamethod that is called
by the standard library's ``tostring()`` function. There's one strange behavior
regarding binary operators. You are not guaranteed that the self pointer you
get actually refers to an instance of your class. This is because Lua doesn't
distinguish the two cases where you get the other operand as left hand value or
right hand value. Consider the following examples::

    class 'my_class'

      function my_class:__init(v)
          self.val = v
      end
        
      function my_class:__sub(v)
          return my_class(self.val - v.val)
      end

      function my_class:__tostring()
          return self.val
      end

This will work well as long as you only subtracts instances of my_class with
each other. But If you want to be able to subtract ordinary numbers from your
class too, you have to manually check the type of both operands, including the
self object. ::

    function my_class:__sub(v)
        if (type(self) == 'number') then
            return my_class(self - v.val)

        elseif (type(v) == 'number') then
            return my_class(self.val - v)
        
        else
            -- assume both operands are instances of my_class
            return my_class(self.val - v.val)

        end
    end

The reason why ``__sub`` is used as an example is because subtraction is not
commutative (the order of the operands matter). That's why luabind cannot
change order of the operands to make the self reference always refer to the
actual class instance.

If you have two different Lua classes with an overloaded operator, the operator
of the right hand side type will be called. If the other operand is a C++ class
with the same operator overloaded, it will be prioritized over the Lua class'
operator. If none of the C++ overloads matches, the Lua class operator will be
called.


Finalizers
----------

If an object needs to perform actions when it's collected we provide a
``__finalize`` function that can be overridden in lua-classes. The
``__finalize`` functions will be called on all classes in the inheritance
chain, starting with the most derived type. ::

    ...

    function lua_testclass:__finalize()
        -- called when the an object is collected
    end


Slicing
-------

If your lua C++ classes don't have wrappers (see `Deriving in lua`_) and
you derive from them in lua, they may be sliced. Meaning, if an object
is passed into C++ as a pointer to its base class, the lua part will be
separated from the C++ base part. This means that if you call virtual
functions on that C++ object, thyey will not be dispatched to the lua
class. It also means that if you adopt the object, the lua part will be
garbage collected.

::

	+--------------------+
	| C++ object         |    <- ownership of this part is transferred
	|                    |       to c++ when adopted
	+--------------------+
	| lua class instance |    <- this part is garbage collected when
	| and lua members    |       instance is adopted, since it cannot
	+--------------------+       be held by c++. 


The problem can be illustrated by this example::

    struct A {};

    A* filter_a(A* a) { return a; }
    void adopt_a(A* a) { delete a; }


::

    using namespace luabind;

    module(L)
    [
        class_<A>("A"),
        def("filter_a", &filter_a),
        def("adopt_a", &adopt_a, adopt(_1))
    ]


In lua::

    a = A()
    b = filter_a(a)
    adopt_a(b)

In this example, lua cannot know that ``b`` actually is the same object as
``a``, and it will therefore consider the object to be owned by the C++ side.
When the ``b`` pointer then is adopted, a runtime error will be raised because
an object not owned by lua is being adopted to C++.

If you have a wrapper for your class, none of this will happen, see
`Object identity`_.


Exceptions
==========

If any of the functions you register throws an exception when called, that
exception will be caught by luabind and converted to an error string and
``lua_error()`` will be invoked. If the exception is a ``std::exception`` or a
``const char*`` the string that is pushed on the Lua stack, as error message,
will be the string returned by ``std::exception::what()`` or the string itself
respectively. If the exception is unknown, a generic string saying that the
function threw an exception will be pushed.

Exceptions thrown from user defined functions have to be caught by luabind. If
they weren't they would be thrown through Lua itself, which is usually compiled
as C code and doesn't support the stack-unwinding that exceptions imply.

Any function that invokes Lua code may throw ``luabind::error``. This exception
means that a Lua run-time error occurred. The error message is found on top of
the Lua stack. The reason why the exception doesn't contain the error string
itself is because it would then require heap allocation which may fail. If an
exception class throws an exception while it is being thrown itself, the
application will be terminated.

Error's synopsis is::

    class error : public std::exception
    {
    public:
        error(lua_State*);
        lua_State* state() const throw();
        virtual const char* what() const throw();
    };

The state function returns a pointer to the Lua state in which the error was
thrown. This pointer may be invalid if you catch this exception after the lua
state is destructed. If the Lua state is valid you can use it to retrieve the
error message from the top of the Lua stack.

An example of where the Lua state pointer may point to an invalid state
follows::

    struct lua_state
    {
        lua_state(lua_State* L): m_L(L) {}
        ~lua_state() { lua_close(m_L); }
        operator lua_State*() { return m_L; }
        lua_State* m_L;
    };

    int main()
    {
        try
        {
            lua_state L = lua_open();
            /* ... */
        }
        catch(luabind::error& e)
        {
            lua_State* L = e.state();
            // L will now point to the destructed
            // Lua state and be invalid
            /* ... */
        }
    }

There's another exception that luabind may throw: ``luabind::cast_failed``,
this exception is thrown from ``call_function<>``, ``call_member<>`` or when
``functor<>`` is invoked. It means that the return value from the Lua function
couldn't be converted to a C++ value. It is also thrown from ``object_cast<>``
if the cast cannot be made.

The synopsis for ``luabind::cast_failed`` is::

    class cast_failed : public std::exception
    {
    public:
        cast_failed(lua_State*);
        lua_State* state() const throw();
        LUABIND_TYPE_INFO info() const throw();
        virtual const char* what() const throw();
    };

Again, the state member function returns a pointer to the Lua state where the
error occurred. See the example above to see where this pointer may be invalid.

The info member function returns the user defined ``LUABIND_TYPE_INFO``, which
defaults to a ``const std::type_info*``. This type info describes the type that
we tried to cast a Lua value to.

If you have defined ``LUABIND_NO_EXCEPTIONS`` none of these exceptions will be
thrown, instead you can set two callback functions that are called instead.
These two functions are only defined if ``LUABIND_NO_EXCEPTIONS`` are defined.

::

    luabind::set_error_callback(void(*)(lua_State*))

The function you set will be called when a runtime-error occur in Lua code. You
can find an error message on top of the Lua stack. This function is not
expected to return, if it does luabind will call ``std::terminate()``.

::

    luabind::set_cast_failed_callback(void(*)(lua_State*, LUABIND_TYPE_INFO))

The function you set is called instead of throwing ``cast_failed``. This function
is not expected to return, if it does luabind will call ``std::terminate()``.


Policies
========

Sometimes it is necessary to control how luabind passes arguments and return
value, to do this we have policies. All policies use an index to associate
them with an argument in the function signature. These indices are ``result`` 
and ``_N`` (where ``N >= 1``). When dealing with member functions ``_1`` refers
to the ``this`` pointer.

.. contents:: Policies currently implemented
    :local:
    :depth: 1

.. include:: adopt.rst
.. include:: dependency.rst
.. include:: out_value.rst
.. include:: pure_out_value.rst
.. include:: return_reference_to.rst
.. include:: copy.rst
.. include:: discard_result.rst
.. include:: return_stl_iterator.rst
.. include:: raw.rst
.. include:: yield.rst

..  old policies section
    ===================================================

    Copy
    ----

    This will make a copy of the parameter. This is the default behavior when
    passing parameters by-value. Note that this can only be used when passing from
    C++ to Lua. This policy requires that the parameter type has a copy
    constructor.

    To use this policy you need to include ``luabind/copy_policy.hpp``.


    Adopt
    -----

    This will transfer ownership of the parameter.

    Consider making a factory function in C++ and exposing it to lua::

        base* create_base()
        {
            return new base();
        }

        ...

        module(L)
        [
            def("create_base", create_base)
        ];

    Here we need to make sure Lua understands that it should adopt the pointer
    returned by the factory-function. This can be done using the adopt-policy.

    ::

        module(L)
        [
            def(L, "create_base", adopt(return_value))
        ];

    To specify multiple policies we just separate them with '+'.

    ::

        base* set_and_get_new(base* ptr)
        {
            base_ptrs.push_back(ptr);
            return new base();
        }

        module(L)
        [
            def("set_and_get_new", &set_and_get_new, 
                adopt(return_value) + adopt(_1))
        ];

    When Lua adopts a pointer, it will call delete on it. This means that it cannot
    adopt pointers allocated with another allocator than new (no malloc for
    example).

    To use this policy you need to include ``luabind/adopt_policy.hpp``.


    Dependency
    ----------

    The dependency policy is used to create life-time dependencies between values.
    Consider the following example::

        struct A
        {
            B member;

            const B& get_member()
            {
                return member;
            }
        };

    When wrapping this class, we would do something like::

        module(L)
        [
            class_<A>("A")
                .def(constructor<>())
                .def("get_member", &A::get_member)
        ];


    However, since the return value of get_member is a reference to a member of A,
    this will create some life-time issues. For example::

        Lua 5.0  Copyright (C) 1994-2003 Tecgraf, PUC-Rio
        a = A()
        b = a:get_member() -- b points to a member of a
        a = nil
        collectgarbage(0)  -- since there are no references left to a, it is
                           -- removed
                           -- at this point, b is pointing into a removed object

    When using the dependency-policy, it is possible to tell luabind to tie the
    lifetime of one object to another, like this::

        module(L)
        [
            class_<A>("A")
                .def(constructor<>())
                .def("get_member", &A::get_member, dependency(result, _1))
        ];

    This will create a dependency between the return-value of the function, and the
    self-object. This means that the self-object will be kept alive as long as the
    result is still alive. ::

        Lua 5.0  Copyright (C) 1994-2003 Tecgraf, PUC-Rio
        a = A()
        b = a:get_member() -- b points to a member of a
        a = nil
        collectgarbage(0)  -- a is dependent on b, so it isn't removed
        b = nil
        collectgarbage(0)  -- all dependencies to a gone, a is removed

    To use this policy you need to include ``luabind/dependency_policy.hpp``.


    Return reference to
    -------------------

    It is very common to return references to arguments or the this-pointer to
    allow for chaining in C++.

    ::

        struct A
        {
            float val;

            A& set(float v)
            {
                val = v;
                return *this;
            }
        };

    When luabind generates code for this, it will create a new object for the
    return-value, pointing to the self-object. This isn't a problem, but could be a
    bit inefficient. When using the return_reference_to-policy we have the ability
    to tell luabind that the return-value is already on the Lua stack.

    ::

        module(L)
        [
            class_<A>("A")
                .def(constructor<>())
                .def("set", &A::set, return_reference_to(_1))
        ];

    Instead of creating a new object, luabind will just copy the object that is
    already on the stack.

    .. warning:: 
       This policy ignores all type information and should be used only it 
       situations where the parameter type is a perfect match to the 
       return-type (such as in the example).

    To use this policy you need to include ``luabind/return_reference_to_policy.hpp``.


    Out value
    ---------

    This policy makes it possible to wrap functions that take non const references
    as its parameters with the intention to write return values to them.

    ::

        void f(float& val) { val = val + 10.f; }

    or

    ::

        void f(float* val) { *val = *val + 10.f; }

    Can be wrapped by doing::

        module(L)
        [
            def("f", &f, out_value(_1))
        ];

    When invoking this function from Lua it will return the value assigned to its 
    parameter.

    ::

        Lua 5.0  Copyright (C) 1994-2003 Tecgraf, PUC-Rio
        > a = f(10)
        > print(a)
        20

    When this policy is used in conjunction with user define types we often need 
    to do ownership transfers.

    ::

        struct A;

        void f1(A*& obj) { obj = new A(); }
        void f2(A** obj) { *obj = new A(); }

    Here we need to make sure luabind takes control over object returned, for 
    this we use the adopt policy::

        module(L)
        [
            class_<A>("A"),
            def("f1", &f1, out_value(_1, adopt(_2)))
            def("f2", &f2, out_value(_1, adopt(_2)))
        ];

    Here we are using adopt as an internal policy to out_value. The index 
    specified, _2, means adopt will be used to convert the value back to Lua. 
    Using _1 means the policy will be used when converting from Lua to C++.

    To use this policy you need to include ``luabind/out_value_policy.hpp``.

    Pure out value
    --------------

    This policy works in exactly the same way as out_value, except that it 
    replaces the parameters with default-constructed objects.

    ::

        void get(float& x, float& y)
        {
            x = 3.f;
            y = 4.f;
        }

        ...

        module(L)
        [
            def("get", &get, 
                pure_out_value(_1) + pure_out_value(_2))
        ];

    ::

        Lua 5.0  Copyright (C) 1994-2003 Tecgraf, PUC-Rio
        > x, y = get()
        > print(x, y)
        3    5

    Like out_value, it is possible to specify an internal policy used then 
    converting the values back to Lua.

    ::

        void get(test_class*& obj)
        {
            obj = new test_class();
        }

        ...

        module(L)
        [
            def("get", &get, pure_out_value(_1, adopt(_1)))
        ];


    Discard result
    --------------

    This is a very simple policy which makes it possible to throw away 
    the value returned by a C++ function, instead of converting it to 
    Lua. This example makes sure the this reference never gets converted 
    to Lua.

    ::

        struct simple
        {
            simple& set_name(const std::string& n)
            {
                name = n;
                return *this;
            }

            std::string name;
        };

        ...

        module(L)
        [
            class_<simple>("simple")
                .def("set_name", &simple::set_name, discard_result)
        ];

    To use this policy you need to include ``luabind/discard_result_policy.hpp``.


    Return STL iterator
    -------------------

    This policy converts an STL container to a generator function that can be used
    in Lua to iterate over the container. It works on any container that defines
    ``begin()`` and ``end()`` member functions (they have to return iterators). It
    can be used like this::

        struct A
        {
            std::vector<std::string> names;
        };


        module(L)
        [
            class_<A>("A")
                .def_readwrite("names", &A::names, return_stl_iterator)
        ];

    The Lua code to iterate over the container::

        a = A()

        for name in a.names do
          print(name)
        end


    To use this policy you need to include ``luabind/iterator_policy.hpp``.


    Yield
    -----    

    This policy will cause the function to always yield the current thread when 
    returning. See the Lua manual for restrictions on yield.


Splitting up the registration
=============================

It is possible to split up a module registration into several
translation units without making each registration dependent
on the module it's being registered in.

``a.cpp``::

    luabind::scope register_a()
    {
        return 
            class_<a>("a")
                .def("f", &a::f)
                ;
    }

``b.cpp``::

    luabind::scope register_b()
    {
        return 
            class_<b>("b")
                .def("g", &b::g)
                ;
    }

``module_ab.cpp``::

    luabind::scope register_a();
    luabind::scope register_b();

    void register_module(lua_State* L)
    {
        module("b", L)
        [
            register_a(),
            register_b()
        ];
    }


Configuration
=============

As mentioned in the `Lua documentation`_, it is possible to pass an
error handler function to ``lua_pcall()``. Luabind makes use of 
``lua_pcall()`` internally when calling methods and functions. It is
possible to set the error handler function that Luabind will use 
globally::

    typedef void(*pcall_callback_fun)(lua_State*);
    void set_pcall_callback(pcall_callback_fun fn);

This is primarily useful for adding more information to the error message
returned by a failed protected call.

.. _Lua documentation: http://www.lua.org/manual/5.0/manual.html

Build options
-------------

There are a number of configuration options available when building luabind.
It is very important that your project has the exact same conmfiguration 
options as the ones given when the library was build! The exceptions are the
``LUABIND_MAX_ARITY`` and ``LUABIND_MAX_BASES`` which are template-based 
options and only matters when you use the library (which means they can 
differ from the settings of the library).

The default settings which will be used if no other settings are given
can be found in ``luabind/config.hpp``.

If you want to change the settings of the library, you can modify the 
config file. It is included and used by all makefiles. You can change paths
to Lua and boost in there as well.

LUABIND_MAX_ARITY
    Controls the maximum arity of functions that are registered with luabind. 
    You can't register functions that takes more parameters than the number 
    this macro is set to. It defaults to 5, so, if your functions have greater 
    arity you have to redefine it. A high limit will increase compilation time.

LUABIND_MAX_BASES
    Controls the maximum number of classes one class can derive from in 
    luabind (the number of classes specified within ``bases<>``). 
    ``LUABIND_MAX_BASES`` defaults to 4. A high limit will increase 
    compilation time.

LUABIND_NO_ERROR_CHECKING
    If this macro is defined, all the Lua code is expected only to make legal 
    calls. If illegal function calls are made (e.g. giving parameters that 
    doesn't match the function signature) they will not be detected by luabind
    and the application will probably crash. Error checking could be disabled 
    when shipping a release build (given that no end-user has access to write 
    custom Lua code). Note that function parameter matching will be done if a 
    function is overloaded, since otherwise it's impossible to know which one 
    was called. Functions will still be able to throw exceptions when error 
    checking is disabled.

    If a functions throws an exception it will be caught by luabind and 
    propagated with ``lua_error()``.

LUABIND_NO_EXCEPTIONS
    This define will disable all usage of try, catch and throw in luabind. 
    This will in many cases disable run-time errors, when performing invalid 
    casts or calling Lua functions that fails or returns values that cannot 
    be converted by the given policy. luabind requires that no function called 
    directly or indirectly by luabind throws an exception (throwing exceptions 
    through Lua has undefined behavior).

    Where exceptions are the only way to get an error report from luabind, 
    they will be replaced with calls to the callback functions set with
    ``set_error_callback()`` and ``set_cast_failed_callback()``.

LUA_API
    If you want to link dynamically against Lua, you can set this define to 
    the import-keyword on your compiler and platform. On windows in devstudio 
    this should be ``__declspec(dllimport)`` if you want to link against Lua 
    as a dll.

LUABIND_EXPORT, LUABIND_IMPORT
    If you want to link against luabind as a dll (in devstudio), you can 
    define ``LUABIND_EXPORT`` to ``__declspec(dllexport)`` and 
    ``LUABIND_IMPORT`` to ``__declspec(dllimport)``. 
    Note that you have to link against Lua as a dll aswell, to make it work.

LUABIND_NO_RTTI
    You can define this if you don't want luabind to use ``dynamic_cast<>``.
    It will disable `Object identity`_.

LUABIND_TYPE_INFO, LUABIND_TYPE_INFO_EQUAL(i1,i2), LUABIND_TYPEID(t), LUABIND_INVALID_TYPE_INFO
    If you don't want to use the RTTI supplied by C++ you can supply your own 
    type-info structure with the ``LUABIND_TYPE_INFO`` define. Your type-info 
    structure must be copyable and must be able to compare itself against 
    other type-info structures. You supply the compare function through the 
    ``LUABIND_TYPE_INFO_EQUAL()`` define. It should compare the two type-info 
    structures it is given and return true if they represent the same type and
    false otherwise. You also have to supply a function to generate your 
    type-info structure. You do this through the ``LUABIND_TYPEID()`` define. 
    It should return your type-info structure and it takes a type as its 
    parameter. That is, a compile time parameter. 
    ``LUABIND_INVALID_TYPE_INFO`` macro should be defined to an invalid type. 
    No other type should be able to produce this type info. To use it you 
    probably have to make a traits class with specializations for all classes 
    that you have type-info for. Like this::

        class A;
        class B;
        class C;

        template<class T> struct typeinfo_trait;

        template<> struct typeinfo_trait<A> { enum { type_id = 0 }; };
        template<> struct typeinfo_trait<B> { enum { type_id = 1 }; };
        template<> struct typeinfo_trait<C> { enum { type_id = 2 }; };

    If you have set up your own RTTI system like this (by using integers to
    identify types) you can have luabind use it with the following defines::

        #define LUABIND_TYPE_INFO const std::type_info*
        #define LUABIND_TYPEID(t) &typeid(t)
        #define LUABIND_TYPE_INFO_EQUAL(i1, i2) *i1 == *i2
        #define LUABIND_INVALID_TYPE_INFO &typeid(detail::null_type)

    Currently the type given through ``LUABIND_TYPE_INFO`` must be less-than 
    comparable!

NDEBUG
    This define will disable all asserts and should be defined in a release 
    build.


Implementation notes
====================

The classes and objects are implemented as user data in Lua. To make sure that
the user data really is the internal structure it is supposed to be, we tag
their metatables. A user data who's metatable contains a boolean member named
``__luabind_classrep`` is expected to be a class exported by luabind. A user
data who's metatable contains a boolean member named ``__luabind_class`` is
expected to be an instantiation of a luabind class.

This means that if you make your own user data and tags its metatable with the
exact same names, you can very easily fool luabind and crash the application.

In the Lua registry, luabind keeps an entry called ``__luabind_classes``. It
should not be removed or overwritten.

In the global table, a variable called ``super`` is used every time a
constructor in a lua-class is called. This is to make it easy for that
constructor to call its base class' constructor. So, if you have a global
variable named super it may be overwritten. This is probably not the best
solution, and this restriction may be removed in the future.

Luabind uses two upvalues for functions that it registers. The first is a
userdata containing a list of overloads for the function, the other is a light
userdata with the value 0x1337, this last value is used to identify functions
registered by luabind. It should be virtually impossible to have such a pointer
as secondary upvalue by pure chance. This means, if you are trying to replace
an existing function with a luabind function, luabind will see that the
secondary upvalue isn't the magic id number and replace it. If it can identify
the function to be a luabind function, it won't replace it, but rather add
another overload to it.

Inside the luabind namespace, there's another namespace called detail. This
namespace contains non-public classes and are not supposed to be used directly.


Error messages
==============

- .. parsed-literal::

    the attribute '*class-name.attribute-name*' is read only

  There is no data member named *attribute-name* in the class *class-name*,
  or there's no setter-method registered on that property name. See the 
  properties section.

- .. parsed-literal:: 

    the attribute '*class-name.attribute-name*' is of type: (*class-name*) and does not match (*class_name*)

  This error is generated if you try to assign an attribute with a value 
  of a type that cannot be converted to the attribute's type.


- .. parsed-literal:: 

    *class-name()* threw an exception, *class-name:function-name()* threw an exception

  The class' constructor or member function threw an unknown exception.
  Known exceptions are const char*, std::exception. See the 
  `exceptions`_ section.

- .. parsed-literal::

    no overload of '*class-name:function-name*' matched the arguments (*parameter-types*)
    no match for function call '*function-name*' with the parameters (*parameter-types*)
    no constructor of *class-name* matched the arguments (*parameter-types*)
    no operator *operator-name* matched the arguments (*parameter-types*)

  No function/operator with the given name takes the parameters you gave 
  it. You have either misspelled the function name, or given it incorrect
  parameters. This error is followed by a list of possible candidate 
  functions to help you figure out what parameter has the wrong type. If
  the candidate list is empty there's no function at all with that name.
  See the signature matching section.

- .. parsed-literal::

    call of overloaded '*class-name:function-name*(*parameter-types*)' is ambiguous
    ambiguous match for function call '*function-name*' with the parameters (*parameter-types*)
    call of overloaded constructor '*class-name*(*parameter-types*)' is ambiguous
    call of overloaded operator *operator-name* (*parameter-types*) is ambiguous

  This means that the function/operator you are trying to call has at least
  one other overload that matches the arguments just as good as the first
  overload.

- .. parsed-literal::

    cannot derive from C++ class '*class-name*'. It does not have a wrapped type.


FAQ
===

What's up with __cdecl and __stdcall?
    If you're having problem with functions
    that cannot be converted from ``void (__stdcall *)(int,int)`` to 
    ``void (__cdecl*)(int,int)``. You can change the project settings to make the
    compiler generate functions with __cdecl calling conventions. This is
    a problem in developer studio.

What's wrong with functions taking variable number of arguments?
    You cannot register a function with ellipses in its signature. Since
    ellipses don't preserve type safety, those should be avoided anyway.

Internal structure overflow in VC
    If you, in visual studio, get fatal error C1204: compiler limit :
    internal structure overflow. You should try to split that compilation
    unit up in smaller ones.

.. the three entries above were removed, why?

What's wrong with precompiled headers in VC?
    Visual Studio doesn't like anonymous namespaces in its precompiled 
    headers. If you encounter this problem you can disable precompiled 
    headers for the compilation unit (cpp-file) that uses luabind.

error C1076: compiler limit - internal heap limit reached in VC
    In visual studio you will probably hit this error. To fix it you have to
    increase the internal heap with a command-line option. We managed to
    compile the test suit with /Zm300, but you may need a larger heap then 
    that.

error C1055: compiler limit \: out of keys in VC
    It seems that this error occurs when too many assert() are used in a
    program, or more specifically, the __LINE__ macro. It seems to be fixed by
    changing /ZI (Program database for edit and continue) to /Zi 
    (Program database).

How come my executable is huge?
    If you're compiling in debug mode, you will probably have a lot of
    debug-info and symbols (luabind consists of a lot of functions). Also, 
    if built in debug mode, no optimizations were applied, luabind relies on 
    that the compiler is able to inline functions. If you built in release 
    mode, try running strip on your executable to remove export-symbols, 
    this will trim down the size.

    Our tests suggests that cygwin's gcc produces much bigger executables 
    compared to gcc on other platforms and other compilers.

.. HUH?! // check the magic number that identifies luabind's functions 

Can I register class templates with luabind?
    Yes you can, but you can only register explicit instantiations of the 
    class. Because there's no Lua counterpart to C++ templates. For example, 
    you can register an explicit instantiation of std::vector<> like this::

        module(L)
        [
            class_<std::vector<int> >("vector")
                .def(constructor<int>)
                .def("push_back", &std::vector<int>::push_back)
        ];

.. Again, irrelevant to docs: Note that the space between the two > is required by C++.

Do I have to register destructors for my classes?
    No, the destructor of a class is always called by luabind when an 
    object is collected. Note that Lua has to own the object to collect it.
    If you pass it to C++ and gives up ownership (with adopt policy) it will 
    no longer be owned by Lua, and not collected.

    If you have a class hierarchy, you should make the destructor virtual if 
    you want to be sure that the correct destructor is called (this apply to C++ 
    in general).

.. And again, the above is irrelevant to docs. This isn't a general C++ FAQ.

Fatal Error C1063 compiler limit \: compiler stack overflow in VC
    VC6.5 chokes on warnings, if you are getting alot of warnings from your 
    code try suppressing them with a pragma directive, this should solve the 
    problem.

Crashes when linking against luabind as a dll in windows
    When you build luabind, Lua and you project, make sure you link against 
    the runtime dynamically (as a dll).

I cannot register a function with a non-const parameter
    This is because there is no way to get a reference to a Lua value. Have 
    a look at out_value and pure_out_value policies.


Known issues
============

- You cannot use strings with extra nulls in them as member names that refers
  to C++ members.

- If one class registers two functions with the same name and the same
  signature, there's currently no error. The last registered function will
  be the one that's used.

- In VC7, classes can not be called test.

- If you register a function and later rename it, error messages will use the
  original function name.

- luabind does not support class hierarchies with virtual inheritance. Casts are
  done with static pointer offsets.

.. remove? - Visual studio have problems selecting the correct overload of std::swap()
  for luabind::object.


Acknowledgments
===============

Written by Daniel Wallin and Arvid Norberg. © Copyright 2003.
All rights reserved.

Evan Wies has contributed with thorough testing, countless bug reports
and feature ideas.

This library was highly inspired by Dave Abrahams' Boost.Python_ library.

.. _Boost.Python: http://www.boost.org/libraries/python

