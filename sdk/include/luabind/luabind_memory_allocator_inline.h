////////////////////////////////////////////////////////////////////////////
//	Module 		: luabind_memory_allocator_inline.h
//	Created 	: 18.03.2007
//  Modified 	: 23.04.2008
//	Author		: Dmitriy Iassenev
//	Description : luabind memory allocator template class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef LUABIND_MEMORY_ALLOCATOR_INLINE_H_INCLUDED
#define LUABIND_MEMORY_ALLOCATOR_INLINE_H_INCLUDED

#define TEMPLATE_SPECIALIZATION	template <class T>
#define MEMORY_ALLOCATOR		luabind::memory_allocator<T>

TEMPLATE_SPECIALIZATION
MEMORY_ALLOCATOR::memory_allocator										()
{
}

TEMPLATE_SPECIALIZATION
MEMORY_ALLOCATOR::memory_allocator										(self_type const&)
{
}

TEMPLATE_SPECIALIZATION
template<class other>
MEMORY_ALLOCATOR::memory_allocator										(memory_allocator<other> const&)
{
}

TEMPLATE_SPECIALIZATION
template<class other>
MEMORY_ALLOCATOR &MEMORY_ALLOCATOR::operator=							(memory_allocator<other> const&)
{
	return			(*this);
}

TEMPLATE_SPECIALIZATION
typename MEMORY_ALLOCATOR::pointer MEMORY_ALLOCATOR::address			(reference value) const
{
	return			(&value);
}

TEMPLATE_SPECIALIZATION
typename MEMORY_ALLOCATOR::const_pointer MEMORY_ALLOCATOR::address		(const_reference value) const
{
	return			(&value);
}

TEMPLATE_SPECIALIZATION
typename MEMORY_ALLOCATOR::pointer MEMORY_ALLOCATOR::allocate			(size_type const n, void const* const p=0) const
{
	pointer			result = (pointer)call_allocator(p,n*sizeof(T));
	if (!n)
		result		= (pointer)call_allocator(p,1*sizeof(T));

	return			(result);
}

TEMPLATE_SPECIALIZATION
char *MEMORY_ALLOCATOR::__charalloc										(size_type const n)
{
	return 			((char _FARQ *)allocate(n));
}

TEMPLATE_SPECIALIZATION
void MEMORY_ALLOCATOR::deallocate										(pointer const p, size_type const n) const
{
	call_allocator	(p,0);
}

TEMPLATE_SPECIALIZATION
void MEMORY_ALLOCATOR::deallocate										(void *p, size_type n) const
{
	call_allocator	(p,0);
}

TEMPLATE_SPECIALIZATION
void MEMORY_ALLOCATOR::construct										(pointer const p, T const& value)
{
	new(p) T		(value);
}

TEMPLATE_SPECIALIZATION
void MEMORY_ALLOCATOR::destroy											(pointer const p)
{
	p->~T			();
}

TEMPLATE_SPECIALIZATION
typename MEMORY_ALLOCATOR::size_type MEMORY_ALLOCATOR::max_size			() const
{
	size_type		count = ((size_type)(-1))/sizeof(T);
	if (count)
		return		(count);

	return			(1);
}

#undef TEMPLATE_SPECIALIZATION
#undef MEMORY_ALLOCATOR

namespace luabind {

template<class _0, class _1>
inline bool operator==	(memory_allocator<_0> const&, memory_allocator<_1> const&)
{
	return 			(true);
}

template<class _0, class _1>
inline bool operator!=	(memory_allocator<_0> const&, memory_allocator<_1> const&)
{
	return 			(false);
}

} // namespace luabind

#endif // #ifndef LUABIND_MEMORY_ALLOCATOR_INLINE_H_INCLUDED