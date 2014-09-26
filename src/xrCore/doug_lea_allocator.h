////////////////////////////////////////////////////////////////////////////
//	Created		: 14.08.2009
//	Author		: Armen Abroyan
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef DOUG_LEA_ALLOCATOR_H_INCLUDED
#define DOUG_LEA_ALLOCATOR_H_INCLUDED

class XRCORE_API doug_lea_allocator {
public:
					doug_lea_allocator	( void* arena, u32 arena_size, LPCSTR arena_id );
					~doug_lea_allocator	( );
			void*	malloc_impl			( u32 size );
			void*	realloc_impl		( void* pointer, u32 new_size );
			void	free_impl			( void*& pointer );
			u32		get_allocated_size	( ) const;
	inline	LPCSTR	get_arena_id		( ) const { return m_arena_id; }

	template <typename T>
	inline void		free_impl			( T*& pointer )
	{
		free_impl	( reinterpret_cast<void*&>(pointer) );
	}

	template < typename T >
	inline	T*		alloc_impl			( u32 const count )
	{
		return		(T*)malloc_impl( count*sizeof(T) );
	}

private:
	LPCSTR	m_arena_id;
	void*	m_dl_arena;
}; // class doug_lea_allocator

#endif // #ifndef DOUG_LEA_ALLOCATOR_H_INCLUDED