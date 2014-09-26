/* boost random/binomial_distribution.hpp header file
 *
 * Copyright Jens Maurer 2002
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
 * $Id: binomial_distribution.hpp,v 1.7 2002/12/22 22:03:10 jmaurer Exp $
 *
 */

#ifndef BOOST_RANDOM_BINOMIAL_DISTRIBUTION_HPP
#define BOOST_RANDOM_BINOMIAL_DISTRIBUTION_HPP

#include <cmath>
#include <cassert>
#include <boost/random/uniform_01.hpp>

namespace boost {

// Knuth
template<class UniformRandomNumberGenerator, class IntType = int,
         class RealType = double,
         class Adaptor = bernoulli_distribution<UniformRandomNumberGenerator, RealType> >
class binomial_distribution
{
public:
  typedef Adaptor adaptor_type;
  typedef UniformRandomNumberGenerator base_type;
  typedef IntType result_type;

  explicit binomial_distribution(base_type & rng, IntType t = 1,
                                 const RealType& p = RealType(0.5))
    : _rng(rng, p), _t(t)
  {
    assert(t >= 0);
    assert(RealType(0) <= 0 && p <= RealType(1));
  }

  // compiler-generated copy ctor and assignment operator are fine

  adaptor_type& adaptor() { return _rng; }
  base_type& base() const { return _rng.base(); }
  IntType t() const { return _t; }
  RealType p() const { return _rng.p(); }
  void reset() { _rng.reset(); }

  result_type operator()()
  {
    // TODO: This is O(_t), but it should be O(log(_t)) for large _t
    result_type n = 0;
    for(IntType i = 0; i < _t; ++i)
      if(_rng())
        ++n;
    return n;
  }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
  friend bool operator==(const binomial_distribution& x, 
                         const binomial_distribution& y)
  {
    return x._t == y._t && x._rng == y._rng;
  }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const binomial_distribution& bd)
  {
    os << bd._rng << " " << bd._t;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, binomial_distribution& bd)
  {
    is >> std::ws >> bd._rng >> std::ws >> bd._t;
    return is;
  }
#else
  // Use a member function
  bool operator==(const binomial_distribution& rhs) const
  {
    return _t == rhs._t && _rng == rhs._rng;
  }
#endif

#endif
private:
  adaptor_type _rng;
  IntType _t;
};

} // namespace boost

#endif // BOOST_RANDOM_BINOMIAL_DISTRIBUTION_HPP
