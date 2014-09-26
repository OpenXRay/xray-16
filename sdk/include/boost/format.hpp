// -*- C++ -*-
//  Boost general library 'format'   ---------------------------
//  See http://www.boost.org for updates, documentation, and revision history.

//  (C) Samuel Krempp 2001
//                  krempp@crans.ens-cachan.fr
//  Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// ideas taken from Rüdiger Loos's format class
// and Karl Nelson's ofstream

// ----------------------------------------------------------------------------
// format.hpp :  primary header
// ----------------------------------------------------------------------------

#ifndef BOOST_FORMAT_HPP
#define BOOST_FORMAT_HPP

#include <vector>
#include <string>
#include <sstream>
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#ifndef BOOST_NO_STD_LOCALE
#include <locale>
#endif


// make sure our local macros wont override something :
#if defined(BOOST_NO_LOCALE_ISDIGIT) || defined(BOOST_OVERLOAD_FOR_NON_CONST) \
  || defined(BOOST_IO_STD) || defined( BOOST_IO_NEEDS_USING_DECLARATION )
#error "a local macro would overwrite a previously defined macro"
#endif


#include <boost/format/macros_stlport.hpp>  // stlport workarounds
#include <boost/format/macros_default.hpp> 

#if defined(BOOST_NO_STD_LOCALE) || \
 ( BOOST_WORKAROUND(__BORLANDC__, <= 0x564) \
   || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT( 0x570 ) )  )
// some future __BORLANDC__ >0x564  versions might not need this
// 0x570 is Borland's kylix branch
#define BOOST_NO_LOCALE_ISIDIGIT
#endif

#ifdef BOOST_NO_LOCALE_ISIDIGIT
#include <cctype>  // we'll use the non-locale  <cctype>'s std::isdigit(int)
#endif


#if  BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x570) ) || BOOST_WORKAROUND( BOOST_MSVC, BOOST_TESTED_AT(1300))
#define BOOST_NO_OVERLOAD_FOR_NON_CONST
#endif


// ****  Forward declarations ----------------------------------
#include <boost/format/format_fwd.hpp>           // basic_format<Ch,Tr>, and other frontends
#include <boost/format/internals_fwd.hpp>        // misc forward declarations for internal use


// ****  Auxiliary structs (stream_format_state<Ch,Tr> , and format_item<Ch,Tr> )
#include <boost/format/internals.hpp>    

// ****  Format  class  interface --------------------------------
#include <boost/format/format_class.hpp>

// **** Exceptions -----------------------------------------------
#include <boost/format/exceptions.hpp>

// **** Implementation -------------------------------------------
#include <boost/format/format_implementation.hpp>   // member functions

#include <boost/format/group.hpp>                   // class for grouping arguments

#include <boost/format/feed_args.hpp>               // argument-feeding functions
#include <boost/format/parsing.hpp>                 // format-string parsing (member-)functions

// **** Implementation of the free functions ----------------------
#include <boost/format/free_funcs.hpp>


// *** Undefine 'local' macros :
#ifdef BOOST_NO_OVERLOAD_FOR_NON_CONST
#undef BOOST_NO_OVERLOAD_FOR_NON_CONST
#endif
#ifdef BOOST_NO_LOCALE_ISIDIGIT
#undef BOOST_NO_LOCALE_ISIDIGIT
#endif
#ifdef BOOST_IO_STD
#undef BOOST_IO_STD
#endif
#ifdef BOOST_IO_NEEDS_USING_DECLARATION
#undef BOOST_IO_NEEDS_USING_DECLARATION
#endif

#endif // BOOST_FORMAT_HPP
