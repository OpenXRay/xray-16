include_guard()

# Probe in a child directory so rejected packages do not leave imported targets
# named SDL2::SDL2 in the scope where the source build creates its own target.
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/SDL2Probe" "${CMAKE_BINARY_DIR}/sdl2-probe")
if (XRAY_SDL2_ACCEPTED)
    find_package(SDL2 2.0.18 REQUIRED)
    message(STATUS "Using installed native SDL ${SDL2_VERSION}")
    return()
endif()

function(xray_build_sdl2)
    include(FetchContent)
    if (POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()

    # Keep this version aligned with the Windows SDL2 NuGet packages.
    FetchContent_Declare(SDL2
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.4/SDL2-2.32.4.tar.gz
        URL_HASH SHA256=f15b478253e1ff6dac62257ded225ff4e7d0c5230204ac3450f1144ee806f934
    )

    # Limit these settings to SDL's build, including when the engine uses unity.
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(CMAKE_UNITY_BUILD OFF)
    set(SDL_SHARED ON)
    set(SDL_STATIC OFF)
    set(SDL_TEST OFF)
    set(SDL_TESTS OFF)
    set(SDL2_DISABLE_SDL2MAIN ON)
    set(SDL2_DISABLE_INSTALL ON)
    FetchContent_MakeAvailable(SDL2)
endfunction()

message(STATUS "Building native SDL 2.32.4 from source")
xray_build_sdl2()
