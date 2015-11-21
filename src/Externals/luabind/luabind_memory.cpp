////////////////////////////////////////////////////////////////////////////
//	Module 		: luabind_memory.cpp
//	Created 	: 24.06.2005
//  Modified 	: 24.09.2006
//	Author		: Dmitriy Iassenev
//	Description : luabind memory allocator template class
////////////////////////////////////////////////////////////////////////////

#include "pch.h"

luabind::memory_allocation_function_pointer		luabind::allocator = 0;
luabind::memory_allocation_function_parameter	luabind::allocator_parameter = 0;
