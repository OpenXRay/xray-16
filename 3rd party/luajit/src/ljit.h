/*
** Interface to JIT engine.
** Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef ljit_h
#define ljit_h

#include "lobject.h"


/* Define this to enable assertions when debugging LuaJIT. */
#ifdef LUAJIT_ASSERT
#include <assert.h>
#define jit_assert(x)	assert(x)
#define DASM_CHECKS
#else
/* A better idea is to define lua_assert() in luaconf.h. */
#define jit_assert(x)	lua_assert(x)
#endif

/* Define this to set the C stack size for the compiler thread. */
/* The compiler runs on the callers C stack otherwise. */
#undef LUAJIT_COMPILER_CSTACK

/* Hardcoded limits for the backend to avoid useless work. */
/* Note: mind you, these are very generous limits. Check jit.opt, too. */
#define LUAJIT_LIM_BYTECODE	3000	/* Max. # of bytecodes. */
#define LUAJIT_LIM_MCODE	128000	/* Max. mcode size of a function. */

/* Global JIT engine flags. */
#define JIT_F_ON		0x0001	/* JIT engine is on. */
#define JIT_F_COMPILING		0x0002	/* Currently compiling. */
#define JIT_F_INIT_FAILED	0x0004	/* Initialization failed. */

#define JIT_F_CPU_CMOV		0x0010	/* CPU has conditional move support. */
#define JIT_F_CPU_SSE2		0x0020	/* CPU has SSE2 support. */

#define JIT_F_DEBUG_CALL	0x0100	/* Compile with call hooks. */
#define JIT_F_DEBUG_INS		0x0200	/* Compile with instruction hooks. */
#define JIT_F_DEBUG		0x0f00	/* Union of all debug flags. */

/* Temporary backend flags. */
#define JIT_TF_USED_DEOPT	0x0001	/* Used .deopt segment. */

/* JIT engine status codes for prototypes (grep "ORDER JIT_S"). */
enum {
  JIT_S_OK,		/* OK, code has been compiled. */
  JIT_S_NONE,		/* Nothing compiled yet (default). */

  JIT_S_OFF,		/* Compilation for this prototype disabled. */
  JIT_S_ENGINE_OFF,	/* JIT engine is turned off. */
  JIT_S_DELAYED,	/* Compilation delayed (recursive invocation). */

  JIT_S_TOOLARGE,	/* Bytecode or machine code is too large. */
  JIT_S_COMPILER_ERROR,	/* Error from compiler frontend. */
  JIT_S_DASM_ERROR,	/* Error from DynASM engine. */

  JIT_S_MAX
};

/* Machine code trailer and mcode fragment map. */
typedef struct jit_MCTrailer {
  char *mcode;			/* Pointer to next machine code block. */
  size_t sz;			/* Size of next machine code block. */
} jit_MCTrailer;

typedef unsigned short jit_Mfm;

/* Deliberately return a void * because the trailer is not fully aligned. */
#define JIT_MCTRAILER(mcode, sz) \
  ((void *)(((char *)(mcode))+(sz)-sizeof(jit_MCTrailer)))
#define JIT_MCMFM(mcode, sz) \
  ((jit_Mfm *)(((char *)(mcode))+(sz)-sizeof(jit_MCTrailer)-sizeof(jit_Mfm)))

#define JIT_MFM_MAX	0x7ff0	/* Max. mcode fragment length. */
#define JIT_MFM_MASK	0x7fff	/* Tag mask. */
#define JIT_MFM_MARK	0x8000	/* Deoptimized (main mfm), seek (deopt mfm). */
#define JIT_MFM_COMBINE	0xfffd	/* Combined with prev. instruction(s). */
#define JIT_MFM_DEAD	0xfffe	/* Dead instruction. */
#define JIT_MFM_STOP	0xffff	/* End of map. */

#define jit_mfm_ismain(mfm)		(!(*(mfm) & JIT_MFM_MARK))
#define jit_mfm_isdeoptpc(mfm, pc)	((mfm)[-(pc)] & JIT_MFM_MARK)

/* Deoptimization hints at end of mfm. */
#define JIT_MFM_DEOPT_PAIRS	0xfffc	/* CALL+TFORLOOP inlined (i)pairs. */

/* Preallocation for the hash part of the compiler state table. */
#define COMSTATE_PREALLOC	128

/* Forward declaration for DynASM state. */
struct dasm_State;

/* Frontend wrapper. */
typedef int (*jit_FrontWrap)(lua_State *L, Table *st);

/* Global JIT state. */
typedef struct jit_State {
  /* Permanent backend environment: */
  struct dasm_State *D;	/* DynASM state. Keep this as the first field. */
  void *mcodeheap;	/* Private heap to allocate executable memory from. */
  void **jsub;		/* Addresses of JIT subroutines. */
  void *jsubmcode;	/* Base address of JSUB mcode. */
  size_t szjsubmcode;	/* Size of JSUB mcode. */
  int numjsub;		/* Number of JSUBs. */

  /* Temporary backend environment (valid only while running): */
  lua_State *L;		/* Compiler thread. */
  Table *comstate;	/* Compiler state table. */
  Proto *pt;		/* Currently compiled prototype. */
  const Instruction *nextins;	/* Pointer to next instruction. */
  jit_Mfm *mfm;		/* Position in temporary mcode fragment map. */
  int nextpc;		/* Next PC. */
  int combine;		/* Number of following instructions to combine. */
  unsigned int tflags;	/* Temporary flags. */
  int dasmstatus;	/* DynASM status code. */

  /* JIT engine fields: */
  jit_FrontWrap frontwrap; /* Compiler frontend wrapper. */
  unsigned int flags;	/* Global JIT engine flags. */
} jit_State;


/* --- ljit_core.c */

/* Initialize and free JIT engine state. */
LUAI_FUNC void luaJIT_initstate(lua_State *L);
LUAI_FUNC void luaJIT_freestate(lua_State *L);

/* Compile and run a function. */
LUAI_FUNC int luaJIT_run(lua_State *L, StkId func, int nresults);
/* Deoptimize the current instruction. Return new mcode addr to continue. */
LUAI_FUNC void *luaJIT_deoptimize(lua_State *L);

/* Find relative PC (0 based) for a bytecode pointer or a JIT mcode address. */
LUAI_FUNC int luaJIT_findpc(Proto *pt, const Instruction *savedpc);
/* Find mcode address for PC (1 based). */
LUAI_FUNC void *luaJIT_findmcode(Proto *pt, int pc);


/* --- ljit_backend.c */

/* Arch string. */
LUAI_DATA const char luaJIT_arch[];
/* Initialize and free compiler backend. */
LUAI_FUNC int luaJIT_initbackend(lua_State *L);
LUAI_FUNC void luaJIT_freebackend(lua_State *L);
/* Compiler backend. */
LUAI_FUNC int luaJIT_backend(lua_State *L);
/* Notify backend that the debug mode may have changed. */
LUAI_FUNC void luaJIT_debugnotify(jit_State *J);


/* ---- ljit_mem.c */

/* Free the mcode heap. */
LUAI_FUNC void luaJIT_freemcodeheap(jit_State *J);
/* Free mcode. */
LUAI_FUNC void luaJIT_freemcode(jit_State *J, void *mcode, size_t sz);
/* Free JIT structures in function prototype. */
LUAI_FUNC void luaJIT_freeproto(lua_State *L, Proto *pt);
/* Link generated code. */
LUAI_FUNC int luaJIT_link(jit_State *J, void **mcodep, size_t *szp);


#endif
