// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifndef stdafxH
#define stdafxH
//#pragma once

#include "../xrCore/xrCore.h"

#include "../xrCore/doug_lea_allocator.h"
#include "../xrCore/memory_allocator_options.h"

#ifdef USE_ARENA_ALLOCATOR
	extern doug_lea_allocator	g_collision_allocator;

#	define CNEW(type)			new ( g_collision_allocator.alloc_impl<type>(1) ) type 
#	define CDELETE(ptr)			cdelete(ptr)
#	define CFREE(ptr)			g_collision_allocator.free_impl(ptr)
#	define CMALLOC(size)		g_collision_allocator.malloc_impl(size)
#	define CALLOC(type,count)	g_collision_allocator.alloc_impl<type>(count)
#else // #ifdef USE_ARENA_ALLOCATOR
#	define CNEW(type)			new ( xr_alloc<type>(1) ) type 
#	define CDELETE(ptr)			xr_delete(ptr)
#	define CFREE(ptr)			xr_free(ptr)
#	define CMALLOC(size)		xr_malloc(size)
#	define CALLOC(type,count)	xr_alloc<type>(count)
#endif // #ifdef USE_ARENA_ALLOCATOR

template <bool _is_pm, typename T>
struct cspecial_free
{
	IC void operator()(T* &ptr)
	{
		void*	_real_ptr	= dynamic_cast<void*>(ptr);
		ptr->~T			();
		CFREE	(_real_ptr);
	}
};

template <typename T>
struct cspecial_free<false,T>
{
	IC void operator()(T* &ptr)
	{
		ptr->~T			();
		CFREE	(ptr);
	}
};

template <class T>
IC	void cdelete		(T* &ptr)
{
	if (ptr) 
	{
		cspecial_free<is_polymorphic<T>::result,T>()(ptr);
		ptr = NULL;
	}
}

#define ENGINE_API
#include "opcode.h"

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // stdafxH