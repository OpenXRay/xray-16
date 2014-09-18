/*
** Interface to JIT engine.
** Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
*/

#define ljit_core_c
#define LUA_CORE

#include <string.h>

#include <lua/lua.h>

#include "lobject.h"
#include "lstate.h"
#include "ldo.h"
#include "lstring.h"
#include "ltable.h"
#include "ldebug.h"
#include "lopcodes.h"

#include "ljit.h"
#include "ljit_hints.h"
#include "luajit.h"

const char luajit_ident[] =
  "$LuaJIT: " LUAJIT_VERSION " " LUAJIT_COPYRIGHT " " LUAJIT_URL " $\n";

/* ------------------------------------------------------------------------ */

/* Initialize JIT engine state. */
void luaJIT_initstate(lua_State *L)
{
  jit_State *J = luaM_new(L, jit_State);
  G(L)->jit_state = J;
  /* Clear JIT engine fields. */
  J->frontwrap = NULL;  /* Filled in by ljitlib before enabling the engine. */
  J->flags = 0;  /* Disable the JIT engine by default. */
  /* Try to initialize the backend. */
  if (luaJIT_initbackend(L) != JIT_S_OK)
    J->flags = JIT_F_INIT_FAILED;
  J->L = NULL;  /* No compiler thread allocated, yet. */
}

/* Free JIT engine state. */
void luaJIT_freestate(lua_State *L)
{
  jit_State *J = G(L)->jit_state;
  if (J == NULL) return;
  luaJIT_freebackend(L);
  luaM_free(L, J);
  G(L)->jit_state = NULL;
}

/* ------------------------------------------------------------------------ */

/* Find relative PC (0 based) for a bytecode pointer or a JIT mcode address. */
int luaJIT_findpc(Proto *pt, const Instruction *savedpc)
{
  ptrdiff_t pcdiff = savedpc - pt->code;
  if (pcdiff >= 0 && pcdiff <= pt->sizecode) { /* Bytecode pointer? */
    return (int)pcdiff-1;
  } else {  /* Else translate JIT mcode address to PC. */
    char *addr = (char *)savedpc;
    jit_MCTrailer tr;
    tr.mcode = (char *)pt->jit_mcode;
    tr.sz = pt->jit_szmcode;
    /* Follow trailer chain until addr is part of an mcode block. */
    while (!((size_t)(addr - tr.mcode) < tr.sz)) {
      memcpy((void *)&tr, JIT_MCTRAILER(tr.mcode, tr.sz),
	     sizeof(jit_MCTrailer));
      if (tr.mcode == NULL) return -1;  /* Not in chain. */
    }
    {
      jit_Mfm *mfm = JIT_MCMFM(tr.mcode, tr.sz);
      int ofs = (int)(addr - tr.mcode);
      int isdeopt = !jit_mfm_ismain(mfm);
      int pc = 0;  /* Prologue fragment is at start of main mfm. */
      while (pc <= pt->sizecode) {
	int m = *mfm--;
	switch (m) {
	default:
	  if (m & JIT_MFM_MARK) {
	    m ^= JIT_MFM_MARK;
	    if (isdeopt) { pc = m; continue; }  /* Seek. */
	  }
	  ofs -= m;
	  if (ofs <= 0) return pc-1;  /* Found! */
	case JIT_MFM_COMBINE:
	case JIT_MFM_DEAD:
	  pc++;
	  break;
	case JIT_MFM_STOP:
	  jit_assert(0);  /* Premature STOP found. */
	  return -1;
	}
      }
      jit_assert(0);  /* Address is in .tail. */
      return -1;
    }
  }
}

/* Lookup mcode address for PC (1 based) in mfm. */
static void *jit_mfmlookup(jit_Mfm *mfm, char *addr, int mpc)
{
  int isdeopt = !jit_mfm_ismain(mfm);
  int pc = 0;  /* Prologue fragment is at start of main mfm. */
  while (pc != mpc) {
    int m = *mfm--;
    switch (m) {
    default:
      if (m & JIT_MFM_MARK) {
	m ^= JIT_MFM_MARK;
	if (isdeopt) { pc = m; continue; }  /* Seek. */
      }
      addr += m;
    case JIT_MFM_COMBINE:
    case JIT_MFM_DEAD:
      pc++;
      break;
    case JIT_MFM_STOP:
      return NULL;
    }
  }
  return (void *)addr;
}

/* Find mcode address for PC (1 based). */
void *luaJIT_findmcode(Proto *pt, int pc)
{
  void *addr = NULL;
  jit_Mfm *mfm;
  jit_MCTrailer tr;
  tr.mcode = (char *)pt->jit_mcode;
  tr.sz = pt->jit_szmcode;
  mfm = JIT_MCMFM(tr.mcode, tr.sz);
  jit_assert(pc >= 1 && pc <= pt->sizecode);
  while (mfm[-pc] == JIT_MFM_COMBINE) pc--;
  while (mfm[-pc] == JIT_MFM_DEAD) pc++;
  jit_assert(pc >= 1 && mfm[-pc] < (JIT_MFM_MARK|JIT_MFM_MAX)); /* Valid? */
  if (jit_mfm_isdeoptpc(mfm, pc)) {  /* Deoptimized instruction. */
    do {  /* Search through deopt mfm chain. */
      memcpy((void *)&tr, JIT_MCTRAILER(tr.mcode, tr.sz),
	     sizeof(jit_MCTrailer));
      if (tr.mcode == NULL) break;  /* Deopt ins missing in chain. */
      mfm = JIT_MCMFM(tr.mcode, tr.sz);
      if (jit_mfm_ismain(mfm)) break;  /* Old main mfm stops search, too. */
      addr = jit_mfmlookup(mfm, tr.mcode, pc);
    } while (addr == NULL);
  } else { /* Not deoptimized. Lookup in main mfm. */
    addr = jit_mfmlookup(mfm, tr.mcode, pc);
  }
  jit_assert(addr != NULL);  /* Corrupt mfm chain. */
  return addr;
}

/* ------------------------------------------------------------------------ */

/* Compile a prototype. */
/* Note: func pointer may be invalidated because of stack reallocation. */
static int jit_compile(lua_State *L, StkId func, Table *st, int force)
{
  jit_State *J = G(L)->jit_state;
  Closure *cl = clvalue(func);
  Proto *pt = cl->l.p;
  int status;

  /* Check if JIT engine is enabled and prevent recursive invocation. */
  if ((J->flags & JIT_F_INIT_FAILED) ||
      (!force && !(J->flags & JIT_F_ON)) ||
      !J->frontwrap) {
    status = JIT_S_ENGINE_OFF;
  } else if (J->flags & JIT_F_COMPILING) {
    status = JIT_S_DELAYED;
  } else if (pt->jit_szmcode != 0 && force < 2) {  /* Prevent recompile. */
    /* TODO: Allow recompiles? Use case? Extra flag for jit.compile()? */
    status = JIT_S_OK;
  } else {
    setclvalue(L, luaH_setstr(L, st, luaS_newliteral(L, "func")), cl);
    /* Call frontend wrapper. */
    J->flags |= JIT_F_COMPILING;
    lua_unlock(L);
    status = J->frontwrap(L, st);
    lua_lock(L);
    J->flags &= ~JIT_F_COMPILING;
  }

  /* Better sanity check what the frontend returns. */
  if ((status == JIT_S_OK && pt->jit_szmcode == 0) || status == JIT_S_NONE)
    status = JIT_S_COMPILER_ERROR;

  /* Update closure callgate and prototype status. */
  cl->l.jit_gate = (status == JIT_S_OK) ? (lua_CFunction)pt->jit_mcode :
					  G(L)->jit_gateJL;
  pt->jit_status = status;
  return status;
}

/* Create the state table and copy the arguments. */
static Table *jit_createstate(lua_State *L, StkId arg, int nargs)
{
  Table *st;
  int i;
  luaC_checkGC(L);
  st = luaH_new(L, nargs, COMSTATE_PREALLOC);
  for (i = 0; i < nargs; i++)
    setobj2t(L, luaH_setnum(L, st, i+1), arg+i);
  return st;
}

/* ------------------------------------------------------------------------ */

/* Compile and run a function. To be used by luaD_precall() only. */
int luaJIT_run(lua_State *L, StkId func, int nresults)
{
  ptrdiff_t funcr = savestack(L, func);
  Table *st = jit_createstate(L, func+1, L->top-(func+1));
  int status = jit_compile(L, func, st, 0);  /* Compile function. */
  func = restorestack(L, funcr);

  /* Run the compiled function on success. Fallback to bytecode on failure. */
  if (status == JIT_S_OK)
    return G(L)->jit_gateLJ(L, func, nresults);
  else
    return luaD_precall(L, func, nresults);
  /* Note: We are called from luaD_precall and we call it again. */
  /* So jit_compile makes sure pt->jit_status != JIT_S_NONE. */
}

/* ------------------------------------------------------------------------ */

/* No more than two ranges for a single deoptimization right now. */
#define DEOPTRANGE_ALLOC	2

/* Find PC range of combined instructions and return a range hint. */
static int combinedrange(jit_Mfm *mfm, int pc)
{
  int lastpc = pc;
  while (mfm[-pc] == JIT_MFM_COMBINE) pc--;  /* 1st comb. ins. */
  while (mfm[-(lastpc+1)] == JIT_MFM_COMBINE) lastpc++;  /* Last comb. ins. */
  return JIT_IH_MKIDX(lastpc-pc, pc);  /* (#ins-1, pc) in hint format. */
}

/* Process deoptimization hints for the given PC range. */
static int deopthints(Proto *pt, jit_Mfm *dh, TValue *dhint, int pcrange)
{
  int m;
  setnvalue(dhint++, (lua_Number)pcrange);
  while ((m = *dh--) != JIT_MFM_STOP) {
    if ((unsigned int)(m - JIT_IH_IDX(pcrange)) <=
	(unsigned int)JIT_IH_LIB(pcrange)) {
      switch (*dh--) {
      case JIT_MFM_DEOPT_PAIRS:  /* CALL [i]pairs(): deopt TFORLOOP+JMP. */
	if (GET_OPCODE(pt->code[m+1-1]) == OP_JMP) {
	  int tfpc = m+2 + GETARG_sBx(pt->code[m+1-1]);
	  if ((unsigned)tfpc < (unsigned)pt->sizecode &&
	      GET_OPCODE(pt->code[tfpc-1]) == OP_TFORLOOP) {
	    setnvalue(dhint++, (lua_Number)JIT_IH_MKIDX(1, tfpc));
	    break;
	  }
	}
	return 1;  /* Bad hint. */
      default:
	return 1;  /* Cannot tolerate unknown deoptimization hints. */
      }
    }
  }
  return 0;  /* Ok. */
}

/* Deoptimize the current instruction. Return new mcode addr to continue. */
void *luaJIT_deoptimize(lua_State *L)
{
  StkId func = L->ci->func;
  Proto *pt = clvalue(func)->l.p;
  int pc = luaJIT_findpc(pt, L->savedpc)+1;  /* Get prev. PC (1 based). */
  jit_Mfm *mfm = JIT_MCMFM(pt->jit_mcode, pt->jit_szmcode);
  int pcrange = combinedrange(mfm, pc);
  if (!jit_mfm_isdeoptpc(mfm, JIT_IH_IDX(pcrange))) {  /* Not deopt. yet? */
    Table *st = jit_createstate(L, NULL, 0);  /* Don't know original args. */
    Table *deopt = luaH_new(L, DEOPTRANGE_ALLOC, 0);
    sethvalue(L, luaH_setstr(L, st, luaS_newliteral(L, "deopt")), deopt);
    if (deopthints(pt, mfm-(pt->sizecode+2), deopt->array, pcrange) ||
	jit_compile(L, func, st, 2) != JIT_S_OK)
      luaG_runerror(L, "deoptimization failed");
  }
  return luaJIT_findmcode(pt, pc);
}

/* ------------------------------------------------------------------------ */

/* API function: Compile a Lua function. Pass arguments as hints. */
LUA_API int luaJIT_compile(lua_State *L, int nargs)
{
  StkId func;
  Table *st;
  int status;
  lua_lock(L);
  api_check(L, (nargs+1) <= (L->top - L->base));
  func = L->top - (nargs+1);
  st = jit_createstate(L, func+1, nargs);
  status = isLfunction(func) ? jit_compile(L, func, st, 1) : -1;
  lua_unlock(L);
  return status;
}

/* Recursively set the mode for all subroutines. */
static void rec_setmode(Proto *pt, int on)
{
  int i;
  for (i = 0; i < pt->sizep; i++) {
    Proto *pti = pt->p[i];
    pti->jit_status = on ? (pti->jit_szmcode?JIT_S_OK:JIT_S_NONE) : JIT_S_OFF;
    rec_setmode(pti, on);  /* Recurse into proto. */
  }
}

/* API function: Set the JIT mode for the whole engine or a function+subs. */
LUA_API int luaJIT_setmode(lua_State *L, int idx, int mode)
{
  jit_State *J = G(L)->jit_state;
  int mm = mode & LUAJIT_MODE_MASK;
  if (J->flags & JIT_F_INIT_FAILED) return -1;  /* Init failed. */
  switch (mm) {
  case LUAJIT_MODE_ENGINE:		/* Set mode for JIT engine. */
    if (mode & LUAJIT_MODE_ON)
      J->flags |= JIT_F_ON;
    else
      J->flags &= ~JIT_F_ON;
    break;
  case LUAJIT_MODE_DEBUG: {		/* Set debug mode. */
    int dbg;
    switch (idx) {
    case 0: dbg = 0; break;
    case 1: dbg = JIT_F_DEBUG_CALL; break;
    case 2: default: dbg = JIT_F_DEBUG_CALL | JIT_F_DEBUG_INS; break;
    }
    J->flags = (J->flags & ~JIT_F_DEBUG) | dbg;
    luaJIT_debugnotify(J);
    break;
  }
  case LUAJIT_MODE_FUNC:		/* Set mode for function. */
  case LUAJIT_MODE_ALLFUNC:		/* Set mode for function + subfuncs. */
  case LUAJIT_MODE_ALLSUBFUNC: {	/* Set mode for subfunctions. */
    StkId func;
    lua_lock(L);
    func = idx == 0 ? (L->ci-1)->func :
	   (idx > 0 ? L->base + (idx-1) : L->top + idx);
    if (isLfunction(func)) {
      Closure *cl = clvalue(func);
      Proto *pt = cl->l.p;
      if (mm != LUAJIT_MODE_ALLSUBFUNC) {
	if (mode & LUAJIT_MODE_ON) {
	  if (pt->jit_szmcode) {  /* Already compiled? */
	    cl->l.jit_gate = (lua_CFunction)pt->jit_mcode;  /* Reenable. */
	    pt->jit_status = JIT_S_OK;
	  } else {
	    pt->jit_status = JIT_S_NONE;  /* (Re-)enable proto compilation */
	  }
	} else {
	  cl->l.jit_gate = G(L)->jit_gateJL;  /* Default callgate. */
	  pt->jit_status = JIT_S_OFF;  /* Disable proto compilation. */
	  /* Note: compiled code must be retained for suspended threads. */
	}
      }
      if (mm != LUAJIT_MODE_FUNC)
	rec_setmode(pt, mode & LUAJIT_MODE_ON);
      lua_unlock(L);
    } else {
      lua_unlock(L);
      return 0;  /* Failed. */
    }
    break;
  }
  default:
    return 0;  /* Failed. */
  }
  return 1;  /* OK. */
}

/* Enforce (dynamic) linker error for version mismatches. See lua.c. */
LUA_API void LUAJIT_VERSION_SYM(void)
{
}

/* ------------------------------------------------------------------------ */

