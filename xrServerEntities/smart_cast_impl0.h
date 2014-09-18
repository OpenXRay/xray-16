////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cast_impl1.h
//	Created 	: 17.09.2004
//  Modified 	: 17.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Smart dynamic cast implementation
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../xrcore/_type_traits.h"
#include "object_type_traits.h"

namespace SmartDynamicCast {
	template <typename T1, typename T2>
	IC	T1* smart_cast(T2 *p);

	template <typename List, typename T, typename P>
	struct CTypeHelper {
		template <typename T1>
		struct add {
			typedef typename T1::Head Head;

			template <bool a>
			struct selector {
				typedef Loki::Typelist<
							Loki::Typelist<
								typename Head::Head,
								Loki::Typelist<
									P,
									typename Head::Tail
								>
							>,
							typename Loki::TL::Erase<
								List,
								Head
							>::Result
						> result;
			};

			template <>
			struct selector<false> {
				typedef typename add<typename T1::Tail>::result result;
			};

			typedef typename selector<is_type<T,typename Head::Head>::value>::result result;
		};

		template <>
		struct add<Loki::NullType> {
			typedef Loki::Typelist<Loki::Typelist<T,Loki::Typelist<P,Loki::NullType> >,List> result;
		};

		typedef typename add<List>::result result;
	};
};

#define	cast_type_list			Loki::NullType
#define	add_to_cast_list(B,A)	typedef SmartDynamicCast::CTypeHelper<cast_type_list,A,B>::result TypeList_##A##B
#define	save_cast_list(B,A)		TypeList_##A##B

#define DECLARE_SPECIALIZATION(B,A,C) \
	class A;\
	class B;\
	template <> extern\
	B* SmartDynamicCast::smart_cast<B,A>(A *p);\
	add_to_cast_list(B,A);
