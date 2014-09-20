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

#ifdef DEBUG
	void add_smart_cast_stats		(LPCSTR,LPCSTR);
#	ifdef SMART_CAST_STATS_ALL
		void add_smart_cast_stats_all	(LPCSTR,LPCSTR);
#	endif
#endif

#ifdef MASTER_GOLD
#	define MAX_SEQUENCE_LENGTH 1
#else // #ifdef MASTER_GOLD
#	define MAX_SEQUENCE_LENGTH 1
#endif // #ifdef MASTER_GOLD

//#define SHOW_SMART_CAST_UNOPTIMIZED_CASES

namespace SmartDynamicCast {

	template <typename List, typename Target>
	struct exists {
		template <typename P>
		struct iterator {
			typedef typename P::Head Head;
			typedef typename P::Tail Tail;
			
			template <typename T>
			struct selector {
				enum { value = iterator<Tail>::value };
			};

			template <>
			struct selector<Target> {
				enum { value = true };
			};

			enum { value = selector<Head>::value };
		};

		template <>
		struct iterator<Loki::NullType> {
			enum { value = false };
		};

		enum { value = iterator<List>::value };
	};

    template <typename List1, typename List2> 
    struct merge
    {
		template <typename T>
		struct iterator {
			typedef Loki::Typelist<typename T::Head,typename iterator<typename T::Tail>::result > result;
		};

		template <>
		struct iterator<Loki::NullType> {
			typedef List2 result;
		};

		typedef typename iterator<List1>::result result;
	};

	template <typename Base, typename Source>
	struct has_conversion {
	template <typename T>
		struct search_base {
			typedef typename T::Head Head;
			typedef typename T::Tail Tail;

			template <bool>
			struct selector {
				typedef typename Head result;
			};

			template <>
			struct selector<false> {
				typedef typename search_base<Tail>::result result;
			};

			typedef typename 
				selector<
					is_type<Base,typename Head::Head>::value
				>::result result;
		};

		template <>
		struct search_base<Loki::NullType> {
			typedef Loki::NullType result;
		};

		template <typename T>
		struct search_conversion {
			template <bool>
			struct selector {
				enum { value = true };
			};

			template <>
			struct selector<false> {
				enum { value = search_conversion<typename T::Tail>::value };
			};

			enum { 
				value = 
					selector<
						object_type_traits::is_same<
							typename T::Head,
							Source
						>::value
					>::value
			};
		};

		template <>
		struct search_conversion<Loki::NullType> {
			enum { value = false};
		};

		enum { value = search_conversion<search_base<cast_type_list>::result>::value };
	};

	template <typename T>
	struct has_any_conversion {
		template <typename P>
		struct iterator {
			typedef typename P::Head Head;
			typedef typename P::Tail Tail;

			template <typename Q>
			struct _selector {
				enum { value = iterator<Tail>::value };
			};

			template <>
			struct _selector<T> {
				enum { value = true };
			};

			enum { value = _selector<typename Head::Head>::value };
		};

		template <>
		struct iterator<Loki::NullType> {
			enum { value = false };
		};

		enum { value = iterator<cast_type_list>::value };
	};

	template <typename Target, typename Source>
	struct CMatcher {

		template <typename T>
		struct CMatchHelper;

		template <typename HeadTail, typename T>
		struct CMatchHelper2 {
			template <typename P>
			struct CMatchHelper3 {
				typedef typename P::Head	Head;
				typedef typename P::Tail	Tail;
				typedef typename T::Head	PrevHead;

				template <bool>
				struct selector {
					template <bool>
					struct _selector {
						typedef Loki::Typelist<typename PrevHead::Head,Loki::Typelist<Target,Loki::NullType> > result;
					};

					template <>
					struct _selector<false> {
						typedef Loki::Typelist<typename PrevHead::Head,Loki::Typelist<Head,Loki::Typelist<Target,Loki::NullType> > > result;
					};

					typedef typename 
						_selector<
							object_type_traits::is_same<
								Head,
								typename PrevHead::Head
							>::value
						>::result result;
				};

				template <>
				struct selector<false> {
					typedef typename CMatchHelper3<Tail>::result result;
				};

				typedef typename selector<has_conversion<Head,Target>::value>::result result;
			};

			template <>
			struct CMatchHelper3<Loki::NullType> {
				typedef typename CMatchHelper<typename T::Tail>::result result;
			};

			typedef typename CMatchHelper3<HeadTail>::result result;
		};

		template <typename T>
		struct CMatchHelper {
			typedef typename T::Head		Head;
			typedef typename T::Tail		Tail;
			typedef typename Head::Tail		HeadTail;

			template <bool>
			struct selector {
				typedef typename CMatchHelper2<Head,T>::result result;
			};

			template <>
			struct selector<false> {
				typedef typename CMatchHelper<Tail>::result result;
			};

			typedef typename selector<
					object_type_traits::is_base_and_derived<typename Head::Head,Source>::value || object_type_traits::is_same<typename Head::Head,Source>::value
				>::result result;
		};

		template <>
		struct CMatchHelper<Loki::NullType> {
			typedef Loki::NullType result;
		};

		typedef typename CMatchHelper<cast_type_list>::result result;
	};

	template <typename Target, typename Source, int max_length, bool can_use_heritage>
	struct conversion_sequence {
		
		template <typename T>
		struct list_iterator {
			typedef typename T::Head		Head;
			typedef typename T::Tail		Tail;
			typedef typename Head::Tail		HeadTail;

			template <typename T, int length, bool use_heritage>
			struct helper {
				typedef typename conversion_sequence<Target,T,length,use_heritage/**,new_visited/**/>::result search_result;

				template <bool>
				struct selector {
					typedef typename list_iterator<Tail>::result result;
				};

				template <>
				struct selector<false> {
					typedef search_result result;
				};

				typedef typename
					selector<
						is_type<Loki::NullType,search_result>::value
					>::result result;
			};

			template <bool>
			struct selector {
				typedef typename helper<typename Head::Head,max_length,false>::result helper_result;

				template <bool>
				struct _selector {
					typedef helper_result result;
				};

				template <>
				struct _selector<false> {
					typedef typename list_iterator<Tail>::result result;
				};

				typedef typename _selector<!is_type<Loki::NullType,helper_result>::value>::result result;
			};

			template <>
			struct selector<false> {
				template <bool>
				struct _selector {
					typedef typename helper<typename Head::Head,max_length-1,true>::result helper_result;

					template <bool>
					struct _selector2 {
						typedef Loki::Typelist<Source,helper_result> result;
					};

					template <>
					struct _selector2<false> {
						typedef typename list_iterator<Tail>::result result;
					};

					typedef typename _selector2<!is_type<Loki::NullType,helper_result>::value>::result result;
				};

				template <>
				struct _selector<false> {
					typedef typename list_iterator<Tail>::result result;
				};

				typedef typename _selector<has_conversion<Source,typename Head::Head>::value>::result result;
			};

			typedef typename 
				selector<
					can_use_heritage &&
					object_type_traits::is_base_and_derived<
						typename Head::Head,
						Source
					>::value
				>::result result;
		};

		template <>
		struct list_iterator<Loki::NullType> {
			typedef Loki::NullType result;
		};


		template <int length>
		struct selector {
			STATIC_CHECK(length > 1,Internal_error_please_report);

			typedef typename selector<1>::result nearest;

			template <bool>
			struct _selector {
				typedef nearest result;
			};

			template <>
			struct _selector<false> {
				typedef typename list_iterator<cast_type_list>::result result;
			};

			typedef typename 
				_selector<
					!is_type<Loki::NullType,nearest>::value
				>::result result;
		};

		template <>
		struct selector<1> {
			typedef typename CMatcher<Target,Source>::result result;
		};

		template <>
		struct selector<0> {
			typedef Loki::NullType result;
		};

		typedef typename selector<max_length>::result result;
	};

	template <typename T1, typename T2>
	struct get_conversion_sequence {
		typedef typename conversion_sequence<T1,T2,MAX_SEQUENCE_LENGTH,true>::result result;
	};

	template <typename T, typename Target>
	struct CSmartCaster {
		typedef typename T::Head		Head;
		typedef typename T::Tail		Tail;
		typedef typename Tail::Head		NextHead;

		template <typename P>
		struct CHelper {
			IC	static Target* smart_cast(Head *p)
			{
				return		(CSmartCaster<Tail,Target>::smart_cast(SmartDynamicCast::smart_cast<NextHead>(p)));
			}
		};

		template <>
		struct CHelper<Loki::NullType> {
			IC	static Target* smart_cast(Head *p)
			{
				return		(SmartDynamicCast::smart_cast<Target>(p));
			}
		};

		IC	static Target* smart_cast(Head *p)
		{
			if (!p)
				return	(reinterpret_cast<Target*>(p));
			return		(CHelper<typename Tail::Tail>::smart_cast(p));
		}
	};

	template <typename T1, typename T2>
	struct CSmartMatcher {
		template <typename T3>
		IC	static T1* smart_cast(T2 *p)
		{
			return		(CSmartCaster<T3,T1>::smart_cast(static_cast<typename T3::Head*>(p)));
		}

		template <>
		IC	static T1* smart_cast<Loki::NullType>(T2 *p)
		{
#ifdef SHOW_SMART_CAST_UNOPTIMIZED_CASES
#pragma todo("Dima to all : this smart_cast is not optimized!")
#endif
#ifdef DEBUG
			add_smart_cast_stats(typeid(T2*).name(),typeid(T1*).name());
#endif
			return		(dynamic_cast<T1*>(p));
		}
	};

	template <typename T1, typename T2>
	struct CHelper1 {
		template <bool base>
		IC	static T1* smart_cast(T2 *p)
		{
			return				(CSmartMatcher<T1,T2>::smart_cast<get_conversion_sequence<T1,T2>::result>(p));
		}

		template <>
		IC	static T1* smart_cast<true>(T2 *p)
		{
			return				(static_cast<T1*>(p));
		}
	};

	template <typename T1, typename T2>
	IC	T1* smart_cast(T2 *p)
	{
		return					(CHelper1<T1,T2>::smart_cast<object_type_traits::is_base_and_derived<T1,T2>::value || object_type_traits::is_same<T1,T2>::value>(p));
	}

	template <typename T2>
	struct CHelper2 {
		template <typename T1>
		IC	static T1* smart_cast(T2* p)
		{
			STATIC_CHECK		(!object_type_traits::is_const<T2>::value || object_type_traits::is_const<T1>::value,Cannot_use_smart_cast_to_convert_const_to_non_const);
			typedef object_type_traits::remove_const<T1>::type _T1;
			typedef object_type_traits::remove_const<T2>::type _T2;
#ifdef DEBUG
			T1					*temp = SmartDynamicCast::smart_cast<_T1>(const_cast<_T2*>(p));
			T1					*test = dynamic_cast<T1*>(p);
			VERIFY2				(
				temp == test,
				make_string(
					"SmartCast<%s*>(%s*) FAILED (result differs from the dynamic_cast) or object is CORRUPTED (0x%08x -> 0x%08x)!",
					typeid(T1).name(),
					typeid(T2).name(),
					*(u32*)&test,
					*(u32*)&temp
				)
			);
			return				(temp);
#else
			return				(SmartDynamicCast::smart_cast<_T1>(const_cast<_T2*>(p)));
#endif
		}

		template <>
		IC	static void* smart_cast<void>(T2* p)
		{
#ifdef SHOW_SMART_CAST_UNOPTIMIZED_CASES
#pragma todo("Dima to all : this smart_cast is not optimized!")
#endif
#ifdef DEBUG
			add_smart_cast_stats(typeid(T2*).name(),typeid(void*).name());
#endif
			if (!p)
				return			((void*)0);
			return				(dynamic_cast<void*>(p));
		}
	};
};

template <typename T1, typename T2>
IC	T1	smart_cast(T2* p)
{
#ifdef PURE_DYNAMIC_CAST_COMPATIBILITY_CHECK
	STATIC_CHECK				(object_type_traits::is_pointer<T1>::value,Invalid_target_type_for_Dynamic_Cast);
	STATIC_CHECK				(object_type_traits::is_void<object_type_traits::remove_pointer<T1>::type>::value || is_polymorphic<object_type_traits::remove_pointer<T1>::type>::result,Invalid_target_type_for_Dynamic_Cast);
	STATIC_CHECK				(is_polymorphic<T2>::result,Invalid_source_type_for_Dynamic_Cast);
#endif
#ifdef SMART_CAST_STATS_ALL
	add_smart_cast_stats_all	(typeid(T2*).name(),typeid(T1).name());
#endif
	if (!p)
		return					(reinterpret_cast<T1>(p));
	return						(SmartDynamicCast::CHelper2<T2>::smart_cast<object_type_traits::remove_pointer<T1>::type>(p));
}

template <typename T1, typename T2>
IC	T1	smart_cast(T2& p)
{
#ifdef PURE_DYNAMIC_CAST_COMPATIBILITY_CHECK
	STATIC_CHECK				(object_type_traits::is_reference<T1>::value,Invalid_target_type_for_Dynamic_Cast);
	STATIC_CHECK				(is_polymorphic<object_type_traits::remove_reference<T1>::type>::result,Invalid_target_type_for_Dynamic_Cast);
	STATIC_CHECK				(is_polymorphic<T2>::result,Invalid_source_type_for_Dynamic_Cast);
#endif
#ifdef SMART_CAST_STATS_ALL
	add_smart_cast_stats_all	(typeid(T2*).name(),typeid(object_type_traits::remove_reference<T1>::type*).name());
#endif
	return						(*SmartDynamicCast::CHelper2<T2>::smart_cast<object_type_traits::remove_reference<T1>::type>(&p));
}

#ifdef XRGAME_EXPORTS
	template <> extern
	CGameObject* SmartDynamicCast::smart_cast<CGameObject,CObject>(CObject *p);
#endif
