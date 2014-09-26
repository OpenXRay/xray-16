//-----------------------------------------------------------------------------
// boost mpl/aux_/vector/iterator.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2000-02
// Aleksey Gurtovoy
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee, 
// provided that the above copyright notice appears in all copies and 
// that both the copyright notice and this permission notice appear in 
// supporting documentation. No representations are made about the 
// suitability of this software for any purpose. It is provided "as is" 
// without express or implied warranty.

#ifndef BOOST_MPL_AUX_VECTOR_ITERATOR_HPP_INCLUDED
#define BOOST_MPL_AUX_VECTOR_ITERATOR_HPP_INCLUDED

#include "boost/mpl/iterator_tag.hpp"
#include "boost/mpl/plus.hpp"
#include "boost/mpl/minus.hpp"
#include "boost/mpl/aux_/iterator_names.hpp"
#include "boost/mpl/aux_/value_wknd.hpp"
#include "boost/mpl/vector/aux_/item.hpp"

namespace boost {
namespace mpl {

template<
      typename Vector
    , typename Pos
    >
struct vector_iterator
{
    typedef ra_iter_tag_ category;
    typedef typename vector_item<
          Vector
        , BOOST_MPL_AUX_VALUE_WKND(Pos)::value
        >::type type;

    typedef Pos pos;
    typedef vector_iterator<Vector,typename Pos::next> next;
    typedef vector_iterator<Vector,typename Pos::prior> prior;

    template< typename Distance >
    struct BOOST_MPL_AUX_ITERATOR_ADVANCE
    {
        typedef vector_iterator<
              Vector
            , typename plus<Pos,Distance>::type
            > type;
    };

    template< typename Other >
    struct BOOST_MPL_AUX_ITERATOR_DISTANCE
    {
        typedef typename minus<typename Other::pos,Pos>::type type;
    };
};

} // namespace mpl
} // namespace boost

#endif // BOOST_MPL_AUX_VECTOR_ITERATOR_HPP_INCLUDED
