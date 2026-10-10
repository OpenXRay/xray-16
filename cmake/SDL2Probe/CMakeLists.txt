# Imported targets from an unsuitable package must remain local to this probe.
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL FALSE)
set(XRAY_SDL2_ACCEPTED FALSE PARENT_SCOPE)
find_package(SDL2 2.0.18 QUIET)

if (NOT SDL2_FOUND OR NOT TARGET SDL2::SDL2)
    return()
endif()

if (NOT DEFINED SDL2_VERSION OR SDL2_VERSION VERSION_LESS 2.0.18 OR SDL2_VERSION VERSION_GREATER_EQUAL 3.0.0)
    message(STATUS "Rejecting installed SDL version '${SDL2_VERSION}': expected 2.0.18 <= version < 3.0.0")
    return()
endif()

# SDL2-compat advertises an SDL2 version but dynamically loads SDL3. Inspect the
# library without loading it, so the check also works when cross-compiling.
get_target_property(sdl2_configurations SDL2::SDL2 IMPORTED_CONFIGURATIONS)
set(sdl2_location_properties IMPORTED_LOCATION)
foreach (configuration IN LISTS sdl2_configurations)
    string(TOUPPER "${configuration}" configuration)
    list(APPEND sdl2_location_properties "IMPORTED_LOCATION_${configuration}")
endforeach()

set(sdl2_library_verified FALSE)
foreach (property IN LISTS sdl2_location_properties)
    get_target_property(library SDL2::SDL2 "${property}")
    if (EXISTS "${library}" AND NOT IS_DIRECTORY "${library}")
        file(STRINGS "${library}" sdl2_compat_marker LIMIT_COUNT 1 REGEX "SDL2COMPAT_")
        if (sdl2_compat_marker)
            message(STATUS "Rejecting SDL2-compat: ${library} requires SDL3")
            return()
        endif()
        set(sdl2_library_verified TRUE)
    endif()
endforeach()

if (sdl2_library_verified)
    set(XRAY_SDL2_ACCEPTED TRUE PARENT_SCOPE)
else()
    message(STATUS "Could not verify the installed SDL2 library; using native SDL2 source")
endif()
