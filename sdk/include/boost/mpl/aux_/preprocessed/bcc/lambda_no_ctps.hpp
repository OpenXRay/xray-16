// preprocessed version of 'boost/mpl/lambda_no_ctps.hpp' header
// see the original for copyright information

namespace boost {
namespace mpl {

namespace aux {

template< int arity_, bool Protect > struct lambda_impl
{
    template< typename T > struct result_
    {
        typedef T type;
    };
};

template<> struct lambda_impl<1, false>
{
    template< typename F > struct result_
    {
        typedef typename F::rebind f_;
        typedef bind1<
              f_
            , typename lambda< typename F::arg1, false >::type
            > type;
    };
};

template<> struct lambda_impl<1, true>
{
    template< typename F > struct result_
    {
        typedef typename F::rebind f_;
        typedef mpl::protect< bind1<
              f_
            , typename lambda< typename F::arg1, false >::type
            > > type;
    };
};

template<> struct lambda_impl<2, false>
{
    template< typename F > struct result_
    {
        typedef typename F::rebind f_;
        typedef bind2<
              f_
            
            ,typename lambda< typename F::arg1, false >::type, typename lambda< typename F::arg2, false >::type
            > type;
    };
};

template<> struct lambda_impl<2, true>
{
    template< typename F > struct result_
    {
        typedef typename F::rebind f_;
        typedef mpl::protect< bind2<
              f_
            
            ,typename lambda< typename F::arg1, false >::type, typename lambda< typename F::arg2, false >::type
            > > type;
    };
};

template<> struct lambda_impl<3, false>
{
    template< typename F > struct result_
    {
        typedef typename F::rebind f_;
        typedef bind3<
              f_
            
            ,typename lambda< typename F::arg1, false >::type, typename lambda< typename F::arg2, false >::type, typename lambda< typename F::arg3, false >::type
            > type;
    };
};

template<> struct lambda_impl<3, true>
{
    template< typename F > struct result_
    {
        typedef typename F::rebind f_;
        typedef mpl::protect< bind3<
              f_
            
            ,typename lambda< typename F::arg1, false >::type, typename lambda< typename F::arg2, false >::type, typename lambda< typename F::arg3, false >::type
            > > type;
    };
};

template<> struct lambda_impl<4, false>
{
    template< typename F > struct result_
    {
        typedef typename F::rebind f_;
        typedef bind4<
              f_
            
            ,typename lambda< typename F::arg1, false >::type, typename lambda< typename F::arg2, false >::type, typename lambda< typename F::arg3, false >::type, typename lambda< typename F::arg4, false >::type
            > type;
    };
};

template<> struct lambda_impl<4, true>
{
    template< typename F > struct result_
    {
        typedef typename F::rebind f_;
        typedef mpl::protect< bind4<
              f_
            
            ,typename lambda< typename F::arg1, false >::type, typename lambda< typename F::arg2, false >::type, typename lambda< typename F::arg3, false >::type, typename lambda< typename F::arg4, false >::type
            > > type;
    };
};

template<> struct lambda_impl<5, false>
{
    template< typename F > struct result_
    {
        typedef typename F::rebind f_;
        typedef bind5<
              f_
            
            ,typename lambda< typename F::arg1, false >::type, typename lambda< typename F::arg2, false >::type, typename lambda< typename F::arg3, false >::type, typename lambda< typename F::arg4, false >::type, typename lambda< typename F::arg5, false >::type
            > type;
    };
};

template<> struct lambda_impl<5, true>
{
    template< typename F > struct result_
    {
        typedef typename F::rebind f_;
        typedef mpl::protect< bind5<
              f_
            
            ,typename lambda< typename F::arg1, false >::type, typename lambda< typename F::arg2, false >::type, typename lambda< typename F::arg3, false >::type, typename lambda< typename F::arg4, false >::type, typename lambda< typename F::arg5, false >::type
            > > type;
    };
};

} // namespace aux

template< typename T, bool Protect = true >
struct lambda
    : aux::lambda_impl<
          ::boost::mpl::aux::template_arity<T>::value

        , Protect

        >::template result_<T>
{
};

} // namespace mpl
} // namespace boost

