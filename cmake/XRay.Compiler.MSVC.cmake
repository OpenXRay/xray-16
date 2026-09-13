include_guard()

# Clear predefined flags which we going to define ourselves
# Exceptions
string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

# Debug information format
string(REGEX REPLACE "/Z(7|i|I)" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})

# Debug information
string(REGEX REPLACE "/DEBUG:(NONE|FULL|FASTLINK)" "" CMAKE_STATIC_LINKER_FLAGS ${CMAKE_STATIC_LINKER_FLAGS})
string(REGEX REPLACE "/DEBUG:(NONE|FULL|FASTLINK)" "" CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
string(REGEX REPLACE "/DEBUG:(NONE|FULL|FASTLINK)" "" CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})

# Incremental LTCG
string(REGEX REPLACE "/LTCG:INCREMENTAL" "" CMAKE_STATIC_LINKER_FLAGS ${CMAKE_STATIC_LINKER_FLAGS})
string(REGEX REPLACE "/LTCG:INCREMENTAL" "" CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
string(REGEX REPLACE "/LTCG:INCREMENTAL" "" CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})

# Set properties:
set(CMAKE_VS_USE_DEBUG_LIBRARIES $<CONFIG:Debug>)

# The MSVC compiler settings:
add_compile_options(
    /GS-         # Disable buffer security check. XXX: consider to enable in Debug
    /MP          # Enable multi-process
    /Gy          # Function-level linking
    /fp:fast     # Fast floating-point model
    /Zi          # Use PDB debug information format
    /Zf          # Faster PDB generation

    # C++ standard conformance:
    /permissive- # C++ standard conformance
    /Zc:inline   # Ensure C++ standard conformance for inlines, remove unreferenced COMDATs

    # Optimization:
    $<$<CONFIG:Release,ReleaseMasterGold>:/O2>  # Maximize speed
    $<$<CONFIG:Release,ReleaseMasterGold>:/Ot>  # Favor speed
    $<$<CONFIG:Release,ReleaseMasterGold>:/Ob2> # Inline function expansion: any suitable
    $<$<CONFIG:Release,ReleaseMasterGold>:/Oi>  # Generate intrinsic functions

    # Enable standard C++ exceptions everywhere except ReleaseMasterGold
    $<$<NOT:$<CONFIG:ReleaseMasterGold>>:/EHsc>

    /FC # Full paths in diagnostic messages
)

# Disable MS STL exceptions on ReleaseMasterGold
add_compile_definitions($<$<CONFIG:ReleaseMasterGold>:_HAS_EXCEPTIONS=0>)

# The MSVC linker settings:
add_link_options(
    /LARGEADDRESSAWARE # Application can handle addresses larger than 2 GBs
    /DEBUG:FULL        # Always generate full debug information

    # Optimizations:
    $<$<CONFIG:Release,ReleaseMasterGold>:/OPT:REF>  # Remove unused references
    $<$<CONFIG:ReleaseMasterGold>:/OPT:ICF>          # COMDAT folding, still disabled on Release for better debugging
    $<$<CONFIG:Release>:/LTCG:INCREMENTAL>           # Incremental LTCG on Release
    $<$<CONFIG:ReleaseMasterGold>:/LTCG>             # Full LTCG on ReleaseMasterGold
)

set(XRAY_ENABLE_WARNINGS
    /WX # Treat warnings as errors
    /W2 # Enable level 2 warnings

    # Disable specific warnings:
    /wd4201 # nonstandard extension used : nameless struct/union
    /wd4251 # class 'x' needs to have dll-interface to be used by clients of class 'y'
    /wd4275 # non dll-interface class 'x' used as base for dll-interface class 'y'
    /wd4530 # C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
)

set(XRAY_DISABLE_WARNINGS /W0)

if (CMAKE_VS_PLATFORM_NAME)
    set(XRAY_PLATFORM ${CMAKE_VS_PLATFORM_NAME})
    string(REPLACE "Win32" "x86" XRAY_PLATFORM ${XRAY_PLATFORM})
else()
    set(XRAY_PLATFORM ${CMAKE_SYSTEM_PROCESSOR})
    string(REPLACE "AMD64" "x64" XRAY_PLATFORM ${XRAY_PLATFORM})
endif()

set(XRAY_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdk" CACHE PATH "Path to the bundled Windows OpenXRay dependencies")
set(XRAY_SDK_INCLUDE_DIR "${XRAY_SDK_DIR}/include")
set(XRAY_SDK_LIBRARY_DIR "${XRAY_SDK_DIR}/libraries/${XRAY_PLATFORM}")
set(XRAY_SDK_BINARY_DIR "${XRAY_SDK_DIR}/binaries/${XRAY_PLATFORM}")

list(APPEND CMAKE_PREFIX_PATH "${XRAY_SDK_DIR}")
list(APPEND CMAKE_LIBRARY_PATH "${XRAY_SDK_LIBRARY_DIR}")

install(DIRECTORY "${XRAY_SDK_BINARY_DIR}/" TYPE BIN COMPONENT OpenXRay)

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

if (MEMORY_ALLOCATOR STREQUAL "mimalloc" AND NOT TARGET mimalloc)
    xray_define_imported_library(mimalloc
        PATH "${XRAY_SDK_LIBRARY_DIR}/mimalloc-static.lib"
        DEBUG_PATH "${XRAY_SDK_LIBRARY_DIR}/mimalloc-static-debug.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}/mimalloc"
    )
endif()

if (NOT TARGET SDL2::SDL2)
    xray_define_imported_library(SDL2::SDL2
        PATH "${XRAY_SDK_LIBRARY_DIR}/SDL2.lib"
        INCLUDE_DIRS "${XRAY_SDK_INCLUDE_DIR}/SDL2"
    )
endif()
