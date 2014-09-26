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

#ifndef BOOST_INDEX_GEN_RG071801_HPP
#define BOOST_INDEX_GEN_RG071801_HPP

#include "boost/array.hpp"
#include "boost/multi_array/index_range.hpp"
#include "boost/multi_array/range_list.hpp"
#include "boost/multi_array/types.hpp"
#include <algorithm> 
#include <cstddef>

namespace boost {
namespace detail {
namespace multi_array {


template <int NumRanges, int NumDims>
struct index_gen {
private:
  typedef index Index;
  typedef std::size_t SizeType;
  typedef index_range<Index,SizeType> range;
public:
  template <int Dims, int Ranges>
  struct gen_type {
    typedef index_gen<Ranges,Dims> type;
  };

  typedef typename range_list_generator<range,NumRanges>::type range_list;
  range_list ranges_;

  index_gen() { }

  template <int ND>
  explicit index_gen(const index_gen<NumRanges-1,ND>& rhs,
            const index_range<Index,SizeType>& range)
  {
    std::copy(rhs.ranges_.begin(),rhs.ranges_.end(),ranges_.begin());
    *ranges_.rbegin() = range;
  }

  index_gen<NumRanges+1,NumDims+1>
  operator[](const index_range<Index,SizeType>& range) const
  {
    index_gen<NumRanges+1,NumDims+1> tmp;
    std::copy(ranges_.begin(),ranges_.end(),tmp.ranges_.begin());
    *tmp.ranges_.rbegin() = range;
    return tmp;
  }

  index_gen<NumRanges+1,NumDims>
  operator[](Index idx) const
  {
    index_gen<NumRanges+1,NumDims> tmp;
    std::copy(ranges_.begin(),ranges_.end(),tmp.ranges_.begin());
    *tmp.ranges_.rbegin() = index_range<Index,SizeType>(idx);
    return tmp;
  }    

  static index_gen<0,0> indices() {
    return index_gen<0,0>();
  }
};

} // namespace multi_array
} // namespace detail
} // namespace boost


#endif // BOOST_INDEX_GEN_RG071801_HPP
