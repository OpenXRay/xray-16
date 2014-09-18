////////////////////////////////////////////////////////////////////////////
//	Module 		: debug_make_final.cpp
//	Created 	: 03.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : make_final class for debug purposes
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "debug_make_final.hpp"

struct A : private boost::noncopyable
{
};

struct B :
	public A,
	private debug::make_final<B>
{
};

struct C : B {};

B	b;

// the next 2 lines won't compile
C	*c0 = new C();
C	c1;