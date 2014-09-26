/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined BOOST_SPIRIT_GRAMMAR_IPP
#define BOOST_SPIRIT_GRAMMAR_IPP

#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
#include "boost/spirit/core/non_terminal/impl/object_with_id.ipp"
#include <algorithm>
#include <functional>
#include <memory> // for std::auto_ptr
#include <boost/weak_ptr.hpp>
#endif

#ifdef BOOST_SPIRIT_THREADSAFE
#include "boost/thread/tss.hpp"
#include "boost/thread/mutex.hpp"
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

template <typename DerivedT, typename ContextT>
struct grammar;

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1200)

BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER(grammar_definition_wrapper, definition);

//////////////////////////////////
template <typename GrammarT, typename ScannerT>
struct grammar_definition
{
    typedef typename impl::grammar_definition_wrapper<GrammarT>
        ::template result_<ScannerT>::param_t type;
};

#else

//////////////////////////////////
template <typename GrammarT, typename ScannerT>
struct grammar_definition
{
    typedef typename GrammarT::template definition<ScannerT> type;
};

#endif

    namespace impl
    {

#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
    struct grammar_tag {};

    //////////////////////////////////
    template <typename GrammarT>
    struct grammar_helper_base
    {
        virtual int undefine(GrammarT *) = 0;
        virtual ~grammar_helper_base() {}
    };

    //////////////////////////////////
    template <typename GrammarT>
    struct grammar_helper_list
    {
        typedef GrammarT                      grammar_t;
        typedef grammar_helper_base<GrammarT> helper_t;
        typedef std::vector<helper_t*>        vector_t;

        grammar_helper_list() {}
        grammar_helper_list(grammar_helper_list const& x)
        {   // Does _not_ copy the helpers member !
        }

        grammar_helper_list& operator=(grammar_helper_list const& x)
        {   // Does _not_ copy the helpers member !
            return *this;
        }

        void push_back(helper_t *helper)
        { helpers.push_back(helper); }

        void pop_back()
        { helpers.pop_back(); }

        typename vector_t::size_type
        size() const
        { return helpers.size(); }

        typename vector_t::reverse_iterator
        rbegin()
        { return helpers.rbegin(); }

        typename vector_t::reverse_iterator
        rend()
        { return helpers.rend(); }

#ifdef BOOST_SPIRIT_THREADSAFE
        boost::mutex & mutex()
        { return m; }
#endif

    private:

        vector_t        helpers;
#ifdef BOOST_SPIRIT_THREADSAFE
        boost::mutex    m;
#endif
    };

    //////////////////////////////////
    struct grammar_extract_helper_list;

#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)    \
    && (!defined(__GNUC__) || (__GNUC__ > 2))

    struct grammar_extract_helper_list
    {
        template<typename GrammarT>
        static grammar_helper_list<GrammarT>&
        do_(GrammarT const* g)
        {
            return g->helpers;
        }
    };

#endif

    //////////////////////////////////
    template <typename GrammarT, typename DerivedT, typename ScannerT>
    struct grammar_helper : private grammar_helper_base<GrammarT>
    {
        typedef GrammarT grammar_t;
        typedef ScannerT scanner_t;
        typedef DerivedT derived_t;
        typedef typename grammar_definition<DerivedT, ScannerT>::type definition_t;

        typedef grammar_helper<grammar_t, derived_t, scanner_t> helper_t;
        typedef boost::shared_ptr<helper_t> helper_ptr_t;
        typedef boost::weak_ptr<helper_t>   helper_weak_ptr_t;

        grammar_helper*
        this_() { return this; }

        grammar_helper(helper_weak_ptr_t& p)
        : definitions_cnt(0)
        , self(this_())
        { p = self; }

        definition_t&
        define(grammar_t const* grammar)
        {
            grammar_helper_list<GrammarT> &helpers =
#if !defined(__GNUC__) || (__GNUC__ > 2)
                grammar_extract_helper_list::do_(grammar);
#else
                grammar->helpers;
#endif
            typename grammar_t::object_id id = grammar->get_object_id();

            if (definitions.size()<=id)
                definitions.resize(id*3/2+1);
            if (definitions[id]!=0)
                return *definitions[id];

            std::auto_ptr<definition_t>
                result(new definition_t(grammar->derived()));

#ifdef BOOST_SPIRIT_THREADSAFE
            boost::mutex::scoped_lock(helpers.mutex());
#endif
            helpers.push_back(this);

            ++definitions_cnt;
            definitions[id] = result.get();
            return *(result.release());
        }

        int
        undefine(grammar_t* grammar)
        {
            typename grammar_t::object_id id = grammar->get_object_id();

            if (definitions.size()<=id)
                return 0;
            delete definitions[id];
            definitions[id] = 0;
            if (--definitions_cnt==0)
                self.reset();
            return 0;
        }

    private:

        std::vector<definition_t*>  definitions;
        unsigned long               definitions_cnt;
        helper_ptr_t                self;
    };

#endif /* defined(BOOST_SPIRIT_NO_MULTIPLE_GRAMMAR_INSTANCES) */

    //////////////////////////////////
    template<typename DerivedT, typename ContextT, typename ScannerT>
    inline typename parser_result<grammar<DerivedT, ContextT>, ScannerT>::type
    grammar_parser_parse(
        grammar<DerivedT, ContextT> const*  self,
        ScannerT const &scan)
    {
        typedef grammar<DerivedT, ContextT> self_t;
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef typename DerivedT::template definition<ScannerT> definition;

#if defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
        static definition    def(self->derived());
#else
        typedef impl::grammar_helper<self_t, DerivedT, ScannerT> helper_t;
        typedef typename helper_t::helper_weak_ptr_t             ptr_t;

# ifdef BOOST_SPIRIT_THREADSAFE
        static boost::thread_specific_ptr<ptr_t> tld_helper;
        if (!tld_helper.get())
            tld_helper.reset(new ptr_t);
        ptr_t &helper = *tld_helper;
# else
        static ptr_t helper;
# endif
        if (!boost::make_shared(helper).get())
            new helper_t(helper);
        definition &def = boost::make_shared(helper)->define(self);
#endif
        return def.start().parse(scan);
    }

    //////////////////////////////////
    template<typename GrammarT>
    inline void
    grammar_destruct(GrammarT* self)
    {
#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
        typedef impl::grammar_helper_base<GrammarT> helper_base_t;
        typedef grammar_helper_list<GrammarT> helper_list_t;
        typedef typename helper_list_t::vector_t::reverse_iterator iterator_t;

        helper_list_t&  helpers =
# if !defined(__GNUC__) || (__GNUC__ > 2)
            grammar_extract_helper_list::do_(self);
# else
            self->helpers;
# endif

# if (defined(BOOST_MSVC) && (BOOST_MSVC <= 1200)) \
    || defined(BOOST_INTEL_CXX_VERSION)
        for (iterator_t i = helpers.rbegin(); i != helpers.rend(); ++i)
            (*i)->undefine(self);
# else
        std::for_each(helpers.rbegin(), helpers.rend(),
            std::bind2nd(std::mem_fun(&helper_base_t::undefine), self));
# endif

#endif
    }

    } // namespace impl

///////////////////////////////////////
#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
#define BOOST_SPIRIT_GRAMMAR_ID , public impl::object_with_id<impl::grammar_tag>
#else
#define BOOST_SPIRIT_GRAMMAR_ID
#endif

///////////////////////////////////////
#if !defined(__GNUC__) || (__GNUC__ > 2)
#define BOOST_SPIRIT_GRAMMAR_ACCESS private:
#else
#define BOOST_SPIRIT_GRAMMAR_ACCESS
#endif

///////////////////////////////////////
#if !defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
#define BOOST_SPIRIT_GRAMMAR_STATE                            \
    BOOST_SPIRIT_GRAMMAR_ACCESS                               \
    friend struct impl::grammar_extract_helper_list;    \
    mutable impl::grammar_helper_list<self_t> helpers;
#else
#define BOOST_SPIRIT_GRAMMAR_STATE
#endif

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif
