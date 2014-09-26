/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2002 Juan Carlos Arevalo-Baeza
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software
    is granted provided this copyright notice appears in all copies.
    This software is provided "as is" without express or implied
    warranty, and with no claim as to its suitability for any purpose.

    See spirit.hpp for full copyright notices.
=============================================================================*/
#ifndef BOOST_SPIRIT_POSITION_ITERATOR_HPP
#define BOOST_SPIRIT_POSITION_ITERATOR_HPP

///////////////////////////////////////////////////////////////////////////////
#include <string>
#include <iterator>

#include "boost/config.hpp"

#include "boost/spirit/core/impl/msvc.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  file_position
//
//  A structure to hold positional information. This includes the file,
//  line and column
//
///////////////////////////////////////////////////////////////////////////////

struct file_position {
    std::string file;
    int line;
    int column;

    file_position(std::string const& file_ = std::string(),
                  int line_ = 1, int column_ = 1):
        file    (file_),
        line    (line_),
        column  (column_)
    {}
};

///////////////////////////////////////////////////////////////////////////////
//
//  position_iterator
//
//  It wraps an iterator, and keeps track of the current position in the input,
//  as it gets incremented.
//
//  The default tab size is 4. You can change this with the set_tabchars member
//  function.
//
//  In order to have begin/end iterators constructed, the end iterator must be
//  empty constructed. Similar to what happens with stream iterators. Note that,
//  if the wrapped iterator is a forward iterator, this one will work as a
//  forward iterator too. The begin iterator must be constructed from both, the
//  begin and end iterators of the wrapped iterator type. This is necessary to
//  implement the lookahead of characters necessary to parse CRLF sequences.
//
//  In order to extract the current positional data from the iterator, you may
//  use the get_position member function.
//
//  You can also use the set_position member function to reset the current
//  position to something new.
//
///////////////////////////////////////////////////////////////////////////////

template < typename IteratorT, typename PositionT = file_position>
class position_iterator {
#if (defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)) \
    || (defined(BOOST_INTEL_CXX_VERSION) && !defined(_STLPORT_VERSION))
    typedef impl::iterator_traits<IteratorT> traits_t;
#else
    typedef std::iterator_traits<IteratorT> traits_t;
#endif
public:
    typedef typename traits_t::value_type value_type;
    typedef typename traits_t::pointer pointer;
    typedef typename traits_t::reference reference;

    typedef typename traits_t::difference_type difference_type;

//  regardless of the underlying iterator type the position_iterator is a
//  forward iterator only
    typedef typename std::forward_iterator_tag iterator_category;

    position_iterator(): m_CharsPerTab(4), m_IsAtEnd(true) {}
    position_iterator(IteratorT const& iter, IteratorT const& iterend,
            std::string const& file, int line = 1, int column = 1):
        m_Iter(iter),
        m_IterEnd(iterend),
        m_Pos(file, line, column),
        m_CharsPerTab(4),
        m_IsAtEnd(m_Iter == m_IterEnd)
    {
    }
    position_iterator(IteratorT const& iter, IteratorT const& iterend,
            PositionT const &pos):
        m_Iter(iter),
        m_IterEnd(iterend),
        m_Pos(pos),
        m_CharsPerTab(4),
        m_IsAtEnd(m_Iter == m_IterEnd)
    {
    }
    reference operator *() const { return *m_Iter; }
    pointer operator ->() const { return &*m_Iter; }
    bool is_equal(const position_iterator& i2) const {
        return (m_IsAtEnd && i2.m_IsAtEnd) ||
               (!m_IsAtEnd && !i2.m_IsAtEnd && m_Iter == i2.m_Iter);
    }
    void set_position(PositionT const& newpos) {
        m_Pos = newpos;
    }
    PositionT& get_position() {
        return m_Pos;
    }
    PositionT const& get_position() const {
        return m_Pos;
    }
    position_iterator& operator ++() {
        inc_current();
        return *this;
    }
    position_iterator operator ++(int) {
        const position_iterator result = *this;
        inc_current();
        return result;
    }
    friend difference_type operator -(const position_iterator& i1,
                                      const position_iterator& i2) {
        return i1.m_Iter - i2.m_Iter;
    }

    void set_tabchars(unsigned int chars) {
        m_CharsPerTab = chars;
    }

private:
    IteratorT m_Iter;
    IteratorT m_IterEnd;

    PositionT m_Pos;
    unsigned int m_CharsPerTab;
    bool m_IsAtEnd;

    void inc_current() {
        if (!m_IsAtEnd) {
            value_type val = *m_Iter;
            if (val == '\n' || val == '\r') {
                ++m_Pos.line;
                m_Pos.column = 1;
                ++m_Iter;
                if (m_Iter != m_IterEnd) {
                    value_type val2 = *m_Iter;
                    if ((val == '\n' && val2 == '\r')
                     || (val == '\r' && val2 == '\n')) {
                        ++m_Iter;
                    }
                }
            } else if (val == '\t') {
                m_Pos.column += m_CharsPerTab - (m_Pos.column - 1) % m_CharsPerTab;
                ++m_Iter;
            } else {
                ++m_Pos.column;
                ++m_Iter;
            }
            if (m_Iter == m_IterEnd) {
                m_IsAtEnd = true;
            }
        }
    }
};

template < typename IteratorT, typename PositionT >
class position_iterator;


template < typename IteratorT, typename PositionT >
inline bool operator ==(
    const position_iterator<IteratorT, PositionT>& i1,
    const position_iterator<IteratorT, PositionT>& i2)
{
    return i1.is_equal(i2);
}

template < typename IteratorT, typename PositionT >
inline bool operator !=(
    const position_iterator<IteratorT, PositionT>& i1,
    const position_iterator<IteratorT, PositionT>& i2)
{
    return !(i1 == i2);
}

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif
