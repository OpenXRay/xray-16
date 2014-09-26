/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_CLOSURE_CONTEXT_HPP)
#define BOOST_SPIRIT_CLOSURE_CONTEXT_HPP

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

#if !defined(BOOST_SPIRIT_CLOSURE_CONTEXT_LINKER_DEFINED)
#define BOOST_SPIRIT_CLOSURE_CONTEXT_LINKER_DEFINED

///////////////////////////////////////////////////////////////////////////////
//
//  closure_context_linker
//  { helper template for the closure extendability }
//
//      This classes can be 'overloaded' (defined elsewhere), to plug
//      in additional functionality into the closure parsing process.
//
///////////////////////////////////////////////////////////////////////////////

template<typename ContextT>
struct closure_context_linker : public ContextT
{
    template <typename ParserT>
    closure_context_linker(ParserT const& p)
    : ContextT(p) {}

    template <typename ParserT, typename ScannerT>
    void pre_parse(ParserT const& p, ScannerT const& scan)
    { ContextT::pre_parse(p, scan); }

    template <typename ResultT, typename ParserT, typename ScannerT>
    ResultT&
    post_parse(ResultT& hit, ParserT const& p, ScannerT const& scan)
    { return ContextT::post_parse(hit, p, scan); }
};

#endif // !defined(BOOST_SPIRIT_CLOSURE_CONTEXT_LINKER_DEFINED)

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif // BOOST_SPIRIT_CLOSURE_CONTEXT_HPP
