/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_RULE_IPP)
#define BOOST_SPIRIT_RULE_IPP

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

    ///////////////////////////////////////////////////////////////////////////
    namespace impl
    {
        ///////////////////////////////////////////////////////////////////////
        //
        //  abstract_parser class
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename ScannerT, typename RT>
        struct abstract_parser
        {
            abstract_parser() {}
            virtual ~abstract_parser() {}

            virtual RT
            do_parse_virtual(ScannerT const& scan) const = 0;

            virtual abstract_parser *
            clone() const = 0;
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  concrete_parser class
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename ParserT, typename ScannerT, typename RT>
        struct concrete_parser : public abstract_parser<ScannerT, RT>
        {
            concrete_parser(ParserT const& p_) : p(p_) {}
            virtual ~concrete_parser() {}

            virtual RT
            do_parse_virtual(ScannerT const& scan) const
            { return p.parse(scan); }

            virtual abstract_parser<ScannerT, RT> *
            clone() const
            {
                return new concrete_parser(p);
            }

            typename ParserT::embed_t p;
        };

    } // namespace impl

    //////////////////////////////////
    template <typename ScannerT, typename ContextT, typename TagT>
    inline rule<ScannerT, ContextT, TagT>::rule()
    : ptr() {}

    //////////////////////////////////
    template <typename ScannerT, typename ContextT, typename TagT>
    inline rule<ScannerT, ContextT, TagT>::~rule() {}

    //////////////////////////////////
    template <typename ScannerT, typename ContextT, typename TagT>
    inline typename rule<ScannerT, ContextT, TagT>::result_t
    rule<ScannerT, ContextT, TagT>::parse(ScannerT const& scan) const
    {
        BOOST_SPIRIT_CONTEXT_PARSE(scan, *this, scanner_t, context_t, result_t)
    }

    //////////////////////////////////
    template <typename ScannerT, typename ContextT, typename TagT>
    inline typename rule<ScannerT, ContextT, TagT>::result_t
    rule<ScannerT, ContextT, TagT>::parse_main(ScannerT const& scan) const
    {
        result_t hit;
        if (ptr.get())
        {
            typename ScannerT::iterator_t s(scan.first);
            hit = ptr->do_parse_virtual(scan);
            scan.group_match(hit, id(), s, scan.first);
        }
        else
        {
            hit = scan.no_match();
        }
        return hit;
    }

    //////////////////////////////////
    template <typename ScannerT, typename ContextT, typename TagT>
    inline parser_id
    rule<ScannerT, ContextT, TagT>::id() const
    {
        return TagT::id(*this);
    }

}} // namespace boost::spirit

#endif
