// preprocessed version of 'boost/mpl/aux_/full_lambda.hpp' header
// see the original for copyright information

namespace boost {
namespace mpl {

template<
      typename T
    , typename Protect = false_
    , typename Arity = int_< aux::template_arity<T>::value >
    >
struct lambda_impl
{
    typedef false_ is_le;
    typedef T type;
};

template<
      typename T
    , typename Arity = int_< aux::template_arity<T>::value >
    >
struct lambda
    : lambda_impl< T,false_,Arity >
{
};

namespace aux {

template<
      bool C1 = false, bool C2 = false, bool C3 = false, bool C4 = false
    , bool C5 = false
    >
struct lambda_or
    : true_
{
};

template<>
struct lambda_or< false,false,false,false,false >
    : false_
{
};

} // namespace aux

template< int N, typename Protect >
struct lambda_impl< arg<N>,Protect,int_<-1> >
{
    typedef true_ is_le;
    typedef arg<N> type;
};

template<
      typename F
    , typename Protect
    >
struct lambda_impl<
      bind0<F>
    , Protect, int_<1>
    >
{
    typedef false_ is_le;
    typedef bind0<
          F
        > type;
};

template<
      template< typename P1 > class F
    , typename T1
    >
struct lambda< F<T1>,int_<1> >
    : lambda_impl< F<T1>,true_,int_<1> >
{
};

namespace aux {

template<
      typename IsLE, typename Protect
    , template< typename P1 > class F
    , typename L1
    >
struct le_result1
{
    typedef F<
          typename L1::type
        > type;
};

template<
      template< typename P1 > class F
    , typename L1
    >
struct le_result1< true_,false_,F,L1 >
{
    typedef bind1<
          quote1<F>
        , typename L1::type
        > type;
};

template<
      template< typename P1 > class F
    , typename L1
    >
struct le_result1< true_,true_,F,L1 >
{
    typedef protect< bind1<
          quote1<F>
        , typename L1::type
        > > type;
};

} // namespace aux

template<
      template< typename P1 > class F
    , typename T1
    , typename Protect
    >
struct lambda_impl< 
      F< T1>,Protect,int_<1 >
    >
{
    typedef lambda_impl<T1> l1;
    typedef aux::lambda_or<
          l1::is_le::value
        > is_le;

    typedef typename aux::le_result1<
          typename is_le::type
        , Protect
        , F
        , l1
        >::type type;
};

template<
      typename F, typename T1
    , typename Protect
    >
struct lambda_impl<
      bind1< F,T1 >
    , Protect, int_<2>
    >
{
    typedef false_ is_le;
    typedef bind1<
          F
        , T1
        > type;
};

template<
      template< typename P1, typename P2 > class F
    , typename T1, typename T2
    >
struct lambda< F<T1,T2>,int_<2> >
    : lambda_impl< F<T1,T2>,true_,int_<2> >
{
};

namespace aux {

template<
      typename IsLE, typename Protect
    , template< typename P1, typename P2 > class F
    , typename L1, typename L2
    >
struct le_result2
{
    typedef F<
          typename L1::type, typename L2::type
        > type;
};

template<
      template< typename P1, typename P2 > class F
    , typename L1, typename L2
    >
struct le_result2< true_,false_,F,L1,L2 >
{
    typedef bind2<
          quote2<F>
        , typename L1::type, typename L2::type
        > type;
};

template<
      template< typename P1, typename P2 > class F
    , typename L1, typename L2
    >
struct le_result2< true_,true_,F,L1,L2 >
{
    typedef protect< bind2<
          quote2<F>
        , typename L1::type, typename L2::type
        > > type;
};

} // namespace aux

template<
      template< typename P1, typename P2 > class F
    , typename T1, typename T2
    , typename Protect
    >
struct lambda_impl< 
      F< T1,T2>,Protect,int_<2 >
    >
{
    typedef lambda_impl<T1> l1;
    typedef lambda_impl<T2> l2;
    
    typedef aux::lambda_or<
          l1::is_le::value, l2::is_le::value
        > is_le;

    typedef typename aux::le_result2<
          typename is_le::type
        , Protect
        , F
        , l1, l2
        >::type type;
};

template<
      typename F, typename T1, typename T2
    , typename Protect
    >
struct lambda_impl<
      bind2< F,T1,T2 >
    , Protect, int_<3>
    >
{
    typedef false_ is_le;
    typedef bind2<
          F
        , T1, T2
        > type;
};

template<
      template< typename P1, typename P2, typename P3 > class F
    , typename T1, typename T2, typename T3
    >
struct lambda< F<T1,T2,T3>,int_<3> >
    : lambda_impl< F<T1,T2,T3>,true_,int_<3> >
{
};

namespace aux {

template<
      typename IsLE, typename Protect
    , template< typename P1, typename P2, typename P3 > class F
    , typename L1, typename L2, typename L3
    >
struct le_result3
{
    typedef F<
          typename L1::type, typename L2::type, typename L3::type
        > type;
};

template<
      template< typename P1, typename P2, typename P3 > class F
    , typename L1, typename L2, typename L3
    >
struct le_result3< true_,false_,F,L1,L2,L3 >
{
    typedef bind3<
          quote3<F>
        , typename L1::type, typename L2::type, typename L3::type
        > type;
};

template<
      template< typename P1, typename P2, typename P3 > class F
    , typename L1, typename L2, typename L3
    >
struct le_result3< true_,true_,F,L1,L2,L3 >
{
    typedef protect< bind3<
          quote3<F>
        , typename L1::type, typename L2::type, typename L3::type
        > > type;
};

} // namespace aux

template<
      template< typename P1, typename P2, typename P3 > class F
    , typename T1, typename T2, typename T3
    , typename Protect
    >
struct lambda_impl< 
      F< T1,T2,T3>,Protect,int_<3 >
    >
{
    typedef lambda_impl<T1> l1;
    typedef lambda_impl<T2> l2;
    typedef lambda_impl<T3> l3;
    
    typedef aux::lambda_or<
          l1::is_le::value, l2::is_le::value, l3::is_le::value
        > is_le;

    typedef typename aux::le_result3<
          typename is_le::type
        , Protect
        , F
        , l1, l2, l3
        >::type type;
};

template<
      typename F, typename T1, typename T2, typename T3
    , typename Protect
    >
struct lambda_impl<
      bind3< F,T1,T2,T3 >
    , Protect, int_<4>
    >
{
    typedef false_ is_le;
    typedef bind3<
          F
        , T1, T2, T3
        > type;
};

template<
      template< typename P1, typename P2, typename P3, typename P4 > class F
    , typename T1, typename T2, typename T3, typename T4
    >
struct lambda< F<T1,T2,T3,T4>,int_<4> >
    : lambda_impl< F<T1,T2,T3,T4>,true_,int_<4> >
{
};

namespace aux {

template<
      typename IsLE, typename Protect
    , template< typename P1, typename P2, typename P3, typename P4 > class F
    , typename L1, typename L2, typename L3, typename L4
    >
struct le_result4
{
    typedef F<
          typename L1::type, typename L2::type, typename L3::type
        , typename L4::type
        > type;
};

template<
      template< typename P1, typename P2, typename P3, typename P4 > class F
    , typename L1, typename L2, typename L3, typename L4
    >
struct le_result4< true_,false_,F,L1,L2,L3,L4 >
{
    typedef bind4<
          quote4<F>
        , typename L1::type, typename L2::type, typename L3::type
        , typename L4::type
        > type;
};

template<
      template< typename P1, typename P2, typename P3, typename P4 > class F
    , typename L1, typename L2, typename L3, typename L4
    >
struct le_result4< true_,true_,F,L1,L2,L3,L4 >
{
    typedef protect< bind4<
          quote4<F>
        , typename L1::type, typename L2::type, typename L3::type
        , typename L4::type
        > > type;
};

} // namespace aux

template<
      template< typename P1, typename P2, typename P3, typename P4 > class F
    , typename T1, typename T2, typename T3, typename T4
    , typename Protect
    >
struct lambda_impl< 
      F< T1,T2,T3,T4>,Protect,int_<4 >
    >
{
    typedef lambda_impl<T1> l1;
    typedef lambda_impl<T2> l2;
    typedef lambda_impl<T3> l3;
    typedef lambda_impl<T4> l4;
    
    typedef aux::lambda_or<
          l1::is_le::value, l2::is_le::value, l3::is_le::value
        , l4::is_le::value
        > is_le;

    typedef typename aux::le_result4<
          typename is_le::type
        , Protect
        , F
        , l1, l2, l3, l4
        >::type type;
};

template<
      typename F, typename T1, typename T2, typename T3, typename T4
    , typename Protect
    >
struct lambda_impl<
      bind4< F,T1,T2,T3,T4 >
    , Protect, int_<5>
    >
{
    typedef false_ is_le;
    typedef bind4<
          F
        , T1, T2, T3, T4
        > type;
};

template<
      template<
          typename P1, typename P2, typename P3, typename P4
        , typename P5
        >
      class F
    , typename T1, typename T2, typename T3, typename T4, typename T5
    >
struct lambda< F<T1,T2,T3,T4,T5>,int_<5> >
    : lambda_impl< F<T1,T2,T3,T4,T5>,true_,int_<5> >
{
};

namespace aux {

template<
      typename IsLE, typename Protect
    , template< typename P1, typename P2, typename P3, typename P4, typename P5 > class F
    , typename L1, typename L2, typename L3, typename L4, typename L5
    >
struct le_result5
{
    typedef F<
          typename L1::type, typename L2::type, typename L3::type
        , typename L4::type, typename L5::type
        > type;
};

template<
      template<
          typename P1, typename P2, typename P3, typename P4
        , typename P5
        >
      class F
    , typename L1, typename L2, typename L3, typename L4, typename L5
    >
struct le_result5< true_,false_,F,L1,L2,L3,L4,L5 >
{
    typedef bind5<
          quote5<F>
        , typename L1::type, typename L2::type, typename L3::type
        , typename L4::type, typename L5::type
        > type;
};

template<
      template<
          typename P1, typename P2, typename P3, typename P4
        , typename P5
        >
      class F
    , typename L1, typename L2, typename L3, typename L4, typename L5
    >
struct le_result5< true_,true_,F,L1,L2,L3,L4,L5 >
{
    typedef protect< bind5<
          quote5<F>
        , typename L1::type, typename L2::type, typename L3::type
        , typename L4::type, typename L5::type
        > > type;
};

} // namespace aux

template<
      template<
          typename P1, typename P2, typename P3, typename P4
        , typename P5
        >
      class F
    , typename T1, typename T2, typename T3, typename T4, typename T5
    , typename Protect
    >
struct lambda_impl< 
      F< T1,T2,T3,T4,T5>,Protect,int_<5 >
    >
{
    typedef lambda_impl<T1> l1;
    typedef lambda_impl<T2> l2;
    typedef lambda_impl<T3> l3;
    typedef lambda_impl<T4> l4;
    typedef lambda_impl<T5> l5;
    
    typedef aux::lambda_or<
          l1::is_le::value, l2::is_le::value, l3::is_le::value
        , l4::is_le::value, l5::is_le::value
        > is_le;

    typedef typename aux::le_result5<
          typename is_le::type
        , Protect
        , F
        , l1, l2, l3, l4, l5
        >::type type;
};

template<
      typename F, typename T1, typename T2, typename T3, typename T4
    , typename T5
    , typename Protect
    >
struct lambda_impl<
      bind5< F,T1,T2,T3,T4,T5 >
    , Protect, int_<6>
    >
{
    typedef false_ is_le;
    typedef bind5<
          F
        , T1, T2, T3, T4, T5
        > type;
};

// special case for 'protect'
template< typename T, typename Protect >
struct lambda_impl< protect<T>,Protect,int_<1> >
{
    typedef false_ is_le;
    typedef protect<T> type;
};

// specializations for main 'bind', 'bind1st' and 'bind2nd' forms
template<
      typename F, typename T1, typename T2, typename T3, typename T4
    , typename T5
    , typename Protect
    >
struct lambda_impl<
      bind< F,T1,T2,T3,T4,T5 >
    , Protect 
    , int_<6>
    >
{
    typedef false_ is_le;
    typedef bind< F,T1,T2,T3,T4,T5 > type;
};

template<
      typename F, typename T
    , typename Protect
    >
struct lambda_impl< bind1st<F,T>,Protect,int_<2> >
{
    typedef false_ is_le;
    typedef bind1st< F,T > type;
};

template<
      typename F, typename T
    , typename Protect
    >
struct lambda_impl< bind2nd<F,T>,Protect,int_<2> >
{
    typedef false_ is_le;
    typedef bind2nd< F,T > type;
};

} // namespace mpl
} // namespace boost

