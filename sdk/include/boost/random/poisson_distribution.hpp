/* boost random/poisson_distribution.hpp header file
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
 * $Id: poisson_distribution.hpp,v 1.9 2002/12/22 22:03:10 jmaurer Exp $
 *
 */

#ifndef BOOST_RANDOM_POISSON_DISTRIBUTION_HPP
#define BOOST_RANDOM_POISSON_DISTRIBUTION_HPP

#include <cmath>
#include <cassert>
#include <boost/random/uniform_01.hpp>

namespace boost {

// Knuth
template<class UniformRandomNumberGenerator, class IntType = int,
         class RealType = double,
         class Adaptor = uniform_01<UniformRandomNumberGenerator, RealType> >
class poisson_distribution
{
public:
  typedef Adaptor adaptor_type;
  typedef UniformRandomNumberGenerator base_type;
  typedef IntType result_type;

  explicit poisson_distribution(base_type & rng,
                                const RealType& mean = RealType(1))
    : _rng(rng), _mean(mean)
  {
    assert(mean > RealType(0));
    init();
  }

  // compiler-generated copy ctor and assignment operator are fine

  adaptor_type& adaptor() { return _rng; }
  base_type& base() const { return _rng.base(); }
  RealType mean() const { return _mean; }
  void reset() { _rng.reset(); }

  result_type operator()()
  {
    // TODO: This is O(_mean), but it should be O(log(_mean)) for large _mean
    RealType product = RealType(1);
    for(int m = 0; ; ++m) {
      product *= _rng();
      if(product <= _exp_mean)
        return m;
    }
  }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
  friend bool operator==(const poisson_distribution& x, 
                         const poisson_distribution& y)
  {
    return x._mean == y._mean && x._rng == y._rng;
  }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const poisson_distribution& pd)
  {
    os << pd._mean;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, poisson_distribution& pd)
  {
    is >> std::ws >> pd._mean;
    pd.init();
    return is;
  }
#endif

#else
  // Use a member function
  bool operator==(const poisson_distribution& rhs) const
  {
    return _mean == rhs._mean && _rng == rhs._rng;
  }
#endif

private:
  void init()
  {
#ifndef BOOST_NO_STDC_NAMESPACE
    // allow for Koenig lookup
    using std::exp;
#endif
    _exp_mean = exp(-_mean);
  }

  adaptor_type _rng;
  RealType _mean;
  // some precomputed data from the parameters
  RealType _exp_mean;
};

} // namespace boost

#endif // BOOST_RANDOM_POISSON_DISTRIBUTION_HPP
