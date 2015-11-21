out_value
----------------

Motivation
~~~~~~~~~~

This policy makes it possible to wrap functions that take non-const references
or pointer to non-const as it's parameters with the intention to write return 
values to them.

Defined in
~~~~~~~~~~

.. parsed-literal::

    #include <luabind/out_value_policy.hpp>

Synopsis
~~~~~~~~

.. parsed-literal::

    out_value(index, policies = none)


Parameters
~~~~~~~~~~

=============== =============================================================
Parameter       Purpose
=============== =============================================================
``index``       The index of the parameter to be used as an out parameter.
``policies``    The policies used internally to convert the out parameter
                to/from Lua. ``_1`` means **to** C++, ``_2`` means **from**
                C++.
=============== =============================================================

Example
~~~~~~~

.. parsed-literal::

    void f1(float& val) { val = val + 10.f; }
    void f2(float\* val) { \*val = \*val + 10.f; }

    module(L)
    [
        def("f", &f, **out_value(_1)**)
    ];

    Lua 5.0  Copyright (C) 1994-2003 Tecgraf, PUC-Rio
    > print(f1(10))
    20
    > print(f2(10))
    20

