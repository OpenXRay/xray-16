///////////////////////////////////////////////////////////////////////////////
//
// Copyright David Abrahams 2002, Joel de Guzman, 2002. Permission to copy,
// use, modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided "as is"
// without express or implied warranty, and with no claim as to its
// suitability for any purpose.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef INIT_JDG20020820_HPP
#define INIT_JDG20020820_HPP

#include <boost/python/detail/type_list.hpp>
#include <boost/python/args_fwd.hpp>
#include <boost/python/detail/make_keyword_range_fn.hpp>

#include <boost/mpl/fold_backward.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/apply_if.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/iterator_range.hpp>
#include <boost/mpl/not.hpp>

# include <boost/python/detail/mpl_lambda.hpp>

#include <boost/mpl/lambda.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/bool.hpp>

#include <boost/type_traits/is_same.hpp>

#include <boost/static_assert.hpp>
#include <boost/preprocessor/enum_params_with_a_default.hpp>
#include <boost/preprocessor/enum_params.hpp>
#include <boost/preprocessor/enum_params.hpp>
#include <boost/preprocessor/repeat.hpp>

#include <utility>

///////////////////////////////////////////////////////////////////////////////
#define BOOST_PYTHON_OVERLOAD_TYPES_WITH_DEFAULT                                \
    BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(                                        \
        BOOST_PYTHON_MAX_ARITY,                                                 \
        class T,                                                                \
        mpl::void_)                                                             \

#define BOOST_PYTHON_OVERLOAD_TYPES                                             \
    BOOST_PP_ENUM_PARAMS_Z(1,                                                   \
        BOOST_PYTHON_MAX_ARITY,                                                 \
        class T)                                                                \

#define BOOST_PYTHON_OVERLOAD_ARGS                                              \
    BOOST_PP_ENUM_PARAMS_Z(1,                                                   \
        BOOST_PYTHON_MAX_ARITY,                                                 \
        T)                                                                      \

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace python {

template <BOOST_PYTHON_OVERLOAD_TYPES_WITH_DEFAULT>
class init; // forward declaration


template <BOOST_PYTHON_OVERLOAD_TYPES_WITH_DEFAULT>
struct optional; // forward declaration

namespace detail
{
  namespace error
  {
    template <int keywords, int init_args>
    struct more_keywords_than_init_arguments
    {
        typedef char too_many_keywords[init_args - keywords >= 0 ? 1 : -1];
    };
  }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  is_optional<T>::value
    //
    //      This metaprogram checks if T is an optional
    //
    ///////////////////////////////////////////////////////////////////////////
    #if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

    template <class T>
    struct is_optional {

    private:

        template <BOOST_PYTHON_OVERLOAD_TYPES>
        static boost::type_traits::yes_type f(optional<BOOST_PYTHON_OVERLOAD_ARGS>);
        static boost::type_traits::no_type f(...);
        static T t();

    public:

        BOOST_STATIC_CONSTANT(
            bool, value =
                sizeof(f(t())) == sizeof(::boost::type_traits::yes_type));
        typedef mpl::bool_<value> type;

        BOOST_PYTHON_MPL_LAMBDA_SUPPORT(1,is_optional,(T))
    };

    ///////////////////////////////////////
    #else // defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

    template <class T>
    struct is_optional_impl {

        BOOST_STATIC_CONSTANT(bool, value = false);
    };

    template <BOOST_PYTHON_OVERLOAD_TYPES>
    struct is_optional_impl<optional<BOOST_PYTHON_OVERLOAD_ARGS> > {

        BOOST_STATIC_CONSTANT(bool, value = true);
    };

    template <class T>
    struct is_optional : is_optional_impl<T>
    {
        typedef mpl::bool_<is_optional_impl<T>::value> type;
        BOOST_PYTHON_MPL_LAMBDA_SUPPORT(1,is_optional,(T))
    };
    #endif // defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

} // namespace detail

template <class DerivedT>
struct init_base
{
    init_base(char const* doc_, detail::keyword_range const& keywords_)
        : m_doc(doc_), m_keywords(keywords_)
    {}
        
    init_base(char const* doc_)
        : m_doc(doc_)
    {}
        
    DerivedT const& derived() const
    {
        return *static_cast<DerivedT const*>(this);
    }
    
    char const* doc_string() const
    {
        return m_doc;
    }

    detail::keyword_range const& keywords() const
    {
        return m_keywords;
    }

    static default_call_policies call_policies()
    {
        return default_call_policies();
    }
    
 private: // data members
    char const* m_doc;
    detail::keyword_range m_keywords;
};

template <class CallPoliciesT, class InitT>
class init_with_call_policies
    : public init_base<init_with_call_policies<CallPoliciesT, InitT> >
{
    typedef init_base<init_with_call_policies<CallPoliciesT, InitT> > base;
 public:
    BOOST_STATIC_CONSTANT(int, n_arguments = InitT::n_arguments);
    BOOST_STATIC_CONSTANT(int, n_defaults = InitT::n_defaults);

    typedef typename InitT::reversed_args reversed_args;

    init_with_call_policies(
        CallPoliciesT const& policies_
        , char const* doc_
        , detail::keyword_range const& keywords
        )
        : base(doc_, keywords)
        , m_policies(policies_)
    {}

    CallPoliciesT const& call_policies() const
    {
        return this->m_policies;
    }
    
 private: // data members
    CallPoliciesT m_policies;
};

template <BOOST_PYTHON_OVERLOAD_TYPES>
class init : public init_base<init<BOOST_PYTHON_OVERLOAD_ARGS> >
{
    typedef init_base<init<BOOST_PYTHON_OVERLOAD_ARGS> > base;
 public:
    typedef init<BOOST_PYTHON_OVERLOAD_ARGS> self_t;

    init(char const* doc_ = 0)
        : base(doc_)
    {
    }
    
    template <class Keywords>
    init(char const* doc_, Keywords const& kw)
        : base(doc_, std::make_pair(kw.base(), kw.base() + Keywords::size))
    {
        typedef typename detail::error::more_keywords_than_init_arguments<
            Keywords::size, n_arguments
            >::too_many_keywords assertion;
    }

    template <class Keywords>
    init(Keywords const& kw, char const* doc_ = 0)
        : base(doc_, kw.range())
    {
        typedef typename detail::error::more_keywords_than_init_arguments<
            Keywords::size, n_arguments
            >::too_many_keywords assertion;
    }

    template <class CallPoliciesT>
    init_with_call_policies<CallPoliciesT, self_t>
    operator[](CallPoliciesT const& policies) const
    {
        return init_with_call_policies<CallPoliciesT, self_t>(
            policies, this->doc_string(), this->keywords());
    }

    typedef detail::type_list<BOOST_PYTHON_OVERLOAD_ARGS> signature_;
    typedef typename mpl::end<signature_>::type finish;

    // Find the optional<> element, if any
    typedef typename mpl::find_if<
        signature_, detail::is_optional<mpl::_>
    >::type opt;


    // Check to make sure the optional<> element, if any, is the last one
    typedef typename mpl::apply_if<
        is_same<opt,finish>
        , mpl::identity<opt>
        , mpl::next<opt>
    >::type expected_finish;
    BOOST_STATIC_ASSERT((is_same<expected_finish, finish>::value));

    typedef typename mpl::apply_if<
        is_same<opt,finish>
        , mpl::list0<>
        , opt
    >::type optional_args;

    // Count the number of default args
    BOOST_STATIC_CONSTANT(int, n_defaults = mpl::size<optional_args>::value);

    typedef typename mpl::iterator_range<
        typename mpl::begin<signature_>::type
        , opt
    >::type required_args;

    // Build a reverse image of all the args, including optionals
    typedef typename mpl::fold<
        required_args
        , mpl::list0<>
        , mpl::push_front<>
    >::type reversed_required;

    typedef typename mpl::fold<
        optional_args
        , reversed_required
        , mpl::push_front<>
    >::type reversed_args;

    // Count the maximum number of arguments
    BOOST_STATIC_CONSTANT(int, n_arguments = mpl::size<reversed_args>::value);
};

///////////////////////////////////////////////////////////////////////////////
//
//  optional
//
//      optional<T0...TN>::type returns a typelist.
//
///////////////////////////////////////////////////////////////////////////////
template <BOOST_PYTHON_OVERLOAD_TYPES>
struct optional
    : detail::type_list<BOOST_PYTHON_OVERLOAD_ARGS>
{
};

namespace detail
{
  template <class ClassT, class CallPoliciesT, class ReversedArgs>
  void def_init_reversed(
      ClassT& cl
      , ReversedArgs const&
      , CallPoliciesT const& policies
      , char const* doc
      , detail::keyword_range const& keywords_
      )
  {
      typedef typename mpl::fold<
          ReversedArgs
          , mpl::list0<>
          , mpl::push_front<>
          >::type args;

      typedef typename ClassT::holder_selector holder_selector_t;
#    if !BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
      typedef typename holder_selector_t::type selector_t;
#    endif 
      typedef typename ClassT::held_type held_type_t;

      cl.def(
            "__init__",
            detail::make_keyword_range_constructor<args>(
                policies
                , keywords_
#    if BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
                // Using runtime type selection works around a CWPro7 bug.
                , holder_selector_t::execute((held_type_t*)0).get()
#    else
                , selector_t::get()
#    endif 
                )
            , doc
            );
  }

  ///////////////////////////////////////////////////////////////////////////////
  //
  //  define_class_init_helper<N>::apply
  //
  //      General case
  //
  //      Accepts a class_ and an arguments list. Defines a constructor
  //      for the class given the arguments and recursively calls
  //      define_class_init_helper<N-1>::apply with one less arguments (the
  //      rightmost argument is shaved off)
  //
  ///////////////////////////////////////////////////////////////////////////////
  template <int N>
  struct define_class_init_helper {

      template <class ClassT, class CallPoliciesT, class ReversedArgs>
      static void apply(
          ClassT& cl
          , CallPoliciesT const& policies
          , ReversedArgs const& args
          , char const* doc
          , detail::keyword_range keywords)
      {
          def_init_reversed(cl, args, policies, doc, keywords);

          if (keywords.second > keywords.first)
              --keywords.second;
          
          typename mpl::pop_front<ReversedArgs>::type next;
          define_class_init_helper<N-1>::apply(cl, policies, next, doc, keywords);
      }
  };

  ///////////////////////////////////////////////////////////////////////////////
  //
  //  define_class_init_helper<0>::apply
  //
  //      Terminal case
  //
  //      Accepts a class_ and an arguments list. Defines a constructor
  //      for the class given the arguments.
  //
  ///////////////////////////////////////////////////////////////////////////////
  template <>
  struct define_class_init_helper<0> {

      template <class ClassT, class CallPoliciesT, class ReversedArgs>
      static void apply(
          ClassT& cl
          , CallPoliciesT const& policies
          , ReversedArgs const& args
          , char const* doc
          , detail::keyword_range const& keywords)
      {
          def_init_reversed(cl, args, policies, doc, keywords);
      }
  };
}

///////////////////////////////////////////////////////////////////////////////
//
//  define_init
//
//      Accepts a class_ and an init-list. Defines a set of constructors for
//      the class given the arguments. The init list (see init above) has
//      n_defaults (number of default arguments and n_arguments (number of
//      actual arguments). This function defines n_defaults + 1 constructors
//      for the class. Each constructor after the first has one less argument
//      to its right. Example:
//
//          init<int, default<char, long, double>
//
//      Defines:
//
//          __init__(int, char, long, double)
//          __init__(int, char, long)
//          __init__(int, char)
//          __init__(int)
//
///////////////////////////////////////////////////////////////////////////////
template <class ClassT, class InitT>
void
define_init(ClassT& cl, InitT const& i)
{
    typedef typename InitT::reversed_args reversed_args;
    detail::define_class_init_helper<InitT::n_defaults>::apply(
        cl, i.call_policies(), reversed_args(), i.doc_string(), i.keywords());
}

}} // namespace boost::python

#undef BOOST_PYTHON_OVERLOAD_TYPES_WITH_DEFAULT
#undef BOOST_PYTHON_OVERLOAD_TYPES
#undef BOOST_PYTHON_OVERLOAD_ARGS
#undef BOOST_PYTHON_IS_OPTIONAL_VALUE
#undef BOOST_PYTHON_APPEND_TO_INIT

///////////////////////////////////////////////////////////////////////////////
#endif // INIT_JDG20020820_HPP








