/*
** Wrapper for architecture-specific DynASM encoder.
** Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
*/

#define ljit_dasm_c
#define LUA_CORE


#include <lua/lua.h>

#include "ljit.h"
#include "ljit_dasm.h"
#include "lmem.h"


/* Glue macros for DynASM memory allocation. */
#define DASM_M_GROW(J, t, p, sz, need) \
  do { \
    size_t _sz = (sz), _need = (need); \
    if (_sz < _need) { \
      if (_sz < 16) _sz = 16; \
      while (_sz < _need) _sz += _sz; \
      (p) = (t *)luaM_realloc_(J->L, (p), (sz), _sz); \
      (sz) = _sz; \
    } \
  } while(0)

#define DASM_M_FREE(J, p, sz)	luaM_freemem(J->L, p, sz)

/* Embed architecture-specific DynASM encoder. */
#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
#include "dasm_x86.h"
#else
#error "No support for this architecture (yet)"
#endif


