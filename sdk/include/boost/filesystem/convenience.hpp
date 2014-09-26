//  boost/filesystem/convenience.hpp  ----------------------------------------//

//  (C) Copyright Beman Dawes, 2002
//  (C) Copyright Vladimir Prus, 2002
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.


//  See http://www.boost.org/libs/filesystem for documentation.

//----------------------------------------------------------------------------// 

#ifndef BOOST_FILESYSTEM_CONVENIENCE_HPP
#define BOOST_FILESYSTEM_CONVENIENCE_HPP

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

namespace boost
{
  namespace filesystem
  {

//  create_directories (contributed by Vladimir Prus)  -----------------------//


    /** Creates directory 'ph' and all necessary parent directories.
        @post exists(directory_ph) && is_directory(directory_ph) && is_empty(directory_ph)
     */
    void create_directories(const path& ph);

  } // namespace filesystem
} // namespace boost
#endif // BOOST_FILESYSTEM_CONVENIENCE_HPP


