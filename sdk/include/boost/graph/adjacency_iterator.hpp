//=======================================================================
// Copyright 2002 Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// This file is part of the Boost Graph Library
//
// You should have received a copy of the License Agreement for the
// Boost Graph Library along with the software; see the file LICENSE.
//
// Permission to modify the code and to distribute modified code is
// granted, provided the text of this NOTICE is retained, a notice that
// the code was modified is included with the above COPYRIGHT NOTICE and
// with the COPYRIGHT NOTICE in the LICENSE file, and that the LICENSE
// file is distributed with the modified code.
//
// LICENSOR MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED.
// By way of example, but not limitation, Licensor MAKES NO
// REPRESENTATIONS OR WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY
// PARTICULAR PURPOSE OR THAT THE USE OF THE LICENSED SOFTWARE COMPONENTS
// OR DOCUMENTATION WILL NOT INFRINGE ANY PATENTS, COPYRIGHTS, TRADEMARKS
// OR OTHER RIGHTS.
//=======================================================================

#ifndef BOOST_ADJACENCY_ITERATOR_HPP
#define BOOST_ADJACENCY_ITERATOR_HPP

#include <boost/iterator_adaptors.hpp>
#include <boost/graph/graph_traits.hpp>

namespace boost {

  namespace detail {

    template <class Graph>
    struct adjacency_iterator_policies : 
      public boost::default_iterator_policies
    {
      inline adjacency_iterator_policies() { }
      inline adjacency_iterator_policies(const Graph* g) : m_g(g) { }

      template <class Iterator>
      inline typename Iterator::reference
      dereference(const Iterator& i) const
        { return target(*i.base(), *m_g); }

      const Graph* m_g;
    };

  } // namespace detail

  template <class Graph,
            class Vertex = typename graph_traits<Graph>::vertex_descriptor,
            class OutEdgeIter=typename graph_traits<Graph>::out_edge_iterator>
  class adjacency_iterator_generator {
    typedef typename boost::detail::iterator_traits<OutEdgeIter>
      ::difference_type difference_type;
  public:
    typedef boost::iterator_adaptor<OutEdgeIter, 
      detail::adjacency_iterator_policies<Graph>,
      Vertex, Vertex, Vertex*, boost::multi_pass_input_iterator_tag,
      difference_type
    > type;
  };

  namespace detail {

    template <class Graph>
    struct inv_adjacency_iterator_policies : 
      public boost::default_iterator_policies
    {
      inline inv_adjacency_iterator_policies() { }
      inline inv_adjacency_iterator_policies(Graph* g) : m_g(g) { }

      template <class Iterator>
      inline typename Iterator::reference
      dereference(const Iterator& i) const
        { return source(*i.base(), *m_g); }

      Graph* m_g;
    };

  } // namespace detail

  template <class Graph, class Vertex, class InEdgeIter>
  class inv_adjacency_iterator_generator {
    typedef typename boost::detail::iterator_traits<InEdgeIter>
      ::difference_type difference_type;
  public:
    typedef boost::iterator_adaptor<InEdgeIter, 
      detail::inv_adjacency_iterator_policies<Graph>,
      Vertex, Vertex, Vertex*, boost::multi_pass_input_iterator_tag,
      difference_type
    > type;
  };

} // namespace boost

#endif // BOOST_DETAIL_ADJACENCY_ITERATOR_HPP
