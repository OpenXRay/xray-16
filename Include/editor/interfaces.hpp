////////////////////////////////////////////////////////////////////////////
//	Module 		: interfaces.hpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : editor library interfaces
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_INTERFACES_HPP_INCLUDED
#define EDITOR_INTERFACES_HPP_INCLUDED

namespace editor {

class ide;
class engine;

typedef void (__cdecl *initialize_function_ptr)	(ide*&, engine*);
typedef void (__cdecl *finalize_function_ptr)	(ide*&);

} // namespace editor

#endif // ifndef EDITOR_INTERFACES_HPP_INCLUDED