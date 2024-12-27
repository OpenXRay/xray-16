#https://github.com/freeorion/freeorion/blob/master/cmake/FindVorbis.cmake
#.rst:
# FindVorbis
# --------
#
# Find the native Vorbis includes and library.
#
# Specify one or more of the following components as you call this
# find module.
#
# ::
#
#   Vorbis                 - The vorbis decoder library.
#   VorbisEnc              - The vorbis encoder library.
#   VorbisFile             - The vorbis file library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` targets
#
# ::
#
#   ``Vorbis::Vorbis``     - The vorbis decoder library.
#   ``Vorbis::VorbisEnc``  - The vorbis encoder library.
#   ``Vorbis::VorbisFile`` - The vorbis file library.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   VORBIS_INCLUDE_DIRS    - where to find codec.h, etc.
#   VORBIS_LIBRARIES       - List of libraries when using vorbis,
#                            vorbisenc and vorbisfile.
#   VORBIS_FOUND           - True if vorbis found.
#
# Hints
# ^^^^^
#
# A user may set ``VORBISDIR`` environment to a ogg installation root
# to tell this module where to look.

set(_VORBIS_SEARCHES)

# Search VORBISDIR first when it is set.
if(ENV{VORBISDIR})
  set(_VORBIS_SEARCH_ROOT PATH ENV{VORBISDIR} NO_DEFAULT_PATH)
  list(APPEND _VORBIS_SEARCHES _VORBIS_SEARCH_ROOT)
endif()

#Normal search.
set(_VORBIS_SEARCH_NORMAL
  PATH ""
  )
list(APPEND _VORBIS_SEARCHES _VORBIS_SEARCH_NORMAL)

set(VORBIS_NAMES vorbis libvorbis)
set(VORBIS_NAMES_DEBUG vorbisd vorbis_d vorbisD vorbis_D)
set(VORBISENC_NAMES vorbisenc libvorbisenc)
set(VORBISENC_NAMES_DEBUG vorbisencd vorbisenc_d vorbisencD vorbisenc_D)
set(VORBISFILE_NAMES vorbisfile libvorbisfile)
set(VORBISFILE_NAMES_DEBUG vorbisfiled vorbisfile_d vorbisfileD vorbisfile_D)

foreach(search ${_VORBIS_SEARCHES})
  find_path(VORBIS_INCLUDE_DIR NAMES vorbis/codec.h ${${search}} PATH_SUFFIXES include)
endforeach()

if(${VORBIS_INCLUDE_DIR} MATCHES ".framework")
    set(_components VORBIS)
else()
    set(_components VORBIS VORBISENC VORBISFILE)
endif()

foreach(_component ${_components})
  # Allow ${_component}_LIBRARY to be set manually, as the location of the
  # corresponding library
  if(NOT ${_component}_LIBRARY)
    foreach(search ${_VORBIS_SEARCHES})
      find_library(${_component}_LIBRARY_RELEASE NAMES ${${_component}_NAMES} ${${search}} PATH_SUFFIXES lib)
      find_library(${_component}_LIBRARY_DEBUG NAMES ${${_component}_NAMES_DEBUG} ${${search}} PATH_SUFFIXES lib)
    endforeach()

    include(SelectLibraryConfigurations)
    select_library_configurations(${_component})
  endif()
endforeach()

if(${VORBIS_INCLUDE_DIR} MATCHES ".framework" AND VORBIS_FOUND)
    set(VORBISENC_FOUND TRUE)
    set(VORBISENC_LIBRARY ${VORBIS_LIBRARY})
    set(VORBISFILE_FOUND TRUE)
    set(VORBISFILE_LIBRARY ${VORBIS_LIBRARY})
endif()

unset(VORBIS_NAMES)
unset(VORBIS_NAMES_DEBUG)
unset(VORBISENC_NAMES)
unset(VORBISENC_NAMES_DEBUG)
unset(VORBISFILE_NAMES)
unset(VORBISFILE_NAMES_DEBUG)

mark_as_advanced(VORBIS_INCLUDE_DIR VORBIS_LIBRARY VORBISENC_LIBRARY VORBISFILE_LIBRARY)

if(VORBIS_FOUND)
    set(Vorbis_Vorbis_FOUND true)
endif()

if(VORBISENC_FOUND)
    set(Vorbis_VorbisEnc_FOUND true)
endif()

if(VORBISFILE_FOUND)
    set(Vorbis_VorbisFile_FOUND true)
endif()

# handle the QUIETLY and REQUIRED argument and set VORBIS_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Vorbis REQUIRED_VARS VORBIS_LIBRARY VORBIS_INCLUDE_DIR HANDLE_COMPONENTS)

unset(Vorbis_Vorbis_FOUND)
unset(Vorbis_VorbisEnc_FOUND)
unset(Vorbis_VorbisFile_FOUND)

if(VORBIS_FOUND)
    set(VORBIS_INCLUDE_DIRS ${VORBIS_INCLUDE_DIR})

    set(VORBIS_LIBRARIES ${VORBIS_LIBRARY})
    if(VORBISENC_FOUND)
        list(APPEND VORBIS_LIBRARIES ${VORBISENC_LIBRARY})
    endif()
    if(VORBISFILE_FOUND)
        list(APPEND VORBIS_LIBRARIES ${VORBISFILE_LIBRARY})
    endif()

    if(NOT TARGET Vorbis::Vorbis)
      add_library(Vorbis::Vorbis UNKNOWN IMPORTED)

      set_target_properties(Vorbis::Vorbis PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${VORBIS_INCLUDE_DIRS}")

      if(VORBIS_LIBRARY_RELEASE)
        set_property(TARGET Vorbis::Vorbis APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(Vorbis::Vorbis PROPERTIES
          IMPORTED_LOCATION_RELEASE "${VORBIS_LIBRARY_RELEASE}")
      endif()

      if(VORBIS_LIBRARY_DEBUG)
        set_property(TARGET Vorbis::Vorbis APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(Vorbis::Vorbis PROPERTIES
          IMPORTED_LOCATION_DEBUG "${VORBIS_LIBRARY_DEBUG}")
      endif()

      if(NOT VORBIS_LIBRARY_RELEASE AND NOT VORBIS_LIBRARY_DEBUG)
        set_property(TARGET Vorbis::Vorbis APPEND PROPERTY
          IMPORTED_LOCATION "${VORBIS_LIBRARY}")
      endif()
    endif()

    if(NOT TARGET Vorbis::VorbisEnc)
      add_library(Vorbis::VorbisEnc UNKNOWN IMPORTED)

      set_target_properties(Vorbis::VorbisEnc PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${VORBISENC_INCLUDE_DIRS}")

      if(VORBISENC_LIBRARY_RELEASE)
        set_property(TARGET Vorbis::VorbisEnc APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(Vorbis::VorbisEnc PROPERTIES
          IMPORTED_LOCATION_RELEASE "${VORBISENC_LIBRARY_RELEASE}")
      endif()

      if(VORBISENC_LIBRARY_DEBUG)
        set_property(TARGET Vorbis::VorbisEnc APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(Vorbis::VorbisEnc PROPERTIES
          IMPORTED_LOCATION_DEBUG "${VORBISENC_LIBRARY_DEBUG}")
      endif()

      if(NOT VORBISENC_LIBRARY_RELEASE AND NOT VORBISENC_LIBRARY_DEBUG)
        set_property(TARGET Vorbis::VorbisEnc APPEND PROPERTY
          IMPORTED_LOCATION "${VORBISENC_LIBRARY}")
      endif()

      add_dependencies(Vorbis::Vorbis Vorbis::VorbisEnc)
    endif()

    if(NOT TARGET Vorbis::VorbisFile)
      add_library(Vorbis::VorbisFile UNKNOWN IMPORTED)

      set_target_properties(Vorbis::VorbisFile PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${VORBISFILE_INCLUDE_DIRS}")

      if(VORBISFILE_LIBRARY_RELEASE)
        set_property(TARGET Vorbis::VorbisFile APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(Vorbis::VorbisFile PROPERTIES
          IMPORTED_LOCATION_RELEASE "${VORBISFILE_LIBRARY_RELEASE}")
      endif()

      if(VORBISFILE_LIBRARY_DEBUG)
        set_property(TARGET Vorbis::VorbisFile APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(Vorbis::VorbisFile PROPERTIES
          IMPORTED_LOCATION_DEBUG "${VORBISFILE_LIBRARY_DEBUG}")
      endif()

      if(NOT VORBISFILE_LIBRARY_RELEASE AND NOT VORBISFILE_LIBRARY_DEBUG)
        set_property(TARGET Vorbis::VorbisFile APPEND PROPERTY
          IMPORTED_LOCATION "${VORBISFILE_LIBRARY}")
      endif()

      add_dependencies(Vorbis::Vorbis Vorbis::VorbisEnc)
    endif()
endif()
