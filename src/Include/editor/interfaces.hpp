////////////////////////////////////////////////////////////////////////////
//	Module 		: interfaces.hpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : editor library interfaces
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_INTERFACES_HPP_INCLUDED
#define EDITOR_INTERFACES_HPP_INCLUDED

namespace editor
{
class ide_base;
class engine;

typedef void(__cdecl* initialize_function_ptr)(ide_base*&, engine*);
typedef void(__cdecl* finalize_function_ptr)(ide_base*&);

} // namespace editor

#endif // ifndef EDITOR_INTERFACES_HPP_INCLUDED
