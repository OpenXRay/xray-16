/*
** Memory management for machine code.
** Copyright (C) 2005-2012 Mike Pall. See Copyright Notice in luajit.h
*/

#define ljit_mem_c
#define LUA_CORE

#include <string.h>

#include "lua.h"

#include "lmem.h"
#include "ldo.h"
#include "ljit.h"
#include "ljit_dasm.h"


/*
** Define this if you want to run LuaJIT with valgrind. You will get random
** errors if you don't. And these errors are usually not caught by valgrind!
**
** This macro evaluates to a no-op if not run with valgrind. I.e. you can
** use the same binary for regular runs, too (without a performance loss).
*/
#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#define MCH_INVALIDATE(ptr, addr) VALGRIND_DISCARD_TRANSLATIONS(ptr, addr)
#else
#define MCH_INVALIDATE(ptr, addr) ((void)0)
#endif


/* ------------------------------------------------------------------------ */

#if defined(_WIN32) && !defined(LUAJIT_MCH_USE_MALLOC)

/* Use a private heap with executable memory for Windows. */
#include <windows.h>

/* No need for serialization. There's already a lock per Lua universe. */
#ifdef HEAP_CREATE_ENABLE_EXECUTE
#define MCH_HCFLAGS	(HEAP_NO_SERIALIZE|HEAP_CREATE_ENABLE_EXECUTE)
#else
#define MCH_HCFLAGS	(HEAP_NO_SERIALIZE|0x00040000)
#endif

/* Free the whole mcode heap. */
void luaJIT_freemcodeheap(jit_State *J)
{
  if (J->mcodeheap) HeapDestroy((HANDLE)J->mcodeheap);
}

/* Allocate a code block from the mcode heap. */
static void *mcode_alloc(jit_State *J, size_t sz)
{
  void *ptr;
  if (J->mcodeheap == NULL) {
    J->mcodeheap = (void *)HeapCreate(MCH_HCFLAGS, 0, 0);
    if (J->mcodeheap == NULL) luaD_throw(J->L, LUA_ERRMEM);
  }
  ptr = HeapAlloc(J->mcodeheap, 0, (sz));
  if (ptr == NULL) luaD_throw(J->L, LUA_ERRMEM);
  return ptr;
}

#define mcode_free(L, J, p, sz)	HeapFree(J->mcodeheap, 0, (p))

/* ------------------------------------------------------------------------ */

#elif defined(LUA_USE_POSIX) && !defined(LUAJIT_MCH_USE_MALLOC)

/*
** Allocate EXECUTABLE memory with mmap() on POSIX systems.
**
** There is no standard way to reuse malloc(). So this is a very small,
** but also very naive memory allocator. This should be ok, because:
**
** 1. Most apps only allocate mcode while running and free all on exit.
**
** 2. Some apps regularly load/unload a bunch of modules ("stages").
**    Allocs/frees come in groups, so coalescing should work fine.
**
** If your app differs, then please elaborate and/or supply code.
** And no -- including a full blown malloc is NOT an option.
**
** Caveat: the mmap()'ed heaps are not freed until exit.
** This shouldn't be too difficult to add, but I didn't bother.
*/

#include <sys/types.h>
#include <sys/mman.h>

/* TODO: move this to luaconf.h */
#define LUAJIT_MCH_CHUNKSIZE		(1<<17)	 /* 128K */

#if defined(MAP_ANONYMOUS)
#define MCH_MMFLAGS	(MAP_PRIVATE|MAP_ANONYMOUS)
#elif defined(MAP_ANON)
#define MCH_MMFLAGS	(MAP_PRIVATE|MAP_ANON)
#else
/* I'm too lazy to add /dev/zero support for ancient systems. */
#error "Your OS has no (easy) support for anonymous mmap(). Please upgrade."
#endif

/* Chunk header. Used for the free chunk list / heap headers. */
typedef struct MCodeHead {
  struct MCodeHead *next;	/* Next free chunk / 1st head: first free. */
  struct MCodeHead *prev;	/* Prev free chunk / 1st head: next head. */
  size_t size;			/* Size of free chunk / Size of heap. */
  size_t dummy; 		/* May or may not overlap with trailer. */
} MCodeHead;

/* Allocation granularity. Assumes sizeof(void *) >= sizeof(size_t). */
#define MCH_GRANULARITY		(4*sizeof(void *))
#define MCH_ROUNDSIZE(x)	(((x) + MCH_GRANULARITY-1) & -MCH_GRANULARITY)
#define MCH_ROUNDHEAP(x)	(((x) + 4095) & -4096)
#define MCH_HEADERSIZE		MCH_ROUNDSIZE(sizeof(MCodeHead))

/* Trailer flags. */
#define MCH_USED		1	/* Next chunk is in use. */
#define MCH_LAST		2	/* Next chunk is the last one. */
#define MCH_FIRST		4	/* Next chunk is the first one. */
/* Note: the last chunk of each heap doesn't have a trailer. */

/* Trailer macros. */
#define MCH_PREVTRAILER(mh)	((size_t *)(mh) - 1)
#define MCH_TRAILER(mh, sz)	((size_t *)((char *)(mh) + (sz)) - 1)
#define MCH_TRFLAGS(tr)		((tr) & (MCH_USED|MCH_LAST))
#define MCH_TRSIZE(tr)		((tr) & ~(MCH_USED|MCH_LAST))

/* Debugging memory allocators is ... oh well. */
#ifdef MCH_DEBUG
#include <stdio.h>
#define MCH_DBGF	stderr
#define MCH_DBG(x)	fprintf x
#else
#define MCH_DBG(x)	((void)0)
#endif

/* Free the whole list of mcode heaps. */
void luaJIT_freemcodeheap(jit_State *J)
{
  MCodeHead *mh = (MCodeHead *)J->mcodeheap;
  while (mh) {
    MCodeHead *prev = mh->prev;  /* Heaps are in the prev chain. */
#ifdef MCH_DEBUG
    munmap((void *)mh, mh->size+4096);
#else
    munmap((void *)mh, mh->size);
#endif
    mh = prev;
  }
  J->mcodeheap = NULL;
}

/* Allocate a new heap of at least the given size. */
static void mcode_newheap(jit_State *J, size_t sz)
{
  MCodeHead *mh, *mhn, *fh;
  void *ptr;

  /* Ensure minimum size or round up. */
  if (sz + MCH_HEADERSIZE <= LUAJIT_MCH_CHUNKSIZE)
    sz = LUAJIT_MCH_CHUNKSIZE;
  else
    sz = MCH_ROUNDHEAP(sz + MCH_HEADERSIZE);

#ifdef MCH_DEBUG
  /* Allocate a new heap plus a guard page. */
  ptr = mmap(NULL, sz+4096, PROT_READ|PROT_WRITE|PROT_EXEC, MCH_MMFLAGS, -1, 0);
  if (ptr == MAP_FAILED) luaD_throw(J->L, LUA_ERRMEM);
  mprotect((char *)ptr+sz, 4096, PROT_NONE);
#else
  /* Allocate a new heap. */
  ptr = mmap(NULL, sz, PROT_READ|PROT_WRITE|PROT_EXEC, MCH_MMFLAGS, -1, 0);
  if (ptr == MAP_FAILED) luaD_throw(J->L, LUA_ERRMEM);
#endif

  /* Initialize free chunk. */
  fh = (MCodeHead *)((char *)ptr + MCH_HEADERSIZE);
  fh->size = sz - MCH_HEADERSIZE;
  *MCH_PREVTRAILER(fh) = MCH_LAST | MCH_FIRST;  /* Zero size, no coalesce. */

  /* Initialize new heap and make it the first heap. */
  mh = (MCodeHead *)J->mcodeheap;
  J->mcodeheap = ptr;
  mhn = (MCodeHead *)ptr;
  mhn->prev = mh;  /* Heaps are in the prev. chain. */
  mhn->size = sz;
  mhn->next = fh;  /* Start of free list is always in the first heap. */
  fh->prev = mhn;
  if (mh) {
    fh->next = mh->next;  /* Old start of free list. */
    mh->next = NULL; /* Just in case. */
  } else {
    fh->next = NULL;  /* No other free chunks yet. */
  }
  MCH_DBG((MCH_DBGF, "HEAP %p %5x\n", mhn, sz));
}

/* Allocate a code block. */
static void *mcode_alloc(jit_State *J, size_t sz)
{
  sz = MCH_ROUNDSIZE(sz + sizeof(size_t));
  for ( ; ; ) {
    MCodeHead *mh = (MCodeHead *)J->mcodeheap;
    if (mh) {  /* Got at least one heap so search free list. */
#ifdef MCH_DEBUG
      int slen = 0;
      for (mh = mh->next; mh ; mh = mh->next, slen++)
#else
      for (mh = mh->next; mh ; mh = mh->next)
#endif
	if (mh->size >= sz) {  /* Very naive first fit. */
	  size_t *trailer = MCH_TRAILER(mh, sz);
	  size_t *ptrailer = MCH_PREVTRAILER(mh);
	  if (mh->size == sz) {  /* Exact match: just unchain chunk. */
	    mh->prev->next = mh->next;
	    if (mh->next)
	      mh->next->prev = mh->prev;
	    *ptrailer |= MCH_USED;
	    MCH_DBG((MCH_DBGF, "NEW  %p %5x  FIT #%d%s\n",
		     mh, sz, slen, (*ptrailer & MCH_LAST) ? " LAST" : ""));
	  } else {  /* Chunk is larger: rechain remainder chunk. */
	    MCodeHead *fh = (MCodeHead *)((char *)mh + sz);
	    size_t tr;
	    fh->size = mh->size - sz;
	    (fh->prev = mh->prev)->next = fh;
	    if ((fh->next = mh->next) != NULL)
	      fh->next->prev = fh;
	    tr = *ptrailer;
	    if (tr & MCH_LAST) {
	      *ptrailer = (tr & ~MCH_LAST) | MCH_USED;
	      *trailer = sz | MCH_LAST;
	      MCH_DBG((MCH_DBGF, "NEW  %p %5x  REST %p %5x #%d LAST\n",
		       mh, sz, fh, fh->size, slen));
	    } else {
	      size_t *ftrailer = MCH_TRAILER(fh, fh->size);
	      *ftrailer = MCH_TRFLAGS(*ftrailer) | fh->size;
	      *ptrailer = tr | MCH_USED;
	      *trailer = sz;
	      MCH_DBG((MCH_DBGF, "NEW  %p %5x  REST %p %5x #%d\n",
		       mh, sz, fh, fh->size, slen));
	    }
	  }
	  return (void *)mh;
	}
    }
    /* No luck. Allocate a new heap. Next loop iteration will succeed. */
    mcode_newheap(J, sz);
  }
}

/* Free a code block. */
static void mcode_free_(jit_State *J, void *ptr, size_t sz)
{
  MCodeHead *mh = (MCodeHead *)ptr;
  size_t *trailer = MCH_TRAILER(mh, sz);
  size_t *ptrailer = MCH_PREVTRAILER(mh);
  size_t tr = *ptrailer;

#ifdef MCH_DEBUG
  if (!(tr & MCH_USED)) MCH_DBG((MCH_DBGF, "**unused %p %5x\n", ptr, sz));
#endif

  if (!(tr & MCH_FIRST)) {
    MCodeHead *ph = (MCodeHead *)((char *)mh - MCH_TRSIZE(tr));
    size_t *pptrailer = MCH_PREVTRAILER(ph);
    if (!(*pptrailer & MCH_USED)) {  /* Prev free? */
      if (!(tr & MCH_LAST) && !(*trailer & MCH_USED)) {  /* Next free? */
	/* Coalesce with previous and next chunk. */
	MCodeHead *nh = (MCodeHead *)((char *)mh + sz);
	MCH_DBG((MCH_DBGF, "free %p %5x  PN  %p %5x  %p %5x%s\n",
		 mh, sz, ph, ph->size, nh, nh->size,
		 (*trailer & MCH_LAST) ? " last" : ""));
	if ((nh->prev->next = nh->next) != NULL)
	  nh->next->prev = nh->prev;
	ph->size += sz + nh->size;
	if (*trailer & MCH_LAST) {
	  *pptrailer |= MCH_LAST;
	} else {
	  trailer = MCH_TRAILER(nh, nh->size);
	  *trailer = MCH_TRFLAGS(*trailer) | ph->size;
	}
	return;
      }
      MCH_DBG((MCH_DBGF, "free %p %5x  P-  %p %5x%s\n",
	       mh, sz, ph, ph->size,
	       (tr & MCH_LAST) ? " last" : ""));
      ph->size += sz;
      if (tr & MCH_LAST)
	*pptrailer |= MCH_LAST;
      else
	*trailer = MCH_TRFLAGS(*trailer) | ph->size;
      return;
    }
  }

  if (!(tr & MCH_LAST) && !(*trailer & MCH_USED)) {  /* Next free? */
    /* Coalesce with next chunk. */
    MCodeHead *nh = (MCodeHead *)((char *)mh + sz);
    MCH_DBG((MCH_DBGF, "free %p %5x  -N  %p %5x%s\n",
	     mh, sz, nh, nh->size, (*trailer & MCH_LAST) ? " last" : ""));
    (mh->prev = nh->prev)->next = mh;
    if ((mh->next = nh->next))
      mh->next->prev = mh;
    mh->size = nh->size + sz;
    if (*trailer & MCH_LAST) {
      *ptrailer = (tr & ~MCH_USED) | MCH_LAST;
    } else {
      trailer = MCH_TRAILER(mh, mh->size);
      *trailer = MCH_TRFLAGS(*trailer) | mh->size;
      *ptrailer = tr & ~MCH_USED;
    }
  } else {
    /* No coalesce possible, just add to free list. */
    MCodeHead *fh = (MCodeHead *)J->mcodeheap;
    MCH_DBG((MCH_DBGF, "free %p %5x  --%s\n",
	     mh, sz, (tr & MCH_LAST) ? "  last" : ""));
    if ((mh->next = fh->next))
      mh->next->prev = mh;
    fh->next = mh;
    mh->prev = fh;
    mh->size = sz;
    *ptrailer = tr & ~MCH_USED;
  }
}

#define mcode_free(L, J, p, sz)	\
  mcode_free_(J, (p), MCH_ROUNDSIZE((sz) + sizeof(size_t)))

/* ------------------------------------------------------------------------ */

#else

/*
** Fallback to Lua's alloc, i.e. probably malloc().
**
** Note: the returned memory is usually not marked executable!
** Running the code will crash if the CPU/OS checks for this.
** E.g. on x86 CPUs that support the NX (No eXecute) bit.
*/

/* There's no heap to free, but the JSUB mcode is. */
void luaJIT_freemcodeheap(jit_State *J)
{
  if (J->jsubmcode) luaM_freemem(J->L, J->jsubmcode, J->szjsubmcode);
}

#define mcode_alloc(J, sz)	luaM_realloc_(J->L, NULL, 0, (sz))
#define mcode_free(L, J, p, sz)	luaM_freemem(L, p, sz)

#endif

/* ------------------------------------------------------------------------ */

/* Free mcode. */
void luaJIT_freemcode(jit_State *J, void *mcode, size_t sz)
{
  mcode_free(J->L, J, mcode, sz);
}

/* Free JIT structures in function prototype. */
void luaJIT_freeproto(lua_State *L, Proto *pt)
{
  char *mcode = (char *)pt->jit_mcode;
  size_t sz = pt->jit_szmcode;
  pt->jit_mcode = NULL;
  pt->jit_szmcode = 0;
  while (sz != 0) {  /* Free whole chain of mcode blocks for this proto. */
    jit_MCTrailer next;
    memcpy((void *)&next, JIT_MCTRAILER(mcode, sz), sizeof(jit_MCTrailer));
    MCH_INVALIDATE(mcode, sz);
    mcode_free(L, G(L)->jit_state, mcode, sz);
    mcode = next.mcode;
    sz = next.sz;
  }
}

/* Link generated code. Return mcode address, size and status. */
int luaJIT_link(jit_State *J, void **mcodep, size_t *szp)
{
  size_t sz;
  void *mcode;

  /* Pass 2: link sections. */
  if ((J->dasmstatus = dasm_link(Dst, &sz))) return JIT_S_DASM_ERROR;

  /* Check for hardcoded limit on mcode size. */
  if (sz > LUAJIT_LIM_MCODE) return JIT_S_TOOLARGE;

  /* TODO: mark mcode readonly when we're done. */
  mcode = mcode_alloc(J, sz);

  /* Pass 3: encode sections. */
  if ((J->dasmstatus = dasm_encode(Dst, mcode)) != 0) {
    mcode_free(J->L, J, mcode, sz);
    return JIT_S_DASM_ERROR;
  }
  *mcodep = mcode;
  *szp = sz;
  return JIT_S_OK;
}

