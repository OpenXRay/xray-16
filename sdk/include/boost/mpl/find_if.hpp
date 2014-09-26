//-----------------------------------------------------------------------------
// boost mpl/find_if.hpp header file
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

#ifndef BOOST_MPL_FIND_IF_HPP_INCLUDED
#define BOOST_MPL_FIND_IF_HPP_INCLUDED

#include "boost/mpl/aux_/iter_fold_if_impl.hpp"
#include "boost/mpl/aux_/iter_apply.hpp"
#include "boost/mpl/or.hpp"
#include "boost/mpl/not.hpp"
#include "boost/mpl/begin_end.hpp"
#include "boost/mpl/always.hpp"
#include "boost/mpl/lambda.hpp"
#include "boost/mpl/bind.hpp"
#include "boost/mpl/apply.hpp"
#include "boost/mpl/void.hpp"
#include "boost/mpl/aux_/void_spec.hpp"
#include "boost/mpl/aux_/lambda_support.hpp"
#include "boost/type_traits/is_same.hpp"

namespace boost {
namespace mpl {

namespace aux {

template< typename LastIterator >
struct find_if_pred
{
    template<
          typename Predicate
        , typename Iterator
        >
    struct apply
    {
        typedef typename not_< or_<
              is_same<Iterator,LastIterator>
            , aux::iter_apply1<Predicate,Iterator>
            > >::type type;
    };
};

} // namespace aux

BOOST_MPL_AUX_AGLORITHM_NAMESPACE_BEGIN

template<
      typename BOOST_MPL_AUX_VOID_SPEC_PARAM(Sequence)
    , typename BOOST_MPL_AUX_VOID_SPEC_PARAM(Predicate)
    >
struct find_if
{
 private:
    typedef typename begin<Sequence>::type first_;
    typedef typename end<Sequence>::type last_;
    typedef typename lambda<Predicate>::type pred_;

 public:
    typedef typename aux::iter_fold_if_impl<
          first_
        , pred_
        , mpl::arg<1>
        , aux::find_if_pred<last_>
        , void
        , always<false_>
        >::iterator type;

    BOOST_MPL_AUX_LAMBDA_SUPPORT(2,find_if,(Sequence,Predicate))
};

BOOST_MPL_AUX_AGLORITHM_NAMESPACE_END

BOOST_MPL_AUX_ALGORITHM_VOID_SPEC(2,find_if)

} // namespace mpl
} // namespace boost

#endif // BOOST_MPL_FIND_IF_HPP_INCLUDED
