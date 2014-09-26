
// (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. This software 
// is provided "as is" without express or implied warranty, and with no claim 
// as to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_CONFIG_HPP_INCLUDED
#define BOOST_TT_CONFIG_HPP_INCLUDED

#ifndef BOOST_CONFIG_HPP
#include "boost/config.hpp"
#endif

//
// whenever we have a conversion function with elipses
// it needs to be declared __cdecl to suppress compiler
// warnings from MS and Borland compilers (this *must*
// appear before we include is_same.hpp below):
#if defined(BOOST_MSVC) || defined(__BORLANDC__)
#   define BOOST_TT_DECL __cdecl
#else
#   define BOOST_TT_DECL /**/
#endif

# if (defined(__MWERKS__) && __MWERKS__ >= 0x3000) || (defined(BOOST_MSVC) && (BOOST_MSVC > 1301)) || defined(BOOST_NO_COMPILER_CONFIG)
#   define BOOST_TT_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION
#endif

#endif // BOOST_TT_CONFIG_HPP_INCLUDED


