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

#if !defined(PARSE_TREE_UTILS_HPP)
#define PARSE_TREE_UTILS_HPP

#include <utility>                          // for std::pair

#include "boost/spirit/tree/parse_tree.hpp" // needed for parse tree generation

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  The function 'get_first_leaf' returnes a reference to the first leaf node
//  of the given parsetree.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
tree_node<T> const &
get_first_leaf (tree_node<T> const &node);

///////////////////////////////////////////////////////////////////////////////
//
//  The function 'find_node' finds a specified node through recursive search.
//  If the return value is true, the variable to which points the parameter
//  'found_node' will contain the address of the node with the given rule_id.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool
find_node (tree_node<T> const &node, parser_id node_to_search,
    tree_node<T> const **found_node);

///////////////////////////////////////////////////////////////////////////////
//
//  The function 'get_node_range' return a pair of iterators pointing at the
//  range, which containes the elements of a specified node. It's very useful
//  for locating all information related with a specified node.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool
get_node_range (tree_node<T> const &node, parser_id node_to_search,
    std::pair<typename tree_node<T>::const_tree_iterator,
        typename tree_node<T>::const_tree_iterator> &nodes);

///////////////////////////////////////////////////////////////////////////////
}   // namespace spirit
}   // namespace boost

#include "impl/parse_tree_utils.ipp"

#endif // !defined(PARSE_TREE_UTILS_HPP)
