////////////////////////////////////////////////////////////////////////////
//	Module 		: backend.h
//	Created 	: 14.04.2008
//  Modified 	: 14.04.2008
//	Author		: Dmitriy Iassenev
//	Description : script debugger backend class
////////////////////////////////////////////////////////////////////////////

#pragma once

struct lua_State;
struct lua_Debug;

namespace cs
{
namespace lua_studio
{
enum icon_type
{
#if 0
    icon_type_nil = unsigned int(0),
    icon_type_boolean = unsigned int(0),
    icon_type_number = unsigned int(0),
    icon_type_string = unsigned int(0),
    icon_type_table = unsigned int(1),
    icon_type_function = unsigned int(3),
    icon_type_thread = unsigned int(0),
    icon_type_class = unsigned int(2),
    icon_type_class_base = unsigned int(5),
    icon_type_class_instance = unsigned int(4),
    icon_type_unknown = unsigned int(0),
#else
    icon_type_nil = (unsigned int) (0),
    icon_type_boolean = (unsigned int) (1),
    icon_type_number = (unsigned int) (2),
    icon_type_string = (unsigned int) (3),
    icon_type_table = (unsigned int) (4),
    icon_type_function = (unsigned int) (5),
    icon_type_coroutine = (unsigned int) (6),
    icon_type_class = (unsigned int) (7),
    icon_type_class_base = (unsigned int) (8),
    icon_type_class_instance = (unsigned int) (9),
    icon_type_unknown = (unsigned int) (10),
#endif
};

struct DECLSPEC_NOVTABLE backend
{
public:
    virtual void CS_LUA_STUDIO_BACKEND_CALL type_to_string(
        char* buffer, unsigned int size, lua_State* state, int index, bool& use_in_description) = 0;
    virtual void CS_LUA_STUDIO_BACKEND_CALL value_to_string(
        char* buffer, unsigned int size, lua_State* state, int index, icon_type& icon_type, bool full_description) = 0;
};

struct DECLSPEC_NOVTABLE value_to_expand
{
    virtual void CS_LUA_STUDIO_BACKEND_CALL add_value(
        char const* id, char const* type, char const* value, icon_type icon_type) = 0;
};
} // namespace lua_studio
} // namespace cs
