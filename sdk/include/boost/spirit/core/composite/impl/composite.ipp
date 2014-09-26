/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined BOOST_SPIRIT_COMPOSITE_IPP
#define BOOST_SPIRIT_COMPOSITE_IPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/call_traits.hpp>
#include <boost/type_traits.hpp>
#include <boost/spirit/core/basics.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

    namespace impl
    {
        ///////////////////////////////////////////////////////////////////////
        //
        //  This class is used when the subject is known to be empty
        //  (zero size). In such a case, the subject is not stored
        //  as a member variable to save space.
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename T, typename BaseT>
        struct empty_subject : public BaseT
        {
            typedef typename boost::call_traits<T>::param_type  param_t;
            typedef T                                           return_t;

                            empty_subject()
                            : BaseT() {}

                            empty_subject(BaseT const& base)
                            : BaseT(base) {}

                            empty_subject(param_t)
                            : BaseT() {}

                            empty_subject(BaseT const& base, param_t)
                            : BaseT(base) {}

            return_t        get() const { return return_t(); }
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  This class is used when the subject is known to be non-empty
        //  (non-zero size). In a such a case, the subject is stored
        //  as a member variable.
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename T, typename BaseT>
        struct non_empty_subject : public BaseT
        {
            typedef typename boost::call_traits<T>::param_type      param_t;
            typedef typename boost::call_traits<T>::const_reference return_t;

                            non_empty_subject()
                            : BaseT(), s() {}

                            non_empty_subject(BaseT const& base)
                            : BaseT(base), s() {}

                            non_empty_subject(param_t s_)
                            : BaseT(), s(s_) {}

                            non_empty_subject(BaseT const& base, param_t s_)
                            : BaseT(base), s(s_) {}

            return_t        get() const { return s; }
            T s;
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  Test T if it is empty and switch the proper implementation
        //  class appropriately (empty_subject or non_empty_subject)
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename T, typename BaseT>
        struct subject_type
        {
            typedef typename mpl::if_<
                is_empty<T>,                // IF
                empty_subject<T, BaseT>,    // THEN
                non_empty_subject<T, BaseT> // ELSE
            >::type type;
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  subject, left_subject and right_subject classes.
        //
        //  The three classes below are virtually similar except for the
        //  names. These classes are used by the public unary and binary
        //  classes for the unary::subject, binary::left and binary::right
        //  implementations.
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename T, typename BaseT>
        struct subject : subject_type<T, BaseT>::type
        {
            typedef typename subject_type<T, BaseT>::type       base_t;
            typedef typename boost::call_traits<T>::param_type  param_t;
            typedef typename base_t::return_t                   return_t;

            subject() : base_t() {}
            subject(BaseT const& base) : base_t(base) {}
            subject(param_t s) : base_t(s) {}
            subject(BaseT const& base, param_t s) : base_t(base, s) {}
        };

        //////////////////////////////////
        template <typename T, typename BaseT>
        struct left_subject : subject_type<T, BaseT>::type
        {
            typedef typename subject_type<T, BaseT>::type       base_t;
            typedef typename boost::call_traits<T>::param_type  param_t;
            typedef typename base_t::return_t                   return_t;

            left_subject() : base_t() {}
            left_subject(BaseT const& base) : base_t(base) {}
            left_subject(param_t s) : base_t(s) {}
            left_subject(BaseT const& base, param_t s) : base_t(base, s) {}

            return_t
            left() const { return base_t::get(); }
        };

        //////////////////////////////////
        template <typename T, typename BaseT>
        struct right_subject : subject_type<T, BaseT>::type
        {
            typedef typename subject_type<T, BaseT>::type       base_t;
            typedef typename boost::call_traits<T>::param_type  param_t;
            typedef typename base_t::return_t                   return_t;

            right_subject() : base_t() {}
            right_subject(BaseT const& base) : base_t(base) {}
            right_subject(param_t s) : base_t(s) {}
            right_subject(BaseT const& base, param_t s) : base_t(base, s) {}

            return_t
            right() const { return base_t::get(); }
        };
    }
}} // namespace boost::spirit

#endif
