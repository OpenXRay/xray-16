#https://raw.githubusercontent.com/freeorion/freeorion/master/cmake/FindSDL.cmake
#.rst:
# FindSDL
# -------
#
# Find the native SDL includes and library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` targets
#
# ::
#
#   ``SDL::SDL``      - The SDL library.
#   ``SDL::main``     - The SDLmain library.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   SDL_INCLUDE_DIRS  - where to find SDL.h, etc.
#   SDL_LIBRARIES     - List of librarie when using SDL
#   SDL_VERSION       - SDL version from SDL_version.hpp
#   SDL_MAJOR_VERSION - SDL major version number (X in X.y.z)
#   SDL_MINOR_VERSION - SDL minor version number (Y in x.Y.z)
#   SDL_PATCH_VERSION - SDL patch version number (Z in x.y.Z)
#   SDL_FOUND         - True if SDL found.
#
# Controls
# ^^^^^^^^
#
# ::
#
#   SDL_NO_MAIN       - If set to true the SDLmain library is skipped
#                       from the SDL_LIBRARIES to let the application
#                       handle the various entry point variants used
#                       by different operating systems.
#
# Hints
# ^^^^^
#
# A user may set ``SDL_ROOT`` to a SDL installation root to tell this
# module where to look.
#
# On OSX, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of SDL2_LIBRARY to
# override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.

#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)


set(_SDL_SEARCHES)

# Search SDL_ROOT first when is set.
if(SDL_ROOT)
  set(_SDL_SEARCH_ROOT PATH ${SDL_ROOT} NO_DEFAULT_PATH)
  list(APPEND _SDL_SEARCHES _OGG_SEARCH_ROOT)
endif()

# Normal search.
set(_SDL_SEARCH_NORMAL
  PATH ""
  )
list(APPEND _SDL_SEARCHES _SDL_SEARCH_NORMAL)

foreach(search ${_SDL_SEARCHES})
  find_path(SDL_INCLUDE_DIR NAMES SDL.h ${${search}} PATH_SUFFIXES SDL2)
endforeach()

if(SDL_INCLUDE_DIR)
    file(STRINGS "${SDL_INCLUDE_DIR}/SDL_version.h" _SDL_VERSION_HPP_CONTENTS REGEX "#define SDL_((MAJOR|MINOR)_VERSION|PATCHLEVEL)")
    foreach(v MAJOR_VERSION MINOR_VERSION PATCHLEVEL)
        if("${_SDL_VERSION_HPP_CONTENTS}" MATCHES "#define SDL_${v} +([0-9]+)")
            set(SDL_${v} "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    set(SDL_PATCH_VERSION ${SDL_PATCHLEVEL})
    unset(SDL_PATCHLEVEL)
    set(SDL_VERSION "${SDL_MAJOR_VERSION}.${SDL_MINOR_VERSION}.${SDL_PATCH_VERSION}")
endif()

# Allow SDL_LIBRARY to be set manually, as the location of the
# SDL library
if(NOT SDL_LIBRARY)
  foreach(search ${_SDL_SEARCHES})
    find_library(SDL_LIBRARY NAMES SDL2 ${${search}} PATH_SUFFIXES lib64 lib)
  endforeach()
endif()

if(NOT SDL_NO_MAIN AND NOT ${SDL_INCLUDE_DIR} MATCHES ".framework")
  foreach(search ${_SDL_SEARCHES})
    # Non-OS X framework versions expect you to also dynamically link to
    # SDL2main. This is mainly for Windows and OS X. Other (Unix) platforms
    # seem to provide SDL2main for compatibility even though they don't
    # necessarily need it.
    find_library(SDL_MAIN_LIBRARY NAMES SDL2main ${${search}} PATH_SUFFIXES lib64 lib)
  endforeach()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL REQUIRED_VARS SDL_LIBRARY SDL_INCLUDE_DIR
                                      VERSION_VAR SDL_VERSION)

if(SDL_FOUND)
    set(SDL_INCLUDE_DIRS ${SDL_INCLUDE_DIR})

    if(NOT SDL_LIBRARIES)
        set(SDL_LIBRARIES ${SDL_LIBRARY})

        # For SDL2main
        if(NOT SDL_NO_MAIN AND SDL_MAIN_LIBRARY)
            set(SDL_LIBRARIES ${SDL_MAIN_LIBRARY} ${SDL_LIBRARIES})
        endif()

        if(APPLE)
            # For OS X, SDL2 uses Cocoa as a backend so it must link to Cocoa.
            set(SDL_LIBRARIES ${SDL_LIBRARIES} "-framework Cocoa")
        endif()
    endif()

    if(NOT TARGET SDL::SDL)
        add_library(SDL::SDL UNKNOWN IMPORTED)
        set_target_properties(SDL::SDL PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SDL_INCLUDE_DIRS}")

        if(SDL_LIBRARY)
            set_property(TARGET SDL::SDL APPEND PROPERTY
                IMPORTED_LOCATION "${SDL_LIBRARY}")
        endif()
    endif()

    if(NOT TARGET SDL::main AND SDL_MAIN_LIBRARY)
        add_library(SDL::main UNKNOWN IMPORTED)

        if(SDL_MAIN_LIBRARY)
            set_property(TARGET SDL::main APPEND PROPERTY
                IMPORTED_LOCATION "${SDL_MAIN_LIBRARY}")
        endif()


        add_dependencies(SDL::SDL SDL::main)
    endif()
endif()
