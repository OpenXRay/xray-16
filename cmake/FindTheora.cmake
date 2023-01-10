# Based on FindVorbis.cmake
#.rst:
# FindTheora
# --------
#
# Find the native Theora includes and library.
#
# Specify one or more of the following components as you call this
# find module.
#
# ::
#
#   Theora                 - The theora library.
#   TheoraDec              - The theora decoder library.
#   TheoraEnc              - The theora encoder library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` targets
#
# ::
#
#   ``Theora::Theora``     - The theora library.
#   ``Theora::TheoraDec``  - The theora decoder library.
#   ``Theora::TheoraEnc``  - The theora encoder library.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   THEORA_INCLUDE_DIRS    - where to find codec.h, etc.
#   THEORA_LIBRARIES       - List of libraries when using theora,
#                            theoradec and theoraenc.
#   THEORA_FOUND           - True if theora found.
#
# Hints
# ^^^^^
#
# A user may set ``THEORADIR`` environment to a theora installation root
# to tell this module where to look.

set(_THEORA_SEARCHES)

# Search THEORADIR first when it is set.
if(ENV{THEORADIR})
  set(_THEORA_SEARCH_ROOT PATH ENV{THEORADIR} NO_DEFAULT_PATH)
  list(APPEND _THEORA_SEARCHES _THEORA_SEARCH_ROOT)
endif()

#Normal search.
set(_THEORA_SEARCH_NORMAL
  PATH ""
  )
list(APPEND _THEORA_SEARCHES _THEORA_SEARCH_NORMAL)

set(THEORA_NAMES theora libtheora)
set(THEORA_NAMES_DEBUG theorad theora_d theoraD theora_D)
set(THEORADEC_NAMES theoradec libtheoradec)
set(THEORADEC_NAMES_DEBUG theoradecd theoradec_d theoradecD theoradec_D)
set(THEORAENC_NAMES theoraenc libtheoraenc)
set(THEORAENC_NAMES_DEBUG theoraencd theoraenc_d theoraencD theoraenc_D)

foreach(search ${_THEORA_SEARCHES})
  find_path(THEORA_INCLUDE_DIR NAMES theora/theora.h ${${search}} PATH_SUFFIXES include)
endforeach()

# XXX: not sure if this is right. Check on macOS
if(${THEORA_INCLUDE_DIR} MATCHES ".framework")
    set(_components THEORA)
else()
    set(_components THEORA THEORADEC THEORAENC)
endif()

foreach(_component ${_components})
  # Allow ${_component}_LIBRARY to be set manually, as the location of the
  # corresponding library
  if(NOT ${_component}_LIBRARY)
    foreach(search ${_THEORA_SEARCHES})
      find_library(${_component}_LIBRARY_RELEASE NAMES ${${_component}_NAMES} ${${search}} PATH_SUFFIXES lib)
      find_library(${_component}_LIBRARY_DEBUG NAMES ${${_component}_NAMES_DEBUG} ${${search}} PATH_SUFFIXES lib)
    endforeach()

    include(SelectLibraryConfigurations)
    select_library_configurations(${_component})
  endif()
endforeach()

# XXX: not sure if this is right. Check on macOS
if(${THEORA_INCLUDE_DIR} MATCHES ".framework" AND THEORA_FOUND)
    set(THEORADEC_FOUND TRUE)
    set(THEORADEC_LIBRARY ${THEORA_LIBRARY})
    set(THEORAENC_FOUND TRUE)
    set(THEORAENC_LIBRARY ${THEORA_LIBRARY})
endif()

unset(THEORA_NAMES)
unset(THEORA_NAMES_DEBUG)
unset(THEORADEC_NAMES)
unset(THEORADEC_NAMES_DEBUG)
unset(THEORAENC_NAMES)
unset(THEORAENC_NAMES_DEBUG)

mark_as_advanced(THEORA_INCLUDE_DIR THEORA_LIBRARY THEORADEC_LIBRARY THEORAENC_LIBRARY)

if(THEORA_FOUND)
    set(Theora_Theora_FOUND true)
endif()

if(THEORADEC_FOUND)
    set(Theora_TheoraDec_FOUND true)
endif()

if(THEORAENC_FOUND)
    set(Theora_TheoraEnc_FOUND true)
endif()

# handle the QUIETLY and REQUIRED argument and set THEORA_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Theora REQUIRED_VARS THEORA_LIBRARY THEORA_INCLUDE_DIR HANDLE_COMPONENTS)

unset(Theora_Theora_FOUND)
unset(Theora_TheoraDec_FOUND)
unset(Theora_TheoraEnc_FOUND)

if(THEORA_FOUND)
    set(THEORA_INCLUDE_DIRS ${THEORA_INCLUDE_DIR})

    set(THEORA_LIBRARIES ${THEORA_LIBRARY})
    if(THEORADEC_FOUND)
        list(APPEND THEORA_LIBRARIES ${THEORADEC_LIBRARY})
    endif()
    if(THEORAENC_FOUND)
        list(APPEND THEORA_LIBRARIES ${THEORAENC_LIBRARY})
    endif()

    if(NOT TARGET Theora::Theora)
      add_library(Theora::Theora UNKNOWN IMPORTED)

      set_target_properties(Theora::Theora PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${THEORA_INCLUDE_DIRS}")

      if(THEORA_LIBRARY_RELEASE)
        set_property(TARGET Theora::Theora APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(Theora::Theora PROPERTIES
          IMPORTED_LOCATION_RELEASE "${THEORA_LIBRARY_RELEASE}")
      endif()

      if(THEORA_LIBRARY_DEBUG)
        set_property(TARGET Theora::Theora APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(Theora::Theora PROPERTIES
          IMPORTED_LOCATION_DEBUG "${THEORA_LIBRARY_DEBUG}")
      endif()

      if(NOT THEORA_LIBRARY_RELEASE AND NOT THEORA_LIBRARY_DEBUG)
        set_property(TARGET Theora::Theora APPEND PROPERTY
          IMPORTED_LOCATION "${THEORA_LIBRARY}")
      endif()
    endif()

    if(NOT TARGET Theora::TheoraDec)
      add_library(Theora::TheoraDec UNKNOWN IMPORTED)

      set_target_properties(Theora::TheoraDec PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${THEORADEC_INCLUDE_DIRS}")

      if(THEORADEC_LIBRARY_RELEASE)
        set_property(TARGET Theora::TheoraDec APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(Theora::TheoraDec PROPERTIES
          IMPORTED_LOCATION_RELEASE "${THEORADEC_LIBRARY_RELEASE}")
      endif()

      if(THEORADEC_LIBRARY_DEBUG)
        set_property(TARGET Theora::TheoraDec APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(Theora::TheoraDec PROPERTIES
          IMPORTED_LOCATION_DEBUG "${THEORADEC_LIBRARY_DEBUG}")
      endif()

      if(NOT THEORADEC_LIBRARY_RELEASE AND NOT THEORADEC_LIBRARY_DEBUG)
        set_property(TARGET Theora::TheoraDec APPEND PROPERTY
          IMPORTED_LOCATION "${THEORADEC_LIBRARY}")
      endif()

      add_dependencies(Theora::Theora Theora::TheoraEnc)
    endif()

    if(NOT TARGET Theora::TheoraEnc)
      add_library(Theora::TheoraEnc UNKNOWN IMPORTED)

      set_target_properties(Theora::TheoraEnc PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${THEORAENC_INCLUDE_DIRS}")

      if(THEORAENC_LIBRARY_RELEASE)
        set_property(TARGET Theora::TheoraEnc APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(Theora::TheoraEnc PROPERTIES
          IMPORTED_LOCATION_RELEASE "${THEORAENC_LIBRARY_RELEASE}")
      endif()

      if(THEORAENC_LIBRARY_DEBUG)
        set_property(TARGET Theora::TheoraEnc APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(Theora::TheoraEnc PROPERTIES
          IMPORTED_LOCATION_DEBUG "${THEORAENC_LIBRARY_DEBUG}")
      endif()

      if(NOT THEORAENC_LIBRARY_RELEASE AND NOT THEORAENC_LIBRARY_DEBUG)
        set_property(TARGET Theora::TheoraEnc APPEND PROPERTY
          IMPORTED_LOCATION "${THEORAENC_LIBRARY}")
      endif()

      add_dependencies(Theora::Theora Theora::TheoraEnc)
    endif()
endif()
