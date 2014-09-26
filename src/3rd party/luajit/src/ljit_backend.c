/*
** LuaJIT wrapper for architecture-specific compiler backend.
** Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
*/

#include <math.h>
#include <string.h>

#define ljit_backend_c
#define LUA_CORE

#include <lua/lua.h>

#include "lobject.h"
#include "lstate.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"
#include "lvm.h"
#include "lopcodes.h"
#include "ldebug.h"
#include "lzio.h"

#include "ljit.h"
#include "ljit_hints.h"
#include "ljit_dasm.h"

/* ------------------------------------------------------------------------ */

/* Get target of combined JMP op. */
static int jit_jmp_target(jit_State *J)
{
  J->combine++;
  jit_assert(GET_OPCODE(*J->nextins)==OP_JMP);
  return J->nextpc + 1 + GETARG_sBx(*J->nextins);
}

/* ------------------------------------------------------------------------ */

/* Include pre-processed architecture-specific backend. */
#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
#ifndef LUA_NUMBER_DOUBLE
#error "No support for other number types on x86 (yet)"
#endif
#include "ljit_x86.h"
#else
#error "No support for this architecture (yet)"
#endif

/* ------------------------------------------------------------------------ */

/* Compile instruction range. */
static void jit_compile_irange(jit_State *J, int firstpc, int lastpc)
{
  J->combine = 0;
  J->nextpc = firstpc;
  J->nextins = J->pt->code + (firstpc-1);
  while (J->nextpc <= lastpc) {
    Instruction ins = *J->nextins++;
    OpCode op = GET_OPCODE(ins);
    int ra = GETARG_A(ins);
    int rb = GETARG_B(ins);
    int rc = GETARG_C(ins);
    int rbx = GETARG_Bx(ins);
    const TValue *combinehint;

    jit_ins_start(J);
    J->nextpc++;

    combinehint = hint_get(J, COMBINE);
    if (ttisboolean(combinehint)) {
      if (bvalue(combinehint)) {  /* COMBINE = true: combine with next ins. */
	if (!(J->flags & JIT_F_DEBUG))  /* But not when debugging. */
	  J->combine = 1;
      } else {  /* COMBINE = false: dead instruction. */
	*J->mfm++ = JIT_MFM_DEAD;
	continue;
      }
    }  /* Other COMBINE hint value types are not defined (yet). */

    if (J->flags & JIT_F_DEBUG_INS)
      jit_ins_debug(J, luaG_checkopenop(ins));

    switch (op) {
    case OP_MOVE: jit_op_move(J, ra, rb); break;
    case OP_LOADK: jit_op_loadk(J, ra, rbx); break;
    case OP_LOADBOOL: jit_op_loadbool(J, ra, rb, rc); break;
    case OP_LOADNIL: jit_op_loadnil(J, ra, rb); break;

    case OP_GETUPVAL: jit_op_getupval(J, ra, rb); break;
    case OP_SETUPVAL: jit_op_setupval(J, ra, rb); break;

    case OP_GETGLOBAL: jit_op_getglobal(J, ra, rbx); break;
    case OP_SETGLOBAL: jit_op_setglobal(J, ra, rbx); break;

    case OP_NEWTABLE: jit_op_newtable(J, ra, rb, rc); break;
    case OP_GETTABLE: jit_op_gettable(J, ra, rb, rc); break;
    case OP_SETTABLE: jit_op_settable(J, ra, rb, rc); break;
    case OP_SELF: jit_op_self(J, ra, rb, rc); break;
    case OP_SETLIST: jit_op_setlist(J, ra, rb, rc); break;

    case OP_ADD: jit_op_arith(J, ra, rb, rc, TM_ADD); break;
    case OP_SUB: jit_op_arith(J, ra, rb, rc, TM_SUB); break;
    case OP_MUL: jit_op_arith(J, ra, rb, rc, TM_MUL); break;
    case OP_DIV: jit_op_arith(J, ra, rb, rc, TM_DIV); break;
    case OP_MOD: jit_op_arith(J, ra, rb, rc, TM_MOD); break;
    case OP_POW: jit_op_arith(J, ra, rb, rc, TM_POW); break;
    case OP_UNM: jit_op_arith(J, ra, rb, rb, TM_UNM); break;  /* rc unused. */

    case OP_LEN: jit_op_len(J, ra, rb); break;
    case OP_NOT: jit_op_not(J, ra, rb); break;

    case OP_CONCAT: jit_op_concat(J, ra, rb, rc); break;

    case OP_EQ: jit_op_eq(J, ra, rb, rc); break;
    case OP_LT: jit_op_arith(J, ra, rb, rc, TM_LT); break;
    case OP_LE: jit_op_arith(J, ra, rb, rc, TM_LE); break;

    case OP_TEST: jit_op_test(J, rc, ra, ra); break;
    case OP_TESTSET: jit_op_test(J, rc, ra, rb); break;

    case OP_JMP: jit_op_jmp(J, J->nextpc + rbx-MAXARG_sBx); break;

    case OP_CALL: jit_op_call(J, ra, rb-1, rc-1); break;
    case OP_TAILCALL: jit_op_tailcall(J, ra, rb-1); break;
    case OP_RETURN: jit_op_return(J, ra, rb-1); break;

    case OP_FORLOOP: jit_op_forloop(J, ra, J->nextpc + rbx-MAXARG_sBx); break;
    case OP_FORPREP: jit_op_forprep(J, ra, J->nextpc + rbx-MAXARG_sBx); break;

    case OP_TFORLOOP: jit_op_tforloop(J, ra, rc); break;

    case OP_CLOSE: jit_op_close(J, ra); break;
    case OP_CLOSURE: jit_op_closure(J, ra, rbx); break;

    case OP_VARARG: jit_op_vararg(J, ra, rb-1); break;

    default: jit_assert(0); break;
    }

    /* Convention: all opcodes start and end with the .code section. */
    if (dasm_checkstep(Dst, DASM_SECTION_CODE)) { J->nextpc--; return; }

    *J->mfm++ = 0;  /* Placeholder mfm entry. Replaced later. */
    if (J->combine > 0) {  /* Combine next J->combine ins with prev ins. */
      J->nextpc += J->combine;
      J->nextins += J->combine;
      do { *J->mfm++ = JIT_MFM_COMBINE; } while (--J->combine);
    }
  }
}

/* Merge temporary mfm (forward) with PC labels to inverse mfm in mcode. */
static void jit_mfm_merge(jit_State *J, jit_Mfm *from, jit_Mfm *to, int maxpc)
{
  int pc = 1, ofs = 0;
  for (;;) {
    int m = *from++;
    if (m & JIT_MFM_MARK) {
      switch (m) {
      default: pc = m ^ JIT_MFM_MARK; break;
      case JIT_MFM_COMBINE: case JIT_MFM_DEAD: break;
      case JIT_MFM_STOP: return;
      }
    } else {
      int idx, nofs;
      for (idx = 0; from[idx] == JIT_MFM_COMBINE; idx++) ;
      idx += pc;
      if (idx == J->nextpc) idx = maxpc + 1;
      nofs = dasm_getpclabel(Dst, idx);
      m = nofs - ofs;
      ofs = nofs;
      jit_assert(nofs >= 0 && m >= 0 && m < JIT_MFM_MAX);
    }
    pc++;
    *to-- = (jit_Mfm)m;
  }
}

/* Compile function prototype. */
static int jit_compile_proto(jit_State *J, Table *deopt)
{
  jit_Mfm *tempmfm;
  void *mcode;
  size_t sz;
  int firstpc = 0, maxpc = J->pt->sizecode;
  int deoptidx = 1;
  int status;
  /* (Ab)use the global string concatenation buffer for the temporary mfm. */
  /* Caveat: the GC must not be run while the backend is active. */
  tempmfm = (jit_Mfm *)luaZ_openspace(J->L, &G(J->L)->buff,
				      (1+maxpc+1+1+1)*sizeof(jit_Mfm));
nextdeopt:
  J->mfm = tempmfm;
  J->tflags = 0;
  /* Setup DynASM. */
  dasm_growpc(Dst, 1+maxpc+2);  /* See jit_ins_last(). */
  dasm_setup(Dst, jit_actionlist);
  if (deopt) {  /* Partial deoptimization. */
    /* TODO: check deopt chain length? problem: pairs TFOR_CTL migration. */
    int pc, lastpc;
    lua_Number n;
    const TValue *obj = luaH_getnum(deopt, deoptidx++);
    if (ttisnil(obj) && deoptidx != 2) return JIT_S_OK;
    if (!ttisnumber(obj)) return JIT_S_COMPILER_ERROR;
    n = nvalue(obj);
    lua_number2int(pc, n);
    firstpc = JIT_IH_IDX(pc);
    lastpc = firstpc + JIT_IH_LIB(pc);
    if (firstpc < 1 || firstpc > maxpc || lastpc > maxpc ||
	J->pt->jit_szmcode == 0)
      return JIT_S_COMPILER_ERROR;
    *J->mfm++ = (jit_Mfm)(JIT_MFM_MARK+firstpc);  /* Seek to firstpc. */
    jit_compile_irange(J, firstpc, lastpc);
    jit_assert(J->nextpc == lastpc+1);  /* Problem with combined ins? */
    if (J->nextpc <= maxpc) jit_ins_chainto(J, J->nextpc);
    *J->mfm++ = (jit_Mfm)(JIT_MFM_MARK+maxpc+1);  /* Seek to .deopt/.tail. */
    for (pc = 1; pc <= maxpc; pc++)
      if (dasm_getpclabel(Dst, pc) == -1) {  /* Undefind label referenced? */
	jit_ins_setpc(J, pc, luaJIT_findmcode(J->pt, pc));  /* => Old mcode. */
      }
  } else {  /* Full compile. */
    *J->mfm++ = 0;  /* Placeholder mfm entry for prologue. */
    jit_prologue(J);
    jit_compile_irange(J, 1, maxpc);
  }
  *J->mfm++ = 0;  /* Placeholder mfm entry for .deopt/.tail. */
  *J->mfm = JIT_MFM_STOP;
  jit_ins_last(J, maxpc, (char *)J->mfm - (char *)tempmfm);

  status = luaJIT_link(J, &mcode, &sz);
  if (status != JIT_S_OK)
    return status;

  jit_mfm_merge(J, tempmfm, JIT_MCMFM(mcode, sz), maxpc);

  if (deopt) {
    jit_MCTrailer tr;
    /* Patch first instruction to jump to the deoptimized code. */
    jit_patch_jmp(J, luaJIT_findmcode(J->pt, firstpc), mcode);
    /* Mark instruction as deoptimized in main mfm. */
    JIT_MCMFM(J->pt->jit_mcode, J->pt->jit_szmcode)[-firstpc] |= JIT_MFM_MARK;
    /* Chain deopt mcode block between main mfm and existing mfms. */
    memcpy(JIT_MCTRAILER(mcode, sz),
	   JIT_MCTRAILER(J->pt->jit_mcode, J->pt->jit_szmcode),
	   sizeof(jit_MCTrailer));
    tr.mcode = (char *)mcode;
    tr.sz = sz;
    memcpy(JIT_MCTRAILER(J->pt->jit_mcode, J->pt->jit_szmcode), (void *)&tr,
	   sizeof(jit_MCTrailer));
    goto nextdeopt;
  }

  if (J->pt->jit_szmcode != 0) {  /* Full recompile? */
    jit_MCTrailer tr;
    /* Patch old mcode entry so other closures get the new callgate. */
    jit_patch_jmp(J, J->pt->jit_mcode, J->jsub[JSUB_GATE_JL]);
    /* Chain old main mfm after new main mfm. */
    tr.mcode = (char *)J->pt->jit_mcode;
    tr.sz = J->pt->jit_szmcode;
    memcpy(JIT_MCTRAILER(mcode, sz), (void *)&tr, sizeof(jit_MCTrailer));
  }
  /* Set new main mcode block. */
  J->pt->jit_mcode = mcode;
  J->pt->jit_szmcode = sz;
  return JIT_S_OK;
}

/* ------------------------------------------------------------------------ */

/* Compiler backend. */
int luaJIT_backend(lua_State *L)
{
  jit_State *J = G(L)->jit_state;
  const TValue *cl;
  int status = JIT_S_COMPILER_ERROR;
  lua_lock(L);
  /* Remember compiler state table. */
  jit_assert(L->top > L->base && ttistable(L->top-1));
  J->comstate = hvalue(L->top-1);
  /* Fetch prototoype. Better check this in case some handler screwed up. */
  cl = luaH_getstr(J->comstate, luaS_newliteral(L, "func"));
  if (isLfunction(cl)) {
    J->pt = clvalue(cl)->l.p;
    if (J->pt->sizecode > LUAJIT_LIM_BYTECODE) {  /* Hard backend limit. */
      status = JIT_S_TOOLARGE;
    } else {
      const TValue *obj = luaH_getstr(J->comstate,
				      luaS_newliteral(J->L, "deopt"));
      status = jit_compile_proto(J, ttistable(obj) ? hvalue(obj) : (Table *)0);
    }
  }
  lua_unlock(L);
  J->comstate = NULL;  /* Just in case. */
  J->pt = NULL;
  if (status == JIT_S_OK) {
    return 0;
  } else {
    if (status == JIT_S_DASM_ERROR) {
      lua_pushinteger(L, J->nextpc);
      lua_setfield(L, 1, "dasm_pc");
      lua_pushinteger(L, J->dasmstatus);
      lua_setfield(L, 1, "dasm_err");
    }
    lua_pushinteger(L, status);
    return 1;
  }
}

/* Initialize compiler backend. */
int luaJIT_initbackend(lua_State *L)
{
  jit_State *J = G(L)->jit_state;
  J->L = L;
  J->pt = NULL;  /* Not in use. */
  J->D = NULL;
  J->mcodeheap = NULL;
  J->jsubmcode = NULL;
  J->szjsubmcode = 0;
  J->numjsub = JSUB__MAX;
  J->jsub = luaM_newvector(J->L, JSUB__MAX, void *);
  memset((void *)J->jsub, 0, JSUB__MAX*sizeof(void *));  /* Just in case. */
  dasm_init(Dst, DASM_MAXSECTION);
  dasm_setupglobal(Dst, J->jsub, JSUB__MAX);
  return jit_compile_jsub(J);
}

/* Free compiler backend. */
void luaJIT_freebackend(lua_State *L)
{
  jit_State *J = G(L)->jit_state;
  J->L = L;
  if (J->jsub) luaM_freearray(L, J->jsub, JSUB__MAX, void *);
  luaJIT_freemcodeheap(J);  /* Frees JSUB mcode, too. */
  dasm_free(Dst);
}

/* ------------------------------------------------------------------------ */

