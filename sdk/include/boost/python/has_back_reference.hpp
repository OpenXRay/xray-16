// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef HAS_BACK_REFERENCE_DWA2002323_HPP
# define HAS_BACK_REFERENCE_DWA2002323_HPP

#if defined(__ALPHA) && defined(__osf__) && defined(__DECCXX_VER)
# include <pyconfig.h>
#endif
#include <boost/config.hpp>

namespace boost { namespace python { 

// traits class which users can specialize to indicate that a class
// contains a back-reference to its owning PyObject*
template <class T>
struct has_back_reference
{
    BOOST_STATIC_CONSTANT(bool, value = false);
};


}} // namespace boost::python

#endif // HAS_BACK_REFERENCE_DWA2002323_HPP
