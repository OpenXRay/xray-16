///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __GSIMEMORY_H__
#define __GSIMEMORY_H__


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gsCommon.h"

// GameSpy allocation wrappers.  Used for quickly adding pre-post  allocation functionality such as
// - routing to a specific mempool
// - collecting mem usage stats
// (x) is a enumerated type for the specific module
#if(1)
	#define GSI_PRE_ALLOC(x)		
	#define GSI_POST_ALLOC()
#elif(0)
	// - collecting mem usage stats
	#define GSI_PRE_ALLOC(x)	gsMemMgrTagPush	(x);
	#define GSI_POST_ALLOC()	gsMemMgrTagPop	();
#elif(0)
	// - routing to a specific mempool
	#define GSI_PRE_ALLOC(x)	gsMemMgrContextPush	(x);
	#define GSI_POST_ALLOC()	gsMemMgrContextPop	();
#endif

#if defined (__cplusplus)
extern "C"
{
#endif


typedef enum
{
	MEMTAG_DEFAULT,
	MEMTAG_SERVER_BROWSER,
	MEMTAG_PEER,
	MEMTAG_GP,
	MEMTAG_QR2,
	MEMTAG_NN,
	MEMTAG_GT2,
	MEMTAG_COUNT
} MEMTAG_SDK;

//--------------------------------------------------------------------------
// GameSpy specific memory functions.  By default these will route to system malloc
// calls.  Use gsiMemoryCallbacksSet or gsiMemoryCallbacksGameSpySet to change this.
void* gsimalloc		(size_t size);
void* gsirealloc	(void* ptr, size_t size);
void  gsifree		(void* ptr);
void* gsimemalign	(size_t boundary, size_t size); // TODO

//--------------------------------------------------------------------------
// Customer supplied memory manager customization interface
// call this to replace the Gamespy specific memory functions with your own.
#ifdef WIN32
typedef	void	*(__cdecl *gsMallocCB)	(size_t size);
typedef	void	 (__cdecl *gsFreeCB)	(void* ptr);
typedef	void	*(__cdecl *gsReallocCB)	(void* ptr, size_t size);
typedef	void	*(__cdecl *gsMemalignCB)(size_t boundary, size_t size); 
#else
typedef	void	*(*gsMallocCB)	(size_t size);
typedef	void	 (*gsFreeCB)	(void* ptr);
typedef	void	*(*gsReallocCB)	(void* ptr, size_t size);
typedef	void	*(*gsMemalignCB)(size_t boundary, size_t size); 
#endif
// call this to override above gsi.... calls with your own.
void gsiMemoryCallbacksSet(gsMallocCB p_malloc, gsFreeCB p_free, gsReallocCB p_realloc, gsMemalignCB p_memalign);

//--------------------------------------------------------------------------
//  GameSpy Built in Memory Manager
// call this to override above gsi.... calls with GameSpy's built in memory manager
// *** You must have GSI_MEM_MANAGED defined, otherwise, you will have a link error ***/

// This is a list of memory pools used.  API specific values determined at run time.
// the gsi mem manager uses the concept of multiple memory pools or contexts.
// use pop and push commands to pop and push the current context off of the stack

typedef enum 
{
	gsMemMgrContext_Invalid=  -1,
	gsMemMgrContext_Default=   0,
	gsMemMgrContext_Count  =  16	// max number of mempools
}gsMemMgrContext;


// call this to enable GameSpy's provided memory manager
// Create a mempool for the given context.  If that context is in use, it will return the next available
// if none are avaible it will return gsMemMgrContext_Invalid
// exx use:  gQR2MemContext = gsMemMgrCreate		(0,0,16 * 1024);
// will find the first avaiable spot, create a mempool of 16k, and return the context handle.
// then later in your API
//	enter an API function
//		gsMemMgrContextPush(gQR2MemContext);
//		do some allocs
//		gQR2MemContextPop()
//	return from function.
// PoolName is purely for debugging and stats feedback purposes only.
// If you want your api to use the current, or the default pool, then don't bother creating one
// just always set the context to 0, and make sure int your app init, the default (0) pool is created.
/*
	Recommended usage:
	Call gsMemMgrCreate once at app start with a static buffer.  Make all calls to this.
	Alternatively, call it once per API to sue a seperate pool per API.
*/
gsMemMgrContext	gsMemMgrCreate		(gsMemMgrContext context, const char *PoolName,void* thePoolBuffer, size_t thePoolSize);	

// Use this to determine which pool and subsequent allocations will be taken from.
//exx use
/*
	fn()
	{
		gsMemMgrContextPush(thisAPIContext);

		make allocations. 

		//restore settings
		gsMemMgrContextPop(thisAPIContext);
		
	}
*/
// note, this is not neccessary for "free".
void			gsMemMgrContextPush	(gsMemMgrContext context);
gsMemMgrContext gsMemMgrContextPop	();


// clear contents, original mempool ptr must still be freed by app.
void			gsMemMgrDestroy(gsMemMgrContext context);	

// -------------Diagnostics------------------------
// These functions all run on the current mempool context.
void 			gsMemMgrDumpStats();
void 			gsMemMgrDumpAllocations();
void 			gsMemMgrValidateMemoryPool();	// walk heap and check integrity

// -------------Tool use	------------------------
// find which mempool context this ptr is part of, if any.
// returns gsMemMgrContext_Invalid otherwise.
gsMemMgrContext gsMemMgrContextFind	(void *ptr);
const char		*MemMgrBufferGetName(gsMemMgrContext context);


// -------------Memory Use Profiling ------------------------
// this tag is added to each concurrent alloc.  Use this to reference allocations.
// For example, you can find out the mem used by all ptr with a given tag
// in order to find out how much mem a module or set of allocs use.
void			gsMemMgrTagPush	(gsi_u8 tag);
void			gsMemMgrTagPop	();
gsi_u8			gsMemMgrTagGet	(void *ptr);
gsi_u32			gsMemMgrMemUsedByTagGet(gsi_u8 tag);

// return total available memory for the given memory pool context
gsi_u32			gsMemMgrMemAvailGet			(gsMemMgrContext context);
// return total used memory for the given memory pool context
gsi_u32			gsMemMgrMemUsedGet			(gsMemMgrContext context);
// return largest allocatable chunk within the given memory pool context.  This 
// will be the same or probably smaller then the value returned by gsMemMgrMemAvailGet
// depending on degree of memory fragmentation.
gsi_u32			gsMemMgrMemLargestAvailGet	(gsMemMgrContext context);

// The Highwater mark for memory used is the highest memory usage ever gets to for this
// given heap.  It is the most important stat, as your mempool must be at least this big.
// Exactly how big your pool needs to be depends on fragmentation.  So it may need to be slightly
// bigger then this amount.  
gsi_u32			gsMemMgrMemHighwaterMarkGet	(gsMemMgrContext context);


// -------------Self Test, not for production use ------------------------
void 			gsMemMgrSelfText();

#if defined (__cplusplus)
}
#endif

#endif // __GSIMEMORY_H__

