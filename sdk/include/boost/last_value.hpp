// last_value function object (documented as part of Boost.Signals)
//
// Copyright (C) 2001 Doug Gregor (gregod@cs.rpi.edu)
//
// Permission to copy, use, sell and distribute this software is granted
// provided this copyright notice appears in all copies.
// Permission to modify the code and to distribute modified code is granted
// provided this copyright notice appears in all copies, and a notice
// that the code was modified is included with the copyright notice.
//
// This software is provided "as is" without express or implied warranty,
// and with no claim as to its suitability for any purpose.
 
// For more information, see http://www.boost.org/libs/signals

#ifndef BOOST_LAST_VALUE_HPP
#define BOOST_LAST_VALUE_HPP

#include <cassert>

namespace boost {
  template<typename T>
  struct last_value {
    typedef T result_type;
    
    template<typename InputIterator>
    T operator()(InputIterator first, InputIterator last) const
    {
      assert(first != last);
      T value = *first++;
      while (first != last)
        value = *first++;
      return value;
    }
  };
  
  template<>
  struct last_value<void> {
    struct unusable {};

  public:
    typedef unusable result_type;
    
    template<typename InputIterator>
    result_type
    operator()(InputIterator first, InputIterator last) const
    {
      while (first != last)
        *first++;
      return result_type();
    }
  };
}
#endif // BOOST_SIGNALS_LAST_VALUE_HPP
