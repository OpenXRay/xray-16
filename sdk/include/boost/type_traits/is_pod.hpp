
// (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. This software 
// is provided "as is" without express or implied warranty, and with no claim 
// as to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_IS_POD_HPP_INCLUDED
#define BOOST_TT_IS_POD_HPP_INCLUDED

#include "boost/type_traits/config.hpp"
#include "boost/type_traits/is_void.hpp"
#include "boost/type_traits/is_scalar.hpp"
#include "boost/type_traits/detail/ice_or.hpp"
#include "boost/type_traits/intrinsics.hpp"

#include <cstddef>

// should be the last #include
#include "boost/type_traits/detail/bool_trait_def.hpp"

namespace boost {

// forward declaration, needed by 'is_pod_array_helper' template below
template< typename T > struct is_POD;

namespace detail {

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

template <typename T> struct is_pod_impl
{ 
    BOOST_STATIC_CONSTANT(
        bool, value =
        (::boost::type_traits::ice_or<
            ::boost::is_scalar<T>::value,
            ::boost::is_void<T>::value,
            BOOST_IS_POD(T)
         >::value));
};

template <typename T, std::size_t sz>
struct is_pod_impl<T[sz]>
    : is_pod_impl<T>
{
};

#else

template <bool is_array = false>
struct is_pod_helper
{
    template <typename T> struct result_
    {
        BOOST_STATIC_CONSTANT(
            bool, value =
            (::boost::type_traits::ice_or<
                ::boost::is_scalar<T>::value,
                ::boost::is_void<T>::value,
                BOOST_IS_POD(T)
            >::value));
    };
};

template <bool b>
struct bool_to_yes_no_type
{
    typedef ::boost::type_traits::no_type type;
};

template <>
struct bool_to_yes_no_type<true>
{
    typedef ::boost::type_traits::yes_type type;
};

template <typename ArrayType>
struct is_pod_array_helper
{
    enum { is_pod = ::boost::is_POD<ArrayType>::value }; // MSVC workaround
    typedef typename bool_to_yes_no_type<is_pod>::type type;
    type instance() const;
};

template <typename T>
is_pod_array_helper<T> is_POD_array(T*);

template <>
struct is_pod_helper<true>
{
    template <typename T> struct result_
    {
        static T& help();
        BOOST_STATIC_CONSTANT(bool, value =
            sizeof(is_POD_array(help()).instance()) == sizeof(::boost::type_traits::yes_type)
            );
    };
};


template <typename T> struct is_pod_impl
{ 
   BOOST_STATIC_CONSTANT(
       bool, value = (
           ::boost::detail::is_pod_helper<
              ::boost::is_array<T>::value
           >::template result_<T>::value
           )
       );
};

#endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

// the following help compilers without partial specialization support:
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_pod,void,true)

#ifndef BOOST_NO_CV_VOID_SPECIALIZATIONS
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_pod,void const,true)
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_pod,void volatile,true)
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_pod,void const volatile,true)
#endif

} // namespace detail

BOOST_TT_AUX_BOOL_TRAIT_DEF1(is_POD,T,::boost::detail::is_pod_impl<T>::value)
BOOST_TT_AUX_BOOL_TRAIT_DEF1(is_pod,T,::boost::is_POD<T>::value)

} // namespace boost

#include "boost/type_traits/detail/bool_trait_undef.hpp"

#endif // BOOST_TT_IS_POD_HPP_INCLUDED
