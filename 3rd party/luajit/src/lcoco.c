/*
** Copyright (C) 2004-2008 Mike Pall. All rights reserved.
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

/* Coco -- True C coroutines for Lua. http://luajit.org/coco.html */
#ifndef COCO_DISABLE

#define lcoco_c
#define LUA_CORE

#include <lua/lua.h>

#include "lobject.h"
#include "lstate.h"
#include "ldo.h"
#include "lvm.h"
#include "lgc.h"


/*
** Define this if you want to run Coco with valgrind. You will get random
** errors about accessing memory from newly allocated C stacks if you don't.
** You need at least valgrind 3.0 for this to work.
**
** This macro evaluates to a no-op if not run with valgrind. I.e. you can
** use the same binary for regular runs, too (without a performance loss).
*/
#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#define STACK_REG(coco, p, sz)	(coco)->vgid = VALGRIND_STACK_REGISTER(p, p+sz);
#define STACK_DEREG(coco)	VALGRIND_STACK_DEREGISTER((coco)->vgid);
#define STACK_VGID		unsigned int vgid;
#else
#define STACK_REG(coco, p, sz)
#define STACK_DEREG(id)
#define STACK_VGID
#endif

/* ------------------------------------------------------------------------ */

/* Use Windows Fibers. */
#if defined(COCO_USE_FIBERS)

#define _WIN32_WINNT 0x0400
#include <windows.h>

#define COCO_MAIN_DECL		CALLBACK

typedef LPFIBER_START_ROUTINE coco_MainFunc;

#define COCO_NEW(OL, NL, cstacksize, mainfunc) \
  if ((L2COCO(NL)->fib = CreateFiber(cstacksize, mainfunc, NL)) == NULL) \
    luaD_throw(OL, LUA_ERRMEM);

#define COCO_FREE(L) \
  DeleteFiber(L2COCO(L)->fib); \
  L2COCO(L)->fib = NULL;

/* See: http://blogs.msdn.com/oldnewthing/archive/2004/12/31/344799.aspx */
#define COCO_JUMPIN(coco) \
  { void *cur = GetCurrentFiber(); \
    coco->back = (cur == NULL || cur == (void *)0x1e00) ? \
      ConvertThreadToFiber(NULL) : cur; } \
  SwitchToFiber(coco->fib);

#define COCO_JUMPOUT(coco) \
  SwitchToFiber(coco->back);

/* CreateFiber() defaults to STACKSIZE from the Windows module .def file. */
#define COCO_DEFAULT_CSTACKSIZE		0

/* ------------------------------------------------------------------------ */

#else /* !COCO_USE_FIBERS */

#ifndef COCO_USE_UCONTEXT

/* Try inline asm first. */
#if __GNUC__ >= 3 && !defined(COCO_USE_SETJMP)

#if defined(__i386) || defined(__i386__)

#ifdef __PIC__
typedef void *coco_ctx[4];  /* eip, esp, ebp, ebx */
static inline void coco_switch(coco_ctx from, coco_ctx to)
{
  __asm__ __volatile__ (
    "call 1f\n" "1:\tpopl %%eax\n\t" "addl $(2f-1b),%%eax\n\t"
    "movl %%eax, (%0)\n\t" "movl %%esp, 4(%0)\n\t"
    "movl %%ebp, 8(%0)\n\t" "movl %%ebx, 12(%0)\n\t"
    "movl 12(%1), %%ebx\n\t" "movl 8(%1), %%ebp\n\t"
    "movl 4(%1), %%esp\n\t" "jmp *(%1)\n" "2:\n"
    : "+S" (from), "+D" (to) : : "eax", "ecx", "edx", "memory", "cc");
}
#else
typedef void *coco_ctx[3];  /* eip, esp, ebp */
static inline void coco_switch(coco_ctx from, coco_ctx to)
{
  __asm__ __volatile__ (
    "movl $1f, (%0)\n\t" "movl %%esp, 4(%0)\n\t" "movl %%ebp, 8(%0)\n\t"
    "movl 8(%1), %%ebp\n\t" "movl 4(%1), %%esp\n\t" "jmp *(%1)\n" "1:\n"
    : "+S" (from), "+D" (to) : : "eax", "ebx", "ecx", "edx", "memory", "cc");
}
#endif

#define COCO_CTX		coco_ctx
#define COCO_SWITCH(from, to)	coco_switch(from, to);
#define COCO_MAKECTX(coco, buf, func, stack, a0) \
  buf[0] = (void *)(func); \
  buf[1] = (void *)(stack); \
  buf[2] = (void *)0; \
  stack[0] = 0xdeadc0c0;  /* Dummy return address. */ \
  coco->arg0 = (size_t)(a0);
#define COCO_STATE_HEAD		size_t arg0;

#elif __mips && _MIPS_SIM == _MIPS_SIM_ABI32 && !defined(__mips_eabi)

/* No way to avoid the function prologue with inline assembler. So use this: */
static const unsigned int coco_switch[] = {
#ifdef __mips_soft_float
#define COCO_STACKSAVE		-10
  0x27bdffd8,  /* addiu sp, sp, -(10*4) */
#else
#define COCO_STACKSAVE		-22
  0x27bdffa8,  /* addiu sp, sp, -(10*4+6*8) */
  /* sdc1 {$f20-$f30}, offset(sp) */
  0xf7be0050, 0xf7bc0048, 0xf7ba0040, 0xf7b80038, 0xf7b60030, 0xf7b40028,
#endif
  /* sw {gp,s0-s8}, offset(sp) */
  0xafbe0024, 0xafb70020, 0xafb6001c, 0xafb50018, 0xafb40014, 0xafb30010,
  0xafb2000c, 0xafb10008, 0xafb00004, 0xafbc0000,
  /* sw sp, 4(a0); sw ra, 0(a0); lw ra, 0(a1); lw sp, 4(a1); move t9, ra */
  0xac9d0004, 0xac9f0000, 0x8cbf0000, 0x8cbd0004, 0x03e0c821,
  /* lw caller-saved-reg, offset(sp) */
  0x8fbe0024, 0x8fb70020, 0x8fb6001c, 0x8fb50018, 0x8fb40014, 0x8fb30010,
  0x8fb2000c, 0x8fb10008, 0x8fb00004, 0x8fbc0000,
#ifdef __mips_soft_float
  0x03e00008, 0x27bd0028  /* jr ra; addiu sp, sp, 10*4 */
#else
  /* ldc1 {$f20-$f30}, offset(sp) */
  0xd7be0050, 0xd7bc0048, 0xd7ba0040, 0xd7b80038, 0xd7b60030, 0xd7b40028,
  0x03e00008, 0x27bd0058  /* jr ra; addiu sp, sp, 10*4+6*8 */
#endif
};

typedef void *coco_ctx[2];  /* ra, sp */
#define COCO_CTX		coco_ctx
#define COCO_SWITCH(from, to) \
  ((void (*)(coco_ctx, coco_ctx))coco_switch)(from, to);
#define COCO_MAKECTX(coco, buf, func, stack, a0) \
  buf[0] = (void *)(func); \
  buf[1] = (void *)&stack[COCO_STACKSAVE]; \
  stack[4] = (size_t)(a0);  /* Assumes o32 ABI. */
#define COCO_STACKADJUST	8
#define COCO_MAIN_PARAM		int _a, int _b, int _c, int _d, lua_State *L

#endif /* arch check */

#endif /* !(__GNUC__ >= 3 && !defined(COCO_USE_SETJMP)) */

/* Try _setjmp/_longjmp with a patched jump buffer. */
#ifndef COCO_MAKECTX
#include <setjmp.h>

/* Check for supported CPU+OS combinations. */
#if defined(__i386) || defined(__i386__)

#define COCO_STATE_HEAD		size_t arg0;
#define COCO_SETJMP_X86(coco, stack, a0) \
  stack[COCO_STACKADJUST-1] = 0xdeadc0c0;  /* Dummy return address. */ \
  coco->arg0 = (size_t)(a0);

#if __GLIBC__ == 2 && defined(JB_SP)		/* x86-linux-glibc2 */
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf->__jmpbuf[JB_PC] = (int)(func); \
  buf->__jmpbuf[JB_SP] = (int)(stack); \
  buf->__jmpbuf[JB_BP] = 0; \
  COCO_SETJMP_X86(coco, stack, a0)
#elif defined(__linux__) && defined(_I386_JMP_BUF_H)	/* x86-linux-libc5 */
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf->__pc = (func); \
  buf->__sp = (stack); \
  buf->__bp = NULL; \
  COCO_SETJMP_X86(coco, stack, a0)
#elif defined(__FreeBSD__)			/* x86-FreeBSD */
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf->_jb[0] = (long)(func); \
  buf->_jb[2] = (long)(stack); \
  buf->_jb[3] = 0; /* ebp */ \
  COCO_SETJMP_X86(coco, stack, a0)
#define COCO_STACKADJUST	2
#elif defined(__NetBSD__) || defined(__OpenBSD__) /* x86-NetBSD, x86-OpenBSD */
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf[0] = (long)(func); \
  buf[2] = (long)(stack); \
  buf[3] = 0; /* ebp */ \
  COCO_SETJMP_X86(coco, stack, a0)
#define COCO_STACKADJUST	2
#elif defined(__solaris__) && _JBLEN == 10	/* x86-solaris */
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf[5] = (int)(func); \
  buf[4] = (int)(stack); \
  buf[3] = 0; \
  COCO_SETJMP_X86(coco, stack, a0)
#elif defined(__MACH__) && defined(_BSD_I386_SETJMP_H)	/* x86-macosx */
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf[12] = (int)(func); \
  buf[9] = (int)(stack); \
  buf[8] = 0; /* ebp */ \
  COCO_SETJMP_X86(coco, stack, a0)
#endif

#elif defined(__x86_64__) || defined(__x86_64)

#define COCO_STATE_HEAD		size_t arg0;

#define COCO_MAIN_PARAM \
  int _a, int _b, int _c, int _d, int _e, int _f, lua_State *L

#if __GLIBC__ == 2 && defined(JB_RSP)		/* x64-linux-glibc2 */
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf->__jmpbuf[JB_PC] = (long)(func); \
  buf->__jmpbuf[JB_RSP] = (long)(stack); \
  buf->__jmpbuf[JB_RBP] = 0; \
  stack[0] = 0xdeadc0c0;  /* Dummy return address. */ \
  coco->arg0 = (size_t)(a0);
#elif defined(__solaris__) && _JBLEN == 8		/* x64-solaris */
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf[7] = (long)(func); \
  buf[6] = (long)(stack); \
  buf[5] = 0; \
  stack[0] = 0xdeadc0c0;  /* Dummy return address. */ \
  coco->arg0 = (size_t)(a0);
#endif

#elif defined(PPC) || defined(__ppc__) || defined(__PPC__) || \
      defined(__powerpc__) || defined(__POWERPC__) || defined(_ARCH_PPC)

#define COCO_STACKADJUST	16
#define COCO_MAIN_PARAM \
  int _a, int _b, int _c, int _d, int _e, int _f, int _g, int _h, lua_State *L

#if defined(__MACH__) && defined(_BSD_PPC_SETJMP_H_)	/* ppc32-macosx */
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf[21] = (int)(func); \
  buf[0] = (int)(stack); \
  stack[6+8] = (size_t)(a0);
#endif

#elif (defined(MIPS) || defined(MIPSEL) || defined(__mips)) && \
  _MIPS_SIM == _MIPS_SIM_ABI32 && !defined(__mips_eabi)

/* Stack layout for o32 ABI. */
#define COCO_STACKADJUST	8
#define COCO_MAIN_PARAM		int _a, int _b, int _c, int _d, lua_State *L

#if __GLIBC__ == 2 || defined(__UCLIBC__)	/* mips32-linux-glibc2 */
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf->__jmpbuf->__pc = (func); /* = t9 in _longjmp. Reqd. for -mabicalls. */ \
  buf->__jmpbuf->__sp = (stack); \
  buf->__jmpbuf->__fp = (void *)0; \
  stack[4] = (size_t)(a0);
#endif

#elif defined(__arm__) || defined(__ARM__)

#if __GLIBC__ == 2 || defined(__UCLIBC__)	/* arm-linux-glibc2 */
#ifndef __JMP_BUF_SP
#define __JMP_BUF_SP	((sizeof(__jmp_buf)/sizeof(int))-2)
#endif
#define COCO_PATCHCTX(coco, buf, func, stack, a0) \
  buf->__jmpbuf[__JMP_BUF_SP+1] = (int)(func); /* pc */ \
  buf->__jmpbuf[__JMP_BUF_SP] = (int)(stack); /* sp */ \
  buf->__jmpbuf[__JMP_BUF_SP-1] = 0; /* fp */ \
  stack[0] = (size_t)(a0);
#define COCO_STACKADJUST	2
#define COCO_MAIN_PARAM		int _a, int _b, int _c, int _d, lua_State *L
#endif

#endif /* arch check */

#ifdef COCO_PATCHCTX
#define COCO_CTX		jmp_buf
#define COCO_MAKECTX(coco, buf, func, stack, a0) \
  _setjmp(buf); COCO_PATCHCTX(coco, buf, func, stack, a0)
#define COCO_SWITCH(from, to)	if (!_setjmp(from)) _longjmp(to, 1);
#endif

#endif /* !defined(COCO_MAKECTX) */

#endif /* !defined(COCO_USE_UCONTEXT) */

/* ------------------------------------------------------------------------ */

/* Use inline asm or _setjmp/_longjmp if available. */
#ifdef COCO_MAKECTX

#ifndef COCO_STACKADJUST
#define COCO_STACKADJUST	1
#endif

#define COCO_FILL(coco, NL, mainfunc) \
{ /* Include the return address to get proper stack alignment. */ \
  size_t *stackptr = &((size_t *)coco)[-COCO_STACKADJUST]; \
  COCO_MAKECTX(coco, coco->ctx, mainfunc, stackptr, NL) \
}

/* ------------------------------------------------------------------------ */

/* Else fallback to ucontext. Slower, because it saves/restores signals. */
#else /* !defined(COCO_MAKECTX) */

#include <ucontext.h>

#define COCO_CTX		ucontext_t

/* Ugly workaround for makecontext() deficiencies on 64 bit CPUs. */
/* Note that WIN64 (which is LLP64) never comes here. See above. */
#if defined(__LP64__) || defined(_LP64) || INT_MAX != LONG_MAX
/* 64 bit CPU: split the pointer into two 32 bit ints. */
#define COCO_MAIN_PARAM		unsigned int lo, unsigned int hi
#define COCO_MAIN_GETL \
  lua_State *L = (lua_State *)((((unsigned long)hi)<<32)+(unsigned long)lo);
#define COCO_MAKECTX(coco, NL, mainfunc) \
  makecontext(&coco->ctx, mainfunc, 2, \
    (int)(ptrdiff_t)NL, (int)((ptrdiff_t)NL>>32));
#else
/* 32 bit CPU: a pointer fits into an int. */
#define COCO_MAKECTX(coco, NL, mainfunc) \
  makecontext(&coco->ctx, mainfunc, 1, (int)NL);
#endif

#define COCO_FILL(coco, NL, mainfunc) \
  getcontext(&coco->ctx); \
  coco->ctx.uc_link = NULL;  /* We never exit from coco_main. */ \
  coco->ctx.uc_stack.ss_sp = coco->allocptr; \
  coco->ctx.uc_stack.ss_size = (char *)coco - (char *)(coco->allocptr); \
  COCO_MAKECTX(coco, NL, mainfunc)

#define COCO_SWITCH(from, to)	swapcontext(&(from), &(to));

#endif /* !defined(COCO_MAKECTX) */


/* Common code for inline asm/setjmp/ucontext to allocate/free the stack. */

struct coco_State {
#ifdef COCO_STATE_HEAD
  COCO_STATE_HEAD
#endif
  COCO_CTX ctx;			/* Own context. */
  COCO_CTX back;		/* Context to switch back to. */
  void *allocptr;		/* Pointer to allocated memory. */
  int allocsize;		/* Size of allocated memory. */
  int nargs;			/* Number of arguments to pass. */
  STACK_VGID			/* Optional valgrind stack id. See above. */
};

typedef void (*coco_MainFunc)(void);

/* Put the Coco state at the end and align it downwards. */
#define ALIGNED_END(p, s, t) \
  ((t *)(((char *)0) + ((((char *)(p)-(char *)0)+(s)-sizeof(t)) & -16)))

/* TODO: use mmap. */
#define COCO_NEW(OL, NL, cstacksize, mainfunc) \
{ \
  void *ptr = luaM_malloc(OL, cstacksize); \
  coco_State *coco = ALIGNED_END(ptr, cstacksize, coco_State); \
  STACK_REG(coco, ptr, cstacksize) \
  coco->allocptr = ptr; \
  coco->allocsize = cstacksize; \
  COCO_FILL(coco, NL, mainfunc) \
  L2COCO(NL) = coco; \
}

#define COCO_FREE(L) \
  STACK_DEREG(L2COCO(L)) \
  luaM_freemem(L, L2COCO(L)->allocptr, L2COCO(L)->allocsize); \
  L2COCO(L) = NULL;

#define COCO_JUMPIN(coco)	COCO_SWITCH(coco->back, coco->ctx)
#define COCO_JUMPOUT(coco)	COCO_SWITCH(coco->ctx, coco->back)

#endif /* !COCO_USE_FIBERS */

/* ------------------------------------------------------------------------ */

#ifndef COCO_MIN_CSTACKSIZE
#define COCO_MIN_CSTACKSIZE		(32768+4096)
#endif

/* Don't use multiples of 64K to avoid D-cache aliasing conflicts. */
#ifndef COCO_DEFAULT_CSTACKSIZE
#define COCO_DEFAULT_CSTACKSIZE		(65536-4096)
#endif

static int defaultcstacksize = COCO_DEFAULT_CSTACKSIZE;

/* Start the Lua or C function. */
static void coco_start(lua_State *L, void *ud)
{
  if (luaD_precall(L, (StkId)ud, LUA_MULTRET) == PCRLUA)
    luaV_execute(L, L->ci - L->base_ci);
}

#ifndef COCO_MAIN_PARAM
#define COCO_MAIN_PARAM		lua_State *L
#endif

#ifndef COCO_MAIN_DECL
#define COCO_MAIN_DECL
#endif

/* Toplevel function for the new coroutine stack. Never exits. */
static void COCO_MAIN_DECL coco_main(COCO_MAIN_PARAM)
{
#ifdef COCO_MAIN_GETL
  COCO_MAIN_GETL
#endif
  coco_State *coco = L2COCO(L);
  for (;;) {
    L->status = (lu_byte)luaD_rawrunprotected(L, coco_start, L->top - coco->nargs+1);
    if (L->status != 0) luaD_seterrorobj(L, L->status, L->top);
    COCO_JUMPOUT(coco)
  }
}

/* Add a C stack to a coroutine. */
lua_State *lua_newcthread(lua_State *OL, int cstacksize)
{
  lua_State *NL = lua_newthread(OL);

  if (cstacksize < 0)
    return NL;
  if (cstacksize == 0)
    cstacksize = defaultcstacksize;
  else if (cstacksize < COCO_MIN_CSTACKSIZE)
    cstacksize = COCO_MIN_CSTACKSIZE;
  cstacksize &= -16;

  COCO_NEW(OL, NL, cstacksize, ((coco_MainFunc)(coco_main)))

  return NL;
}

/* Free the C stack of a coroutine. Called from lstate.c. */
void luaCOCO_free(lua_State *L)
{
  COCO_FREE(L)
}

/* Resume a coroutine with a C stack. Called from ldo.c. */
int luaCOCO_resume(lua_State *L, int nargs)
{
  coco_State *coco = L2COCO(L);
  coco->nargs = nargs;
  COCO_JUMPIN(coco)
#ifndef COCO_DISABLE_EARLY_FREE
  if (L->status != LUA_YIELD) {
    COCO_FREE(L)
  }
#endif
  return L->status;
}

/* Yield from a coroutine with a C stack. Called from ldo.c. */
int luaCOCO_yield(lua_State *L)
{
  coco_State *coco = L2COCO(L);
  L->status = LUA_YIELD;
  COCO_JUMPOUT(coco)
  L->status = 0;
  {
    StkId base = L->top - coco->nargs;
    StkId rbase = L->base;
    if (rbase < base) {  /* Need to move args down? */
      while (base < L->top)
	setobjs2s(L, rbase++, base++);
      L->top = rbase;
    }
  }
  L->base = L->ci->base;  /* Restore invariant. */
  return coco->nargs;
}

/* Get/set the default C stack size. */
int luaCOCO_cstacksize(int cstacksize)
{
  int oldsz = defaultcstacksize;
  if (cstacksize >= 0) {
    if (cstacksize == 0)
      cstacksize = COCO_DEFAULT_CSTACKSIZE;
    else if (cstacksize < COCO_MIN_CSTACKSIZE)
      cstacksize = COCO_MIN_CSTACKSIZE;
    defaultcstacksize = cstacksize;
  }
  return oldsz;
}

#endif
