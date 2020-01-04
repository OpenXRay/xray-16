////////////////////////////////////////////////////////////////////////////
//	Module 		: interfaces.hpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : editor library interfaces
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_INTERFACES_HPP_INCLUDED
#define EDITOR_INTERFACES_HPP_INCLUDED

namespace XRay
{
namespace Editor
{
class ide_base;
class engine_base;

using initialize_function_ptr = void(*)(ide_base*&);
using finalize_function_ptr = void(*)(ide_base*&);

// XXX: is __cdecl needed?
//typedef void(__cdecl* initialize_function_ptr)(ide_base*&, engine_base*);
//typedef void(__cdecl* finalize_function_ptr)(ide_base*&);
}
}

#endif // ifndef EDITOR_INTERFACES_HPP_INCLUDED
