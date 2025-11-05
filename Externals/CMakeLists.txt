if (XRAY_USE_LUAJIT)
    add_subdirectory(LuaJIT-proj)
else()
    find_package(Lua51 REQUIRED)
endif()

add_subdirectory(luabind)
add_subdirectory(xrLuaFix)
add_subdirectory(OPCODE)
add_subdirectory(ode)

if (WIN32)
    add_subdirectory(DiscordGameSDK)
endif()

# Silence all warnings for external libraries
add_compile_options(${XRAY_DISABLE_WARNINGS})

add_subdirectory(GameSpy)
add_subdirectory(imgui-proj)

if(MSVC AND CMAKE_GENERATOR MATCHES "Visual Studio")
    set(ozz_build_samples OFF CACHE BOOL "Build ozz sample executables" FORCE)
    set(ozz_build_howtos OFF CACHE BOOL "Build ozz howtos" FORCE)
    set(ozz_build_tests OFF CACHE BOOL "Build ozz unit tests" FORCE)
endif()

add_subdirectory(ozz-animation)

# GLFW - Window and input library for Vulkan/OpenGL
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build GLFW example programs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build GLFW test programs" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build GLFW documentation" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Generate GLFW installation target" FORCE)
add_subdirectory(glfw)

# EnTT - Entity Component System library (header-only)
add_subdirectory(entt)

if (NOT TARGET xrLuabind)
    message(FATAL_ERROR
        "You probably have downloaded OpenXRay sources as ZIP archive, you can't do that. Use git to clone the repository.\n"
        "Read the build instructions: https://github.com/OpenXRay/xray-16/wiki"
    )
endif()
