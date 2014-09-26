/*
  Default header file for malloc-2.7.2, written by Doug Lea
  and released to the public domain.  Use, modify, and redistribute
  this code without permission or acknowledgement in any way you wish.
  Send questions, comments, complaints, performance data, etc to
  dl@cs.oswego.edu.
 
  last update: Sun Feb 25 18:38:11 2001  Doug Lea  (dl at gee)

  This header is for ANSI C/C++ only.  You can set either of
  the following #defines before including:

  * If USE_DL_PREFIX is defined, it is assumed that malloc.c 
    was also compiled with this option, so all routines
    have names starting with "dl".

  * If HAVE_USR_INCLUDE_MALLOC_H is defined, it is assumed that this
    file will be #included AFTER <malloc.h>. This is needed only if
    your system defines a struct mallinfo that is incompatible with the
    standard one declared here.  Otherwise, you can include this file
    INSTEAD of your system system <malloc.h>.  At least on ANSI, all
    declarations should be compatible with system versions
*/

#ifndef MALLOC_270_H
#define MALLOC_270_H

// config
#define USE_DL_PREFIX

// 
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>   /* for size_t */

/*
  malloc(size_t n)
  Returns a pointer to a newly allocated chunk of at least n bytes, or
  null if no space is available. Additionally, on failure, errno is
  set to ENOMEM on ANSI C systems.

  If n is zero, malloc returns a minimum-sized chunk. The minimum size
  is 16 bytes on most 32bit systems, and either 24 or 32 bytes on
  64bit systems, depending on internal size and alignment restrictions.

  On most systems, size_t is an unsigned type.  Calls with values of n
  that appear "negative" when signed are interpreted as requests for
  huge amounts of space, which will most often fail.

  The maximum allowed value of n differs across systems, but is in all
  cases less (typically by 8K) than the maximum representable value of
  a size_t. Requests greater than this value result in failure.
*/

#ifndef USE_DL_PREFIX
void*  malloc(size_t);
#else
void*  dlmalloc(size_t);
#endif

/*
  free(void* p)
  Releases the chunk of memory pointed to by p, that had been previously
  allocated using malloc or a related routine such as realloc.
  It has no effect if p is null. It can have arbitrary (and bad!)
  effects if p has already been freed or was not obtained via malloc.

  Unless disabled using mallopt, freeing very large spaces will,
  when possible, automatically trigger operations that give
  back unused memory to the system, thus reducing program footprint.
*/
#ifndef USE_DL_PREFIX
void     free(void*);
#else
void     dlfree(void*);
#endif

/*
  calloc(size_t n_elements, size_t element_size);
  Returns a pointer to n_elements * element_size bytes, with all locations
  set to zero.
*/
#ifndef USE_DL_PREFIX
void*  calloc(size_t, size_t);
#else
void*  dlcalloc(size_t, size_t);
#endif

/*
  realloc(void* p, size_t n)
  Returns a pointer to a chunk of size n that contains the same data
  as does chunk p up to the minimum of (n, p's size) bytes.

  The returned pointer may or may not be the same as p. The algorithm
  prefers extending p when possible, otherwise it employs the
  equivalent of a malloc-copy-free sequence.

  If p is null, realloc is equivalent to malloc.  

  If space is not available, realloc returns null, errno is set (if on
  ANSI) and p is NOT freed.

  if n is for fewer bytes than already held by p, the newly unused
  space is lopped off and freed if possible.  Unless the #define
  REALLOC_ZERO_BYTES_FREES is set, realloc with a size argument of
  zero (re)allocates a minimum-sized chunk.

  Large chunks that were internally obtained via mmap will always
  be reallocated using malloc-copy-free sequences unless
  the system supports MREMAP (currently only linux).

  The old unix realloc convention of allowing the last-free'd chunk
  to be used as an argument to realloc is not supported.
*/

#ifndef USE_DL_PREFIX
void*  realloc(void*, size_t);
#else
void*  dlrealloc(void*, size_t);
#endif

/*
  memalign(size_t alignment, size_t n);
  Returns a pointer to a newly allocated chunk of n bytes, aligned
  in accord with the alignment argument.

  The alignment argument should be a power of two. If the argument is
  not a power of two, the nearest greater power is used.
  8-byte alignment is guaranteed by normal malloc calls, so don't
  bother calling memalign with an argument of 8 or less.

  Overreliance on memalign is a sure way to fragment space.
*/

#ifndef USE_DL_PREFIX
void*  memalign(size_t, size_t);
#else
void*  dlmemalign(size_t, size_t);
#endif


/*
  valloc(size_t n);
  Allocates a page-aligned chunk of at least n bytes.
  Equivalent to memalign(pagesize, n), where pagesize is the page
  size of the system. If the pagesize is unknown, 4096 is used.
*/

#ifndef USE_DL_PREFIX
void*  valloc(size_t);
#else
void*  dlvalloc(size_t);
#endif


/*
  independent_calloc(size_t n_elements, size_t element_size, void* chunks[]);

  independent_calloc is similar to calloc, but instead of returning a
  single cleared space, it returns an array of pointers to n_elements
  independent elements, each of which can hold contents of size
  elem_size.  Each element starts out cleared, and can be
  independently freed, realloc'ed etc. The elements are guaranteed to
  be adjacently allocated (this is not guaranteed to occur with
  multiple callocs or mallocs), which may also improve cache locality
  in some applications.

  The "chunks" argument is optional (i.e., may be null, which is
  probably the most typical usage). If it is null, the returned array
  is itself dynamically allocated and should also be freed when it is
  no longer needed. Otherwise, the chunks array must be of at least
  n_elements in length. It is filled in with the pointers to the
  chunks.

  In either case, independent_calloc returns this pointer array, or
  null if the allocation failed.  If n_elements is zero and "chunks"
  is null, it returns a chunk representing an array with zero elements
  (which should be freed if not wanted).

  Each element must be individually freed when it is no longer
  needed. If you'd like to instead be able to free all at once, you
  should instead use regular calloc and assign pointers into this
  space to represent elements.  (In this case though, you cannot
  independently free elements.)
  
  independent_calloc simplifies and speeds up implementations of many
  kinds of pools.  It may also be useful when constructing large data
  structures that initially have a fixed number of fixed-sized nodes,
  but the number is not known at compile time, and some of the nodes
  may later need to be freed. For example:

  struct Node { int item; struct Node* next; };
  
  struct Node* build_list() {
    struct Node** pool;
    int n = read_number_of_nodes_needed();
    if (n <= 0) return 0;
    pool = (struct Node**)(independent_calloc(n, sizeof(struct Node), 0);
    if (pool == 0) return 0; // failure
    // organize into a linked list... 
    struct Node* first = pool[0];
    for (i = 0; i < n-1; ++i) 
      pool[i]->next = pool[i+1];
    free(pool);     // Can now free the array (or not, if it is needed later)
    return first;
  }
*/

#ifndef USE_DL_PREFIX
void** independent_calloc(size_t, size_t, void**);
#else
void** dlindependent_calloc(size_t, size_t, void**);
#endif

/*
  independent_comalloc(size_t n_elements, size_t sizes[], void* chunks[]);

  independent_comalloc allocates, all at once, a set of n_elements
  chunks with sizes indicated in the "sizes" array.    It returns
  an array of pointers to these elements, each of which can be
  independently freed, realloc'ed etc. The elements are guaranteed to
  be adjacently allocated (this is not guaranteed to occur with
  multiple callocs or mallocs), which may also improve cache locality
  in some applications.

  The "chunks" argument is optional (i.e., may be null). If it is null
  the returned array is itself dynamically allocated and should also
  be freed when it is no longer needed. Otherwise, the chunks array
  must be of at least n_elements in length. It is filled in with the
  pointers to the chunks.

  In either case, independent_comalloc returns this pointer array, or
  null if the allocation failed.  If n_elements is zero and chunks is
  null, it returns a chunk representing an array with zero elements
  (which should be freed if not wanted).
  
  Each element must be individually freed when it is no longer
  needed. If you'd like to instead be able to free all at once, you
  should instead use a single regular malloc, and assign pointers at
  particular offsets in the aggregate space. (In this case though, you 
  cannot independently free elements.)

  independent_comallac differs from independent_calloc in that each
  element may have a different size, and also that it does not
  automatically clear elements.

  independent_comalloc can be used to speed up allocation in cases
  where several structs or objects must always be allocated at the
  same time.  For example:

  struct Head { ... }
  struct Foot { ... }

  void send_message(char* msg) {
    int msglen = strlen(msg);
    size_t sizes[3] = { sizeof(struct Head), msglen, sizeof(struct Foot) };
    void* chunks[3];
    if (independent_comalloc(3, sizes, chunks) == 0)
      die();
    struct Head* head = (struct Head*)(chunks[0]);
    char*        body = (char*)(chunks[1]);
    struct Foot* foot = (struct Foot*)(chunks[2]);
    // ...
  }

  In general though, independent_comalloc is worth using only for
  larger values of n_elements. For small values, you probably won't
  detect enough difference from series of malloc calls to bother.

  Overuse of independent_comalloc can increase overall memory usage,
  since it cannot reuse existing noncontiguous small chunks that
  might be available for some of the elements.
*/

#ifndef USE_DL_PREFIX
void** independent_comalloc(size_t, size_t*, void**);
#else
void** dlindependent_comalloc(size_t, size_t*, void**);
#endif


/*
  pvalloc(size_t n);
  Equivalent to valloc(minimum-page-that-holds(n)), that is,
  round up n to nearest pagesize.
 */

#ifndef USE_DL_PREFIX
void*  pvalloc(size_t);
#else
void*  dlpvalloc(size_t);
#endif

/*
  cfree(void* p);
  Equivalent to free(p).

  cfree is needed/defined on some systems that pair it with calloc,
  for odd historical reasons (such as: cfree is used in example 
  code in the first edition of K&R).
*/

#ifndef USE_DL_PREFIX
void     cfree(void*);
#else
void     dlcfree(void*);
#endif


/*
  malloc_trim(size_t pad);

  If possible, gives memory back to the system (via negative
  arguments to sbrk) if there is unused memory at the `high' end of
  the malloc pool. You can call this after freeing large blocks of
  memory to potentially reduce the system-level memory requirements
  of a program. However, it cannot guarantee to reduce memory. Under
  some allocation patterns, some large free blocks of memory will be
  locked between two used chunks, so they cannot be given back to
  the system.
  
  The `pad' argument to malloc_trim represents the amount of free
  trailing space to leave untrimmed. If this argument is zero,
  only the minimum amount of memory to maintain internal data
  structures will be left (one page or less). Non-zero arguments
  can be supplied to maintain enough trailing space to service
  future expected allocations without having to re-obtain memory
  from the system.
  
  Malloc_trim returns 1 if it actually released any memory, else 0.
  On systems that do not support "negative sbrks", it will always
  return 0.
*/

#ifndef USE_DL_PREFIX
int      malloc_trim(size_t);
#else
int      dlmalloc_trim(size_t);
#endif


/*
  malloc_usable_size(void* p);

  Returns the number of bytes you can actually use in an allocated
  chunk, which may be more than you requested (although often not) due
  to alignment and minimum size constraints.  You can use this many
  bytes without worrying about overwriting other allocated
  objects. This is not a particularly great programming practice. But
  malloc_usable_size can be more useful in debugging and assertions,
  for example:

  p = malloc(n);
  assert(malloc_usable_size(p) >= 256);
*/

#ifndef USE_DL_PREFIX
size_t   malloc_usable_size(void*);
#else
size_t   dlmalloc_usable_size(void*);
#endif


/*
  malloc_stats();
  Prints on stderr the amount of space obtained from the system (both
  via sbrk and mmap), the maximum amount (which may be more than
  current if malloc_trim and/or munmap got called), and the current
  number of bytes allocated via malloc (or realloc, etc) but not yet
  freed. Note that this is the number of bytes allocated, not the
  number requested. It will be larger than the number requested
  because of alignment and bookkeeping overhead. Because it includes
  alignment wastage as being in use, this figure may be greater than
  zero even when no user-level chunks are allocated.

  The reported current and maximum system memory can be inaccurate if
  a program makes other calls to system memory allocation functions
  (normally sbrk) outside of malloc.

  malloc_stats prints only the most commonly interesting statistics.
  More information can be obtained by calling mallinfo.
*/

#ifndef USE_DL_PREFIX
void     malloc_stats();
#else
void     dlmalloc_stats();
#endif

/*
  mallinfo()
  Returns (by copy) a struct containing various summary statistics:

  arena:     current total non-mmapped bytes allocated from system 
  ordblks:   the number of free chunks 
  smblks:    the number of fastbin blocks (i.e., small chunks that
               have been freed but not use resused or consolidated)
  hblks:     current number of mmapped regions 
  hblkhd:    total bytes held in mmapped regions 
  usmblks:   the maximum total allocated space. This will be greater
                than current total if trimming has occurred.
  fsmblks:   total bytes held in fastbin blocks 
  uordblks:  current total allocated space (normal or mmapped)
  fordblks:  total free space 
  keepcost:  the maximum number of bytes that could ideally be released
               back to system via malloc_trim. ("ideally" means that
               it ignores page restrictions etc.)

  The names of some of these fields don't bear much relation with
  their contents because this struct was defined as standard in
  SVID/XPG so reflects the malloc implementation that was then used
  in SystemV Unix.  

  The original SVID version of this struct, defined on most systems
  with mallinfo, declares all fields as ints. But some others define
  as unsigned long. If your system defines the fields using a type of
  different width than listed here, you should #include your system
  version before including this file.  The struct declaration is
  suppressed if _MALLOC_H is defined (which is done in most system
  malloc.h files). You can also suppress it by defining
  HAVE_USR_INCLUDE_MALLOC_H.

  Because these fields are ints, but internal bookkeeping is done with
  unsigned longs, the reported values may appear as negative, and may
  wrap around zero and thus be inaccurate.
*/

#ifndef HAVE_USR_INCLUDE_MALLOC_H
#ifndef _MALLOC_H
struct mallinfo {
  int arena;    
  int ordblks;  
  int smblks;   
  int hblks;    
  int hblkhd;   
  int usmblks;  
  int fsmblks;  
  int uordblks; 
  int fordblks; 
  int keepcost; 
};
#endif
#endif

#ifndef USE_DL_PREFIX
struct mallinfo mallinfo(void);
#else
struct mallinfo dlmallinfo(void);
#endif

/*
  mallopt(int parameter_number, int parameter_value)
  Sets tunable parameters The format is to provide a
  (parameter-number, parameter-value) pair.  mallopt then sets the
  corresponding parameter to the argument value if it can (i.e., so
  long as the value is meaningful), and returns 1 if successful else
  0.  SVID/XPG defines four standard param numbers for mallopt,
  normally defined in malloc.h.  Only one of these (M_MXFAST) is used
  in this malloc. The others (M_NLBLKS, M_GRAIN, M_KEEP) don't apply,
  so setting them has no effect. But this malloc also supports four
  other options in mallopt. See below for details.  Briefly, supported
  parameters are as follows (listed defaults are for "typical"
  configurations).

  Symbol            param #   default    allowed param values
  M_MXFAST          1         64         0-80  (0 disables fastbins)
  M_TRIM_THRESHOLD -1         128*1024   any   (-1U disables trimming)
  M_TOP_PAD        -2         0          any  
  M_MMAP_THRESHOLD -3         128*1024   any   (or 0 if no MMAP support)
  M_MMAP_MAX       -4         65536      any   (0 disables use of mmap)
*/

#ifndef USE_DL_PREFIX
int  mallopt(int, int);
#else
int  dlmallopt(int, int);
#endif

/* Descriptions of tuning options */

/*
  M_MXFAST is the maximum request size used for "fastbins", special bins
  that hold returned chunks without consolidating their spaces. This
  enables future requests for chunks of the same size to be handled
  very quickly, but can increase fragmentation, and thus increase the
  overall memory footprint of a program.

  This malloc manages fastbins very conservatively yet still
  efficiently, so fragmentation is rarely a problem for values less
  than or equal to the default.  The maximum supported value of MXFAST
  is 80. You wouldn't want it any higher than this anyway.  Fastbins
  are designed especially for use with many small structs, objects or
  strings -- the default handles structs/objects/arrays with sizes up
  to 8 4byte fields, or small strings representing words, tokens,
  etc. Using fastbins for larger objects normally worsens
  fragmentation without improving speed.

  You can reduce M_MXFAST to 0 to disable all use of fastbins.  This
  causes the malloc algorithm to be a closer approximation of
  fifo-best-fit in all cases, not just for larger requests, but will
  generally cause it to be slower.
*/

#ifndef M_MXFAST
#define M_MXFAST  1
#endif

/*
  M_TRIM_THRESHOLD is the maximum amount of unused top-most memory
  to keep before releasing via malloc_trim in free().

  Automatic trimming is mainly useful in long-lived programs.
  Because trimming via sbrk can be slow on some systems, and can
  sometimes be wasteful (in cases where programs immediately
  afterward allocate more large chunks) the value should be high
  enough so that your overall system performance would improve by
  releasing this much memory.

  The trim threshold and the mmap control parameters (see below)
  can be traded off with one another. Trimming and mmapping are
  two different ways of releasing unused memory back to the
  system. Between these two, it is often possible to keep
  system-level demands of a long-lived program down to a bare
  minimum. For example, in one test suite of sessions measuring
  the XF86 X server on Linux, using a trim threshold of 128K and a
  mmap threshold of 192K led to near-minimal long term resource
  consumption.

  If you are using this malloc in a long-lived program, it should
  pay to experiment with these values.  As a rough guide, you
  might set to a value close to the average size of a process
  (program) running on your system.  Releasing this much memory
  would allow such a process to run in memory.  Generally, it's
  worth it to tune for trimming rather tham memory mapping when a
  program undergoes phases where several large chunks are
  allocated and released in ways that can reuse each other's
  storage, perhaps mixed with phases where there are no such
  chunks at all.  And in well-behaved long-lived programs,
  controlling release of large blocks via trimming versus mapping
  is usually faster.

  However, in most programs, these parameters serve mainly as
  protection against the system-level effects of carrying around
  massive amounts of unneeded memory. Since frequent calls to
  sbrk, mmap, and munmap otherwise degrade performance, the default
  parameters are set to relatively high values that serve only as
  safeguards.

  The trim value It must be greater than page size to have any useful
  effect.  To disable trimming completely, you can set to 
  (unsigned long)(-1)

  Trim settings interact with fastbin (MXFAST) settings: Unless
  compiled with TRIM_FASTBINS defined, automatic trimming never takes
  place upon freeing a chunk with size less than or equal to
  MXFAST. Trimming is instead delayed until subsequent freeing of
  larger chunks. However, you can still force an attempted trim by
  calling malloc_trim.

  Also, trimming is not generally possible in cases where
  the main arena is obtained via mmap.

  Note that the trick some people use of mallocing a huge space and
  then freeing it at program startup, in an attempt to reserve system
  memory, doesn't have the intended effect under automatic trimming,
  since that memory will immediately be returned to the system.
*/

#define M_TRIM_THRESHOLD    -1

/*
  M_TOP_PAD is the amount of extra `padding' space to allocate or
  retain whenever sbrk is called. It is used in two ways internally:

  * When sbrk is called to extend the top of the arena to satisfy
  a new malloc request, this much padding is added to the sbrk
  request.

  * When malloc_trim is called automatically from free(),
  it is used as the `pad' argument.

  In both cases, the actual amount of padding is rounded
  so that the end of the arena is always a system page boundary.

  The main reason for using padding is to avoid calling sbrk so
  often. Having even a small pad greatly reduces the likelihood
  that nearly every malloc request during program start-up (or
  after trimming) will invoke sbrk, which needlessly wastes
  time.

  Automatic rounding-up to page-size units is normally sufficient
  to avoid measurable overhead, so the default is 0.  However, in
  systems where sbrk is relatively slow, it can pay to increase
  this value, at the expense of carrying around more memory than
  the program needs.
*/

#define M_TOP_PAD           -2


/*
  M_MMAP_THRESHOLD is the request size threshold for using mmap()
  to service a request. Requests of at least this size that cannot
  be allocated using already-existing space will be serviced via mmap.
  (If enough normal freed space already exists it is used instead.)

  Using mmap segregates relatively large chunks of memory so that
  they can be individually obtained and released from the host
  system. A request serviced through mmap is never reused by any
  other request (at least not directly; the system may just so
  happen to remap successive requests to the same locations).

  Segregating space in this way has the benefits that:

   1. Mmapped space can ALWAYS be individually released back 
      to the system, which helps keep the system level memory 
      demands of a long-lived program low. 
   2. Mapped memory can never become `locked' between
      other chunks, as can happen with normally allocated chunks, which
      means that even trimming via malloc_trim would not release them.
   3. On some systems with "holes" in address spaces, mmap can obtain
      memory that sbrk cannot.

  However, it has the disadvantages that:

   1. The space cannot be reclaimed, consolidated, and then
      used to service later requests, as happens with normal chunks.
   2. It can lead to more wastage because of mmap page alignment
      requirements
   3. It causes malloc performance to be more dependent on host
      system memory management support routines.

  The advantages of mmap nearly always outweigh disadvantages for
  "large" chunks, but the value of "large" varies across systems.  The
  default is an empirically derived value that works well in most
  systems.
*/

#define M_MMAP_THRESHOLD    -3

/*
  M_MMAP_MAX is the maximum number of requests to simultaneously
  service using mmap. This parameter exists because
  some systems have a limited number of internal tables for
  use by mmap, and using more than a few of them may degrade
  performance.

  The default is set to a value that serves only as a safeguard.
  Setting to 0 disables use of mmap for servicing large requests.  If
  mmap is not supported on a system, the default value is 0, and
  attempts to set it to non-zero values in mallopt will fail.
*/

#define M_MMAP_MAX          -4


/* Unused SVID2/XPG mallopt options, listed for completeness */

#ifndef M_NBLKS
#define M_NLBLKS  2    /* UNUSED in this malloc */
#endif
#ifndef M_GRAIN
#define M_GRAIN   3    /* UNUSED in this malloc */
#endif
#ifndef M_KEEP
#define M_KEEP    4    /* UNUSED in this malloc */
#endif

/* 
  Some malloc.h's declare alloca, even though it is not part of malloc.
*/

//#ifndef _ALLOCA_H
//extern void* alloca(size_t);
//#endif

#ifdef __cplusplus
};  /* end of extern "C" */
#endif

#endif /* MALLOC_270_H */
