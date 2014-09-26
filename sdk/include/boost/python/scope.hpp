// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef SCOPE_DWA2002724_HPP
# define SCOPE_DWA2002724_HPP

# include <boost/python/object_core.hpp>
# include <boost/python/refcount.hpp>
# include <boost/utility.hpp>

namespace boost { namespace python { 

class BOOST_PYTHON_DECL scope
    : public object
{
 public:
    inline scope(scope const&);
    inline scope(object const&);
    inline scope();
    inline ~scope();
    
 private: // data members
    PyObject* m_previous_scope;

 private: // unimplemented functions
    void operator=(scope const&);
    
 private: // static members
    
    // Use a PyObject* to avoid problems with static destruction after Py_Finalize
    static PyObject* current_scope;
};

inline scope::scope(object const& new_scope)
    : object(new_scope)
    , m_previous_scope(current_scope)
{
    current_scope = python::incref(new_scope.ptr());
}

inline scope::scope()
    : object(detail::borrowed_reference(
                 current_scope ? current_scope : Py_None
                 ))
      , m_previous_scope(python::xincref(current_scope))
{
}

inline scope::~scope()
{
    python::xdecref(current_scope);
    current_scope = m_previous_scope;
}

namespace converter
{
  template <>
  struct object_manager_traits<scope>
      : object_manager_traits<object>
  {
  };
}

// Placing this after the specialization above suppresses a CWPro8.3 bug
inline scope::scope(scope const& new_scope)
    : object(new_scope)
    , m_previous_scope(current_scope)
{
    current_scope = python::incref(new_scope.ptr());
}

}} // namespace boost::python

#endif // SCOPE_DWA2002724_HPP
