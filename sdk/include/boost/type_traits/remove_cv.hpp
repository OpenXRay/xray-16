
// (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
// Hinnant & John Maddock 2000.  Permission to copy, use, modify,
// sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_REMOVE_CV_HPP_INCLUDED
#define BOOST_TT_REMOVE_CV_HPP_INCLUDED

#include "boost/type_traits/broken_compiler_spec.hpp"
#include "boost/type_traits/detail/cv_traits_impl.hpp"
#include "boost/config.hpp"

#include <cstddef>

// should be the last #include
#include "boost/type_traits/detail/type_trait_def.hpp"

namespace boost {

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

//  convert a type T to a non-cv-qualified type - remove_cv<T>
BOOST_TT_AUX_TYPE_TRAIT_DEF1(remove_cv,T,typename detail::cv_traits_imp<T*>::unqualified_type)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T,remove_cv,T&,T&)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T,std::size_t N,remove_cv,T const[N],T type[N])
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T,std::size_t N,remove_cv,T volatile[N],T type[N])
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T,std::size_t N,remove_cv,T const volatile[N],T type[N])

#else

namespace detail {
template <typename T>
struct remove_cv_impl
{
    typedef typename remove_volatile_impl< 
          typename remove_const_impl<T>::type
        >::type type;
};
}

BOOST_TT_AUX_TYPE_TRAIT_DEF1(remove_cv,T,typename detail::remove_cv_impl<T>::type)

#endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

} // namespace boost

#include "boost/type_traits/detail/type_trait_undef.hpp"

#endif // BOOST_TT_REMOVE_CV_HPP_INCLUDED
