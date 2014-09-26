#if !defined(BOOST_PP_IS_ITERATING)

// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.

# ifndef MEMBER_FUNCTION_CAST_DWA2002311_HPP
#  define MEMBER_FUNCTION_CAST_DWA2002311_HPP 

#  include <boost/python/detail/preprocessor.hpp>

#  include <boost/mpl/if.hpp>
#  include <boost/type_traits/composite_traits.hpp>

#  include <boost/preprocessor/comma_if.hpp>
#  include <boost/preprocessor/iterate.hpp>
#  include <boost/preprocessor/debug/line.hpp>

#  include <boost/preprocessor/repetition/enum_params.hpp>
#  include <boost/preprocessor/repetition/enum_trailing_params.hpp>

namespace boost { namespace python { namespace detail { 

template <class S, class FT>
struct cast_helper
{
    struct yes_helper
    {
        static FT stage3(FT x) { return x; }
    };

    struct no_helper
    {
        template <class T>
        static T stage3(T x) { return x; }
    };

    static yes_helper stage2(S*) { return yes_helper(); }
    static no_helper stage2(void*) { return no_helper(); }
};

struct non_member_function_cast_impl
{
    template <class T>
    static non_member_function_cast_impl stage1(T) { return non_member_function_cast_impl(); }

    template <class T>
    static non_member_function_cast_impl stage2(T) { return non_member_function_cast_impl(); }

    template <class T>
    T stage3(T x) { return x; }
};

template <class T>
struct member_function_cast_impl
{
#  ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
    template <class U>
    static non_member_function_cast_impl stage1(U)
    {
        return non_member_function_cast_impl();
    }
#  endif

// Member functions
#  define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 3, <boost/python/detail/member_function_cast.hpp>))
#  include BOOST_PP_ITERATE()
};

template <class T, class SF>
struct member_function_cast
# ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
    : member_function_cast_impl<T>
# else 
    : mpl::if_c<
        is_member_function_pointer<SF>::value
        , member_function_cast_impl<T>
        , non_member_function_cast_impl
    >::type
# endif 
{
};

}}} // namespace boost::python::detail

# endif // MEMBER_FUNCTION_CAST_DWA2002311_HPP

#elif BOOST_PP_ITERATION_DEPTH() == 1
// outer over cv-qualifiers

# define BOOST_PP_ITERATION_PARAMS_2 (3, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/detail/member_function_cast.hpp>))
# include BOOST_PP_ITERATE()

#elif BOOST_PP_ITERATION_DEPTH() == 2
# line BOOST_PP_LINE(__LINE__, member_function_cast.hpp)
// inner over arities

# define N BOOST_PP_ITERATION()
# define Q BOOST_PYTHON_CV_QUALIFIER(BOOST_PP_RELATIVE_ITERATION(1))
# define P BOOST_PP_ENUM_PARAMS_Z(1, N, A)

    template <
        class S, class R
        BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class A)
        >
    static cast_helper<S, R (T::*)( P ) Q>
    stage1(R (S::*)( P ) Q)
    {
        return cast_helper<S, R (T::*)( P ) Q>();
    }

# undef P
# undef N
# undef Q

#endif
