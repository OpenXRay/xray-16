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

#ifndef ITERATOR_RG071801_HPP
#define ITERATOR_RG071801_HPP

//
// iterator.hpp - implementation of iterators for the
// multi-dimensional array class
//

#include "boost/multi_array/base.hpp"
#include "boost/multi_array/iterator_adaptors.hpp"
#include "boost/iterator_adaptors.hpp"
#include <cstddef>
#include <iterator>

namespace boost {
namespace detail {
namespace multi_array {

/////////////////////////////////////////////////////////////////////////
// iterator components
/////////////////////////////////////////////////////////////////////////

template <typename T, typename TPtr>
struct iterator_base : private multi_array_base {
  typedef multi_array_base super_type;
  typedef super_type::index index;
  typedef super_type::size_type size_type;

  index idx_;
  TPtr base_;
  const size_type* extents_;
  const index* strides_;
  const index* index_base_;

  iterator_base(int idx, TPtr base, const size_type* extents,
                const index* strides,
                const index* index_base) :
    idx_(idx), base_(base), extents_(extents),
    strides_(strides), index_base_(index_base) {
  }

  template <typename OPtr>
  iterator_base(const iterator_base<T,OPtr>& rhs) :
    idx_(rhs.idx_), base_(rhs.base_), extents_(rhs.extents_),
    strides_(rhs.strides_), index_base_(rhs.index_base_) {
  }

  // default constructor required
  iterator_base() {}
};

template<typename T, std::size_t NumDims>
struct iterator_policies :
  public boost::detail::multi_array::default_iterator_policies,
  private value_accessor_generator<T,NumDims>::type {
private:
  typedef typename value_accessor_generator<T,NumDims>::type super_type;
public:
  template <class IteratorAdaptor>
  typename IteratorAdaptor::reference
  dereference(const IteratorAdaptor& iter) const {
    typedef typename IteratorAdaptor::reference reference;
    return super_type::access(boost::type<reference>(),
                              iter.base().idx_,
                              iter.base().base_,
                              iter.base().extents_,
                              iter.base().strides_,
                              iter.base().index_base_);
  }
  
  template <class IteratorAdaptor>
  static void increment(IteratorAdaptor& x) { ++x.base().idx_; }

  template <class IteratorAdaptor>
  static void decrement(IteratorAdaptor& x) { --x.base().idx_; }

  template <class IteratorAdaptor1, class IteratorAdaptor2>
  bool equal(IteratorAdaptor1& lhs, IteratorAdaptor2& rhs) const {
    return (lhs.base().idx_ == rhs.base().idx_) &&
      (lhs.base().base_ == rhs.base().base_) &&
      (lhs.base().extents_ == rhs.base().extents_) &&
      (lhs.base().strides_ == rhs.base().strides_) &&
      (lhs.base().index_base_ == rhs.base().index_base_);
  }

  template <class IteratorAdaptor, class DifferenceType>
  static void advance(IteratorAdaptor& x, DifferenceType n) {
    x.base().idx_ += n;
  }

  template <class IteratorAdaptor1, class IteratorAdaptor2>
  typename IteratorAdaptor1::difference_type
  distance(IteratorAdaptor1& lhs, IteratorAdaptor2& rhs) const {
    return rhs.base().idx_ - lhs.base().idx_;
  }
};


template <typename T, typename base_type,
  std::size_t NumDims, typename value_type,
  typename reference_type, typename tag, typename difference_type>
struct iterator_gen_helper {
private:
  typedef iterator_policies<T,NumDims> policies;
  typedef value_type* pointer_type;
  typedef tag category;
public:
  typedef boost::detail::multi_array::iterator_adaptor<base_type,policies,value_type,
    reference_type,pointer_type,category,difference_type> type;
};


template <typename T, std::size_t NumDims, typename value_type,
  typename reference_type, typename tag, typename difference_type>
struct iterator_generator {
private:
  typedef iterator_base<T,T*> base_type;
public:
  typedef typename iterator_gen_helper<T,base_type,NumDims,value_type,
    reference_type,tag,difference_type>::type type;
};

template <typename T,  std::size_t NumDims, typename value_type,
  typename reference_type, typename tag, typename difference_type>
struct const_iterator_generator {
private:
  typedef iterator_base<T,const T*> base_type;
public:
  typedef typename iterator_gen_helper<T,base_type,NumDims,value_type,
    reference_type,tag,difference_type>::type type;
};

template <typename T, std::size_t NumDims, typename value_type,
  typename reference_type, typename tag, typename difference_type>
struct reverse_iterator_generator {
private:
  typedef iterator_base<T,T*> base_type;
  typedef typename iterator_gen_helper<T,base_type,NumDims,value_type,
    reference_type,tag,difference_type>::type it_type;
public:
  typedef typename boost::reverse_iterator_generator<it_type>::type type;
};

template <typename T,  std::size_t NumDims, typename value_type,
  typename reference_type, typename tag, typename difference_type>
struct const_reverse_iterator_generator {
private:
  typedef iterator_base<T,const T*> base_type;
  typedef typename iterator_gen_helper<T,base_type,NumDims,value_type,
    reference_type,tag,difference_type>::type it_type;
public:
  typedef typename boost::reverse_iterator_generator<it_type>::type type;
};

} // namespace multi_array
} // namespace detail
} // namespace boost

#endif // ITERATOR_RG071801_HPP
