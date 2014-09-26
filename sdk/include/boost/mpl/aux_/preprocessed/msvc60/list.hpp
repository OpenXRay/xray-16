// preprocessed version of 'boost/mpl/list.hpp' header
// see the original for copyright information

namespace boost {
namespace mpl {

namespace aux {
template< nttp_int N > struct list_impl_chooser;
}

namespace aux {

template<>
struct list_impl_chooser<0>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list0<
              
            > type;
    };
};

} // namespace aux

namespace aux {

template<>
struct list_impl_chooser<1>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list1<
               T0
            > type;
    };
};

} // namespace aux

namespace aux {

template<>
struct list_impl_chooser<2>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list2<
               T0, T1
            > type;
    };
};

} // namespace aux

namespace aux {

template<>
struct list_impl_chooser<3>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list3<
               T0, T1, T2
            > type;
    };
};

} // namespace aux

namespace aux {

template<>
struct list_impl_chooser<4>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list4<
               T0, T1, T2, T3
            > type;
    };
};

} // namespace aux

namespace aux {

template<>
struct list_impl_chooser<5>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list5<
               T0, T1, T2, T3, T4
            > type;
    };
};

} // namespace aux

namespace aux {

template<>
struct list_impl_chooser<6>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list6<
               T0, T1, T2, T3, T4, T5
            > type;
    };
};

} // namespace aux

namespace aux {

template<>
struct list_impl_chooser<7>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list7<
               T0, T1, T2, T3, T4, T5, T6
            > type;
    };
};

} // namespace aux

namespace aux {

template<>
struct list_impl_chooser<8>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list8<
               T0, T1, T2, T3, T4, T5, T6, T7
            > type;
    };
};

} // namespace aux

namespace aux {

template<>
struct list_impl_chooser<9>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list9<
               T0, T1, T2, T3, T4, T5, T6, T7, T8
            > type;
    };
};

} // namespace aux

namespace aux {

template<>
struct list_impl_chooser<10>
{
    template<
          typename T0, typename T1, typename T2, typename T3, typename T4
        , typename T5, typename T6, typename T7, typename T8, typename T9
        >
    struct result_
    {
        typedef list10<
               T0, T1, T2, T3, T4, T5, T6, T7, T8, T9
            > type;
    };
};

} // namespace aux

namespace aux {

template< typename T >
struct is_list_arg
{
    enum { value = true };
};

template<>
struct is_list_arg<void_>
{
    enum { value = false };
};

template<
      typename T1, typename T2, typename T3, typename T4, typename T5
    , typename T6, typename T7, typename T8, typename T9, typename T10
    >
struct list_count_args
{
    enum { value =
          is_list_arg<T1>::value + is_list_arg<T2>::value 
        + is_list_arg<T3>::value + is_list_arg<T4>::value 
        + is_list_arg<T5>::value + is_list_arg<T6>::value 
        + is_list_arg<T7>::value + is_list_arg<T8>::value 
        + is_list_arg<T9>::value + is_list_arg<T10>::value
        };
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    >
struct list_impl
{
    typedef aux::list_count_args< T0,T1,T2,T3,T4,T5,T6,T7,T8,T9 > arg_num_;
    typedef typename aux::list_impl_chooser< 
          arg_num_::value
        >::template result_< T0,T1,T2,T3,T4,T5,T6,T7,T8,T9 >::type type;
};

} // namespace aux

template<
      typename T0 = void_, typename T1 = void_, typename T2 = void_
    , typename T3 = void_, typename T4 = void_, typename T5 = void_
    , typename T6 = void_, typename T7 = void_, typename T8 = void_
    , typename T9 = void_
    >
struct list
    : aux::list_impl< T0,T1,T2,T3,T4,T5,T6,T7,T8,T9 >::type
{
    typedef typename aux::list_impl<
           T0, T1, T2, T3, T4, T5, T6, T7, T8, T9
        >::type type;
};

} // namespace mpl
} // namespace boost

