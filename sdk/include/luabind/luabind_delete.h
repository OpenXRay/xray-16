////////////////////////////////////////////////////////////////////////////
//	Module 		: luabind_delete.h
//	Created 	: 24.06.2005
//  Modified 	: 20.05.2008
//	Author		: Dmitriy Iassenev
//	Description : luabind delete
////////////////////////////////////////////////////////////////////////////

#ifndef LUABIND_DELETE_H_INCLUDED
#define LUABIND_DELETE_H_INCLUDED

namespace luabind {
	extern LUABIND_API	memory_allocation_function_pointer	allocator;

	template <typename T>
	inline void delete_helper2		(T *&pointer, void *top_pointer)
	{
		pointer->~T					();
		call_allocator				(top_pointer,0);
		pointer						= 0;
	}

	template <typename T, bool polymorphic>
	struct delete_helper {
		static inline void apply	(T *&pointer)
		{
			delete_helper2			(pointer,dynamic_cast<void*>(pointer));
		}
	};

	template <typename T>
	struct delete_helper<T,false> {
		static inline void apply	(T *&pointer)
		{
			delete_helper2			(pointer,pointer);
		}
	};

	template <typename T>
	inline void luabind_delete		(T *&pointer)
	{
		if (!pointer)
			return;

		delete_helper<
			T,
			boost::is_polymorphic<T>::value
		>::apply					(
			pointer
		);
	}
} // namespace luabind

#endif // #ifndef LUABIND_DELETE_H_INCLUDED