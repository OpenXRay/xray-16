/*
** Copyright (C) 2005-2008 Mike Pall. All rights reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
** [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

/* LuaJIT -- a Just-In-Time Compiler for Lua. http://luajit.org/ */

/* LuaJIT public C API. */
#ifndef luajit_h
#define luajit_h

#include <lua/lua.h>


#define LUAJIT_VERSION		"LuaJIT 1.1.4"
#define LUAJIT_VERSION_NUM	10104  /* Version 1.1.4 = 01.01.04. */
#define LUAJIT_VERSION_SYM	luaJIT_version_1_1_4
#define LUAJIT_COPYRIGHT	"Copyright (C) 2005-2008 Mike Pall"
#define LUAJIT_URL		"http://luajit.org/"

/* Modes for luaJIT_setmode. */
#define LUAJIT_MODE_MASK	0x00ff

enum {
  LUAJIT_MODE_ENGINE,		/* Set mode for whole JIT engine. */
  LUAJIT_MODE_DEBUG,		/* Set debug mode (idx = level). */

  LUAJIT_MODE_FUNC,		/* Change mode for a function. */
  LUAJIT_MODE_ALLFUNC,		/* Recurse into subroutine protos. */
  LUAJIT_MODE_ALLSUBFUNC,	/* Change only the subroutines. */
  LUAJIT_MODE_MAX
};

/* Flags or'ed in to the mode. */
#define LUAJIT_MODE_OFF		0x0000	/* Disable JIT compilation. */
#define LUAJIT_MODE_ON		0x0100	/* (Re-)enable JIT compilation. */


/* Compile a Lua function. Pass arguments as hints. */
LUA_API int luaJIT_compile(lua_State *L, int nargs);

/* Set the JIT mode for the whole engine or a function (idx = 0: self). */
LUA_API int luaJIT_setmode(lua_State *L, int idx, int mode);

/* Enforce (dynamic) linker error for version mismatches. Call from main. */
LUA_API void LUAJIT_VERSION_SYM(void);

#endif
