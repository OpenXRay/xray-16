////////////////////////////////////////////////////////////////////////////
//	Module 		: static_cast_checked.cpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : checked static_cast implementation for debug purposes
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "static_cast_checked.hpp"

// non-polymorphic test
struct A {};
struct B : A {};

A			a;
B*			b0 = static_cast_checked<B*>		(&a);
B&			b1 = static_cast_checked<B&>		(a);
B const *	b2 = static_cast_checked<B const *>	(&a);
B const &	b3 = static_cast_checked<B const &>	(a);
B const *	b4 = static_cast_checked<B const *>	((A const *)&a);
B const &	b5 = static_cast_checked<B const &>	((A const &)a);
// the next 2 lines won't compile
B*			b6 = static_cast_checked<B*>		((A const *)&a);
B&			b7 = static_cast_checked<B&>		((A const &)a);

// polymorphic test
struct C {virtual ~C() {}};
struct D : C {};

C			c;
D*			d0 = static_cast_checked<D*>		(&c);
D&			d1 = static_cast_checked<D&>		(c);
D const *	d2 = static_cast_checked<D const *>	(&c);
D const &	d3 = static_cast_checked<D const &>	(c);
D const *	d4 = static_cast_checked<D const *>	((C const *)&c);
D const &	d5 = static_cast_checked<D const &>	((C const &)c);
// the next 2 lines won't compile
D*			d6 = static_cast_checked<D*>		((C const *)&c);
D&			d7 = static_cast_checked<D&>		((C const &)c);