//  boost/filesystem/exception.hpp  ------------------------------------------//

// < ----------------------------------------------------------------------- > 
// <   Copyright © 2002 Beman Dawes                                          > 
// <   Copyright © 2001 Dietmar Kühl, All Rights Reserved                    > 
// <                                                                         > 
// <   Permission to use, copy, modify, distribute and sell this             > 
// <   software for any purpose is hereby granted without fee, provided      > 
// <   that the above copyright notice appears in all copies and that        > 
// <   both that copyright notice and this permission notice appear in       > 
// <   supporting documentation. The authors make no representations about   > 
// <   the suitability of this software for any purpose. It is provided      > 
// <   "as is" without express or implied warranty.                          > 
// < ----------------------------------------------------------------------- > 

//  See http://www.boost.org/libs/filesystem for documentation.

//----------------------------------------------------------------------------// 

#ifndef BOOST_FILESYSTEM_EXCEPTION_HPP
#define BOOST_FILESYSTEM_EXCEPTION_HPP

#include <boost/filesystem/path.hpp>

#include <string>
#include <stdexcept>

//----------------------------------------------------------------------------// 

namespace boost
{
  namespace filesystem
  {
    namespace detail
    {
      int system_error_code(); // artifact of POSIX and WINDOWS error reporting
    }

    enum error_code
    {
      no_error = 0,
      system_error,     // system generated error; if possible, is translated
                        // to one of the more specific errors below.
      other_error,      // library generated error
      security_error,   // includes access rights, permissions failures
      read_only_error,
      io_error,
      path_error,
      not_found_error,
      not_directory_error,
      busy_error,       // implies trying again might succeed
      already_exists_error,
      not_empty_error,
      is_directory_error,
      out_of_space_error,
      out_of_memory_error,
      out_of_resource_error
    };


    class filesystem_error : public std::runtime_error
    {
    public:

      filesystem_error(
        const std::string & who,
        const std::string & message ); // assumed to be error_code::other_error

      filesystem_error(
        const std::string & who,
        const path & path1,
        const std::string & message ); // assumed to be error_code::other_error

      filesystem_error(
        const std::string & who,
        const path & path1,
        int sys_err_code );

      filesystem_error(
        const std::string & who,
        const path & path1,
        const path & path2,
        int sys_err_code );

      ~filesystem_error() throw();

      int             native_error() const { return m_sys_err; }
      // Note: a value of 0 implies a library (rather than system) error
      error_code      error() const { return m_err; }
      const std::string &  who() const; // name of who throwing exception
      const path &    path1() const; // argument 1 to who; may be empty()
      const path &    path2() const; // argument 2 to who; may be empty()

    private:
      int             m_sys_err;
      error_code      m_err;
      std::string     m_who;
      path            m_path1;
      path            m_path2;
    };

  } // namespace filesystem
} // namespace boost

#endif // BOOST_FILESYSTEM_EXCEPTION_HPP
