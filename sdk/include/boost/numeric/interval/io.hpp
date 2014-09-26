/* Boost interval/io.hpp template implementation file
 *
 * Copyright Jens Maurer 2000
 * Copyright Hervé Brönnimann, Guillaume Melquiond, Sylvain Pion 2002
 * Permission to use, copy, modify, sell, and distribute this software
 * is hereby granted without fee provided that the above copyright notice
 * appears in all copies and that both that copyright notice and this
 * permission notice appear in supporting documentation,
 *
 * None of the above authors nor Polytechnic University make any
 * representation about the suitability of this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 *
 * $Id: io.hpp,v 1.2 2003/02/05 17:34:30 gmelquio Exp $
 */

#ifndef BOOST_NUMERIC_INTERVAL_IO_HPP
#define BOOST_NUMERIC_INTERVAL_IO_HPP

#include <iostream>
#include <string>        // for non-explicit string constructor
// #include <locale>
#include <boost/numeric/interval.hpp>


namespace boost {
namespace numeric {

/*
 * Input/Output
 */

#if 0
template<class Ch, class ChTr, class T, class Policies>
std::basic_ostream<Ch, ChTr>&
operator<<(std::basic_ostream<Ch, ChTr>& os, const interval<T, Policies>& r)
{
  typename std::basic_ostream<Ch, ChTr>::sentry sentry(os);
  if (sentry) {
    T l = r.lower(), u = r.upper();
    os << '[' << l << ',' << u << ']';
  }
  return os;
}
#else
// Ensure generation of outer bounds in all cases
template<class T, class Policies> inline
std::ostream& operator<<(std::ostream& os, const interval<T, Policies>& r)
{
  std::streamsize p = os.precision(); // decimal notation
  // FIXME poor man's power of 10, only up to 1E-15
  p = (p > 15) ? 15 : p - 1;
  double eps = 1.0; while (p>0) { eps /= 10.0; --p; }
  // widen the interval so output is correct
  interval<T, Policies> r_wide = widen(r, static_cast<T>(eps / 2.0));
  os << "[" << r_wide.lower() << "," << r_wide.upper() << "]";
  return os;
}
#endif


#if 0 // FIXME: make the code work for g++ 3.*
template<class Ch, class ChTr, class T, class Policies>
std::basic_istream<Ch, ChTr>&
operator>>(std::basic_istream<Ch, ChTr>& is, const interval<T, Policies>& r)
{
  T l = 0, u = 0;
  std::locale loc = is.getloc();
  const std::ctype<Ch> c_type = std::use_facet<c_type<Ch> >(loc);
  const char punct[] = "[,]";
  Ch wpunct[3];
  c_type.widen(punct, punct+3, wpunct);
  Ch c;
  is >> c;
  if (ChTr::eq(c, wpunct[0])) {
    is >> l >> c;
    if (ChTr::eq(c, wpunct[1]))
      is >> u >> c;
    if (!ChTr::eq(c, wpunct[2]))
      is.setstate(is.failbit);
  } else {
    is.putback(c);
    is >> l;
    u = l;
  }
  if (is)
    r = succ(interval<T, Policies>(l, r));   // round outward by 1 ulp
  return is;
}
#else
template<class T, class Policies> inline
std::istream& operator>>(std::istream& is, interval<T, Policies>& r)
{
  T l, u;
  char c = 0;
  // we assume that l and u are representable numbers
  if(is.peek() == '[') {
    is >> c;
    is >> l;
    if(is.peek() == ',') {
      is >> c;
      is >> u;
      is >> c;
    }
  }
  if (c != ']')
    is.setstate(is.failbit);
  else if (is.good())
    r.assign(l, u);
  return is;
}
#endif

} // namespace numeric
} // namespace boost

#endif // BOOST_NUMERIC_INTERVAL_IO_HPP
