// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef OBJECT_CORE_DWA2002615_HPP
# define OBJECT_CORE_DWA2002615_HPP

#if defined(__ALPHA) && defined(__osf__) && defined(__DECCXX_VER)
# include <pyconfig.h>
#endif
# include <boost/type.hpp>

# include <boost/python/call.hpp>
# include <boost/python/handle_fwd.hpp>
# include <boost/python/errors.hpp>
# include <boost/python/slice_nil.hpp>
# include <boost/python/detail/raw_pyobject.hpp>
# include <boost/python/refcount.hpp>
# include <boost/python/detail/preprocessor.hpp>
# include <boost/python/tag.hpp>
# include <boost/python/detail/dependent.hpp>

# include <boost/preprocessor/iterate.hpp>
# include <boost/preprocessor/debug/line.hpp>

namespace boost { namespace python { 

namespace converter
{
  template <class T> struct arg_to_python;
}

// Put this in an inner namespace so that the generalized operators won't take over
namespace api
{
  
// This file contains the definition of the object class and enough to
// construct/copy it, but not enough to do operations like
// attribute/item access or addition.

  template <class Policies> class proxy;
  
  struct const_attribute_policies;
  struct attribute_policies;
  struct const_item_policies;
  struct item_policies;
  struct const_slice_policies;
  struct slice_policies;

  typedef proxy<const_attribute_policies> const_object_attribute;
  typedef proxy<attribute_policies> object_attribute;
  typedef proxy<const_item_policies> const_object_item;
  typedef proxy<item_policies> object_item;
  typedef proxy<const_slice_policies> const_object_slice;
  typedef proxy<slice_policies> object_slice;

  //
  // is_proxy -- proxy type detection
  //
# ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
  template <class T>
  struct is_proxy
  {
      BOOST_STATIC_CONSTANT(bool, value = false);
  };
  template <class T>
  struct is_proxy<proxy<T> >
  {
      BOOST_STATIC_CONSTANT(bool, value = true);
  };
# else
  typedef char yes_proxy;
  typedef char (&no_proxy)[2];
  template <class T>
  yes_proxy is_proxy_helper(boost::type<proxy<T> >*);
  no_proxy is_proxy_helper(...);
  template <class T>
  struct is_proxy
  {
      BOOST_STATIC_CONSTANT(
          bool, value = (sizeof(is_proxy_helper((boost::type<T>*)0))
                         == sizeof(yes_proxy)));
  };
# endif 

  template <bool is_proxy, bool is_object_manager>  struct object_initializer;
  
  class object;
  typedef PyObject* (object::*bool_type)() const;
  
  template <class U>
  class object_operators
  {
   protected:
# if !defined(BOOST_MSVC) || BOOST_MSVC > 1200
      typedef object const& object_cref;
# else 
      typedef object object_cref;
# endif
   public:
      // function call
      //
      object operator()() const;

# define BOOST_PP_ITERATION_PARAMS_1 (3, (1, BOOST_PYTHON_MAX_ARITY, <boost/python/object_call.hpp>))
# include BOOST_PP_ITERATE()

      // truth value testing
      //
      operator bool_type() const;
      bool operator!() const; // needed for vc6

      // Attribute access
      //
      const_object_attribute attr(char const*) const;
      object_attribute attr(char const*);

      // item access
      //
      const_object_item operator[](object_cref) const;
      object_item operator[](object_cref);
    
      template <class T>
      const_object_item
      operator[](T const& key) const
# if !defined(BOOST_MSVC) || BOOST_MSVC > 1300
          ;
# else 
      {
          return (*this)[object(key)];
      }
# endif 
    
      template <class T>
      object_item
      operator[](T const& key)
# if !defined(BOOST_MSVC) || BOOST_MSVC > 1300
          ;
# else 
      {
          return (*this)[object(key)];
      }
# endif

      // slicing
      //
      const_object_slice slice(object_cref, object_cref) const;
      object_slice slice(object_cref, object_cref);

      const_object_slice slice(slice_nil, object_cref) const;
      object_slice slice(slice_nil, object_cref);
                             
      const_object_slice slice(object_cref, slice_nil) const;
      object_slice slice(object_cref, slice_nil);

      template <class T, class V>
      const_object_slice
      slice(T const& start, V const& end) const
# if !defined(BOOST_MSVC) || BOOST_MSVC > 1300
          ;
# else
      {
          return this->slice(
               slice_bound<T>::type(start)
              ,  slice_bound<V>::type(end));
      }
# endif
    
      template <class T, class V>
      object_slice
      slice(T const& start, V const& end)
# if !defined(BOOST_MSVC) || BOOST_MSVC > 1300
          ;
# else
      {
          return this->slice(
              slice_bound<T>::type(start)
              , slice_bound<V>::type(end));
      }
# endif 
   private:
     // there is a confirmed CWPro8 codegen bug here. We prevent the
     // early destruction of a temporary by binding a named object
     // instead.
# if __MWERKS__ < 0x3000 || __MWERKS__ > 0x3003
    typedef object const& object_cref2;
# else
    typedef object const object_cref2;
# endif
  };

  // VC6 and VC7 require this base class in order to generate the
  // correct copy constructor for object. We can't define it there
  // explicitly or it will complain of ambiguity.
  struct object_base : object_operators<object>
  {
      // copy constructor without NULL checking, for efficiency. 
      inline object_base(object_base const&);
      inline object_base(PyObject* ptr);
      
      object_base& operator=(object_base const& rhs);
      ~object_base();
        
      // Underlying object access -- returns a borrowed reference
      PyObject* ptr() const;
      
   private:
      PyObject* m_ptr;
  };

  class object : public object_base
  {
   public:
      // default constructor creates a None object
      object();
      // explicit conversion from any C++ object to Python
      template <class T>
      explicit object(T const& x)
          : object_base(
              object_initializer<
                 is_proxy<T>::value
                 , converter::is_object_manager<T>::value
              >::get(&x, detail::convertible<object const*>::check(&x)))
      {
      }

      // Throw error_already_set() if the handle is null.
      BOOST_PYTHON_DECL explicit object(handle<> const&);

   public: // implementation detail -- for internal use only
      explicit object(detail::borrowed_reference);
      explicit object(detail::new_reference);
      explicit object(detail::new_non_null_reference);
  };

  // Macros for forwarding constructors in classes derived from
  // object. Derived classes will usually want these as an
  // implementation detail
# define BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS_(derived, base)       \
    inline explicit derived(python::detail::borrowed_reference p)       \
        : base(p) {}                                                    \
    inline explicit derived(python::detail::new_reference p)            \
        : base(p) {}                                                    \
    inline explicit derived(python::detail::new_non_null_reference p)   \
        : base(p) {}

# if !defined(BOOST_MSVC) || BOOST_MSVC > 1200
#  define BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS_
# else
  // MSVC6 has a bug which causes an explicit template constructor to
  // be preferred over an appropriate implicit conversion operator
  // declared on the argument type. Normally, that would cause a
  // runtime failure when using extract<T> to extract a type with a
  // templated constructor. This additional constructor will turn that
  // runtime failure into an ambiguity error at compile-time due to
  // the lack of partial ordering, or at least a link-time error if no
  // generalized template constructor is declared.
#  define BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(derived, base)       \
    BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS_(derived, base)            \
    template <class T>                                                  \
    explicit derived(extract<T> const&);
# endif

  //
  // object_initializer -- get the handle to construct the object with,
  // based on whether T is a proxy or derived from object
  //
  template <bool is_proxy = false, bool is_object_manager = false>
  struct object_initializer
  {
      static PyObject*
      get(object const* x, detail::yes_convertible)
      {
          return python::incref(x->ptr());
      }
      
      template <class T>
      static PyObject*
      get(T const* x, detail::no_convertible)
      {
          return python::incref(converter::arg_to_python<T>(*x).get());
      }
  };
      
  template <>
  struct object_initializer<true, false>
  {
      template <class Policies>
      static PyObject* 
      get(proxy<Policies> const* x, detail::no_convertible)
      {
          return python::incref(x->operator object().ptr());
      }
  };

  template <>
  struct object_initializer<false, true>
  {
      template <class T>
      static PyObject*
      get(T const* x, ...)
      {
          return python::incref(get_managed_object(*x, tag));
      }
  };

  template <>
  struct object_initializer<true, true>
  {}; // empty implementation should cause an error
}
using api::object;
template <class T> struct extract;

//
// implementation
//

inline object::object()
    : object_base(python::incref(Py_None))
{}

// copy constructor without NULL checking, for efficiency
inline api::object_base::object_base(object_base const& rhs)
    : m_ptr(python::incref(rhs.m_ptr))
{}

inline api::object_base::object_base(PyObject* p)
    : m_ptr(p)
{}

inline api::object_base& api::object_base::operator=(api::object_base const& rhs)
{
    Py_INCREF(rhs.m_ptr);
    Py_DECREF(this->m_ptr);
    this->m_ptr = rhs.m_ptr;
    return *this;
}

inline api::object_base::~object_base()
{
    Py_DECREF(m_ptr);
}

inline object::object(detail::borrowed_reference p)
    : object_base(python::incref((PyObject*)p))
{}

inline object::object(detail::new_reference p)
    : object_base(expect_non_null((PyObject*)p))
{}

inline object::object(detail::new_non_null_reference p)
    : object_base((PyObject*)p)
{}

inline PyObject* api::object_base::ptr() const
{
    return m_ptr;
}

//
// Converter specialization implementations
//
namespace converter
{
  template <class T> struct object_manager_traits;
  
  template <>
  struct object_manager_traits<object>
  {
      BOOST_STATIC_CONSTANT(bool, is_specialized = true);
      static bool check(PyObject*) { return true; }
      
      static python::detail::new_non_null_reference adopt(PyObject* x)
      {
          return python::detail::new_non_null_reference(x);
      }
  };
}

inline PyObject* get_managed_object(object const& x, tag_t)
{
    return x.ptr();
}

}} // namespace boost::python

#endif // OBJECT_CORE_DWA2002615_HPP
