////////////////////////////////////////////////////////////////////////////
//	Created		: 14.08.2009
//	Author		: Armen Abroyan
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "doug_lea_allocator.h"

#define USE_DL_PREFIX
#define MSPACES 1
#define USE_OUT_OF_MEMORY_HANDLER
#define USE_LOCKS 0
#include "ptmalloc3/malloc-2.8.3.h"

static void __stdcall out_of_memory		( mspace const space, void const* const parameter, int const first_time )
{
	if ( first_time )
		return;

	doug_lea_allocator*	const allocator = ( doug_lea_allocator* )parameter;
	Debug.fatal		( DEBUG_INFO, "not enough memory for arena [%s]", allocator->get_arena_id( ) );
}

doug_lea_allocator::doug_lea_allocator	( void* arena, u32 arena_size, LPCSTR arena_id ) :
	m_arena_id		( arena_id )
{
	VERIFY			( m_arena_id );

	if ( arena && arena_size )
		m_dl_arena	= create_mspace_with_base( arena, arena_size, 0, &out_of_memory, (void*)this );
	else
		m_dl_arena	= create_mspace( 0, 0, 0, 0 );
}

doug_lea_allocator::~doug_lea_allocator	( )
{
	VERIFY			( m_dl_arena );
	destroy_mspace	( m_dl_arena );
}

void* doug_lea_allocator::malloc_impl	( u32 size )
{
	return			mspace_malloc( m_dl_arena, size );
}

void* doug_lea_allocator::realloc_impl	( void* pointer, u32 new_size )
{
	return			mspace_realloc( m_dl_arena, pointer, new_size );
}

void doug_lea_allocator::free_impl		( void*& pointer )
{
	mspace_free		( m_dl_arena, pointer );
	pointer			= 0;
}

u32 doug_lea_allocator::get_allocated_size	( ) const
{
	return			(u32)mspace_mallinfo(m_dl_arena).uordblks;
}