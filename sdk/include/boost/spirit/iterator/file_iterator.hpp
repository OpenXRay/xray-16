/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2002 Jeff Westfahl
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/

// This code is inspired by FILEiter.h by Stephen Cleary, and is basically a
// superset of the functionality provided there. FILEiter.h is available from
// http://groups.yahoo.com/group/boost/files/2000/IOGlue/IOGlue.zip and has
// the following copyright notice:

// FILEiter: Input and Output iterators for FILE * objects

// (C) Copyright Stephen Cleary 2000.  Permission to copy, use, modify, sell and
// distribute this software is granted provided this copyright notice appears
// in all copies.  This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.

//
// Notes on 64-bit file access:
//
// The following systems are known to have 64-bit lseek support. Note that on
// Linux, you must define _FILE_OFFSET_BITS=64 and _LARGEFILE_SOURCE. This
// should be done on the compiler command line so that it is used globally.
//
//    MSVC  (tested on VC6 SP4)
//    MINGW (tested on recent CYGWIN)
//    Linux (recent, tested on Mandrake Linux 8.2 X86 and PPC)
//    Intel C++ (untested, but it uses system headers and libraries)
//
// The following systems are known to not have 64-bit lseek support, and do
// not seem to export any 64-bit file access functions. This may or may not
// be a problem for you. I know that I don't have a lot of files larger than
// 2GB laying around on my hard drives, except for testing. But, you never
// can tell how people might want to use your software.
//
//    Borland 5.5.1
//    CYGWIN
//
// Basically, this iterator should work fine on any files < 2GB in size on any
// system.

#if !defined FILE_ITERATOR_HPP
#define      FILE_ITERATOR_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/config.hpp>
#include <boost/iterator.hpp>
#include <cstdio>
#if _MSL_
#include <sys/stat.h>
#else
#include <sys/types.h>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
namespace impl {

// Systems that don't have a 64-bit off_t, but do have a 64-bit lseek
#if defined BOOST_MSVC              \
 || defined __MINGW32_VERSION       \
 || defined BOOST_INTEL_CXX_VERSION

   // Always use 64-bit file access
   typedef __int64 off_t;

#else

   // Use the system defined offset size
   using ::off_t;

#endif

///////////////////////////////////////////////////////////////////////////////
} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// warning C4284: return type for 'boost::spirit::file_iterator<char>::operator ->'
// is 'const char *'
#if defined BOOST_MSVC
#pragma warning ( push )
#pragma warning ( disable : 4284 )
#endif

///////////////////////////////////////////////////////////////////////////////
template <typename CharT = char>
class file_iterator
 : public boost::iterator <
      std::random_access_iterator_tag,
      CharT,
      impl::off_t,
      CharT const *,
      CharT const &
      >
{
   public:

      // Iterator traits
      typedef impl::off_t   difference_type;
      typedef CharT         value_type;
      typedef CharT const * pointer;
      typedef CharT const & reference;

      // Standard constructor. Opens the specified file.
      explicit file_iterator (char const * pFilename);

      // Destructor
      ~file_iterator ();

      // Make an end of file iterator for the current file
      file_iterator make_end ();

      // Returns false if no file is associated with this iterator
      operator bool () const;

      // Returns file size in bytes
      difference_type size () const;

      //
      // Default Constructible
      //

      // Default constructor
      file_iterator ();

      //
      // Assignable
      //

      // Copy constructor
      file_iterator (file_iterator const & rCopyMe);

      // Assignment operator
      file_iterator & operator = (file_iterator const & rAssignMe);

      // Swap
      void swap (file_iterator & rA, file_iterator & rB);

      //
      // Trivial Iterator
      //

      // Dereference
      //
      //    Note: Assigning to this will not fail, but is pointless as this
      //    iterator is only meant to be read from.
      reference operator * () const;

      // Member access
      pointer operator -> () const;

      //
      // Input and Forward Iterator
      //

      // Preincrement
      file_iterator const & operator ++ () const;

      // Postincrement
      file_iterator const operator ++ (int) const;

      //
      // Bidirectional Iterator
      //

      // Predecrement
      file_iterator const & operator -- () const;

      // Postdecrement
      file_iterator const operator -- (int) const;

      //
      // Random Access Iterator
      //

      // Iterator addition (i += n)
      file_iterator const & operator += (difference_type Distance) const;

      // Iterator addition (i + n)
      file_iterator const operator + (difference_type Distance) const;

      // Iterator subtraction (i -= n)
      file_iterator const & operator -= (difference_type Distance) const;

      // Iterator subtraction (i - n)
      file_iterator const operator - (difference_type Distance) const;

      // Difference (i - j)
      difference_type operator - (file_iterator const & rIterator) const;

      // Element
      value_type operator [] (difference_type Index) const;

      //
      // Equality Comparable
      //

      bool operator_equal_to (file_iterator const & rOther) const;

      //
      // LessThan Comparable
      //

      bool operator_less_than (file_iterator const & rOther) const;

      bool operator_greater_than (file_iterator const & rOther) const;

   private:

      // Character size
      BOOST_STATIC_CONSTANT (int, kCharSize = sizeof (CharT));

      // Length of the file in bytes
      difference_type m_Filesize;

      // File descriptor
      int m_File;

      // Current offset into the file
      mutable difference_type m_Offset;

      // The next character to be returned
      mutable CharT m_NextChar;

      // True if this instance is responsible for closing the file
      bool m_CloseFile;
};

// Iterator addition (n + i)
template <typename CharT>
file_iterator <CharT> const operator + (
   typename file_iterator <CharT>::difference_type   Distance,
   file_iterator <CharT> const                     & rIterator
   );

//
// Equality Comparable
//

// Equality
template <typename CharT>
bool operator == (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   );

// Inequality
template <typename CharT>
bool operator != (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   );

//
// LessThan Comparable
//

// Less
template <typename CharT>
bool operator < (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   );

// Greater
template <typename CharT>
bool operator > (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   );

// Less or equal
template <typename CharT>
bool operator <= (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   );

// Greater or equal
template <typename CharT>
bool operator >= (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   );

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

///////////////////////////////////////////////////////////////////////////////
#if defined BOOST_MSVC
#pragma warning ( pop )
#endif

///////////////////////////////////////////////////////////////////////////////
#endif // #if !defined  FILE_ITERATOR_HPP

///////////////////////////////////////////////////////////////////////////////
#if !defined FILE_ITERATOR_IPP
   #include "boost/spirit/iterator/impl/file_iterator.ipp"
#endif
