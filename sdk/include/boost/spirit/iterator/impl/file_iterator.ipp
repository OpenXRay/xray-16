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

#if !defined FILE_ITERATOR_IPP
#define      FILE_ITERATOR_IPP

///////////////////////////////////////////////////////////////////////////////
#include <cassert>
#include <fcntl.h>

#if (defined (__COMO_VERSION__) && !defined (BOOST_DISABLE_WIN32)) \
      || defined _BORLANDC_ || defined BOOST_INTEL_CXX_VERSION  \
      || defined BOOST_MSVC
    #include <io.h>
#elif defined (__GLIBC__) && (__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 3)

   #include <unistd.h>

#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
namespace impl {

// Systems that have _lseeki64, such as Win32.
#if defined BOOST_MSVC              \
 || defined __MINGW32_VERSION       \
 || defined BOOST_INTEL_CXX_VERSION

   // These systems default to CR-LF translation for opened files. Using
   // O_BINARY turns off this translation.
   static const int kOpenFlags = O_RDONLY | O_BINARY;

   // Import _lseeki64 into this namespace under the name lseek
   inline off_t lseek (int File, off_t Offset, int Whence)
   {
      return _lseeki64 (File, Offset, Whence);
   }

// Systems that have lseek, whether it is 64-bit or 32-bit
#else

   // Some non-POSIX systems default to CR-LF translation for opened
   // files. Using O_BINARY turns off this translation.
   #if defined __CYGWIN__ || defined __BORLANDC__

      static const int kOpenFlags = O_RDONLY | O_BINARY;

   // POSIX systems don't do any CR-LF translation, and don't have an
   // O_BINARY flag. POSIX systems don't do this because it is ignored
   // on all POSIX conforming systems, according to the Linux man page
   // for fopen.
   #else

      static const int kOpenFlags = O_RDONLY;

   #endif

   // Import lseek into this namespace
   using ::lseek;

#endif

///////////////////////////////////////////////////////////////////////////////
} // namespace impl

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT>::file_iterator (char const * pFilename)
 : m_Filesize  (0),
   m_File      (-1),
   m_Offset    (0),
   m_NextChar  (),
   m_CloseFile (false)
{
   assert (NULL != pFilename);

   m_File = open (pFilename, impl::kOpenFlags);

   if (-1 != m_File)
   {
      // This instance is responsible for closing the file
      m_CloseFile = true;

      // Get the file size in bytes
      m_Filesize = impl::lseek (m_File, 0, SEEK_END);

      // The above shouldn't fail, but you never know...
      if (0 > m_Filesize)
      {
         close (m_File);

         m_CloseFile = false;

         m_File = -1;
      }

      // Move the file pointer back to the beginning
      impl::lseek (m_File, 0, SEEK_SET);

      // Read the first character
      read (m_File, &m_NextChar, kCharSize);
   }
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT>::~file_iterator ()
{
   if (
         m_CloseFile
      && (-1 != m_File)
      )
   {
      close (m_File);
   }
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT> file_iterator <CharT>::make_end ()
{
   file_iterator <CharT> _Iterator (*this);

   // Make sure the offset is correctly aligned
   _Iterator.m_Offset = m_Filesize - (m_Filesize % kCharSize);

   return _Iterator;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT>::operator bool () const
{
   return (-1 != m_File);
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline typename file_iterator <CharT>::difference_type
   file_iterator <CharT>::size () const
{
   return m_Filesize;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT>::file_iterator ()
 : m_Filesize  (0),
   m_File      (-1),
   m_Offset    (-1),
   m_NextChar  (),
   m_CloseFile (false)
{
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT>::file_iterator (
   file_iterator const & rCopyMe
   )
 : m_Filesize  (rCopyMe.m_Filesize),
   m_File      (rCopyMe.m_File),
   m_Offset    (rCopyMe.m_Offset),
   m_NextChar  (rCopyMe.m_NextChar),
   m_CloseFile (false)
{
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT> & file_iterator <CharT>::operator = (
   file_iterator const & rAssignMe
   )
{
   // Clean up previous file if necessary
   if (
         m_CloseFile
      && (-1 != m_File)
      && (m_File != rAssignMe.m_File)
      )
   {
      close (m_File);
   }

   m_File      = rAssignMe.m_File;
   m_Offset    = rAssignMe.m_Offset;
   m_Filesize  = rAssignMe.m_Filesize;
   m_NextChar  = rAssignMe.m_NextChar;

   return *this;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline void file_iterator <CharT>::swap (
   file_iterator <CharT> & rA,
   file_iterator <CharT> & rB
   )
{
   file_iterator <CharT> _Temp (rA);

   rA = rB;

   rB = _Temp;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline typename file_iterator <CharT>::reference
   file_iterator <CharT>::operator * () const
{
   assert (-1 != m_File);

   return m_NextChar;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline typename file_iterator <CharT>::pointer
   file_iterator <CharT>::operator -> () const
{
   assert (-1 != m_File);

   return &m_NextChar;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT> const &
   file_iterator <CharT>::operator ++ () const
{
   assert (-1 != m_File);

   m_Offset += kCharSize;

   impl::lseek (m_File, m_Offset, SEEK_SET);

   read (m_File, &m_NextChar, kCharSize);

   return *this;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT> const
   file_iterator <CharT>::operator ++ (int) const
{
   assert (-1 != m_File);

   file_iterator _Temp (*this);

   m_Offset += kCharSize;

   impl::lseek (m_File, m_Offset, SEEK_SET);

   read (m_File, &m_NextChar, kCharSize);

   return _Temp;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT> const &
   file_iterator <CharT>::operator -- () const
{
   assert (-1 != m_File);

   m_Offset -= kCharSize;

   impl::lseek (m_File, m_Offset, SEEK_SET);

   read (m_File, &m_NextChar, kCharSize);

   return *this;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline file_iterator <CharT> const
   file_iterator <CharT>::operator -- (int) const
{
   assert (-1 != m_File);

   file_iterator _Temp (*this);

   m_Offset -= kCharSize;

   impl::lseek (m_File, m_Offset, SEEK_SET);

   read (m_File, &m_NextChar, kCharSize);

   return _Temp;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
file_iterator <CharT> const & file_iterator <CharT>::operator += (
   difference_type Distance
   ) const
{
   assert (-1 != m_File);

   m_Offset += Distance * kCharSize;

   impl::lseek (m_File, m_Offset, SEEK_SET);

   read (m_File, &m_NextChar, kCharSize);

   return *this;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
file_iterator <CharT> const file_iterator <CharT>::operator + (
   difference_type Distance
   ) const
{
   file_iterator <CharT> _Temp (*this);

   _Temp += Distance;

   return _Temp;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
file_iterator <CharT> const & file_iterator <CharT>::operator -= (
   difference_type Distance
   ) const
{
   assert (-1 != m_File);

   m_Offset -= Distance * kCharSize;

   impl::lseek (m_File, m_Offset, SEEK_SET);

   read (m_File, &m_NextChar, kCharSize);

   return *this;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
file_iterator <CharT> const file_iterator <CharT>::operator - (
   difference_type Distance
   ) const
{
   file_iterator <CharT> _Temp (*this);

   _Temp -= Distance;

   return _Temp;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
typename file_iterator <CharT>::difference_type
   file_iterator <CharT>::operator - (
   file_iterator <CharT> const & rIterator
   ) const
{
   return ((m_Offset - rIterator.m_Offset) / kCharSize);
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline typename file_iterator <CharT>::value_type
   file_iterator <CharT>::operator [] (difference_type Index) const
{
   assert (-1 != m_File);

   CharT _ReturnValue;

   impl::lseek (m_File, m_Offset, SEEK_SET);

   read (m_File, &m_NextChar, kCharSize);

   return _ReturnValue;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
file_iterator <CharT> const operator + (
   typename file_iterator <CharT>::difference_type   Distance,
   file_iterator <CharT> const                     & rIterator
   )
{
   file_iterator <CharT> _Temp (rIterator);

   _Temp += Distance;

   return _Temp;
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline bool file_iterator <CharT>::operator_equal_to (
   const file_iterator <CharT> & rOther
   ) const
{
//   assert (m_File == rOther.m_File);

   return (m_Offset == rOther.m_Offset);
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline bool operator == (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   )
{
   return rA.operator_equal_to (rB);
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline bool operator != (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   )
{
   return !(rA.operator_equal_to (rB));
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline bool file_iterator <CharT>::operator_less_than (
   file_iterator <CharT> const & rOther
   ) const
{
   assert (m_File == rOther.m_File);

   return (m_Offset < rOther.m_Offset);
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline bool operator < (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   )
{
   return rA.operator_less_than (rB);
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline bool operator >= (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   )
{
   return !(rA.operator_less_than (rB));
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline bool file_iterator <CharT>::operator_greater_than (
   file_iterator <CharT> const & rOther
   ) const
{
   assert (m_File == rOther.m_File);

   return (m_Offset > rOther.m_Offset);
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline bool operator > (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   )
{
   return (rA.operator_greater_than (rB));
}

///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline bool operator <= (
   file_iterator <CharT> const & rA,
   file_iterator <CharT> const & rB
   )
{
   return !(rA.operator_greater_than (rB));
}

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif // #if !defined FILE_ITERATOR_IPP
