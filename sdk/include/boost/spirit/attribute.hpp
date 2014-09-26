/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_ATTRIBUTE_MAIN_HPP)
#define BOOST_SPIRIT_ATTRIBUTE_MAIN_HPP

///////////////////////////////////////////////////////////////////////////////
//
//  Master header for Spirit.Attributes
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  Phoenix predefined maximum limit. This limit defines the maximum
//  number of elements a tuple can hold. This number defaults to 3. The
//  actual maximum is rounded up in multiples of 3. Thus, if this value
//  is 4, the actual limit is 6. The ultimate maximum limit in this
//  implementation is 15.
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(PHOENIX_LIMIT)
#define PHOENIX_LIMIT 3
#endif // !defined(PHOENIX_LIMIT)

///////////////////////////////////////////////////////////////////////////////
#include "boost/spirit/attribute/parametric.hpp"
#include "boost/spirit/attribute/closure.hpp"

#endif // !defined(BOOST_SPIRIT_ATTRIBUTE_MAIN_HPP)
