/*
** Lua library for the JIT engine.
** Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
*/

#include <stdio.h>
#include <string.h>

#define ljitlib_c
#define LUA_LIB

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include "luajit.h"
#include <lua/lualib.h>

/* This file is not a pure C API user. Some internals are required. */
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "lfunc.h"
#include "lgc.h"
#include "lopcodes.h"

#include "ljit.h"
#include "ljit_hints.h"

#define STRING_HINTS
#include "ljit_hints.h"

/* ------------------------------------------------------------------------ */

/* Static pointer addresses used as registry keys. */
/* The values do not matter, but must be different to prevent joining. */
static const int regkey_frontend = 0x6c6a6c01;
static const int regkey_comthread = 0x6c6a6c02;

/* Check that the first argument is a Lua function and return its closure. */
static Closure *check_LCL(lua_State *L)
{
  StkId o = L->base;
  switch (lua_type(L, 1)) {
  case LUA_TBOOLEAN:
    o = (L->ci-1)->func;
  case LUA_TFUNCTION:
    if (isLfunction(o))
      return clvalue(o);
    break;
  }
  luaL_argerror(L, 1, "Lua function expected");
  return NULL;
}

/* Create a new closure from a prototype. */
/* Note: upvalues are assumed to be after first two slots. */
static void push_LCL(lua_State *L, Proto *pt, Table *env)
{
  Closure *cl;
  int i, nup = pt->nups;
  /* Adjust the number of stack slots to the number of upvalues. */
  luaL_checkstack(L, nup, "too many upvalues");
  lua_settop(L, 2+nup);
  /* Create a closure from the subroutine prototype. */
  cl = luaF_newLclosure(L, nup, env);
  cl->l.p = pt;
  /* Allocate new upvalues and close them. */
  for (i = 0; i < nup; i++)
    cl->l.upvals[i] = luaF_findupval(L, L->base + (2+i));
  luaF_close(L, L->base + 2);
  lua_settop(L, 2);  /* Remove upvalues. */
  setclvalue(L, L->top++, cl);  /* Return closure on top of stack. */
  luaC_checkGC(L);
}

/* ------------------------------------------------------------------------ */

/* Set JIT mode for the engine or a closure and/or its subroutines. */
static int setmode(lua_State *L, int mode)
{
  int idx = 0;
  switch (lua_type(L, 1)) {
  case LUA_TNONE:	/* jit.on/off() */
  case LUA_TNIL:	/* jit.on/off(nil) */
    luaJIT_setmode(L, 0, mode | LUAJIT_MODE_ENGINE);
    break;
  case LUA_TFUNCTION:	/* jit.on/off(func, nil|true|false) */
    idx = 1;
  case LUA_TBOOLEAN:	/* jit.on/off(true, nil|true|false) (parent frame) */
    if (lua_isboolean(L, 2))
      mode |= lua_toboolean(L, 2)?LUAJIT_MODE_ALLFUNC:LUAJIT_MODE_ALLSUBFUNC;
    else
      mode |= LUAJIT_MODE_FUNC;
    if (luaJIT_setmode(L, idx, mode) == 1)  /* Ok? */
      break;
  default:
    luaL_argerror(L, 1, "Lua function expected");
    break;
  }
  return 0;
}

/* Set JIT mode to on: (re-)enable compilation. */
static int j_on(lua_State *L)
{
  return setmode(L, LUAJIT_MODE_ON);
}

/* Set JIT mode to off: disable compilation. */
static int j_off(lua_State *L)
{
  return setmode(L, LUAJIT_MODE_OFF);
}

/* Set JIT debug level. Defaults to maximum level for use with -j. */
static int j_debug(lua_State *L)
{
  luaJIT_setmode(L, luaL_optinteger(L, 1, 100), LUAJIT_MODE_DEBUG);
  return 0;
}

/* ------------------------------------------------------------------------ */

/* Report the compilation status. */
static int compstatus(lua_State *L, int status)
{
  if (status == -1)
    return luaL_argerror(L, 1, "Lua function expected");
  else if (status == JIT_S_OK)
    return 0;
  else {
    lua_pushinteger(L, status);
    return 1;
  }
}

/* Compile a function. Pass typical args to help the optimizer. */
static int j_compile(lua_State *L)
{
  int nargs = lua_gettop(L) - 1;
  return compstatus(L, nargs >= 0 ? luaJIT_compile(L, nargs) : -1);
}

/* Recursively compile all subroutine prototypes. */
static int rec_compile(lua_State *L, Proto *pt, Table *env, int stoponerror)
{
  int rstatus = JIT_S_OK;
  int i;
  for (i = 0; i < pt->sizep; i++) {
    Proto *pti = pt->p[i];
    int status;
    push_LCL(L, pti, env);  /* Assumes stack is at 2 (no upvalues). */
    status = luaJIT_compile(L, 0);
    lua_settop(L, 2);  /* Clear stack */
    if (status != JIT_S_OK) {
      rstatus = status;
      if (stoponerror) break;
    }
    status = rec_compile(L, pti, env, stoponerror);
    if (status != JIT_S_OK) {
      rstatus = status;
      if (stoponerror) break;
    }
  }
  return rstatus;
}

/* Compile all subroutines of a function. */
/* Note: the function itself is _not_ compiled (use jit.compile()). */
static int j_compilesub(lua_State *L)
{
  Closure *cl = check_LCL(L);
  int stoponerror = lua_toboolean(L, 2);  /* Stop on first error? */
  lua_settop(L, 2);
  return compstatus(L, rec_compile(L, cl->l.p, cl->l.env, stoponerror));
}

/* jit.* functions. */
static const luaL_Reg jitlib[] = {
  { "on",		j_on },
  { "off",		j_off },
  { "debug",		j_debug },
  { "compile",		j_compile },
  { "compilesub",	j_compilesub },
  /* j_attach is added below. */
  { NULL, NULL }
};

/* ------------------------------------------------------------------------ */

/* Get the compiler pipeline table from an upvalue (j_attach, j_frontend). */
#define COMPIPE		lua_upvalueindex(1)

/* Attach/detach handler to/from compiler pipeline. */
static int j_attach(lua_State *L)
{
  int pipesz;
  luaL_checktype(L, 1, LUA_TFUNCTION);
  pipesz = lua_objlen(L, COMPIPE);
  if (lua_isnoneornil(L, 2)) {  /* Detach if no priority given. */
    int i;
    for (i = 1; i <= pipesz; i += 2) {
      lua_rawgeti(L, COMPIPE, i);
      if (lua_rawequal(L, 1, -1)) {  /* Found: delete from pipeline. */
	for (; i+2 <= pipesz; i++) {  /* Shift down. */
	  lua_rawgeti(L, COMPIPE, i+2);
	  lua_rawseti(L, COMPIPE, i);
	}
	/* Clear last two elements. */
	lua_pushnil(L); lua_rawseti(L, COMPIPE, i);
	lua_pushnil(L); lua_rawseti(L, COMPIPE, i+1);
	return 0;
      }
      lua_pop(L, 1);
    }
    return 0;  /* Not found: ignore detach request. */
  } else {  /* Attach if priority given. */
    int prio = luaL_checkint(L, 2);
    int pos, i;
    for (pos = 2; pos <= pipesz; pos += 2) {
      lua_rawgeti(L, COMPIPE, pos);
      if (prio > (int)lua_tointeger(L, -1)) break; /* Insertion point found. */
      lua_pop(L, 1);
    }
    for (i = pipesz+2; i > pos; i--) {  /* Shift up. */
      lua_rawgeti(L, COMPIPE, i-2);
      lua_rawseti(L, COMPIPE, i);
    }
    /* Set handler and priority. */
    lua_pushvalue(L, 1); lua_rawseti(L, COMPIPE, i-1);
    lua_pushvalue(L, 2); lua_rawseti(L, COMPIPE, i);
    return 0;
  }
}

/* Compiler frontend. Runs in the compiler thread. */
/* First and only arg is the compiler state table. */
static int j_frontend(lua_State *L)
{
  int status = JIT_S_OK;
  int pos;
  /* Loop through all handlers in the compiler pipeline. */
  for (pos = 1; ; pos += 2) {
    if (status != JIT_S_OK) {  /* Pending failure? */
      int prio;
      lua_rawgeti(L, COMPIPE, pos+1);  /* Must check for odd/even priority. */
      if (lua_isnil(L, -1)) break;  /* End of pipeline. */
      prio = (int)lua_tointeger(L, -1);
      lua_pop(L, 1);
      if ((prio & 1) == 0) continue;  /* Skip handlers with even priority. */
    }
    /* Call handler with compiler state table and optional failure status. */
    lua_rawgeti(L, COMPIPE, pos);
    if (lua_isnil(L, -1)) break;  /* End of pipeline. */
    lua_pushvalue(L, 1);
    if (status != JIT_S_OK)
      lua_pushinteger(L, status);
    lua_call(L, status ? 2 : 1, 1);
    if (!lua_isnil(L, -1))  /* Remember failure status. */
      status = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
  }
  lua_pushinteger(L, status);
  return 1;
}

/* Compiler frontend wrapper. */
static int frontwrap(lua_State *L, Table *st)
{
  jit_State *J = G(L)->jit_state;
  lua_State *JL;
  int status;

  /* Allocate compiler thread on demand. */
  if (J->L == NULL) {
    if (!lua_checkstack(L, 3)) return JIT_S_COMPILER_ERROR;
    sethvalue(L, L->top++, st);  /* Prevent GC of state table. */
    lua_pushlightuserdata(L, (void *)&regkey_comthread);
    /* Cannot use C stack, since it's deallocated early in Coco. */
    /* But we don't need one -- the compiler thread never yields, anyway. */
    J->L = lua_newthread(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
    L->top--;  /* Remove state table from this stack. */
  }
  JL = J->L;

  /* Initialize compiler thread stack with frontend and state table. */
  lua_settop(JL, 0);
  lua_pushlightuserdata(JL, (void *)&regkey_frontend);
  lua_rawget(JL, LUA_REGISTRYINDEX);
  sethvalue(JL, JL->top, st);
  JL->top++;

  /* Start the frontend by resuming the compiler thread. */
  if (lua_resume(JL, 1) != 0) {  /* Failed? */
    /* Note: LUA_YIELD is treated like any other error. */
    J->L = NULL;  /* Get a new thread next time. */
    fprintf(stderr, "[LuaJIT frontend failed: %s]\n",
      lua_isstring(JL, -1) ? lua_tostring(JL, -1) : "(unknown error)");
    return JIT_S_COMPILER_ERROR;
  }

  /* Get status from terminated thread. */
  status = (int)lua_tointeger(JL, -1);
  lua_settop(JL, 0);  /* Help the GC. */
  return status;
}

/* Create the compiler pipeline and register it. */
static void makepipeline(lua_State *L)
{
  lua_createtable(L, 20, 0);  /* 10 handlers+priorities should be enough. */
  lua_pushcfunction(L, luaJIT_backend);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, 0);  /* Fill in the backend at prio 0. */
  lua_rawseti(L, -2, 2);

  /* Store the compiler frontend in the registry. */
  lua_pushlightuserdata(L, (void *)&regkey_frontend);
  lua_pushvalue(L, -2);  /* Pipeline table as upvalue. */
  lua_pushcclosure(L, j_frontend, 1);
  lua_rawset(L, LUA_REGISTRYINDEX);

  /* Register the frontend wrapper. */
  G(L)->jit_state->frontwrap = frontwrap;

  /* Add jit.attach with the pipeline table as upvalue. */
  lua_pushcclosure(L, j_attach, 1);
  lua_setfield(L, -2, "attach");  /* "jit" table must be below. */
}

/* ------------------------------------------------------------------------ */

/* Calculate total mcode size without mfm and only for active mcode blocks. */
static size_t mcodesize(Proto *pt)
{
  jit_MCTrailer tr;
  size_t sz = 0;
  tr.mcode = (char *)pt->jit_mcode;
  tr.sz = pt->jit_szmcode;
  do {
    jit_Mfm *mfm = JIT_MCMFM(tr.mcode, tr.sz);
    if (sz != 0 && jit_mfm_ismain(mfm)) break;  /* Stop at old main mfm. */
    while (*mfm != JIT_MFM_STOP) mfm--;  /* Search for end of mcode. */
    sz += (char *)mfm-(char *)tr.mcode;  /* Add size of mcode without mfm. */
    memcpy((void *)&tr, JIT_MCTRAILER(tr.mcode, tr.sz), sizeof(jit_MCTrailer));
  } while (tr.mcode != NULL);
  return sz;
}

#define setintfield(name, i) \
  do { lua_pushinteger(L, i); lua_setfield(L, -2, name); } while (0)

/* local stats = jit.util.stats(func) */
static int ju_stats(lua_State *L)
{
  if (!(L->top > L->base))
    luaL_argerror(L, 1, "Lua function expected");
  if (isLfunction(L->base)) {
    Proto *pt = clvalue(L->base)->l.p;
    lua_createtable(L, 0, 11);
    setintfield("status", pt->jit_status);
    setintfield("stackslots", pt->maxstacksize);
    setintfield("params", pt->numparams);
    setintfield("bytecodes", pt->sizecode);
    setintfield("consts", pt->sizek);
    setintfield("upvalues", pt->nups);
    setintfield("subs", pt->sizep);
    lua_pushboolean(L, pt->is_vararg);
    lua_setfield(L, -2, "isvararg");
    lua_getfenv(L, 1);
    lua_setfield(L, -2, "env");
    if (pt->jit_szmcode != 0) {
      setintfield("mcodesize", (int)mcodesize(pt));
      lua_pushnumber(L, (lua_Number)(size_t)pt->jit_mcode);
      lua_setfield(L, -2, "mcodeaddr");
    }
    return 1;
  } else {
    return 0;  /* Don't throw an error like the other util functions. */
  }
}

/* local op, a, b, c, test = jit.util.bytecode(func, pc) */
static int ju_bytecode(lua_State *L)
{
  Proto *pt = check_LCL(L)->l.p;
  int pc = luaL_checkint(L, 2);
  if (pc >= 1 && pc <= pt->sizecode) {
    Instruction ins = pt->code[pc-1];
    OpCode op = GET_OPCODE(ins);
    if (pc > 1 && (((int)OP_SETLIST) << POS_OP) ==
	(pt->code[pc-2] & (MASK1(SIZE_OP,POS_OP) | MASK1(SIZE_C,POS_C)))) {
      lua_pushstring(L, luaP_opnames[OP_SETLIST]);
      lua_pushnumber(L, (lua_Number)ins);  /* Fake extended op. */
      return 1;
    }
    if (op >= NUM_OPCODES) return 0;  /* Just in case. */
    lua_pushstring(L, luaP_opnames[op]);
    lua_pushinteger(L, GETARG_A(ins));
    switch (getOpMode(op)) {
    case iABC: {
      int b = GETARG_B(ins), c = GETARG_C(ins);
      switch (getBMode(op)) {
      case OpArgN: lua_pushnil(L); break;
      case OpArgK: if (ISK(b)) b = -1-INDEXK(b);
      case OpArgR: case OpArgU: lua_pushinteger(L, b); break;
      }
      switch (getCMode(op)) {
      case OpArgN: lua_pushnil(L); break;
      case OpArgK: if (ISK(c)) c = -1-INDEXK(c);
      case OpArgR: case OpArgU: lua_pushinteger(L, c); break;
      }
      lua_pushboolean(L, testTMode(op));
      return 5;
    }
    case iABx: {
      int bx = GETARG_Bx(ins);
      lua_pushinteger(L, getBMode(op) == OpArgK ? -1-bx : bx);
      return 3;
    }
    case iAsBx:
      lua_pushinteger(L, GETARG_sBx(ins));
      return 3;
    }
  }
  return 0;
}

/* local const, ok = jit.util.const(func, idx) */
static int ju_const(lua_State *L)
{
  Proto *pt = check_LCL(L)->l.p;
  int idx = luaL_checkint(L, 2);
  if (idx < 0) idx = -idx;  /* Handle both positive and negative indices. */
  if (idx >= 1 && idx <= pt->sizek) {
    setobj2s(L, L->top-1, &pt->k[idx-1]);
    lua_pushboolean(L, 1);
    return 2;
  }
  lua_pushnil(L);
  lua_pushboolean(L, 0);
  return 2;
}

/* local upvalue, ok = jit.util.upvalue(func, idx) */
static int ju_upvalue(lua_State *L)
{
  Closure *cl = check_LCL(L);
  Proto *pt = cl->l.p;
  int idx = luaL_checkint(L, 2);
  if (idx >= 0 && idx < pt->nups) {
    setobj2s(L, L->top-1, cl->l.upvals[idx]->v);
    lua_pushboolean(L, 1);
    return 2;
  }
  lua_pushnil(L);
  lua_pushboolean(L, 0);
  return 2;
}

/* local nup = jit.util.closurenup(func, idx) */
static int ju_closurenup(lua_State *L)
{
  Closure *cl = check_LCL(L);
  Proto *pt = cl->l.p;
  int idx = luaL_checkint(L, 2);
  if (idx >= 0 && idx < pt->sizep) {
    lua_pushinteger(L, pt->p[idx]->nups);
    return 1;
  }
  return 0;
}

/* for tag, mark in mfmiter do ... end. */
static int ju_mfmiter(lua_State *L)
{
  jit_Mfm *mfm = (jit_Mfm *)lua_touserdata(L, lua_upvalueindex(1));
  int m = *mfm--;
  switch (m) {
  case JIT_MFM_STOP: return 0;
  case JIT_MFM_COMBINE: lua_pushliteral(L, "COMBINE"); lua_pushnil(L); break;
  case JIT_MFM_DEAD: lua_pushliteral(L, "DEAD"); lua_pushnil(L); break;
  default:
    lua_pushinteger(L, m & JIT_MFM_MASK);
    lua_pushboolean(L, m & JIT_MFM_MARK);
    break;
  }
  lua_pushlightuserdata(L, (void *)mfm);
  lua_replace(L, lua_upvalueindex(1));
  return 2;
}

/* local addr, mcode, mfmiter = jit.util.mcode(func, block) */
static int ju_mcode(lua_State *L)
{
  Proto *pt = check_LCL(L)->l.p;
  if (pt->jit_szmcode == 0) {  /* Not compiled (yet): return nil, status. */
    lua_pushnil(L);
    lua_pushinteger(L, pt->jit_status);
    return 2;
  } else {
    jit_Mfm *mfm;
    jit_MCTrailer tr;
    int block = luaL_checkint(L, 2);
    tr.mcode = (char *)pt->jit_mcode;
    tr.sz = pt->jit_szmcode;
    while (--block > 0) {
      void *trp = JIT_MCTRAILER(tr.mcode, tr.sz);
      memcpy((void *)&tr, trp, sizeof(jit_MCTrailer));
      if (tr.sz == 0) return 0;
    }
    mfm = JIT_MCMFM(tr.mcode, tr.sz);
    while (*mfm != JIT_MFM_STOP) mfm--;  /* Search for end of mcode. */
    lua_pushnumber(L, (lua_Number)(size_t)tr.mcode);
    lua_pushlstring(L, (const char *)tr.mcode, (char *)mfm-(char *)tr.mcode);
    lua_pushlightuserdata(L, (void *)JIT_MCMFM(tr.mcode, tr.sz));
    lua_pushvalue(L, 1);  /* Must hold onto function to avoid GC. */
    lua_pushcclosure(L, ju_mfmiter, 2);
    return 3;
  }
}

/* local addr [, mcode] = jit.util.jsubmcode([idx]) */
static int ju_jsubmcode(lua_State *L)
{
  jit_State *J = G(L)->jit_state;
  if (lua_isnoneornil(L, 1)) {
    lua_pushnumber(L, (lua_Number)(size_t)J->jsubmcode);
    lua_pushlstring(L, (const char *)J->jsubmcode, J->szjsubmcode);
    return 2;
  } else {
    int idx = luaL_checkint(L, 1);
    if (idx >= 0 && idx < J->numjsub) {
      lua_pushnumber(L, (lua_Number)(size_t)J->jsub[idx]);
      return 1;
    }
    return 0;
  }
}

/* FOR INTERNAL DEBUGGING USE ONLY: local addr = jit.util.stackptr() */
static int ju_stackptr(lua_State *L)
{
  jit_State *J = G(L)->jit_state;
  size_t addr = cast(size_t (*)(void), J->jsub[0])();  /* JSUB_STACKPTR == 0! */
  lua_pushnumber(L, (lua_Number)addr);
  return 1;
}

/* jit.util.* functions. */
static const luaL_Reg jitutillib[] = {
  {"stats",		ju_stats },
  {"bytecode",		ju_bytecode },
  {"const",		ju_const },
  {"upvalue",		ju_upvalue },
  {"closurenup",	ju_closurenup },
  {"mcode",		ju_mcode },
  {"jsubmcode",		ju_jsubmcode },
  {"stackptr",		ju_stackptr },
  { NULL, NULL }
};

/* Make hint name to hint number map. */
static void makehints(lua_State *L, const char *const *t, int tmax,
		      const char *name)
{
  int i;
  lua_createtable(L, 0, tmax);
  for (i = 1; i < tmax; i++) {
    lua_pushinteger(L, JIT_H2NUM(i));
    lua_setfield(L, -2, t[i-1]);
  }
  lua_setfield(L, -2, name);
}

/* CHECK: must match with ljit.h (grep "ORDER JIT_S"). */
static const char *const status_list[] = {
  "OK",
  "NONE",
  "OFF",
  "ENGINE_OFF",
  "DELAYED",
  "TOOLARGE",
  "COMPILER_ERROR",
  "DASM_ERROR"
};

/* Make bidirectional status name to status number map. */
static void makestatus(lua_State *L, const char *name)
{
  int i;
  lua_createtable(L, JIT_S_MAX-1, JIT_S_MAX+1);  /* Codes are not 1-based. */
  for (i = 0; i < JIT_S_MAX; i++) {
    lua_pushstring(L, status_list[i]);
    lua_pushinteger(L, i);
    lua_pushvalue(L, -2);
    lua_rawseti(L, -4, i);
    lua_rawset(L, -3);
  }
  lua_setfield(L, -2, name);
}

/* ------------------------------------------------------------------------ */

/*
** Open JIT library
*/
LUALIB_API int luaopen_jit(lua_State *L)
{
  /* Add the core JIT library. */
  luaL_register(L, LUA_JITLIBNAME, jitlib);
  lua_pushliteral(L, LUAJIT_VERSION);
  lua_setfield(L, -2, "version");
  setintfield("version_num", LUAJIT_VERSION_NUM);
  lua_pushstring(L, luaJIT_arch);
  lua_setfield(L, -2, "arch");
  makepipeline(L);

  /* Add the utility JIT library. */
  luaL_register(L, LUA_JITLIBNAME ".util", jitutillib);
  makestatus(L, "status");
  makehints(L, hints_H, JIT_H_MAX, "hints");
  makehints(L, hints_FH, JIT_FH_MAX, "fhints");
  lua_pop(L, 1);

  /* Everything ok, so turn the JIT engine on. Vroooom! */
  if (luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_ON) <= 0) {
    /* Ouch. Someone screwed up DynASM or the JSUBs. Probably me. */
    /* But if you get 999999999, look at jit_consistency_check(). */
    return luaL_error(L, "JIT engine init failed (%d)",
	G(L)->jit_state->dasmstatus);
  }

  return 1;
}

