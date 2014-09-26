/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2002 Jeff Westfahl
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_ITERATOR_MAIN_HPP)
#define BOOST_SPIRIT_ITERATOR_MAIN_HPP

///////////////////////////////////////////////////////////////////////////////
//
//  Master header for Spirit.Iterators
//
///////////////////////////////////////////////////////////////////////////////

#include "boost/spirit/iterator/file_iterator.hpp"
#include "boost/spirit/iterator/fixed_size_queue.hpp"
#include "boost/spirit/iterator/position_iterator.hpp"

#include "boost/config.hpp"
#if !defined(BOOST_MSVC) || (BOOST_MSVC > 1300)
#include "boost/spirit/iterator/multi_pass.hpp"
#endif

#endif // !defined(BOOST_SPIRIT_ITERATOR_MAIN_HPP)
