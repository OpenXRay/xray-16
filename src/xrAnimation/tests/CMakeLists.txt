cmake_minimum_required(VERSION 3.8)

set(_xrAnimation_skip_tests FALSE)
if(NOT CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" _xrAnimation_effective_config)
    if(_xrAnimation_effective_config STREQUAL "RELEASEMASTERGOLD")
        set(_xrAnimation_skip_tests TRUE)
    endif()
endif()

if(_xrAnimation_skip_tests)
    message(STATUS "Skipping xrAnimation tests for ReleaseMasterGold configuration")
    return()
endif()

enable_testing()

set(OZZ_ROOT ${CMAKE_SOURCE_DIR}/Externals/ozz-animation)
set(OZZ_SAMPLES_ROOT ${OZZ_ROOT}/samples)

if (NOT TARGET gtest)
    add_subdirectory(${OZZ_ROOT}/extern/gtest ${CMAKE_BINARY_DIR}/ozz_gtest)
endif()

# Ensure GoogleTest fused headers are reachable when the ozz gtest target is
# brought in via our super-build. The upstream CMakeLists.txt assumes
# PROJECT_SOURCE_DIR points at the ozz tree, which is not the case here.
if (TARGET gtest)
    target_include_directories(gtest PUBLIC
        $<BUILD_INTERFACE:${OZZ_ROOT}/extern/gtest/fused-src>)
endif()

set_property(TARGET gtest PROPERTY EXCLUDE_FROM_DEFAULT_BUILD_RELEASEMASTERGOLD ON)

add_executable(xrAnimation_converter_tests
    test_converter.cpp
    test_startup_conversion.cpp
    ozz_kinematics_test_main.cpp)

target_include_directories(xrAnimation_converter_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${OZZ_ROOT}/include
    ${OZZ_SAMPLES_ROOT}
    ${CMAKE_SOURCE_DIR}/Externals/tracy/public)

target_sources(xrAnimation_converter_tests PRIVATE
    ${OZZ_SAMPLES_ROOT}/framework/mesh.cc)

target_link_libraries(xrAnimation_converter_tests PRIVATE
    xrAnimation
    ozz_animation
    ozz_animation_offline
    ozz_base
    xrEngine
    xrScriptEngine
    gtest)

set_property(TARGET xrAnimation_converter_tests PROPERTY EXCLUDE_FROM_DEFAULT_BUILD_RELEASEMASTERGOLD ON)

target_compile_features(xrAnimation_converter_tests PRIVATE cxx_std_17)

target_compile_definitions(xrAnimation_converter_tests PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

set_target_properties(xrAnimation_converter_tests PROPERTIES
    FOLDER "xrAnimation/Tests")

add_test(NAME xrAnimation_converter_tests COMMAND xrAnimation_converter_tests)

add_executable(ozz_kinematics_tests
    ozz_kinematics_parity_tests.cpp
    ozz_kinematics_test_main.cpp)

target_include_directories(ozz_kinematics_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/src
    ${OZZ_ROOT}/include
    ${CMAKE_SOURCE_DIR}/Externals/tracy/public)

target_sources(ozz_kinematics_tests PRIVATE
    ${OZZ_SAMPLES_ROOT}/framework/mesh.cc)

target_link_libraries(ozz_kinematics_tests PRIVATE
    xrAnimation
    xrMaterialSystem
    ozz_animation
    ozz_base
    gtest
    xrEngine
    xrScriptEngine)

set_property(TARGET ozz_kinematics_tests PROPERTY EXCLUDE_FROM_DEFAULT_BUILD_RELEASEMASTERGOLD ON)

target_compile_features(ozz_kinematics_tests PRIVATE cxx_std_17)

target_compile_definitions(ozz_kinematics_tests PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

set_target_properties(ozz_kinematics_tests PROPERTIES
    FOLDER "xrAnimation/Tests")

add_test(NAME ozz_kinematics_tests COMMAND ozz_kinematics_tests)

if(CMAKE_CONFIGURATION_TYPES)
    set_tests_properties(
        xrAnimation_converter_tests
        ozz_kinematics_tests
        PROPERTIES LABELS "skip_release_master_gold")
endif()
