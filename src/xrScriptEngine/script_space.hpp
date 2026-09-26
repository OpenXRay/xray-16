////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_script_space.h
//	Created 	: 22.09.2003
//  Modified 	: 22.09.2003
//	Author		: Dmitriy Iassenev
//	Description : XRay Script space
////////////////////////////////////////////////////////////////////////////

#pragma once

#pragma warning(push)

#pragma warning(disable : 4244)
#pragma warning(disable : 4995)
#pragma warning(disable : 4267)
#pragma warning(disable : 4100) // unreferenced formal parameter

#include "lua.hpp"

#ifndef LUA_OK
#define LUA_OK 0
#endif

#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4456) // declaration of 'x' hides previous local declaration
#pragma warning(disable : 4458) // declaration of 'x' hides class member
#pragma warning(disable : 4459) // declaration of 'x' hides global declaration
#pragma warning(disable : 4913) // user defined binary operator 'x' exists but no overload could convert all operands
#pragma warning(disable : 4297) // function assumed not to throw exception but does

#include <luabind/luabind.hpp>
#include <luabind/class.hpp>
#include <luabind/object.hpp>
#include <luabind/operator.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/return_reference_to_policy.hpp>
#include <luabind/out_value_policy.hpp>
#include <luabind/iterator_policy.hpp>

#pragma warning(pop)

XRSCRIPTENGINE_API size_t luabind_it_distance(luabind::iterator first, const luabind::iterator& last);
