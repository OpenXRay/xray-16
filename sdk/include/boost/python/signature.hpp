///////////////////////////////////////////////////////////////////////////////
//
// Copyright David Abrahams 2002, Joel de Guzman, 2002. Permission to copy,
// use, modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided "as is"
// without express or implied warranty, and with no claim as to its
// suitability for any purpose.
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_PP_IS_ITERATING)

# ifndef SIGNATURE_JDG20020813_HPP
#  define SIGNATURE_JDG20020813_HPP

#  include <boost/python/detail/preprocessor.hpp>
#  include <boost/preprocessor/repeat.hpp>
#  include <boost/preprocessor/enum.hpp>
#  include <boost/preprocessor/enum_params.hpp>
#  include <boost/preprocessor/empty.hpp>
#  include <boost/preprocessor/arithmetic/sub.hpp>
#  include <boost/preprocessor/iterate.hpp>
#  include <boost/python/detail/type_list.hpp>

#  include <boost/preprocessor/debug/line.hpp>
#  include <boost/preprocessor/arithmetic/sub.hpp>
#  include <boost/preprocessor/arithmetic/inc.hpp>
#  include <boost/preprocessor/repetition/enum_trailing_params.hpp>

# define BOOST_PYTHON_LIST_INC(n)        \
   BOOST_PP_CAT(mpl::list, BOOST_PP_INC(n))

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace python { namespace detail {

///////////////////////////////////////////////////////////////////////////////
//
//  The following macros generate expansions for:
//
//      template <class RT, class T0... class TN>
//      inline mpl::list<RT, T0...TN>
//      get_signature(RT(*)(T0...TN))
//      {
//          return mpl::list<RT, T0...TN>();
//      }
//
//   And, for an appropriate assortment of cv-qualifications
//
//      template <class RT, class ClassT, class T0... class TN>
//      inline mpl::list<RT, ClassT cv&, T0...TN>
//      get_signature(RT(ClassT::*)(T0...TN) cv))
//      {
//          return mpl::list<RT, ClassT&, T0...TN>();
//      }
//
//  These functions extract the return type, class (for member
//  functions) and arguments of the input signature and stuffs them in
//  an mpl::list.  Note that cv-qualification is dropped from the
//  target class; that is a necessary sacrifice to ensure that an
//  lvalue from_python converter is used.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
#  define BOOST_PP_ITERATION_PARAMS_1                                   \
    (3, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/signature.hpp>))

#  include BOOST_PP_ITERATE()

}

}} // namespace boost::python


# undef BOOST_PYTHON_LIST_INC

///////////////////////////////////////////////////////////////////////////////
# endif // SIGNATURE_JDG20020813_HPP

#elif BOOST_PP_ITERATION_DEPTH() == 1 // defined(BOOST_PP_IS_ITERATING)

# define N BOOST_PP_ITERATION()

template <
    class RT BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class T)>
inline BOOST_PYTHON_LIST_INC(N)<
    RT BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, T)>
get_signature(RT(*)(BOOST_PP_ENUM_PARAMS_Z(1, N, T)))
{
    return BOOST_PYTHON_LIST_INC(N)<
            RT BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, T)
        >();
}

# undef N

# define BOOST_PP_ITERATION_PARAMS_2 \
    (3, (0, 3, <boost/python/signature.hpp>))
# include BOOST_PP_ITERATE()

#else

# define N BOOST_PP_RELATIVE_ITERATION(1)
# define Q BOOST_PYTHON_CV_QUALIFIER(BOOST_PP_ITERATION())

template <
    class RT, class ClassT BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class T)>
inline BOOST_PYTHON_LIST_INC(BOOST_PP_INC(N))<
    RT, ClassT& BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, T)>
get_signature(RT(ClassT::*)(BOOST_PP_ENUM_PARAMS_Z(1, N, T)) Q)
{
    return BOOST_PYTHON_LIST_INC(BOOST_PP_INC(N))<
            RT, ClassT& BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, T)
        >();
}

# undef Q
# undef N

#endif // !defined(BOOST_PP_IS_ITERATING)
