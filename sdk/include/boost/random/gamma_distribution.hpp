/* boost random/gamma_distribution.hpp header file
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
 * $Id: gamma_distribution.hpp,v 1.7 2002/12/22 22:03:10 jmaurer Exp $
 *
 */

#ifndef BOOST_RANDOM_GAMMA_DISTRIBUTION_HPP
#define BOOST_RANDOM_GAMMA_DISTRIBUTION_HPP

#include <cmath>
#include <cassert>
#include <boost/random/uniform_01.hpp>

namespace boost {

// Knuth
// deterministic polar method, uses trigonometric functions
template<class UniformRandomNumberGenerator, class RealType = double,
         class Adaptor = uniform_01<UniformRandomNumberGenerator, RealType> >
class gamma_distribution
{
public:
  typedef Adaptor adaptor_type;
  typedef UniformRandomNumberGenerator base_type;
  typedef RealType result_type;

  explicit gamma_distribution(base_type & rng,
                              const result_type& alpha = result_type(1))
    : _rng(rng), _exp(rng, result_type(1)), _alpha(alpha)
  {
    assert(alpha > result_type(0));
    init();
  }

  // compiler-generated copy ctor and assignment operator are fine

  adaptor_type& adaptor() { return _rng; }
  base_type& base() const { return _rng.base(); }
  RealType alpha() const { return _alpha; }

  void reset() { _rng.reset(); _exp.reset(); }

  result_type operator()()
  {
#ifndef BOOST_NO_STDC_NAMESPACE
    // allow for Koenig lookup
    using std::tan; using std::sqrt; using std::exp; using std::log;
    using std::pow;
#endif
    if(_alpha == result_type(1)) {
      return _exp();
    } else if(_alpha > result_type(1)) {
      // Can we have a boost::mathconst please?
      const result_type pi = result_type(3.14159265358979323846);
      for(;;) {
        result_type y = tan(pi * _rng());
        result_type x = sqrt(result_type(2)*_alpha-result_type(1))*y
          + _alpha-result_type(1);
        if(x <= result_type(0))
          continue;
        if(_rng() >
           (result_type(1)+y*y) * exp((_alpha-result_type(1))
                                        *log(x/(_alpha-result_type(1)))
                                        - sqrt(result_type(2)*_alpha
                                               -result_type(1))*y))
          continue;
        return x;
      }
    } else /* alpha < 1.0 */ {
      for(;;) {
        result_type u = _rng();
        result_type y = _exp();
        result_type x, q;
        if(u < _p) {
          x = exp(-y/_alpha);
          q = _p*exp(-x);
        } else {
          x = result_type(1)+y;
          q = _p + (result_type(1)-_p) * pow(x, _alpha-result_type(1));
        }
        if(u >= q)
          continue;
        return x;
      }
    }
  }
#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
  friend bool operator==(const gamma_distribution& x, 
                         const gamma_distribution& y)
  {
    return x._alpha == y._alpha && x._rng == y._rng;
  }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const gamma_distribution& gd)
  {
    os << gd._alpha;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, gamma_distribution& gd)
  {
    is >> std::ws >> gd._alpha;
    gd.init();
    return is;
  }
#endif

#else
  // Use a member function
  bool operator==(const gamma_distribution& rhs) const
  {
    return _alpha == rhs._alpha && _rng == rhs._rng;
  }
#endif

private:
  void init()
  {
#ifndef BOOST_NO_STDC_NAMESPACE
    // allow for Koenig lookup
    using std::exp;
#endif
    _p = exp(result_type(1)) / (_alpha + exp(result_type(1)));
  }

  adaptor_type _rng;
  exponential_distribution<base_type, RealType, Adaptor> _exp;
  result_type _alpha;
  // some data precomputed from the parameters
  result_type _p;
};

} // namespace boost

#endif // BOOST_RANDOM_GAMMA_DISTRIBUTION_HPP
