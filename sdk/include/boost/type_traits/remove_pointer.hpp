
// (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. This software 
// is provided "as is" without express or implied warranty, and with no claim 
// as to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_REMOVE_POINTER_HPP_INCLUDED
#define BOOST_TT_REMOVE_POINTER_HPP_INCLUDED

#include "boost/type_traits/broken_compiler_spec.hpp"
#include "boost/config.hpp"

// should be the last #include
#include "boost/type_traits/detail/type_trait_def.hpp"

namespace boost {

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

BOOST_TT_AUX_TYPE_TRAIT_DEF1(remove_pointer,T,T)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T,remove_pointer,T*,T)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T,remove_pointer,T* const,T)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T,remove_pointer,T* volatile,T)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T,remove_pointer,T* const volatile,T)

#else

BOOST_TT_AUX_TYPE_TRAIT_DEF1(remove_pointer,T,typename detail::remove_pointer_impl<T>::type)

#endif

} // namespace boost

#include "boost/type_traits/detail/type_trait_undef.hpp"

#endif // BOOST_TT_REMOVE_POINTER_HPP_INCLUDED
