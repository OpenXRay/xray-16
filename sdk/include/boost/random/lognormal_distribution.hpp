/* boost random/lognormal_distribution.hpp header file
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
 * $Id: lognormal_distribution.hpp,v 1.13 2003/02/25 10:29:29 bjorn_karlsson Exp $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_LOGNORMAL_DISTRIBUTION_HPP
#define BOOST_RANDOM_LOGNORMAL_DISTRIBUTION_HPP

#include <cmath>      // std::exp, std::sqrt
#include <cassert>
#include <boost/random/normal_distribution.hpp>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
  using ::log;
  using ::sqrt;
}
#endif

namespace boost {

#if defined(__GNUC__) && (__GNUC__ < 3)
// Special gcc workaround: gcc 2.95.x ignores using-declarations
// in template classes (confirmed by gcc author Martin v. Loewis)
  using std::sqrt;
  using std::exp;
#endif

template<class UniformRandomNumberGenerator, class RealType = double,
        class Adaptor = uniform_01<UniformRandomNumberGenerator, RealType> >
class lognormal_distribution
{
public:
  typedef Adaptor adaptor_type;
  typedef UniformRandomNumberGenerator base_type;
  typedef RealType result_type;

  explicit lognormal_distribution(base_type & rng,
                                  result_type mean = result_type(1),
                                  result_type sigma = result_type(1))
    : _mean(mean), _sigma(sigma),
      _rng(rng, std::log(mean*mean/std::sqrt(sigma*sigma + mean*mean)),
           std::sqrt(std::log(sigma*sigma/mean/mean+result_type(1))))
  { 
    assert(mean > result_type(0));
  }

  // compiler-generated copy ctor and assignment operator are fine

  adaptor_type& adaptor() { return _rng.adaptor(); }
  base_type& base() const { return _rng.base(); }
  RealType mean() const { return _mean; }
  RealType sigma() const { return _sigma; }
  void reset() { _rng.reset(); }

  result_type operator()()
  {
#ifndef BOOST_NO_STDC_NAMESPACE
    // allow for Koenig lookup
    using std::exp;
#endif
    return exp(_rng());
  }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
  friend bool operator==(const lognormal_distribution& x, 
                         const lognormal_distribution& y)
  { return x._rng == y._rng; }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const lognormal_distribution& ld)
  {
    os << ld._rng << " " << ld._mean << " " << ld._sigma;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, lognormal_distribution& ld)
  {
    is >> std::ws >> ld._rng >> std::ws >> ld._mean >> std::ws >> ld._sigma;
    return is;
  }
#endif

#else
  // Use a member function
  bool operator==(const lognormal_distribution& rhs) const
  { return _rng == rhs._rng;  }
#endif
private:
  RealType _mean, _sigma;
  normal_distribution<base_type, result_type, adaptor_type> _rng;
};

} // namespace boost

#endif // BOOST_RANDOM_LOGNORMAL_DISTRIBUTION_HPP
