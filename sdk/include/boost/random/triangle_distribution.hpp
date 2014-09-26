/* boost random/triangle_distribution.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Permission to use, copy, modify, sell, and distribute this software
 * is hereby granted without fee provided that the above copyright notice
 * appears in all copies and that both that copyright notice and this
 * permission notice appear in supporting documentation,
 *
 * Jens Maurer makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: triangle_distribution.hpp,v 1.9 2002/12/22 22:03:11 jmaurer Exp $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_TRIANGLE_DISTRIBUTION_HPP
#define BOOST_RANDOM_TRIANGLE_DISTRIBUTION_HPP

#include <cmath>
#include <cassert>
#include <boost/random/uniform_01.hpp>

namespace boost {

// triangle distribution, with a smallest, b most probable, and c largest
// value.
template<class UniformRandomNumberGenerator, class RealType = double,
        class Adaptor = uniform_01<UniformRandomNumberGenerator, RealType> >
class triangle_distribution
{
public:
  typedef Adaptor adaptor_type;
  typedef UniformRandomNumberGenerator base_type;
  typedef RealType result_type;

  explicit triangle_distribution(base_type & rng,
                                 result_type a = result_type(0),
                                 result_type b = result_type(0.5),
                                 result_type c = result_type(1))
    : _rng(rng), _a(a), _b(b), _c(c)
  {
    assert(_a <= _b && _b <= _c);
    init();
  }

  // compiler-generated copy ctor and assignment operator are fine

  adaptor_type& adaptor() { return _rng; }
  base_type& base() const { return _rng.base(); }
  void reset() { _rng.reset(); }

  result_type operator()()
  {
#ifndef BOOST_NO_STDC_NAMESPACE
    using std::sqrt;
#endif
    result_type u = _rng();
    if( u <= q1 )
      return _a + p1*sqrt(u);
    else
      return _c - d3*sqrt(d2*u-d1);
  }
#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
  friend bool operator==(const triangle_distribution& x, 
                         const triangle_distribution& y)
  { return x._a == y._a && x._b == y._b && x._c == y._c && x._rng == y._rng; }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const triangle_distribution& td)
  {
    os << td._a << " " << td._b << " " << td._c;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, triangle_distribution& td)
  {
    is >> std::ws >> td._a >> std::ws >> td._b >> std::ws >> td._c;
    td.init();
    return is;
  }
#endif

#else
  // Use a member function
  bool operator==(const triangle_distribution& rhs) const
  { return _a == rhs._a && _b == rhs._b && _c == rhs._c && _rng == rhs._rng;  }
#endif

private:
  void init()
  {
#ifndef BOOST_NO_STDC_NAMESPACE
    using std::sqrt;
#endif
    d1 = _b - _a;
    d2 = _c - _a;
    d3 = sqrt(_c - _b);
    q1 = d1 / d2;
    p1 = sqrt(d1 * d2);
  }

  adaptor_type _rng;
  result_type _a, _b, _c;
  result_type d1, d2, d3, q1, p1;
};

} // namespace boost

#endif // BOOST_RANDOM_TRIANGLE_DISTRIBUTION_HPP
