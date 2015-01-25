////////////////////////////////////////////////////////////////////////////
//	Module 		: luabind_memory_manager_generator.h
//	Created 	: 05.01.2006
//  Modified 	: 23.04.2008
//	Author		: Dmitriy Iassenev
//	Description : memory manager generator
////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_PP_IS_ITERATING

#	ifndef LUABIND_MEMORY_MANAGER_GENERATOR
#		define LUABIND_MEMORY_MANAGER_GENERATOR

#		include <boost/preprocessor/repetition.hpp>
#		include <boost/preprocessor/iteration/iterate.hpp>

#		define PARAMETERS(J,I,D)				const BOOST_PP_CAT(P,I) & BOOST_PP_CAT(p,I)

// generate specializations
#		define BOOST_PP_ITERATION_LIMITS		(0, LUABIND_MEMORY_MANAGER_GENERATOR_MAX_ARITY)
#		define BOOST_PP_FILENAME_1				<luabind/luabind_memory_manager_generator.h> // this file
#		include BOOST_PP_ITERATE()

#	endif // #ifndef LUABIND_MEMORY_MANAGER_GENERATOR

#else // #ifndef BOOST_PP_IS_ITERATING

#	define n									BOOST_PP_ITERATION()
#	define merge_helper(a,b)					a##b
#	define merge(a,b)							merge_helper(a,b)
#	define luabind_new_detail					merge(luabind_new_detail,n)
#	define luabind_new_detail_copy_constructor	merge(luabind_new_detail_copy_constructor,n)

namespace luabind {

template <bool>
struct luabind_new_detail {
	template <typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, class P)>
	static inline T* initialize (T *result BOOST_PP_ENUM_TRAILING(n,PARAMETERS,BOOST_PP_EMPTY))
	{
		return	(new (result) T(BOOST_PP_ENUM_PARAMS(n,p)));
	}
};

template <>
struct luabind_new_detail<true> {
	template <typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, class P)>
	static inline T* initialize (T *result BOOST_PP_ENUM_TRAILING(n,PARAMETERS,BOOST_PP_EMPTY))
	{
		return	(result);
	}
};

struct luabind_new_detail_copy_constructor {
	template <typename T>
	static inline T* initialize (T *result, const T &value)
	{
		return	(new (result) T(value));
	}

	template <typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, class P)>
	static inline T* initialize (T *result BOOST_PP_ENUM_TRAILING(n,PARAMETERS,BOOST_PP_EMPTY))
	{
		return	(luabind_new_detail<__is_pod(T)>::initialize(result BOOST_PP_ENUM_TRAILING_PARAMS(n,p)));
	}
};

template <typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, class P)>
inline T* luabind_new (BOOST_PP_ENUM(n,PARAMETERS,BOOST_PP_EMPTY))
{
	T			*result = (T*)call_allocator(0,sizeof(T));
	return		(luabind_new_detail_copy_constructor::initialize(result BOOST_PP_ENUM_TRAILING_PARAMS(n,p)));
}

}

#	undef n
#	undef merge_helper
#	undef merge
#	undef luabind_new_detail
#	undef luabind_new_detail_copy_constructor

#endif // #ifndef BOOST_PP_IS_ITERATING