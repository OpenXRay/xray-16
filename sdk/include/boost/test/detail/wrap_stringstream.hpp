//  (C) Copyright Gennadiy Rozental 2002.
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied warranty,
//  and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version including documentation.
//
//  File        : $RCSfile: wrap_stringstream.hpp,v $
//
//  Version     : $Id: wrap_stringstream.hpp,v 1.2.2.1 2003/03/18 04:07:29 rwgk Exp $
//
//  Description : wraps strstream and stringstream (depends with one is present )
//                to prodive the unified interface
// ***************************************************************************

#ifndef BOOST_WRAP_STRINGSTREAM_HPP
#define BOOST_WRAP_STRINGSTREAM_HPP

// STL
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>        // for std::ostrstream
#else
#include <sstream>          // for std::ostringstream
#endif // BOOST_NO_STRINGSTREAM
#include <string>           // std::string
namespace boost {

// ************************************************************************** //
// **************               wrap_stringstream              ************** //
// ************************************************************************** //

class wrap_stringstream {
#ifdef BOOST_NO_STRINGSTREAM
    typedef std::ostrstream     wrapped_stream;
#else
    typedef std::ostringstream  wrapped_stream;
#endif // BOOST_NO_STRINGSTREAM
public:

    // access methods
    inline wrap_stringstream&   ref();
    inline wrapped_stream&      stream();
    inline std::string const&   str();

private:
    // Data members
    wrapped_stream              m_stream;
    std::string                 m_str;
};

//____________________________________________________________________________//

template <class T>
wrap_stringstream&
operator<<( wrap_stringstream& targ, T const& t )
{
    targ.stream() << t;
    return targ;
}

//____________________________________________________________________________//

inline wrap_stringstream::wrapped_stream&
wrap_stringstream::stream()
{
    return m_stream;
}

//____________________________________________________________________________//

inline wrap_stringstream&
wrap_stringstream::ref()
{ 
    return *this;
}

//____________________________________________________________________________//

inline wrap_stringstream&
operator<<( wrap_stringstream& targ, wrap_stringstream& src )
{
    targ << src.str();
    return targ;
}

//____________________________________________________________________________//

inline std::string const&
wrap_stringstream::str()
{

#ifdef BOOST_NO_STRINGSTREAM
    m_str.assign( m_stream.str(), m_stream.pcount() );
    m_stream.freeze( false );
#else
    m_str = m_stream.str();
#endif

    return m_str;
}

//____________________________________________________________________________//

}  // namespace boost

#endif  // BOOST_WRAP_STRINGSTREAM_HPP
