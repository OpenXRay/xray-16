////////////////////////////////////////////////////////////////////////////
//	Module 		: world.h
//	Created 	: 10.04.2008
//  Modified 	: 10.04.2008
//	Author		: Dmitriy Iassenev
//	Description : lua studio backend world class
////////////////////////////////////////////////////////////////////////////

#ifndef CS_LUA_STUDIO_BACKEND_WORLD_H_INCLUDED
#define CS_LUA_STUDIO_BACKEND_WORLD_H_INCLUDED

struct lua_State;

namespace cs {
namespace lua_studio {

struct engine;

struct DECLSPEC_NOVTABLE world {
	virtual	void CS_LUA_STUDIO_BACKEND_CALL	add					(lua_State *state) = 0;
	virtual	void CS_LUA_STUDIO_BACKEND_CALL	remove				(lua_State *state) = 0;
	virtual	int  CS_LUA_STUDIO_BACKEND_CALL	on_error			(lua_State *state) = 0;
	virtual	void CS_LUA_STUDIO_BACKEND_CALL	add_log_line		(const char *log_line) = 0;
	virtual	bool CS_LUA_STUDIO_BACKEND_CALL	evaluating_watch	() = 0;
}; // struct DECLSPEC_NOVTABLE world

typedef void*	maf_parameter;
typedef void*	(CS_LUA_STUDIO_BACKEND_CALL *maf_ptr) (maf_parameter parameter, void const *, size_t);

typedef world*	(CS_LUA_STUDIO_BACKEND_CALL *create_world_function_type)	(engine& engine, bool, bool);
typedef void	(CS_LUA_STUDIO_BACKEND_CALL *destroy_world_function_type)	(world*& world);
typedef void	(CS_LUA_STUDIO_BACKEND_CALL *memory_allocator_function_type)(maf_ptr memory_allocator, maf_parameter parameter);
typedef size_t	(CS_LUA_STUDIO_BACKEND_CALL *memory_stats_function_type)	();

} // namespace lua_studio
} // namespace cs

extern "C" {
	CS_LUA_STUDIO_BACKEND_API	cs::lua_studio::world*			CS_LUA_STUDIO_BACKEND_CALL	cs_lua_studio_backend_create_world				(cs::lua_studio::engine& engine, bool use_bugtrap, bool create_log_file);
	CS_LUA_STUDIO_BACKEND_API	void							CS_LUA_STUDIO_BACKEND_CALL	cs_lua_studio_backend_destroy_world				(cs::lua_studio::world*& world);

	CS_LUA_STUDIO_BACKEND_API	void							CS_LUA_STUDIO_BACKEND_CALL	cs_lua_studio_backend_memory_allocator			(cs::lua_studio::maf_ptr memory_allocator, cs::lua_studio::maf_parameter parameter);
	CS_LUA_STUDIO_BACKEND_API	size_t							CS_LUA_STUDIO_BACKEND_CALL	cs_lua_studio_backend_memory_stats				();
}

#endif // #ifndef CS_LUA_STUDIO_BACKEND_WORLD_H_INCLUDED