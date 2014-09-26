////////////////////////////////////////////////////////////////////////////
//	Module 		: luabind_types.h
//	Created 	: 18.03.2007
//  Modified 	: 23.04.2008
//	Author		: Dmitriy Iassenev
//	Description : luabind types
////////////////////////////////////////////////////////////////////////////

#ifndef LUABIND_TYPES_H_INCLUDED
#define LUABIND_TYPES_H_INCLUDED

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include <luabind/luabind_memory_allocator.h>

namespace luabind {
	typedef void* memory_allocation_function_parameter;
	typedef void* (__cdecl *memory_allocation_function_pointer) (memory_allocation_function_parameter parameter, void const *, size_t);

	template	<typename T>									class	internal_vector 		: public std::vector<T,memory_allocator<T> >							{ public: inline unsigned int size() const {return (unsigned int)(std::vector<T,memory_allocator<T> >::size());}};
	template	<typename T>									class	internal_list 			: public std::list<T,memory_allocator<T> >								{ public: };
	template	<typename K, class P=std::less<K> >				class	internal_set			: public std::set<K,P,memory_allocator<K> >								{ public: };
	template	<typename K, class P=std::less<K> >				class	internal_multiset		: public std::multiset<K,P,memory_allocator<K> >						{ public: };
	template	<typename K, class V, class P=std::less<K> >	class	internal_map 			: public std::map<K,V,P,memory_allocator<std::pair<const K,V> > >		{ public: };
	template	<typename K, class V, class P=std::less<K> >	class	internal_multimap		: public std::multimap<K,V,P,memory_allocator<std::pair<const K,V> > >	{ public: };

	typedef		std::basic_string<char, std::char_traits<char>, memory_allocator<char> >		internal_string;
} // namespace luabind

#endif // #ifndef LUABIND_TYPES_H_INCLUDED