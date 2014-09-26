// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef ITERATOR_DWA2002510_HPP
# define ITERATOR_DWA2002510_HPP

# include <boost/python/object/iterator_core.hpp>
# include <boost/python/class.hpp>
# include <boost/python/object/class_detail.hpp>
# include <boost/python/return_value_policy.hpp>
# include <boost/python/return_by_value.hpp>
# include <boost/python/object/function_object.hpp>
# include <boost/python/handle.hpp>
# include <boost/type.hpp>
# include <boost/python/arg_from_python.hpp>
# include <boost/mpl/apply.hpp>
# include <boost/bind.hpp>
# include <boost/bind/protect.hpp>
# include <boost/python/detail/raw_pyobject.hpp>
# include <boost/type_traits/add_reference.hpp>
# include <boost/type_traits/add_const.hpp>

namespace boost { namespace python { namespace objects {

// CallPolicies for the next() method of iterators. We don't want
// users to have to explicitly specify that the references returned by
// iterators are copied, so we just replace the result_converter from
// the default_iterator_call_policies with a permissive one which
// always copies the result.
typedef return_value_policy<return_by_value> default_iterator_call_policies;

// Instantiations of these are wrapped to produce Python iterators.
template <class NextPolicies, class Iterator>
struct iterator_range
{
    iterator_range(object sequence, Iterator start, Iterator finish);

    object m_sequence; // Keeps the sequence alive while iterating.
    Iterator m_start;
    Iterator m_finish;
};

namespace detail
{
  // Guts of the iterator's next() function. We can't just wrap an
  // ordinary function because we don't neccessarily know the result
  // type of dereferencing the iterator. This also saves us from
  // throwing C++ exceptions to indicate end-of-sequence.
  template <class Iterator, class Policies>
  struct iterator_next
  {
      static PyObject* execute(PyObject* args_, PyObject* kw, Policies const& policies)
      {
          typedef iterator_range<Policies,Iterator> range_;
          
          PyObject* py_self = PyTuple_GET_ITEM(args_, 0);
          arg_from_python<range_*> c0(py_self);
          range_* self = c0(py_self);

          // Done iterating?
          if (self->m_start == self->m_finish)
          {
              objects::set_stop_iteration_error();
              return 0;
          }
    
          // note: precall happens before we can check for the result
          // converter in this case, to ensure it happens before the
          // iterator is dereferenced. However, the arity is 1 so
          // there's not much risk that this will amount to anything.
          if (!policies.precall(args_)) return 0;

          PyObject* result = iterator_next::convert_result(*self->m_start);
          ++self->m_start;
          
          return policies.postcall(args_, result);
      }
   private:
      // Convert the result of dereferencing the iterator. Dispatched
      // here because we can't neccessarily get the value_type of the
      // iterator without PTS. This way, we deduce the value type by
      // dereferencing.
      template <class ValueType>
      static PyObject* convert_result(ValueType& x)
      {
          typedef typename Policies::result_converter result_converter;
          typename mpl::apply1<result_converter,ValueType&>::type cr;

          return cr(x);
      }
      template <class ValueType>
      static PyObject* convert_result(ValueType const& x)
      {
          typedef typename Policies::result_converter result_converter;
          typename mpl::apply1<result_converter,ValueType const&>::type cr;

          return cr(x);
      }
  };

  // Get a Python class which contains the given iterator and
  // policies, creating it if neccessary. Requires: NextPolicies is
  // default-constructible.
  template <class Iterator, class NextPolicies>
  object demand_iterator_class(char const* name, Iterator* = 0, NextPolicies const& policies = NextPolicies())
  {
      typedef iterator_range<NextPolicies,Iterator> range_;

      // Check the registry. If one is already registered, return it.
      handle<> class_obj(
          objects::registered_class_object(python::type_id<range_>()));
        
      if (class_obj.get() != 0)
          return object(class_obj);

      // Make a callable object which can be used as the iterator's next() function.
      object next_function = 
          objects::function_object(
                  bind(&detail::iterator_next<Iterator,NextPolicies>::execute, _1, _2, policies)
              , 1);
    
      return class_<range_>(name, no_init)
          .def("__iter__", identity_function())
          .setattr("next", next_function)
          ;
  }

  // This class template acts as a generator for an ordinary function
  // which builds a Python iterator.
  template <class Target, class Iterator
            , class Accessor1, class Accessor2
            , class NextPolicies
            >
  struct make_iterator_help
  {
      // Extract an object x of the Target type from the first Python
      // argument, and invoke get_start(x)/get_finish(x) to produce
      // iterators, which are used to construct a new iterator_range<>
      // object that gets wrapped into a Python iterator.
      static PyObject* create(
          Accessor1 const& get_start, Accessor2 const& get_finish
          , PyObject* args_, PyObject* /*kw*/)
      {
          // Make sure the Python class is instantiated.
          detail::demand_iterator_class("iterator", (Iterator*)0, NextPolicies());

          to_python_value<iterator_range<NextPolicies,Iterator> > cr;

          // Extract x from the first argument
          PyObject* arg0 = PyTuple_GET_ITEM(args_, 0);
          arg_from_python<Target> c0(arg0);
          if (!c0.convertible()) return 0;
          typename arg_from_python<Target>::result_type x = c0(arg0);

          // Build and convert the iterator_range<>.
          return cr(
              iterator_range<NextPolicies,Iterator>(
                  object((python::detail::borrowed_reference)arg0)
                  , get_start(x), get_finish(x)));
      }
  };

  template <class NextPolicies, class Target, class Iterator, class Accessor1, class Accessor2>
  inline object make_iterator_function(
      Accessor1 const& get_start, Accessor2 const& get_finish, Iterator const& (*)(), boost::type<Target>*, NextPolicies*, int)
  {
      return 
          objects::function_object(
              boost::bind(
                  &make_iterator_help<
                      Target,Iterator,Accessor1,Accessor2,NextPolicies
                  >::create
                  , get_start, get_finish, _1, _2)
              , 1 );
  }

  template <class NextPolicies, class Target, class Iterator, class Accessor1, class Accessor2>
  inline object make_iterator_function(
      Accessor1 const& get_start, Accessor2 const& get_finish, Iterator& (*)(), boost::type<Target>*, NextPolicies*, ...)
  {
      return make_iterator_function(
          get_start, get_finish, (Iterator const&(*)())0
          , (boost::type<Target>*)0, (NextPolicies*)0, 0);
  }

}

// Create a Python callable object which accepts a single argument
// convertible to the C++ Target type and returns a Python
// iterator. The Python iterator uses get_start(x) and get_finish(x)
// (where x is an instance of Target) to produce begin and end
// iterators for the range, and an instance of NextPolicies is used as
// CallPolicies for the Python iterator's next() function. 
template <class NextPolicies, class Target, class Accessor1, class Accessor2>
inline object make_iterator_function(
    Accessor1 const& get_start, Accessor2 const& get_finish
    , boost::type<Target>* = 0, NextPolicies* = 0)
{
    typedef typename Accessor1::result_type iterator;
    typedef typename add_const<iterator>::type iterator_const;
    typedef typename add_reference<iterator_const>::type iterator_cref;
      
    return detail::make_iterator_function(
        get_start, get_finish
        , (iterator_cref(*)())0
        , (boost::type<Target>*)0
        , (NextPolicies*)0
        , 0
        );
}

//
// implementation
//
template <class NextPolicies, class Iterator>
inline iterator_range<NextPolicies,Iterator>::iterator_range(
    object sequence, Iterator start, Iterator finish)
    : m_sequence(sequence), m_start(start), m_finish(finish)
{
}

}}} // namespace boost::python::objects

#endif // ITERATOR_DWA2002510_HPP
