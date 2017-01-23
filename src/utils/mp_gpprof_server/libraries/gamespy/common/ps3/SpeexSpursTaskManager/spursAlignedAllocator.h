// Gamespy Technology
// NOTE:  this code has been provided by Sony for usage in Speex SPURS Manager

/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef BT_ALIGNED_ALLOCATOR
#define BT_ALIGNED_ALLOCATOR

///we probably replace this with our own aligned memory allocator
///so we replace _aligned_malloc and _aligned_free with our own
///that is better portable and more predictable

void*	spursAlignedAlloc	(int size, int alignment);

void	spursAlignedFree	(void* ptr);


typedef int	size_type;


template < typename T , unsigned Alignment >
class spursAlignedAllocator {
	
	typedef spursAlignedAllocator< T , Alignment > self_type;
	
public:

	//just going down a list:
	spursAlignedAllocator() {}
	/*
	btAlignedAllocator( const self_type & ) {}
	*/

	template < typename Other >
	spursAlignedAllocator( const spursAlignedAllocator< Other , Alignment > & ) {}

	typedef const T*         const_pointer;
	typedef const T&         const_reference;
	typedef T*               pointer;
	typedef T&               reference;
	typedef T                value_type;

	pointer       address   ( reference        ref ) const                           { return &ref; }
	const_pointer address   ( const_reference  ref ) const                           { return &ref; }
	pointer       allocate  ( size_type        n   , const_pointer *      hint = 0 ) {
		(void)hint;
		return reinterpret_cast< pointer >(spursAlignedAlloc( sizeof(value_type) * n , Alignment ));
	}
	void          construct ( pointer          ptr , const value_type &   value    ) { new (ptr) value_type( value ); }
	void          deallocate( pointer          ptr ) {
		spursAlignedFree( reinterpret_cast< void * >( ptr ) );
	}
	void          destroy   ( pointer          ptr )                                 { ptr->~value_type(); }
	

	template < typename O > struct rebind {
		typedef spursAlignedAllocator< O , Alignment > other;
	};
	template < typename O >
	self_type & operator=( const spursAlignedAllocator< O , Alignment > & ) { return *this; }

	friend bool operator==( const self_type & , const self_type & ) { return true; }
};



#endif //BT_ALIGNED_ALLOCATOR

