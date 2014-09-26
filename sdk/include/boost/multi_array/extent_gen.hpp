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

#ifndef BOOST_EXTENT_GEN_RG071801_HPP
#define BOOST_EXTENT_GEN_RG071801_HPP

#include "boost/multi_array/extent_range.hpp"
#include "boost/multi_array/range_list.hpp"
#include "boost/multi_array/types.hpp"
#include "boost/array.hpp"
#include <algorithm>

namespace boost {
namespace detail {
namespace multi_array {


template <std::size_t NumRanges>
class extent_gen {
public:
  typedef boost::detail::multi_array::index index;
  typedef boost::detail::multi_array::size_type size_type;
private:
  typedef extent_range<index,size_type> range;
  typedef typename range_list_generator<range,NumRanges>::type range_list;
public:
  template <std::size_t Ranges>
  struct gen_type {
    typedef extent_gen<Ranges> type;
  };

  range_list ranges_;

  extent_gen() { }

  // Used by operator[] to expand extent_gens
  extent_gen(const extent_gen<NumRanges-1>& rhs,
            const range& a_range)
  {
    std::copy(rhs.ranges_.begin(),rhs.ranges_.end(),ranges_.begin());
    *ranges_.rbegin() = a_range;
  }

  extent_gen<NumRanges+1>
  operator[](const range& a_range)
  {
    return extent_gen<NumRanges+1>(*this,a_range);    
  }

  extent_gen<NumRanges+1>
  operator[](index idx)
  {
    return extent_gen<NumRanges+1>(*this,range(0,idx));    
  }    

  static extent_gen<0> extents() {
    return extent_gen<0>();
  }
};

} // namespace multi_array
} // namespace detail
} // namespace boost


#endif // BOOST_EXTENT_GEN_RG071801_HPP
