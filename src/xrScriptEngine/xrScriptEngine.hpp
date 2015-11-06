#pragma once

#include "xrCore/Platform.h"

#ifdef XRSCRIPTENGINE_EXPORTS
#define XRSCRIPTENGINE_API XR_EXPORT
#else
#define XRSCRIPTENGINE_API XR_IMPORT
#endif

#pragma warning(disable:4244)
#pragma warning(disable:4995)
#pragma warning(disable:4267)
#pragma warning(disable:4100) // unreferenced formal parameter


extern "C"
{
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>
}

#pragma warning(disable:4458) // declaration of 'x' hides class member
#pragma warning(disable:4459) // declaration of 'x' hides global declaration
#pragma warning(disable:4913) // user defined binary operator 'x' exists but no overload could convert all operands
#pragma warning(disable:4297) // function assumed not to throw exception but does

#include <luabind/luabind.hpp>
#include <luabind/class.hpp>
#include <luabind/object.hpp>
#include <luabind/functor.hpp>
#include <luabind/operator.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/return_reference_to_policy.hpp>
#include <luabind/out_value_policy.hpp>
#include <luabind/iterator_policy.hpp>

#pragma warning(default:4458)
#pragma warning(default:4459)
#pragma warning(default:4913)
#pragma warning(default:4297)

#pragma warning(default:4244)
#pragma warning(default:4995)
#pragma warning(default:4267)
#pragma warning(default:4100)
