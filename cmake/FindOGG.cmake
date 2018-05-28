#https://raw.githubusercontent.com/freeorion/freeorion/master/cmake/FindOgg.cmake
#.rst:
# FindOgg
# -------
#
# Find the native Ogg includes and library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``Ogg::Ogg``, if
# Ogg has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   OGG_INCLUDE_DIRS    - where to find ogg.h, etc.
#   OGG_LIBRARIES       - List of libraries when using ogg.
#   OGG_FOUND           - True if ogg found.
#
# Hints
# ^^^^^
#
# A user may set ``OGGDIR`` environment to a ogg installation root
# to tell this module where to look.


set(_OGG_SEARCHES)

# Search OGGDIR first when is set.
if(ENV{OGGDIR})
  set(_OGG_SEARCH_ROOT PATHS $ENV{OGGDIR} NO_DEFAULT_PATH)
  list(APPEND _OGG_SEARCHES _OGG_SEARCH_ROOT)
endif()

# Normal search.
set(_OGG_SEARCH_NORMAL
  PATH ""
  )
list(APPEND _OGG_SEARCHES _OGG_SEARCH_NORMAL)

set(OGG_NAMES ogg libogg)
set(OGG_NAMES_DEBUG oggd ogg_D oggD ogg_D)

foreach(search ${_OGG_SEARCHES})
  find_path(OGG_INCLUDE_DIR NAMES ogg.h ${${search}} PATH_SUFFIXES ogg)
endforeach()

# Allow OGG_LIBRARY to be set manually, as the location of the
# ogg library
if(NOT OGG_LIBRARY)
  foreach(search ${_OGG_SEARCHES})
    find_library(OGG_LIBRARY_RELEASE NAMES ${OGG_NAMES} ${${search}} PATH_SUFFIXES lib)
    find_library(OGG_LIBRARY_DEBUG NAMES ${OGG_NAMES_DEBUG} ${${search}} PATH_SUFFIXES lib)
  endforeach()

  include(SelectLibraryConfigurations)
  select_library_configurations(OGG)
endif()

unset(OGG_NAMES)
unset(OGG_NAMES_DEBUG)

mark_as_advanced(OGG_LIBRARY OGG_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED argument and set OGG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ogg REQUIRED_VARS OGG_LIBRARY OGG_INCLUDE_DIR)

if(OGG_FOUND)
    set(OGG_INCLUDE_DIRS ${OGG_INCLUDE_DIR})

    if(NOT OGG_LIBRARIES)
        set(OGG_LIBRARIES ${OGG_LIBRARY})
    endif()

    if(NOT TARGET Ogg::Ogg)
      add_library(Ogg::Ogg UNKNOWN IMPORTED)
      set_target_properties(Ogg::Ogg PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OGG_INCLUDE_DIRS}")

      if(OGG_LIBRARY_RELEASE)
        set_property(TARGET Ogg::Ogg APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(Ogg::Ogg PROPERTIES
          IMPORTED_LOCATION_RELEASE "${OGG_LIBRARY_RELEASE}")
      endif()

      if(OGG_LIBRARY_DEBUG)
        set_property(TARGET Ogg::Ogg APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(Ogg::Ogg PROPERTIES
          IMPORTED_LOCATION_DEBUG "${OGG_LIBRARY_DEBUG}")
      endif()

      if(NOT OGG_LIBRARY_RELEASE AND NOT OGG_LIBRARY_DEBUG)
        set_property(TARGET Ogg::Ogg APPEND PROPERTY
          IMPORTED_LOCATION "${OGG_LIBRARY}")
      endif()
    endif()
endif()