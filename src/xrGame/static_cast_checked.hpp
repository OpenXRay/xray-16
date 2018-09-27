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

namespace debug
{
namespace detail
{
namespace static_cast_checked
{
template <typename destination_type>
struct value
{
    template <typename source_type>
    inline static void check(source_type* source)
    {
        VERIFY(smart_cast<destination_type>(source) == static_cast<destination_type>(source));
    }

    template <typename source_type>
    inline static void check(source_type& source)
    {
        VERIFY(&smart_cast<destination_type>(source) == &static_cast<destination_type>(source));
    }
};

template <typename source_type, typename destination_type>
struct helper
{
    template <bool is_polymorphic>
    inline static void check(source_type source)
    {
        if constexpr (is_polymorphic)
            value<destination_type>::check(source);
    }
};

} // namespace static_cast_checked
} // namespace detail
} // namespace debug

template <typename destination_type, typename source_type>
inline destination_type static_cast_checked(source_type const& source)
{
    using pointerless_type = typename object_type_traits::remove_pointer<source_type>::type;
    using pure_source_type = typename object_type_traits::remove_reference<pointerless_type>::type;

    debug::detail::static_cast_checked::helper<source_type const&,
        destination_type>::template check<std::is_polymorphic<pure_source_type>::value>(source);

    return (static_cast<destination_type>(source));
}

template <typename destination_type, typename source_type>
inline destination_type static_cast_checked(source_type& source)
{
    using pointerless_type = typename object_type_traits::remove_pointer<source_type>::type;
    using pure_source_type = typename object_type_traits::remove_reference<pointerless_type>::type;

    debug::detail::static_cast_checked::helper<source_type&,
        destination_type>::template check<std::is_polymorphic<pure_source_type>::value>(source);

    return (static_cast<destination_type>(source));
}

#else // #ifdef DEBUG
#define static_cast_checked static_cast
#endif // #ifdef DEBUG

#endif // STATIC_CAST_CHECKED_HPP_INCLUDED
