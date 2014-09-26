/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_MATCH_IPP)
#define BOOST_SPIRIT_MATCH_IPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/ref.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

    template <typename T>
    class match;

    namespace impl
    {
        ///////////////////////////////////////////////////////////////////////
        //
        //  Assignment of the match result
        //
        //      Implementation note: This assignment is wrapped by a
        //      template to allow its specialization for other types
        //      elsewhere.
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename T1>
        struct convert
        {
            template <typename T>
            static T
            to_result(T1 const &t) { return T(t); }
        };

    #if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

        struct match_attr_helper1   // T is not a boost::reference_wrapper
        {
            template <typename T>
            struct apply
            {
                template <typename MatchT>
                static T get(MatchT const& m)
                { return T(m.value()); }

                static T get(match<nil_t> const&)
                { return T(); }

                static T get_default()
                { return T(); }
            };
        };

        struct match_attr_helper2   // T is a boost::reference_wrapper
        {
            template <typename T>
            struct apply
            {
                template <typename MatchT>
                static T get(MatchT const& m)
                {
                    return convert<typename MatchT::attr_t>
                        ::template to_result<T>(m.value());
                }

                static T get(match<nil_t> const&)
                {
                    typedef typename T::type plain_type;
                    static plain_type v;
                    return T(v);
                }

                static T get_default()
                {
                    typedef typename T::type plain_type;
                    static plain_type v;
                    return T(v);
                }
            };
        };

        template <typename T>
        struct match_attr
        {
            typedef mpl::if_<
                boost::is_reference_wrapper<T>, //  IF
                match_attr_helper2,             //  THEN
                match_attr_helper1              //  ELSE
            >::type select_t;

            template <typename MatchT>
            static T get(MatchT const& m)
            { return select_t::template apply<T>::get(m); }

            static T get(match<nil_t> const& m)
            { return select_t::template apply<T>::get(m); }

            static T get_default()
            { return select_t::template apply<T>::get_default(); }
        };

    #else // !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

        template <typename T>
        struct match_attr
        {
            template <typename MatchT>
            static T get(MatchT const& m)
            {
                return convert<typename MatchT::attr_t>
                    ::template to_result<T>(m.value());
            }

            static T get(match<nil_t> const&)
            { return T(); }

            static T get_default()
            { return T(); }
        };

        template <typename T>
        struct match_attr<boost::reference_wrapper<T> >
        {
            template <typename MatchT>
            static boost::reference_wrapper<T>
            get(MatchT const& m)
            { return boost::reference_wrapper<T>(m.value()); }

            static boost::reference_wrapper<T>
            get(match<nil_t> const&)
            {
                static T v;
                return boost::reference_wrapper<T>(v);
            }

            static boost::reference_wrapper<T>
            get_default()
            {
                static T v;
                return boost::reference_wrapper<T>(v);
            }
        };

    #endif // defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

        template <>
        struct match_attr<nil_t>
        {
            template <typename MatchT>
            static nil_t get(MatchT const&)
            { return nil_t(); }
            static nil_t get_default()
            { return nil_t(); }
        };


    #if !defined(__BORLANDC__)
        struct dummy { void nonnull() {}; };
        typedef void (dummy::*safe_bool)();
    #else
        typedef bool safe_bool;
    #endif

    #if !defined(__BORLANDC__)
    #define BOOST_SPIRIT_SAFE_BOOL(cond)  ((cond) ? &impl::dummy::nonnull : 0)
    #else
    #define BOOST_SPIRIT_SAFE_BOOL(cond)  (cond)
    #endif

}}} // namespace boost::spirit::impl

#endif

