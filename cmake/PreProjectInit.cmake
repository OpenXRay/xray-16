include_guard()

message(VERBOSE "CMAKE_VERSION: ${CMAKE_VERSION}")

cmake_policy(SET CMP0129 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0129.html

if (POLICY CMP0138)
    cmake_policy(SET CMP0138 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0138.html
endif()

if (POLICY CMP0149)
    cmake_policy(SET CMP0149 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0149.html
endif()

if (POLICY CMP0162)
    cmake_policy(SET CMP0162 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0162.html
endif()

if (POLICY CMP0164)
    cmake_policy(SET CMP0164 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0164.html
endif()

include(XRay.Configurations)

include(utils)

if (CMAKE_BUILD_TYPE STREQUAL "ReleaseMasterGold")
    set(BUILD_SHARED_LIBS_DEFAULT_VALUE OFF)
else()
    set(BUILD_SHARED_LIBS_DEFAULT_VALUE ON)
endif()
