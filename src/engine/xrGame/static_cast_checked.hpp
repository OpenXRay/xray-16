////////////////////////////////////////////////////////////////////////////
//	Module 		: static_cast_checked.hpp
//	Created 	: 04.12.2007
//  Modified 	: 04.12.2007
//	Author		: Dmitriy Iassenev
//	Description : checked static_cast implementation for debug purposes
////////////////////////////////////////////////////////////////////////////

#ifndef STATIC_CAST_CHECKED_HPP_INCLUDED
#define STATIC_CAST_CHECKED_HPP_INCLUDED

#ifdef DEBUG

namespace debug {
namespace detail {
namespace static_cast_checked {

template <typename destination_type>
struct value {
	template <typename source_type>
	inline static void check		(source_type *source)
	{
		VERIFY		(smart_cast<destination_type>(source) == static_cast<destination_type>(source));
	}

	template <typename source_type>
	inline static void check		(source_type &source)
	{
		VERIFY		(&smart_cast<destination_type>(source) == &static_cast<destination_type>(source));
	}

};

template <typename source_type, typename destination_type>
struct helper {
	template <bool is_polymrphic>
	inline static void check		(source_type source)
	{
		value<
			destination_type
		>::check	(source);
	}

	template <>
	inline static void check<false>	(source_type source)
	{
	}
};

} // namespace static_cast_checked
} // namespace detail
} // namespace debug

template <typename destination_type, typename source_type>
inline destination_type static_cast_checked	(source_type const & source)
{
	typedef object_type_traits::remove_pointer<source_type>::type			pointerless_type;
	typedef object_type_traits::remove_reference<pointerless_type>::type	pure_source_type;

	debug::detail::static_cast_checked::helper<
		source_type const &,
		destination_type
	>::check<
		is_polymorphic<
			pure_source_type
		>::result
	>				(source);

	return			(static_cast<destination_type>(source));
}

template <typename destination_type, typename source_type>
inline destination_type static_cast_checked	(source_type & source)
{
	typedef object_type_traits::remove_pointer<source_type>::type			pointerless_type;
	typedef object_type_traits::remove_reference<pointerless_type>::type	pure_source_type;

	debug::detail::static_cast_checked::helper<
		source_type &,
		destination_type
	>::check<
		is_polymorphic<
			pure_source_type
		>::result
	>				(source);

	return			(static_cast<destination_type>(source));
}

#else // #ifdef DEBUG
#	define static_cast_checked	static_cast
#endif // #ifdef DEBUG

#endif // STATIC_CAST_CHECKED_HPP_INCLUDED