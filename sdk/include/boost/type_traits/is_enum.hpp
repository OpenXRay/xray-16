
// (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
// Hinnant & John Maddock 2000.  Permission to copy, use, modify,
// sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_IS_ENUM_HPP_INCLUDED
#define BOOST_TT_IS_ENUM_HPP_INCLUDED

#include "boost/type_traits/add_reference.hpp"
#include "boost/type_traits/is_arithmetic.hpp"
#include "boost/type_traits/is_reference.hpp"
#include "boost/type_traits/is_convertible.hpp"
#include "boost/type_traits/config.hpp"

#ifdef BOOST_TT_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION
#   include "boost/type_traits/is_class.hpp"
#endif

// should be the last #include
#include "boost/type_traits/detail/bool_trait_def.hpp"

namespace boost {

#if !(defined(__BORLANDC__) && (__BORLANDC__ <= 0x551))

namespace detail {
  
struct int_convertible
{
    int_convertible(int);
};

// Don't evaluate convertibility to int_convertible unless the type
// is non-arithmetic. This suppresses warnings with GCC.
template <bool is_typename_arithmetic_or_reference = true>
struct is_enum_helper
{
    template <typename T> struct type
    {
        BOOST_STATIC_CONSTANT(bool, value = false);
    };
};

template <>
struct is_enum_helper<false>
{
    template <typename T> struct type
        : ::boost::is_convertible<T,::boost::detail::int_convertible>
    {
    };
};

template <typename T> struct is_enum_impl
{
   typedef ::boost::add_reference<T> ar_t;
   typedef typename ar_t::type r_type;
       
#if defined(BOOST_TT_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION)
   BOOST_STATIC_CONSTANT(bool, selector =
      (::boost::type_traits::ice_or<
           ::boost::is_arithmetic<T>::value
         , ::boost::is_reference<T>::value
       // We MUST do this on conforming compilers in order to
       // correctly deduce that noncopyable types are not enums (dwa
       // 2002/04/15)...
         , ::boost::is_class<T>::value
      >::value));
#else 
   BOOST_STATIC_CONSTANT(bool, selector =
      (::boost::type_traits::ice_or<
           ::boost::is_arithmetic<T>::value
         , ::boost::is_reference<T>::value
       // However, not doing this on non-conforming compilers prevents
       // a dependency recursion.
      >::value));
#endif

#ifdef __BORLANDC__
    typedef ::boost::detail::is_enum_helper<
          ::boost::detail::is_enum_impl<T>::selector
        > se_t;
#else
    typedef ::boost::detail::is_enum_helper<selector> se_t;
#endif
    typedef typename se_t::template type<r_type> helper;
    BOOST_STATIC_CONSTANT(bool, value = helper::value);
};

// Specializations suppress some nasty warnings with GCC
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_enum,float,false)
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_enum,double,false)
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_enum,long double,false)
// these help on compilers with no partial specialization support:
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_enum,void,false)
#ifndef BOOST_NO_CV_VOID_SPECIALIZATIONS
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_enum,void const,false)
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_enum,void volatile,false)
BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(is_enum,void const volatile,false)
#endif

} // namespace detail

BOOST_TT_AUX_BOOL_TRAIT_DEF1(is_enum,T,::boost::detail::is_enum_impl<T>::value)

#else // __BORLANDC__
//
// buggy is_convertible prevents working 
// implementation of is_enum:
BOOST_TT_AUX_BOOL_TRAIT_DEF1(is_enum,T,false)

#endif

} // namespace boost

#include "boost/type_traits/detail/bool_trait_undef.hpp"

#endif // BOOST_TT_IS_ENUM_HPP_INCLUDED
