//  boost/filesystem/path.hpp  -----------------------------------------------//

//  (C) Copyright Beman Dawes 2002. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.


//  See http://www.boost.org/libs/filesystem for documentation.

//----------------------------------------------------------------------------// 

#ifndef BOOST_FILESYSTEM_PATH_HPP
#define BOOST_FILESYSTEM_PATH_HPP

#include <boost/iterator_adaptors.hpp>
#include <string>
#include <cassert>

//----------------------------------------------------------------------------//

namespace boost
{
  namespace filesystem
  {
    class path;

    namespace detail
    {
      struct path_itr_imp
      {
        std::string             name;     // cache current element.
        const path *            path_ptr; // path being iterated over.
        std::string::size_type  pos;      // position of name in
                                          // path_ptr->string(). The
                                          // end() iterator is indicated by 
                                          // pos == path_ptr->string().size()

        const std::string & operator*() const { return name; }
        void operator++();
        void operator--();
        bool operator==( const path_itr_imp & rhs ) const
          { return path_ptr == rhs.path_ptr && pos == rhs.pos; }
      };
    } // detail

    enum path_format { native }; // ugly enough to discourage use
                                 // except when actually needed

  //  path -------------------------------------------------------------------//

    class path
    {
    public:
      // compiler generates copy constructor, copy assignment, and destructor

      path(){}

      path( const std::string & src );
      path( const char * src );

      path( const std::string & src, path_format );
      path( const char * src, path_format );

      // append operations:
      path & operator /=( const path & rhs );
      path operator /( const path & rhs ) const
        { return path( *this ) /= rhs; }

      // conversion functions:
      const std::string & string() const { return m_path; }
      std::string native_file_string() const;
      std::string native_directory_string() const;

      // decomposition functions:
      path         root_path() const;
      std::string  root_name() const;
      std::string  root_directory() const;
      path         relative_path() const;
      std::string  leaf() const;
      path         branch_path() const;

      // query functions:
      bool empty() const { return m_path.empty(); } // name consistent with std containers

      bool is_complete() const;

      bool has_root_path() const;
      bool has_root_name() const;
      bool has_root_directory() const;
      bool has_relative_path() const;
      bool has_leaf() const { return !m_path.empty(); }
      bool has_branch_path() const;

      // iteration over the names in the path:
      typedef boost::iterator_adaptor<
        detail::path_itr_imp,
        boost::default_iterator_policies,
        std::string,
        const std::string &,
        const std::string *,
        std::bidirectional_iterator_tag,
        std::ptrdiff_t 
        > iterator;

      iterator begin() const;
      iterator end() const
      {
        iterator itr;
        itr.base().path_ptr = this;
        itr.base().pos = m_path.size();
        return itr;
      }

    private:
      // Note: This is an implementation for POSIX and Windows, where there
      // are only minor differences between generic and system-specific
      // constructor input formats.  Private members might be quite different
      // in other implementations, particularly where there were wide
      // differences between generic and system-specific argument formats,
      // or between native_file_string() and native_directory_string() formats.

      std::string  m_path;

      friend class directory_iterator;
      friend struct boost::filesystem::detail::path_itr_imp;

      enum source_context { generic, platform, nocheck };

      void m_path_append( const std::string & src,
        source_context context = generic );

    public: // should be private, but friend functions don't work for me
      void m_replace_leaf( const char * new_leaf );
    };

  //  path non-member functions  ---------------------------------------------//

    inline path operator / ( const char * lhs, const path & rhs )
      { return path( lhs ) /= rhs; }

    inline path operator / ( const std::string & lhs, const path & rhs )
      { return path( lhs ) /= rhs; }
   
  //  error checking  --------------------------------------------------------//

// TODO: write a program that probes valid file and directory names.  Ask
// Boost people to report results from many operating systems.  Use results
// to adjust generic_name().

    //  generic_name() is extremely permissive; its intent is not to ensure
    //  general portablity, but rather to detect names so trouble-prone that
    //  they likely represent coding errors or gross misconceptions.
    //
    //  Any characters are allowed except:
    //
    //     Those characters < ' ', including '\0'. These are the so-called
    //     control characters, in both ASCII (and its decendents) and EBCDIC.
    //     Hard to imagine how these could be useful in a generic path name.
    //
    //     < > : " / \ | * ?  These have special meaning to enough operating
    //     systems that use in a generic name would be a serious problem.
    //
    //  The names "." and ".." are not allowed.
    //  An empty name (null string) is not allowed.
    //  Names beginning or ending with spaces are not allowed.
    //
    bool generic_name( const std::string & name ); 

    //  posix_name() is based on POSIX (IEEE Std 1003.1-2001)
    //  "Portable Filename Character Set" rules.
    //  http://www.opengroup.org/onlinepubs/007904975/basedefs/xbd_chap03.html
    //
    //  That character set only allows 0-9, a-z, A-Z, '.', '_', and '-'.
    //  Note that such names are also portable to other popular operating
    //  systems, such as Windows.
    //
    bool posix_name( const std::string & name );

    const path & check_posix_leaf( const path & ph );
    //  Throws: if !posix_name( ph.leaf() )
    //  Returns: ph
    //  Note: Although useful in its own right, check_posix_leaf() also serves
    //  as an example.  A user might provide similar functions; behavior might
    //  be to assert or warn rather than throw. A user provided function
    //  could also check the entire path instead of just the leaf; a leaf
    //  check is often, but not always, the required behavior.
    //  Rationale: For the "const path &" rather than "void" return is to
    //  allow (and encourage portability checking) uses like:
    //      create_directory( check_posix_leaf( "foo" ) );
    //  While there is some chance of misuse (by passing through a reference
    //  to a temporary), the benefits outweigh the costs.

    //  For Boost, we often tighten name restrictions for maximum portability:
    //
    //    * The portable POSIX character restrictions, plus
    //    * Maximum name length 31 characters (for Classic Mac OS).
    //    * Lowercase only (so code written on case-insensitive platforms like
    //      Windows works properly when used on case-sensitive systems like
    //      POSIX.
    //    * Directory names do not contain '.', as this is not a valid character
    //      for directory names on some systems.
    //
    //  TODO: provide some check_boost_xxx functions once error handling
    //  approach ratified.

    bool boost_file_name( const std::string & name );
    bool boost_directory_name( const std::string & name );

  } // namespace filesystem
} // namespace boost

#endif // BOOST_FILESYSTEM_PATH_HPP
