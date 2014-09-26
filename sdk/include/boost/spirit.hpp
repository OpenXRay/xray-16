/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2001-2003 Hartmut Kaiser
    Copyright (c) 2002-2003 Martin Wille
    Copyright (c) 2002 Juan Carlos Arevalo-Baeza
    Copyright (c) 2002 Raghavendra Satish
    Copyright (c) 2002 Jeff Westfahl
    Copyright (c) 2001 Bruce Florman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(SPIRIT_HPP)
#define SPIRIT_HPP

#define SPIRIT_VERSION 0x1502
#define SPIRIT_PIZZA_VERSION SPIRIT_PIZZA_EVERYTHING_EXCEPT_FISH  // :-)

///////////////////////////////////////////////////////////////////////////////
//
//  If SPIRIT_DEBUG is defined, the following header includes the
//  Spirit.Debug layer, otherwise the non-debug Spirit.Core is included.
//
///////////////////////////////////////////////////////////////////////////////
#include "boost/spirit/core.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit.ErrorHandling
//
///////////////////////////////////////////////////////////////////////////////
#include "boost/spirit/error_handling.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit.Iterators
//
///////////////////////////////////////////////////////////////////////////////
#include "boost/spirit/iterator.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit.Symbols
//
///////////////////////////////////////////////////////////////////////////////
#include "boost/spirit/symbols.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit.Utilities
//
///////////////////////////////////////////////////////////////////////////////
#include "boost/spirit/utility.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit.Attributes
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_MSVC) || (BOOST_MSVC > 1300)
#include "boost/spirit/attribute.hpp"
#endif

#endif // !defined(SPIRIT_HPP)
