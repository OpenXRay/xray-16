// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef DATA_MEMBERS_DWA2002328_HPP
# define DATA_MEMBERS_DWA2002328_HPP

# include <boost/python/return_value_policy.hpp>
# include <boost/python/return_by_value.hpp>
# include <boost/python/return_internal_reference.hpp>
# include <boost/python/arg_from_python.hpp>

# include <boost/python/object/function_object.hpp>

# include <boost/python/converter/builtin_converters.hpp>

# include <boost/python/detail/indirect_traits.hpp>
# include <boost/python/detail/config.hpp>
# include <boost/python/detail/wrap_python.hpp>

# include <boost/type_traits/transform_traits.hpp>
# include <boost/type_traits/add_const.hpp>
# include <boost/type_traits/add_reference.hpp>

# include <boost/mpl/if.hpp>

# include <boost/bind.hpp>

namespace boost { namespace python { 

namespace detail
{
  template <class Data, class Class, class Policies>
  struct member
  {
      static PyObject* get(Data Class::*pm, PyObject* args_, PyObject*, Policies const& policies)
      {
          arg_from_python<Class*> c0(PyTuple_GET_ITEM(args_, 0));
          if (!c0.convertible()) return 0;

          // find the result converter
          typedef typename Policies::result_converter result_converter;
          typedef typename boost::add_reference<Data>::type source;
          typename mpl::apply1<result_converter,source>::type cr;
        
          if (!policies.precall(args_)) return 0;

          PyObject* result = cr( (c0(PyTuple_GET_ITEM(args_, 0)))->*pm );
        
          return policies.postcall(args_, result);
      }
  
      static PyObject* set(Data Class::*pm, PyObject* args_, PyObject*, Policies const& policies)
      {
          // check that each of the arguments is convertible
          arg_from_python<Class&> c0(PyTuple_GET_ITEM(args_, 0));
          if (!c0.convertible()) return 0;

          typedef typename add_const<Data>::type target1;
          typedef typename add_reference<target1>::type target;
          arg_from_python<target> c1(PyTuple_GET_ITEM(args_, 1));
      
          if (!c1.convertible()) return 0;

          if (!policies.precall(args_)) return 0;

          (c0(PyTuple_GET_ITEM(args_, 0))).*pm = c1(PyTuple_GET_ITEM(args_, 1));
        
          return policies.postcall(args_, detail::none());
      }
  };

  // If it's a regular class type (not an object manager or other
  // type for which we have to_python specializations, use
  // return_internal_reference so that we can do things like
  //    x.y.z =  1
  // and get the right result.
  template <class T>
  struct default_getter_policy
  {
      typedef typename add_reference<
          typename add_const<T>::type
      >::type t_cref;

      BOOST_STATIC_CONSTANT(
          bool, by_ref = to_python_value<t_cref>::uses_registry
                        && is_reference_to_class<t_cref>::value);

      typedef typename mpl::if_c<
        by_ref
        , return_internal_reference<>
        , return_value_policy<return_by_value>
      >::type type;
  };
}

template <class C, class D>
object make_getter(D C::*pm)
{
    typedef typename detail::default_getter_policy<D>::type policy;
    
    return objects::function_object(
        ::boost::bind(
            &detail::member<D,C,policy>::get, pm, _1, _2
            , policy())
        , 1);
        
}

template <class C, class D, class Policies>
object make_getter(D C::*pm, Policies const& policies)
{
    return objects::function_object(
            ::boost::bind(
                &detail::member<D,C,Policies>::get, pm, _1, _2
                , policies)
        , 1);
}

template <class C, class D>
object make_setter(D C::*pm)
{
    return objects::function_object(
        ::boost::bind(
            &detail::member<D,C,default_call_policies>::set, pm, _1, _2
            , default_call_policies())
        , 2);
}

template <class C, class D, class Policies>
object make_setter(D C::*pm, Policies const& policies)
{
    return objects::function_object(
        ::boost::bind(
            &detail::member<D,C,Policies>::set, pm, _1, _2
            , policies)
        , 2);
}

    
}} // namespace boost::python

#endif // DATA_MEMBERS_DWA2002328_HPP
