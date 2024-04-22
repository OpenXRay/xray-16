function(restore_from_nuget)
    message(STATUS "Generator is set to ${CMAKE_GENERATOR} - restoring NuGet dependencies...")
    find_program(NUGET nuget)
    if(NOT NUGET)
        message(FATAL "Cannot find nuget. Please install it using: winget install -e --id Microsoft.NuGet")
    endif()
    FILE(GLOB_RECURSE NUGET_SOURCES "${CMAKE_SOURCE_DIR}/packages.config")
    foreach(config_path IN LISTS NUGET_SOURCES )
        message(STATUS "Restoring NuGet dependencies from ${config_path}")
        execute_process(COMMAND
            ${NUGET} restore ${config_path} -SolutionDirectory ${CMAKE_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endforeach()

    SET(NUGET_PACKAGES_LOCATION "${CMAKE_BINARY_DIR}/packages")

    SET(NUGET_SDL_PACKAGE_LOCATION "${NUGET_PACKAGES_LOCATION}/sdl2.nuget.2.28.5/build/native/")
    add_library(SDL2::SDL2 UNKNOWN IMPORTED)
    set_target_properties(
        SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${NUGET_SDL_PACKAGE_LOCATION}/lib/${CMAKE_GENERATOR_PLATFORM}/"
        INTERFACE_INCLUDE_DIRECTORIES "${NUGET_SDL_PACKAGE_LOCATION}/include/"
    )

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(NUGET_D3DX_BUILD_TYPE "debug")
    else()
        set(NUGET_D3DX_BUILD_TYPE "release")
    endif()
    SET(NUGET_D3DX_PACKAGE_LOCATION "${NUGET_PACKAGES_LOCATION}/Microsoft.DXSDK.D3DX.9.29.952.8/build/native/")
    add_library(Microsoft::DXSDK::D3DX UNKNOWN IMPORTED)
    # TODO: Fix x86
    set_target_properties(
        Microsoft::DXSDK::D3DX PROPERTIES
        IMPORTED_LOCATION "${NUGET_D3DX_PACKAGE_LOCATION}/${NUGET_D3DX_BUILD_TYPE}/bin/${CMAKE_GENERATOR_PLATFORM}/"
        IMPORTED_IMPLIB "${NUGET_D3DX_PACKAGE_LOCATION}/${NUGET_D3DX_BUILD_TYPE}/lib/${CMAKE_GENERATOR_PLATFORM}/"
        INTERFACE_INCLUDE_DIRECTORIES "${NUGET_D3DX_PACKAGE_LOCATION}/include/"
    )
endfunction()
