#ifndef BOOST_ALGORITHM_RG071801_HPP
#define BOOST_ALGORITHM_RG071801_HPP

//
//
// Copyright (c) 1994
// Hewlett-Packard Company
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Hewlett-Packard Company makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.
//
//
// Copyright (c) 1996-1998
// Silicon Graphics Computer Systems, Inc.
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Silicon Graphics makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.
//
// Copyright (C) 2002 Ronald Garcia
//
// Permission to copy, use, sell and distribute this software is granted
// provided this copyright notice appears in all copies. 
// Permission to modify the code and to distribute modified code is granted
// provided this copyright notice appears in all copies, and a notice 
// that the code was modified is included with the copyright notice.
//
// This software is provided "as is" without express or implied warranty, 
// and with no claim as to its suitability for any purpose.
//


#include "boost/iterator.hpp"

namespace boost {

//--------------------------------------------------
// copy_n (not part of the C++ standard)
#if 1

template <class InputIter, class Size, class OutputIter>
OutputIter copy_n(InputIter first, Size count,
                  OutputIter result) {
  for ( ; count > 0; --count) {
    *result = *first;
    ++first;
    ++result;
  }
  return result;
}
#else // !1

template <class InputIter, class Size, class OutputIter>
OutputIter copy_n__(InputIter first, Size count,
                                       OutputIter result,
                                       std::input_iterator_tag) {
  for ( ; count > 0; --count) {
    *result = *first;
    ++first;
    ++result;
  }
  return result;
}

template <class RAIter, class Size, class OutputIter>
inline OutputIter
copy_n__(RAIter first, Size count,
         OutputIter result,
         std::random_access_iterator_tag) {
  RAIter last = first + count;
  return std::copy(first, last, result);
}

template <class InputIter, class Size, class OutputIter>
inline OutputIter
copy_n__(InputIter first, Size count, OutputIter result) {
  typedef typename std::iterator_traits<InputIter>::iterator_category cat;
  return copy_n__(first, count, result, cat());
}

template <class InputIter, class Size, class OutputIter>
inline OutputIter
copy_n(InputIter first, Size count, OutputIter result) {
  return copy_n__(first, count, result);
}

#endif // 1

} // namespace boost

#endif // BOOST_ALGORITHM_RG071801_HPP
