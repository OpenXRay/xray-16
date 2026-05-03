include_guard()

message(VERBOSE "CMAKE_VERSION: ${CMAKE_VERSION}")

cmake_policy(SET CMP0138 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0138.html
cmake_policy(SET CMP0149 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0149.html
cmake_policy(SET CMP0162 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0162.html
cmake_policy(SET CMP0164 NEW) # https://cmake.org/cmake/help/latest/policy/CMP0164.html

include(XRay.Configurations)

include(utils)

get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (_is_multi_config OR CMAKE_BUILD_TYPE STREQUAL "ReleaseMasterGold")
    set(BUILD_SHARED_LIBS_DEFAULT_VALUE OFF)
else()
    set(BUILD_SHARED_LIBS_DEFAULT_VALUE ON)
endif()
