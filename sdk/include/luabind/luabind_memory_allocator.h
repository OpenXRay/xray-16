////////////////////////////////////////////////////////////////////////////
//	Module 		: luabind_memory_allocator.h
//	Created 	: 24.06.2005
//  Modified 	: 23.04.2008
//	Author		: Dmitriy Iassenev
//	Description : luabind memory allocator template class
////////////////////////////////////////////////////////////////////////////

#ifndef LUABIND_MEMORY_ALLOCATOR_H_INCLUDED
#define LUABIND_MEMORY_ALLOCATOR_H_INCLUDED

namespace luabind {

template <class T>
class memory_allocator {
private:
	typedef memory_allocator<T>	self_type;

public:
	typedef	size_t				size_type;
	typedef ptrdiff_t			difference_type;
	typedef T*					pointer;
	typedef const T*			const_pointer;
	typedef T&					reference;
	typedef const T&			const_reference;
	typedef T					value_type;

public:
	template<class _1>	
	struct rebind			{
		typedef memory_allocator<_1> other;
	};

public:
								memory_allocator	();
								memory_allocator	(const memory_allocator<T> &);

public:
	template<class _1>
	inline						memory_allocator	(const memory_allocator<_1>&);

public:
	template<class _1>
	inline	memory_allocator<T>	&operator=			(const memory_allocator<_1>&);

public:
	inline pointer				address				(reference value) const;
	inline const_pointer		address				(const_reference value) const;
	inline	pointer				allocate			(size_type n, const void * p=0) const;
	inline	char				*__charalloc		(size_type n);
	inline	void				deallocate			(pointer p, size_type n) const;
	inline	void				deallocate			(void *p, size_type n) const;
	inline	void				construct			(pointer p, const T& value);
	inline	void				destroy				(pointer p);
	inline	size_type			max_size			() const;
}; // class memory_allocator

	template<class _0, class _1>
	inline	bool				operator==			(const memory_allocator<_0>&, const memory_allocator<_1>&);

	template<class _0, class _1>
	inline	bool				operator!=			(const memory_allocator<_0>&, const memory_allocator<_1>&);
} // namespace luabind

#include <luabind/luabind_memory_allocator_inline.h>

#endif // #ifndef LUABIND_MEMORY_ALLOCATOR_H_INCLUDED