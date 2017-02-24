/* 
  -------------------------------------------------------------- 

  Emulation of UNIX memory routines for win32. 
  Donated by J. Walter <Walter@GeNeSys-e.de> and improved by Niall Douglas
  For additional information about this code, and malloc on Win32, see 
     http://www.genesys-e.de/jwalter/ or
	 http://www.nedprod.com/programs/Win32/ptmalloc2/index.html
*/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x600
#ifndef _XBOX
#	include <windows.h>
#else // #ifndef _XBOX
#	include <xtl.h>
#endif // #ifndef _XBOX

#include <assert.h>

/* Win32 doesn't supply or need the following headers */
#define LACKS_UNISTD_H
#define LACKS_SYS_PARAM_H
#define LACKS_SYS_MMAN_H
#define LACKS_STDINT_H

/* Use the supplied emulation of sbrk */
#define MORECORE sbrk
#define MORECORE_CONTIGUOUS 1
#define MORECORE_FAILURE    ((void*)(-1))

/* Use the supplied emulation of mmap and munmap */
#define HAVE_MMAP 1
#define MUNMAP_FAILURE  (-1)
#define MMAP_CLEARS 1

/* For the windows mmap emulation */
#define MAP_FIXED 0
#define MAP_PRIVATE 1
#define MAP_ANONYMOUS 2
#define MAP_NORESERVE 4
#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2

/* Supply definitions of things glibc would normally define for malloc.c beforehand */
#define __const const

#ifndef HEAP_MAX_SIZE
#define HEAP_MAX_SIZE (1024*1024) /* must be a power of two */
#endif

/* Define INTERNAL_SIZE_T specially */
#ifdef _WIN64
 #define INTERNAL_SIZE_T unsigned __int64
 #define INTERNAL_INTPTR_T __int64
#else
 #define INTERNAL_SIZE_T unsigned __int32
 #define INTERNAL_INTPTR_T __int32
#endif


#ifdef _DEBUG
/* #define TRACE */
#endif

#ifdef __cplusplus
#define INLINE inline
#else
#define INLINE
#endif

// Use inline assembly where possible
#if defined(_M_IX86)
static INLINE LONG interlockedexchange(LONG volatile *data, LONG value)
{
	LONG myret;
	__asm
	{
		mov ecx, [data]
		mov eax, [value]
		pause
		xchg eax, [ecx]
		mov [myret], eax
	}
	return myret;
}
#else
#if defined(_MSC_VER) && _MSC_VER>=1310
 // Use MSVC intrinsics
 #pragma intrinsic (_InterlockedExchange)
 #define interlockedexchange _InterlockedExchange
#else
 #define interlockedexchange InterlockedExchange
#endif
#endif

/* Wait for spin lock */
static INLINE LONG slwait (volatile LONG *sl) {
    while (interlockedexchange (sl, 1) != 0) 
#ifndef FX_SMPBUILD		// Faster not to sleep on multiprocessor machines
	    Sleep (0)
#endif
		;
    return 0;
}

/* Try waiting for spin lock */
static INLINE LONG sltrywait (volatile LONG *sl) {
    return (interlockedexchange (sl, 1) != 0);
}

/* Release spin lock */
static INLINE LONG slrelease (volatile LONG *sl) {
    interlockedexchange (sl, 0);
    return 0;
}

/* Spin lock for emulation code */
static volatile LONG g_sl;

/* getpagesize for windows */
static INTERNAL_INTPTR_T getpagesize (void) {
    static INTERNAL_INTPTR_T g_pagesize = 0;
    if (! g_pagesize) {
#ifdef WIN32
        SYSTEM_INFO system_info;
        GetSystemInfo (&system_info);
        g_pagesize = system_info.dwPageSize;
#elif defined(_XBOX) // #ifdef WIN32
        g_pagesize = 4096;
#else // #elif defined(_XBOX) 
#	error please define your platform
#endif // #elif defined(_XBOX) 
    }
    return g_pagesize;
}
static INTERNAL_INTPTR_T getregionsize (void) {
    static INTERNAL_INTPTR_T g_regionsize = 0;
    if (! g_regionsize) {
#ifdef WIN32
        SYSTEM_INFO system_info;
        GetSystemInfo (&system_info);
        g_regionsize = system_info.dwAllocationGranularity;
#elif defined(_XBOX) // #ifdef WIN32
        g_regionsize = 64*1024;
#else // #elif defined(_XBOX) 
#	error please define your platform
#endif // #elif defined(_XBOX) 
    }
    return g_regionsize;
}

#if 0
/* A region list entry */
typedef struct _region_list_entry {
    void *top_allocated;
    void *top_committed;
    void *top_reserved;
    INTERNAL_INTPTR_T reserve_size;
    struct _region_list_entry *previous;
} region_list_entry;

/* Allocate and link a region entry in the region list */
static int region_list_append (region_list_entry **last, void *base_reserved, INTERNAL_INTPTR_T reserve_size) {
    region_list_entry *next = (region_list_entry *) HeapAlloc (GetProcessHeap (), 0, sizeof (region_list_entry));
    if (! next)
        return FALSE;
    next->top_allocated = (char *) base_reserved;
    next->top_committed = (char *) base_reserved;
    next->top_reserved = (char *) base_reserved + reserve_size;
    next->reserve_size = reserve_size;
    next->previous = *last;
    *last = next;
    return TRUE;
}
/* Free and unlink the last region entry from the region list */
static int region_list_remove (region_list_entry **last) {
    region_list_entry *previous = (*last)->previous;
    if (! HeapFree (GetProcessHeap (), sizeof (region_list_entry), *last))
        return FALSE;
    *last = previous;
    return TRUE;
}

#define CEIL(size,to)	(((size)+(to)-1)&~((to)-1))
#define FLOOR(size,to)	((size)&~((to)-1))

#define SBRK_SCALE  0
/* #define SBRK_SCALE  1 */
/* #define SBRK_SCALE  2 */
/* #define SBRK_SCALE  4  */

/* sbrk for windows */
static void *sbrk (INTERNAL_INTPTR_T size) {
    static INTERNAL_INTPTR_T g_pagesize, g_my_pagesize;
    static INTERNAL_INTPTR_T g_regionsize, g_my_regionsize;
    static region_list_entry *g_last;
    void *result = (void *) MORECORE_FAILURE;
#ifdef TRACE
    printf ("sbrk %d\n", size);
#endif
    /* Wait for spin lock */
    slwait (&g_sl);
   /* First time initialization */
    if (! g_pagesize) {
        g_pagesize = getpagesize ();
        g_my_pagesize = g_pagesize << SBRK_SCALE;
    }
    if (! g_regionsize) {
        g_regionsize = getregionsize ();
        g_my_regionsize = g_regionsize << SBRK_SCALE;
    }
    if (! g_last) {
        if (! region_list_append (&g_last, 0, 0)) 
           goto sbrk_exit;
    }
    /* Assert invariants */
    assert (g_last);
    assert ((char *) g_last->top_reserved - g_last->reserve_size <= (char *) g_last->top_allocated &&
            g_last->top_allocated <= g_last->top_committed);
    assert ((char *) g_last->top_reserved - g_last->reserve_size <= (char *) g_last->top_committed &&
            g_last->top_committed <= g_last->top_reserved &&
            (unsigned) g_last->top_committed % g_pagesize == 0);
    assert ((unsigned) g_last->top_reserved % g_regionsize == 0);
    assert ((unsigned) g_last->reserve_size % g_regionsize == 0);
    /* Allocation requested? */
    if (size >= 0) {
        /* Allocation size is the requested size */
        INTERNAL_INTPTR_T allocate_size = size;
        /* Compute the size to commit */
        INTERNAL_INTPTR_T to_commit = (char *) g_last->top_allocated + allocate_size - (char *) g_last->top_committed;
        /* Do we reach the commit limit? */
        if (to_commit > 0) {
            /* Round size to commit */
            INTERNAL_INTPTR_T commit_size = CEIL (to_commit, g_my_pagesize);
            /* Compute the size to reserve */
            INTERNAL_INTPTR_T to_reserve = (char *) g_last->top_committed + commit_size - (char *) g_last->top_reserved;
            /* Do we reach the reserve limit? */
            if (to_reserve > 0) {
                /* Compute the remaining size to commit in the current region */
                INTERNAL_INTPTR_T remaining_commit_size = (char *) g_last->top_reserved - (char *) g_last->top_committed;
                if (remaining_commit_size > 0) {
                    /* Assert preconditions */
                    assert ((unsigned) g_last->top_committed % g_pagesize == 0);
                    assert (0 < remaining_commit_size && remaining_commit_size % g_pagesize == 0); {
                        /* Commit this */
                        void *base_committed = VirtualAlloc (g_last->top_committed, remaining_commit_size,
							                                 MEM_COMMIT, PAGE_READWRITE);
                        /* Check returned pointer for consistency */
                        if (base_committed != g_last->top_committed)
                            goto sbrk_exit;
                        /* Assert postconditions */
                        assert ((unsigned) base_committed % g_pagesize == 0);
#ifdef TRACE
                        printf ("Commit %p %d\n", base_committed, remaining_commit_size);
#endif
                        /* Adjust the regions commit top */
                        g_last->top_committed = (char *) base_committed + remaining_commit_size;
                    }
                } {
                    /* Now we are going to search and reserve. */
                    int contiguous = -1;
                    int found = FALSE;
                    MEMORY_BASIC_INFORMATION memory_info;
                    void *base_reserved;
                    INTERNAL_INTPTR_T reserve_size;
                    do {
                        /* Assume contiguous memory */
                        contiguous = TRUE;
                        /* Round size to reserve */
                        reserve_size = CEIL (to_reserve, g_my_regionsize);
                        /* Start with the current region's top */
                        memory_info.BaseAddress = g_last->top_reserved;
                        /* Assert preconditions */
                        assert ((unsigned) memory_info.BaseAddress % g_pagesize == 0);
                        assert (0 < reserve_size && reserve_size % g_regionsize == 0);
                        while (VirtualQuery (memory_info.BaseAddress, &memory_info, sizeof (memory_info))) {
                            /* Assert postconditions */
                            assert ((unsigned) memory_info.BaseAddress % g_pagesize == 0);
#ifdef TRACE
                            printf ("Query %p %d %s\n", memory_info.BaseAddress, memory_info.RegionSize, 
                                    memory_info.State == MEM_FREE ? "FREE": 
                                    (memory_info.State == MEM_RESERVE ? "RESERVED":
                                     (memory_info.State == MEM_COMMIT ? "COMMITTED": "?")));
#endif
                            /* Region is free, well aligned and big enough: we are done */
                            if (memory_info.State == MEM_FREE &&
                                (unsigned) memory_info.BaseAddress % g_regionsize == 0 &&
                                memory_info.RegionSize >= (unsigned) reserve_size) {
                                found = TRUE;
                                break;
                            }
                            /* From now on we can't get contiguous memory! */
                            contiguous = FALSE;
                            /* Recompute size to reserve */
                            reserve_size = CEIL (allocate_size, g_my_regionsize);
                            memory_info.BaseAddress = (char *) memory_info.BaseAddress + memory_info.RegionSize;
                            /* Assert preconditions */
                            assert ((unsigned) memory_info.BaseAddress % g_pagesize == 0);
                            assert (0 < reserve_size && reserve_size % g_regionsize == 0);
                        }
                        /* Search failed? */
                        if (! found) 
                            goto sbrk_exit;
                        /* Assert preconditions */
                        assert ((unsigned) memory_info.BaseAddress % g_regionsize == 0);
                        assert (0 < reserve_size && reserve_size % g_regionsize == 0);
                        /* Try to reserve this */
                        base_reserved = VirtualAlloc (memory_info.BaseAddress, reserve_size, 
					                                  MEM_RESERVE, PAGE_NOACCESS);
                        if (! base_reserved) {
                            int rc = GetLastError ();
                            if (rc != ERROR_INVALID_ADDRESS) 
                                goto sbrk_exit;
                        }
                        /* A null pointer signals (hopefully) a race condition with another thread. */
                        /* In this case, we try again. */
                    } while (! base_reserved);
                    /* Check returned pointer for consistency */
                    if (memory_info.BaseAddress && base_reserved != memory_info.BaseAddress)
                        goto sbrk_exit;
                    /* Assert postconditions */
                    assert ((unsigned) base_reserved % g_regionsize == 0);
#ifdef TRACE
                    printf ("Reserve %p %d\n", base_reserved, reserve_size);
#endif
                    /* Did we get contiguous memory? */
                    if (contiguous) {
                        INTERNAL_INTPTR_T start_size = (char *) g_last->top_committed - (char *) g_last->top_allocated;
                        /* Adjust allocation size */
                        allocate_size -= start_size;
                        /* Adjust the regions allocation top */
                        g_last->top_allocated = g_last->top_committed;
                        /* Recompute the size to commit */
                        to_commit = (char *) g_last->top_allocated + allocate_size - (char *) g_last->top_committed;
                        /* Round size to commit */
                        commit_size = CEIL (to_commit, g_my_pagesize);
                    } 
                    /* Append the new region to the list */
                    if (! region_list_append (&g_last, base_reserved, reserve_size))
                        goto sbrk_exit;
                    /* Didn't we get contiguous memory? */
                    if (! contiguous) {
                        /* Recompute the size to commit */
                        to_commit = (char *) g_last->top_allocated + allocate_size - (char *) g_last->top_committed;
                        /* Round size to commit */
                        commit_size = CEIL (to_commit, g_my_pagesize);
                    }
                }
            } 
            /* Assert preconditions */
            assert ((unsigned) g_last->top_committed % g_pagesize == 0);
            assert (0 < commit_size && commit_size % g_pagesize == 0); {
                /* Commit this */
                void *base_committed = VirtualAlloc (g_last->top_committed, commit_size, 
				    			                     MEM_COMMIT, PAGE_READWRITE);
                /* Check returned pointer for consistency */
                if (base_committed != g_last->top_committed)
                    goto sbrk_exit;
                /* Assert postconditions */
                assert ((unsigned) base_committed % g_pagesize == 0);
#ifdef TRACE
                printf ("Commit %p %d\n", base_committed, commit_size);
#endif
                /* Adjust the regions commit top */
                g_last->top_committed = (char *) base_committed + commit_size;
            }
        } 
        /* Adjust the regions allocation top */
        g_last->top_allocated = (char *) g_last->top_allocated + allocate_size;
        result = (char *) g_last->top_allocated - size;
    /* Deallocation requested? */
    } else if (size < 0) {
        INTERNAL_INTPTR_T deallocate_size = - size;
        /* As INTERNAL_INTPTR_T as we have a region to release */
        while ((char *) g_last->top_allocated - deallocate_size < (char *) g_last->top_reserved - g_last->reserve_size) {
            /* Get the size to release */
            INTERNAL_INTPTR_T release_size = g_last->reserve_size;
            /* Get the base address */
            void *base_reserved = (char *) g_last->top_reserved - release_size;
            /* Assert preconditions */
            assert ((unsigned) base_reserved % g_regionsize == 0); 
            assert (0 < release_size && release_size % g_regionsize == 0); {
                /* Release this */
                int rc = VirtualFree (base_reserved, 0, 
                                      MEM_RELEASE);
                /* Check returned code for consistency */
                if (! rc)
                    goto sbrk_exit;
#ifdef TRACE
                printf ("Release %p %d\n", base_reserved, release_size);
#endif
            }
            /* Adjust deallocation size */
            deallocate_size -= (char *) g_last->top_allocated - (char *) base_reserved;
            /* Remove the old region from the list */
            if (! region_list_remove (&g_last))
                goto sbrk_exit;
        } {
            /* Compute the size to decommit */
            INTERNAL_INTPTR_T to_decommit = (char *) g_last->top_committed - ((char *) g_last->top_allocated - deallocate_size);
            if (to_decommit >= g_my_pagesize) {
                /* Compute the size to decommit */
                INTERNAL_INTPTR_T decommit_size = FLOOR (to_decommit, g_my_pagesize);
                /*  Compute the base address */
                void *base_committed = (char *) g_last->top_committed - decommit_size;
                /* Assert preconditions */
                assert ((unsigned) base_committed % g_pagesize == 0);
                assert (0 < decommit_size && decommit_size % g_pagesize == 0); {
                    /* Decommit this */
                    int rc = VirtualFree ((char *) base_committed, decommit_size, 
                                          MEM_DECOMMIT);
                    /* Check returned code for consistency */
                    if (! rc)
                        goto sbrk_exit;
#ifdef TRACE
                    printf ("Decommit %p %d\n", base_committed, decommit_size);
#endif
                }
                /* Adjust deallocation size and regions commit and allocate top */
                deallocate_size -= (char *) g_last->top_allocated - (char *) base_committed;
                g_last->top_committed = base_committed;
                g_last->top_allocated = base_committed;
            }
        }
        /* Adjust regions allocate top */
        g_last->top_allocated = (char *) g_last->top_allocated - deallocate_size;
        /* Check for underflow */
        if ((char *) g_last->top_reserved - g_last->reserve_size > (char *) g_last->top_allocated ||
            g_last->top_allocated > g_last->top_committed) {
            /* Adjust regions allocate top */
            g_last->top_allocated = (char *) g_last->top_reserved - g_last->reserve_size;
            goto sbrk_exit;
        }
        result = g_last->top_allocated;
    }
    /* Assert invariants */
    assert (g_last);
    assert ((char *) g_last->top_reserved - g_last->reserve_size <= (char *) g_last->top_allocated &&
            g_last->top_allocated <= g_last->top_committed);
    assert ((char *) g_last->top_reserved - g_last->reserve_size <= (char *) g_last->top_committed &&
            g_last->top_committed <= g_last->top_reserved &&
            (unsigned) g_last->top_committed % g_pagesize == 0);
    assert ((unsigned) g_last->top_reserved % g_regionsize == 0);
    assert ((unsigned) g_last->reserve_size % g_regionsize == 0);

sbrk_exit:
    /* Release spin lock */
    slrelease (&g_sl);
    return result;
}
#endif // #if 0

//#define USE_PTMALLOC3_ARENA

#ifdef USE_PTMALLOC3_ARENA
#	include "virtual_alloc.h"
#endif // #ifdef USE_PTMALLOC3_ARENA

#pragma warning(disable:4100)
#pragma warning(disable:4127)

/* mmap for windows */
//static
void *mmap ( void *ptr, INTERNAL_INTPTR_T size, INTERNAL_INTPTR_T prot, INTERNAL_INTPTR_T type, INTERNAL_INTPTR_T handle, INTERNAL_INTPTR_T arg) {
#ifndef USE_PTMALLOC3_ARENA
    static INTERNAL_INTPTR_T g_pagesize;
    static INTERNAL_INTPTR_T g_regionsize;
	DWORD alloc=MEM_RESERVE|MEM_TOP_DOWN, ntprot=0;
	INTERNAL_INTPTR_T rounding=0;
	char *p;
#ifdef TRACE
    printf ("mmap %p %d %d %d\n", ptr, size, prot, type);
#endif
    /* Wait for spin lock */
    slwait (&g_sl);
    /* First time initialization */
    if (! g_pagesize) 
        g_pagesize = getpagesize ();
    if (! g_regionsize) 
        g_regionsize = getregionsize ();
    /* Assert preconditions */
    assert (*(unsigned*) &ptr % g_pagesize == 0);
    assert (size % g_pagesize == 0);
    /* Allocate this */
	if(!(type & MAP_NORESERVE)) alloc|=MEM_COMMIT;
	if((prot & (PROT_READ|PROT_WRITE))==(PROT_READ|PROT_WRITE)) ntprot|=PAGE_READWRITE;
	else if(prot & PROT_READ) ntprot|=PAGE_READONLY;
	else if(prot & PROT_WRITE) ntprot|=PAGE_READWRITE;
	else
	{
		ntprot|=PAGE_NOACCESS;
		if(size==HEAP_MAX_SIZE)
		{
			rounding=size;
			size<<=1;
#ifdef TRACE
			printf("Rounding to multiple of %d\n", rounding);
#endif
		}
		if(ptr)
		{	/* prot==PROT_NONE also appears to be a euphemism for free */
			MEMORY_BASIC_INFORMATION mbi;
			DWORD read=0;
			for(p=((char *)ptr)+read; read<(DWORD) size && VirtualQuery(p, &mbi, sizeof(mbi)); read+=mbi.RegionSize)
			{
				if(mbi.State & MEM_COMMIT)
				{
//					if(!VirtualFree((LPVOID) p, mbi.RegionSize, MEM_DECOMMIT))
//						goto mmap_exit;
					if(!VirtualAlloc((LPVOID) p, mbi.RegionSize, MEM_RESERVE, PAGE_NOACCESS))
						goto mmap_exit;
#ifdef TRACE
					printf ("Release %p %d\n", p, mbi.RegionSize);
#endif
				}
			}
			ptr=0; /* success */
			goto mmap_exit;
		}
	}
    ptr = VirtualAlloc (ptr, size, alloc, ntprot);
    if (! ptr) {
        ptr = (void *) MORECORE_FAILURE;
        goto mmap_exit;
    }
	if(rounding)
	{
//		VirtualFree(ptr, 0, MEM_RELEASE);
		VirtualAlloc(ptr, 0, MEM_RESERVE, PAGE_NOACCESS);
		ptr=(void *)(((INTERNAL_SIZE_T)ptr + (rounding-1)) & ~(rounding-1));
//		if(!(ptr=VirtualAlloc(ptr, rounding, alloc, ntprot)))
		if(0==(ptr=VirtualAlloc(ptr, rounding, alloc, ntprot)))
		{
			ptr = (void *) MORECORE_FAILURE;
			goto mmap_exit;
		}
		assert ((unsigned) ptr % rounding == 0);
		size=rounding;
	}
	else
	{
		/* Assert postconditions */
		assert ((unsigned) ptr % g_regionsize == 0);
	}
#ifdef TRACE
	printf ("%s %p %d %d %d\n", (type & MAP_NORESERVE) ? "Reserve" : "Commit", ptr, size, prot, type);
#endif
mmap_exit:
    /* Release spin lock */
    slrelease (&g_sl);
    return ptr;
#else // #ifndef USE_PTMALLOC3_ARENA
	void*		result;
    slwait		( &g_sl );
	result		= virtual_alloc ( &g_ptmalloc3_arena, (unsigned int)size );
    slrelease	( &g_sl );
	return		( result );
#endif // #ifndef USE_PTMALLOC3_ARENA
}

/* munmap for windows */
//static
INTERNAL_INTPTR_T munmap ( void *ptr, INTERNAL_INTPTR_T size) {
#ifndef USE_PTMALLOC3_ARENA
    static INTERNAL_INTPTR_T g_pagesize;
    int rc = MUNMAP_FAILURE;
#ifdef TRACE
    printf ("munmap %p %d\n", ptr, size);
#endif
    /* Wait for spin lock */
    /* slwait (&g_sl); */
    /* First time initialization */
    if (! g_pagesize) 
        g_pagesize = getpagesize ();
    /* Assert preconditions */
	assert (((INTERNAL_SIZE_T) ptr) % g_pagesize == 0);
    assert (size % g_pagesize == 0);
    /* Free this */
//    if (! VirtualFree (ptr, 0, 
//                       MEM_RELEASE))
    if (! VirtualAlloc (ptr, 0, 
                       MEM_RESERVE, PAGE_NOACCESS))
        goto munmap_exit;
    rc = 0;
#ifdef TRACE
    printf ("Release %p %d\n", ptr, size);
#endif
munmap_exit:
    /* Release spin lock */
    /* slrelease (&g_sl); */
    return rc;
#else // #ifndef USE_PTMALLOC3_ARENA
    slwait		( &g_sl );
	virtual_free( &g_ptmalloc3_arena, ptr, size );
    slrelease	( &g_sl );
	return		( 0 );
#endif // #ifndef USE_PTMALLOC3_ARENA
}

#if 0
static int mprotect(const void *addr, INTERNAL_INTPTR_T len, int prot)
{
    static INTERNAL_INTPTR_T g_pagesize;
    static INTERNAL_INTPTR_T g_regionsize;
    int rc = -1;
	DWORD ntprot=0, oldntprot=0;
	MEMORY_BASIC_INFORMATION mbi;
	DWORD read=0;
	char *p;
#ifdef TRACE
    printf ("mprotect %p %d %d\n", addr, len, prot);
#endif
    /* Wait for spin lock */
    /* slwait (&g_sl); */
    /* First time initialization */
    if (! g_pagesize) 
        g_pagesize = getpagesize ();
    if (! g_regionsize) 
        g_regionsize = getregionsize ();
    /* Assert preconditions */
    assert ((unsigned) addr % g_pagesize == 0);
    assert (len% g_pagesize == 0);

	if((prot & (PROT_READ|PROT_WRITE))==(PROT_READ|PROT_WRITE)) ntprot|=PAGE_READWRITE;
	else if(prot & PROT_READ) ntprot|=PAGE_READONLY;
	else if(prot & PROT_WRITE) ntprot|=PAGE_READWRITE;
	else ntprot|=PAGE_NOACCESS;
	if(prot)
	{	/* Do we need to commit any? */
		MEMORY_BASIC_INFORMATION mbi;
		DWORD read=0;
		for(; read<(DWORD) len && VirtualQuery(((char *)(addr))+read, &mbi, sizeof(mbi)); read+=mbi.RegionSize)
		{
			if(!(mbi.State & MEM_COMMIT))
			{	/* Might as well do the lot */
				if(!VirtualAlloc((LPVOID) addr, len, MEM_COMMIT, ntprot))
					goto mprotect_exit;
#ifdef TRACE
				printf ("Commit (mprotect) %p %d\n", addr, len);
#endif
				break;
			}
		}
	}
	else
	{	/* prot==PROT_NONE also appears to be a euphemism for free */
		for(p=((char *)addr)+read; read<(DWORD) len && VirtualQuery(p, &mbi, sizeof(mbi)); read+=mbi.RegionSize)
		{
			if(mbi.State & MEM_COMMIT)
			{
				if(!VirtualFree((LPVOID) p, mbi.RegionSize, MEM_DECOMMIT))
					goto mprotect_exit;
#ifdef TRACE
				printf ("Release (mprotect) %p %d\n", p, mbi.RegionSize);
#endif
			}
		}
	}
    /* Change */
    if (! VirtualProtect ((LPVOID) addr, len, ntprot, &oldntprot))
        goto mprotect_exit;
    rc = 0;
#ifdef TRACE
    printf ("Protect %p %d %d\n", addr, len, prot);
#endif
mprotect_exit:
    /* Release spin lock */
    /* slrelease (&g_sl); */
    return rc;
}

static void vminfo (INTERNAL_SIZE_T  *free, INTERNAL_SIZE_T  *reserved, INTERNAL_SIZE_T  *committed) {
    MEMORY_BASIC_INFORMATION memory_info;
    memory_info.BaseAddress = 0;
    *free = *reserved = *committed = 0;
    while (VirtualQuery (memory_info.BaseAddress, &memory_info, sizeof (memory_info))) {
        switch (memory_info.State) {
        case MEM_FREE:
            *free += memory_info.RegionSize;
            break;
        case MEM_RESERVE:
            *reserved += memory_info.RegionSize;
            break;
        case MEM_COMMIT:
            *committed += memory_info.RegionSize;
            break;
        }
        memory_info.BaseAddress = (char *) memory_info.BaseAddress + memory_info.RegionSize;
    }
}

static int cpuinfo (int whole, INTERNAL_SIZE_T  *kernel, INTERNAL_SIZE_T  *user) {
    if (whole) {
        __int64 creation64, exit64, kernel64, user64;
        int rc = GetProcessTimes (GetCurrentProcess (), 
                                  (FILETIME *) &creation64,  
                                  (FILETIME *) &exit64, 
                                  (FILETIME *) &kernel64, 
                                  (FILETIME *) &user64);
        if (! rc) {
            *kernel = 0;
            *user = 0;
            return FALSE;
        } 
        *kernel = (INTERNAL_SIZE_T) (kernel64 / 10000);
        *user = (INTERNAL_SIZE_T) (user64 / 10000);
        return TRUE;
    } else {
        __int64 creation64, exit64, kernel64, user64;
        int rc = GetThreadTimes (GetCurrentThread (), 
                                 (FILETIME *) &creation64,  
                                 (FILETIME *) &exit64, 
                                 (FILETIME *) &kernel64, 
                                 (FILETIME *) &user64);
        if (! rc) {
            *kernel = 0;
            *user = 0;
            return FALSE;
        } 
        *kernel = (INTERNAL_SIZE_T) (kernel64 / 10000);
        *user = (INTERNAL_SIZE_T) (user64 / 10000);
        return TRUE;
    }
}
#endif // #if 0

#endif /* WIN32 */