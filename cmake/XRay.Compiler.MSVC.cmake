include_guard()

# The MSVC compiler settings:
# Set properties:
set(CMAKE_VS_USE_DEBUG_LIBRARIES "$<CONFIG:Debug>")

# Clear predefined flags which we going to define ourselves
string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # exceptions
string(REGEX REPLACE "/Z(7|i|I)" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG}) # debug information format

# Enable standard C++ exceptions everywhere except ReleaseMasterGold
add_compile_options($<$<NOT:$<CONFIG:ReleaseMasterGold>>:/EHsc>)

# Disable MS STL exceptions on ReleaseMasterGold
add_compile_definitions($<$<CONFIG:ReleaseMasterGold>:_HAS_EXCEPTIONS=0>)

# Enable debug information for all configurations and allow parallel PDB writes
add_compile_options(
    /Zi
    /FS
)

# Enable SSE2 for 32-bit build
# (on x64 it's always enabled and produces error if try to to enable it)
add_compile_options($<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>:/arch:SSE2>)

# Disable specific warnings
add_compile_options(
    /wd4201 # nonstandard extension used : nameless struct/union
    /wd4251 # class 'x' needs to have dll-interface to be used by clients of class 'y'
    /wd4275 # non dll-interface class 'x' used as base for dll-interface class 'y'
    /wd4458 # declaration hides class member
)

# The MSVC linker settings:
add_link_options("/LARGEADDRESSAWARE")

set(XRAY_DISABLE_WARNINGS "/w")

set(XRAY_ENABLE_WARNINGS
    /W3
)

add_compile_definitions(
    MSVC
    dSINGLE
    WIN32
    USE_OPENSSL
    IMGUI_DISABLE_OBSOLETE_KEYIO
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    IMGUI_DEFINE_MATH_OPERATORS
    _CRT_SECURE_NO_WARNINGS
)

set(_xray_vs_platform "")
if (DEFINED CMAKE_VS_PLATFORM_NAME AND CMAKE_VS_PLATFORM_NAME)
    set(_xray_vs_platform "${CMAKE_VS_PLATFORM_NAME}")
elseif (CMAKE_GENERATOR_PLATFORM)
    set(_xray_vs_platform "${CMAKE_GENERATOR_PLATFORM}")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_xray_vs_platform "x64")
else()
    set(_xray_vs_platform "x86")
endif()

string(TOLOWER "${_xray_vs_platform}" _xray_vs_platform_lower)
if (_xray_vs_platform_lower STREQUAL "win32" OR _xray_vs_platform_lower STREQUAL "x86")
    set(XRAY_SDK_PLATFORM "x86")
elseif (_xray_vs_platform_lower STREQUAL "x64" OR _xray_vs_platform_lower STREQUAL "amd64")
    set(XRAY_SDK_PLATFORM "x64")
elseif (_xray_vs_platform_lower STREQUAL "arm64")
    set(XRAY_SDK_PLATFORM "ARM64")
else()
    set(XRAY_SDK_PLATFORM "${_xray_vs_platform}")
endif()
unset(_xray_vs_platform_lower)

set(XRAY_SDK_DIR "${CMAKE_SOURCE_DIR}/sdk" CACHE PATH "Path to the bundled X-Ray Windows SDK")
set(XRAY_SDK_INCLUDE_DIR "${XRAY_SDK_DIR}/include")
set(XRAY_SDK_LIBRARY_DIR "${XRAY_SDK_DIR}/libraries/${XRAY_SDK_PLATFORM}")
set(XRAY_SDK_BINARY_DIR "${XRAY_SDK_DIR}/binaries/${XRAY_SDK_PLATFORM}")

foreach(_xray_required_dir XRAY_SDK_DIR XRAY_SDK_INCLUDE_DIR XRAY_SDK_LIBRARY_DIR)
    if (NOT EXISTS "${${_xray_required_dir}}")
        message(FATAL_ERROR "${_xray_required_dir} not found at '${${_xray_required_dir}}'. Please point XRAY_SDK_DIR to a valid SDK installation.")
    endif()
endforeach()

set(MEMORY_ALLOCATOR "mimalloc" CACHE STRING "Use specific memory allocator (mimalloc/standard)")
set_property(CACHE MEMORY_ALLOCATOR PROPERTY STRINGS "mimalloc" "standard")

function(xray_define_imported_library target)
    cmake_parse_arguments(ARG "" "PATH;DEBUG_PATH;INCLUDE_DIRS" "" ${ARGN})
    if (TARGET ${target})
        return()
    endif()

    if (NOT ARG_PATH)
        message(FATAL_ERROR "xray_define_imported_library requires PATH for target ${target}")
    endif()

    if (NOT EXISTS "${ARG_PATH}")
        message(FATAL_ERROR "Expected library for ${target} at '${ARG_PATH}' not found. Update XRAY_SDK_DIR if you use a custom SDK location.")
    endif()

    add_library(${target} UNKNOWN IMPORTED)
    set_target_properties(${target} PROPERTIES IMPORTED_LOCATION "${ARG_PATH}")

    if (ARG_DEBUG_PATH)
        if (NOT EXISTS "${ARG_DEBUG_PATH}")
            message(FATAL_ERROR "Debug library for ${target} at '${ARG_DEBUG_PATH}' not found.")
        endif()
        set_target_properties(${target} PROPERTIES IMPORTED_LOCATION_DEBUG "${ARG_DEBUG_PATH}")
    endif()

    foreach(_cfg IN ITEMS Mixed Release ReleaseMasterGold)
        string(TOUPPER "${_cfg}" _cfg_upper)
        set_target_properties(${target} PROPERTIES "IMPORTED_LOCATION_${_cfg_upper}" "${ARG_PATH}")
    endforeach()

    if (ARG_INCLUDE_DIRS)
        set_target_properties(${target} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${ARG_INCLUDE_DIRS}")
    endif()
endfunction()

if (NOT TARGET JPEG::JPEG)
    xray_define_imported_library(JPEG::JPEG
        PATH "${XRAY_SDK_LIBRARY_DIR}/jpeg-static.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}"
    )
endif()

if (NOT TARGET LZO::LZO)
    xray_define_imported_library(LZO::LZO
        PATH "${XRAY_SDK_LIBRARY_DIR}/lzo.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}"
    )
endif()

if (NOT TARGET Ogg::Ogg)
    xray_define_imported_library(Ogg::Ogg
        PATH "${XRAY_SDK_LIBRARY_DIR}/libogg_static.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}"
    )
endif()

if (NOT TARGET Vorbis::Vorbis)
    xray_define_imported_library(Vorbis::Vorbis
        PATH "${XRAY_SDK_LIBRARY_DIR}/libvorbis_static.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}"
    )
endif()

if (NOT TARGET Vorbis::VorbisFile)
    xray_define_imported_library(Vorbis::VorbisFile
        PATH "${XRAY_SDK_LIBRARY_DIR}/libvorbisfile.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}"
    )
endif()

if (NOT TARGET Theora::Theora)
    xray_define_imported_library(Theora::Theora
        PATH "${XRAY_SDK_LIBRARY_DIR}/libtheora_static.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}"
    )
endif()

if (NOT TARGET OpenAL::OpenAL)
    xray_define_imported_library(OpenAL::OpenAL
        PATH "${XRAY_SDK_LIBRARY_DIR}/OpenAL32.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}/AL"
    )
endif()

if (NOT TARGET OpenSSL::Crypto)
    xray_define_imported_library(OpenSSL::Crypto
        PATH "${XRAY_SDK_LIBRARY_DIR}/libcrypto.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}"
    )
endif()

if (MEMORY_ALLOCATOR STREQUAL "mimalloc" AND NOT TARGET mimalloc)
    xray_define_imported_library(mimalloc
        PATH "${XRAY_SDK_LIBRARY_DIR}/mimalloc-static.lib"
        DEBUG_PATH "${XRAY_SDK_LIBRARY_DIR}/mimalloc-static-debug.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}/mimalloc"
    )
endif()

if (NOT TARGET xray::BugTrap)
    xray_define_imported_library(xray::BugTrap
        PATH "${XRAY_SDK_LIBRARY_DIR}/BugTrap.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}"
    )
endif()

if (NOT TARGET SDL2::SDL2)
    xray_define_imported_library(SDL2::SDL2
        PATH "${XRAY_SDK_LIBRARY_DIR}/SDL2.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}/SDL2"
    )
endif()

if (NOT TARGET SDL2::SDL2main)
    xray_define_imported_library(SDL2::SDL2main
        PATH "${XRAY_SDK_LIBRARY_DIR}/SDL2main.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}/SDL2"
    )
endif()
