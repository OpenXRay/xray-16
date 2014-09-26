// (C) Copyright David Abrahams 2000. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// (C) Copyright Jeremy Siek 2000. Permission to copy, use, modify,
// sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.

//  See http://www.boost.org/libs/utility/iterator_adaptors.htm for documentation.

// Revision History:

// 01 Feb 2002   Jeremy Siek
//      Added more comments in default_iterator_policies.
// 08 Jan 2001   David Abrahams
//      Moved concept checks into a separate class, which makes MSVC
//      better at dealing with them.
// 07 Jan 2001   David Abrahams
//      Choose proxy for operator->() only if the reference type is not a reference.
//      Updated workarounds for __MWERKS__ == 0x2406
// 20 Dec 2001   David Abrahams
//      Adjusted is_convertible workarounds for __MWERKS__ == 0x2406
// 03 Nov 2001   Jeremy Siek
//      Changed the named template parameter interface and internal.
// 04 Oct 2001   Jeremy Siek
//      Changed projection_iterator to not rely on the default reference,
//      working around a limitation of detail::iterator_traits.
// 04 Oct 2001   David Abrahams
//      Applied indirect_iterator patch from George A. Heintzelman <georgeh@aya.yale.edu>
//      Changed name of "bind" to "select" to avoid problems with MSVC.
// 26 Sep 2001   David Abrahams
//      Added borland bug fix
// 08 Mar 2001   Jeremy Siek
//      Added support for optional named template parameters.
// 19 Feb 2001   David Abrahams
//      Rolled back reverse_iterator_pair_generator again, as it doesn't
//      save typing on a conforming compiler.
// 18 Feb 2001   David Abrahams
//      Reinstated reverse_iterator_pair_generator
// 16 Feb 2001   David Abrahams
//      Add an implicit conversion operator to operator_arrow_proxy
//      as CW and BCC workarounds.
// 11 Feb 2001   David Abrahams
//      Switch to use of BOOST_STATIC_CONSTANT where possible
// 11 Feb 2001   Jeremy Siek
//      Removed workaround for older MIPSpro compiler. The workaround
//        was preventing the proper functionality of the underlying
//        iterator being carried forward into the iterator adaptor.
//        Also added is_bidirectional enum to avoid EDG compiler error.
// 11 Feb 2001   David Abrahams
//      Borland fixes up the wazoo. It finally works!
// 10 Feb 2001   David Abrahams
//      Removed traits argument from iterator_adaptor<> and switched to
//        explicit trait specification for maximum ease-of-use.
//      Added comments to detail::iterator_defaults<>
//      Began using detail::iterator_defaults<> unconditionally for code clarity
//      Changed uses of `Iterator' to `Base' where non-iterators can be used.
//
// 10 Feb 2001   David Abrahams
//      Rolled in supposed Borland fixes from John Maddock, but not seeing any
//        improvement yet
//      Changed argument order to indirect_ generator, for convenience in the
//        case of input iterators (where Reference must be a value type).
//      Removed derivation of filter_iterator_policies from
//        default_iterator_policies, since the iterator category is likely to be
//        reduced (we don't want to allow illegal operations like decrement).
//      Support for a simpler filter iterator interface.
//
// 09 Feb 2001   David Abrahams
//      Improved interface to indirect_ and reverse_ iterators
//      Rolled back Jeremy's new constructor for now; it was causing
//        problems with counting_iterator_test
//      Attempted fix for Borland
//
// 09 Feb 2001   Jeremy Siek
//      Added iterator constructor to allow const adaptor
//        from non-const adaptee.
//      Changed make_xxx to pass iterators by-value to
//        get arrays converted to pointers.
//      Removed InnerIterator template parameter from
//        indirect_iterator_generator.
//      Rearranged parameters for make_filter_iterator
//
// 07 Feb 2001   Jeremy Siek
//      Removed some const iterator adaptor generators.
//      Added make_xxx_iterator() helper functions for remaining
//        iterator adaptors.
//      Removed some traits template parameters where they
//        where no longer needed thanks to detail::iterator_traits.
//      Moved some of the compile-time logic into enums for
//      EDG compatibility.
//
// 07 Feb 2001  David Abrahams
//      Removed iterator_adaptor_pair_generator and
//        reverse_iterator_pair_generator (more such culling to come)
//      Improved comments
//      Changed all uses of std::iterator_traits as default arguments
//        to boost::detail::iterator_traits for improved utility in
//        non-generic contexts
//      Fixed naming convention of non-template parameter names
//
// 06 Feb 2001   David Abrahams
//      Produce operator-> proxy objects for InputIterators
//      Added static assertions to do some basic concept checks
//      Renamed single-type generators -> xxx_generator
//      Renamed const/nonconst iterator generators -> xxx_pair_generator
//      Added make_transform_iterator(iter, function)
//      The existence of boost::detail::iterator_traits allowed many
//        template arguments to be defaulted. Some arguments had to be
//        moved to accomplish it.
//
// 04 Feb 2001  MWERKS bug workaround, concept checking for proper
//              reference types (David Abrahams)

#ifndef BOOST_ITERATOR_ADAPTOR_DWA053000_HPP_
# define BOOST_ITERATOR_ADAPTOR_DWA053000_HPP_

# include <boost/iterator.hpp>
# include <boost/utility.hpp>
# include <boost/compressed_pair.hpp>
# include <boost/concept_check.hpp>
# include <boost/type.hpp>
# include <boost/static_assert.hpp>
# include <boost/type_traits.hpp>
# include <boost/type_traits/conversion_traits.hpp>
# include <boost/detail/iterator.hpp>
# include <boost/detail/select_type.hpp>
# include <boost/detail/workaround.hpp>

# if BOOST_WORKAROUND(__GNUC__, == 2) && __GNUC_MINOR__ <= 96 && !defined(__STL_USE_NAMESPACES)
#  define BOOST_RELOPS_AMBIGUITY_BUG 1
# endif

namespace boost {

//============================================================================
// Concept checking classes that express the requirements for iterator
// policies and adapted types. These classes are mostly for
// documentation purposes, and are not used in this header file. They
// merely provide a more succinct statement of what is expected of the
// iterator policies.

template <class Policies, class Adapted, class Traits>
struct TrivialIteratorPoliciesConcept
{
  typedef typename Traits::reference reference;
  void constraints() {
    function_requires< AssignableConcept<Policies> >();
    function_requires< DefaultConstructibleConcept<Policies> >();
    function_requires< AssignableConcept<Adapted> >();
    function_requires< DefaultConstructibleConcept<Adapted> >();

    const_constraints();
  }
  void const_constraints() const {
    reference r = p.dereference(x);
    b = p.equal(x, x);
    ignore_unused_variable_warning(r);
  }
  Policies p;
  Adapted x;
  mutable bool b;
};

// Add InputIteratorPoliciesConcept?

template <class Policies, class Adapted, class Traits>
struct ForwardIteratorPoliciesConcept
{
  typedef typename Traits::iterator_category iterator_category;
  void constraints() {
    function_requires<
      TrivialIteratorPoliciesConcept<Policies, Adapted, Traits>
      >();

    p.increment(x);
    std::forward_iterator_tag t = iterator_category();
    ignore_unused_variable_warning(t);
  }
  Policies p;
  Adapted x;
  iterator_category category;
};

template <class Policies, class Adapted, class Traits>
struct BidirectionalIteratorPoliciesConcept
{
  typedef typename Traits::iterator_category iterator_category;
  void constraints() {
    function_requires<
      ForwardIteratorPoliciesConcept<Policies, Adapted, Traits>
      >();

    p.decrement(x);
    std::bidirectional_iterator_tag t = iterator_category();
    ignore_unused_variable_warning(t);
  }
  Policies p;
  Adapted x;
};

template <class Policies, class Adapted, class Traits>
struct RandomAccessIteratorPoliciesConcept
{
  typedef typename Traits::difference_type DifferenceType;
  typedef typename Traits::iterator_category iterator_category;
  void constraints() {
    function_requires<
      BidirectionalIteratorPoliciesConcept<Policies, Adapted, Traits>
      >();

    p.advance(x, n);
    std::random_access_iterator_tag t = iterator_category();
    const_constraints();
    ignore_unused_variable_warning(t);
  }
  void const_constraints() const {
    n = p.distance(x, x);
  }
  Policies p;
  Adapted x;
  mutable DifferenceType n;
  mutable bool b;
};


//============================================================================
// Default policies for iterator adaptors. You can use this as a base
// class if you want to customize particular policies.
struct default_iterator_policies
{
    // Some of the member functions were defined static, but Borland
    // got confused and thought they were non-const. Also, Sun C++
    // does not like static function templates. 
    //
    // The reason some members were defined static is because there is
    // not state (data members) needed by those members of the
    // default_iterator_policies class. If your policies class member
    // functions need to access state stored in the policies object,
    // then the member functions should not be static (they can't be).

    template <class Base>
    void initialize(Base&)
        { }

    template <class IteratorAdaptor>
    typename IteratorAdaptor::reference dereference(const IteratorAdaptor& x) const
        { return *x.base(); }

    template <class IteratorAdaptor>
    void increment(IteratorAdaptor& x)
        { ++x.base(); }

    template <class IteratorAdaptor>
    void decrement(IteratorAdaptor& x)
        { --x.base(); }

    template <class IteratorAdaptor, class DifferenceType>
    void advance(IteratorAdaptor& x, DifferenceType n)
        { x.base() += n; }

    template <class IteratorAdaptor1, class IteratorAdaptor2>
    typename IteratorAdaptor1::difference_type
    distance(const IteratorAdaptor1& x, const IteratorAdaptor2& y) const
        { return y.base() - x.base(); }

    template <class IteratorAdaptor1, class IteratorAdaptor2>
    bool equal(const IteratorAdaptor1& x, const IteratorAdaptor2& y) const
        { return x.base() == y.base(); }
};

// putting the comparisons in a base class avoids the g++
// ambiguous overload bug due to the relops operators

#ifdef BOOST_RELOPS_AMBIGUITY_BUG
template <class Derived, class Base>
struct iterator_comparisons : Base { };

template <class D1, class D2, class Base1, class Base2>
inline bool operator==(const iterator_comparisons<D1,Base1>& xb,
                       const iterator_comparisons<D2,Base2>& yb)
{
        const D1& x = static_cast<const D1&>(xb);
    const D2& y = static_cast<const D2&>(yb);
    return x.policies().equal(x, y);
}

template <class D1, class D2, class Base1, class Base2>
inline bool operator!=(const iterator_comparisons<D1,Base1>& xb,
                       const iterator_comparisons<D2,Base2>& yb)
{
    const D1& x = static_cast<const D1&>(xb);
    const D2& y = static_cast<const D2&>(yb);
    return !x.policies().equal(x, y);
}

template <class D1, class D2, class Base1, class Base2>
inline bool operator<(const iterator_comparisons<D1,Base1>& xb,
                      const iterator_comparisons<D2,Base2>& yb)
{
    const D1& x = static_cast<const D1&>(xb);
    const D2& y = static_cast<const D2&>(yb);
    return x.policies().distance(y, x) < 0;
}

template <class D1, class D2, class Base1, class Base2>
inline bool operator>(const iterator_comparisons<D1,Base1>& xb,
                      const iterator_comparisons<D2,Base2>& yb)
{
    const D1& x = static_cast<const D1&>(xb);
    const D2& y = static_cast<const D2&>(yb);
    return x.policies().distance(y, x) > 0;
}

template <class D1, class D2, class Base1, class Base2>
inline bool operator>=(const iterator_comparisons<D1,Base1>& xb,
                       const iterator_comparisons<D2,Base2>& yb)
{
    const D1& x = static_cast<const D1&>(xb);
    const D2& y = static_cast<const D2&>(yb);
    return x.policies().distance(y, x) >= 0;
}

template <class D1, class D2, class Base1, class Base2>
inline bool operator<=(const iterator_comparisons<D1,Base1>& xb,
                       const iterator_comparisons<D2,Base2>& yb)
{
    const D1& x = static_cast<const D1&>(xb);
    const D2& y = static_cast<const D2&>(yb);
    return x.policies().distance(y, x) <= 0;
}
#endif

namespace detail {

  // operator->() needs special support for input iterators to strictly meet the
  // standard's requirements. If *i is not a reference type, we must still
  // produce a (constant) lvalue to which a pointer can be formed. We do that by
  // returning an instantiation of this special proxy class template.

  template <class T>
  struct operator_arrow_proxy
  {
      operator_arrow_proxy(const T& x) : m_value(x) {}
      const T* operator->() const { return &m_value; }
      // This function is needed for MWCW and BCC, which won't call operator->
      // again automatically per 13.3.1.2 para 8
      operator const T*() const { return &m_value; }
      T m_value;
  };

  template <class Iter>
  inline operator_arrow_proxy<typename Iter::value_type>
  operator_arrow(const Iter& i, std::input_iterator_tag) {
    typedef typename Iter::value_type value_t; // VC++ needs this typedef
    return operator_arrow_proxy<value_t>(*i);
  }

  template <class Iter>
  inline typename Iter::pointer
  operator_arrow(const Iter& i, std::forward_iterator_tag) {
    return &(*i);
  }

  template <class Value, class Reference, class Pointer>
  struct operator_arrow_result_generator
  {
      typedef operator_arrow_proxy<Value> proxy;
      // Borland chokes unless it's an actual enum (!)
      enum { use_proxy = !boost::is_reference<Reference>::value };

      typedef typename boost::detail::if_true<(use_proxy)>::template
      then<
        proxy,
   // else
        Pointer
      >::type type;
  };


# if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) || defined(BOOST_NO_STD_ITERATOR_TRAITS)

   // Select default pointer and reference types for adapted non-pointer
   // iterators based on the iterator and the value_type. Poor man's partial
   // specialization is in use here.
   template <bool is_pointer>
   struct iterator_defaults_select
   {
       template <class Iterator,class Value>
       struct traits
       {
           // The assumption is that iterator_traits can deduce these types
           // properly as long as the iterator is not a pointer.
           typedef typename boost::detail::iterator_traits<Iterator>::pointer pointer;
           typedef typename boost::detail::iterator_traits<Iterator>::reference reference;
       };
   };

   // Select default pointer and reference types for adapted pointer iterators
   // given a (possibly-const) value_type.
   template <>
   struct iterator_defaults_select<true>
   {
       template <class Iterator,class Value>
       struct traits
       {
           typedef Value* pointer;
           typedef Value& reference;
       };
   };

   // Consolidate selection of the default pointer and reference type
   template <class Iterator,class Value>
   struct iterator_defaults
   {
       BOOST_STATIC_CONSTANT(bool, is_ptr = boost::is_pointer<Iterator>::value);

       typedef typename iterator_defaults_select<is_ptr>::template traits<Iterator,Value> traits;
       typedef typename traits::pointer pointer;
       typedef typename traits::reference reference;
   };
# else
   template <class Iterator,class Value>
   struct iterator_defaults : iterator_traits<Iterator>
   {
       // Trying to factor the common is_same expression into an enum or a
       // static bool constant confused Borland.
       typedef typename if_true<(
               ::boost::is_same<Value,typename iterator_traits<Iterator>::value_type>::value
           )>::template then<
                typename iterator_traits<Iterator>::pointer,
                Value*
       >::type pointer;

       typedef typename if_true<(
               ::boost::is_same<Value,typename iterator_traits<Iterator>::value_type>::value
           )>::template then<
                typename iterator_traits<Iterator>::reference,
                Value&
       >::type reference;

   };
# endif

  //===========================================================================
  // Specify the defaults for iterator_adaptor's template parameters

  struct default_argument { };
  // This class template is a workaround for MSVC.
  struct dummy_default_gen {
    template <class Base, class Traits>
    struct select { typedef default_argument type; };
  };
  // This class template is a workaround for MSVC.
  template <class Gen> struct default_generator {
    typedef dummy_default_gen type;
  };

  struct default_value_type {
    template <class Base, class Traits>
    struct select {
      typedef typename boost::detail::iterator_traits<Base>::value_type type;
    };
  };
  template <> struct default_generator<default_value_type>
  { typedef default_value_type type; }; // VC++ workaround

  struct default_difference_type {
    template <class Base, class Traits>
    struct select {
      typedef typename boost::detail::iterator_traits<Base>::difference_type type;
    };
  };
  template <> struct default_generator<default_difference_type>
  { typedef default_difference_type type; }; // VC++ workaround

  struct default_iterator_category {
    template <class Base, class Traits>
    struct select {
      typedef typename boost::detail::iterator_traits<Base>::iterator_category type;
    };
  };
  template <> struct default_generator<default_iterator_category>
  { typedef default_iterator_category type; }; // VC++ workaround

  struct default_pointer {
    template <class Base, class Traits>
    struct select {
      typedef typename Traits::value_type Value;
      typedef typename boost::detail::iterator_defaults<Base,Value>::pointer
        type;
    };
  };
  template <> struct default_generator<default_pointer>
  { typedef default_pointer type; }; // VC++ workaround

  struct default_reference {
    template <class Base, class Traits>
    struct select {
      typedef typename Traits::value_type Value;
      typedef typename boost::detail::iterator_defaults<Base,Value>::reference
        type;
    };
  };
  template <> struct default_generator<default_reference>
  { typedef default_reference type; }; // VC++ workaround

} // namespace detail


  //===========================================================================
  // Support for named template parameters

struct named_template_param_base { };

namespace detail {
  struct value_type_tag { };
  struct reference_tag { };
  struct pointer_tag { };
  struct difference_type_tag { };
  struct iterator_category_tag { };

  // avoid using std::pair because A or B might be a reference type, and g++
  // complains about forming references to references inside std::pair
  template <class A, class B>
  struct cons_type {
    typedef A first_type;
    typedef B second_type;
  };

} // namespace detail

template <class Value> struct value_type_is : public named_template_param_base
{
  typedef detail::cons_type<detail::value_type_tag, Value> type;
};
template <class Reference> struct reference_is : public named_template_param_base
{
  typedef detail::cons_type<detail::reference_tag, Reference> type;
};
template <class Pointer> struct pointer_is : public named_template_param_base
{
  typedef detail::cons_type<detail::pointer_tag, Pointer> type;
};
template <class Difference> struct difference_type_is
  : public named_template_param_base
{
  typedef detail::cons_type<detail::difference_type_tag, Difference> type;
};
template <class IteratorCategory> struct iterator_category_is
  : public named_template_param_base
{
  typedef detail::cons_type<detail::iterator_category_tag, IteratorCategory> type;
};

namespace detail {

  struct end_of_list { };

  // Given an associative list, find the value with the matching key.
  // An associative list is a list of key-value pairs. The list is
  // built out of cons_type's and is terminated by end_of_list.

# if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) || BOOST_WORKAROUND(__BORLANDC__, != 0)
  template <class AssocList, class Key>
  struct find_param;

  struct find_param_continue {
    template <class AssocList, class Key2> struct select {
      typedef typename AssocList::first_type Head;
      typedef typename Head::first_type Key1;
      typedef typename Head::second_type Value;
      typedef typename if_true<(is_same<Key1, Key2>::value)>::template
      then<Value,
        typename find_param<typename AssocList::second_type, Key2>::type
      >::type type;
    };
  };
  struct find_param_end {
    template <class AssocList, class Key>
    struct select { typedef detail::default_argument type; };
  };
  template <class AssocList> struct find_param_helper1
  { typedef find_param_continue type; };
  template <> struct find_param_helper1<end_of_list>
  { typedef find_param_end type; };

  template <class AssocList, class Key>
  struct find_param {
    typedef typename find_param_helper1<AssocList>::type select1;
    typedef typename select1::template select<AssocList, Key>::type type;
  };
# else
  template <class AssocList, class Key> struct find_param;

  template <class Key>
  struct find_param<end_of_list, Key> { typedef default_argument type; };

  // Found a matching Key, return the associated Value
  template <class Key, class Value, class Rest>
  struct find_param<detail::cons_type< detail::cons_type<Key, Value>, Rest>, Key> {
    typedef Value type;
  };

  // Non-matching keys, continue the search
  template <class Key1, class Value, class Rest, class Key2>
  struct find_param<detail::cons_type< detail::cons_type<Key1, Value>, Rest>, Key2> {
    typedef typename find_param<Rest, Key2>::type type;
  };
# endif

  struct make_named_arg {
    template <class Key, class Value>
    struct select { typedef typename Value::type type; };
  };
  struct make_key_value {
    template <class Key, class Value>
    struct select { typedef detail::cons_type<Key, Value> type; };
  };

  template <class Value>
  struct is_named_parameter
  {
      enum { value = is_convertible< typename add_reference< Value >::type, add_reference< named_template_param_base >::type >::value };
  };

# if BOOST_WORKAROUND(__MWERKS__, <= 0x2407) // workaround for broken is_convertible implementation
  template <class T> struct is_named_parameter<value_type_is<T> > { enum { value = true }; };
  template <class T> struct is_named_parameter<reference_is<T> > { enum { value = true }; };
  template <class T> struct is_named_parameter<pointer_is<T> > { enum { value = true }; };
  template <class T> struct is_named_parameter<difference_type_is<T> > { enum { value = true }; };
  template <class T> struct is_named_parameter<iterator_category_is<T> > { enum { value = true }; };
# endif

  template <class Key, class Value>
  struct make_arg {
# if BOOST_WORKAROUND(__BORLANDC__, > 0)
    // Borland C++ doesn't like the extra indirection of is_named_parameter
    typedef typename
      if_true<(is_convertible<Value,named_template_param_base>::value)>::
      template then<make_named_arg, make_key_value>::type Make;
# else
    enum { is_named = is_named_parameter<Value>::value };
    typedef typename if_true<(is_named)>::template
      then<make_named_arg, make_key_value>::type Make;
# endif
    typedef typename Make::template select<Key, Value>::type type;
  };

  // Mechanism for resolving the default argument for a template parameter.

  template <class T> struct is_default { typedef type_traits::no_type type; };
  template <> struct is_default<default_argument>
  { typedef type_traits::yes_type type; };

  struct choose_default {
    template <class Arg, class DefaultGen, class Base, class Traits>
    struct select {
      typedef typename default_generator<DefaultGen>::type Gen;
      typedef typename Gen::template select<Base,Traits>::type type;
    };
  };
  struct choose_arg {
    template <class Arg, class DefaultGen, class Base, class Traits>
    struct select {
      typedef Arg type;
    };
  };

  template <class UseDefault>
  struct choose_arg_or_default { typedef choose_arg type; };
  template <> struct choose_arg_or_default<type_traits::yes_type> {
    typedef choose_default type;
  };

  template <class Arg, class DefaultGen, class Base, class Traits>
  class resolve_default {
    typedef typename choose_arg_or_default<typename is_default<Arg>::type>::type
      Selector;
  public:
    typedef typename Selector
      ::template select<Arg, DefaultGen, Base, Traits>::type type;
  };

  template <class Base, class Value, class Reference, class Pointer,
            class Category, class Distance>
  class iterator_adaptor_traits_gen
  {
    // Form an associative list out of the template parameters
    // If the argument is a normal parameter (not named) then make_arg
    // creates a key-value pair. If the argument is a named parameter,
    // then make_arg extracts the key-value pair defined inside the
    // named parameter.
    typedef detail::cons_type< typename make_arg<value_type_tag, Value>::type,
      detail::cons_type<typename make_arg<reference_tag, Reference>::type,
      detail::cons_type<typename make_arg<pointer_tag, Pointer>::type,
      detail::cons_type<typename make_arg<iterator_category_tag, Category>::type,
      detail::cons_type<typename make_arg<difference_type_tag, Distance>::type,
                end_of_list> > > > > ArgList;

    // Search the list for particular parameters
    typedef typename find_param<ArgList, value_type_tag>::type Val;
    typedef typename find_param<ArgList, difference_type_tag>::type Diff;
    typedef typename find_param<ArgList, iterator_category_tag>::type Cat;
    typedef typename find_param<ArgList, pointer_tag>::type Ptr;
    typedef typename find_param<ArgList, reference_tag>::type Ref;

    typedef boost::iterator<Category, Value, Distance, Pointer, Reference>
      Traits0;

    // Compute the defaults if necessary
    typedef typename resolve_default<Val, default_value_type, Base, Traits0>::type
      value_type;
    // if getting default value type from iterator_traits, then it won't be const
    typedef typename resolve_default<Diff, default_difference_type, Base,
      Traits0>::type difference_type;
    typedef typename resolve_default<Cat, default_iterator_category, Base,
      Traits0>::type iterator_category;

    typedef boost::iterator<iterator_category, value_type, difference_type,
      Pointer, Reference> Traits1;

    // Compute the defaults for pointer and reference. This is done as a
    // separate step because the defaults for pointer and reference depend
    // on value_type.
    typedef typename resolve_default<Ptr, default_pointer, Base, Traits1>::type
      pointer;
    typedef typename resolve_default<Ref, default_reference, Base, Traits1>::type
      reference;

  public:
    typedef boost::iterator<iterator_category,
      typename remove_const<value_type>::type,
      difference_type, pointer, reference> type;
  };

  // This is really a partial concept check for iterators. Should it
  // be moved or done differently?
  template <class Category, class Value, class Difference, class Pointer, class Reference>
  struct validator
  {
      BOOST_STATIC_CONSTANT(
          bool, is_input_or_output_iter
          = (boost::is_convertible<Category*,std::input_iterator_tag*>::value
             | boost::is_convertible<Category*,std::output_iterator_tag*>::value));

      // Iterators should satisfy one of the known categories
      BOOST_STATIC_ASSERT(is_input_or_output_iter);

      // Iterators >= ForwardIterator must produce real references
      // as required by the C++ standard requirements in Table 74.
      BOOST_STATIC_CONSTANT(
          bool, forward_iter_with_real_reference
          = ((!boost::is_convertible<Category*,std::forward_iterator_tag*>::value)
             | boost::is_same<Reference,Value&>::value
             | boost::is_same<Reference,typename add_const<Value>::type&>::value));

      BOOST_STATIC_ASSERT(forward_iter_with_real_reference);
  };

  template <class T, class Result> struct dependent
  {
    typedef Result type;
  };

} // namespace detail



// This macro definition is only temporary in this file
# if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
#  define BOOST_ARG_DEPENDENT_TYPENAME typename
# else
#  define BOOST_ARG_DEPENDENT_TYPENAME
# endif

//============================================================================
//iterator_adaptor - Adapts a generic piece of data as an iterator. Adaptation
//      is especially easy if the data being adapted is itself an iterator
//
//   Base - the base (usually iterator) type being wrapped.
//
//   Policies - a set of policies determining how the resulting iterator
//      works.
//
//   Value - if supplied, the value_type of the resulting iterator, unless
//      const. If const, a conforming compiler strips constness for the
//      value_type. If not supplied, iterator_traits<Base>::value_type is used
//
//   Reference - the reference type of the resulting iterator, and in
//      particular, the result type of operator*(). If not supplied but
//      Value is supplied, Value& is used. Otherwise
//      iterator_traits<Base>::reference is used.
//
//   Pointer - the pointer type of the resulting iterator, and in
//      particular, the result type of operator->(). If not
//      supplied but Value is supplied, Value* is used. Otherwise
//      iterator_traits<Base>::pointer is used.
//
//   Category - the iterator_category of the resulting iterator. If not
//      supplied, iterator_traits<Base>::iterator_category is used.
//
//   Distance - the difference_type of the resulting iterator. If not
//      supplied, iterator_traits<Base>::difference_type is used.
template <class Base, class Policies,
    class Value = ::boost::detail::default_argument,
    class Reference = ::boost::detail::default_argument,
    class Pointer = ::boost::detail::default_argument,
    class Category = ::boost::detail::default_argument,
    class Distance = ::boost::detail::default_argument
         >
struct iterator_adaptor :
#ifdef BOOST_RELOPS_AMBIGUITY_BUG
    iterator_comparisons<
          iterator_adaptor<Base,Policies,Value,Reference,Pointer,Category,Distance>,
    typename detail::iterator_adaptor_traits_gen<Base,Value,Reference,Pointer,Category, Distance>::type
 >
#else
    detail::iterator_adaptor_traits_gen<Base,Value,Reference,Pointer,Category,Distance>::type
#endif
{
    typedef iterator_adaptor<Base,Policies,Value,Reference,Pointer,Category,Distance> self;
 public:
    typedef detail::iterator_adaptor_traits_gen<Base,Value,Reference,Pointer,Category,Distance> TraitsGen;
    typedef typename TraitsGen::type Traits;

    typedef typename Traits::difference_type difference_type;
    typedef typename Traits::value_type value_type;
    typedef typename Traits::pointer pointer;
    typedef typename Traits::reference reference;
    typedef typename Traits::iterator_category iterator_category;

    typedef Base base_type;
    typedef Policies policies_type;

 private:
    typedef detail::validator<
        iterator_category,value_type,difference_type,pointer,reference
        > concept_check;

 public:
    iterator_adaptor()
    {
    }

    explicit
    iterator_adaptor(const Base& it, const Policies& p = Policies())
        : m_iter_p(it, p) {
        policies().initialize(base());
    }

    template <class Iter2, class Value2, class Pointer2, class Reference2>
    iterator_adaptor (
        const iterator_adaptor<Iter2,Policies,Value2,Reference2,Pointer2,Category,Distance>& src)
            : m_iter_p(src.base(), src.policies())
    {
        policies().initialize(base());
    }

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1300) || BOOST_WORKAROUND(__BORLANDC__, > 0)
    // This is required to prevent a bug in how VC++ generates
    // the assignment operator for compressed_pair
    iterator_adaptor& operator= (const iterator_adaptor& x) {
        m_iter_p = x.m_iter_p;
        return *this;
    }
#endif
    reference operator*() const {
         return policies().dereference(*this);
    }

#if BOOST_WORKAROUND(BOOST_MSVC, > 0)
# pragma warning(push)
# pragma warning( disable : 4284 )
#endif

    typename boost::detail::operator_arrow_result_generator<value_type,reference,pointer>::type
    operator->() const
        { return detail::operator_arrow(*this, iterator_category()); }

#if BOOST_WORKAROUND(BOOST_MSVC, > 0)
# pragma warning(pop)
#endif

    template <class diff_type>
    typename detail::dependent<diff_type, value_type>::type operator[](diff_type n) const
        { return *(*this + n); }

    self& operator++() {
#if !BOOST_WORKAROUND(__MWERKS__, <  0x2405)
        policies().increment(*this);
#else
        // Odd bug, MWERKS couldn't  deduce the type for the member template
        // Workaround by explicitly specifying the type.
        policies().increment<self>(*this);
#endif
        return *this;
    }

    self operator++(int) { self tmp(*this); ++*this; return tmp; }

    self& operator--() {
#if !BOOST_WORKAROUND(__MWERKS__, <  0x2405)
        policies().decrement(*this);
#else
        policies().decrement<self>(*this);
#endif
        return *this;
    }

    self operator--(int) { self tmp(*this); --*this; return tmp; }

    self& operator+=(difference_type n) {
        policies().advance(*this, n);
        return *this;
    }

    self& operator-=(difference_type n) {
        policies().advance(*this, -n);
        return *this;
    }

    base_type const& base() const { return m_iter_p.first(); }

    // Moved from global scope to avoid ambiguity with the operator-() which
    // subtracts iterators from one another.
    self operator-(difference_type x) const
        { self result(*this); return result -= x; }
private:
    compressed_pair<Base,Policies> m_iter_p;

public: // implementation details (too many compilers have trouble when these are private).
    base_type& base() { return m_iter_p.first(); }
    Policies& policies() { return m_iter_p.second(); }
    const Policies& policies() const { return m_iter_p.second(); }
};

template <class Base, class Policies, class Value, class Reference, class Pointer,
    class Category, class Distance1, class Distance2>
iterator_adaptor<Base,Policies,Value,Reference,Pointer,Category,Distance1>
operator+(
    iterator_adaptor<Base,Policies,Value,Reference,Pointer,Category,Distance1> p,
    Distance2 x)
{
    return p += x;
}

template <class Base, class Policies, class Value, class Reference, class Pointer,
    class Category, class Distance1, class Distance2>
iterator_adaptor<Base,Policies,Value,Reference,Pointer,Category,Distance1>
operator+(
    Distance2 x,
    iterator_adaptor<Base,Policies,Value,Reference,Pointer,Category,Distance1> p)
{
    return p += x;
}

template <class Iterator1, class Iterator2, class Policies, class Value1, class Value2,
    class Reference1, class Reference2, class Pointer1, class Pointer2, class Category,
    class Distance>
typename iterator_adaptor<Iterator1,Policies,Value1,Reference1,Pointer1,Category,Distance>::difference_type
operator-(
    const iterator_adaptor<Iterator1,Policies,Value1,Reference1,Pointer1,Category,Distance>& x,
    const iterator_adaptor<Iterator2,Policies,Value2,Reference2,Pointer2,Category,Distance>& y)
{
  typedef typename iterator_adaptor<Iterator1,Policies,Value1,Reference1,
    Pointer1,Category,Distance>::difference_type difference_type;
  return x.policies().distance(y, x);
}

#ifndef BOOST_RELOPS_AMBIGUITY_BUG
template <class Iterator1, class Iterator2, class Policies, class Value1, class Value2,
    class Reference1, class Reference2, class Pointer1, class Pointer2,
    class Category, class Distance>
inline bool
operator==(
    const iterator_adaptor<Iterator1,Policies,Value1,Reference1,Pointer1,Category,Distance>& x,
    const iterator_adaptor<Iterator2,Policies,Value2,Reference2,Pointer2,Category,Distance>& y)
{
    return x.policies().equal(x, y);
}

template <class Iterator1, class Iterator2, class Policies, class Value1, class Value2,
    class Reference1, class Reference2, class Pointer1, class Pointer2,
    class Category, class Distance>
inline bool
operator<(
    const iterator_adaptor<Iterator1,Policies,Value1,Reference1,Pointer1,Category,Distance>& x,
    const iterator_adaptor<Iterator2,Policies,Value2,Reference2,Pointer2,Category,Distance>& y)
{
    return x.policies().distance(y, x) < 0;
}

template <class Iterator1, class Iterator2, class Policies, class Value1, class Value2,
    class Reference1, class Reference2, class Pointer1, class Pointer2,
    class Category, class Distance>
inline bool
operator>(
    const iterator_adaptor<Iterator1,Policies,Value1,Reference1,Pointer1,Category,Distance>& x,
    const iterator_adaptor<Iterator2,Policies,Value2,Reference2,Pointer2,Category,Distance>& y)
{
    return x.policies().distance(y, x) > 0;
}

template <class Iterator1, class Iterator2, class Policies, class Value1, class Value2,
    class Reference1, class Reference2, class Pointer1, class Pointer2,
    class Category, class Distance>
inline bool
operator>=(
    const iterator_adaptor<Iterator1,Policies,Value1,Reference1,Pointer1,Category,Distance>& x,
    const iterator_adaptor<Iterator2,Policies,Value2,Reference2,Pointer2,Category,Distance>& y)
{
    return x.policies().distance(y, x) >= 0;
}

template <class Iterator1, class Iterator2, class Policies, class Value1, class Value2,
    class Reference1, class Reference2, class Pointer1, class Pointer2,
    class Category, class Distance>
inline bool
operator<=(
    const iterator_adaptor<Iterator1,Policies,Value1,Reference1,Pointer1,Category,Distance>& x,
    const iterator_adaptor<Iterator2,Policies,Value2,Reference2,Pointer2,Category,Distance>& y)
{
    return x.policies().distance(y, x) <= 0;
}

template <class Iterator1, class Iterator2, class Policies, class Value1, class Value2,
    class Reference1, class Reference2, class Pointer1, class Pointer2,
    class Category, class Distance>
inline bool
operator!=(
    const iterator_adaptor<Iterator1,Policies,Value1,Reference1,Pointer1,Category,Distance>& x,
    const iterator_adaptor<Iterator2,Policies,Value2,Reference2,Pointer2,Category,Distance>& y)
{
    return !x.policies().equal(x, y);
}
#endif

//=============================================================================
// Transform Iterator Adaptor
//
// Upon deference, apply some unary function object and return the
// result by value.

template <class AdaptableUnaryFunction>
struct transform_iterator_policies : public default_iterator_policies
{
    transform_iterator_policies() { }
    transform_iterator_policies(const AdaptableUnaryFunction& f) : m_f(f) { }

    template <class IteratorAdaptor>
    typename IteratorAdaptor::reference
    dereference(const IteratorAdaptor& iter) const
        { return m_f(*iter.base()); }

    AdaptableUnaryFunction m_f;
};

template <class AdaptableUnaryFunction, class Iterator>
class transform_iterator_generator
{
    typedef typename AdaptableUnaryFunction::result_type value_type;
public:
    typedef iterator_adaptor<Iterator,
      transform_iterator_policies<AdaptableUnaryFunction>,
        value_type, value_type, value_type*, std::input_iterator_tag>
      type;
};

template <class AdaptableUnaryFunction, class Iterator>
inline typename transform_iterator_generator<AdaptableUnaryFunction,Iterator>::type
make_transform_iterator(
    Iterator base,
    const AdaptableUnaryFunction& f = AdaptableUnaryFunction())
{
    typedef typename transform_iterator_generator<AdaptableUnaryFunction,Iterator>::type result_t;
    return result_t(base, f);
}

//=============================================================================
// Indirect Iterators Adaptor

// Given a pointer to pointers (or iterator to iterators),
// apply a double dereference inside operator*().
//
// We use the term "outer" to refer to the first level iterator type
// and "inner" to refer to the second level iterator type.  For
// example, given T**, T* is the inner iterator type and T** is the
// outer iterator type. Also, const T* would be the const inner
// iterator.

// We tried to implement this with transform_iterator, but that required
// using boost::remove_ref, which is not compiler portable.

struct indirect_iterator_policies : public default_iterator_policies
{
    template <class IteratorAdaptor>
    typename IteratorAdaptor::reference dereference(const IteratorAdaptor& x) const
        { return **x.base(); }
};

namespace detail {
# if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300) // strangely instantiated even when unused! Maybe try a recursive template someday ;-)
  template <class T>
  struct traits_of_value_type {
      typedef typename boost::detail::iterator_traits<T>::value_type outer_value;
      typedef typename boost::detail::iterator_traits<outer_value>::value_type value_type;
      typedef typename boost::detail::iterator_traits<outer_value>::reference reference;
      typedef typename boost::detail::iterator_traits<outer_value>::pointer pointer;
  };
# endif
}

template <class OuterIterator,      // Mutable or Immutable, does not matter
          class Value
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
                = BOOST_ARG_DEPENDENT_TYPENAME detail::traits_of_value_type<
                        OuterIterator>::value_type
#endif
          , class Reference
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
                = BOOST_ARG_DEPENDENT_TYPENAME detail::traits_of_value_type<
                        OuterIterator>::reference
#else
                = Value &
#endif
          , class Category = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_traits<
                        OuterIterator>::iterator_category
          , class Pointer
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
                = BOOST_ARG_DEPENDENT_TYPENAME detail::traits_of_value_type<
                        OuterIterator>::pointer
#else
                = Value*
#endif
         >
struct indirect_iterator_generator
{
    typedef iterator_adaptor<OuterIterator,
        indirect_iterator_policies,Value,Reference,Pointer,Category> type;
};

template <class OuterIterator,      // Mutable or Immutable, does not matter
          class Value
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
                = BOOST_ARG_DEPENDENT_TYPENAME detail::traits_of_value_type<
                        OuterIterator>::value_type
#endif
          , class Reference
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
                = BOOST_ARG_DEPENDENT_TYPENAME detail::traits_of_value_type<
                        OuterIterator>::reference
#else
                = Value &
#endif
          , class ConstReference = Value const&
          , class Category = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_traits<
                OuterIterator>::iterator_category
          , class Pointer
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
                = BOOST_ARG_DEPENDENT_TYPENAME detail::traits_of_value_type<
                        OuterIterator>::pointer
#else
                = Value*
#endif
          , class ConstPointer = Value const*
           >
struct indirect_iterator_pair_generator
{
  typedef typename indirect_iterator_generator<OuterIterator,
    Value, Reference,Category,Pointer>::type iterator;
  typedef typename indirect_iterator_generator<OuterIterator,
    Value, ConstReference,Category,ConstPointer>::type const_iterator;
};

#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
template <class OuterIterator>
inline typename indirect_iterator_generator<OuterIterator>::type
make_indirect_iterator(OuterIterator base)
{
    typedef typename indirect_iterator_generator
        <OuterIterator>::type result_t;
    return result_t(base);
}
#endif

//=============================================================================
// Reverse Iterators Adaptor

struct reverse_iterator_policies : public default_iterator_policies
{
    template <class IteratorAdaptor>
    typename IteratorAdaptor::reference dereference(const IteratorAdaptor& x) const
        { return *boost::prior(x.base()); }

    template <class BidirectionalIterator>
    void increment(BidirectionalIterator& x) const
        { --x.base(); }

    template <class BidirectionalIterator>
    void decrement(BidirectionalIterator& x) const
        { ++x.base(); }

    template <class BidirectionalIterator, class DifferenceType>
    void advance(BidirectionalIterator& x, DifferenceType n) const
        { x.base() -= n; }

    template <class Iterator1, class Iterator2>
    typename Iterator1::difference_type distance(
        const Iterator1& x, const Iterator2& y) const
        { return x.base() - y.base(); }

    template <class Iterator1, class Iterator2>
    bool equal(const Iterator1& x, const Iterator2& y) const
        { return x.base() == y.base(); }
};

template <class BidirectionalIterator,
    class Value = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_traits<BidirectionalIterator>::value_type,
    class Reference = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_defaults<BidirectionalIterator,Value>::reference,
    class Pointer = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_defaults<BidirectionalIterator,Value>::pointer,
    class Category = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_traits<BidirectionalIterator>::iterator_category,
    class Distance = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_traits<BidirectionalIterator>::difference_type
         >
struct reverse_iterator_generator
{
    typedef iterator_adaptor<BidirectionalIterator,reverse_iterator_policies,
        Value,Reference,Pointer,Category,Distance> type;
};

template <class BidirectionalIterator>
inline typename reverse_iterator_generator<BidirectionalIterator>::type
make_reverse_iterator(BidirectionalIterator base)
{
    typedef typename reverse_iterator_generator<BidirectionalIterator>::type result_t;
    return result_t(base);
}

//=============================================================================
// Projection Iterators Adaptor

template <class AdaptableUnaryFunction>
struct projection_iterator_policies : public default_iterator_policies
{
    projection_iterator_policies() { }
    projection_iterator_policies(const AdaptableUnaryFunction& f) : m_f(f) { }

    template <class IteratorAdaptor>
    typename IteratorAdaptor::reference dereference(IteratorAdaptor const& iter) const {
        return m_f(*iter.base());
    }

    AdaptableUnaryFunction m_f;
};

template <class AdaptableUnaryFunction, class Iterator>
class projection_iterator_generator {
    typedef typename AdaptableUnaryFunction::result_type value_type;
    typedef projection_iterator_policies<AdaptableUnaryFunction> policies;
public:
    typedef iterator_adaptor<Iterator,policies,value_type,value_type&,value_type*> type;
};

template <class AdaptableUnaryFunction, class Iterator>
class const_projection_iterator_generator {
    typedef typename AdaptableUnaryFunction::result_type value_type;
    typedef projection_iterator_policies<AdaptableUnaryFunction> policies;
public:
    typedef iterator_adaptor<Iterator,policies,value_type,const value_type&,const value_type*> type;
};

template <class AdaptableUnaryFunction, class Iterator, class ConstIterator>
struct projection_iterator_pair_generator {
    typedef typename projection_iterator_generator<AdaptableUnaryFunction, Iterator>::type iterator;
    typedef typename const_projection_iterator_generator<AdaptableUnaryFunction, ConstIterator>::type const_iterator;
};


template <class AdaptableUnaryFunction, class Iterator>
inline typename projection_iterator_generator<AdaptableUnaryFunction, Iterator>::type
make_projection_iterator(
    Iterator iter,
    const AdaptableUnaryFunction& f = AdaptableUnaryFunction())
{
    typedef typename projection_iterator_generator<AdaptableUnaryFunction, Iterator>::type result_t;
    return result_t(iter, f);
}

template <class AdaptableUnaryFunction, class Iterator>
inline typename const_projection_iterator_generator<AdaptableUnaryFunction, Iterator>::type
make_const_projection_iterator(
    Iterator iter,
    const AdaptableUnaryFunction& f = AdaptableUnaryFunction())
{
    typedef typename const_projection_iterator_generator<AdaptableUnaryFunction, Iterator>::type result_t;
    return result_t(iter, f);
}

//=============================================================================
// Filter Iterator Adaptor

template <class Predicate, class Iterator>
class filter_iterator_policies
{
public:
    filter_iterator_policies() { }

    filter_iterator_policies(const Predicate& p, const Iterator& end)
        : m_predicate(p), m_end(end) { }

    void initialize(Iterator& x) {
        satisfy_predicate(x);
    }

    // The Iter template argument is neccessary for compatibility with a MWCW
    // bug workaround
    template <class IteratorAdaptor>
    void increment(IteratorAdaptor& x) {
        ++x.base();
        satisfy_predicate(x.base());
    }

    template <class IteratorAdaptor>
    typename IteratorAdaptor::reference dereference(const IteratorAdaptor& x) const
        { return *x.base(); }

    template <class IteratorAdaptor1, class IteratorAdaptor2>
    bool equal(const IteratorAdaptor1& x, const IteratorAdaptor2& y) const
        { return x.base() == y.base(); }

 private:
    void satisfy_predicate(Iterator& iter);
    Predicate m_predicate;
    Iterator m_end;
};

template <class Predicate, class Iterator>
void filter_iterator_policies<Predicate,Iterator>::satisfy_predicate(
    Iterator& iter)
{
    while (m_end != iter && !m_predicate(*iter))
        ++iter;
}



namespace detail {
  // A type generator returning Base if T is derived from Base, and T otherwise.
  template <class Base, class T>
  struct reduce_to_base_class
  {
      typedef typename if_true<(
            ::boost::is_convertible<T*,Base*>::value
          )>::template then<Base,T>::type type;
  };

  // "Steps down" the category of iterators below bidirectional so the category
  // can be used with filter iterators.
  template <class Iterator>
  struct non_bidirectional_category
  {
# if !BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
      typedef typename reduce_to_base_class<
              std::forward_iterator_tag,
                   typename iterator_traits<Iterator>::iterator_category
      >::type type;
   private:
      // For some reason, putting this assertion in filter_iterator_generator fails inexplicably under MSVC
      BOOST_STATIC_CONSTANT(
          bool, is_bidirectional
          = (!boost::is_convertible<type*, std::bidirectional_iterator_tag*>::value));
      BOOST_STATIC_ASSERT(is_bidirectional);
# else
      // is_convertible doesn't work with MWERKS
      typedef typename iterator_traits<Iterator>::iterator_category input_category;
  public:
      typedef typename if_true<(
          boost::is_same<input_category,std::random_access_iterator_tag>::value
          || boost::is_same<input_category,std::bidirectional_iterator_tag>::value
        )>::template then<
          std::forward_iterator_tag,
          input_category
      >::type type;
# endif
  };
}

template <class Predicate, class Iterator,
    class Value = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_traits<Iterator>::value_type,
    class Reference = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_defaults<Iterator,Value>::reference,
    class Pointer = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_defaults<Iterator,Value>::pointer,
    class Category = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::non_bidirectional_category<Iterator>::type,
    class Distance = BOOST_ARG_DEPENDENT_TYPENAME boost::detail::iterator_traits<Iterator>::difference_type
         >
class filter_iterator_generator {
    BOOST_STATIC_CONSTANT(bool, is_bidirectional
        = (boost::is_convertible<Category*, std::bidirectional_iterator_tag*>::value));
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300) // I don't have any idea why this occurs, but it doesn't seem to hurt too badly.
    BOOST_STATIC_ASSERT(!is_bidirectional);
#endif
    typedef filter_iterator_policies<Predicate,Iterator> policies_type;
 public:
    typedef iterator_adaptor<Iterator,policies_type,
        Value,Reference,Pointer,Category,Distance> type;
};

// This keeps MSVC happy; it doesn't like to deduce default template arguments
// for template function return types
namespace detail {
  template <class Predicate, class Iterator>
  struct filter_generator {
    typedef typename boost::filter_iterator_generator<Predicate,Iterator>::type type;
  };
}

template <class Predicate, class Iterator>
inline typename detail::filter_generator<Predicate, Iterator>::type
make_filter_iterator(Iterator first, Iterator last, const Predicate& p = Predicate())
{
  typedef filter_iterator_generator<Predicate, Iterator> Gen;
  typedef filter_iterator_policies<Predicate,Iterator> policies_t;
  typedef typename Gen::type result_t;
  return result_t(first, policies_t(p, last));
}

} // namespace boost
# undef BOOST_ARG_DEPENDENT_TYPENAME


#endif



