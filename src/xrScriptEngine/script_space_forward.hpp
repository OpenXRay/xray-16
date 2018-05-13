////////////////////////////////////////////////////////////////////////////
//	Module 		: script_space_forward.h
//	Created 	: 21.07.2004
//  Modified 	: 21.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script space forward declarations
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace luabind
{
namespace adl
{
class object;
}

template <typename TResult, typename... Policies>
class functor;
template <class T, class ValueWrapper>
T object_cast(ValueWrapper const& value_wrapper);
}
