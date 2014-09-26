/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2001-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#ifndef BOOST_SPIRIT_TREE_TREE_ITERATOR_HPP
#define BOOST_SPIRIT_TREE_TREE_ITERATOR_HPP

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
#define BOOST_SPIRIT_IT_NS impl
#else
#define BOOST_SPIRIT_IT_NS std
#endif

#if (defined(BOOST_INTEL_CXX_VERSION) && !defined(_STLPORT_VERSION))
#undef BOOST_SPIRIT_IT_NS
#define BOOST_SPIRIT_IT_NS impl
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  A parse_tree_iterator allows to parse a previously generated parse tree as
//  if it would be a normal input stream.
//  It traverses the parsetree to return only the tokens from the leaf nodes.
//
///////////////////////////////////////////////////////////////////////////////
template <typename ParseTreeMatchT>
class parse_tree_iterator
{
public:
    typedef ParseTreeMatchT tree_match_t;

    typedef typename tree_match_t::container_t tree_t;
    typedef typename tree_match_t::parse_node_t parse_node_t;
    typedef typename tree_match_t::tree_iterator tree_iterator_t;
    typedef typename tree_match_t::const_tree_iterator
        const_tree_iterator_t;

    typedef typename parse_node_t::const_iterator_t const_iterator_t;
    typedef BOOST_SPIRIT_IT_NS::iterator_traits<const_iterator_t> 
        iterator_traits;

    typedef typename iterator_traits::value_type value_type;
    typedef typename iterator_traits::pointer pointer;
    typedef typename iterator_traits::reference reference;
    typedef typename iterator_traits::difference_type difference_type;
    typedef std::forward_iterator_tag iterator_category;

private:
///////////////////////////////////////////////////////////////////////////////
//
//  The class node_stack contains the way back up the hierarchy from the
//  current parse node to the root parse node (needed, because the parsetree
//  itself doesn't contain uplinks).
//  This stack contains (node *, iterator) pairs.
//
///////////////////////////////////////////////////////////////////////////////
    class node_stack :
        public std::stack<std::pair<tree_t const *, const_tree_iterator_t> >
    {
        typedef std::stack<std::pair<tree_t const *, const_tree_iterator_t> > base_t;

    public:
        void push (tree_t const &target, const_tree_iterator_t iter);
        void adjust_forward();
        void go_up();       // goes up, 'till the next parse_node is found
        void go_down();     // goes down, 'till a leaf node is found

    // returns a reference to the current iterator of the current parse_node
        const_tree_iterator_t &act_iter();

    // returns the current and end iterators of the current parse node
        const_tree_iterator_t act_const_iter() const;
        const_tree_iterator_t node_end() const;
    };

public:
    parse_tree_iterator();
    parse_tree_iterator(tree_t const &target_, const_tree_iterator_t iter_);

    reference operator*() const;
    pointer operator->() const;
    parse_tree_iterator& operator++();
    parse_tree_iterator operator++(int);

    bool operator==(tree_iterator_t const &rhs) const;
    bool operator!=(tree_iterator_t const &rhs) const;

    bool operator==(parse_tree_iterator const &rhs) const;
    bool operator!=(parse_tree_iterator const &rhs) const;

private:
    node_stack nodes;
};

}} // namespace boost::spirit

#undef BOOST_SPIRIT_IT_NS
#endif // BOOST_SPIRIT_TREE_TREE_ITERATOR_HPP

