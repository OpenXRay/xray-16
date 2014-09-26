// Boost.Signals library
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

#ifndef BOOST_VISIT_EACH_HPP
#define BOOST_VISIT_EACH_HPP

#include <boost/config.hpp>

namespace boost {
  template<typename Visitor, typename T>
  inline void visit_each(Visitor& visitor, const T& t, long)
  {
    visitor(t);
  }

  template<typename Visitor, typename T>
  inline void visit_each(Visitor& visitor, const T& t)
  {
    visit_each(visitor, t, 0);
  }
}

#endif // BOOST_VISIT_EACH_HPP
