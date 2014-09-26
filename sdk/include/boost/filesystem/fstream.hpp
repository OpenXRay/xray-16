//  boost/filesystem/fstream.hpp  --------------------------------------------//

//  (C) Copyright Beman Dawes 2002. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

//  See http://www.boost.org/libs/filesystem for documentation.

//----------------------------------------------------------------------------// 

#ifndef BOOST_FILESYSTEM_FSTREAM_HPP
#define BOOST_FILESYSTEM_FSTREAM_HPP

#include <boost/filesystem/path.hpp>

#include <iosfwd>
#include <fstream>

namespace boost
{
  namespace filesystem
  {
    template < class charT, class traits = std::char_traits<charT> >
    class basic_filebuf : public std::basic_filebuf<charT,traits>
    {
    public:
      virtual ~basic_filebuf() {}

#if !defined(BOOST_MSVC) || BOOST_MSVC > 1200 // VC++ 6.0 can't handle this
      std::basic_filebuf<charT,traits> * open( const path & file_ph,
        std::ios_base::openmode mode )
      {
        return std::basic_filebuf<charT,traits>::open(
          file_ph.native_file_string().c_str(), mode );
      }
#endif
    };

    typedef basic_filebuf<char> filebuf;
#   ifndef BOOST_NO_STD_WSTRING
    typedef basic_filebuf<wchar_t> wfilebuf;
#   endif

    template < class charT, class traits = std::char_traits<charT> >
    class basic_ifstream : public std::basic_ifstream<charT,traits>
    {
    public:
      basic_ifstream() {}
      explicit basic_ifstream( const path & file_ph,
        std::ios_base::openmode mode = std::ios_base::in )
        : std::basic_ifstream<charT,traits>(
        file_ph.native_file_string().c_str(), mode ) {}
      virtual ~basic_ifstream() {}
#if !defined(BOOST_MSVC) || BOOST_MSVC > 1200 // VC++ 6.0 can't handle this
      void open( const path & file_ph,
        std::ios_base::openmode mode = std::ios_base::in )
      {
        std::basic_ifstream<charT,traits>::open(
          file_ph.native_file_string().c_str(), mode );
      }
#endif
    };

    typedef basic_ifstream<char> ifstream;
#   ifndef BOOST_NO_STD_WSTRING
    typedef basic_ifstream<wchar_t> wifstream;
#   endif

    template < class charT, class traits = std::char_traits<charT> >
    class basic_ofstream : public std::basic_ofstream<charT,traits>
    {
    public:
      basic_ofstream() {}
      explicit basic_ofstream( const path & file_ph,
        std::ios_base::openmode mode = std::ios_base::out )
        : std::basic_ofstream<charT,traits>(
        file_ph.native_file_string().c_str(), mode ) {}
      virtual ~basic_ofstream() {}
#if !defined(BOOST_MSVC) || BOOST_MSVC > 1200 // VC++ 6.0 can't handle this
      void open( const path & file_ph,
        std::ios_base::openmode mode = std::ios_base::out )
      {
        std::basic_ofstream<charT,traits>::open(
          file_ph.native_file_string().c_str(), mode );
      }
#endif
    };

    typedef basic_ofstream<char> ofstream;
#   ifndef BOOST_NO_STD_WSTRING
    typedef basic_ofstream<wchar_t> wofstream;
#   endif

    template < class charT, class traits = std::char_traits<charT> >
    class basic_fstream : public std::basic_fstream<charT,traits>
    {
    public:
      basic_fstream() {}
      explicit basic_fstream( const path & file_ph,
        std::ios_base::openmode mode = std::ios_base::in|std::ios_base::out )
        : std::basic_fstream<charT,traits>(
        file_ph.native_file_string().c_str(), mode ) {}
      virtual ~basic_fstream() {}
#if !defined(BOOST_MSVC) || BOOST_MSVC > 1200 // VC++ 6.0 can't handle this
      void open( const path & file_ph,
        std::ios_base::openmode mode = std::ios_base::in|std::ios_base::out )
      {
        std::basic_fstream<charT,traits>::open(
          file_ph.native_file_string().c_str(), mode );
      }
#endif
    };
 
    typedef basic_fstream<char> fstream;
#   ifndef BOOST_NO_STD_WSTRING
    typedef basic_fstream<wchar_t> wfstream;
#   endif
  } // namespace filesystem

} // namespace boost

#endif  // BOOST_FILESYSTEM_FSTREAM_HPP

