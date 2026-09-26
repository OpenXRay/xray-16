cmake_minimum_required(VERSION 3.23)
project(ContactBodyEffectorLifetime LANGUAGES CXX)

if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This regression harness currently uses the native MSVC x64 Release build.")
endif()

get_filename_component(XRAY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
add_compile_options(/EHsc /fsanitize=address /Zi /Oy-)
add_link_options(/DEBUG)
add_compile_definitions(MSVC WIN32 dSINGLE XRAY_EXCEPTIONS=1 LUABIND_DYNAMIC_LINK NDEBUG)

file(STRINGS "${XRAY_ROOT}/src/xrPhysics/packages.config" sdl_package REGEX "id=\"sdl2.nuget\"")
string(REGEX MATCH "version=\"([^\"]+)\"" sdl_version "${sdl_package}")
set(sdl_include "${XRAY_ROOT}/src/packages/sdl2.nuget.${CMAKE_MATCH_1}/build/native/include")

include_directories("${XRAY_ROOT}/src" "${XRAY_ROOT}/src/xrPhysics"
    "${XRAY_ROOT}/sdk/include" "${XRAY_ROOT}/Externals"
    "${XRAY_ROOT}/Externals/ode/include" "${XRAY_ROOT}/Externals/LuaJIT/src"
    "${XRAY_ROOT}/Externals/luabind" "${XRAY_ROOT}/Externals/imgui"
    "${XRAY_ROOT}/Externals/tracy/public" "${sdl_include}")

foreach(name xrCore xrPhysics xrMaterialSystem ODE)
    add_library(${name} SHARED IMPORTED)
    set_target_properties(${name} PROPERTIES
        IMPORTED_IMPLIB "${XRAY_ROOT}/build/lib/x64/Release/${name}.lib"
        IMPORTED_LOCATION "${XRAY_ROOT}/bin/x64/Release/${name}.dll")
endforeach()

add_executable(ContactBodyEffectorLifetime ContactBodyEffectorLifetime.cpp
    "${XRAY_ROOT}/src/xrPhysics/PHContactBodyEffector.cpp")
target_link_libraries(ContactBodyEffectorLifetime PRIVATE xrCore xrPhysics xrMaterialSystem ODE)
