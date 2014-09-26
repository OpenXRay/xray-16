
// (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. This software 
// is provided "as is" without express or implied warranty, and with no claim 
// as to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_REMOVE_REFERENCE_HPP_INCLUDED
#define BOOST_TT_REMOVE_REFERENCE_HPP_INCLUDED

#include "boost/type_traits/broken_compiler_spec.hpp"
#include "boost/config.hpp"

// should be the last #include
#include "boost/type_traits/detail/type_trait_def.hpp"

namespace boost {

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

BOOST_TT_AUX_TYPE_TRAIT_DEF1(remove_reference,T,T)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T,remove_reference,T&,T)

#if defined(__BORLANDC__)
// these are illegal specialisations; cv-qualifies applied to
// references have no effect according to [8.3.2p1],
// C++ Builder requires them though as it treats cv-qualified
// references as distinct types...
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T,remove_reference,T& const,T)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T,remove_reference,T& volatile,T)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T,remove_reference,T& const volatile,T)
#endif

#else

BOOST_TT_AUX_TYPE_TRAIT_DEF1(remove_reference,T,typename detail::remove_reference_impl<T>::type)

#endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

} // namespace boost

#include "boost/type_traits/detail/type_trait_undef.hpp"

#endif // BOOST_TT_REMOVE_REFERENCE_HPP_INCLUDED
