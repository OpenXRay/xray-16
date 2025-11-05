include_guard()

message(VERBOSE "CMAKE_VERSION: ${CMAKE_VERSION}")

cmake_policy(SET CMP0138 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0138.html
cmake_policy(SET CMP0149 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0149.html
cmake_policy(SET CMP0162 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0162.html
cmake_policy(SET CMP0164 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0164.html

include(XRay.Configurations)

include(utils)

if (CMAKE_GENERATOR_PLATFORM AND NOT WIN32)
    message(WARNING
        "Generator '${CMAKE_GENERATOR}' ignores platform '${CMAKE_GENERATOR_PLATFORM}'. "
        "Clearing CMAKE_GENERATOR_PLATFORM to avoid configuration failures."
    )
    unset(CMAKE_GENERATOR_PLATFORM CACHE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "ReleaseMasterGold")
    set(BUILD_SHARED_LIBS_DEFAULT_VALUE OFF)
else()
    set(BUILD_SHARED_LIBS_DEFAULT_VALUE ON)
endif()
