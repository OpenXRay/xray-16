//  boost/filesystem/directory.hpp  ------------------------------------------//

// < ----------------------------------------------------------------------- > 
// <   Copyright © 2002 Beman Dawes.                                         >
// <   Copyright © 2002 Jan Langer.                                          >
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

#ifndef BOOST_FILESYSTEM_DIRECTORY_HPP
#define BOOST_FILESYSTEM_DIRECTORY_HPP

#include <boost/config.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/iterator.hpp>

#include <string>

//----------------------------------------------------------------------------//

namespace boost
{
  namespace filesystem
  {

//  query functions  ---------------------------------------------------------//

    bool exists( const path & ph );

    bool is_directory( const path & ph );

    // VC++ 7.0 and earlier has a serious namespace bug that causes a clash
    // between boost::filesystem::is_empty and the unrelated type trait
    // boost::is_empty. The workaround for those who must use broken versions
    // of VC++ is to use the function _is_empty. All others should use the
    // correct is_empty name.
    bool _is_empty( const path & ph ); // deprecated

#   if !defined( BOOST_MSVC ) || BOOST_MSVC > 1300
    inline bool is_empty( const path & ph ) { return _is_empty( ph ); }
#   endif

//  operations  --------------------------------------------------------------//

    void create_directory( const path & directory_ph );

    bool remove( const path & ph );
    unsigned long remove_all( const path & ph );

    void rename( const path & from_path,
                 const path & to_path );

    void copy_file( const path & from_file_ph,
                    const path & to_file_ph );

    path          current_path();
    const path &  initial_path();

    path          system_complete( const path & ph );
    path          complete( const path & ph, const path & base = initial_path() );

//  directory_iterator  ------------------------------------------------------//

    class directory_iterator
      : public boost::iterator< std::input_iterator_tag,
          path, std::ptrdiff_t, const path *, const path & >
    {
    private:
      typedef directory_iterator self;
    public:
      directory_iterator();  // creates the "end" iterator
      explicit directory_iterator( const path & p );

      reference operator*() const { return m_deref(); }
      pointer   operator->() const { return &m_deref(); }
      self &    operator++() { m_inc(); return *this; }

      friend bool operator==( const self & x, const self & y )
        { return x.m_imp == y.m_imp; }
      friend bool operator!=( const self & x, const self & y )
        { return !(x.m_imp == y.m_imp); }

      struct path_proxy // allows *i++ to work, as required by std
      {
        path pv;
        explicit path_proxy( const path & p ) : pv(p) {}
        path operator*() const { return pv; }
      };

      path_proxy operator++(int)
      {
        path_proxy pp( m_deref() );
        ++*this;
        return pp;
      }

    private:
      class dir_itr_imp;
      // shared_ptr provides shallow-copy semantics required for InputIterators
      typedef boost::shared_ptr< dir_itr_imp > m_imp_ptr;
      m_imp_ptr  m_imp;
      reference  m_deref() const;
      void       m_inc();
    };

  } // namespace filesystem
} // namespace boost

#endif // BOOST_FILESYSTEM_DIRECTORY_HPP
