#ifndef PCH_H
#define PCH_H

#include <luabind/lua_include.hpp>
#include <luabind/luabind.hpp>

#ifdef NDEBUG

namespace std {
	void terminate();
} // namespace std

#endif // #ifdef NDEBUG

#endif // PCH_H