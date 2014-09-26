/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2003-2003 Aleksey Gurtovoy
    Copyright (c) 2002 Raghavendra Satish
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#ifndef BOOST_SPIRIT_MSVC_HPP
#define BOOST_SPIRIT_MSVC_HPP

#if (defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)) \
    || (defined(BOOST_INTEL_CXX_VERSION) && !defined(_STLPORT_VERSION))
#include <iterator>

namespace boost { namespace spirit { namespace impl {

 #if !defined(BOOST_INTEL_CXX_VERSION)

#if BOOST_MSVC <= 1200

       //////////////////////////////////
        template< typename T >
        struct msvc_never_true
        { enum { value = false }; };

        // warning: not a well-formed C++
        // workaround for MSVC 6.5's "dependent template typedef bug"
        // workaround by Aleksey Gurtovoy (from boost::mpl library)

        //////////////////////////////////
    #define BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER(name, templ)            \
    namespace impl                                                          \
    {                                                                       \
        template <typename F>                                               \
        struct name                                                         \
        {                                                                   \
            template <bool> struct f_ : F {};                               \
                                                                            \
            template <> struct f_<true>                                     \
            {                                                               \
                template <typename P>                                       \
                struct templ { typedef P param_t; };                        \
            };                                                              \
                                                                            \
            template <typename T> struct result_                            \
            : f_<msvc_never_true<F>::value>::template templ<T>              \
            {                                                               \
                typedef f_<msvc_never_true<F>::value>::                     \
                    template templ<T> param_t;                              \
            };                                                              \
        };                                                                  \
                                                                            \
        template <> struct name<int>                                        \
        {                                                                   \
            template <typename T> struct result_                            \
            {                                                               \
                typedef T type;                                             \
            };                                                              \
        };                                                                  \
    }                                                                       \

    #define BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER2(name, templ)           \
    namespace impl                                                          \
    {                                                                       \
        template <typename F>                                               \
        struct name                                                         \
        {                                                                   \
            template <bool> struct f_ : F {};                               \
                                                                            \
            template <> struct f_<true>                                     \
            {                                                               \
                template <typename T1, typename T2>                         \
                struct templ { typedef T1 param_t; };                       \
            };                                                              \
                                                                            \
            template <typename T1, typename T2> struct result_              \
            : f_<msvc_never_true<F>::value>::template templ<T1, T2>         \
            {                                                               \
                typedef f_<msvc_never_true<F>::value>::                     \
                    template templ<T1, T2> param_t;                         \
            };                                                              \
                                                                            \
        };                                                                  \
                                                                            \
        template <> struct name<int>                                        \
        {                                                                   \
            template <typename T1, typename T2> struct result_              \
            {                                                               \
                typedef T1 type;                                            \
            };                                                              \
        };                                                                  \
    }                                                                       \

    #define BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER3(name, templ)           \
    namespace impl                                                          \
    {                                                                       \
        template <typename F>                                               \
        struct name                                                         \
        {                                                                   \
            template <bool> struct f_ : F {};                               \
                                                                            \
            template <> struct f_<true>                                     \
            {                                                               \
                template <typename T1, typename T2, typename T3>            \
                struct templ { typedef T1 param_t; };                       \
            };                                                              \
                                                                            \
            template <typename T1, typename T2, typename T3> struct result_ \
            : f_<msvc_never_true<F>::value>::template templ<T1, T2, T3>     \
            {                                                               \
                typedef f_<msvc_never_true<F>::value>::                     \
                    template templ<T1, T2, T3> param_t;                     \
            };                                                              \
        };                                                                  \
                                                                            \
        template <> struct name<int>                                        \
        {                                                                   \
            template <typename T1, typename T2, typename T3> struct result_ \
            {                                                               \
                typedef T1 type;                                            \
            };                                                              \
        };                                                                  \
    }                                                                       \

    #define BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER4(name, templ)           \
    namespace impl                                                          \
    {                                                                       \
        template <typename F>                                               \
        struct name                                                         \
        {                                                                   \
            template <bool> struct f_ : F {};                               \
                                                                            \
            template <> struct f_<true>                                     \
            {                                                               \
                template <                                                  \
                    typename T1, typename T2, typename T3, typename T4>     \
                struct templ { typedef T1 param_t; };                       \
            };                                                              \
                                                                            \
            template <typename T1, typename T2, typename T3, typename T4>   \
            struct result_                                                  \
            : f_<msvc_never_true<F>::value>::template templ<T1, T2, T3, T4> \
            {                                                               \
                typedef f_<msvc_never_true<F>::value>::                     \
                    template templ<T1, T2, T3, T4> param_t;                 \
            };                                                              \
        };                                                                  \
                                                                            \
        template <> struct name<int>                                        \
        {                                                                   \
            template <typename T1, typename T2, typename T3, typename T4>   \
            struct result_                                                  \
            {                                                               \
                typedef T1 type;                                            \
            };                                                              \
        };                                                                  \
    }                                                                       \

#else

struct int_convertible_
{
    int_convertible_(int);
};

template< typename T >
struct is_msvc_70_ETI_arg
{
    typedef char (&no_tag)[1];
    typedef char (&yes_tag)[2];

    static no_tag test(...);
    static yes_tag test(int_convertible_);
    static T get();

    BOOST_STATIC_CONSTANT(bool, value =
          sizeof(test(get())) == sizeof(yes_tag)
        );
};

template<>
struct is_msvc_70_ETI_arg<int>
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};

    #define BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER(name, templ)            \
    namespace impl                                                          \
    {                                                                       \
        template <bool> struct name##_impl                                  \
        {                                                                   \
            template <typename F, typename T1>                              \
            struct result_                                                  \
            {                                                               \
                typedef int type;                                           \
            };                                                              \
        };                                                                  \
                                                                            \
        template <> struct name##_impl<false>                               \
        {                                                                   \
            template <typename F, typename T1>                              \
            struct result_                                                  \
                : F::template templ<T1>                                     \
            {                                                               \
            };                                                              \
        };                                                                  \
                                                                            \
        template <typename F>                                               \
        struct name                                                         \
        {                                                                   \
            template <typename T1>                                          \
            struct result_                                                  \
                : name##_impl< is_msvc_70_ETI_arg<F>::value >               \
                    ::template result_<F,T1>                                \
            {                                                               \
            };                                                              \
        };                                                                  \
    }                                                                       \

    #define BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER2(name, templ)           \
    namespace impl                                                          \
    {                                                                       \
        template <bool> struct name##_impl                                  \
        {                                                                   \
            template <typename F, typename T1, typename T2>                 \
            struct result_                                                  \
            {                                                               \
                typedef int type;                                           \
            };                                                              \
        };                                                                  \
                                                                            \
        template <> struct name##_impl<false>                               \
        {                                                                   \
            template <typename F, typename T1, typename T2>                 \
            struct result_                                                  \
                : F::template templ<T1,T2>                                  \
            {                                                               \
            };                                                              \
        };                                                                  \
                                                                            \
        template <typename F>                                               \
        struct name                                                         \
        {                                                                   \
            template <typename T1, typename T2>                             \
            struct result_                                                  \
                : name##_impl< is_msvc_70_ETI_arg<F>::value >               \
                    ::template result_<F,T1,T2>                             \
            {                                                               \
            };                                                              \
        };                                                                  \
    }                                                                       \

    #define BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER3(name, templ)           \
    namespace impl                                                          \
    {                                                                       \
        template <bool> struct name##_impl                                  \
        {                                                                   \
            template <typename F, typename T1, typename T2, typename T3>    \
            struct result_                                                  \
            {                                                               \
                typedef int type;                                           \
            };                                                              \
        };                                                                  \
                                                                            \
        template <> struct name##_impl<false>                               \
        {                                                                   \
            template <typename F, typename T1, typename T2, typename T3>    \
            struct result_                                                  \
                : F::template templ<T1,T2,T3>                               \
            {                                                               \
            };                                                              \
        };                                                                  \
                                                                            \
        template <typename F>                                               \
        struct name                                                         \
        {                                                                   \
            template <typename T1, typename T2, typename T3>                \
            struct result_                                                  \
                : name##_impl< is_msvc_70_ETI_arg<F>::value >               \
                    ::template result_<F,T1,T2,T3>                          \
            {                                                               \
            };                                                              \
        };                                                                  \
    }                                                                       \

    #define BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER4(name, templ)           \
    namespace impl                                                          \
    {                                                                       \
        template <bool> struct name##_impl                                  \
        {                                                                   \
            template <typename F,                                           \
                typename T1, typename T2, typename T3, typename T4>         \
            struct result_                                                  \
            {                                                               \
                typedef int type;                                           \
            };                                                              \
        };                                                                  \
                                                                            \
        template <> struct name##_impl<false>                               \
        {                                                                   \
            template <typename F,                                           \
                typename T1, typename T2, typename T3, typename T4>         \
            struct result_                                                  \
                : F::template templ<T1,T2,T3,T4>                            \
            {                                                               \
            };                                                              \
        };                                                                  \
                                                                            \
        template <typename F>                                               \
        struct name                                                         \
        {                                                                   \
            template <typename T1, typename T2, typename T3, typename T4>   \
            struct result_                                                  \
                : name##_impl< is_msvc_70_ETI_arg<F>::value >               \
                    ::template result_<F,T1,T2,T3,T4>                       \
            {                                                               \
            };                                                              \
        };                                                                  \
    }                                                                       \

#endif // BOOST_MSVC <= 1200


#endif
        ///////////////////////////////////////////////////////////////////////
        //
        //      Iterator traits require partial specialization. The VC++
        //      iterator_traits class in "utility" does not define pointer
        //      or reference types. The "difference_type" is called the
        //      distance_type to enure conformity we define an iterator
        //      traits class inside spirit namespace. The user will have to
        //      SPECIALIZE this iterator type if they use iterators
        //
        ///////////////////////////////////////////////////////////////////////

        template<typename IterT>
        struct iterator_traits
        {
           typedef typename IterT::difference_type difference_type;
           typedef typename IterT::value_type value_type;
           typedef typename IterT::pointer pointer;
           typedef typename IterT::reference reference;
           typedef typename IterT::iterator_category iterator_category;
        };

        //commonly used iterator_traits
        template<>
        struct iterator_traits<char const*>
        {
            typedef std::random_access_iterator_tag iterator_category;
            typedef char            value_type;
            typedef ptrdiff_t       difference_type;
            typedef const char*     pointer;
            typedef const char&     reference;
        };

        template<>
        struct iterator_traits<char*>
        {
            typedef std::random_access_iterator_tag iterator_category;
            typedef char            value_type;
            typedef ptrdiff_t       difference_type;
            typedef char*           pointer;
            typedef char&           reference;
        };

        template<>
        struct iterator_traits<wchar_t const*>
        {
            typedef std::random_access_iterator_tag iterator_category;
            typedef wchar_t         value_type;
            typedef ptrdiff_t       difference_type;
            typedef const wchar_t*  pointer;
            typedef const wchar_t&  reference;
        };

        template<>
        struct iterator_traits<wchar_t*>
        {
            typedef std::random_access_iterator_tag iterator_category;
            typedef wchar_t         value_type;
            typedef ptrdiff_t       difference_type;
            typedef wchar_t*        pointer;
            typedef wchar_t&        reference;
        };

        // the istream_iterator of VC++6.0 doesn't have the appropriate
        // traits classes defined. For supporting multi-pass

        template<>
        struct iterator_traits<
#if defined (_STLPORT_VERSION)
            std::istream_iterator<char> >
#else
            std::istream_iterator<unsigned char, char, std::char_traits<char> > >
#endif
        {
            typedef std::forward_iterator_tag iterator_category;
            typedef char*           pointer;
            typedef char&           reference;
            typedef char            value_type;
            typedef ptrdiff_t       difference_type;
        };

}}} // namespace boost::spirit::impl

#endif
#endif
